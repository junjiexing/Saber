#include "DebugCore.h"

#include <vector>

#include <QProcess>
#include <QDir>
#include <QFile>
#include <QDebug>
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
    //ptrace (PT_SIGEXC, 0, 0, 0);    // Get BSD signals as mach exceptions

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


template <typename T>
class QCFType
{
public:
    inline QCFType(const T &t = 0) : type(t) {}
    inline QCFType(const QCFType &helper) : type(helper.type) { if (type) CFRetain(type); }
    inline ~QCFType() { if (type) CFRelease(type); }
    inline operator T() { return type; }
    inline QCFType operator =(const QCFType &helper)
    {
        if (helper.type)
            CFRetain(helper.type);
        CFTypeRef type2 = type;
        type = helper.type;
        if (type2)
            CFRelease(type2);
        return *this;
    }
    inline T *operator&() { return &type; }
    template <typename X> X as() const { return reinterpret_cast<X>(type); }
    static QCFType constructFromGet(const T &t)
    {
        CFRetain(t);
        return QCFType<T>(t);
    }
protected:
    T type;
};

class Q_CORE_EXPORT QCFString : public QCFType<CFStringRef>
{
public:
    inline QCFString(const QString &str) : QCFType<CFStringRef>(0), string(str) {}
    inline QCFString(const CFStringRef cfstr = 0) : QCFType<CFStringRef>(cfstr) {}
    inline QCFString(const QCFType<CFStringRef> &other) : QCFType<CFStringRef>(other) {}
    operator QString() const;
    operator CFStringRef() const;
    static QString toQString(CFStringRef cfstr);
    static CFStringRef toCFStringRef(const QString &str);

private:
    QString string;
};

DebugCore::DebugCore()
    :QObject(nullptr)
{

}

