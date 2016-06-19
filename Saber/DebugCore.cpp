#include "DebugCore.h"

DebugCore::DebugCore(QObject* parent)
    :QObject(parent)
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

bool DebugCore::debugNew(const std::string &path, const std::string &args)
{
    m_pid = fork();
    if (m_pid < 0)
    {
        return false;
    }

    if (m_pid == 0)
    {
        //子进程执行
        ptrace(PT_TRACE_ME, 0, 0, 0);
        ptrace (PT_SIGEXC, 0, 0, 0);
        if (setgid (getgid ()) == 0)
        {
            setpgid (0, 0);
            sleep (1);

            execl(path.c_str(), path.c_str(), args.c_str());
        }
        exit (1);
    }
    else
    {
        auto self = shared_from_this();
        m_debugThread = std::thread([self, this]{debugLoop();});
    }

    return true;
}

void DebugCore::debugLoop()
{
    //父进程执行
    kern_return_t err = task_for_pid(current_task(), m_pid, &m_task);
    if (err != KERN_SUCCESS)
    {
        printf("task_for_pid() failed\n");
        return;
    }

    while (1)
    {
        //等待子进程信号
        int status;
        wait(&status);
        printf("status = %d\n", status);
        if (WIFEXITED(status))//子进程发送退出信号，退出循环
            break;
        //让子进程继续执行
        ptrace(PT_CONTINUE, m_pid, (caddr_t)1, 0);
    }
}
