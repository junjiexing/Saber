
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

enum class ContinueType
{
	ContinueRun,
	ContinueStepIn,
	ContinueStepOver
};

class DebugCore : public std::enable_shared_from_this<DebugCore>
{
public:
    DebugCore();
    ~DebugCore();

	std::vector<MemoryRegion> getMemoryMap();
    bool findRegion(uint64_t address, uint64_t& start, uint64_t& size);
    bool readMemory(mach_vm_address_t address, void* buffer, mach_vm_size_t size, bool bypassBreakpoint = true);
    bool writeMemory(mach_vm_address_t address, const void* buffer, mach_vm_size_t size, bool bypassBreakpoint = true);

    bool debugNew(const QString &path, const QString &args);
	bool attach(pid_t pid);
	bool pause();
	void stop();
    void continueDebug();
	void stepIn();
	void stepOver();
    mach_vm_address_t findBaseAddress();
    bool getEntryAndDataAddr();
    Register getAllRegisterState(mach_port_t thread);
	bool setRegisterState(mach_port_t thread, RegisterType type, uint64_t value);

    using BreakpointPtr = std::shared_ptr<Breakpoint>;
	using BreakpointWeakPtr = std::weak_ptr<Breakpoint>;
    bool addBreakpoint(uint64_t address, bool enabled = true, bool isHardware = false, bool oneTime = false);
	bool removeBreakpoint(uint64_t address);
	bool removeBreakpoint(BreakpointPtr bp);
    bool addOrEnableBreakpoint(uint64_t address, bool isHardware = false, bool oneTime = false);
    BreakpointPtr findBreakpoint(uint64_t address);
	std::vector<BreakpointPtr> const& breakpoints(){ return m_breakpoints; }
	uint64_t excAddr() { return m_excAddr; }
	uint64_t entryAddr() { return m_entryAddr; }
	uint64_t dataAddr() { return m_dataAddr; }
	uint64_t stackAddr() { return m_stackAddr; }
	ExceptionInfo const& excInfo() { return m_excInfo; }
private:
    void debugLoop();
    bool handleException(ExceptionInfo const& info);

    bool handleBreakpoint();
private:
//    QString m_path;
//    QString m_args;

	bool m_isAttach;

	std::thread m_debugThread;

    std::vector<BreakpointPtr> m_breakpoints;

    std::vector<Segment> m_segments;

    void waitForContinue();
    std::mutex m_continueMtx;
    std::condition_variable m_continueCV;

	uint64_t m_entryAddr = 0;
	uint64_t m_dataAddr = 0;

	uint64_t m_excAddr = 0;
	uint64_t m_stackAddr = 0;
	ExceptionInfo m_excInfo;

	ContinueType m_continueType = ContinueType::ContinueRun;
	bool doContinueDebug();

	DebugProcess* m_process;

	BreakpointPtr m_currentHitBP;
};

