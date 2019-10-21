#ifndef EDITORINFO_H
#define EDITORINFO_H

#include <QDateTime>
#include <QLabel>
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexer.h>
#include "editorconfig.h"


namespace Ui {
	class MainWindow;
}

class EditorInfo : public QObject {
	Q_OBJECT

	friend class FindReplace;
	friend class MainWindow;

	QsciScintilla* editor;
	Ui::MainWindow* ui;
	QString path;
	bool modified = false;
	bool canUndo;
	bool canRedo;
	QDateTime whenOpen;

	static int untitled_next;
	static QList<int> untitled_rest;

private slots:
	void modificationChanged(bool m);
public slots:
	void updateUndoRedoState();
	void updateSelectionState();
	void updateStatusInfo();
public:
	EditorInfo(QsciScintilla* e, Ui::MainWindow* ui);
	virtual ~EditorInfo();
	QString getPath() const {
		return path;
	}
	bool isUntitled() const {
		return path[0] == '#';
	}
	bool isAtSavePoint() const {
		return !modified;
	}
	void generateUntitled();
	QString generateName() const;
	QString generateTitle() const {
		return generateName() + (isAtSavePoint() ? "" : " *");
	}
	void updateEditorConfig(const EditorConfigure& cfg);
	bool open(const QString& cpath);
	bool write(const QString& cpath);
	bool saveas();
	bool save();
	bool askSave();
	bool shallSyntaxHighlight() const;
	bool compile() const;
	void run();
	bool isModifiedByOthers() const;
	void reload();
signals:
	void pathChange(QString cpath, QString ppath);
};


#endif // EDITORINFO_H
