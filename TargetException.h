//
// Created by 邢俊杰 on 16/7/22.
//

#pragma once

#include "Common.h"

#include <map>
#include <thread>
#include <atomic>

using ExceptionCallback = std::function<bool(ExceptionInfo const&)>;

class TargetException
{
public:
    TargetException();
    bool setExceptionCallback(ExceptionCallback callback);

    bool run();
    void stop();

    kern_return_t onCatchMachExceptionRaise(
            mach_port_t excPort, mach_port_t threadPort,
            mach_port_t taskPort, exception_type_t excType,
            mach_exception_data_t excData, mach_msg_type_number_t excDataCount);
    kern_return_t forwardException(mach_port_t thread,
                                   mach_port_t task, exception_type_t exception,
                                   mach_exception_data_t data, mach_msg_type_number_t dataCount);
	static TargetException& instance();

private:
    ExceptionCallback m_callback;

    std::atomic<bool> m_stop;

    struct
    {
        mach_msg_type_number_t count;
        exception_mask_t      masks[EXC_TYPES_COUNT];
        exception_handler_t   ports[EXC_TYPES_COUNT];
        exception_behavior_t  behaviors[EXC_TYPES_COUNT];
        thread_state_flavor_t flavors[EXC_TYPES_COUNT];
    } m_oldExcPorts;

    mach_port_name_t m_exceptionPort;

    struct
    {
        mach_msg_header_t head;
        char data[256];
    } m_sendMsg;

    struct
    {
        mach_msg_header_t head;
        mach_msg_body_t msgh_body;
        char data[1024];
    } m_rcvMsg;


};