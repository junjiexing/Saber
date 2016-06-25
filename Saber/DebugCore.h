
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
    bool debugNew(const QString &path, const QString &args);

signals:
    void memoryMapRefreshed(std::vector<MemoryRegion>& regions);
    void debugLoopFinished(DebugProcess* p);

private:
    void debugLoop();
private slots:
    void onDebugLoopFinished(DebugProcess* p);
private:
    std::vector<MemoryRegion> m_memoryRegions;
//    QString m_path;
//    QString m_args;

    pid_t m_pid;
    mach_port_t m_task;

    std::thread m_debugThread;
};

