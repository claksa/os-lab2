#ifndef OS_QUERY_IOCTL_H
#define OS_QUERY_IOCTL_H
#include <linux/ioctl.h>

#define IOCTL_MAGIC 'k'
#define NET_DEV_NAME_SIZE 16

typedef unsigned long long	u64;

typedef enum CLOCK_TYPE
{
    CLOCK_PROF,
    CLOCK_VIRT
} clock_type;
static const char * cl_str = {"CLOCK_PROF", "CLOCK_VIRT"};

typedef struct
{
    pid_t pid;
    clock_type cl_id;
    u64 expires;
    u64 incr;
} query_cpu_itimer;

typedef struct
{
    pid_t pid;
    char name[NET_DEV_NAME_SIZE];
    unsigned long mem_end;
    unsigned long mem_start;
    unsigned long base_addr;
    int irq;
    unsigned char if_port;
    unsigned char dma;
    unsigned long state;

} query_net_device;

#define QUERY_GET_CPU_ITIMER _IOR(IOCTL_MAGIC, 1, query_cpu_itimer*)
#define QUERY_SET_CPU_ITIMER _IOW(IOCTL_MAGIC, 2, query_cpu_itimer*)

#define QUERY_GET_NET_DEVICE _IOR(IOCTL_MAGIC, 3, query_net_device*)
#define QUERY_SET_NET_DEVICE _IOW(IOCTL_MAGIC, 4, query_net_device*)

#endif //OS_QUERY_IOCTL_H
