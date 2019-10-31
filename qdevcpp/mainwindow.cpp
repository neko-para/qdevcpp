#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QClipboard>
#include <QFileDialog>
#include <QFileInfo>
#include <QRegularExpression>
#include <QMimeData>
#include "global.h"
#include "aboutqdevcpp.h"
#include "compileconfig.h"
#include "editorconfig.h"
#include "environmentconfig.h"
#include "findreplace.h"
#include "subprocess.h"

static QList<CompileConfigure> compileConfig;
static int currentConfigIdx = -1;
static QClipboard* clipboard = nullptr;
static FindReplaceConfig findConfig;

EditorConfigure editorConfig;
MainWindow* window;
CompileConfigure* currentConfig;

CoreEditor* createEditor() {
	CoreEditor* editor = new CoreEditor(nullptr);
	editor->setTabWidth(4);
	editor->setBraceMatching(QsciScintilla::StrictBraceMatch);
	editor->setMargins(2);
	editor->setMarginWidth(1, "00");
	editor->setMarginLineNumbers(1, true);
	editor->setFolding(QsciScintilla::BoxedTreeFoldStyle, 0);
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
	QString cfg = QString("%1/.qdevcpp/qdevcpp.json").arg(QDir::homePath());
	QFile file(cfg);
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
	QString dir = QString("%1/.qdevcpp").arg(QDir::homePath());
	QString cfg = QString("%1/qdevcpp.json").arg(dir);
	QDir().mkpath(dir);
	QFile file(cfg);
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
	for (int i = 0; i < ST_COUNT; ++i) {
		auto& l = status[i];
		l = new QLabel(ui->statusBar);
		ui->statusBar->addWidget(l, 1);
	}
	autoSave = new QTimer(this);
	autoSave->setSingleShot(false);
	connect(autoSave, &QTimer::timeout, [&]() {
		for (auto ei : info.values()) {
			if (!ei->isUntitled()) {
				ei->save();
			}
		}
	});
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
	if (editorConfig.enableAutoSave) {
		autoSave->start(editorConfig.saveInterval * 60 * 1000);
	}
	clipboard = QApplication::clipboard();
	finddlg = new FindReplace(findConfig, this);
	connect(clipboard, &QClipboard::changed, this, &MainWindow::updatePasteAction);
	connect(ui->SrcTab, &QTabWidget::currentChanged, this, &MainWindow::updateTab);
	connect(ui->SrcTab, &QTabWidget::tabCloseRequested, [&](int idx) {
		closeTab(ui->SrcTab->widget(idx));
	});
	connect(ui->actionNew, &QAction::triggered, [&]() {
		CoreEditor* e = createEditor();
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
		open(paths);
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
		closeTab(ui->SrcTab->currentWidget());
	});
	connect(ui->actionCloseAll, &QAction::triggered, [&]() {
		while (ui->SrcTab->count() && closeTab(ui->SrcTab->widget(0))) {
			;
		}
	});
	connect(ui->actionExit, &QAction::triggered, [&]() {
		while (ui->SrcTab->count() && closeTab(ui->SrcTab->widget(0))) {
			;
		}
		if (!ui->SrcTab->count()) {
			close();
		}
	});
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
	connect(ui->actionToggleComment, &QAction::triggered, [&]() {
		auto e = currentEditor();
		int sl, si, el, ei;
		e->getSelection(&sl, &si, &el, &ei);
		if (sl == -1) {
			int l, i;
			e->getCursorPosition(&l, &i);
			sl = el = l;
		}
		e->beginUndoAction();
		e->setSelection(sl, 0, el, e->lineLength(el));
		QString text = e->selectedText();
		QRegularExpression nc(R"(\n(([^/]|/[^/])|$))");
		bool atend = (el + 1 == e->lines());
		if (!atend) {
			text = text.left(text.length() - 1);
		}
		text = "\n" + text;
		if (nc.match(text).hasMatch()) {
			text = text.replace("\n", "\n//");
		} else {
			text = text.replace("\n//", "\n");
		}
		text = text.mid(1);
		if (!atend) {
			text = text + "\n";
		}
		e->replaceSelectedText(text);
		e->setSelection(sl, 0, el, std::max(0, e->lineLength(el) - 1));
		e->endUndoAction();
	});
	connect(ui->actionCopyRow, &QAction::triggered, [&]() {
		auto e = currentEditor();
		int l, i;
		e->getCursorPosition(&l, &i);
		e->beginUndoAction();
		e->setSelection(l, 0, l, e->lineLength(l));
		QString d = e->selectedText();
		if (l + 1 == e->lines()) {
			d += "\n";
		}
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
	connect(ui->actionMoveRowUp, &QAction::triggered, [&]() {
		auto e = currentEditor();
		int l, i;
		e->getCursorPosition(&l, &i);
		int sl, si, el, ei;
		e->getSelection(&sl, &si, &el, &ei);
		if (sl == -1) {
			sl = el = l;
		}
		if (sl == 0) {
			return;
		}
		bool atend = (el + 1 == e->lines());
		e->beginUndoAction();
		e->setSelection(sl, 0, el, e->lineLength(el));
		QString s = e->selectedText();
		if (atend) {
			s += "\n";
		}
		e->removeSelectedText();
		e->insertAt(s, sl - 1, 0);
		if (atend) {
			e->setSelection(el, e->lineLength(el) - 1, el, e->lineLength(el));
			e->removeSelectedText();
		}
		e->setSelection(sl - 1, 0, el - 1, e->lineLength(el - 1) - 1);
		e->endUndoAction();
	});
	connect(ui->actionMoveRowDown, &QAction::triggered, [&]() {
		auto e = currentEditor();
		int l, i;
		e->getCursorPosition(&l, &i);
		int sl, si, el, ei;
		e->getSelection(&sl, &si, &el, &ei);
		if (sl == -1) {
			sl = el = l;
		}
		if (el + 1 == e->lines()) {
			return;
		}
		bool atend = (el + 2 == e->lines());
		e->beginUndoAction();
		e->setSelection(sl, 0, el, e->lineLength(el));
		QString s = e->selectedText();
		if (atend) {
			s = "\n" + s.left(s.length() - 1);
		}
		e->removeSelectedText();
		e->insertAt(s, sl, e->lineLength(sl));
		e->setSelection(sl + 1, 0, el + 1, e->lineLength(el + 1) - (atend ? 0 : 1));
		e->endUndoAction();
	});
	connect(ui->actionIndent, &QAction::triggered, [&]() {
		auto e = currentEditor();
		int sl, si, el, ei;
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
		auto e = currentEditor();
		int sl, si, el, ei;
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
	connect(ui->actionFindReplace, &QAction::triggered, [&]() {
		finddlg->show();
	});
	connect(ui->dockInfo, &QDockWidget::visibilityChanged, ui->actionCompileInfoDock, &QAction::setChecked);
	connect(ui->actionCompileInfoDock, &QAction::toggled, ui->dockInfo, &QDockWidget::setVisible);
	connect(ui->dockDebug, &QDockWidget::visibilityChanged, ui->actionDebugToolDock, &QAction::setChecked);
	connect(ui->actionDebugToolDock, &QAction::toggled, ui->dockDebug, &QDockWidget::setVisible);
	connect(ui->actionStatusBar, &QAction::toggled, ui->statusBar, &QStatusBar::setVisible);
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
	connect(ui->actionEnvironmentConfig, &QAction::triggered, [&]() {
		EnvironmentConfig dlg(this);
		dlg.exec();
	});
	connect(ui->actionEditorConfig, &QAction::triggered, [&]() {
		EditorConfig dlg(editorConfig, this);
		if (QDialog::Accepted == dlg.exec()) {
			editorConfig = dlg.configure();
			for (auto ei : info.values()) {
				ei->updateEditorConfig(editorConfig);
			}
			if (editorConfig.enableAutoSave) {
				autoSave->start(editorConfig.saveInterval * 60 * 1000);
			} else {
				autoSave->stop();
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

CoreEditor* MainWindow::currentEditor() {
	return dynamic_cast<CoreEditor*>(ui->SrcTab->currentWidget());
}

bool MainWindow::closeTab(QWidget* e) {
	auto ei = info[dynamic_cast<CoreEditor*>(e)];
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

void MainWindow::open(const QStringList& paths) {
	for (const auto& p : paths) {
		auto pei = findTab(p);
		if (pei) {
			ui->SrcTab->setCurrentWidget(pei->editor);
			pei->editor->setFocus();
		} else {
			CoreEditor* e = createEditor();
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
}

void setEnabled(bool e, QList<QAction*> actions) {
	for (auto a : actions) {
		a->setEnabled(e);
	}
}

void MainWindow::updateTab(int idx) {
	if (idx == -1) {
		::setEnabled(false, {
						 ui->actionSave,
						 ui->actionSaveAs,
						 ui->actionClose,
						 ui->actionCloseAll,
						 ui->actionUndo,
						 ui->actionRedo,
						 ui->actionCopy,
						 ui->actionCut,
						 ui->actionPaste,
						 ui->actionSelectAll,
						 ui->actionToggleComment,
						 ui->actionCopyRow,
						 ui->actionDelRow,
						 ui->actionIndent,
						 ui->actionUnindent,
						 ui->actionMoveRowUp,
						 ui->actionMoveRowDown,
					 });
		for (auto l : status) {
			l->setVisible(false);
		}
		finddlg->setEditorInfo(nullptr);
	} else {
		::setEnabled(true, {
						 ui->actionSave,
						 ui->actionSaveAs,
						 ui->actionClose,
						 ui->actionCloseAll,
						 ui->actionPaste,
						 ui->actionSelectAll,
						 ui->actionToggleComment,
						 ui->actionCopyRow,
						 ui->actionDelRow,
						 ui->actionMoveRowUp,
						 ui->actionMoveRowDown,
					 });
		EditorInfo* ei = info[currentEditor()];
		ei->updateUndoRedoState();
		ei->updateSelectionState();
		finddlg->setEditorInfo(ei);
		for (auto l : status) {
			l->setVisible(true);
		}
		ei->updateStatusInfo();
	}
	updateCompileActions();
	updatePasteAction();
	updateWindowTitle();

}

void MainWindow::updateCompileActions() {
	bool enabled = currentEditor();
	ui->actionRun->setEnabled(enabled);
//	ui->actionClean->setEnabled(enabled);
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

void MainWindow::closeEvent(QCloseEvent* e) {
	while (ui->SrcTab->count() && closeTab(dynamic_cast<CoreEditor*>(ui->SrcTab->widget(0)))) {
		;
	}
	if (ui->SrcTab->count()) {
		e->ignore();
	} else {
		e->accept();
	}
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e) {
	e->acceptProposedAction();
	e->accept();
}

void MainWindow::dropEvent(QDropEvent* e) {
	QList<QUrl> urls = e->mimeData()->urls();
	QStringList paths;
	for (auto u : urls) {
		paths.push_back(u.toLocalFile());
	}
	open(paths);
	e->accept();
}
