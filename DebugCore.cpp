#include "DebugCore.h"
#include "TargetException.h"
#include "EventDispatcher.h"
#include "global.h"
#include "utils.h"
#include "libasmx64.h"

#include <vector>

#include <QProcess>
#include <QDir>
#include <QFile>
#include <QDebug>

#include <spawn.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mach-o/loader.h>

#include <CoreFoundation/CoreFoundation.h>

class DebugProcess : public QProcess
{
public:
    DebugProcess()
    {}

    // QProcess interface
protected:
    void setupChildProcess() override;
};

void DebugProcess::setupChildProcess()
{
    //--------------------------------------------------------------
    // Child process
    //--------------------------------------------------------------
    ptrace (PT_TRACE_ME, 0, 0, 0);    // Debug this process
    ptrace (PT_SIGEXC, 0, 0, 0);    // Get BSD signals as mach exceptions

    // If our parent is setgid, lets make sure we don't inherit those
    // extra powers due to nepotism.
    if (setgid (getgid ()) == 0)
    {

        // Let the child have its own process group. We need to execute
        // this call in both the child and parent to avoid a race condition
        // between the two processes.
        setpgid (0, 0);    // Set the child process group to match its pid

        // Sleep a bit to before the exec call
        sleep (1);
    }
}


DebugCore::DebugCore()
{

}

DebugCore::~DebugCore()
{
    stop();
}

std::vector<MemoryRegion> DebugCore::getMemoryMap()
{
	std::vector<MemoryRegion> memoryRegions;
    mach_vm_address_t start = 0;
    do
    {
        mach_vm_size_t size = 0;
        natural_t depth = 0;
        vm_region_submap_short_info_data_64_t info;
        mach_msg_type_number_t count = VM_REGION_SUBMAP_SHORT_INFO_COUNT_64;

        kern_return_t kr = mach_vm_region_recurse(g_task, &start, &size,
                              &depth, (vm_region_recurse_info_t)&info, &count);
        if (kr != KERN_SUCCESS)
        {
            break;
        }

        bool needAdd = true;
        if (!memoryRegions.empty())
        {
            auto& region = memoryRegions.back();
            if (start == region.start + region.size)
            {
                auto& prevInfo = region.info;
                if ((info.protection != prevInfo.protection)
                    || (info.max_protection != prevInfo.max_protection)
                    || (info.inheritance != prevInfo.inheritance)
                    || (info.share_mode != prevInfo.share_mode))
                {
                    region.size += size;
                    needAdd = false;
                }
            }
        }
        if (needAdd)
        {
            memoryRegions.emplace_back(MemoryRegion{start, size, info});
        }

        start += size;

    } while (start != 0);

	return memoryRegions;
}

bool DebugCore::findRegion(uint64_t address, uint64_t &start, uint64_t &size)
{
    mach_vm_address_t _start = address;
    mach_vm_size_t _size = 0;
    natural_t depth = 0;
    vm_region_submap_short_info_data_64_t info;
    mach_msg_type_number_t count = VM_REGION_SUBMAP_SHORT_INFO_COUNT_64;

    if (mach_vm_region_recurse(g_task, &_start, &_size,
       &depth, (vm_region_recurse_info_t)&info, &count) != KERN_SUCCESS)
    {
        return false;
    }

    start = _start;
    size = _size;
    return true;
}

