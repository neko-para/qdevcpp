#ifndef ENVIRONMENTCONFIG_H
#define ENVIRONMENTCONFIG_H

#include <QDialog>

namespace Ui {
	class EnvironmentConfig;
}

struct LinkTool {
	static bool installed();
	static bool install();
	static bool query(const QString& mime);
	static bool set(const QString& mime);
};

class EnvironmentConfig : public QDialog {
	Q_OBJECT

public:
	explicit EnvironmentConfig(QWidget *parent = 0);
	~EnvironmentConfig();

private:
	Ui::EnvironmentConfig *ui;
	bool installed;
};

#endif // ENVIRONMENTCONFIG_H
