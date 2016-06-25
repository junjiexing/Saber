#include "DebugCore.h"
#include <QProcess>
#include <QDir>
#include <QDebug>
#include <fcntl.h>
#include <sys/ioctl.h>


#include <QProcess>

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
    //父进程执行
    kern_return_t err = task_for_pid(current_task(), m_pid, &m_task);
    if (err != KERN_SUCCESS)
    {
        //qDebug() << "task_for_pid() failed:" << mach_error_string(err);
        //return;
    }

    while (1)
    {
        //等待子进程信号
        int status;
        wait(&status);
        //printf("status = %d\n", status);
        if (WIFEXITED(status))//子进程发送退出信号，退出循环
            break;
        //让子进程继续执行
        ptrace(PT_CONTINUE, m_pid, (caddr_t)1, 0);
    }
}

void DebugCore::onDebugLoopFinished(DebugProcess *p)
{
    p->waitForFinished();
    p->deleteLater();
}
