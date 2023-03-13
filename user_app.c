#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include "query_ioctl.h"

void get_cpu_itimer_structure(int fd, int pid, int clk_id)
{
    query_cpu_itimer q_it;
    q_it.pid = pid;
    q_it.cl_id = clk_id;
    printf("entered pid: %d\n", q_it.pid);
    printf("entered clock_id: %d\n", clk_id);
    printf("entered clock_type: %s\n", cl_str[q_it.cl_id]);
    if ( ioctl(fd, QUERY_SET_CPU_ITIMER, &q_it) == -1 )
    {
        perror("ioctl cpu_itimer set");
    }
    if ( ioctl(fd, QUERY_GET_CPU_ITIMER, &q_it) == -1 )
    {
        perror("ioctl cpu_itimer get");
    } else {
        printf("cpu itimer: expires = %llu, incr = %llu\n", q_it.expires, q_it.incr);
    }
}

void print_net_device(query_net_device *q_net_list)
{
    for (int i = 0; i < NR_NETD; ++i)
    {
        printf("---NET_DEVICE---\n");
        printf("name: %s\n", q_net_list[i].name);
        printf("mem_start: %lu\n", q_net_list[i].mem_start);
        printf("mem_end: %lu\n", q_net_list[i].mem_end);
        printf("base_addr: %lu\n", q_net_list[i].base_addr);
        printf("state: %lu\n", q_net_list[i].state);
        printf("irq: %d\n", q_net_list[i].irq);
        printf("if_port: %u\n", q_net_list[i].if_port);
        printf("dma: %u\n", q_net_list[i].dma);
        printf("\n");
    }
}

void get_net_device_structure(int fd, int pid)
{
    net_device_data net_dd;
    query_net_device *q_net = (query_net_device*) malloc(NR_NETD * sizeof(query_net_device));
    if (!q_net){
        fprintf(stderr, "failed mem alloc");
        return;
    }
    net_dd.pid = pid;
    net_dd.q_net_list = q_net;
    net_dd.q_net_list[0].pid = pid;
    printf("entered pid: %d\n", net_dd.pid);
    if ( ioctl(fd, QUERY_SET_NET_DEVICE, &net_dd) == -1 )
    {
        perror("ioctl net_device set");
    }
    print_net_device(net_dd.q_net_list);
    free(q_net);
}

// ./user_app struct_type pid cpu_type (if it needs)
int main(int argc, char *argv[])
{
    char *file_name = "/dev/my-ioctl";
    int fd, pid;
    int clk_id;

    enum
    {
        CPU_ITIMER,
        NET_DEVICE
    } query_struct;

    if (argc == 1)
    {
        fprintf(stderr, "enter struct type as 2nd argument: 0 -- CPU_ITIMER, 1 -- NET_DEVICE\n",
                "as 3rd argument -- pid\n",
                "as clock type as 4th argument (only for cpu_itimer): 0 -- CLOCK_PROF, 1 -- CLOCK_VIRT\n");
    } else if (argc >= 3)
    {
        if ( (strcmp(argv[1], "0") == 0) && (argc != 4) )
        {
            fprintf(stderr, "enter clock type as 4th argument: 0 -- CLOCK_PROF, 1 -- CLOCK_VIRT");
            return 1;

            if ((strcmp(argv[3], "0") == 0) || (strcmp(argv[3], "1") == 0)) clk_id = atoi(argv[3]);
            else
            {
                fprintf(stderr, "0 -- CLOCK_PROF, 1 -- CLOCK_VIRT");
                return 1;
            }
        }
        if ( (strcmp(argv[1], "0") == 0) || (strcmp(argv[1], "1") == 0) ) query_struct = atoi(argv[1]);
        else
        {
            fprintf(stderr, "0 -- cpu_ititmer, 1 -- net_device\n");
            return 1;
        }
    } else {
        fprintf(stderr, "enter struct type as 2nd argument: 0 -- CPU_ITIMER, 1 -- NET_DEVICE\n",
                "as 3rd argument -- pid\n",
                "as clock type as 4th argument (only for cpu_itimer): 0 -- CLOCK_PROF, 1 -- CLOCK_VIRT");
        return 1;
    }

    fd = open(file_name, O_RDWR);
    pid = atoi(argv[2]);

    if (fd == -1)
    {
        perror("driver open");
        return 2;
    }

    switch (query_struct)
    {
        case CPU_ITIMER:
            clk_id = atoi(argv[3]);
            get_cpu_itimer_structure(fd, pid, clk_id);
            break;
        case NET_DEVICE:
            get_net_device_structure(fd, pid);
            break;
        default:
            break;
    }
    close (fd);
    return 0;
}