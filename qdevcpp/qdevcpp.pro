#-------------------------------------------------
#
# Project created by QtCreator 2019-10-05T14:14:53
#
#-------------------------------------------------

QT       += core gui
CONFIG += qscintilla2

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qdevcpp
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    compileconfig.cpp \
    subprocess.cpp \
    editorinfo.cpp \
    aboutqdevcpp.cpp \
    findreplace.cpp \
    editorconfig.cpp

HEADERS += \
        mainwindow.h \
    compileconfig.h \
    global.h \
    subprocess.h \
    editorinfo.h \
    aboutqdevcpp.h \
    findreplace.h \
    config.h \
    confighelp.h \
    editorconfig.h

FORMS += \
        mainwindow.ui \
    compileconfig.ui \
    aboutqdevcpp.ui \
    findreplace.ui \
    editorconfig.ui

RESOURCES += \
    resource.qrc

DISTFILES +=

win32:RC_ICONS = qdevcpp.ico
