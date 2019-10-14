#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <Qsci/qscilexercpp.h>
#include <QJsonDocument>
#include <QJsonArray>
#include <QEventLoop>
#include "global.h"
#include "compileconfig.h"
#include "subprocess.h"
#include "aboutqdevcpp.h"

static QList<CompileConfigure> config;
static int currentConfigIdx;
CompileConfigure* currentConfig;

void MainWindow::updateTab(int idx) {
	if (idx == -1) {
		ui->actionSave->setEnabled(false);
		ui->actionSaveAs->setEnabled(false);
		ui->actionClose->setEnabled(false);
		ui->actionCloseAll->setEnabled(false);
		ui->actionUndo->setEnabled(false);
		ui->actionRedo->setEnabled(false);
	} else {
		ui->actionSave->setEnabled(true);
		ui->actionSaveAs->setEnabled(true);
		ui->actionClose->setEnabled(true);
		ui->actionCloseAll->setEnabled(true);
		info[currentEditor()]->updateUndoRedoState();
	}
	updateCompileActions();
}

QsciLexerCPP* createLexer() {
	QsciLexerCPP* lexer = new QsciLexerCPP;
	lexer->setFont(QFont("ubuntu mono"));
	return lexer;
}

QsciScintilla* createEditor(QWidget* parent) {
	QsciScintilla* editor = new QsciScintilla(parent);
	editor->setLexer(createLexer());
	editor->setTabWidth(4);
	editor->setMarginWidth(0, "000000");
	editor->setMarginLineNumbers(0, true);
	return editor;
}

void loadConfig() {
	QFile file("qdevcpp.json");
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}
	QJsonDocument doc;
	doc = QJsonDocument::fromJson(file.readAll());
	file.close();
	QJsonObject obj = doc.object();
	QJsonArray compileconfig = obj["CompileConfig"].toArray();
	for (auto it = compileconfig.begin(); it != compileconfig.end(); ++it) {
		CompileConfigure cfg;
		cfg.fromJson(it->toObject());
		config.push_back(cfg);
	}
	if (config.size()) {
		currentConfigIdx = obj["CurrentCompileConfig"].toInt();
		currentConfig = &config[currentConfigIdx];
	} else {
		currentConfigIdx = -1;
		currentConfig = nullptr;
	}
}

