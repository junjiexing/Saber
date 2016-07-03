#-------------------------------------------------
#
# Project created by QtCreator 2016-06-17T10:53:23
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Saber
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../QtFlex5/release/ -lQt5Flex
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../QtFlex5/debug/ -lQt5Flexd
else:unix:CONFIG(release,debug|release): LIBS += -L$$OUT_PWD/../QtFlex5/ -lQt5Flex
else:unix:CONFIG(debug,debug|release): LIBS += -L$$OUT_PWD/../QtFlex5/ -lQt5Flexd

LIBS += -framework CoreFoundation

QMAKE_POST_LINK += install_name_tool -change libQt5Flexd.1.dylib @executable_path/libQt5Flexd.dylib $$OUT_PWD/Saber.app/Contents/MacOS/Saber

INCLUDEPATH += $$PWD/../QtFlex5
DEPENDPATH += $$PWD/../QtFlex5

SOURCES += main.cpp\
        MainWidget.cpp \
    DebugCore.cpp \
    RegisterModel.cpp

HEADERS  += MainWidget.h \
    DebugCore.h \
    Common.h \
    RegisterModel.h

FORMS +=
