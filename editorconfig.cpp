#include "editorconfig.h"
#include "ui_editorconfig.h"
#include "confighelp.h"
#include <QJsonObject>
#include <QDebug>

QJsonValue EditorConfigure::toJson() const {
	QJsonObject obj;
	JSON_SET(autoIndent);
	JSON_SET(showWhiteSpace);
	JSON_SET(enableRightMargin);
	JSON_SET(marginWidth);
	JSON_SET(enableAutoSave);
	JSON_SET(saveInterval);
	JSON_SET(showLineNumber);
	JSON_SET(highlightCurrent);
	JSON_SET(font);
	JSON_SET(fontSize);
	return obj;
}

void EditorConfigure::fromJson(QJsonValue value) {
	QJsonObject obj = value.toObject();
	JSON_GET(autoIndent);
	JSON_GET(showWhiteSpace);
	JSON_GET(enableRightMargin);
	JSON_GET(marginWidth);
	JSON_GET(enableAutoSave);
	JSON_GET(saveInterval);
	JSON_GET(showLineNumber);
	JSON_GET(highlightCurrent);
	JSON_GET(font);
	JSON_GET(fontSize);
}

EditorConfig::EditorConfig(const EditorConfigure& cfg, QWidget *parent) : QDialog(parent), ui(new Ui::EditorConfig), config(cfg) {
	ui->setupUi(this);
	BIND_CONFIG_BOOL(autoIndent);
	BIND_CONFIG_ENUM(showWhiteSpace);
	BIND_CONFIG_BOOL(enableRightMargin);
	BIND_CONFIG_INT(marginWidth, QSpinBox);
	BIND_CONFIG_BOOL(enableAutoSave);
	BIND_CONFIG_INT(saveInterval, QSlider);
	BIND_CONFIG_BOOL(showLineNumber);
	BIND_CONFIG_BOOL(highlightCurrent);
	BIND_CONFIG_INT(fontSize, QSpinBox);
	connect(ui->font, &QFontComboBox::currentFontChanged, [&](QFont f) {
		config.font = f.family();
	});
	ui->font->setCurrentFont(QFont(config.font));
	connect(ui->ok, &QPushButton::clicked, this, &EditorConfig::accept);
	connect(ui->cancel, &QPushButton::clicked, this, &EditorConfig::reject);
}

EditorConfig::~EditorConfig() {
	delete ui;
}