bool DebugCore::readMemory(mach_vm_address_t address, void* buffer, mach_vm_size_t size, bool bypassBreakpoint)
{
	mach_vm_address_t regionAddress = address;
	mach_vm_size_t regionSize = 0;
	natural_t depth = 0;
	vm_region_submap_short_info_data_64_t info;
	mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;
	bool needRestore = false;
	auto _ = finally([this, &needRestore, &address, &size, &info]
	{
		if (!needRestore)
		{
			return;
		}
		log("restore");
		kern_return_t kr = mach_vm_protect(g_task, address, size, 0, info.protection);
		if (kr != KERN_SUCCESS)
		{
			log(QString("mach_vm_protect还原内存属性失败：").append(mach_error_string(kr)), LogType::Warning);
		}
	});

	kern_return_t kr = mach_vm_region_recurse(g_task, &regionAddress, &regionSize, &depth, (vm_region_recurse_info_t)&info, &count);
	if (kr != KERN_SUCCESS)
	{
		log(QString("读取内存失败，mach_vm_region_recurse：").append(mach_error_string(kr)), LogType::Warning);
		return false;
	}

	//outputMessage(QString("region: %1").arg(regionAddress, 0, 16), MessageType::Info);
	if ((info.protection & VM_PROT_READ) == 0)
	{
		kr = mach_vm_protect(g_task, address, size, 0, info.protection | VM_PROT_READ);
		if (kr != KERN_SUCCESS)
		{
			log(QString("读取内存失败，mach_vm_protect：").append(mach_error_string(kr)), LogType::Warning);
			return false;
		}
		needRestore = true;
	}

    /* read memory - vm_read_overwrite because we supply the buffer */
    mach_vm_size_t nread;
    kr = mach_vm_read_overwrite(g_task, address, size, (mach_vm_address_t)buffer, &nread);
    if (kr != KERN_SUCCESS)
    {
        log(QString("mach_vm_read_overwrite failed at address: 0x%1 with error: %2").arg(address, 0, 16).arg(mach_error_string(kr)), LogType::Warning);
        return false;
    }
    else if (nread != size)
    {
        log(QString("mach_vm_read_overwrite failed, requested size: %1 read: %2").arg(size).arg(nread), LogType::Warning);
        return false;
    }

	if (!bypassBreakpoint)
	{
		return true;
	}

	for (auto bp : m_breakpoints)
	{
		auto bpAddr = bp->address();
		if (bpAddr >= address && bpAddr < address + size)
		{
			((uint8_t*)buffer)[bpAddr - address] = bp->orgByte();
		}
	}

	return true;
}

bool DebugCore::writeMemory(mach_vm_address_t address, const void *buffer, mach_vm_size_t size, bool bypassBreakpoint)
{
    mach_vm_address_t regionAddress = address;
    mach_vm_size_t regionSize = 0;
    natural_t depth = 0;
    vm_region_submap_short_info_data_64_t info;
    mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;
	bool needRestore = false;
	auto _ = finally([this, &needRestore, &address, &size, &info]
	{
		if (!needRestore)
		{
			return;
		}
		log("restore");
		kern_return_t kr = mach_vm_protect(g_task, address, size, 0, info.protection);
		if (kr != KERN_SUCCESS)
		{
			log(QString("mach_vm_protect还原内存属性失败：").append(mach_error_string(kr)), LogType::Warning);
		}
	});
    kern_return_t kr = mach_vm_region_recurse(g_task, &regionAddress, &regionSize, &depth, (vm_region_recurse_info_t)&info, &count);
    if (kr != KERN_SUCCESS)
    {
        log(QString("写入内存失败，mach_vm_region_recurse：").append(mach_error_string(kr)), LogType::Warning);
        return false;
    }

    //outputMessage(QString("region: %1").arg(regionAddress, 0, 16), MessageType::Info);
    if ((info.protection & VM_PROT_WRITE) == 0)
    {
        kr = mach_vm_protect(g_task, address, size, 0, info.protection | VM_PROT_WRITE);
        if (kr != KERN_SUCCESS)
        {
            log(QString("写入内存失败，mach_vm_protect：").append(mach_error_string(kr)), LogType::Warning);
            return false;
        }

		needRestore = true;
    }

    kr = mach_vm_write(g_task, address, (vm_offset_t)buffer, size);
    if (kr != KERN_SUCCESS)
    {
        log(QString("mach_vm_write() failed: %1, address: 0x%2").arg(mach_error_string(kr)).arg(QString::number(address, 16)), LogType::Warning);
        return false;
    }

	if (!bypassBreakpoint)
	{
		return true;
	}

	for (auto bp : m_breakpoints)
	{
		auto bpAddr = bp->address();
		if (bpAddr >= address && bpAddr < address + size)
		{
			kr = mach_vm_write(g_task, bpAddr, (vm_offset_t)&Breakpoint::bpData, 1);
			if (kr != KERN_SUCCESS)
			{
				//TODO: ?????????
				log(QString("mach_vm_write() failed: %1, address: 0x%2").arg(mach_error_string(kr)).arg(QString::number(address, 16)), LogType::Error);
				return false;
			}

			bp->setOrgByte(((uint8_t*)buffer)[bpAddr - address]);
		}
	}
    // TODO: 还原内存属性
    return true;
}


