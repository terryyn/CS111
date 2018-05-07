/* pthread_create : attr=NULL
   pthread_join: no need to return 
   Wall time vs CPU time:  real time vs CPU working time on thread
        when CPUtime>Wall: multiple thread running at the same time 
   
   use gnuplot version 5 (not 4.6!) --> modify the script gnuplot location
   --yield will make each iteration time longer

*/
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>


long long count = 0;
int thread_num = 1;
int iterations = 1;
int opt_yield = 0;
char opt_sync = 'n';
int sync_flag  = 0;
pthread_mutex_t mutex =  PTHREAD_MUTEX_INITIALIZER;
static int my_spinlock = 0;

void add(long long *pointer, long long value) {
    long long sum = *pointer + value;
    if (opt_yield)
            sched_yield();
    *pointer = sum;
}
void* add_temp()
{
    int i = 0;
    for(;i<iterations;i++)
    {
        if(!sync_flag)
            add(&count,1);
        else if(opt_sync =='m') /*mutex*/
        {
            pthread_mutex_lock(&mutex);
            add(&count,1);
            pthread_mutex_unlock(&mutex);
        }
        else if(opt_sync=='s') /*spin lock*/
        {
            while(__sync_lock_test_and_set(&my_spinlock,1))
                ;
            add(&count,1);
            __sync_lock_release(&my_spinlock);
        }
        else if(opt_sync=='c')
        {
            long long old,new;
            do
            {
                if(opt_yield)
                    sched_yield();
                old  = count;
                new = old+1;
            }
            while(__sync_val_compare_and_swap(&count,old,new)!=old);
        }
    }
    int k = 0;
    for(;k<iterations;k++)
        if(!sync_flag)
            add(&count,-1);
        else if(opt_sync =='m') /*mutex*/
        {
            pthread_mutex_lock(&mutex);
            add(&count,-1);
            pthread_mutex_unlock(&mutex);
        }
        else if(opt_sync=='s') /*spin lock*/
        {
            while(__sync_lock_test_and_set(&my_spinlock,1))
                ;
            add(&count,-1);
            __sync_lock_release(&my_spinlock);
        }
        else if(opt_sync=='c')
        {
            long long old,new;
            do
            {
                if(opt_yield)
                    sched_yield();
                old  = count;
                new = old-1;
            }
            while(__sync_val_compare_and_swap(&count,old,new)!=old);
        }
    return NULL;
}

void check(int a,char* s)
{
    if(a<0)
    {
        fprintf(stderr,"%s error: %s\n",s,strerror(errno));
        exit(1);
    }
}

int main(int argc,char** argv)
{
    int c;
    static struct option long_options[]={
        {"threads",required_argument,0,'t'},
        {"iterations",required_argument,0,'i'},
        {"yield",no_argument,0,'y'},
        {"sync",required_argument,0,'s'}
    };
    while(1)
    {
        c = getopt_long(argc,argv,"t:i:y:s:",long_options,NULL);
        if(c==-1)
            break;
        else if(c=='t')
            thread_num = atoi(optarg);
        else if(c=='i')
            iterations = atoi(optarg);
        else if(c=='y')
            opt_yield = 1;
        else if(c=='s'){
            sync_flag =1;
            opt_sync = optarg[0];
        }

        else{
            fprintf(stderr,"invalid argument\n");
            exit(1);
        }
    }
    struct timespec start_time;
    int clock_result = clock_gettime(CLOCK_MONOTONIC,&start_time);
    check(clock_result,"getstarttime");
    int i=0;
    pthread_t* thread_list = (pthread_t*)malloc(sizeof(pthread_t)*thread_num);
    if(thread_list==NULL)
    {
        check(-1,"malloc threads");
    }
    for(;i<thread_num;i++)
    {
        int thread_temp = pthread_create(&thread_list[i],NULL,add_temp,NULL);
        if(thread_temp<0)
        {
            fprintf(stderr,"thread_create error:%s\n",strerror(errno));
            free(thread_list);
            exit(1);
        }  /* cannot use check here because of memory leak */
    }
    i = 0;
    for(;i<thread_num;i++)
    {
        int join_temp = pthread_join(thread_list[i],NULL);
        if(join_temp<0)
        {
            fprintf(stderr,"thread_join error:%s\n",strerror(errno));
            free(thread_list);
            exit(1);
        }
    }
    struct timespec end_time;
    int endclock_result = clock_gettime(CLOCK_MONOTONIC,&end_time);
    if (endclock_result<0)
    {
        fprintf(stderr,"clock_gettime error:%s\n",strerror(errno));
        free(thread_list);
        exit(1);
    }
    int operation_num = thread_num*iterations*2;
    long long run_time = 1000000000*(end_time.tv_sec - start_time.tv_sec);
    run_time +=end_time.tv_nsec;
    run_time -= start_time.tv_nsec;
    int average = run_time / operation_num;
    char* tag = "add-none";
    if(opt_yield && !sync_flag)
        tag = "add-yield-none";
    else if(opt_yield && sync_flag &&opt_sync =='m')
        tag = "add-yield-m";
    else if(opt_yield && sync_flag &&opt_sync =='s')
        tag = "add-yield-s";
    else if(opt_yield && sync_flag &&opt_sync =='c')
        tag = "add-yield-c";
    else if(!opt_yield&&opt_sync =='m')
        tag = "add-m";
    else if(!opt_yield&&opt_sync =='s')
        tag = "add-s";
    else if(!opt_yield&&opt_sync =='c')
        tag = "add-c";  
    printf("%s,%d,%d,%d,%lld,%d,%lld\n",tag,thread_num,iterations,operation_num,run_time,average,count);
    exit(0);
}

