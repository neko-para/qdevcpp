#include "aboutqdevcpp.h"
#include "ui_aboutqdevcpp.h"

AboutQDevCpp::AboutQDevCpp(QWidget *parent) : QDialog(parent), ui(new Ui::AboutQDevCpp) {
	ui->setupUi(this);
	connect(ui->ok, &QPushButton::clicked, this, &AboutQDevCpp::accept);
}

AboutQDevCpp::~AboutQDevCpp() {
	delete ui;
}
