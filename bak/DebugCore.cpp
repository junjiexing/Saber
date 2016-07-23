#include "DebugCore.h"

#include <vector>

#include <QProcess>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QMessageBox>

#include <spawn.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <mach-o/loader.h>

#define PTRACE_EVENT_CLONE 3
#define PTRACE_GETSIGINFO 0x4202


#include <CoreFoundation/CoreFoundation.h>
#include <mach/notify.h>

//#include "logging.hpp"

#include <fstream>

// Routine mach_exception_raise
extern "C"
kern_return_t catch_mach_exception_raise
        (
                mach_port_t exception_port,
                mach_port_t thread,
                mach_port_t task,
                exception_type_t exception,
                mach_exception_data_t code,
                mach_msg_type_number_t codeCnt
        );

extern "C"
kern_return_t catch_mach_exception_raise_state
        (
                mach_port_t exception_port,
                exception_type_t exception,
                const mach_exception_data_t code,
                mach_msg_type_number_t codeCnt,
                int *flavor,
                const thread_state_t old_state,
                mach_msg_type_number_t old_stateCnt,
                thread_state_t new_state,
                mach_msg_type_number_t *new_stateCnt
        );

// Routine mach_exception_raise_state_identity
extern "C"
kern_return_t catch_mach_exception_raise_state_identity
        (
                mach_port_t exception_port,
                mach_port_t thread,
                mach_port_t task,
                exception_type_t exception,
                mach_exception_data_t code,
                mach_msg_type_number_t codeCnt,
                int *flavor,
                thread_state_t old_state,
                mach_msg_type_number_t old_stateCnt,
                thread_state_t new_state,
                mach_msg_type_number_t *new_stateCnt
        );

extern "C" boolean_t mach_exc_server(
        mach_msg_header_t *InHeadP,
        mach_msg_header_t *OutHeadP);

extern "C"
kern_return_t
catch_mach_exception_raise_state
        (
                mach_port_t                 exc_port,
                exception_type_t            exc_type,
                const mach_exception_data_t exc_data,
                mach_msg_type_number_t      exc_data_count,
                int *                       flavor,
                const thread_state_t        old_state,
                mach_msg_type_number_t      old_stateCnt,
                thread_state_t              new_state,
                mach_msg_type_number_t *    new_stateCnt
        )
{
    return KERN_FAILURE;
}

extern "C"
kern_return_t
catch_mach_exception_raise_state_identity
        (
                mach_port_t             exc_port,
                mach_port_t             thread_port,
                mach_port_t             task_port,
                exception_type_t        exc_type,
                mach_exception_data_t   exc_data,
                mach_msg_type_number_t  exc_data_count,
                int *                   flavor,
                thread_state_t          old_state,
                mach_msg_type_number_t  old_stateCnt,
                thread_state_t          new_state,
                mach_msg_type_number_t *new_stateCnt
        )
{
    //mach_port_deallocate (mach_task_self (), task_port);
    //mach_port_deallocate (mach_task_self (), thread_port);

    return KERN_FAILURE;
}

static DebugEvent g_debugEvent;

extern "C"
kern_return_t
catch_mach_exception_raise
        (
                mach_port_t             exc_port,
                mach_port_t             thread_port,
                mach_port_t             task_port,
                exception_type_t        exc_type,
                mach_exception_data_t   exc_data,
                mach_msg_type_number_t  exc_data_count)
{
    thread_suspend(thread_port);
    // FIXME: 根据exc_port区分是谁的请求
    g_debugEvent.threadPort = thread_port;
    g_debugEvent.taskPort = task_port;
    g_debugEvent.exceptionType = exc_type;
    for (int i = 0; i < exc_data_count; ++i)
    {
        g_debugEvent.exceptionData.emplace_back(exc_data[i]);
    }

    if (exc_type == EXC_BREAKPOINT || exc_type == EXC_BAD_ACCESS || exc_type == EXC_BAD_INSTRUCTION)
    {
        return KERN_SUCCESS;
    }
    return KERN_FAILURE;
}

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
//    if (setgid (getgid ()) == 0)
//    {
//
//        // Let the child have its own process group. We need to execute
//        // this call in both the child and parent to avoid a race condition
//        // between the two processes.
//        setpgid (0, 0);    // Set the child process group to match its pid
//
//        // Sleep a bit to before the exec call
//        sleep (1);
//    }
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



