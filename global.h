//
// Created by System Administrator on 16/8/10.
//

#pragma once

#include "Common.h"

#include <cstdint>

extern uint64_t g_highlightAddress;
extern task_port_t g_task;
extern pid_t g_pid;

void log(QString const & msg, LogType t = LogType::Info);