mach_vm_address_t DebugCore::findBaseAddress()
{
    mach_vm_address_t addr = 0;
    for (;;)
    {
        mach_header mh = {0};
        mach_vm_size_t size = 0;
        uint32_t depth;
        vm_region_submap_short_info_data_64_t info;
        mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;

        kern_return_t kr = mach_vm_region_recurse(g_task, &addr, &size, &depth, (vm_region_recurse_info_t)&info, &count);
        if (kr != KERN_SUCCESS)
        {
            log(QString("查找基地址失败，vm_region_recurse_64：").append(mach_error_string(kr)), LogType::Error);
            return 0;
        }

        ;
        if (!readMemory(addr, &mh, sizeof(struct mach_header)))
        {
            log(QString("查找基地址失败，readMemory() error"), LogType::Error);
            return 0;
        }
        /* only one image with MH_EXECUTE filetype */
        if (mh.filetype == MH_EXECUTE)
        {
			if (mh.magic == MH_MAGIC)
			{
				log("调试目标是32位程序,暂不支持调试32位程序", LogType::Error);
				return 0;
			}

			if (mh.magic == MH_MAGIC_64)
			{
				return addr;
			}
        }

        addr += size;
    }
}

bool DebugCore::getEntryAndDataAddr()
{
    mach_vm_address_t aslrBase = findBaseAddress();
	if (aslrBase == 0)
	{
		return false;
	}
    mach_header header = {0};
    if (!readMemory(aslrBase, &header, sizeof(header)))
	{
		return false;
	}

    std::vector<char> cmdBuff(header.sizeofcmds);
    auto p = cmdBuff.data();
    if (!readMemory(aslrBase + sizeof(mach_header_64), p, cmdBuff.size()))
	{
		return false;
	}

    for (int i = 0; i < header.ncmds; ++i)
    {
        load_command* cmd = (load_command*)p;
		log(QString("cmd->cmd: %1").arg(cmd->cmd, 8, 16));
        if (cmd->cmd == LC_MAIN)
        {
            entry_point_command* epcmd = (entry_point_command*)p;
            m_entryAddr = aslrBase + epcmd->entryoff;
            log(QString("aslr base: 0x%1, entry: 0x%2").arg(QString::number(aslrBase, 16)).arg(QString::number(m_entryAddr, 16)), LogType::Info);
        }
		else if (cmd->cmd == LC_UNIXTHREAD || cmd->cmd == LC_THREAD)
		{
			//LC_UNIXTHREAD和LC_THREAD对应的结构体thread_commant不完整,这里直接通过偏移找到
			m_entryAddr = *reinterpret_cast<uint64_t*>(p + 16 * 9);
		}
		else if (cmd->cmd == LC_SEGMENT_64)
		{
			segment_command_64* segcmd = (segment_command_64*)p;
			if (std::strncmp(segcmd->segname, SEG_DATA, 6) == 0)
			{
				for (int j = 0; j < segcmd->nsects; ++j)
				{
					auto secloc = p + sizeof(segment_command_64) + j * sizeof(section_64);
					section_64* sec = (section_64*)secloc;
					if (std::strncmp(sec->sectname, SECT_DATA, 6) == 0)
					{
						m_dataAddr = aslrBase + sec->addr;
						log(QString("__data section addr is %1").arg(m_dataAddr));
					}
				}
			}
		}
        p += cmd->cmdsize;
    }

	return true;
}

