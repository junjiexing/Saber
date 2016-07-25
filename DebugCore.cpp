#include "DebugCore.h"
#include "TargetException.h"
#include "EventDispatcher.h"
#include "Log.h"

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
    :QObject(nullptr)
{

}

DebugCore::~DebugCore()
{
    m_targetException.stop();
}

void DebugCore::refreshMemoryMap()
{
    mach_vm_address_t start = 0;
    do
    {
        mach_vm_size_t size = 0;
        natural_t depth = 0;
        vm_region_submap_short_info_data_64_t info;
        mach_msg_type_number_t count = VM_REGION_SUBMAP_SHORT_INFO_COUNT_64;

        kern_return_t kr = mach_vm_region_recurse(m_task, &start, &size,
                              &depth, (vm_region_recurse_info_t)&info, &count);
        if (kr != KERN_SUCCESS)
        {
            break;
        }

        bool needAdd = true;
        if (!m_memoryRegions.empty())
        {
            auto& region = m_memoryRegions.back();
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
            m_memoryRegions.emplace_back(MemoryRegion{start, size, info});
        }

        start += size;

    } while (start != 0);

    emit memoryMapRefreshed(m_memoryRegions);
}

bool DebugCore::findRegion(uint64_t address, uint64_t &start, uint64_t &size)
{
    refreshMemoryMap();
    for (auto region : m_memoryRegions)
    {
        if (region.start <= address && (region.start + region.size) >= address)
        {
            start = region.start;
            size = region.size;
            return true;
        }
    }

    mach_vm_address_t _start = address;
    mach_vm_size_t _size = 0;
    natural_t depth = 0;
    vm_region_submap_short_info_data_64_t info;
    mach_msg_type_number_t count = VM_REGION_SUBMAP_SHORT_INFO_COUNT_64;

    if (mach_vm_region_recurse(m_task, &_start, &_size,
       &depth, (vm_region_recurse_info_t)&info, &count) != KERN_SUCCESS)
    {
        return false;
    }

    start = _start;
    size = _size;
    return true;
}

bool DebugCore::readMemory(mach_vm_address_t address, void* buffer, mach_vm_size_t size)
{
    /* read memory - vm_read_overwrite because we supply the buffer */
    mach_vm_size_t nread;
    kern_return_t kr = mach_vm_read_overwrite(m_task, address, size, (mach_vm_address_t)buffer, &nread);
    if (kr != KERN_SUCCESS)
    {
        log(QString("mach_vm_read_overwrite failed with error: ").append(mach_error_string(kr)), LogType::Error);
        return false;
    }
    else if (nread != size)
    {
        log(QString("mach_vm_read_overwrite failed, requested size: %1 read: %2").arg(size).arg(nread), LogType::Error);
        return false;
    }
    return true;
}

bool DebugCore::writeMemory(mach_vm_address_t address, const void *buffer, mach_vm_size_t size)
{
    mach_vm_address_t regionAddress = address;
    mach_vm_size_t regionSize = 0;
    natural_t depth = 0;
    vm_region_submap_short_info_data_64_t info;
    mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;

    kern_return_t kr = mach_vm_region_recurse(m_task, &regionAddress, &regionSize, &depth, (vm_region_recurse_info_t)&info, &count);
    if (kr != KERN_SUCCESS)
    {
        log(QString("写入内存失败，mach_vm_region_recurse：").append(mach_error_string(kr)), LogType::Error);
        return 0;
    }

    //outputMessage(QString("region: %1").arg(regionAddress, 0, 16), MessageType::Info);
    if ((info.protection & VM_PROT_WRITE) == 0)
    {
        kr = mach_vm_protect(m_task, address, size, 0, info.protection | VM_PROT_WRITE);
        if (kr != KERN_SUCCESS)
        {
            log(QString("写入内存失败，mach_vm_protect：").append(mach_error_string(kr)), LogType::Error);
            return 0;
        }
    }

    kr = mach_vm_write(m_task, address, (vm_offset_t)buffer, size);
    if (kr != KERN_SUCCESS)
    {
        log(QString("mach_vm_write() failed: %1, address: 0x%2").arg(mach_error_string(kr)).arg(QString::number(address, 16)), LogType::Error);
        return false;
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

        kern_return_t kr = mach_vm_region_recurse(m_task, &addr, &size, &depth, (vm_region_recurse_info_t)&info, &count);
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
        if ( (mh.magic == MH_MAGIC || mh.magic == MH_MAGIC_64) && mh.filetype == MH_EXECUTE)
        {
            return addr;
        }

        addr += size;
    }
}

