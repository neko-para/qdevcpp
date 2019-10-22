#include "findreplace.h"
#include "ui_findreplace.h"
#include "confighelp.h"
#include <QJsonObject>
#include <QMessageBox>
#include <QKeyEvent>
#include <QRegularExpression>

QJsonValue FindReplaceConfig::toJson() const {
	QJsonObject obj;
	JSON_SET(useRegex);
	JSON_SET(caseInsensitive);
	JSON_SET(matchWord);
	JSON_SET(findBackward);
	JSON_SET(wrap);
	return obj;
}

void FindReplaceConfig::fromJson(QJsonValue value) {
	QJsonObject obj = value.toObject();
	JSON_GET(useRegex);
	JSON_GET(caseInsensitive);
	JSON_GET(matchWord);
	JSON_GET(findBackward);
	JSON_GET(wrap);
}

FindReplace::FindReplace(FindReplaceConfig& cfg, QWidget *parent) : QDialog(parent), ui(new Ui::FindReplace), config(cfg) {
	ui->setupUi(this);
	ui->findPattern->installEventFilter(this);
	ui->replacePattern->installEventFilter(this);
	BIND_CONFIG_BOOL(useRegex);
	BIND_CONFIG_BOOL(caseInsensitive);
	BIND_CONFIG_BOOL(matchWord);
	BIND_CONFIG_BOOL(findBackward);
	BIND_CONFIG_BOOL(wrap);
	connect(ui->find, &QPushButton::clicked, [&]() {
		if (config.findBackward && ei->editor->selectedText().length()) {
			QApplication::sendEvent(ei->editor, new QKeyEvent(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier));
		}
		ei->editor->findFirst(ui->findPattern->text(),
							  config.useRegex,
							  !config.caseInsensitive,
							  config.matchWord,
							  config.wrap,
							  !config.findBackward, -1, -1, true, true) || findFail();
	});
	connect(ui->replace, &QPushButton::clicked, [&]() {
		if (config.findBackward && ei->editor->selectedText().length()) {
			QApplication::sendEvent(ei->editor, new QKeyEvent(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier));
		}
		if (ei->editor->findFirst(ui->findPattern->text(),
								  config.useRegex,
								  !config.caseInsensitive,
								  config.matchWord,
								  config.wrap,
								  !config.findBackward, -1, -1, true, true)) {
			if (config.useRegex) {
				QRegularExpression re(ui->findPattern->text());
				ei->editor->replace(ei->editor->selectedText().replace(re, ui->replacePattern->text()));
			} else {
				ei->editor->replace(ui->replacePattern->text());
			}
		} else {
			findFail();
		}
	});
	connect(ui->replaceAll, &QPushButton::clicked, [&]() {
		if (config.findBackward) {
			int line = ei->editor->lines() - 1;
			ei->editor->setCursorPosition(line, ei->editor->lineLength(line));
		} else {
			ei->editor->setCursorPosition(0, 0);
		}
		if (!ei->editor->findFirst(ui->findPattern->text(),
								  config.useRegex,
								  !config.caseInsensitive,
								  config.matchWord,
								  config.wrap,
								  !config.findBackward, -1, -1, true, true)) {
			findFail();
		}
		do {
			if (config.useRegex) {
			   QRegularExpression re(ui->findPattern->text());
			   ei->editor->replace(ei->editor->selectedText().replace(re, ui->replacePattern->text()));
			} else {
			   ei->editor->replace(ui->replacePattern->text());
			}
			if (config.findBackward && ei->editor->selectedText().length()) {
				QApplication::sendEvent(ei->editor, new QKeyEvent(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier));
			}
		} while (ei->editor->findNext());
	});
	connect(ui->close, &QPushButton::clicked, this, &FindReplace::hide);
	setEditorInfo(nullptr);
}

FindReplace::~FindReplace() {
	delete ui;
}

void FindReplace::setEditorInfo(EditorInfo* i) {
	ei = i;
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
}

bool FindReplace::findFail() {
	QMessageBox::warning(this, "qdevcpp", QString("未找到‘%1’").arg(ui->findPattern->text()));
	return true;
}

bool FindReplace::eventFilter(QObject* watched, QEvent* event) {
	QLineEdit* le = dynamic_cast<QLineEdit*>(watched);
	if (le && event->type() == QEvent::KeyPress) {
		QKeyEvent* ke = dynamic_cast<QKeyEvent*>(event);
		if (ke->key() == Qt::Key_Tab) {
			le->insert("\t");
			return true;
		}
	}
	return QDialog::eventFilter(watched, event);
}