static task_t task_for_pid_workaround(pid_t Pid)
{
    host_t myhost = mach_host_self();
    mach_port_t psDefault = 0;
    mach_port_t psDefault_control = 0;
    task_array_t tasks = NULL;
    mach_msg_type_number_t numTasks = 0;
    kern_return_t kr;
    int i;
    if (Pid == -1)
        return 0;

    kr = processor_set_default (myhost, &psDefault);
    if (kr != KERN_SUCCESS)
        return 0;

    kr = host_processor_set_priv (myhost, psDefault, &psDefault_control);
    if (kr != KERN_SUCCESS) {
//        eprintf ("host_processor_set_priv failed with error 0x%x\n", kr);
        //mach_error ("host_processor_set_priv",kr);
        return 0;
    }

    numTasks = 0;
    kr = processor_set_tasks (psDefault_control, &tasks, &numTasks);
    if (kr != KERN_SUCCESS) {
//        eprintf ("processor_set_tasks failed with error %x\n", kr);
        return 0;
    }

    /* kernel task */
    if (Pid == 0)
        return tasks[0];

    for (i = 0; i < numTasks; i++) {
        int pid;
        pid_for_task (i, &pid);
        if (pid == Pid)
            return (tasks[i]);
    }
    return 0;
}

task_t pid_to_task (pid_t pid)
{
    task_t task = 0;
    kern_return_t err = task_for_pid (mach_task_self (), (pid_t)pid, &task);
    if ((err != KERN_SUCCESS) || !MACH_PORT_VALID (task))
    {
//        AVHTTP_LOG_ERR << "task_for_pid failed: " << mach_error_string(err);
        std::ofstream ofs("./dbg.log", std::ios::app | std::ios::out);
        ofs << "task_for_pid failed: " << mach_error_string(err) << std::endl;
        task = task_for_pid_workaround (pid);
        if (task == 0)
        {
            return task;
        }
    }
    return task;
}

DebugCore::DebugCore()
    :QObject(nullptr)
{

}

void DebugCore::refreshMemoryMap()
{
    mach_vm_address_t start = 1;
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

        //outputMessage(QString("Start: %1").arg(start, 0, 16), MessageType::Info);
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
    if (kr != KERN_SUCCESS)
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
        outputMessage(QString("写入内存失败，mach_vm_region_recurse：").append(mach_error_string(kr)), MessageType::Error);
        return 0;
    }

    //outputMessage(QString("region: %1").arg(regionAddress, 0, 16), MessageType::Info);
    if ((info.protection & VM_PROT_WRITE) == 0)
    {
        kr = mach_vm_protect(m_task, address, size, 0, info.protection | VM_PROT_WRITE);
        if (kr != KERN_SUCCESS)
        {
            outputMessage(QString("写入内存失败，mach_vm_protect：").append(mach_error_string(kr)), MessageType::Error);
            return 0;
        }
    }

    kr = mach_vm_write(m_task, address, (vm_offset_t)buffer, size);
    if (kr != KERN_SUCCESS)
    {
        outputMessage(QString("mach_vm_write() failed: %1, address: 0x%2").arg(mach_error_string(kr)).arg(QString::number(address, 16)), MessageType::Error);
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

        addr += size;
    }
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
        if (cmd->cmd == LC_MAIN)
        {
            entry_point_command* epcmd = (entry_point_command*)p;
            uint64_t entryoff = aslrBase + epcmd->entryoff;
            outputMessage(QString("aslr base: 0x%1, entry: 0x%2").arg(QString::number(aslrBase, 16)).arg(QString::number(entryoff, 16)), MessageType::Info);
            return entryoff;
        }
        p += cmd->cmdsize;
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
    regs.rip = state.uts.ts64.__rip;
    regs.rflags = state.uts.ts64.__rflags;

    return regs;
}

