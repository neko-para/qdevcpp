#include "environmentconfig.h"
#include "ui_environmentconfig.h"
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QApplication>
#include <QProcess>
#include <QEventLoop>
#include <functional>

bool LinkTool::installed() {
#ifdef Q_OS_LINUX
	return QFile::exists(QString("%1/.local/share/applications/qdevcpp.desktop").arg(QDir::homePath()));
#endif
}

static bool extract(QString ipath, QString opath, std::function<void(QByteArray& )> filter) {
	QFile i(ipath), o(opath);
	i.open(QIODevice::ReadOnly);
	QByteArray data = i.readAll();
	i.close();
	QString dir = QFileInfo(opath).absolutePath();
	if (!QDir().mkpath(dir)) {
		return false;
	}
	if (!o.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return false;
	}
	filter(data);
	o.write(data);
	o.close();
	return true;
}

static bool extract(QString ipath, QString opath) {
	return extract(ipath, opath, [](QByteArray&) {});
}

bool LinkTool::install() {
#ifdef Q_OS_LINUX
	QString share = QString("%1/.local/share/").arg(QDir::homePath());
	return extract(":/qdevcpp.desktop", QString("%1/applications/qdevcpp.desktop").arg(share), [](QByteArray& data) {
		QString path = QApplication::applicationFilePath();
		if (QFileInfo(path).isSymLink()) {
			path = QFile(path).symLinkTarget();
		}
		data = QString::fromUtf8(data).replace('@', path).toUtf8();
	}) && extract(":/qdevcpp.svg", QString("%1/icons/qdevcpp.svg").arg(share));
	return true;
#elif defined(Q_OS_WIN)
#endif
}

bool LinkTool::query(const QString& mime) {
#ifdef Q_OS_LINUX
	QProcess proc;
	QEventLoop loop;
	proc.start("/usr/bin/xdg-mime", QStringList { "query", "default", mime });
	QObject::connect(&proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [&]() {
		loop.exit(QString::fromUtf8(proc.readAll()) != "qdevcpp.desktop");
	});
	return loop.exec();
#elif defined(Q_OS_WIN)
#endif
}

bool LinkTool::set(const QString& mime) {
#ifdef Q_OS_LINUX
	QProcess proc;
	QEventLoop loop;
	proc.start("/usr/bin/xdg-mime", QStringList { "default", "qdevcpp.desktop", mime });
	QObject::connect(&proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &loop, &QEventLoop::exit);
	return !loop.exec();
#elif defined(Q_OS_WIN)
#endif
}

#define QUERY_BIND(btn, mime) \
	do { \
		if (LinkTool::query("text/x-" mime)) { \
			ui->btn->setText(QString("%1 -  已关联").arg(ui->btn->text())); \
		} else { \
			ui->btn->setEnabled(true); \
			connect(ui->btn, &QPushButton::clicked, [&]() { \
				if (LinkTool::set("text/x-" mime)) { \
					ui->btn->setText(QString("%1 -  已关联").arg(ui->btn->text())); \
					ui->btn->setEnabled(false); \
				} else { \
					QMessageBox::warning(this, "qdevcpp", "安装失败"); \
				} \
			}); \
		} \
	} while (0)

EnvironmentConfig::EnvironmentConfig(QWidget *parent) : QDialog(parent), ui(new Ui::EnvironmentConfig) {
	ui->setupUi(this);
	connect(ui->install, &QPushButton::clicked, [&]() {
		if (LinkTool::install()) {
			ui->install->setText("重新安装");
		} else {
			QMessageBox::warning(this, "qdevcpp", "安装失败");
			ui->install->setText(installed ? "重新安装" : "安装");
		}
	});
	if (LinkTool::installed()) {
		ui->install->setText("重新安装");
		installed = true;
		QUERY_BIND(csrc, "csrc");
		QUERY_BIND(chdr, "chdr");
		QUERY_BIND(cxxsrc, "c++src");
		QUERY_BIND(cxxhdr, "c++hdr");
	}
}

EnvironmentConfig::~EnvironmentConfig() {
	delete ui;
}