void DebugCore::refreshMemoryMap()
{
    mach_vm_address_t start = 0;
    do
    {
        mach_vm_size_t size = 0;
        natural_t depth = 1024;
        vm_region_submap_short_info_data_64_t info;
        mach_msg_type_number_t info_size = VM_REGION_SUBMAP_SHORT_INFO_COUNT_64;

        kern_return_t kr = mach_vm_region_recurse(mach_task_self(), &start, &size,
                              &depth, (vm_region_recurse_info_t)&info, &info_size);
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

bool DebugCore::readMemory(mach_vm_address_t address, void* buffer, mach_vm_size_t size)
{
    /* read memory - vm_read_overwrite because we supply the buffer */
    mach_vm_size_t nread;
    kern_return_t kr = mach_vm_read_overwrite(m_task, address, size, (mach_vm_address_t)buffer, &nread);
    if (kr)
    {
        outputMessage(QString("mach_vm_read_overwrite failed with error: ").append(mach_error_string(kr)), MessageType::Error);
        return false;
    }
    else if (nread != size)
    {
        outputMessage(QString("mach_vm_read_overwrite failed, requested size: %1 read: %2").arg(size).arg(nread), MessageType::Error);
        return false;
    }
    return true;
}

mach_vm_address_t DebugCore::findBaseAddress()
{
    vm_address_t iter = 0;
    for (;;)
    {
        mach_header mh = {0};
        vm_address_t addr = iter;
        vm_size_t lsize = 0;
        uint32_t depth;
        mach_vm_size_t bytes_read = 0;
        struct vm_region_submap_info_64 info;
        mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;

        kern_return_t kr = vm_region_recurse_64(m_task, &addr, &lsize, &depth, (vm_region_info_t)&info, &count);
        if (kr != KERN_SUCCESS)
        {
            outputMessage(QString("查找基地址失败，vm_region_recurse_64：").append(mach_error_string(kr)), MessageType::Error);
            return 0;
        }

        ;
        if (!readMemory(addr, &mh, sizeof(struct mach_header)))
        {
            outputMessage(QString("查找基地址失败，readMemory() error"), MessageType::Error);
            return 0;
        }
        /* only one image with MH_EXECUTE filetype */
        if ( (mh.magic == MH_MAGIC || mh.magic == MH_MAGIC_64) && mh.filetype == MH_EXECUTE)
        {
            return addr;
        }

        iter = addr + lsize;
    }
    outputMessage(QString("查找基地址失败"), MessageType::Error);
    return 0;
}

mach_vm_address_t DebugCore::getEntryPoint()
{
    mach_vm_address_t aslrBase = findBaseAddress();
    mach_header header = {0};
    //TODO:
    readMemory(aslrBase, &header, sizeof(header));
    outputMessage(QString("Magic:%1, ncmds: %2").arg(header.magic).arg(header.ncmds), MessageType::Info);

    std::vector<char> cmdBuff(header.sizeofcmds);
    auto p = cmdBuff.data();
    readMemory(aslrBase + sizeof(mach_header_64), p, cmdBuff.size());

    for (int i = 0; i < header.ncmds; ++i)
    {
        load_command* cmd = (load_command*)p;
        emit outputMessage(QString("Cmd: %1, Size: %2").arg(cmd->cmd).arg(cmd->cmdsize), MessageType::Info);
        if (cmd->cmd != LC_MAIN)
        {
            p += cmd->cmdsize;
            emit outputMessage(QString("p: %1").arg((uint64_t)p), MessageType::Info);

            continue;
        }

        entry_point_command* epcmd = (entry_point_command*)p;
        mach_vm_address_t ep = aslrBase + epcmd->entryoff;
        outputMessage(QString("Entry point is %1").arg(ep, 16), MessageType::Info);
        return ep;
    }

    return 0;
}

Register DebugCore::getAllRegisterState(pid_t pid)
{
    mach_port_name_t task;
    kern_return_t err = task_for_pid(mach_task_self(), pid, &task);
    if (err != KERN_SUCCESS)
    {
        outputMessage(QString("task_for_pid() error: \"%1\" 获取所有寄存器状态失败。").arg(mach_error_string(err)), MessageType::Error);
        return {};
    }

//          /* Suspend the target process */
//          err = task_suspend(task);
//          if (err != KERN_SUCCESS) {
//            fprintf(stderr, "task_suspend() failed\n");
//            exit(EXIT_FAILURE);
//          }

      /* Get all threads in the specified task */
    thread_act_port_array_t threadList;
    mach_msg_type_number_t threadCount;
    err = task_threads(task, &threadList, &threadCount);
    if (err != KERN_SUCCESS)
    {
        outputMessage(QString("task_threads() error: \"%1\" 获取所有寄存器状态失败。").arg(mach_error_string(err)), MessageType::Error);
        return {};
    }

    /* Get the thread state for the first thread */
    x86_thread_state_t state;
    mach_msg_type_number_t stateCount = x86_THREAD_STATE_COUNT;
    err = thread_get_state(threadList[0],
                     x86_THREAD_STATE,
                     (thread_state_t)&state,
                     &stateCount);
    if (err != KERN_SUCCESS)
    {
        outputMessage(QString("thread_get_state() error: \"%1\" 获取所有寄存器状态失败。").arg(mach_error_string(err)), MessageType::Error);
        return {};
    }

    Register regs;
    regs.cs = state.uts.ts64.__cs;
    regs.fs = state.uts.ts64.__fs;
    regs.gs = state.uts.ts64.__gs;
    regs.r8 = state.uts.ts64.__r8;
    regs.r9 = state.uts.ts64.__r9;
    regs.r10 = state.uts.ts64.__r10;
    regs.r11 = state.uts.ts64.__r11;
    regs.r12 = state.uts.ts64.__r12;
    regs.r13 = state.uts.ts64.__r13;
    regs.r14 = state.uts.ts64.__r14;
    regs.r15 = state.uts.ts64.__r15;
    regs.rax = state.uts.ts64.__rax;
    regs.rbx = state.uts.ts64.__rbx;
    regs.rcx = state.uts.ts64.__rcx;
    regs.rdx = state.uts.ts64.__rdx;
    regs.rbp = state.uts.ts64.__rbp;
    regs.rdi = state.uts.ts64.__rdi;
    regs.rsi = state.uts.ts64.__rsi;
    regs.rsp = state.uts.ts64.__rsp;
    regs.rflags = state.uts.ts64.__rflags;

    return regs;
}

bool DebugCore::debugNew(const QString &path, const QString &args)
{
    auto p = new DebugProcess;
    QString command = path + " " + args;
    p->start(command);
    m_pid = p->pid();
    if (m_pid <= 0)
    {
        outputMessage(QString("启动调试进程失败：%1").arg(p->errorString()), MessageType::Error);
        return false;
    }

    //父进程执行
    kern_return_t err = task_for_pid(current_task(), m_pid, &m_task);
    if (err != KERN_SUCCESS)
    {
        outputMessage(QString("task_for_pid() error: %1 启动调试进程失败").arg(mach_error_string(err)), MessageType::Error);
        return false;
    }

    //waitForFinished不能在其他线程中执行，只能写成信号槽方式
    connect(this, SIGNAL(debugLoopFinished(DebugProcess*)),
            this, SLOT(onDebugLoopFinished(DebugProcess*)), Qt::BlockingQueuedConnection);

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
    while (1)
    {
        //等待子进程信号
        int status;
        m_currPid = wait(&status);
        //printf("status = %d\n", status);
        if (WIFEXITED(status))//子进程发送退出信号，退出循环
        {
            outputMessage("调试目标已退出。", MessageType::Info);
            break;
        }
        getEntryPoint();

        refreshRegister(getAllRegisterState(m_currPid));
        //让子进程继续执行
        ptrace(PT_CONTINUE, m_pid, (caddr_t)1, 0);
    }
}

void DebugCore::onDebugLoopFinished(DebugProcess *p)
{
    p->waitForFinished();
    p->deleteLater();
}
