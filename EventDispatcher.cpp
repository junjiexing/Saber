//
// Created by System Administrator on 16/7/25.
//

#include "EventDispatcher.h"
#include <QApplication>
#include "Log.h"

const QEvent::Type SetDisasmAddressEvent::eventType = (QEvent::Type)QEvent::registerEventType();
const QEvent::Type SetDebugCoreEvent::eventType = (QEvent::Type)QEvent::registerEventType();

EventDispatcher* EventDispatcher::instance()
{
    static EventDispatcher dispatcher;
    return &dispatcher;
}

EventDispatcher::EventDispatcher()
    :QObject(nullptr)
{}

void EventDispatcher::setDisasmAddress(uint64_t addr)
{
    log("setDisasmAddress");
    std::lock_guard<std::mutex> _(m_mtx);

    auto event = new SetDisasmAddressEvent(addr);
    auto set = m_recivers[SetDisasmAddressEvent::eventType];
    for (auto it : set)
    {
        log("sendEvent SetDisasmAddressEvent");
        QApplication::sendEvent(it, event);
    }
}

void EventDispatcher::setDebugCore(DebugCore* debugCore)
{
    log("setDebugCore");
    std::lock_guard<std::mutex> _(m_mtx);

    auto event = new SetDebugCoreEvent(debugCore);
    auto set = m_recivers[SetDebugCoreEvent::eventType];
    for (auto it : set)
    {
        log("sendEvent SetDebugCoreEvent");
        QApplication::sendEvent(it, event);
    }
}

void EventDispatcher::removeReceiver(QEvent::Type t, QObject *receiver)
{
    std::lock_guard<std::mutex> _(m_mtx);

    auto & set = m_recivers[t];
    auto it = set.find(receiver);
    if (it != set.cend())
    {
        set.erase(it);
    }
}

void EventDispatcher::registerReceiver(QEvent::Type t, QObject *receiver)
{
    std::lock_guard<std::mutex> _(m_mtx);
    m_recivers[t].emplace(receiver);
}
