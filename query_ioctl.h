//
// Created by DELL on 26.02.2023.
//

#ifndef OS_QUERY_IOCTL_H
#define OS_QUERY_IOCTL_H
#include <linux/ioctl.h>

#define IOCTL_MAGIC 'k'

typedef unsigned long long	u64;

typedef enum CLOCK_TYPE
{
    CLOCK_PROF,
    CLOCK_VIRT
} clock_type;
static const char * cl_str = {"CLOCK_PROF", "CLOCK_VIRT"};

typedef struct
{
    int status, dignity, ego;
} query_arg_t;

typedef struct
{
    pid_t pid;
    clock_type cl_id;
    u64 expires;
    u64 incr;
} query_cpu_itimer;

#define QUERY_GET_CPU_ITIMER _IOR(IOCTL_MAGIC, 1, query_cpu_itimer*)
#define QUERY_SET_CPU_ITIMER _IOW(IOCTL_MAGIC, 2, query_cpu_itimer*)

#endif //OS_QUERY_IOCTL_H