bool DebugCore::getAllSegment()
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

bool DebugCore::addBreakPoint(uint64_t address, bool enabled, bool isHardware)
{
    assert(!isHardware);    //TODO:

    auto bp = std::make_shared<BreakPoint>(this);
    bp->m_address = address;
    if (!bp->setEnabled(enabled))
    {
        return false;
    }


    m_breakPoints.emplace_back(bp);
    return true;
}

bool DebugCore::debugNew(const QString &path, const QString &args)
{
    auto p = new DebugProcess;
    QString command = path + " " + args;
    p->start(command);
    m_pid = p->pid();



    //waitForFinished不能在其他线程中执行，只能写成信号槽方式
    connect(this, SIGNAL(debugLoopFinished(DebugProcess*)),
            this, SLOT(onDebugLoopFinished(DebugProcess*)), Qt::BlockingQueuedConnection);

    std::thread thd([p, path, args, this]
    {
        //m_pid = createProcess(path,args);
        if (m_pid == 0)
        {
            outputMessage(QString("fork_and_ptraceme() error,启动调试进程失败"), MessageType::Error);
            return;
        }
        //父进程执行
        kern_return_t err = task_for_pid(current_task(), m_pid, &m_task);
        if (err != KERN_SUCCESS)
        {
            outputMessage(QString("task_for_pid() error: %1 启动调试进程失败").arg(mach_error_string(err)), MessageType::Error);
            return;
        }

//        if (!setExceptionPort())
//        {
//            outputMessage(QString("setExceptionPort() error: 启动调试进程失败"), MessageType::Error);
//            return;
//        }

        outputMessage(QString("启动调试进程成功"), MessageType::Info);
        debugLoop();
        emit debugLoopFinished(p);
    });
    thd.detach();
    return true;
}

