
#pragma once

#include <vector>
#include <thread>
#include <memory>
#include <string>
#include <condition_variable>

#include <sys/types.h>
#include <unistd.h>
#include <mach/mach_vm.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include <QObject>

#include "Common.h"
#include "Breakpoint.h"
#include "TargetException.h"


class DebugProcess;

class DebugCore : public QObject
{
    Q_OBJECT
public:
    DebugCore();
    ~DebugCore();

    void refreshMemoryMap();
    bool findRegion(uint64_t address, uint64_t& start, uint64_t& size);
    bool readMemory(mach_vm_address_t address, void* buffer, mach_vm_size_t size, bool bypassBreakpoint = true);
    bool writeMemory(mach_vm_address_t address, const void* buffer, mach_vm_size_t size, bool bypassBreakpoint = true);

    bool debugNew(const QString &path, const QString &args);
	void stop();
    void continueDebug();
    void stepIn();
    mach_vm_address_t findBaseAddress();
    mach_vm_address_t getEntryPoint();
    Register getAllRegisterState(task_t thread);

    void getAllSegment();

    using BreakpointPtr = std::shared_ptr<Breakpoint>;
    bool addBreakpoint(uint64_t address, bool enabled = true, bool isHardware = false, bool oneTime = false);
	bool removeBreakpoint(uint64_t address);
	bool removeBreakpoint(BreakpointPtr bp);
    bool addOrEnableBreakpoint(uint64_t address, bool isHardware = false, bool oneTime = false);
    BreakpointPtr findBreakpoint(uint64_t address);
signals:
    void debugLoopFinished(DebugProcess* p);

private:
    void debugLoop();
    bool handleException(ExceptionInfo const& info);

    bool handleBreakpoint(ExceptionInfo const& info);
private:
    std::vector<MemoryRegion> m_memoryRegions;
//    QString m_path;
//    QString m_args;

    pid_t m_pid;
	mach_port_t m_currThread = 0;
    mach_port_t m_task;

	std::thread m_debugThread;

    std::vector<BreakpointPtr> m_breakpoints;

    std::vector<Segment> m_segments;

    TargetException m_targetException;

    void waitForContinue();
    std::mutex m_continueMtx;
    std::condition_variable m_continueCV;

	bool m_stepIn = false;
	bool doContinueDebug();

	DebugProcess* m_process;
};

