#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);
	QFont uiFont("ubuntu");
	a.setFont(uiFont);
	MainWindow w;
	w.show();

	return a.exec();
}
