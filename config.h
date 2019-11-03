#ifndef CONFIG_H
#define CONFIG_H

#define QDEVCPP_VERSION_MAJOR 0
#define QDEVCPP_VERSION_MINOR 1

#include <QJsonValue>

struct Config {
	virtual ~Config() {}
	virtual QJsonValue toJson() const = 0;
	virtual void fromJson(QJsonValue value) = 0;
};

#endif // CONFIG_H
