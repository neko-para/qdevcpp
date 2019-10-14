#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QList>
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexer.h>
#include <QDebug>
#include <QProcess>

namespace Ui {
	class MainWindow;
}

class SubProcess : public QObject {
	Q_OBJECT

	QPlainTextEdit* log;
	QString exe;
	QProcess* proc;
#ifdef Q_OS_LINUX

#endif
private slots:
	void fin(int code);
public:
	SubProcess(QPlainTextEdit* log, const QString& exe, QObject* parent = nullptr);
	bool start();
signals:
	void finished(int code);
};

class EditorInfo : public QObject {
	Q_OBJECT

	QsciScintilla* editor;
	QTabWidget* tabs;
	QPlainTextEdit* log;
	QTableWidget* result;
	QString path;
	bool modified = false;
	bool canUndo;
	bool canRedo;
	QAction* undo = nullptr;
	QAction* redo = nullptr;

	static int untitled_next;
	static QList<int> untitled_rest;
private slots:
	void modificationChanged(bool m);
public slots:
	void updateUndoRedoState();
public:
	EditorInfo(QsciScintilla* e, Ui::MainWindow* ui);
	virtual ~EditorInfo() {
		if (path[0] == '#') {
			untitled_rest.push_back(path.mid(1).toInt());
		}
		if (editor->lexer()) {
			editor->lexer()->deleteLater();
			editor->setLexer(0);
		}
		editor->deleteLater();
	}
	QString getPath() const {
		return path;
	}
	bool isUntitled() const {
		return path[0] == '#';
	}
	bool isAtSavePoint() const {
		return !modified;
	}
	void generateUntitled() {
		int id;
		if (untitled_rest.size()) {
			std::sort(untitled_rest.begin(), untitled_rest.end());
			id = untitled_rest.takeFirst();
		} else {
			id = ++untitled_next;
		}
		path = QString("#%1").arg(id);
	}

	QString generateName() const {
		if (path[0] == '#') {
			return QString("未命名 %1").arg(path.mid(1));
		} else {
			return QFileInfo(path).fileName();
		}
	}
	QString generateTitle() const {
		return generateName() + (isAtSavePoint() ? "" : " *");
	}
	bool open(const QString& cpath) {
		QFile file(cpath);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QMessageBox::warning(editor, "qdevcpp", QString("%1打开失败:%2").arg(QFileInfo(cpath).fileName(), file.errorString()));
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
			return true;
		} else {
			file.close();
			delete[] buffer;
			QMessageBox::warning(editor, "qdevcpp", QString("%1打开失败:%2").arg(QFileInfo(cpath).fileName(), file.errorString()));
			return false;
		}

	}
	bool write(const QString& cpath) {
		auto buffer = editor->text().toUtf8();
		QFile file(cpath);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
			QMessageBox::warning(editor, "qdevcpp", QString("%1保存失败:%2").arg(QFileInfo(cpath).fileName(), file.errorString()));
			return false;
		}
		if (buffer.size() == file.write(buffer)) {
			file.close();
			path = cpath;
			editor->setModified(false);
			return true;
		} else {
			file.close();
			QMessageBox::warning(editor, "qdevcpp", QString("%1保存失败:%2").arg(QFileInfo(cpath).fileName(), file.errorString()));
			return false;
		}
	}
	bool saveas() {
		QString cpath = QFileDialog::getSaveFileName(editor, "qdevcpp - 另存为", "", "C源代码 (*.c);;C++源代码 (*.cpp *.cc *.cxx);;C/C++头文件 (*.h);;C++头文件 (*.hpp);;所有文件 (*.*)");
		if (cpath == "") {
			return false;
		}
		return write(cpath);
	}
	bool save() {
		QString cpath = path;
		if (isUntitled()) {
			cpath = QFileDialog::getSaveFileName(editor, "qdevcpp - 保存", "", "C源代码 (*.c);;C++源代码 (*.cpp *.cc *.cxx);;C/C++头文件 (*.h);;C++头文件 (*.hpp);;所有文件 (*.*)");
			if (cpath == "") {
				return false;
			}
		}
		return write(cpath);
	}
	bool askSave() {
		if (!isUntitled() && isAtSavePoint()) {
			return true;
		}
		switch (QMessageBox::question(editor, "qdevcpp", QString("是否保存%1").arg(generateName()), QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel)) {
		case QMessageBox::Yes:
			if (save()) {
				return true;
			} else {
				return false;
			}
		case QMessageBox::Cancel:
			return false;
		case QMessageBox::No:
			return true;
		default:
			// wtf
			return true;
		}
	}
	bool shallSyntaxHighlight() const {
		return isUntitled() ? true : QStringList({"c", "cpp", "cxx", "cc", "h", "hpp"}).contains(QFileInfo(path).suffix());
	}
	bool compile() const;
	void run();
};

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	QsciScintilla* currentEditor();

	bool closeTab(QsciScintilla* e);

	void updateCompileActions();

private slots:
	void updateTab(int idx);

private:
	Ui::MainWindow *ui;
	QMap<QsciScintilla*, EditorInfo*> info;
};

#endif // MAINWINDOW_H
