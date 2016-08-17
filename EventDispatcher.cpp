//
// Created by System Administrator on 16/7/25.
//

#include "EventDispatcher.h"

#include <QVector>

EventDispatcher* EventDispatcher::instance()
{
    static EventDispatcher dispatcher;
    return &dispatcher;
}
void EventDispatcher::registerMetaType()
{
	qRegisterMetaType<uint64_t>("uint64_t");
	qRegisterMetaType<Register>("Register");
	qRegisterMetaType<QVector<int>>("QVector<int>");
}



