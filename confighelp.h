#ifndef CONFIGHELP_H
#define CONFIGHELP_H

#define JSON_SET(key) \
	do { \
		obj.insert(#key, QJsonValue(key)); \
	} while (false)

#define JSON_GET(key) \
	do { \
		if (obj.contains(#key)) { \
			key = obj[#key].toVariant().value<decltype(key)>(); \
		} \
	} while (false)

#define BIND_CONFIG_BOOL(key) \
	do { \
		connect(ui->key, &QCheckBox::toggled, [&](bool c) { \
			config.key = c; \
		}); \
		ui->key->setChecked(config.key); \
	} while (false)

#define BIND_CONFIG_ENUM(key) \
	do { \
		connect(ui->key, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int i) { \
			config.key = i; \
		}); \
		ui->key->setCurrentIndex(config.key); \
	} while (false)

#define BIND_CONFIG_INT(key, type) \
	do { \
		connect(ui->key, QOverload<int>::of(&type::valueChanged), [&](int i) { \
			config.key = i; \
		}); \
		ui->key->setValue(config.key); \
	} while (false)

#endif // CONFIGHELP_H
