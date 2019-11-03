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

#include <QDebug>

void CoreEditor::wheelEvent(QWheelEvent* e) {
	if (e->modifiers() == Qt::ControlModifier) {
		QPoint ns = e->angleDelta() / QWheelEvent::DefaultDeltasPerStep;
		if (ns.ry() < 0) {
			zoomOut(2 * -ns.ry());
		} else {
			zoomIn(2 * ns.ry());
		}
		info->updateLineNumber();
		e->accept();
	} else {
		QsciScintilla::wheelEvent(e);
	}
}

void CoreEditor::focusInEvent(QFocusEvent* e) {
	auto ei = ::window->info[this];
	if (ei->isModifiedByOthers()) {
		ei->reload();
	}
	QsciScintilla::focusInEvent(e);
}


#define COMPLETE_UN(brace, braces, opt) \
	case Qt::brace: \
		if (!editorConfig.opt) { \
			break; \
		} \
		if (selectedText().length()) { \
			replaceSelectedText(braces + selectedText() + braces); \
			justCompleteBrace = 0; \
		} else { \
			int l, i; \
			getCursorPosition(&l, &i); \
			if (justCompleteBrace == Qt::brace) { \
				setCursorPosition(l, i + 1); \
				justCompleteBrace = 0; \
			} else { \
				insert(braces braces); \
				setCursorPosition(l, i + 1); \
				justCompleteBrace = Qt::brace; \
			} \
		} \
		e->accept(); \
		return;

#define COMPLETE_BI(lbrace, rbrace, lbraces, rbraces, opt) \
	case Qt::lbrace: \
		if (!editorConfig.opt) { \
			break; \
		} \
		if (selectedText().length()) { \
			replaceSelectedText(lbraces + selectedText() + rbraces); \
			justCompleteBrace = 0; \
		} else { \
			int l, i; \
			getCursorPosition(&l, &i); \
			insert(lbraces rbraces); \
			setCursorPosition(l, i + 1); \
			justCompleteBrace = Qt::lbrace; \
		} \
		e->accept(); \
		return; \
	case Qt::rbrace: \
		if (!editorConfig.opt) { \
			break; \
		} \
		if (justCompleteBrace == Qt::lbrace) { \
			int l, i; \
			getCursorPosition(&l, &i); \
			setCursorPosition(l, i + 1); \
			justCompleteBrace = 0; \
			e->accept(); \
			return; \
		} \
		justCompleteBrace = 0; \
		break;


void CoreEditor::keyPressEvent(QKeyEvent* e) {
	if (e->modifiers() == Qt::ControlModifier) {
		switch (e->key()) {
		case Qt::Key_D:
			::window->ui->actionDelRow->trigger();
			e->accept();
			return;
		case Qt::Key_Slash:
			::window->ui->actionToggleComment->trigger();
			e->accept();
			return;
		}
	} else if (e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::ShiftModifier) {
		if (justCompleteBrace) {
			if (e->key() == Qt::Key_Backspace) {
				int l, i;
				getCursorPosition(&l, &i);
				setSelection(l, i - 1, l, i + 1);
				removeSelectedText();
				e->accept();
				return;
			}
		}
		switch (e->key()) {
			COMPLETE_BI(Key_ParenLeft, Key_ParenRight, "(", ")", completeSBrace);
			COMPLETE_BI(Key_BracketLeft, Key_BracketRight, "[", "]", completeMBrace);
			COMPLETE_BI(Key_BraceLeft, Key_BraceRight, "{", "}", completeLBrace);
			COMPLETE_UN(Key_QuoteDbl, "\"", completeDQuote);
			COMPLETE_UN(Key_Apostrophe, "'", completeSQuote);
		}
		if (e->key()) {
			justCompleteBrace = 0;
		}
	}
	return QsciScintilla::keyPressEvent(e);
}

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

void EditorInfo::updateLineNumber() {
	int lines = editor->lines();
	int w = 0;
	while (lines) {
		lines /= 10;
		++w;
	}
	editor->setMarginWidth(1, QString(w + 1, '0'));
}

void EditorInfo::updateSelectionState() {
	int sl, si, el, ei;
	editor->getSelection(&sl, &si, &el, &ei);
	bool e = (sl != -1);
	ui->actionCopy->setEnabled(e);
	ui->actionCut->setEnabled(e);
	e = e && (sl != el);
	ui->actionIndent->setEnabled(e);
	ui->actionUnindent->setEnabled(e);
}

void EditorInfo::updateStatusInfo() {
	int l, i;
	editor->getCursorPosition(&l, &i);
	window->status[MainWindow::ST_ROW]->setText(QString("行:%1").arg(l + 1));
	window->status[MainWindow::ST_COL]->setText(QString("列:%1").arg(i + 1));
	window->status[MainWindow::ST_AR]->setText(QString("总行:%1").arg(editor->lines()));
	window->status[MainWindow::ST_AL]->setText(QString("总长:%1").arg(editor->text().length()));
	int sl = editor->selectedText().length();
	if (sl) {
		window->status[MainWindow::ST_S]->setText(QString("选择:%1").arg(sl));
	} else {
		window->status[MainWindow::ST_S]->setText("");
	}
}

