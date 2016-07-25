
#pragma once

#include <vector>
#include <thread>
#include <memory>
#include <string>

#include <sys/types.h>
#include <unistd.h>
#include <mach/mach_vm.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include <QObject>

#include "Common.h"
#include "Breakpoint.h"


class DebugProcess;

class DebugCore : public QObject
{
    Q_OBJECT
public:
    DebugCore();

    void refreshMemoryMap();
    bool readMemory(mach_vm_address_t address, void* buffer, mach_vm_size_t size);
    bool writeMemory(mach_vm_address_t address, const void* buffer, mach_vm_size_t size);

    bool debugNew(const QString &path, const QString &args);
    bool attach(pid_t pid);
    bool suspendAllThreads(pid_t pid);
    bool continueRun(pid_t pid);
    mach_vm_address_t findBaseAddress();
    mach_vm_address_t getEntryPoint();
    Register getAllRegisterState(pid_t pid);

    bool getAllSegment();

    bool addBreakPoint(uint64_t address, bool enabled = true, bool isHardware = false);

signals:
    void memoryMapRefreshed(std::vector<MemoryRegion>& regions);
    void debugLoopFinished(DebugProcess* p);
    void outputMessage(const QString& msg, MessageType type);
    void refreshRegister(const Register regs);

private:
    pid_t createProcess(const QString& path, const QString& args);
    void debugLoop();
    bool waitForDebugEvent(DebugEvent& e);

    bool setExceptionPort();
private slots:
    void onDebugLoopFinished(DebugProcess* p);
private:
    std::vector<MemoryRegion> m_memoryRegions;
//    QString m_path;
//    QString m_args;

    pid_t m_pid;
    pid_t m_currPid = 0;
    mach_port_t m_task;

    std::vector<std::shared_ptr<BreakPoint>> m_breakPoints;

    std::vector<Segment> m_segments;


    mach_port_t m_exceptionPort;
    enum {MAX_EXCEPTION_PORTS = 16};
    struct {
        mach_msg_type_number_t count;
        exception_mask_t      masks[MAX_EXCEPTION_PORTS];
        exception_handler_t   ports[MAX_EXCEPTION_PORTS];
        exception_behavior_t  behaviors[MAX_EXCEPTION_PORTS];
        thread_state_flavor_t flavors[MAX_EXCEPTION_PORTS];
    } m_oldExcPorts;
};

