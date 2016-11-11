//
// Created by System Administrator on 16/8/10.
//

#include "global.h"
#include "EventDispatcher.h"

uint64_t g_highlightAddress = 0;

task_port_t g_task = 0;
pid_t g_pid = 0;

void log(QString const &msg, LogType t)
{
	emit EventDispatcher::instance()->addLog(msg, t);
}
