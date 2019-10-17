#ifndef CONFIGHELP_H
#define CONFIGHELP_H

#define JSON_SET(key) \
	do { \
		JSON_OBJ.insert(#key, QJsonValue(key)); \
	} while (false)

#define JSON_GET(key) \
	do { \
		if (JSON_OBJ.contains(#key)) { \
			key = JSON_OBJ[#key].toVariant().value<decltype(key)>(); \
		} \
	} while (false)

#endif // CONFIGHELP_H
