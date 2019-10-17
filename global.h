#ifndef GLOBAL_H
#define GLOBAL_H

#include <qsystemdetection.h>
#include <QString>

#if defined(Q_OS_WIN32)
const QChar pathSep = ';';
const QString exeSuf = ".exe";
#elif defined(Q_OS_LINUX)
const QChar pathSep = ':';
const QString exeSuf = "";
#endif

class MainWindow;

const QStringList cSuf = {"c"};
const QStringList cxxSuf = {"cpp", "cxx", "cc"};
extern MainWindow* window;

#endif // GLOBAL_H
