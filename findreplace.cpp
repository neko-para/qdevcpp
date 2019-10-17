#include "findreplace.h"
#include "ui_findreplace.h"
#include "confighelp.h"
#include <QJsonObject>
#include <QMessageBox>

QJsonValue FindReplaceConfig::toJson() const {
	QJsonObject obj;
#define JSON_OBJ obj
	JSON_SET(useRegex);
	JSON_SET(caseInsensitive);
	JSON_SET(matchWord);
	JSON_SET(informBeforeReplace);
	JSON_SET(findBackward);
	JSON_SET(startAtBegin);
	JSON_SET(onlyInSelected);
#undef JSON_OBJ
	return obj;
}

void FindReplaceConfig::fromJson(QJsonValue value) {
	QJsonObject obj = value.toObject();
#define JSON_OBJ obj
	JSON_GET(useRegex);
	JSON_GET(caseInsensitive);
	JSON_GET(matchWord);
	JSON_GET(informBeforeReplace);
	JSON_GET(findBackward);
	JSON_GET(wrap);
#undef JSON_OBJ
}

FindReplace::FindReplace(FindReplaceConfig& cfg, QWidget *parent) : QDialog(parent), ui(new Ui::FindReplace), config(cfg) {
	ui->setupUi(this);
#define BIND_CONFIG(key) \
	do { \
		ui->key->setChecked(config.key); \
		connect(ui->key, &QCheckBox::toggled, [&](bool c) { \
			config.key = c; \
		}); \
	} while (false)
	BIND_CONFIG(useRegex);
	BIND_CONFIG(caseInsensitive);
	BIND_CONFIG(matchWord);
	BIND_CONFIG(informBeforeReplace);
	BIND_CONFIG(findBackward);
	BIND_CONFIG(wrap);
	connect(ui->findPattern, &QLineEdit::textChanged, [&]() {
//		findBefore = false;
	});
	connect(ui->find, &QPushButton::clicked, [&]() {
	});

	connect(ui->close, &QPushButton::clicked, this, &FindReplace::hide);
	setEditorInfo(nullptr);
}

FindReplace::~FindReplace() {
	delete ui;
}

void FindReplace::setEditorInfo(EditorInfo* i) {
	ei = i;
//	finding = false;
	if (!ei) {
		ui->labelFind->setEnabled(false);
		ui->labelReplace->setEnabled(false);
		ui->find->setEnabled(false);
		ui->replace->setEnabled(false);
		ui->options->setEnabled(false);
	} else {
		ui->labelFind->setEnabled(true);
		ui->labelReplace->setEnabled(true);
		ui->find->setEnabled(true);
		ui->replace->setEnabled(true);
		ui->options->setEnabled(true);
	}
	ui->findNext->setEnabled(false);
	ui->findNext->setEnabled(false);
}

void FindReplace::findFail() {
	QMessageBox::warning(this, "qdevcpp", QString("未找到‘%1’").arg(ui->findPattern->text()));
}
