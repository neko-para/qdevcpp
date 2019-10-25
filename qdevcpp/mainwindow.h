#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include "editorinfo.h"
#include "findreplace.h"

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT

	friend class CoreEditor;
	friend class EditorInfo;

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	CoreEditor* currentEditor();

	bool closeTab(QWidget* e);
	EditorInfo* findTab(const QString& path, EditorInfo* except = nullptr);
	void removeOther(EditorInfo* ei);

	void open(const QStringList& paths);

public slots:
	void updateTab(int idx);
	void updateCompileActions();
	void updatePasteAction();
	void updateWindowTitle();

protected:
	virtual void closeEvent(QCloseEvent* );
	virtual void dragEnterEvent(QDragEnterEvent* );
	virtual void dropEvent(QDropEvent* );

private:
	Ui::MainWindow *ui;
	QMap<CoreEditor*, EditorInfo*> info;
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
