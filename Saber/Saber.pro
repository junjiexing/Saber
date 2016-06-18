#-------------------------------------------------
#
# Project created by QtCreator 2016-06-17T10:53:23
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Saber
TEMPLATE = app

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../QtFlex5/release/ -lQt5Flex
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../QtFlex5/debug/ -lQt5Flexd
else:unix:CONFIG(release,debug|release): LIBS += -L$$OUT_PWD/../QtFlex5/ -lQt5Flex
else:unix:CONFIG(debug,debug|release): LIBS += -L$$OUT_PWD/../QtFlex5/ -lQt5Flexd

INCLUDEPATH += $$PWD/../QtFlex5
DEPENDPATH += $$PWD/../QtFlex5

SOURCES += main.cpp\
        MainWidget.cpp

HEADERS  += MainWidget.h
