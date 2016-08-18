
#pragma once

#include <QString>
#include <mach/mach.h>
#include <vector>

enum class LogType
{
	Info,
	Warning,
	Error
};

struct MemoryRegion
{
    mach_vm_address_t start;
    mach_vm_size_t size;
    vm_region_submap_short_info_64 info;
};

struct Register
{
	x86_thread_state64_t threadState;
//	x86_float_state64_t floatState;
//	x86_avx_state64_t avxState;
//	x86_debug_state64_t debugState;
};

struct Segment
{
    QString segname;    	/* segment name */
    uint64_t vmaddr;		/* memory address of this segment */
    uint64_t vmsize;		/* memory size of this segment */
    uint64_t fileoff;   	/* file offset of this segment */
    uint64_t filesize;  	/* amount to map from the file */
};


struct ExceptionInfo
{
    mach_port_t threadPort;
    mach_port_t taskPort;
    exception_type_t exceptionType;
    std::vector<mach_exception_data_type_t> exceptionData;
    uint64_t exceptionAddr;
};