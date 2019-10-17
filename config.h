#ifndef CONFIG_H
#define CONFIG_H

#include <QJsonValue>

struct Config {
	virtual QJsonValue toJson() const = 0;
	virtual void fromJson(QJsonValue value) = 0;
};

#endif // CONFIG_H
