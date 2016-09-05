#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#define MAX_THREAD (32)

#define PROC_FILE_PATH_SIZE (255)

typedef struct cpu_info_
{
    long long user;
    long long nice;
    long long system;
    long long idle;
    long long iowait;
    long long irq;
    long long softirq;
    long long steal;
    long long guest;
    long long guest_nice;
} cpu_info;

// @brief This structure is based on following proc man infomation.
// @ref https://linuxjm.osdn.jp/html/LDP_man-pages/man5/proc.5.html
typedef struct process_info_
{
    // 1
    int pid;

    // 2
    char comm[255];

    // 14 : user time
    long long utime;

    // 15 : kernel time
    long long stime;

    // 39
    int processor;

} process_info;

void get_cpu_info(cpu_info *info)
{
    FILE *fd = NULL;

    fd = fopen("/proc/stat", "r");
    if(fd == NULL)
    {
        printf("[ERROR] open error\n");
        return;
    }

    fscanf(fd, "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
            &info->user, &info->nice, &info->system, &info->idle,
            &info->iowait, &info->irq, &info->softirq, &info->steal,
            &info->guest, &info->guest_nice);

    fclose(fd);
}

void put_process_info(FILE *fd, process_info *info)
{
    char str[100];
    int pos = 1;

    while(fscanf(fd, "%s", str) != EOF)
    {
        switch(pos)
        {
            case 1:
                info->pid = atoi(str);
                break;
            case 2:
                strcpy(info->comm, str);
                break;
            case 14:
                info->utime = atoll(str);
                break;
            case 15:
                info->stime = atoll(str);
                break;
            case 39:
                info->processor = atoi(str);
                break;
        }

        pos++;
    }
}

void get_process_info(int pid, process_info *info)
{
    char path[PROC_FILE_PATH_SIZE];
    FILE *fd = NULL;

    sprintf(path, "/proc/%d/stat", pid);

    fd = fopen(path, "r");
    if(fd == NULL)
    {
        printf("[ERROR] open error (pid = %d)\n", pid);
        return;
    }

    put_process_info(fd, info);

    fclose(fd);
}

void get_lwp_info(int pid, int tid, process_info *info)
{
    char path[PROC_FILE_PATH_SIZE];
    FILE *fd = NULL;

    sprintf(path, "/proc/%d/task/%d/stat", pid, tid);

    fd = fopen(path, "r");
    if(fd == NULL)
    {
        printf("[ERROR] open error(pid = %d, tid = %d)\n", pid, tid);
        return;
    }

    put_process_info(fd, info);

    fclose(fd);
}

void get_thread_list(int pid, int list[], int list_max, int *thread_num)
{
    char path[PROC_FILE_PATH_SIZE];
    DIR *dir;
    struct dirent *dp;
    int pos = 0;

    sprintf(path, "/proc/%d/task", pid);

    dir = opendir(path);
    if(dir == NULL)
    {
        printf("[ERROR] opendir error\n");
        return;
    }

    while((dp = readdir(dir)) != NULL)
    {
        if(strcmp(".", dp->d_name) == 0)
        {
            continue;
        }
        if(strcmp("..", dp->d_name) == 0)
        {
            continue;
        }

        list[pos] = atoi(dp->d_name);
        pos++;

        if(list_max == pos)
        {
            break;
        }
    }

    *thread_num = pos;

    closedir(dir);
}

int main(int argc, char *argv[])
{
    int i, j;
    int pid;
    int SC_CLK_TCK = sysconf(_SC_CLK_TCK);

    if(argc != 2)
    {
        printf("usage : procflow <pid>\n");
        return 1;
    }

    pid = atoi(argv[1]);

    while(1)
    {
        cpu_info     cinfo[2];
        process_info pinfo[2];
        int thread_num[2];
        int thread_list[2][MAX_THREAD];
        process_info tinfo[2][MAX_THREAD];

        get_cpu_info(&cinfo[0]);

//        printf("user   = %f[sec]\n", (double)cinfo.user/(double)SC_CLK_TCK);
//        printf("system = %f[sec]\n", (double)cinfo.system/(double)SC_CLK_TCK);
//        printf("idle   = %f[sec]\n", (double)cinfo.idle/(double)SC_CLK_TCK);

        get_process_info(pid, &pinfo[0]);

//        printf("pid       = %d\n", pinfo.pid);
//        printf("comm      = %s\n", pinfo.comm);
//        printf("utime     = %llu\n", pinfo.utime);
//        printf("stime     = %llu\n", pinfo.stime);
//        printf("processor = %llu\n", pinfo.processor);

        get_thread_list(pid, thread_list[0], MAX_THREAD, &thread_num[0]);
        for(i = 0 ; i < thread_num[0] ; i++)
        {
            get_lwp_info(pid, thread_list[0][i], &tinfo[0][i]);
        }

        usleep(1000*100);

        get_cpu_info(&cinfo[1]);
        get_process_info(pid, &pinfo[1]);

        get_thread_list(pid, thread_list[1], MAX_THREAD, &thread_num[1]);
        for(i = 0 ; i < thread_num[1] ; i++)
        {
            get_lwp_info(pid, thread_list[1][i], &tinfo[1][i]);
        }

        long long cdiff[4] = {
            (cinfo[1].user   - cinfo[0].user),
            (cinfo[1].nice   - cinfo[0].nice),
            (cinfo[1].system - cinfo[0].system),
            (cinfo[1].idle   - cinfo[0].idle)
        };

        long long total_time = cdiff[0] + cdiff[1] + cdiff[2] + cdiff[3];

        printf("Total : user = %3.2f%%, nice = %3.2f%%, system = %3.2f%%, idle = %3.2f%%\n",
                cdiff[0]*100.0/(double)total_time, 
                cdiff[1]*100.0/(double)total_time, 
                cdiff[2]*100.0/(double)total_time, 
                cdiff[3]*100.0/(double)total_time);

        long long pdiff[2] = {
            (pinfo[1].utime - pinfo[0].utime),
            (pinfo[1].stime - pinfo[0].stime),
        };

        printf("Process : user = %3.2f%%, system = %3.2f%%\n",
                pdiff[0]*100.0/(double)total_time, 
                pdiff[1]*100.0/(double)total_time);

        // search same thread
        for(i = 0 ; i < thread_num[0] ; i++)
        {
            for(j = 0 ; j < thread_num[1] ; j++)
            {
                if(thread_list[0][i] == thread_list[1][j])
                {
                    // find same thread
                    long long tdiff[2] = {
                        (tinfo[1][j].utime - tinfo[0][i].utime),
                        (tinfo[1][j].stime - tinfo[0][i].stime),
                    };

                    printf("Thread(%d) : core = %d, user = %3.2f%%, system = %3.2f%%\n",
                            thread_list[0][i],
                            tinfo[1][j].processor,  // choose latest Processor ID
                            tdiff[0]*100.0/(double)total_time, 
                            tdiff[1]*100.0/(double)total_time);

                    break;
                }
            }
        }
    }
}
