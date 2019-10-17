#ifndef COMPILECONFIG_H
#define COMPILECONFIG_H

#include <QDialog>
#include <QMap>
#include <QProcess>
#include <QJsonObject>
#include "config.h"

namespace Ui {
	class CompileConfig;
}

struct CompileConfigure : public Config {
	enum Optimize {
		O_NONE,
		O_LOW,
		O_MED,
		O_HIGH,
		O_HIGHEST,
		O_DEBUG
	};
	enum CStd {
		CS_C90,
		CS_C99,
		CS_C11
	};
	enum CxxStd {
		CXS_CXX98,
		CXS_CXX11,
		CXS_CXX14,
		CXS_CXX17
	};
	enum Bit {
		B_32,
		B_64
	};
	enum Warning {
		W_IGNORE,
		W_NORMAL,
		W_ALL,
		W_EXTRA
	};
	QString name = "新配置";
	QString gccPath = "";
	QString extraCompile = "";
	QString extraLink = "";
	int optimize = O_NONE;
	int cstd = CS_C90;
	int cxxstd = CXS_CXX98;
	int bit = B_32;
	int warning = W_NORMAL;
	bool werror = false;
	bool debug = false;

	virtual QJsonValue toJson() const;
	virtual void fromJson(QJsonValue value);
	void start(QProcess& proc, const QString& src);
};

extern CompileConfigure* currentConfig;

class CompileConfig : public QDialog {
	Q_OBJECT

public:
	explicit CompileConfig(QList<CompileConfigure> cfg, int cur, QWidget *parent = 0);
	~CompileConfig();

	const QList<CompileConfigure>& configure() const {
		return config;
	}

	int currentConfigure() const {
		return current;
	}


private:
	Ui::CompileConfig *ui;
	QList<CompileConfigure> config;
	int current;
	CompileConfigure* currentConfig = nullptr;
};

#endif // COMPILECONFIG_H