Register DebugCore::getAllRegisterState(mach_port_t thread)
{
    /* Get the thread state for the first thread */
    Register reg;
    mach_msg_type_number_t stateCount = x86_THREAD_STATE64_COUNT;
    auto err = thread_get_state(thread, x86_THREAD_STATE64, (thread_state_t)&reg.threadState, &stateCount);
    if (err != KERN_SUCCESS)
    {
        log(QString("thread_get_state() error: \"%1\" 获取通用寄存器状态失败。").arg(mach_error_string(err)), LogType::Error);
        return {};
    }

//	stateCount = x86_FLOAT_STATE64_COUNT;
//	err = thread_get_state(thread, x86_FLOAT_STATE64, (thread_state_t)&reg.floatState, &stateCount);
//	if (err != KERN_SUCCESS)
//	{
//		log(QString("thread_get_state() error: \"%1\" 获取浮点寄存器状态失败。").arg(mach_error_string(err)), LogType::Error);
//		return {};
//	}
//
//	stateCount = x86_AVX_STATE64_COUNT;
//	err = thread_get_state(thread, x86_AVX_STATE64, (thread_state_t)&reg.avxState, &stateCount);
//	if (err != KERN_SUCCESS)
//	{
//		log(QString("thread_get_state() error: \"%1\" 获取AVX寄存器状态失败。").arg(mach_error_string(err)), LogType::Error);
//		return {};
//	}
//
//	stateCount = x86_DEBUG_STATE64_COUNT;
//	err = thread_get_state(thread, x86_DEBUG_STATE64, (thread_state_t)&reg.debugState, &stateCount);
//	if (err != KERN_SUCCESS)
//	{
//		log(QString("thread_get_state() error: \"%1\" 获取调试寄存器状态失败。").arg(mach_error_string(err)), LogType::Error);
//		return {};
//	}

	return reg;
}

bool DebugCore::addBreakpoint(uint64_t address, bool enabled, bool isHardware, bool oneTime)
{
    assert(!isHardware);    //TODO:
    assert(!findBreakpoint(address));

    auto bp = std::make_shared<Breakpoint>(this);
    bp->setAddress(address);
	bp->setOneTime(oneTime);
    if (!bp->setEnabled(enabled))
    {
        return false;
    }

    m_breakpoints.emplace_back(bp);
	emit EventDispatcher::instance()->breakpointChanged();
    return true;
}

bool DebugCore::removeBreakpoint(uint64_t address)
{
	auto it = std::find_if(m_breakpoints.cbegin(), m_breakpoints.cend(),
		[address](BreakpointPtr bp)
	{
		return bp->address() == address;
	});

	if (it == m_breakpoints.cend())
		return false;

	if (!(*it)->setEnabled(false))
	{
		return false;
	}

	m_breakpoints.erase(it);
	emit EventDispatcher::instance()->breakpointChanged();
	return true;
}
bool DebugCore::removeBreakpoint(DebugCore::BreakpointPtr bp)
{
	return removeBreakpoint(bp->address());
}

bool DebugCore::debugNew(const QString &path, const QString &args)
{
	//TODO: 检查文件是否是64位程序,不是则停止调试

    m_process = new DebugProcess; //TODO: 泄露怎么处理??
    QString command = path + " " + args;
	m_process->start(command);
    g_pid = (pid_t)m_process->pid();

    if (g_pid <= 0)
    {
        log(QString("启动调试进程失败：%1").arg(m_process->errorString()), LogType::Error);
        return false;
    }

    //父进程执行
    kern_return_t err = task_for_pid(mach_task_self(), g_pid, &g_task);
    if (err != KERN_SUCCESS)
    {
        log(QString("task_for_pid() error: %1 启动调试进程失败").arg(mach_error_string(err)), LogType::Error);
        return false;
    }

	if (!TargetException::instance()
		.setExceptionCallback(std::bind(&DebugCore::handleException, this, std::placeholders::_1)))
    {
        log("setExceptionCallback failed, 启动调试进程失败", LogType::Error);
        return false;
    }

	m_isAttach = false;
	auto self = shared_from_this();
    m_debugThread = std::thread([this, self]
    {
        debugLoop();
    });
    return true;
}

bool DebugCore::attach(pid_t pid)
{
	g_pid = pid;
	kern_return_t err = task_for_pid(mach_task_self(), g_pid, &g_task);
	if (err != KERN_SUCCESS)
	{
		log(QString("task_for_pid() error: %1 附加目标进城失败").arg(mach_error_string(err)), LogType::Error);
		return false;
	}

	if (!TargetException::instance()
		.setExceptionCallback(std::bind(&DebugCore::handleException, this, std::placeholders::_1)))
	{
		log("setExceptionCallback failed, 附加目标进城失败", LogType::Error);
		return false;
	}

	if (ptrace(PT_ATTACHEXC, pid, 0, 0) != 0)
	{
		log("ptrace PT_ATTACHEXC failed, 附加目标进城失败", LogType::Error);
		return false;
	}

	m_isAttach = true;
	auto self = shared_from_this();
	m_debugThread = std::thread([this, self]
	{
		debugLoop();
	});
	return true;
}

