#ifndef FINDREPLACE_H
#define FINDREPLACE_H

#include <QDialog>
#include "editorinfo.h"

namespace Ui {
	class FindReplace;
}

struct FindReplaceConfig {
	bool useRegex = false;
	bool caseInsensitive = false;
	bool matchWord = false;
	bool informBeforeReplace = false;
	bool findAll = false;
	bool startAtBegin = false;
	bool onlyInSelected = false;
};

class FindReplace : public QDialog {
	Q_OBJECT

public:
	explicit FindReplace(QWidget *parent = 0);
	~FindReplace();

	void setEditorInfo(EditorInfo* i);

private:
	Ui::FindReplace *ui;
	EditorInfo* ei = nullptr;
	bool findBefore = false;
};

#endif // FINDREPLACE_H
