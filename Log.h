//
// Created by System Administrator on 16/7/24.
//

#pragma once

#include <QString>
#include <functional>

enum class LogType
{
    Info,
    Warning,
    Error
};

QString logTypeToString(LogType type);

void setLogFunc(std::function<void(QString const &, LogType)> func);

void log(QString const & msg, LogType type = LogType::Info);
