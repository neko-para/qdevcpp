#include "compileconfig.h"
#include "ui_compileconfig.h"
#include "global.h"
#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>
#include <QDebug>
#include <QJsonArray>
#include <QInputDialog>

QJsonObject CompileConfigure::toJson() const {
	QJsonObject obj;
#define JSON_INSERT(key) \
	obj.insert(#key, QJsonValue(key))
	JSON_INSERT(name);
	JSON_INSERT(gccPath);
	JSON_INSERT(extraCompile);
	JSON_INSERT(extraLink);
	JSON_INSERT(optimize);
	JSON_INSERT(cstd);
	JSON_INSERT(cxxstd);
	JSON_INSERT(bit);
	JSON_INSERT(warning);
	JSON_INSERT(werror);
	JSON_INSERT(debug);
#undef JSON_INSERT
	return obj;
}

void CompileConfigure::fromJson(QJsonObject obj) {
#define JSON_GET(key) \
	do { \
		if (obj.contains(#key)) { \
			key = obj[#key].toVariant().value<decltype(key)>(); \
		} \
	} while (false)
	JSON_GET(name);
	JSON_GET(gccPath);
	JSON_GET(extraCompile);
	JSON_GET(extraLink);
	JSON_GET(optimize);
	JSON_GET(cstd);
	JSON_GET(cxxstd);
	JSON_GET(bit);
	JSON_GET(warning);
	JSON_GET(werror);
	JSON_GET(debug);
#undef JSON_GET
}

void CompileConfigure::start(QProcess& proc, const QString& src) {
	QStringList arg;
	QFileInfo si(src);
	arg << src << "-o" << (si.path() + QDir::separator() + si.baseName() + exeSuf);
	arg << ("-O" + (QStringList{"0", "1", "2", "3", "fast", "g"}[optimize]));
	if (cSuf.contains(si.suffix())) {
		arg << ("-std=c" + (QStringList{"90", "99", "11"}[cstd]));
	} else if (cxxSuf.contains(si.suffix())) {
		arg << ("-std=c++" + (QStringList{"98", "11", "14", "17"}[cxxstd]));
	}
	arg << ("-m" + (QStringList{"32", "64"}[bit]));
	switch (warning) {
	case 0:
		arg << "-w";
		break;
	case 2:
		arg << "-Wall";
		break;
	case 3:
		arg << "-Wall" << "-Wextra";
		break;
	default:
		;
	}
	if (werror) {
		arg << "-werror";
	}
	if (debug) {
		arg << "-g";
	}
	arg << extraCompile.split(QRegularExpression(R"(\n)"), QString::SkipEmptyParts);
	proc.start(gccPath, arg, QIODevice::ReadOnly);
}

CompileConfig::CompileConfig(QList<CompileConfigure> cfg, int cur, QWidget *parent) : QDialog(parent), ui(new Ui::CompileConfig), config(cfg) {
	ui->setupUi(this);
	connect(ui->ok, &QPushButton::clicked, this, &CompileConfig::accept);
	connect(ui->cancel, &QPushButton::clicked, this, &CompileConfig::reject);
	connect(ui->configure, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int idx) {
		current = idx;
		if (current == -1) {
			currentConfig = nullptr;
			ui->configure->setEnabled(false);
			ui->configDel->setEnabled(false);
			ui->configDup->setEnabled(false);
			ui->configRen->setEnabled(false);
			ui->gccLable->setEnabled(false);
			ui->gccPath->setEnabled(false);
			ui->gccBrowse->setEnabled(false);
			ui->configTab->setEnabled(false);
		} else {
			currentConfig = &config[current];
			ui->configure->setEnabled(true);
			ui->configDel->setEnabled(true);
			ui->configDup->setEnabled(true);
			ui->configRen->setEnabled(true);
			ui->gccLable->setEnabled(true);
			ui->gccPath->setEnabled(true);
			ui->gccPath->setText(currentConfig->gccPath);
			ui->gccBrowse->setEnabled(true);
			ui->configTab->setEnabled(true);
			ui->extraCompile->clear();
			ui->extraCompile->insertPlainText(currentConfig->extraCompile);
			ui->extraLink->clear();
			ui->extraLink->insertPlainText(currentConfig->extraLink);
			ui->optimize->setCurrentIndex(currentConfig->optimize);
			ui->cstd->setCurrentIndex(currentConfig->cstd);
			ui->cxxstd->setCurrentIndex(currentConfig->cxxstd);
			ui->bit->setCurrentIndex(currentConfig->bit);
			ui->warning->setCurrentIndex(currentConfig->warning);
			ui->werror->setChecked(currentConfig->werror);
			ui->debug->setChecked(currentConfig->debug);
		}
	});
	connect(ui->configAdd, &QPushButton::clicked, [&]() {
		config.push_back(CompileConfigure());
		ui->configure->addItem(config.back().name);
	});
	connect(ui->configDup, &QPushButton::clicked, [&]() {
		config.insert(current, config[current]);
		ui->configure->insertItem(current, config[current].name);
	});
	connect(ui->configDel, &QPushButton::clicked, [&]() {
		config.removeAt(current);
		ui->configure->removeItem(current);
	});
	connect(ui->configRen, &QPushButton::clicked, [&]() {
		bool ok = false;
		QString name = QInputDialog::getText(this, "qdevcpp - 重命名", "输入配置名称：", QLineEdit::Normal, currentConfig->name, &ok);
		if (ok) {
			currentConfig->name = name;
			ui->configure->setItemText(current, name);
		}
	});
	connect(ui->gccBrowse, &QToolButton::clicked, [&]() {
		QString path = QFileDialog::getOpenFileName(this, "qdevcpp - 打开gcc", "", "gcc (gcc" + exeSuf + ")");
		if (path != "") {
			currentConfig->gccPath = path;
			ui->gccPath->setText(path);
		}
		// gcc -v -x c - -pipe
		// gcc -v -x c++ - -pipe
		// #include <...> search starts here:
		// End of search list.
		// LIBRARY_PATH=
	});
	connect(ui->extraCompile, &QPlainTextEdit::textChanged, [&]() {
		currentConfig->extraCompile = ui->extraCompile->toPlainText();
	});
	connect(ui->extraLink, &QPlainTextEdit::textChanged, [&]() {
		currentConfig->extraLink = ui->extraLink->toPlainText();
	});
#define CONNECT_ENUM(obj) \
	connect(ui->obj, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int idx) { \
		currentConfig->obj = idx; \
	})
	CONNECT_ENUM(optimize);
	CONNECT_ENUM(cstd);
	CONNECT_ENUM(cxxstd);
	CONNECT_ENUM(bit);
	CONNECT_ENUM(warning);
#undef CONNECT_ENUM
#define CONNECT_BOOL(obj) \
	connect(ui->obj, &QCheckBox::stateChanged, [&](int state) { \
		currentConfig->obj = (state == Qt::Checked); \
	})
	CONNECT_BOOL(werror);
	CONNECT_BOOL(debug);
#undef CONNECT_BOOL
	if (config.size()) {
		for (const auto& cfg : config) {
			ui->configure->addItem(cfg.name);
		}
		ui->configure->setCurrentIndex(cur);
	} else {
		current = -1;
	}
}

CompileConfig::~CompileConfig() {
	delete ui;
}
