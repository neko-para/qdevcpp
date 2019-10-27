#include "environmentconfig.h"
#include "ui_environmentconfig.h"
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QApplication>
#include <QProcess>
#include <QEventLoop>
#include <functional>
#include <QSettings>
#ifdef Q_OS_WIN32
#include <shlobj.h>
#endif

bool LinkTool::installed() {
#ifdef Q_OS_LINUX
	return QFile::exists(QString("%1/.local/share/applications/qdevcpp.desktop").arg(QDir::homePath()));
#elif defined(Q_OS_WIN32)
	QSettings reg(R"(HKEY_CLASSES_ROOT)", QSettings::NativeFormat);
	for (QString s : QStringList { "c", "cpp", "h", "hpp" }) {
		if (!reg.childGroups().contains(QString("QDevCpp.%1").arg(s))) {
			return false;
		}
	}
	return true;
#endif
}

#ifdef Q_OS_LINUX
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
#endif

static QMap<QString, QString> suffix({
										 {"csrc", "c"},
										 {"chdr", "h"},
										 {"c++src", "cpp"},
										 {"c++hdr", "hpp"}
									 });

static QMap<QString, QString> name({
										 {"c", "C源文件"},
										 {"h", "C/C++头文件"},
										 {"cpp", "C++源文件"},
										 {"hpp", "C++头文件"}
									 });

bool LinkTool::install() {
	QString path = QApplication::applicationFilePath();
	if (QFileInfo(path).isSymLink()) {
		path = QFile(path).symLinkTarget();
	}
#ifdef Q_OS_LINUX
	QString share = QString("%1/.local/share/").arg(QDir::homePath());
	return extract(":/qdevcpp.desktop", QString("%1/applications/qdevcpp.desktop").arg(share), [&](QByteArray& data) {
		data = QString::fromUtf8(data).replace('@', path).toUtf8();
	}) && extract(":/qdevcpp.svg", QString("%1/icons/qdevcpp.svg").arg(share));
#elif defined(Q_OS_WIN32)
	path = path.replace('/', '\\');
	// TODO: ask UAC
	int id = 1;
	for (QString s : QStringList { "c", "cpp", "h", "hpp" }) {
		QSettings reg(QString(R"(HKEY_CLASSES_ROOT\QDevCpp.%1)").arg(s), QSettings::NativeFormat);
		if (!reg.isWritable()) {
			return false;
		}
		reg.setValue("Default", name[s]);
		reg.setValue("FriendlyAppName", "QDevCpp 编辑器");
		reg.beginGroup("DefaultIcon");
		reg.setValue("Default", QString("%1,%2").arg(path).arg(++id));
		reg.endGroup();
		reg.beginGroup("Shell");
		reg.beginGroup("Open");
		reg.beginGroup("Command");
		reg.setValue("Default", QString("\"%1\"").arg(path) + " \"%1\"");
		reg.endGroup();
		reg.endGroup();
		reg.endGroup();
		reg.beginGroup("SupportedTypes");
		reg.beginGroup("." + s);
		reg.setValue("Default", "");
		reg.endGroup();
		reg.endGroup();
	}
	return true;
#endif
}

bool LinkTool::query(const QString& mime) {
#ifdef Q_OS_LINUX
	QProcess proc;
	QEventLoop loop;
	proc.start("/usr/bin/xdg-mime", QStringList { "query", "default", "text/x-" + mime });
	QObject::connect(&proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [&]() {
		loop.exit(QString::fromUtf8(proc.readAll()) != "qdevcpp.desktop");
	});
	return loop.exec();
#elif defined(Q_OS_WIN)
	QSettings reg(QString(R"(HKEY_CLASSES_ROOT\.%1)").arg(suffix[mime]), QSettings::NativeFormat);
	return reg.value("Default").toString() == QString("QDevCpp.%1").arg(suffix[mime]);
#endif
}

bool LinkTool::set(const QString& mime) {
#ifdef Q_OS_LINUX
	QProcess proc;
	QEventLoop loop;
	proc.start("/usr/bin/xdg-mime", QStringList { "default", "qdevcpp.desktop", "text/x-" + mime });
	QObject::connect(&proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &loop, &QEventLoop::exit);
	return !loop.exec();
#elif defined(Q_OS_WIN)
	QSettings reg(QString(R"(HKEY_CLASSES_ROOT\.%1)").arg(suffix[mime]), QSettings::NativeFormat);
	if (!reg.isWritable()) {
		return false;
	}
	reg.setValue("Default", QString("QDevCpp.%1").arg(suffix[mime]));
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_DWORD | SHCNF_FLUSH, nullptr, nullptr);
	return true;
#endif
}

EnvironmentConfig::EnvironmentConfig(QWidget *parent) : QDialog(parent), ui(new Ui::EnvironmentConfig) {
	ui->setupUi(this);
	connect(ui->install, &QPushButton::clicked, [&]() {
		if (LinkTool::install()) {
			bind();
		} else {
			QMessageBox::warning(this, "qdevcpp", "安装失败");
			ui->install->setText(installed ? "重新安装" : "安装");
		}
	});
	if (LinkTool::installed()) {
		bind();
	}
}

EnvironmentConfig::~EnvironmentConfig() {
	delete ui;
}

#define QUERY_BIND(btn, mime) \
	do { \
		if (LinkTool::query(mime)) { \
			ui->btn->setText(QString("%1 -  已关联").arg(name[suffix[mime]])); \
		} else { \
			ui->btn->setEnabled(true); \
			ui->btn->setText(name[suffix[mime]]); \
			connect(ui->btn, &QPushButton::clicked, [&]() { \
				if (LinkTool::set(mime)) { \
					ui->btn->setText(QString("%1 -  已关联").arg(name[suffix[mime]])); \
					ui->btn->setEnabled(false); \
				} else { \
					QMessageBox::warning(this, "qdevcpp", "安装失败"); \
				} \
			}); \
		} \
	} while (0)

void EnvironmentConfig::bind() {
	ui->install->setText("重新安装");
	installed = true;
	QUERY_BIND(csrc, "csrc");
	QUERY_BIND(chdr, "chdr");
	QUERY_BIND(cxxsrc, "c++src");
	QUERY_BIND(cxxhdr, "c++hdr");
}
