//
// Created by System Administrator on 16/7/24.
//

#include "Log.h"
#include <QDebug>

static std::function<void(QString const &, LogType)> g_logFunc;

void log(QString const &msg, LogType type)
{
    if (!g_logFunc)
    {
        qDebug() << logTypeToString(type) << ":\t" << msg;
        return;
    }

    g_logFunc(msg, type);
}

QString logTypeToString(LogType type)
{
    switch (type)
    {
        case LogType::Info:
            return "Info";
        case LogType::Warning:
            return "Warning";
        case LogType::Error:
            return "Error";
    }
}

void setLogFunc(std::function<void(QString const &, LogType)> func)
{
    g_logFunc = std::move(func);
}
