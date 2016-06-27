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

bool DebugCore::readMemory(void* buffer, mach_vm_address_t address,
                mach_vm_size_t size, vm_region_basic_info_data_64_t *info)
{
    kern_return_t kr;

    mach_msg_type_number_t info_cnt = sizeof (vm_region_basic_info_data_64_t);
    mach_port_t object_name;
    mach_vm_size_t size_info;
    mach_vm_address_t address_info = address;
    kr = mach_vm_region(m_task, &address_info, &size_info, VM_REGION_BASIC_INFO_64, (vm_region_info_t)info, &info_cnt, &object_name);
    if (kr)
    {
        outputMessage(QString("mach_vm_region failed with error: ").append(mach_error_string(kr)), MessageType::Error);
        return false;
    }

    /* read memory - vm_read_overwrite because we supply the buffer */
    mach_vm_size_t nread;
    kr = mach_vm_read_overwrite(m_task, address, size, (mach_vm_address_t)buffer, &nread);
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

mach_vm_address_t DebugCore::findMainBinary()
{
    vm_address_t iter = 0;
    while (1)
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
            outputMessage(QString("查找入口点失败，vm_region_recurse_64：").append(mach_error_string(kr)), MessageType::Error);
            return 0;
        }

        kr = mach_vm_read_overwrite(m_task,
                (mach_vm_address_t)addr,
                (mach_vm_size_t)sizeof(struct mach_header),
                (mach_vm_address_t)&mh, &bytes_read);
        if (kr == KERN_SUCCESS && bytes_read == sizeof(struct mach_header))
        {
            /* only one image with MH_EXECUTE filetype */
            if ( (mh.magic == MH_MAGIC || mh.magic == MH_MAGIC_64) && mh.filetype == MH_EXECUTE)
            {
                return addr;
            }
        }
        iter = addr + lsize;
    }
    outputMessage(QString("查找入口点失败"), MessageType::Error);
    return 0;
}

mach_vm_address_t DebugCore::getEntryPoint()
{
    mach_vm_address_t aslrBase = findMainBinary();
    mach_header header = {0};
    vm_region_basic_info_data_64_t region_info = {0};
    //TODO:
    readMemory(&header, aslrBase, sizeof(header), &region_info);
    emit outputMessage(QString("Magic:%1, ncmds: %2").arg(header.magic).arg(header.ncmds), MessageType::Info);

    std::vector<char> cmdBuff(header.sizeofcmds);
    auto p = cmdBuff.data();
    readMemory(p, aslrBase + sizeof(mach_header_64), cmdBuff.size(), &region_info);

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
        emit outputMessage(QString("Entry point is %1").arg(ep), MessageType::Info);
        return ep;
    }

    return 0;
}

bool DebugCore::debugNew(const QString &path, const QString &args)
{
    auto p = new DebugProcess;
    QString command = path + " " + args;
    p->start(command);
    m_pid = p->pid();
    if (m_pid <= 0)
    {
        qDebug() << "Start DebugEE process failed:" << p->errorString();
        return false;
    }

    //父进程执行
    kern_return_t err = task_for_pid(current_task(), m_pid, &m_task);
    if (err != KERN_SUCCESS)
    {
        qDebug() << "task_for_pid() failed:" << mach_error_string(err);
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
        wait(&status);
        getEntryPoint();
        //printf("status = %d\n", status);
        if (WIFEXITED(status))//子进程发送退出信号，退出循环
        {
            outputMessage("调试目标已退出。", MessageType::Info);
            break;
        }
        //让子进程继续执行
        ptrace(PT_CONTINUE, m_pid, (caddr_t)1, 0);
    }
}

void DebugCore::onDebugLoopFinished(DebugProcess *p)
{
    p->waitForFinished();
    p->deleteLater();
}
