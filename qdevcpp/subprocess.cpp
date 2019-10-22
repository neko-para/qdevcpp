#include "subprocess.h"
#include <QEventLoop>
#include <QTimer>

SubProcess::SubProcess(QPlainTextEdit* log, const QString& exe, QObject* parent) : QObject(parent), log(log), exe(exe) {
	proc = new QProcess(this);
}

bool SubProcess::start() {
#ifdef Q_OS_WIN32
//	proc->start("C:\\Windows\\system32\\cmd.exe", QStringList{"-e", QString("\"%1\"; echo $?")});
#elif defined(Q_OS_LINUX)
	proc->start("/usr/bin/x-terminal-emulator", QStringList{"-e", QString("\"%1\"; echo \"\\n\\n返回值$?。按任意键继续……\"; read X").arg(exe)});
#endif
	log->appendPlainText(QString("[执行]%1").arg(exe));
	QEventLoop loop;
	QTimer timeout;
	timeout.setInterval(30000);
	timeout.setSingleShot(true);
	connect(&timeout, &QTimer::timeout, [&]() {
		proc->kill();
		loop.exit(1);
	});
	connect(proc, &QProcess::started, &loop, &QEventLoop::quit);
	if (loop.exec()) {
		log->appendPlainText(QString("[错误]无法执行"));
		return false;
	} else {
		timeout.stop();
		connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &SubProcess::fin);
		return true;
	}
}

void SubProcess::fin(int code) {
	emit finished(code);
	deleteLater();
}
