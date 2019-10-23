#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include "config.h"

void buildParser(QCommandLineParser& parser) {
	parser.addHelpOption();
	parser.addVersionOption();
	QCommandLineOption optFile(QStringList{"f", "file"});
	optFile.setValueName("file");
	optFile.setDescription("Files to be opened");
	parser.addOption(optFile);
}

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);
	a.setApplicationName("qdevcpp");
	a.setApplicationVersion(QString("%1.%2").arg(QDEVCPP_VERSION_MAJOR).arg(QDEVCPP_VERSION_MINOR));
	QCommandLineParser parser;
	buildParser(parser);
	parser.process(a);
	MainWindow w;
	w.show();
	w.open(parser.values("file"));
	return a.exec();
}
