#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexer.h>
#include <QTimer>
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
	virtual void closeEvent(QCloseEvent* e);

private:
	Ui::MainWindow *ui;
	QMap<QsciScintilla*, EditorInfo*> info;
	FindReplace* finddlg = nullptr;
	QTimer* autoSave;
	enum StatusType {
		ST_ROW,
		ST_COL,
		ST_AR,
		ST_AL,
		ST_S,
		ST_COUNT
	};
	QLabel* status[ST_COUNT];
};

#endif // MAINWINDOW_H
