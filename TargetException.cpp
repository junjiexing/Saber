//
// Created by 邢俊杰 on 16/7/22.
//

#include <mach/exc.h>
#include "mach_exc.h"
#include "Common.h"
#include <thread>
#include "TargetException.h"
#include <map>
#include <mutex>
#include <functional>
#include <mach/mig.h>

extern "C" kern_return_t  catch_mach_exception_raise_state(
        mach_port_t exc_port, exception_type_t exc_type, const mach_exception_data_t exc_data,
        mach_msg_type_number_t exc_data_count, int* flavor, const thread_state_t old_state,
        mach_msg_type_number_t old_stateCnt, thread_state_t new_state, mach_msg_type_number_t* new_stateCnt)
{
    return KERN_FAILURE;
}

extern "C" kern_return_t catch_mach_exception_raise_state_identity(
        mach_port_t exc_port, mach_port_t thread_port, mach_port_t task_port,
        exception_type_t exc_type, mach_exception_data_t exc_data,
        mach_msg_type_number_t exc_data_count, int* flavor, thread_state_t old_state,
        mach_msg_type_number_t old_stateCnt, thread_state_t new_state, mach_msg_type_number_t *new_stateCnt)
{
    //mach_port_deallocate (mach_task_self (), task_port);
    //mach_port_deallocate (mach_task_self (), thread_port);

    return KERN_FAILURE;
}





struct InternalInfo
{
    ExceptionCallback callback;
    struct
    {
        mach_msg_type_number_t count;
        exception_mask_t      masks[16];
        exception_handler_t   ports[16];
        exception_behavior_t  behaviors[16];
        thread_state_flavor_t flavors[16];
    } oldExcPorts;

    mach_port_name_t exceptionPort;

    struct
    {
        mach_msg_header_t head;
        char data[256];
    } sendMsg;

    struct
    {
        mach_msg_header_t head;
        mach_msg_body_t msgh_body;
        char data[1024];
    } rcvMsg;
};

std::map<task_t, InternalInfo> g_taskToInternalInfo;
std::mutex  g_taskToInternalInfoLock;

InternalInfo* getInternalInfo(task_t task)
{
    std::lock_guard<std::mutex> _(g_taskToInternalInfoLock);
    auto it = g_taskToInternalInfo.find(task);
    if (it == g_taskToInternalInfo.end())
    {
        return nullptr;
    }

    return &it->second;
}

kern_return_t forward_exception(InternalInfo* info, mach_port_t thread,
        mach_port_t task, exception_type_t exception,
        mach_exception_data_t data, mach_msg_type_number_t data_count)
{
    kern_return_t r;

    thread_state_data_t thread_state;
    mach_msg_type_number_t thread_state_count = THREAD_STATE_MAX;

    int i;
    for(i=0; i < info->oldExcPorts.count; ++i)
    {
        if(info->oldExcPorts.masks[i] & (1 << exception))
            break;
    }

    if(i == info->oldExcPorts.count)
        return KERN_FAILURE;

    mach_port_t port = info->oldExcPorts.ports[i];
    exception_behavior_t behavior = info->oldExcPorts.behaviors[i];
    thread_state_flavor_t flavor = info->oldExcPorts.flavors[i];

    if(behavior != EXCEPTION_DEFAULT)
    {
        r = thread_get_state(thread, flavor, thread_state, &thread_state_count);
        if(r != KERN_SUCCESS)
            return r;
    }

    switch(behavior)
    {
        case EXCEPTION_DEFAULT:
            r = mach_exception_raise(port, thread, task, exception, data, data_count);
            break;
        case EXCEPTION_STATE:
            r = mach_exception_raise_state(info->exceptionPort, exception, data,
                       data_count, &flavor, thread_state, thread_state_count, thread_state, &thread_state_count);
            break;
        case EXCEPTION_STATE_IDENTITY:
            r = mach_exception_raise_state_identity(port, thread, task, exception, data,
                       data_count, &flavor, thread_state, thread_state_count, thread_state, &thread_state_count);
            break;
        default:
            r = KERN_FAILURE; /* make gcc happy */
//            ABORT("forward_exception: unknown behavior");
            break;
    }

    if(behavior != EXCEPTION_DEFAULT)
    {
        r = thread_set_state(thread, flavor, thread_state, thread_state_count);
//        if(r != KERN_SUCCESS)
//            ABORT("thread_set_state failed in forward_exception");
    }

    return r;
}