bool DebugCore::pause()
{
	return kill(g_pid, SIGINT) == 0;
}

void DebugCore::stop()
{
	if (g_pid == 0)
	{
		return;
	}

	auto ret = kill(g_pid, SIGKILL);
	if (ret != 0)
	{
		log(QString("Stop debug SIGKILL failed: %1").arg(ret), LogType::Warning);
	}
	TargetException::instance().stop();

	if (m_isAttach)
	{
		//TODO: 删除所有断点
		if (ptrace(PT_DETACH, g_pid, (caddr_t)1, 0) != 0)
		{
			log("Stop debug detach failed", LogType::Error);
			return;
		}
	}
	else
	{
		ret = ptrace(PT_KILL, g_pid, 0, 0);
		if (ret != -1)
		{
			log(QString("Stop debug ptrace kill failed: %1").arg(ret), LogType::Error);
			return;
		}
	}

	if (m_debugThread.joinable())
	{
		m_debugThread.join();
	}

	g_pid = 0;
}

void DebugCore::debugLoop()
{
    if (!TargetException::instance().run())
    {
        log("TargetException.run() failed.", LogType::Error);
    }

	log("TargetException.run() exited.");

//    for (;;)
//    {
//        //等待子进程信号
//        int status;
//        m_currPid = wait(&status);
//        //printf("status = %d\n", status);
//        if (WIFEXITED(status))//子进程发送退出信号，退出循环
//        {
//            outputMessage("调试目标已退出。", MessageType::Info);
//            break;
//        }
//        refreshRegister(getAllRegisterState(m_currPid));
//        outputMessage(QString("Status: %1").arg(status), MessageType::Info);
//
//        ExceptionInfo exc;
//        if (getException(exc))
//        {
//            outputMessage(QString("Exception: %1").arg(exc.exceptionType), MessageType::Info);
//        }
//
//        //让子进程继续执行
////        ptrace(PT_STEP, m_pid, (caddr_t)1, 0);
//        ptrace(PT_CONTINUE, m_pid, (caddr_t)1, 0);
//    }
}

bool DebugCore::handleException(ExceptionInfo const&info)
{
	m_excInfo = info;
    auto str = QString("Exception: %1, Data size %2").arg(m_excInfo.exceptionType).arg(m_excInfo.exceptionData.size());
    for (auto it : m_excInfo.exceptionData)
    {
        str += "," + QString::number(it, 16);
    }

    log(str, LogType::Info);

	auto regInfo = getAllRegisterState(m_excInfo.threadPort);
    emit EventDispatcher::instance()->showRegisters(regInfo);
	m_stackAddr = regInfo.threadState.__rsp;
	emit EventDispatcher::instance()->setStackAddress(m_stackAddr);
    switch (m_excInfo.exceptionType)
    {
        case EXC_SOFTWARE:
            if (m_excInfo.exceptionData.size() == 2 && m_excInfo.exceptionData[0] == EXC_SOFT_SIGNAL)
            {
                //调试目标的signal, data[1]为signal的值
                if (m_excInfo.exceptionData[1] == SIGTRAP)
                {
                    //当子进程执行exec系列函数时会产生sigtrap信号
                    //TODO: 有多个子进程应该如何处理?
					if (!getEntryAndDataAddr())
					{
						log("获取入口点失败,正在停止调试", LogType::Error);
						stop();
						return false;
					}
                    addOrEnableBreakpoint(m_entryAddr, false, true);
					emit EventDispatcher::instance()->setMemoryViewAddress(m_dataAddr);
                }
				else
				{
					waitForContinue();
				}
                ptrace(PT_CONTINUE, g_pid, (caddr_t)1, 0);
            }
            else if (m_excInfo.exceptionData.size() >=1 && m_excInfo.exceptionData[0] == 1)
            {
                //lldb中将这种情况当做breakpoint进行处理的
                return handleBreakpoint();
            }
            return false;
        case EXC_BREAKPOINT:
        {
            return handleBreakpoint();
        }
        case EXC_BAD_ACCESS:
        case EXC_BAD_INSTRUCTION:
        case EXC_ARITHMETIC:
        case EXC_EMULATION:
        case EXC_SYSCALL:
        case EXC_MACH_SYSCALL:
        case EXC_RPC_ALERT:
        case EXC_CRASH:
        case EXC_RESOURCE:
        case EXC_GUARD:
        case EXC_CORPSE_NOTIFY:
			m_excAddr = regInfo.threadState.__rip;
            waitForContinue();
            //TODO:如果用户处理了异常应该返回true阻止程序自己处理异常
            return false;
        default:
            return false;
    }
}

