#include "mainwindow.h"
#include <QApplication>
#include "config.h"

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);
	a.setApplicationName("qdevcpp");
	a.setApplicationVersion(QString("%1.%2").arg(QDEVCPP_VERSION_MAJOR).arg(QDEVCPP_VERSION_MINOR));
	QStringList files = QApplication::arguments();
	files.removeAll("");
	MainWindow w;
	w.show();
	w.open(files);
	return a.exec();
}
