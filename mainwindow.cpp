#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QClipboard>
#include "global.h"
#include "compileconfig.h"
#include "findreplace.h"
#include "subprocess.h"
#include "aboutqdevcpp.h"

static QList<CompileConfigure> compileConfig;
static int currentConfigIdx = -1;
static QClipboard* clipboard = nullptr;
static FindReplaceConfig findConfig;

MainWindow* window;
CompileConfigure* currentConfig;

QsciScintilla* createEditor(QWidget* parent) {
	QsciScintilla* editor = new QsciScintilla(parent);
	editor->setTabWidth(4);
	editor->setMarginWidth(0, "000000");
	editor->setMarginLineNumbers(0, true);
	editor->installEventFilter(window);
	return editor;
}

template <typename Type>
void loadMultiConfig(QList<Type>& config, QJsonArray array) {
	config.clear();
	for (auto it = array.begin(); it != array.end(); ++it) {
		Type cfg;
		cfg.fromJson(*it);
		config.push_back(cfg);
	}
}

template <typename Type>
QJsonArray saveMultiConfig(const QList<Type>& config) {
	QJsonArray array;
	for (const auto& cfg : config) {
		array.append(cfg.toJson());
	}
	return array;
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
	loadMultiConfig(compileConfig, obj["CompileConfig"].toArray());
	findConfig.fromJson(obj["FindConfig"]);
	if (compileConfig.size()) {
		currentConfigIdx = obj["CurrentCompileConfig"].toInt();
		currentConfig = &compileConfig[currentConfigIdx];
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
	obj.insert("CompileConfig", saveMultiConfig(compileConfig));
	obj.insert("CurrentCompileConfig", currentConfigIdx);
	obj.insert("FindConfig", findConfig.toJson());
	QJsonDocument doc(obj);
	file.write(doc.toJson());
	file.close();
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
	::window = this;
	ui->setupUi(this);
	ui->compileResult->horizontalHeader()->setStretchLastSection(true);
	ui->compileResult->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->compileResult->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	ui->compileResult->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	ui->compileResult->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	loadConfig();
	clipboard = QApplication::clipboard();
	finddlg = new FindReplace(findConfig, this);
	connect(clipboard, &QClipboard::changed, this, &MainWindow::updatePasteAction);
	connect(ui->actionUndo, &QAction::triggered, [&]() {
		currentEditor()->undo();
	});
	connect(ui->actionRedo, &QAction::triggered, [&]() {
		currentEditor()->redo();
	});
	connect(ui->actionCopy, &QAction::triggered, [&]() {
		currentEditor()->copy();
	});
	connect(ui->actionCut, &QAction::triggered, [&]() {
		currentEditor()->cut();
	});
	connect(ui->actionPaste, &QAction::triggered, [&]() {
		currentEditor()->paste();
	});
	connect(ui->SrcTab, &QTabWidget::currentChanged, this, &MainWindow::updateTab);
	connect(ui->actionNew, &QAction::triggered, [&]() {
		QsciScintilla* e = createEditor(ui->SrcTab);
		auto& ei = info[e];
		ei = new EditorInfo(e, ui);
		ui->SrcTab->addTab(e, ei->generateTitle());
		ui->SrcTab->setCurrentWidget(e);
	});
	connect(ui->actionOpen, &QAction::triggered, [&]() {
		QStringList paths = QFileDialog::getOpenFileNames(this, "qdevcpp - 打开", "", "所有支持类型 (*.c *.cpp *.cc *.cxx *.h *.hpp);;所有文件 (*.*)");
		if (!paths.size()) {
			return;
		}
		for (const auto& p : paths) {
			auto pei = findTab(p);
			if (pei) {
				ui->SrcTab->setCurrentWidget(pei->editor);
				pei->editor->setFocus();
			} else {
				QsciScintilla* e = createEditor(ui->SrcTab);
				auto& ei = info[e];
				ei = new EditorInfo(e, ui);
				if (ei->open(p)) {
					ui->SrcTab->addTab(e, ei->generateTitle());
					ui->SrcTab->setCurrentWidget(e);
				} else {
					ei->deleteLater();
				}
			}
		}
	});
	connect(ui->actionSave, &QAction::triggered, [&]() {
		auto ei = info[currentEditor()];
		if (!ei->save()) {
			return;
		}
		removeOther(ei);
	});
	connect(ui->actionSaveAs, &QAction::triggered, [&]() {
		auto ei = info[currentEditor()];
		if (!ei->saveas()) {
			return;
		}
		removeOther(ei);
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
	connect(ui->actionFindReplace, &QAction::triggered, [&]() {
		finddlg->show();
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
		removeOther(ei);
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
		removeOther(ei);
		if (ei->compile()) {
			ei->run();
		}

	});
	connect(ui->actionCompileConfig, &QAction::triggered, [&]() {
		CompileConfig dlg(compileConfig, currentConfigIdx, this);
		if (QDialog::Accepted == dlg.exec()) {
			compileConfig = dlg.configure();
			currentConfigIdx = dlg.currentConfigure();
			if (currentConfigIdx != -1) {
				currentConfig = &compileConfig[currentConfigIdx];
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
	connect(ui->compileResult, &QTableWidget::doubleClicked, [&](const QModelIndex& idx) {
		QString file, row, col;
		file = ui->compileResult->item(idx.row(), 0)->text();
		row = ui->compileResult->item(idx.row(), 1)->text();
		col = ui->compileResult->item(idx.row(), 2)->text();
		if (QFileInfo(file).exists()) {
			auto ei = findTab(file);
			if (!ei) {
				return;
			}
			ui->SrcTab->setCurrentWidget(ei->editor);
			ei->editor->setFocus();
			int nrow = 0, ncol = 0;
			if (row.length()) {
				nrow = row.toInt() - 1;
				if (col.length()) {
					ncol = col.toInt() - 1;
				}
				ei->editor->setCursorPosition(nrow, ncol);
			}
		}
	});
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
	return true;
}

EditorInfo* MainWindow::findTab(const QString& path, EditorInfo* except) {
	for (auto ei : info.values()) {
		if (except != ei && ei->path == path) {
			return ei;
		}
	}
	return nullptr;
}

void MainWindow::removeOther(EditorInfo* ei) {
	auto pei = findTab(ei->path, ei);
	if (pei) {
		ui->SrcTab->removeTab(ui->SrcTab->indexOf(pei->editor));
		pei->deleteLater();
	}
}

void MainWindow::updateTab(int idx) {
	if (idx == -1) {
		ui->actionSave->setEnabled(false);
		ui->actionSaveAs->setEnabled(false);
		ui->actionClose->setEnabled(false);
		ui->actionCloseAll->setEnabled(false);
		ui->actionUndo->setEnabled(false);
		ui->actionRedo->setEnabled(false);
		ui->actionCopy->setEnabled(false);
		ui->actionCut->setEnabled(false);
		finddlg->setEditorInfo(nullptr);
	} else {
		ui->actionSave->setEnabled(true);
		ui->actionSaveAs->setEnabled(true);
		ui->actionClose->setEnabled(true);
		ui->actionCloseAll->setEnabled(true);
		EditorInfo* ei = info[currentEditor()];
		ei->updateUndoRedoState();
		ei->updateCopyCutState();
		finddlg->setEditorInfo(ei);
	}
	updateCompileActions();
	updatePasteAction();
	updateWindowTitle();

}

void MainWindow::updateCompileActions() {
	bool enabled = currentEditor();
	ui->actionRun->setEnabled(enabled);
	ui->actionClean->setEnabled(enabled);
	enabled = enabled && (currentConfigIdx != -1);
	ui->actionCompile->setEnabled(enabled);
	ui->actionCompileRun->setEnabled(enabled);
}

void MainWindow::updatePasteAction() {
	ui->actionPaste->setEnabled(currentEditor() && clipboard->text().length());
}

void MainWindow::updateWindowTitle() {
	if (currentEditor()) {
		EditorInfo* ei = info[currentEditor()];
		setWindowTitle(QString("%1 - QDevCpp").arg(ei->generateName()));
	} else {
		setWindowTitle("QDevCpp");
	}
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
	QsciScintilla* e = dynamic_cast<QsciScintilla*>(watched);
	if (event->type() == QEvent::FocusIn && e) {
		auto ei = info[e];
		if (ei->isModifiedByOthers()) {
			ei->reload();
		}
	}
	return QMainWindow::eventFilter(watched, event);
}
