#include "MainWidget.h"
#include <QApplication>
#include <QtFlexStyle.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setStyle(new FlexStyle());

    MainWidget w;
    w.showMaximized();

    return a.exec();
}
