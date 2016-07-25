//
// Created by System Administrator on 16/7/25.
//

#include <map>
#include <set>
#include <mutex>

#include <QObject>
#include <QEvent>

class DebugCore;

//TODO: 将重复代码改为宏定义

struct SetDisasmAddressEvent : public QEvent
{
    SetDisasmAddressEvent(uint64_t addr)
            :QEvent(eventType),address(addr)
    {}
    static const Type eventType;
    uint64_t address;
};

struct SetDebugCoreEvent : public QEvent
{
    SetDebugCoreEvent(DebugCore* dc)
            :QEvent(eventType),debugCore(dc)
    {}
    static const Type eventType;
    DebugCore* debugCore;
};



class EventDispatcher : public QObject
{
    Q_OBJECT
public:
    static EventDispatcher* instance();

    void setDisasmAddress(uint64_t addr);
    void setDebugCore(DebugCore* debugCore);

    void registerReceiver(QEvent::Type t, QObject* receiver);
    void removeReceiver(QEvent::Type t, QObject* receiver);
private:
    EventDispatcher();

    std::map<QEvent::Type, std::set<QObject*>> m_recivers;
    std::mutex m_mtx;
};

