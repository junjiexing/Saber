
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

class DebugCore : public QObject, public std::enable_shared_from_this<DebugCore>
{
    Q_OBJECT
public:
    DebugCore(QObject *parent);

    void refreshMemoryMap();
    bool debugNew(const std::string &path, const std::string &args);

signals:
    void memoryMapRefreshed(std::vector<MemoryRegion>& regions);

private:
    void debugLoop();

private:
    std::vector<MemoryRegion> m_memoryRegions;
//    QString m_path;
//    QString m_args;

    pid_t m_pid;
    mach_port_t m_task;

    std::thread m_debugThread;
};