mach_vm_address_t DebugCore::getEntryPoint()
{

    mach_vm_address_t aslrBase = findBaseAddress();
    mach_header header = {0};
    //TODO:
    readMemory(aslrBase, &header, sizeof(header));
    log(QString("Magic:%1, ncmds: %2").arg(header.magic).arg(header.ncmds), LogType::Info);

    std::vector<char> cmdBuff(header.sizeofcmds);
    auto p = cmdBuff.data();
    readMemory(aslrBase + sizeof(mach_header_64), p, cmdBuff.size());

    for (int i = 0; i < header.ncmds; ++i)
    {
        load_command* cmd = (load_command*)p;
        if (cmd->cmd == LC_MAIN)
        {
            entry_point_command* epcmd = (entry_point_command*)p;
            uint64_t entryoff = aslrBase + epcmd->entryoff;
            log(QString("aslr base: 0x%1, entry: 0x%2").arg(QString::number(aslrBase, 16)).arg(QString::number(entryoff, 16)), LogType::Info);
            return entryoff;
        }
        p += cmd->cmdsize;
    }

    return 0;
}

Register DebugCore::getAllRegisterState(task_t thread)
{
//          /* Suspend the target process */
//          err = task_suspend(task);
//          if (err != KERN_SUCCESS) {
//            fprintf(stderr, "task_suspend() failed\n");
//            exit(EXIT_FAILURE);
//          }

//      /* Get all threads in the specified task */
//    thread_act_port_array_t threadList;
//    mach_msg_type_number_t threadCount;
//    auto err = task_threads(thread, &threadList, &threadCount);
//    if (err != KERN_SUCCESS)
//    {
//        log(QString("task_threads() error: \"%1\" 获取所有寄存器状态失败。").arg(mach_error_string(err)), LogType::Error);
//        return {};
//    }

    /* Get the thread state for the first thread */
    x86_thread_state64_t state;
    mach_msg_type_number_t stateCount = x86_THREAD_STATE64_COUNT;
    auto err = thread_get_state(thread, x86_THREAD_STATE64, (thread_state_t)&state, &stateCount);
    if (err != KERN_SUCCESS)
    {
        log(QString("thread_get_state() error: \"%1\" 获取所有寄存器状态失败。").arg(mach_error_string(err)), LogType::Error);
        return {};
    }

    Register regs;
    regs.cs = state.__cs;
    regs.fs = state.__fs;
    regs.gs = state.__gs;
    regs.r8 = state.__r8;
    regs.r9 = state.__r9;
    regs.r10 = state.__r10;
    regs.r11 = state.__r11;
    regs.r12 = state.__r12;
    regs.r13 = state.__r13;
    regs.r14 = state.__r14;
    regs.r15 = state.__r15;
    regs.rax = state.__rax;
    regs.rbx = state.__rbx;
    regs.rcx = state.__rcx;
    regs.rdx = state.__rdx;
    regs.rbp = state.__rbp;
    regs.rdi = state.__rdi;
    regs.rsi = state.__rsi;
    regs.rsp = state.__rsp;
    regs.rip = state.__rip;
    regs.rflags = state.__rflags;

    return regs;
}

void DebugCore::getAllSegment()
{
    auto base = findBaseAddress();
    mach_header_64 header;
    readMemory(base, &header, sizeof(header));
    std::vector<uint8_t> buf(header.sizeofcmds);
    readMemory(base + sizeof(header), buf.data(), buf.size());
    auto p = buf.data();
    for (int i = 0 ; i < header.ncmds; ++i)
    {
        load_command* cmd = (load_command*)p;
        if (cmd->cmd == LC_SEGMENT_64)
        {
            segment_command_64* seg = (segment_command_64*)p;
            Segment tmp;
            tmp.segname = seg->segname;
            tmp.vmaddr = seg->vmaddr;
            tmp.vmsize = seg->vmsize;
            tmp.fileoff = seg->fileoff;
            tmp.filesize = seg->filesize;
            m_segments.emplace_back(tmp);
//            outputMessage(QString("name: %1 vmaddr: 0x%2 vmsize: 0x%3 fileoff: 0x%4 filesize: 0x%5")
//                          .arg(tmp.segname)
//                          .arg(QString::number(tmp.vmaddr,16))
//                          .arg(QString::number(tmp.vmsize,16))
//                          .arg(QString::number(tmp.fileoff,16))
//                          .arg(QString::number(tmp.filesize,16)),
//                          MessageType::Info);
        }

        p += cmd->cmdsize;
    }
}

