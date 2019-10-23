#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QJsonDocument>
#include <QMessageBox>
#include <QFile>
#include <QCryptographicHash>

#include <QDebug>

QString rootRawUrl = "https://raw.githubusercontent.com/neko-para/qdevcpp";
QString rootReleaseUrl = "https://github.com/neko-para/qdevcpp/releases/download";

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
	ui->setupUi(this);
	QFile cfg("qdevcpp.release");
	if (cfg.open(QIODevice::ReadOnly)) {
		ui->currentVer->setText(QString::fromUtf8(cfg.readAll()));
		cfg.close();
	}
	manager = new QNetworkAccessManager(this);
	connect(ui->updateInfo, &QPushButton::clicked, [&]() {
		QByteArray data;
		if (download(QString("%1/master/RELEASE.json").arg(rootRawUrl), data)) {
			info = QJsonDocument::fromJson(data).object();
			ui->stableVer->setText(info["stable"].toString());
			ui->developVer->setText(info["develop"].toString());
			getInfo = true;
		}
	});
	connect(ui->updateStable, &QPushButton::clicked, [&]() {
		QMessageBox::warning(this, "qdevcpp updater", QString("更新%1").arg(updateTo(info["stable"].toString()) ? "成功" : "失败"));
	});
	connect(ui->updateDevelop, &QPushButton::clicked, [&]() {
		QMessageBox::warning(this, "qdevcpp updater", QString("更新%1").arg(updateTo(info["develop"].toString()) ? "成功" : "失败"));
	});
}

MainWindow::~MainWindow() {
	delete manager;
	delete ui;
}

QByteArray md5(QString path) {
	QFile file(path);
	file.open(QIODevice::ReadOnly);
	return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Md5);
}

bool MainWindow::updateTo(QString ver) {
	QJsonObject index = info["file"].toObject()[ver].toObject();
	for (auto k : index.keys()) {
		if (!QFile(k).exists() || md5(k) != QByteArray::fromHex(index[k].toString().toUtf8())) {
			QByteArray data;
			if (!download(QString("%1/%2/%3").arg(rootReleaseUrl, ver, k), data)) {
				return false;
			}
			QFile file(k);
			if (!file.open(QIODevice::WriteOnly)) {
				return false;
			}
			file.write(data);
			file.close();
		}
	}
	return true;
}

void MainWindow::enableButtons(bool e) const {
	ui->updateInfo->setEnabled(e);
	ui->updateStable->setEnabled(e);
	ui->updateDevelop->setEnabled(e);
}

bool MainWindow::download(const QString& url, QByteArray& data) {
	enableButtons(false);
	reply = manager->get(QNetworkRequest(QUrl(url)));
	ui->downloadUrl->setText(reply->url().toString());
	ui->downloadProgress->setValue(0);
	connect(reply, &QNetworkReply::downloadProgress, [&](qint64 r, qint64 t) {
		ui->downloadProgress->setValue(r);
		ui->downloadProgress->setMaximum(t);
	});
	QEventLoop loop(this);
	connect(reply, &QNetworkReply::finished, [&]() {
		ui->downloadProgress->setValue(ui->downloadProgress->maximum());
		enableButtons(true);
		data = reply->readAll();
		reply->deleteLater();
		loop.quit();
	});
	connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), [&](QNetworkReply::NetworkError) {
		QMessageBox::critical(this, "qdevcpp updater", QString("%1").arg(reply->errorString()));
		enableButtons(true);
		reply->deleteLater();
		loop.exit(1);
	});
	return !loop.exec();

}
