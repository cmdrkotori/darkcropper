#-------------------------------------------------
#
# Project created by QtCreator 2016-05-19T22:59:57
#
#-------------------------------------------------

QT       += core gui widgets opengl

TARGET = darkcropper
TEMPLATE = app
CONFIG += C++11

SOURCES += main.cpp\
        mainwindow.cpp \
    imagewindow.cpp

HEADERS  += mainwindow.h \
    imagewindow.h

FORMS    += mainwindow.ui

DISTFILES += \
    .gitignore \
    LICENSE

RESOURCES += \
    resources.qrc