bool DebugCore::addBreakpoint(uint64_t address, bool enabled, bool isHardware)
{
    assert(!isHardware);    //TODO:
    assert(!findBreakpoint(address));

    auto bp = std::make_shared<Breakpoint>(this);
    bp->m_address = address;
    if (!bp->setEnabled(enabled))
    {
        return false;
    }

    m_breakpoints.emplace_back(bp);
    return true;
}


bool DebugCore::debugNew(const QString &path, const QString &args)
{
    auto p = new DebugProcess;
    QString command = path + " " + args;
    p->start(command);
    m_pid = (pid_t)p->pid();

    //waitForFinished不能在其他线程中执行，只能写成信号槽方式
    connect(this, SIGNAL(debugLoopFinished(DebugProcess*)),
            this, SLOT(onDebugLoopFinished(DebugProcess*)), Qt::BlockingQueuedConnection);

    if (m_pid <= 0)
    {
        log(QString("启动调试进程失败：%1").arg(p->errorString()), LogType::Error);
        return false;
    }

    //父进程执行
    kern_return_t err = task_for_pid(mach_task_self(), m_pid, &m_task);
    if (err != KERN_SUCCESS)
    {
        log(QString("task_for_pid() error: %1 启动调试进程失败").arg(mach_error_string(err)), LogType::Error);
        return false;
    }

    if (!m_targetException.setExceptionPort(m_task, std::bind(&DebugCore::handleException, this, std::placeholders::_1)))
    {
        log("setExceptionPort failed, 启动调试进程失败", LogType::Error);
        return false;
    }

    std::thread thd([p,this]
    {
        debugLoop();
        emit debugLoopFinished(p);
    });
    thd.detach();
    return true;
}

void DebugCore::debugLoop()
{
    if (!m_targetException.run())
    {
        log("TargetException.run() failed.", LogType::Error);
    }
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
//        refreshMemoryMap();
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

void DebugCore::onDebugLoopFinished(DebugProcess *p)
{
    p->waitForFinished();
    p->deleteLater();
}

bool DebugCore::handleException(ExceptionInfo const&info)
{
    auto str = QString("Exception: %1, Data size %2").arg(info.exceptionType).arg(info.exceptionData.size());
    for (auto it : info.exceptionData)
    {
        str += "," + QString::number(it, 16);
    }

    log(str, LogType::Info);

    emit refreshRegister(getAllRegisterState(info.threadPort));
    switch (info.exceptionType)
    {
        case EXC_SOFTWARE:
            if (info.exceptionData.size() == 2 && info.exceptionData[0] == EXC_SOFT_SIGNAL)
            {
                //调试目标的signal, data[1]为signal的值
                if (info.exceptionData[1] == SIGTRAP)
                {
                    //当子进程执行exec系列函数时会产生sigtrap信号
                    //TODO: 有多个子进程应该如何处理?
                    addOrEnableBreakpoint(getEntryPoint());
                }
                ptrace(PT_CONTINUE, m_pid, (caddr_t)1, 0);
            }
            else if (info.exceptionData.size() >=1 && info.exceptionData[0] == 1)
            {
                //lldb中将这种情况当做breakpoint进行处理的
                //TODO: handleBreakpoint();
            }
            return false;
        case EXC_BREAKPOINT:
        {
            //TODO: handleBreakpoint();
            EventDispatcher::instance()->setDisasmAddress(getEntryPoint());
            return waitForContinue();

        }
        case EXC_BAD_ACCESS:
            return false;
        default:
            return false;
    }
    return false;
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

bool DebugCore::addOrEnableBreakpoint(uint64_t address, bool isHardware)
{
    auto bp = findBreakpoint(address);
    if (bp)
    {
        return bp->setEnabled(true);
    }

    return addBreakpoint(address, true, isHardware);
}

void DebugCore::continueDebug(bool notForwardExc)
{
    m_notFprwardExc = notForwardExc;
    m_continueCV.notify_all();
}

bool DebugCore::waitForContinue()
{
    std::unique_lock<std::mutex> lock(m_continueMtx);
    m_continueCV.wait(lock);
    return m_notFprwardExc;
}
