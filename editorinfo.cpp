#include "editorinfo.h"
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <Qsci/qscilexercpp.h>
#include "global.h"
#include "compileconfig.h"
#include "mainwindow.h"
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

EditorInfo::EditorInfo(QsciScintilla *e, Ui::MainWindow* ui) : editor(e), ui(ui) {
	connect(e, &QsciScintilla::modificationChanged, this, &EditorInfo::modificationChanged);
	connect(e, &QsciScintilla::textChanged, this, &EditorInfo::updateUndoRedoState);
	connect(e, &QsciScintilla::selectionChanged, this, &EditorInfo::updateCopyCutState);
	connect(this, &EditorInfo::pathChange, [&](QString) {
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

	connect(this, &EditorInfo::pathChange, window, &MainWindow::updateWindowTitle);
	generateUntitled();
}

EditorInfo::~EditorInfo()  {
	if (path[0] == '#') {
		untitled_rest.push_back(path.mid(1).toInt());
	}
	if (editor->lexer()) {
		editor->lexer()->deleteLater();
		editor->setLexer(0);
	}
	window->info.remove(editor);
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
	QString cpath = QString("#%1").arg(id);
	QString ppath = path;
	path = cpath;
	emit pathChange(cpath, ppath);
}

QString EditorInfo::generateName() const {
	if (isUntitled()) {
		return QString("未命名 %1").arg(path.mid(1));
	} else {
		return QFileInfo(path).fileName();
	}
}

bool EditorInfo::open(const QString& cpath)  {
	QFile file(cpath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		ui->Log->appendPlainText(QString("[错误]%1打开失败 - %2").arg(QFileInfo(cpath).fileName(), file.errorString()));;
		return false;
	}
	qint64 length = file.size();
	char* buffer = new char[length + 1];
	if (length == file.read(buffer, length)) {
		file.close();
		buffer[length] = 0;
		QString ppath = path;
		path = cpath;
		emit pathChange(cpath, ppath);
		editor->setText(QString::fromUtf8(buffer));
		delete[] buffer;
		editor->setModified(false);
		whenOpen = QFileInfo(path).lastModified();
		return true;
	} else {
		file.close();
		delete[] buffer;
		ui->Log->appendPlainText(QString("[错误]%1打开失败 - %2").arg(QFileInfo(cpath).fileName(), file.errorString()));
		return false;
	}
}

bool EditorInfo::write(const QString& cpath)  {
	auto buffer = editor->text().toUtf8();
	QFile file(cpath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
		ui->Log->appendPlainText(QString("[错误]%1保存失败 - %2").arg(QFileInfo(cpath).fileName(), file.errorString()));
		return false;
	}
	if (buffer.size() == file.write(buffer)) {
		file.close();
		if (path != cpath) {
			QString ppath = path;
			path = cpath;
			emit pathChange(cpath, ppath);
		}
		editor->setModified(false);
		whenOpen = QFileInfo(path).lastModified();
		return true;
	} else {
		file.close();
		ui->Log->appendPlainText(QString("[错误]%1保存失败 - %2").arg(QFileInfo(cpath).fileName(), file.errorString()));
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
	proc.setEnvironment(QProcess::systemEnvironment() << "LANGUAGE=en_US.UTF-8");
	currentConfig->start(proc, path);
	ui->Log->appendPlainText(QString("[执行]%1").arg(currentConfig->gccPath));
	proc.setReadChannel(QProcess::StandardError);
	int retCode = -1;
	QEventLoop loop;
	QTimer timeout;
	timeout.setInterval(30 * 1000);
	timeout.setSingleShot(true);
	connect(&proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [&](int exitCode) {
		retCode = exitCode;
		loop.quit();
	});
	connect(&timeout, &QTimer::timeout, [&]() {
		proc.kill();
		loop.exit(1);
	});
	if (loop.exec()) {
		ui->Log->appendPlainText(QString("[错误]运行超时"));
		return false;
	} else {
		timeout.stop();
	}
	QString output = QString::fromUtf8(proc.readAll());
	if (output.length()) {
		ui->Log->appendPlainText(QString("[输出]%1").arg(output));
	}
	ui->compileResult->clearContents();
	ui->compileResult->setRowCount(0);
	int crow = 0;
	for (QString err : output.split('\n', QString::SkipEmptyParts)) {
		QRegularExpression pattern(R"(^([\s\S]+?)(?::([0-9]+)(?::([0-9]+))?)?: (?:(error|warning|note): )?([\s\S]+)$)");
		auto match = pattern.match(err);
		if (!match.hasMatch()) {
			continue;
		}
		ui->compileResult->insertRow(crow);
		ui->compileResult->setItem(crow, 0, generateItem(match.captured(1), Qt::AlignVCenter));
		ui->compileResult->setItem(crow, 1, generateItem(match.captured(2)));
		ui->compileResult->setItem(crow, 2, generateItem(match.captured(3)));
		ui->compileResult->setItem(crow, 3, generateItem(match.captured(4)));
		ui->compileResult->setItem(crow, 4, generateItem(match.captured(5), Qt::AlignVCenter));
		++crow;
	}
	ui->Log->appendPlainText(QString("[返回]%1").arg(retCode));
	return !retCode;
}

void EditorInfo::run() {
	if (isUntitled() && !askSave()) {
		return;
	}
	QFileInfo si(path);
	QString exe = si.path() + QDir::separator() + si.baseName() + exeSuf;
	if (!QFileInfo(exe).exists()) {
		QMessageBox::warning(editor, "qdevcpp", "代码尚未编译");
	} else {
		SubProcess* sp = new SubProcess(ui->Log, exe, editor);
		sp->start();
	}
}

bool EditorInfo::isModifiedByOthers() const {
	return isUntitled() ? false : whenOpen != QFileInfo(path).lastModified();
}

void EditorInfo::reload() {
	int line, index;
	editor->getCursorPosition(&line, &index);
	if (open(path)) {
		editor->setCursorPosition(line, index);
	}
}
