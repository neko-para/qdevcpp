#ifndef FINDREPLACE_H
#define FINDREPLACE_H

#include <QDialog>
#include "editorinfo.h"

namespace Ui {
	class FindReplace;
}

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
