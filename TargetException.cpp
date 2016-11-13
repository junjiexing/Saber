//
// Created by 邢俊杰 on 16/7/22.
//

#include "TargetException.h"
#include "mach_exc.h"
#include "global.h"

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

static bool isValidTask(task_t task)
{
	struct task_basic_info info;

	mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
	return ::task_info (task, TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS;
}


extern "C" kern_return_t catch_mach_exception_raise(
        mach_port_t exc_port, mach_port_t thread_port,
        mach_port_t task_port, exception_type_t exc_type,
        mach_exception_data_t exc_data, mach_msg_type_number_t exc_data_count)
{
	if (task_port != g_task && !isValidTask(g_task))
	{
		if (exc_type == EXC_SOFTWARE && exc_data_count == 2 && exc_data[0] == EXC_SOFT_SIGNAL && exc_data[1] == SIGTRAP)
		{
			g_task = task_port;
		}
	}
    return TargetException::instance().onCatchMachExceptionRaise(exc_port, thread_port,
            task_port, exc_type, exc_data, exc_data_count);
}


TargetException::TargetException()
    :m_stop(false)
{
}

bool TargetException::setExceptionCallback(ExceptionCallback callback)
{
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
            g_task,
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
            g_task, EXC_MASK_ALL, m_exceptionPort,
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
	//TODO:detach时需要将原来的exception port恢复
    m_stop = true;
    mach_port_destroy(mach_task_self(), m_exceptionPort);
}

kern_return_t
TargetException::onCatchMachExceptionRaise(
        mach_port_t excPort, mach_port_t threadPort, mach_port_t taskPort,
        exception_type_t excType, mach_exception_data_t excData,
        mach_msg_type_number_t excDataCount)
{
    if (!m_callback)
    {
        return KERN_FAILURE;
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
		return KERN_FAILURE;
    }

    return KERN_SUCCESS;
}

TargetException &TargetException::instance()
{
	static TargetException t;
	return t;
}

