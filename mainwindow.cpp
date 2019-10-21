#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QClipboard>
#include "global.h"
#include "compileconfig.h"
#include "editorconfig.h"
#include "findreplace.h"
#include "subprocess.h"
#include "aboutqdevcpp.h"

static EditorConfigure editorConfig;
static QList<CompileConfigure> compileConfig;
static int currentConfigIdx = -1;
static QClipboard* clipboard = nullptr;
static FindReplaceConfig findConfig;

MainWindow* window;
CompileConfigure* currentConfig;

QsciScintilla* createEditor(QWidget* parent) {
	QsciScintilla* editor = new QsciScintilla(parent);
	editor->setTabWidth(4);
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
	editorConfig.fromJson(obj["EditorConfig"]);
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
	obj.insert("EditorConfig", editorConfig.toJson());
	obj.insert("FindConfig", findConfig.toJson());
	QJsonDocument doc(obj);
	file.write(doc.toJson(QJsonDocument::Compact));
	file.close();
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
	::window = this;
	ui->setupUi(this);
	// Disable Debug
	ui->dockDebug->setVisible(false);
	ui->actionDebug->setEnabled(false);
	ui->actionDebugToolDock->setEnabled(false);
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
	connect(ui->actionSelectAll, &QAction::triggered, [&]() {
		currentEditor()->selectAll();
	});
	connect(ui->actionCopyRow, &QAction::triggered, [&]() {
		auto e = currentEditor();
		int l, i;
		e->getCursorPosition(&l, &i);
		e->beginUndoAction();
		e->setSelection(l, 0, l, e->lineLength(l));
		QString d = e->selectedText();
		e->insertAt(d, l, 0);
		e->setCursorPosition(l, i);
		e->endUndoAction();
	});
	connect(ui->actionDelRow, &QAction::triggered, [&]() {
		auto e = currentEditor();
		int l, i;
		e->getCursorPosition(&l, &i);
		e->beginUndoAction();
		if (l + 1 == e->lines()) {
			if (l) {
				e->setSelection(l - 1, e->lineLength(l - 1) - 1, l, e->lineLength(l));
			} else {
				e->setSelection(0, 0, 0, e->lineLength(0));
			}
		} else {
			e->setSelection(l, 0, l + 1, 0);
		}
		e->removeSelectedText();
		e->endUndoAction();
	});
	connect(ui->actionIndent, &QAction::triggered, [&]() {
		int sl, si, el, ei;
		auto e = currentEditor();
		e->getSelection(&sl, &si, &el, &ei);
		if (sl != el) {
			e->beginUndoAction();
			si = 0;
			if (ei) {
				if (e->lines() == el + 1) {
					ei = e->lineLength(el);
				} else {
					++el;
					ei = 0;
				}
			}
			for (int i = el; i >= sl; --i) {
				e->indent(i);
			}
			e->endUndoAction();
			e->setSelection(sl, 0, el, ei);
		}
	});
	connect(ui->actionUnindent, &QAction::triggered, [&]() {
		int sl, si, el, ei;
		auto e = currentEditor();
		e->getSelection(&sl, &si, &el, &ei);
		if (sl != el) {
			e->beginUndoAction();
			si = 0;
			if (ei) {
				if (e->lines() == el + 1) {
					ei = e->lineLength(el);
				} else {
					++el;
					ei = 0;
				}
			}
			for (int i = el; i >= sl; --i) {
				e->unindent(i);
			}
			e->endUndoAction();
			e->setSelection(sl, 0, el, ei);
		}
	});
	connect(ui->SrcTab, &QTabWidget::currentChanged, this, &MainWindow::updateTab);
	connect(ui->actionNew, &QAction::triggered, [&]() {
		QsciScintilla* e = createEditor(ui->SrcTab);
		auto& ei = info[e];
		ei = new EditorInfo(e, ui);
		ei->updateEditorConfig(editorConfig);
		ui->SrcTab->addTab(e, ei->generateTitle());
		ui->SrcTab->setCurrentWidget(e);
		e->setFocus();
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
					ei->updateEditorConfig(editorConfig);
					ui->SrcTab->addTab(e, ei->generateTitle());
					ui->SrcTab->setCurrentWidget(e);
					e->setFocus();
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
	connect(ui->actionEditorConfig, &QAction::triggered, [&]() {
		EditorConfig dlg(editorConfig, this);
		if (QDialog::Accepted == dlg.exec()) {
			editorConfig = dlg.configure();
			for (auto ei : info.values()) {
				ei->updateEditorConfig(editorConfig);
			}
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
		ui->actionSelectAll->setEnabled(false);
		finddlg->setEditorInfo(nullptr);
	} else {
		ui->actionSave->setEnabled(true);
		ui->actionSaveAs->setEnabled(true);
		ui->actionClose->setEnabled(true);
		ui->actionCloseAll->setEnabled(true);
		ui->actionSelectAll->setEnabled(true);
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
	if (e) {
		if (event->type() == QEvent::FocusIn) {
			auto ei = info[e];
			if (ei->isModifiedByOthers()) {
				ei->reload();
			}
		} else if (event->type() == QEvent::KeyPress) {
			QKeyEvent* ke = dynamic_cast<QKeyEvent*>(event);
			if (ke->key() == Qt::Key_D && ke->modifiers() == Qt::ControlModifier) {
				ui->actionDelRow->trigger();
				ke->accept();
				return true;
			}
		}
	}
	return QMainWindow::eventFilter(watched, event);
}

void MainWindow::closeEvent(QCloseEvent* e) {
	while (ui->SrcTab->count() && closeTab(dynamic_cast<QsciScintilla*>(ui->SrcTab->widget(0)))) {
		;
	}
	if (ui->SrcTab->count()) {
		e->ignore();
	} else {
		e->accept();
	}
}