void DebugCore::debugLoop()
{
    outputMessage("in debug loop", MessageType::Info);
    for (;;)
    {
//        DebugEvent event;
//        if (!waitForDebugEvent(event))
//        {
//            outputMessage("waitForDebugEvent failed.", MessageType::Error);
//            return;
//        }
//
//        outputMessage(QString("Debug event: %1").arg(event.exceptionType), MessageType::Info);
//
//
//        if (!continueRun(m_pid))
//        {
//            outputMessage("continueRun failed", MessageType::Error);
//            return;
//        }


//        /* Send the reply */
//        r = mach_msg(
//                &reply.head,
//                MACH_SEND_MSG,
//                reply.head.msgh_size,
//                0,
//                MACH_PORT_NULL,
//                MACH_MSG_TIMEOUT_NONE,
//                MACH_PORT_NULL);
//        if(r != MACH_MSG_SUCCESS)
//        {
//            outputMessage(QString("mach_msg send error: %1").arg(mach_error_string(r)), MessageType::Error);
//            return;
//        }


//        exc_msg msg = {};
//        auto kr =xnu_wait(msg);
//        outputMessage(QString("Message: %1, %2").arg(msg.exception).arg(mach_error_string(kr)), MessageType::Info);
//        if (kr != KERN_SUCCESS)
//            return;

//        mig_reply_error_t reply;
//        bool ret = validate_mach_message (m_pid, &msg);
//        if (!ret)
//        {
//            if (msg.hdr.msgh_id == 0x48)
//            {
//                m_pid = -1;
//                return;
//            }
//
//            encode_reply (&reply, &msg.hdr, KERN_FAILURE);
//            auto kr = mach_msg (&reply.Head, MACH_SEND_MSG | MACH_SEND_INTERRUPT,
//                    reply.Head.msgh_size, 0,
//                    MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE,
//                    MACH_PORT_NULL);
//            if (reply.Head.msgh_remote_port != 0 && kr != MACH_MSG_SUCCESS) {
//                kr = mach_port_deallocate(mach_task_self (), reply.Head.msgh_remote_port);
//                if (kr != KERN_SUCCESS){}
//                    //eprintf ("failed to deallocate reply port\n");
//            }
//            continue;
//        }

//        kern_return_t kr;
//        switch (msg->exception) {
//        case EXC_BAD_ACCESS:
//    //		ret = R_DEBUG_REASON_SEGFAULT;
//            *ret_code = KERN_FAILURE;
//            kr = task_suspend (msg->task.name);
//            if (kr != KERN_SUCCESS)
//                eprintf ("failed to suspend task bad access\n");
//            eprintf ("EXC_BAD_ACCESS\n");
//            break;
//        case EXC_BAD_INSTRUCTION:
//    //		ret = R_DEBUG_REASON_ILLEGAL;
//            *ret_code = KERN_FAILURE;
//            kr = task_suspend (msg->task.name);
//            if (kr != KERN_SUCCESS)
//                eprintf ("failed to suspend task bad instruction\n");
//            eprintf ("EXC_BAD_INSTRUCTION\n");
//            break;
//        case EXC_ARITHMETIC:
//            eprintf ("EXC_ARITHMETIC\n");
//            break;
//        case EXC_EMULATION:
//            eprintf ("EXC_EMULATION\n");
//            break;
//        case EXC_SOFTWARE:
//            eprintf ("EXC_SOFTWARE\n");
//            break;
//        case EXC_BREAKPOINT:
//            kr = task_suspend (msg->task.name);
//            if (kr != KERN_SUCCESS)
//                eprintf ("failed to suspend task breakpoint\n");
//    //		ret = R_DEBUG_REASON_BREAKPOINT;
//            break;
//        default:
//            eprintf ("UNKNOWN\n");
//            break;
//        }
//        kr = mach_port_deallocate (mach_task_self (), msg->task.name);
//        if (kr != KERN_SUCCESS) {
//            eprintf ("failed to deallocate task port %s-%d\n",
//                __FILE__, __LINE__);
//        }
//        kr = mach_port_deallocate (mach_task_self (), msg->thread.name);
//        if (kr != KERN_SUCCESS) {
//            eprintf ("failed to deallocated task port %s-%d\n",
//                __FILE__, __LINE__);
//        }

//        encode_reply (&reply, &msg.hdr, KERN_SUCCESS);
//        auto kr = mach_msg (&reply.Head, MACH_SEND_MSG | MACH_SEND_INTERRUPT,
//                reply.Head.msgh_size, 0,
//                MACH_PORT_NULL, 0,
//                MACH_PORT_NULL);
//        if (reply.Head.msgh_remote_port != 0 && kr != MACH_MSG_SUCCESS) {
//            kr = mach_port_deallocate(mach_task_self (), reply.Head.msgh_remote_port);
//            if (kr != KERN_SUCCESS)
//                outputMessage("failed to deallocate reply port", MessageType::Warning);
//        }

//        if (addBreakPoint(getEntryPoint()))
//        {
//            outputMessage("add breakpoint to entry point success", MessageType::Info);
//        }
//        else
//        {
//            outputMessage("add breakpoint to entry point failed", MessageType::Error);
//        }
//
//        xnu_continue(m_pid);



        //等待子进程信号
        int status;
        m_currPid = wait(&status);
        outputMessage(QString("status = %1").arg(status), MessageType::Info);
        //printf("status = %d\n", status);
        if (WIFEXITED(status))//子进程发送退出信号，退出循环
        {
            outputMessage("调试目标已退出。", MessageType::Info);
            break;
        }
//        refreshMemoryMap();
//        refreshRegister(getAllRegisterState(m_currPid));
        outputMessage(QString("Status: %1").arg(status), MessageType::Info);

//        static bool bp = false;
//        if (!bp)
//        {
//            bp = true;
//            if (addBreakPoint(getEntryPoint()))
//            {
//                outputMessage("add breakpoint to entry point success", MessageType::Info);
//            }
//            else
//            {
//                outputMessage("add breakpoint to entry point failed", MessageType::Error);
//            }
//        }
//
//
//        if (WSTOPSIG(status))
//        {
//            outputMessage("WSTOPSIG", MessageType::Info);
//            DebugEvent event;
//            if (!waitForDebugEvent(event))
//            {
//                outputMessage("waitForDebugEvent failed.", MessageType::Error);
//                return;
//            }
//
//            outputMessage(QString("Debug event: %1").arg(event.exceptionType), MessageType::Info);
//        }
//        else if (WIFCONTINUED(status))
//        {
//            outputMessage("WIFCONTINUED", MessageType::Info);
//        }
//        else if (WIFSTOPPED(status))
//        {
//            outputMessage("WIFSTOPPED", MessageType::Info);
//        }
//        else if (WIFEXITED(status))
//        {
//            outputMessage("WIFEXITED", MessageType::Info);
//        }
//        else if (WIFSIGNALED(status))
//        {
//            outputMessage("WIFSIGNALED", MessageType::Info);
//        }
//        else if (WTERMSIG(status))
//        {
//            outputMessage("WTERMSIG", MessageType::Info);
//        }
//        else if (WCOREDUMP(status))
//        {
//            outputMessage("WCOREDUMP", MessageType::Info);
//        }
//        else
//        {
//            outputMessage("???????", MessageType::Info);
//        }


        //让子进程继续执行
//        ptrace(PT_STEP, m_pid, (caddr_t)1, 0);
        ptrace(PT_CONTINUE, m_currPid, (caddr_t)1, 0);
        outputMessage(QString("after continue"), MessageType::Info);
    }
}

