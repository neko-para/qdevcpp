#include "editorinfo.h"
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <Qsci/qscilexercpp.h>
#include "global.h"
#include "compileconfig.h"
#include "ui_mainwindow.h"
#include "subprocess.h"

int EditorInfo::untitled_next;
QList<int> EditorInfo::untitled_rest;

void EditorInfo::modificationChanged(bool m) {
	modified = m;
	ui->SrcTab->setTabText(ui->SrcTab->indexOf(editor), generateTitle());
}

void EditorInfo::updateUndoRedoState() {
	ui->actionUndo->setEnabled(editor->isUndoAvailable());
	ui->actionRedo->setEnabled(editor->isRedoAvailable());
}

void EditorInfo::updateCopyCutState() {
	bool e = editor->selectedText().length();
	ui->actionCopy->setEnabled(e);
	ui->actionCut->setEnabled(e);
}

QsciLexerCPP* createLexer() {
	QsciLexerCPP* lexer = new QsciLexerCPP;
	lexer->setFont(QFont("ubuntu mono"));
	return lexer;
}

EditorInfo::EditorInfo(QsciScintilla *e, Ui::MainWindow *ui) : editor(e), ui(ui) {
	connect(e, &QsciScintilla::modificationChanged, this, &EditorInfo::modificationChanged);
	connect(e, &QsciScintilla::textChanged, this, &EditorInfo::updateUndoRedoState);
	connect(e, &QsciScintilla::selectionChanged, this, &EditorInfo::updateCopyCutState);
	connect(this, &EditorInfo::pathChange, [&](QString path) {
		if (shallSyntaxHighlight()) {
			if (!editor->lexer()) {
				editor->setLexer(createLexer());
			}
		} else {
			if (editor->lexer()) {
				editor->lexer()->deleteLater();
				editor->setLexer(0);
			}
		}
	});
}

EditorInfo::~EditorInfo()  {
	if (path[0] == '#') {
		untitled_rest.push_back(path.mid(1).toInt());
	}
	if (editor->lexer()) {
		editor->lexer()->deleteLater();
		editor->setLexer(0);
	}
	editor->deleteLater();
}

void EditorInfo::generateUntitled()  {
	int id;
	if (untitled_rest.size()) {
		std::sort(untitled_rest.begin(), untitled_rest.end());
		id = untitled_rest.takeFirst();
	} else {
		id = ++untitled_next;
	}
	path = QString("#%1").arg(id);
	emit pathChange(path);
}

QString EditorInfo::generateName() const {
	if (path[0] == '#') {
		return QString("未命名 %1").arg(path.mid(1));
	} else {
		return QFileInfo(path).fileName();
	}
}

bool EditorInfo::open(const QString& cpath)  {
	QFile file(cpath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		ui->compileLog->appendPlainText(QString("[错误]%1打开失败 - %2").arg(QFileInfo(cpath).fileName(), file.errorString()));;
		return false;
	}
	qint64 length = file.size();
	char* buffer = new char[length + 1];
	if (length == file.read(buffer, length)) {
		file.close();
		buffer[length] = 0;
		path = cpath;
		editor->setText(QString::fromUtf8(buffer));
		delete[] buffer;
		editor->setModified(false);
		emit pathChange(path);
		return true;
	} else {
		file.close();
		delete[] buffer;
		ui->compileLog->appendPlainText(QString("[错误]%1打开失败 - %2").arg(QFileInfo(cpath).fileName(), file.errorString()));
		return false;
	}
}

bool EditorInfo::write(const QString& cpath)  {
	auto buffer = editor->text().toUtf8();
	QFile file(cpath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
		ui->compileLog->appendPlainText(QString("[错误]%1保存失败 - %2").arg(QFileInfo(cpath).fileName(), file.errorString()));
		return false;
	}
	if (buffer.size() == file.write(buffer)) {
		file.close();
		if (path != cpath) {
			path = cpath;
			emit pathChange(path);
		}
		editor->setModified(false);
		return true;
	} else {
		file.close();
		ui->compileLog->appendPlainText(QString("[错误]%1保存失败 - %2").arg(QFileInfo(cpath).fileName(), file.errorString()));
		return false;
	}
}

