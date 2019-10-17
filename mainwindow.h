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
#include "editorinfo.h"
#include "findreplace.h"

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT

	friend class EditorInfo;

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	QsciScintilla* currentEditor();

	bool closeTab(QsciScintilla* e);
	EditorInfo* findTab(const QString& path, EditorInfo* except = nullptr);
	void removeOther(EditorInfo* ei);

public slots:
	void updateTab(int idx);
	void updateCompileActions();
	void updatePasteAction();
	void updateWindowTitle();

protected:
	virtual bool eventFilter(QObject *watched, QEvent *event);

private:
	Ui::MainWindow *ui;
	QMap<QsciScintilla*, EditorInfo*> info;
	FindReplace* finddlg = nullptr;
};

#endif // MAINWINDOW_H
