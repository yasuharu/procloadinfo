#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define MAX_THREAD (10)

pthread_t thread_id[MAX_THREAD];
int cancel_flag = 0;

void proc(void *arg)
{
    while(cancel_flag == 0)
    {
    }
}

int main(int argc, char *argv[])
{
    int thread_num = 0;
    int i;

    if(argc != 2)
    {
        printf("usage : test <number of threads>\n");
        return 1;
    }

    thread_num = atoi(argv[1]);

    for(i = 0 ; i < thread_num ; i++)
    {
        pthread_create(&thread_id[i], NULL, (void*)proc, NULL);
    }

    printf("If you want to stop this program, please press any key.\n");
    getchar();

    cancel_flag = 1;

    for(i = 0 ; i < thread_num ; i++)
    {
        pthread_join(thread_id[i], NULL);
    }

    return 0;
}
