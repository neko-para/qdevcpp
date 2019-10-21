#include "aboutqdevcpp.h"
#include "ui_aboutqdevcpp.h"
#include "config.h"

AboutQDevCpp::AboutQDevCpp(QWidget *parent) : QDialog(parent), ui(new Ui::AboutQDevCpp) {
	ui->setupUi(this);
	connect(ui->ok, &QPushButton::clicked, this, &AboutQDevCpp::accept);
	ui->labelQDevCpp->setText(QString("QDevCpp v%1.%2").arg(QDEVCPP_VERSION_MAJOR).arg(QDEVCPP_VERSION_MINOR));
}

AboutQDevCpp::~AboutQDevCpp() {
	delete ui;
}
