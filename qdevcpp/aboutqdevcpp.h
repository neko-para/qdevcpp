#ifndef ABOUTQDEVCPP_H
#define ABOUTQDEVCPP_H

#include <QDialog>

namespace Ui {
	class AboutQDevCpp;
}

class AboutQDevCpp : public QDialog {
	Q_OBJECT

public:
	explicit AboutQDevCpp(QWidget *parent = 0);
	~AboutQDevCpp();

private:
	Ui::AboutQDevCpp *ui;
};

#endif // ABOUTQDEVCPP_H
