#ifndef FINDREPLACE_H
#define FINDREPLACE_H

#include <QDialog>
#include "editorinfo.h"
#include "config.h"

namespace Ui {
	class FindReplace;
}

struct FindReplaceConfig : public Config {
	bool useRegex = false;
	bool caseInsensitive = false;
	bool matchWord = false;
	bool informBeforeReplace = false;
	bool findBackward = false;
	bool onlyInSelected = false;
	bool startAtBegin = false;
	bool wrap = false;

	virtual QJsonValue toJson() const;
	virtual void fromJson(QJsonValue value);
};

class FindReplace : public QDialog {
	Q_OBJECT

public:
	explicit FindReplace(FindReplaceConfig& cfg, QWidget *parent = 0);
	~FindReplace();

	void setEditorInfo(EditorInfo* i);

private:
	Ui::FindReplace *ui;
	EditorInfo* ei = nullptr;
	bool findBefore = false;
	FindReplaceConfig& config;
};

#endif // FINDREPLACE_H