DebugCore::BreakpointPtr DebugCore::findBreakpoint(uint64_t address)
{
    for (auto it : m_breakpoints)
    {
        if (it->address() == address)
        {
            return it;
        }
    }

    return nullptr;
}

bool DebugCore::addOrEnableBreakpoint(uint64_t address, bool isHardware, bool oneTime)
{
    auto bp = findBreakpoint(address);
    if (bp)
    {
        return bp->setEnabled(true);
    }

    return addBreakpoint(address, true, isHardware, oneTime);
}

void DebugCore::continueDebug()
{
    m_continueType = ContinueType::ContinueRun;
    m_continueCV.notify_all();
}

void DebugCore::stepIn()
{
	m_continueType = ContinueType::ContinueStepIn;
	m_continueCV.notify_all();
}

void DebugCore::stepOver()
{
	m_continueType = ContinueType::ContinueStepOver;
	m_continueCV.notify_all();
}

void DebugCore::waitForContinue()
{
	emit EventDispatcher::instance()->debugEvent();
    std::unique_lock<std::mutex> lock(m_continueMtx);
    m_continueCV.wait(lock);
}

bool DebugCore::handleBreakpoint()
{
    x86_thread_state64_t state;
    mach_msg_type_number_t stateCount = x86_THREAD_STATE64_COUNT;
    auto err = thread_get_state(m_excInfo.threadPort, x86_THREAD_STATE64, (thread_state_t)&state, &stateCount);
    if (err != KERN_SUCCESS)
    {
        log(QString("In handleBreakpoint, thread_get_state failed: %1").arg(mach_error_string(err)), LogType::Error);
        return false;
    }
    log(QString("rip: 0x%1").arg(state.__rip, 0, 16));

    if (m_excInfo.exceptionData[0] == 1)	//单步
    {
		if (m_currentHitBP)
		{
			assert(!m_currentHitBP->enabled());
			m_currentHitBP->setEnabled(true);
			m_currentHitBP.reset();
		}

		//如果不是单步但是触发了单步异常,说明是为了绕过断点
		if (m_continueType == ContinueType::ContinueRun)
		{
			return doContinueDebug();
		}

		//正常的单步步入或者没有遇到call的单步步过
		m_excAddr = state.__rip;
        waitForContinue();

        return doContinueDebug();
    }

	//int3 断点
    --state.__rip;
	m_excAddr = state.__rip;

    auto bp = findBreakpoint(state.__rip);
    if (!bp)
    {
		//这个断点并非我们调试器所加的,
        log(QString("Un known breakpoint at 0x%1").arg(state.__rip), LogType::Warning);
        ++state.__rip;
    }
	else if (bp->isOneTime())
	{
		if (!removeBreakpoint(bp))
		{
			log(QString("删除一次性断点 0x%1 失败").arg(bp->address(), 0, 16), LogType::Warning);
		}
	}

	err = thread_set_state(m_excInfo.threadPort, x86_THREAD_STATE64, (thread_state_t)&state, stateCount);
	if (err != KERN_SUCCESS)
	{
		log(QString("In handleBreakpoint, thread_set_state failed: %1").arg(mach_error_string(err)), LogType::Error);
		return false;
	}

    waitForContinue();

    return doContinueDebug();
}

