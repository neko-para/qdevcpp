#include "editorconfig.h"
#include "ui_editorconfig.h"
#include "confighelp.h"
#include <QJsonObject>
#include <QColorDialog>
#include <QDebug>

QJsonValue EditorConfigure::toJson() const {
	QJsonObject obj;
	JSON_SET(autoIndent);
	JSON_SET(showWhiteSpace);
	JSON_SET(enableRightMargin);
	JSON_SET(marginWidth);
	JSON_SET(enableAutoSave);
	JSON_SET(saveInterval);
	JSON_SET(highlightCurrent);
	obj.insert("currentColor", QJsonValue(currentColor.name(QColor::HexArgb)));
	JSON_SET(font);
	JSON_SET(fontSize);
	JSON_SET(completeSBrace);
	JSON_SET(completeMBrace);
	JSON_SET(completeLBrace);
	JSON_SET(completeSQuote);
	JSON_SET(completeDQuote);
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
	JSON_GET(highlightCurrent);
	if (obj.contains("currentColor")) {
		currentColor.setNamedColor(obj["currentColor"].toVariant().value<QString>());
	}
	JSON_GET(font);
	JSON_GET(fontSize);
	JSON_GET(completeSBrace);
	JSON_GET(completeMBrace);
	JSON_GET(completeLBrace);
	JSON_GET(completeSQuote);
	JSON_GET(completeDQuote);
}

EditorConfig::EditorConfig(const EditorConfigure& cfg, QWidget *parent) : QDialog(parent), ui(new Ui::EditorConfig), config(cfg) {
	ui->setupUi(this);
	BIND_CONFIG_BOOL(autoIndent);
	BIND_CONFIG_ENUM(showWhiteSpace);
	BIND_CONFIG_BOOL(enableRightMargin);
	BIND_CONFIG_INT(marginWidth, QSpinBox);
	BIND_CONFIG_BOOL(enableAutoSave);
	BIND_CONFIG_INT(saveInterval, QSlider);
	BIND_CONFIG_BOOL(highlightCurrent);
	BIND_CONFIG_BOOL(completeSBrace);
	BIND_CONFIG_BOOL(completeMBrace);
	BIND_CONFIG_BOOL(completeLBrace);
	BIND_CONFIG_BOOL(completeSQuote);
	BIND_CONFIG_BOOL(completeDQuote);
	connect(ui->browseCurrentColor, &QToolButton::clicked, [&]() {
		QColor c = QColorDialog::getColor(config.currentColor, this, "qdevcpp - 选取当前行颜色");
		if (c.isValid()) {
			config.currentColor = c;
			ui->currentColor->setStyleSheet(QString("#currentColor { background: %1 }").arg(config.currentColor.name(QColor::HexArgb)));
		}
	});
	ui->currentColor->setStyleSheet(QString("#currentColor { background: %1 }").arg(config.currentColor.name(QColor::HexArgb)));
	connect(ui->font, &QFontComboBox::currentFontChanged, [&](QFont f) {
		config.font = f.family();
	});
	ui->font->setCurrentFont(QFont(config.font));
	BIND_CONFIG_INT(fontSize, QSpinBox);
	connect(ui->ok, &QPushButton::clicked, this, &EditorConfig::accept);
	connect(ui->cancel, &QPushButton::clicked, this, &EditorConfig::reject);
}

EditorConfig::~EditorConfig() {
	delete ui;
}
