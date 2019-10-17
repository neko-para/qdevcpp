#include "findreplace.h"
#include "ui_findreplace.h"
#include <QMessageBox>

FindReplace::FindReplace(QWidget *parent) : QDialog(parent), ui(new Ui::FindReplace) {
	ui->setupUi(this);
	connect(ui->findPattern, &QLineEdit::textChanged, [&]() {
		findBefore = false;
	});
	connect(ui->find, &QPushButton::clicked, [&]() {
		if (findBefore) {
			if (!ei->editor->findNext()) {
				QMessageBox::warning(this, "qdevcpp", QString("未找到‘%1’").arg(ui->findPattern->text()));
			}
		} else if (ei) {
			findBefore = true;
			if (ui->searchSelected->isChecked())
//			ei->editor->findFirst()
		}
	});

}

FindReplace::~FindReplace() {
	delete ui;
}

void FindReplace::setEditorInfo(EditorInfo* i) {
	ei = i;
	findBefore = false;
	if (ei) {
		ui->find->setEnabled(false);
		ui->replace->setEnabled(false);
	} else {
		ui->find->setEnabled(true);
		ui->replace->setEnabled(true);
	}
}