bool DebugCore::doContinueDebug()
{
    x86_thread_state64_t state;
    mach_msg_type_number_t stateCount = x86_THREAD_STATE64_COUNT;
    auto err = thread_get_state(m_excInfo.threadPort, x86_THREAD_STATE64, (thread_state_t)&state, &stateCount);
    if (err != KERN_SUCCESS)
    {
        log(QString("In DebugCore::stepIn, thread_get_state failed: %1").arg(mach_error_string(err)), LogType::Error);
        return false;
    }

	//查找要继续运行的地址上是否有断点
	//如果有断点,需要先禁用该断点,然后单步执行
	//除服单步异常后,重新启用该断点
	m_currentHitBP = findBreakpoint(state.__rip);
	if (m_currentHitBP)
	{
		if (!m_currentHitBP->setEnabled(false))
		{
			log("disable breakpoint failed", LogType::Error);
			//TODO: 询问用户是将异常传递给程序还是从断点指令下一条指令执行
		}
	}
	//FIXME: 在call上下断点,单步步过会变成单步步入
    if (m_continueType == ContinueType::ContinueStepIn || m_currentHitBP)
    {
        state.__rflags |= (1 << 8);
    }
	else if (m_continueType == ContinueType::ContinueStepOver)
	{
		uint8_t code[15];
		if (!readMemory(state.__rip, code, 15))
		{
			//读取内存失败就当做单步步入处理
			state.__rflags |= (1 << 8);
		}
		else
		{
			x64dis decoder;
			x86dis_insn* insn = decoder.decode(code, 15, state.__rip);
			if (insn->invalid || !std::strstr(insn->name, "call"))
			{
				state.__rflags |= (1 << 8);
			}
			else
			{
				addBreakpoint(state.__rip + insn->size, true, false, true);
				state.__rflags &= ~(1 << 8);
			}
		}

	}
    else
    {
        state.__rflags &= ~(1 << 8);
    }

    log(QString("RFLAGS: 0x%1").arg(state.__rflags, 0, 16));

    err = thread_set_state(m_excInfo.threadPort, x86_THREAD_STATE64, (thread_state_t)&state, stateCount);
    if (err != KERN_SUCCESS)
    {
        log(QString("In DebugCore::stepIn, thread_set_state failed: %1").arg(mach_error_string(err)), LogType::Error);
        return false;
    }

    return ptrace(PT_CONTINUE, g_pid, (caddr_t)1, 0) == -1;
}
bool DebugCore::setRegisterState(mach_port_t thread, RegisterType type, uint64_t value)
{
	x86_thread_state64_t state;
	mach_msg_type_number_t stateCount = x86_THREAD_STATE64_COUNT;
	auto err = thread_get_state(thread, x86_THREAD_STATE64, (thread_state_t)&state, &stateCount);
	if (err != KERN_SUCCESS)
	{
		log(QString("In DebugCore::setRegisterState, thread_get_state failed: %1").arg(mach_error_string(err)), LogType::Error);
		return false;
	}

	switch (type)
	{
	case RegisterType::RAX :
		state.__rax = value;
		break;
	case RegisterType::RBX :
		state.__rbx = value;
		break;
	case RegisterType::RCX :
		state.__rcx = value;
		break;
	case RegisterType::RDX :
		state.__rdx = value;
		break;
	case RegisterType::RDI :
		state.__rdi = value;
		break;
	case RegisterType::RSI :
		state.__rsi = value;
		break;
	case RegisterType::RBP :
		state.__rbp = value;
		break;
	case RegisterType::RSP :
		state.__rsp = value;
		break;
	case RegisterType::R8 :
		state.__r8 = value;
		break;
	case RegisterType::R9 :
		state.__r9 = value;
		break;
	case RegisterType::R10 :
		state.__r10 = value;
		break;
	case RegisterType::R11 :
		state.__r11 = value;
		break;
	case RegisterType::R12 :
		state.__r12 = value;
		break;
	case RegisterType::R13 :
		state.__r13 = value;
		break;
	case RegisterType::R14 :
		state.__r14 = value;
		break;
	case RegisterType::R15 :
		state.__r15 = value;
		break;
	case RegisterType::RIP :
		state.__rip = value;
		break;
	case RegisterType::RFLAGS :
		state.__rflags = value;
		break;
	case RegisterType::CS :
		state.__cs = value;
		break;
	case RegisterType::FS :
		state.__fs = value;
		break;
	case RegisterType::GS :
		state.__gs = value;
		break;
	}

	err = thread_set_state(thread, x86_THREAD_STATE64, (thread_state_t)&state, stateCount);
	if (err != KERN_SUCCESS)
	{
		log(QString("In DebugCore::setRegisterState, thread_set_state failed: %1").arg(mach_error_string(err)), LogType::Error);
		return false;
	}

	return true;
}