bool EditorInfo::saveas()  {
	QString cpath = QFileDialog::getSaveFileName(editor, "qdevcpp - 另存为", "", "C源代码 (*.c);;C++源代码 (*.cpp *.cc *.cxx);;C/C++头文件 (*.h);;C++头文件 (*.hpp);;所有文件 (*.*)");
	if (cpath == "") {
		return false;
	}
	return write(cpath);
}

bool EditorInfo::save() {
	QString cpath = path;
	if (isUntitled()) {
		cpath = QFileDialog::getSaveFileName(editor, "qdevcpp - 保存", "", "C源代码 (*.c);;C++源代码 (*.cpp *.cc *.cxx);;C/C++头文件 (*.h);;C++头文件 (*.hpp);;所有文件 (*.*)");
		if (cpath == "") {
			return false;
		}
	}
	return write(cpath);
}

bool EditorInfo::askSave()  {
	if (!isUntitled() && isAtSavePoint()) {
		return true;
	}
	switch (QMessageBox::question(editor, "qdevcpp", QString("是否保存%1").arg(generateName()), QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel)) {
	case QMessageBox::Yes:
		return save();
	case QMessageBox::Cancel:
		return false;
	case QMessageBox::No:
		return true;
	default:
		// wtf
		return true;
	}
}

bool EditorInfo::shallSyntaxHighlight() const {
	return isUntitled() ? true : QStringList({"c", "cpp", "cxx", "cc", "h", "hpp"}).contains(QFileInfo(path).suffix());
}

static QTableWidgetItem* generateItem(const QString& s, int align = Qt::AlignCenter) {
	auto i = new QTableWidgetItem(s);
	i->setTextAlignment(align);
	return i;
}

bool EditorInfo::compile() const {
	QProcess proc;
	currentConfig->start(proc, path);
	ui->compileLog->appendPlainText(QString("[执行]%1").arg(currentConfig->gccPath));
	proc.setReadChannel(QProcess::StandardError);
	int retCode = -1;
	QEventLoop loop;
	QTimer timeout;
	timeout.setInterval(30 * 1000);
	timeout.setSingleShot(true);
	connect(&proc, QOverload<int>::of(&QProcess::finished), [&](int code) {
		retCode = code;
		loop.quit();
	});
	connect(&timeout, &QTimer::timeout, [&]() {
		proc.kill();
		loop.exit(1);
	});
	if (loop.exec()) {
		ui->compileLog->appendPlainText(QString("[错误]运行超时"));
		return false;
	} else {
		timeout.stop();
	}
	QString output = QString::fromUtf8(proc.readAll());
	ui->compileResult->clearContents();
	int crow = 0;
	for (QString err : output.split('\n')) {
		if (err.startsWith(path)) {
			QString row, col, info;
			err = err.mid(path.length() + 1); // IGNORE 'xxx.cpp:'
			QRegularExpression rc(R"(^([0-9]+):([0-9]+):)");
			auto match = rc.match(err);
			if (match.hasMatch()) {
				row = match.captured(1);
				col = match.captured(2);
				err = err.mid(match.captured(0).length());
			}
			info = err.mid(1); // ignore ' '
			ui->compileResult->insertRow(crow);

			ui->compileResult->setItem(crow, 0, generateItem(row));
			ui->compileResult->setItem(crow, 1, generateItem(col));
			ui->compileResult->setItem(crow, 2, generateItem(info, Qt::AlignVCenter));
			++crow;
		}
	}
	ui->compileLog->appendPlainText(QString("[返回]%1").arg(retCode));
	return !retCode;
}

void EditorInfo::run() {
	if (isUntitled() && !askSave()) {
		return;
	}
	QFileInfo si(path);
	QString exe = si.path() + QDir::separator() + si.baseName() + exeSuf;
	if (!QFileInfo(exe).exists()) {
		if (QMessageBox::Yes == QMessageBox::question(editor, "qdevcpp", "代码尚未编译，是否现在编译？", QMessageBox::Yes, QMessageBox::No)) {
//			compilerun();
		}
	}
	SubProcess* sp = new SubProcess(ui->compileLog, exe, editor);
	sp->start();
}
