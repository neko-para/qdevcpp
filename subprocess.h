#ifndef SUBPROCESS_H
#define SUBPROCESS_H

#include <QPlainTextEdit>
#include <QProcess>

class SubProcess : public QObject {
	Q_OBJECT

	QPlainTextEdit* log;
	QString exe;
	QProcess* proc;
#ifdef Q_OS_LINUX

#endif
private slots:
	void fin(int code);
public:
	SubProcess(QPlainTextEdit* log, const QString& exe, QObject* parent = nullptr);
	bool start();
signals:
	void finished(int code);
};


#endif // SUBPROCESS_H
