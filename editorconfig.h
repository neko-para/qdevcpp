#ifndef EDITORCONFIG_H
#define EDITORCONFIG_H

#include <QDialog>
#include "config.h"

namespace Ui {
	class EditorConfig;
}

struct EditorConfigure : public Config {
	enum WhiteSpace {
		WS_HIDE,
		WS_SHOW,
		WS_SHOW_EXCEPT_INDENT
	};
	bool autoIndent = false;
	int showWhiteSpace = WS_HIDE;
	bool enableRightMargin = false;
	int marginWidth = 80;
	bool enableAutoSave = false;
	int saveInterval = 5;
	bool showLineNumber = true;
	bool highlightCurrent = true;
	QString font; // "Consolas"
	int fontSize = 20;

	EditorConfigure() : font("Consolas") {}
	virtual QJsonValue toJson() const;
	virtual void fromJson(QJsonValue value);
};

class EditorConfig : public QDialog {
	Q_OBJECT

public:
	explicit EditorConfig(const EditorConfigure& cfg, QWidget *parent = 0);
	~EditorConfig();
	EditorConfigure configure() const {
		return config;
	}

private:
	Ui::EditorConfig *ui;
	EditorConfigure config;
};

#endif // EDITORCONFIG_H
