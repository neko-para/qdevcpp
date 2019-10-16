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

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	QsciScintilla* currentEditor();

	bool closeTab(QsciScintilla* e);

private slots:
	void updateTab(int idx);
	void updateCompileActions();
	void updatePasteAction();

private:
	Ui::MainWindow *ui;
	QMap<QsciScintilla*, EditorInfo*> info;
};

#endif // MAINWINDOW_H