void saveConfig() {
	QFile file("qdevcpp.json");
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		return;
	}
	QJsonObject obj;
	QJsonArray compileconfig;
	for (const auto& cfg : config) {
		compileconfig.append(cfg.toJson());
	}
	obj.insert("CompileConfig", compileconfig);
	obj.insert("CurrentCompileConfig", currentConfigIdx);
	QJsonDocument doc(obj);
	file.write(doc.toJson());
	file.close();
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
	ui->setupUi(this);
	ui->compileResult->horizontalHeader()->setStretchLastSection(true);
	ui->compileResult->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->compileResult->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	loadConfig();
	connect(ui->actionUndo, &QAction::triggered, [&]() {
		currentEditor()->undo();
	});
	connect(ui->actionRedo, &QAction::triggered, [&]() {
		currentEditor()->redo();
	});
	connect(ui->SrcTab, &QTabWidget::currentChanged, this, &MainWindow::updateTab);
	connect(ui->actionNew, &QAction::triggered, [&]() {
		QsciScintilla* e = createEditor(ui->SrcTab);
		auto& ei = info[e];
		ei = new EditorInfo(e, ui);
		ei->generateUntitled();
		ui->SrcTab->addTab(e, ei->generateTitle());
		ui->SrcTab->setCurrentWidget(e);
	});
	connect(ui->actionOpen, &QAction::triggered, [&]() {
		QStringList paths = QFileDialog::getOpenFileNames(this, "qdevcpp - 打开", "", "所有支持类型 (*.c *.cpp *.cc *.cxx *.h *.hpp);;所有文件 (*.*)");
		if (!paths.size()) {
			return;
		}
		for (const auto& p : paths) {
			QsciScintilla* e = createEditor(ui->SrcTab);
			auto& ei = info[e];
			ei = new EditorInfo(e, ui);
			if (ei->open(p)) {
				ui->SrcTab->addTab(e, ei->generateTitle());
				ui->SrcTab->setCurrentWidget(e);
			} else {
				ei->deleteLater();
				info.remove(e);
			}
		}
	});
	connect(ui->actionSave, &QAction::triggered, [&]() {
		info[currentEditor()]->save();
	});
	connect(ui->actionSaveAs, &QAction::triggered, [&]() {
		info[currentEditor()]->saveas();
	});
	connect(ui->actionClose, &QAction::triggered, [&]() {
		closeTab(currentEditor());
	});
	connect(ui->actionCloseAll, &QAction::triggered, [&]() {
		while (ui->SrcTab->count() && closeTab(dynamic_cast<QsciScintilla*>(ui->SrcTab->widget(0)))) {
			;
		}
	});
	connect(ui->actionExit, &QAction::triggered, [&]() {
		while (ui->SrcTab->count() && closeTab(dynamic_cast<QsciScintilla*>(ui->SrcTab->widget(0)))) {
			;
		}
		if (!ui->SrcTab->count()) {
			close();
		}
	});
	connect(ui->SrcTab, &QTabWidget::tabCloseRequested, [&](int idx) {
		closeTab(dynamic_cast<QsciScintilla*>(ui->SrcTab->widget(idx)));
	});
	connect(ui->dockInfo, &QDockWidget::visibilityChanged, ui->actionCompileInfoDock, &QAction::setChecked);
	connect(ui->actionCompileInfoDock, &QAction::toggled, ui->dockInfo, &QDockWidget::setVisible);
	connect(ui->dockDebug, &QDockWidget::visibilityChanged, ui->actionDebugToolDock, &QAction::setChecked);
	connect(ui->actionDebugToolDock, &QAction::toggled, ui->dockDebug, &QDockWidget::setVisible);
	connect(ui->actionStatusBar, &QAction::toggled, ui->StatusBar, &QStatusBar::setVisible);
	connect(ui->actionCompile, &QAction::triggered, [&]() {
		auto ei = info[currentEditor()];
		if (!ei->save()) {
			return;
		}
		ui->compileInfo->setCurrentIndex(ei->compile());
	});
	connect(ui->actionRun, &QAction::triggered, [&]() {
		info[currentEditor()]->run();
	});
	connect(ui->actionCompileRun, &QAction::triggered, [&]() {
		auto ei = info[currentEditor()];
		if (!ei->save()) {
			return;
		}
		if (ei->compile()) {
			ei->run();
		}

	});
	connect(ui->actionCompileConfig, &QAction::triggered, [&]() {
		CompileConfig dlg(config, currentConfigIdx, this);
		if (QDialog::Accepted == dlg.exec()) {
			config = dlg.configure();
			currentConfigIdx = dlg.currentConfigure();
			if (currentConfigIdx != -1) {
				currentConfig = &config[currentConfigIdx];
			} else {
				currentConfig = nullptr;
			}
			updateCompileActions();
		}
	});
	connect(ui->actionAboutQDevCpp, &QAction::triggered, [&]() {
		AboutQDevCpp dlg(this);
		dlg.exec();
	});
	connect(ui->actionAboutQt, &QAction::triggered, &QApplication::aboutQt);
}

MainWindow::~MainWindow() {
	saveConfig();
	delete ui;
}

QsciScintilla* MainWindow::currentEditor() {
	return dynamic_cast<QsciScintilla*>(ui->SrcTab->currentWidget());
}

bool MainWindow::closeTab(QsciScintilla* e) {
	auto ei = info[e];
	if (!ei->askSave()) {
		return false;
	}
	ui->SrcTab->removeTab(ui->SrcTab->indexOf(e));
	updateTab(ui->SrcTab->currentIndex());
	ei->deleteLater();
	info.remove(e);
	return true;
}

void MainWindow::updateCompileActions() {
	bool enabled = currentEditor();
	ui->actionRun->setEnabled(enabled);
	ui->actionClean->setEnabled(enabled);
	enabled = enabled && (currentConfigIdx != -1);
	ui->actionCompile->setEnabled(enabled);
	ui->actionCompileRun->setEnabled(enabled);
}
