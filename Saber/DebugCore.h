
#pragma once

#include <vector>
#include <thread>
#include <memory>
#include <string>

#include <sys/types.h>
#include <unistd.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include <QObject>

#include "Common.h"


struct MemoryRegion
{
    mach_vm_address_t start;
    mach_vm_size_t size;
    vm_region_submap_short_info_64 info;
};

class DebugProcess;

class DebugCore : public QObject
{
    Q_OBJECT
public:
    DebugCore();

    void refreshMemoryMap();
    bool readMemory(void* buffer, mach_vm_address_t address,
                    mach_vm_size_t size, vm_region_basic_info_data_64_t *info);

    bool debugNew(const QString &path, const QString &args);
    mach_vm_address_t findMainBinary();
    mach_vm_address_t getEntryPoint();
    Register getAllRegisterState(pid_t pid);


signals:
    void memoryMapRefreshed(std::vector<MemoryRegion>& regions);
    void debugLoopFinished(DebugProcess* p);
    void outputMessage(const QString& msg, MessageType type);
    void refreshRegister(const Register regs);

private:
    void debugLoop();
private slots:
    void onDebugLoopFinished(DebugProcess* p);
private:
    std::vector<MemoryRegion> m_memoryRegions;
//    QString m_path;
//    QString m_args;

    pid_t m_pid;
    pid_t m_currPid = 0;
    mach_port_t m_task;

    std::thread m_debugThread;
};

