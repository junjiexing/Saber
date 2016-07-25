//
// Created by 邢俊杰 on 16/7/22.
//

#include "TargetException.h"
#include "mach_exc.h"
#include "Log.h"

extern "C" boolean_t mach_exc_server(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

extern "C" kern_return_t  catch_mach_exception_raise_state(
        mach_port_t exc_port, exception_type_t exc_type, const mach_exception_data_t exc_data,
        mach_msg_type_number_t exc_data_count, int* flavor, const thread_state_t old_state,
        mach_msg_type_number_t old_stateCnt, thread_state_t new_state, mach_msg_type_number_t* new_stateCnt)
{
    log("In catch_mach_exception_raise_state");
    return KERN_FAILURE;
}

extern "C" kern_return_t catch_mach_exception_raise_state_identity(
        mach_port_t exc_port, mach_port_t thread_port, mach_port_t task_port,
        exception_type_t exc_type, mach_exception_data_t exc_data,
        mach_msg_type_number_t exc_data_count, int* flavor, thread_state_t old_state,
        mach_msg_type_number_t old_stateCnt, thread_state_t new_state, mach_msg_type_number_t *new_stateCnt)
{
    log("In catch_mach_exception_raise_state_identity");
    mach_port_deallocate (mach_task_self (), task_port);
    mach_port_deallocate (mach_task_self (), thread_port);

    return KERN_FAILURE;
}


extern "C" kern_return_t catch_mach_exception_raise(
        mach_port_t exc_port, mach_port_t thread_port,
        mach_port_t task_port, exception_type_t exc_type,
        mach_exception_data_t exc_data, mach_msg_type_number_t exc_data_count)
{
    auto self = TargetException::getSelfByTask(task_port);
    if (!self)
    {
        log("TargetException::getSelfByTask failed", LogType::Error);
        return KERN_FAILURE;
    }

    return self->onCatchMachExceptionRaise(exc_port, thread_port,
            task_port, exc_type, exc_data, exc_data_count);
}


TargetException::TargetException()
    :m_task(0),m_stop(false)
{
}

bool TargetException::setExceptionPort(task_t task, ExceptionCallback callback)
{
    {
        std::lock_guard<std::mutex> _(taskToSelfMtx);
        auto it = taskToSelf.find(m_task);
        if (it != taskToSelf.end())
        {
            taskToSelf.erase(it);
        }

        m_task = task;
        taskToSelf.emplace(task, this);
    }

    m_callback = std::move(callback);
    auto kr = mach_port_allocate(mach_task_self(),MACH_PORT_RIGHT_RECEIVE,&m_exceptionPort);
    if (kr != KERN_SUCCESS)
    {
        log(QString("mach_port_allocate failde: %1").arg(mach_error_string(kr)), LogType::Error);
        return false;
    }

    kr = mach_port_insert_right(mach_task_self(), m_exceptionPort, m_exceptionPort, MACH_MSG_TYPE_MAKE_SEND);
    if (kr != KERN_SUCCESS)
    {
        log(QString("mach_port_insert_right failde: %1").arg(mach_error_string(kr)), LogType::Error);
        return false;
    }

    kr = task_get_exception_ports(
            m_task,
            EXC_MASK_ALL,
            m_oldExcPorts.masks,
            &m_oldExcPorts.count,
            m_oldExcPorts.ports,
            m_oldExcPorts.behaviors,
            m_oldExcPorts.flavors
    );
    if (kr != KERN_SUCCESS)
    {
        log(QString("task_get_exception_ports failde: %1").arg(mach_error_string(kr)), LogType::Error);
        return false;
    }

    kr = task_set_exception_ports(
            m_task, EXC_MASK_ALL, m_exceptionPort,
            EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES,
            THREAD_STATE_NONE
    );

    if (kr != KERN_SUCCESS)
    {
        log(QString("task_set_exception_ports failde: %1").arg(mach_error_string(kr)), LogType::Error);
        return false;
    }

    return true;
}

bool TargetException::run()
{
    m_stop = false;
    for (;;)
    {
        auto kr = mach_msg(&m_rcvMsg.head,
               MACH_RCV_MSG, 0,
               sizeof(m_rcvMsg), m_exceptionPort,
               MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);

        if (m_stop)
        {
            return true;
        }

        if (kr != MACH_MSG_SUCCESS)
        {
            log(QString("mach_msg rcv failde: %1").arg(mach_error_string(kr)), LogType::Error);
            return false;
        }

        /* Handle the message (calls catch_exception_raise) */
        // we should use mach_exc_server for 64bits
        if (mach_exc_server(&m_rcvMsg.head, &m_sendMsg.head) != TRUE)
        {
            log(QString("mach_exc_server failde"), LogType::Error);
            return false;
        }

        kr = mach_msg(&m_sendMsg.head, MACH_SEND_MSG,
                m_sendMsg.head.msgh_size, 0, MACH_PORT_NULL,
                MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
        if (kr != MACH_MSG_SUCCESS)
        {
            log(QString("mach_msg send failde: %1").arg(mach_error_string(kr)), LogType::Error);
            return false;
        }
    }
}

void TargetException::stop()
{
    m_stop = true;
}

std::map<task_t, TargetException*> TargetException::taskToSelf;
std::mutex TargetException::taskToSelfMtx;

TargetException *TargetException::getSelfByTask(task_t task)
{
    std::lock_guard<std::mutex> _(taskToSelfMtx);
    auto it = taskToSelf.find(task);
    if (it != taskToSelf.end())
    {
        return it->second;
    }

    return nullptr;
}

kern_return_t
TargetException::onCatchMachExceptionRaise(
        mach_port_t excPort, mach_port_t threadPort, mach_port_t taskPort,
        exception_type_t excType, mach_exception_data_t excData,
        mach_msg_type_number_t excDataCount)
{
    if (!m_callback)
    {
        return forwardException(threadPort, taskPort, excType, excData, excDataCount);
    }

    ExceptionInfo exceptionInfo;
    exceptionInfo.threadPort = threadPort;
    exceptionInfo.taskPort = taskPort;
    exceptionInfo.exceptionType = excType;
    for (int i = 0; i < excDataCount; ++i)
    {
        exceptionInfo.exceptionData.emplace_back(excData[i]);
    }


    /* you could just as easily put your code in here, I'm just doing this to
     point out the required code */
    if(!m_callback(exceptionInfo))
    {
        return forwardException(threadPort, taskPort, excType, excData, excDataCount);
    }

    return KERN_SUCCESS;
}

kern_return_t TargetException::forwardException(
        mach_port_t thread, mach_port_t task, exception_type_t exception,
        mach_exception_data_t data, mach_msg_type_number_t dataCount)
{
    thread_state_data_t threadState = {};
    mach_msg_type_number_t threadStateCount = THREAD_STATE_MAX;

    int i;
    for(i=0; i < m_oldExcPorts.count; ++i)
    {
        if(m_oldExcPorts.masks[i] & (1 << exception))
            break;
    }

    if(i == m_oldExcPorts.count)
    {
        log(QString("no old exception port can process the exception"), LogType::Error);
        return KERN_FAILURE;
    }

    mach_port_t port = m_oldExcPorts.ports[i];
    exception_behavior_t behavior = m_oldExcPorts.behaviors[i];
    thread_state_flavor_t flavor = m_oldExcPorts.flavors[i];

    //log(QString("exception index: %1, exception count: %2, port: %3, behavior: %4, flavor: %5").arg(i).arg(m_oldExcPorts.count).arg(port).arg(behavior).arg(flavor));

    kern_return_t kr;
    if(behavior != EXCEPTION_DEFAULT)
    {
        kr = thread_get_state(thread, flavor, threadState, &threadStateCount);
        if(kr != KERN_SUCCESS)
        {
            log(QString("thread_get_state failde: %1").arg(mach_error_string(kr)), LogType::Error);
            return kr;
        }
    }

    switch(behavior)
    {
        case EXCEPTION_DEFAULT:
            kr = mach_exception_raise(port, thread, task, exception, data, dataCount);
            break;
        case EXCEPTION_STATE:
            kr = mach_exception_raise_state(port, exception, data,
                    dataCount, &flavor, threadState, threadStateCount, threadState, &threadStateCount);
            break;
        case EXCEPTION_STATE_IDENTITY:
            kr = mach_exception_raise_state_identity(port, thread, task, exception, data,
                    dataCount, &flavor, threadState, threadStateCount, threadState, &threadStateCount);
            break;
        default:
            log(QString("forwardException: unknown behavior: %1").arg(behavior), LogType::Info);
            return KERN_FAILURE; /* make gcc happy */
    }

    if(behavior != EXCEPTION_DEFAULT)
    {
        kr = thread_set_state(thread, flavor, threadState, threadStateCount);
        if(kr != KERN_SUCCESS)
            log(QString("thread_set_state failed: %1").arg(mach_error_string(kr)), LogType::Error);
    }

    return kr;
}