void DebugCore::onDebugLoopFinished(DebugProcess *p)
{
    p->waitForFinished();
    p->deleteLater();
}

bool DebugCore::setExceptionPort()
{
    auto kr = mach_port_allocate(mach_task_self(),MACH_PORT_RIGHT_RECEIVE,&m_exceptionPort);
    if (kr != KERN_SUCCESS)
    {
        return false;
    }

    kr = mach_port_insert_right(mach_task_self(), m_exceptionPort, m_exceptionPort, MACH_MSG_TYPE_MAKE_SEND);
    if (kr != KERN_SUCCESS)
    {
        return false;
    }


//    kr = task_get_exception_ports(
//            m_task,
//            EXC_MASK_ALL,
//            m_oldExcPorts.masks,
//            &m_oldExcPorts.count,
//            m_oldExcPorts.ports,
//            m_oldExcPorts.behaviors,
//            m_oldExcPorts.flavors
//    );
    if (kr != KERN_SUCCESS)
    {
        return false;
    }

    kr = task_set_exception_ports(
            m_task,
            EXC_MASK_ALL,
            m_exceptionPort,
            EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES,
            THREAD_STATE_NONE
    );
    if (kr != KERN_SUCCESS)
    {
        return false;
    }

    return true;
}

bool DebugCore::waitForDebugEvent(DebugEvent& e)
{
    struct
    {
        mach_msg_header_t head;
        char data[256];
    } reply;

    struct
    {
        mach_msg_header_t head;
        mach_msg_body_t msgh_body;
        char data[1024];
    } msg;

    mach_msg_return_t r = mach_msg(&msg.head,
                 MACH_RCV_MSG,
                 0,
                 sizeof(msg),
                 m_exceptionPort,
                 MACH_MSG_TIMEOUT_NONE,
                 MACH_PORT_NULL);

    if (r != MACH_MSG_SUCCESS)
    {
        outputMessage(QString("mach_msg recv failed on waitForDebugEvent: %1").arg(mach_error_string(r)), MessageType::Error);
        return false;
    }

    /* Handle the message (calls catch_exception_raise) */
    // we should use mach_exc_server for 64bits
    mach_exc_server(&msg.head, &reply.head);

    e = g_debugEvent;

    r = mach_msg(
            &reply.head,
            MACH_SEND_MSG,
            reply.head.msgh_size,
            0,
            MACH_PORT_NULL,
            MACH_MSG_TIMEOUT_NONE,
            MACH_PORT_NULL);

    if(r != MACH_MSG_SUCCESS)
    {
        outputMessage(QString("mach_msg send failed on waitForDebugEvent: %1").arg(mach_error_string(r)), MessageType::Error);
        return false;
    }

    return true;
}