extern "C" kern_return_t catch_mach_exception_raise(
        mach_port_t exc_port, mach_port_t thread_port,
        mach_port_t task_port, exception_type_t exc_type,
        mach_exception_data_t exc_data, mach_msg_type_number_t exc_data_count)
{
    auto info = getInternalInfo(task_port);
    if (!info)
    {
        return KERN_FAILURE;
    }
    if (!info->callback)
    {
        return forward_exception(info, thread_port, task_port, exc_type, exc_data, exc_data_count);
    }

    ExceptionInfo exceptionInfo;
    exceptionInfo.threadPort = thread_port;
    exceptionInfo.taskPort = task_port;
    exceptionInfo.exceptionType = exc_type;
    for (int i = 0; i < exc_data_count; ++i)
    {
        exceptionInfo.exceptionData.emplace_back(exc_data[i]);
    }


    /* you could just as easily put your code in here, I'm just doing this to
     point out the required code */
    if(!info->callback(exceptionInfo))
    {
        return forward_exception(info, thread_port, task_port, exc_type, exc_data, exc_data_count);
    }

    return KERN_SUCCESS;
}

extern "C" boolean_t mach_exc_server(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

bool waitException(task_t task)
{
    auto info = getInternalInfo(task);
    if (!info)
    {
        return false;
    }
    mach_msg_return_t r = mach_msg(&info->rcvMsg.head,
                                   MACH_RCV_MSG,
                                   0,
                                   sizeof(info->rcvMsg),
                                   info->exceptionPort,
                                   MACH_MSG_TIMEOUT_NONE,
                                   MACH_PORT_NULL);

    if (r != MACH_MSG_SUCCESS)
    {
        return false;
    }

    /* Handle the message (calls catch_exception_raise) */
    // we should use mach_exc_server for 64bits
    return mach_exc_server(&info->rcvMsg.head, &info->sendMsg.head) == TRUE;
}

bool replyException(task_t task)
{
    auto info = getInternalInfo(task);
    if (!info)
    {
        return false;
    }
    return mach_msg(
            &info->sendMsg.head,
            MACH_SEND_MSG,
            info->sendMsg.head.msgh_size,
            0,
            MACH_PORT_NULL,
            MACH_MSG_TIMEOUT_NONE,
            MACH_PORT_NULL) == MACH_MSG_SUCCESS;
}

bool setExceptionPort(task_t task, ExceptionCallback callback)
{
    InternalInfo info = {std::move(callback)};
    auto kr = mach_port_allocate(mach_task_self(),MACH_PORT_RIGHT_RECEIVE,&info.exceptionPort);
    if (kr != KERN_SUCCESS)
    {
        return false;
    }

    kr = mach_port_insert_right(mach_task_self(), info.exceptionPort, info.exceptionPort, MACH_MSG_TYPE_MAKE_SEND);
    if (kr != KERN_SUCCESS)
    {
        return false;
    }


    kr = task_get_exception_ports(
            task,
            EXC_MASK_ALL,
            info.oldExcPorts.masks,
            &info.oldExcPorts.count,
            info.oldExcPorts.ports,
            info.oldExcPorts.behaviors,
            info.oldExcPorts.flavors
    );
    if (kr != KERN_SUCCESS)
    {
        return false;
    }

    kr = task_set_exception_ports(
            task,
            EXC_MASK_ALL,
            info.exceptionPort,
            EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES,
            THREAD_STATE_NONE
    );

    std::lock_guard<std::mutex> _(g_taskToInternalInfoLock);
    g_taskToInternalInfo.emplace(task, info);
    return kr == KERN_SUCCESS;
}

