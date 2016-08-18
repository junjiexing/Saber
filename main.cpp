#include "MainWindow.h"
#include "EventDispatcher.h"

#include <QtFlexStyle.h>

#include <QApplication>
#include <QProcess>

#include <unistd.h>

int main(int argc, char *argv[])
{
    if (getuid() != 0)
    {
        QString aScript("do shell script \"\\\"");
        aScript += argv[0];
        aScript += "\\\"\" with administrator privileges";

        QString osascript = "/usr/bin/osascript";
        QStringList processArguments;
        processArguments << "-l" << "AppleScript";

        QProcess p;
        p.start(osascript, processArguments);
        p.write(aScript.toUtf8());
        p.closeWriteChannel();
        p.waitForReadyRead(-1);
        return 0;
    }

    QApplication a(argc, argv);
    EventDispatcher::registerMetaType();
    QApplication::setStyle(new FlexStyle());

    MainWindow w;
    w.showMaximized();

    return a.exec();
}