pid_t DebugCore::createProcess(const QString &path, const QString &args)
{
    posix_spawnattr_t attr;
    int retval = posix_spawnattr_init (&attr);
    // set process flags
    // the new process will start in a suspended state and permissions reset to real uid/gid
#ifndef _POSIX_SPAWN_DISABLE_ASLR
#define _POSIX_SPAWN_DISABLE_ASLR 0x0100
#endif
    short flags = POSIX_SPAWN_RESETIDS | POSIX_SPAWN_START_SUSPENDED /*| _POSIX_SPAWN_DISABLE_ASLR*/;
    retval = posix_spawnattr_setflags(&attr, flags);

    // reset signals, ripped from LLDB :-]
    sigset_t no_signals;
    sigset_t all_signals;
    sigemptyset (&no_signals);
    sigfillset (&all_signals);
    posix_spawnattr_setsigmask(&attr, &no_signals);
    posix_spawnattr_setsigdefault(&attr, &all_signals);
    // set the target cpu to be used, due to fat binaries
    size_t copied = 0;
    cpu_type_t cpu  = CPU_TYPE_I386 | CPU_TYPE_X86_64;
    retval = posix_spawnattr_setbinpref_np(&attr, 1, &cpu, &copied);

    char *spawnedEnv[] = { nullptr };

//    int cmd_line_len = args.length();
//    if (cmd_line_len >= ARG_MAX)
//    {
//        outputMessage("args too long", MessageType::Error);
//        return -1;
//    }
    // execute with no arguments
    auto tmp = path;
    char * argv[] = {tmp.toUtf8().data(), nullptr};

    pid_t pid = 0;
    retval = posix_spawnp(&pid, argv[0], NULL, &attr, argv, spawnedEnv);
    if (retval)
    {
        outputMessage("posix_spawnp failed.", MessageType::Error);
        return 0;
    }
    // parent
    // initialize the mach port into the debugee
    retval = attach(pid);
    // failed to attach
    if (retval == 0)
    {
        kill(pid, SIGCONT); // leave no zombies behind!
        kill(pid, SIGKILL);
        return 0;
    }
    // suspend all threads
    suspendAllThreads(pid);
    // and now we can continue the process, threads are still suspended!
    kill(pid, SIGCONT);

    return pid;
}

bool DebugCore::attach(pid_t pid)
{
    m_task = pid_to_task(pid);
    if (m_task == 0)
    {
        return false;
    }

    return setExceptionPort();
}

bool DebugCore::suspendAllThreads(pid_t pid)
{
    thread_act_port_array_t thread_list;
    mach_msg_type_number_t thread_count,i;

    task_t task = pid_to_task(pid);
    if(task == 0)
    {
        outputMessage("pid_to_task failed", MessageType::Error);
        return false;

    }
    if (task_threads(task, &thread_list, &thread_count) != KERN_SUCCESS)
    {
        outputMessage("task_threads failed", MessageType::Error);
    }

    if (thread_count > 0)
    {
        i = thread_count;
        while (i--)
        {
            thread_suspend(thread_list[i]);
        }
    }
    return true;
}

bool DebugCore::continueRun(pid_t pid)
{
    thread_act_port_array_t thread_list;
    mach_msg_type_number_t thread_count,i;

    task_t task = pid_to_task(pid);
    if(task == 0)
    {
        outputMessage("pid_to_task failed", MessageType::Error);
        return false;

    }
    if (task_threads(task, &thread_list, &thread_count) != KERN_SUCCESS)
    {
        outputMessage("task_threads failed", MessageType::Error);
    }

    if (thread_count > 0)
    {
        i = thread_count;
        while (i--)
        {
            thread_resume(thread_list[i]);
        }
    }
    return true;
}