QsciLexerCPP* createLexer() {
	QsciLexerCPP* lexer = new QsciLexerCPP;
	lexer->setFont(QFont("ubuntu mono"));
	return lexer;
}

EditorInfo::EditorInfo(CoreEditor *e, Ui::MainWindow* ui) : editor(e), ui(ui) {
	editor->info = this;
	language = Language::query("demo.txt");
	connect(editor, &CoreEditor::modificationChanged, this, &EditorInfo::modificationChanged);
	connect(editor, &CoreEditor::textChanged, this, &EditorInfo::updateUndoRedoState);
	connect(editor, &CoreEditor::textChanged, this, &EditorInfo::updateLineNumber);
	connect(editor, &CoreEditor::selectionChanged, this, &EditorInfo::updateSelectionState);
	connect(editor, &CoreEditor::selectionChanged, this, &EditorInfo::updateStatusInfo);
	connect(editor, &CoreEditor::cursorPositionChanged, this, &EditorInfo::updateStatusInfo);
	connect(this, &EditorInfo::pathChange, [&](QString cpath) {
		if (editor->lexer()) {
			editor->lexer()->deleteLater();
		}
		delete language;
		language = Language::query(cpath);
		editor->setLexer(language->lexer());
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

void EditorInfo::updateEditorConfig(const EditorConfigure& cfg) {
	editor->setAutoIndent(cfg.autoIndent);
	editor->setWhitespaceVisibility((QsciScintilla::WhitespaceVisibility)cfg.showWhiteSpace);
	if (cfg.enableRightMargin) {
		editor->setEdgeMode(QsciScintilla::EdgeLine);
		editor->setEdgeColumn(cfg.marginWidth);
	} else {
		editor->setEdgeMode(QsciScintilla::EdgeNone);
	}
	editor->setCaretLineVisible(cfg.highlightCurrent);
	if (cfg.highlightCurrent) {
		editor->setCaretLineBackgroundColor(cfg.currentColor);
	}
	QFont font(cfg.font, cfg.fontSize);
	if (editor->lexer()) {
		editor->lexer()->setFont(font);
	} else {
		editor->setFont(font);
	}
}

bool EditorInfo::open(const QString& cpath)  {
	QFile file(cpath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		ui->Log->appendPlainText(QString("[错误]%1打开失败 - %2:%3").arg(QFileInfo(cpath).fileName()).arg(file.error()).arg(file.errorString()));;
		return false;
	}
	QByteArray buffer = file.readAll();
	file.close();
	QString ppath = path;
	path = cpath;
	emit pathChange(cpath, ppath);
	editor->setText(QString::fromUtf8(buffer));
	editor->setModified(false);
	whenOpen = QFileInfo(path).lastModified();
	return true;
}

bool EditorInfo::write(const QString& cpath)  {
	auto buffer = editor->text().toUtf8();
	QFile file(cpath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
		ui->Log->appendPlainText(QString("[错误]%1保存失败 - %2").arg(QFileInfo(cpath).fileName()).arg(file.errorString()));
		return false;
	}
	file.write(buffer);
	file.close();
	if (path != cpath) {
		QString ppath = path;
		path = cpath;
		emit pathChange(cpath, ppath);
	}
	editor->setModified(false);
	whenOpen = QFileInfo(path).lastModified();
	return true;
}

bool EditorInfo::saveas()  {
	QString cpath = QFileDialog::getSaveFileName(editor, "qdevcpp - 另存为", "", "C源代码 (*.c);;C++源代码 (*.cpp);;C/C++头文件 (*.h);;C++头文件 (*.hpp);;所有文件 (*.*)");
	if (cpath == "") {
		return false;
	}
	return write(cpath);
}

bool EditorInfo::save() {
	QString cpath = path;
	if (isUntitled()) {
		cpath = QFileDialog::getSaveFileName(editor, "qdevcpp - 保存", "", "C源代码 (*.c);;C++源代码 (*.cpp);;C/C++头文件 (*.h);;C++头文件 (*.hpp);;所有文件 (*.*)");
		if (cpath == "") {
			return false;
		}
	}
	return write(cpath);
}

bool EditorInfo::askSave()  {
	if (isAtSavePoint()) {
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

static QTableWidgetItem* generateItem(const QString& s, int align = Qt::AlignCenter) {
	auto i = new QTableWidgetItem(s);
	i->setTextAlignment(align);
	return i;
}

bool EditorInfo::compile() const {
	if (!language->test<CompilableLanguage>()) {
		return false;
	}
	QProcess proc;
	proc.setEnvironment(QProcess::systemEnvironment() << "LANGUAGE=en_US.UTF-8");
	QString cc = currentConfig->start(proc, path, language);
	ui->Log->appendPlainText(QString("[执行]%1").arg(cc));
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
