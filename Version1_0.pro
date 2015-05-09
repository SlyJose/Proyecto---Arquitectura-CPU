#-------------------------------------------------
#
# Project created by QtCreator 2015-04-21T11:16:38
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Version1_0
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    principalthread.cpp \
    info.cpp

HEADERS  += mainwindow.h \
    principalthread.h \
    info.h

FORMS    += mainwindow.ui \
    info.ui
