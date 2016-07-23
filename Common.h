
#pragma once

#include <QString>
#include <mach/mach.h>
#include <vector>

enum class MessageType
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
    uint64_t	rax;
    uint64_t	rbx;
    uint64_t	rcx;
    uint64_t	rdx;
    uint64_t	rdi;
    uint64_t	rsi;
    uint64_t	rbp;
    uint64_t	rsp;
    uint64_t	r8;
    uint64_t	r9;
    uint64_t	r10;
    uint64_t	r11;
    uint64_t	r12;
    uint64_t	r13;
    uint64_t	r14;
    uint64_t	r15;
    uint64_t	rip;
    uint64_t	rflags;
    uint64_t	cs;
    uint64_t	fs;
    uint64_t	gs;
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