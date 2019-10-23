#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	void enableButtons(bool e) const;
	bool updateTo(QString ver);

	bool download(const QString& url, QByteArray& data);

private:
	Ui::MainWindow *ui;
	bool getInfo = false;
	QNetworkAccessManager* manager;
	QNetworkReply* reply;
	QJsonObject info;
};

#endif // MAINWINDOW_H
