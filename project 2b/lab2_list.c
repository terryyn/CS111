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
#include "SortedList.h"

struct sublist
{
    SortedList_t* own_list;
    pthread_mutex_t mutex_lock;
    int spin_lock;
};
typedef struct sublist sublist_t;

static int thread_num = 1;
static int iterations = 1;
int yield_flag = 0;
int insert_flag = 0;
int delete_flag = 0;
int lookup_flag = 0;
int opt_yield = 0;
int sync_flag = 0;
char sync_opt ;

static SortedListElement_t * elements = NULL;
static int element_num;
static int max_length = 10;
static int lock = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
long long lock_time[100] = {0};
sublist_t* list_list;
static int list_num = 1;



unsigned int hash(const char* key)
{
    unsigned int value = 0;
    const char* temp = key;
    while(*temp!='\0')
    {
        value+= (*temp-'a');
        temp++;
    }
    return value%list_num;
}

void free_all()
{
    free(elements);
    int i=0;
    for(;i<list_num;i++)
    {
        free(list_list[i].own_list);
    }
    free(list_list);
    return;
}

char* random_string()
{
    char available[27] = "abcdefghijklmnopqrstuvwxyz";
    int length =  rand()%max_length;
    char* s = (char*)malloc(sizeof(char)*length);
    int i=0;
    for(;i<length;i++)
        s[i] = available[rand()%26];
    return s;
}

void* thread_init(void* a)
{
    struct timespec before,after;
    int index = *((int*)a);
    int i = index;
    int threadid = index/iterations;
    for(;i<index+iterations;i++)
    {
        int hash_index = hash(elements[i].key);
        if(!sync_flag)
            SortedList_insert(list_list[hash_index].own_list,&elements[i]);
        else if(sync_opt =='m')
        {
            if(clock_gettime(CLOCK_MONOTONIC,&before)<0)
            {
                fprintf(stderr,"error when get time \n");
                exit(1);
            }
            pthread_mutex_lock(&list_list[hash_index].mutex_lock);
            if(clock_gettime(CLOCK_MONOTONIC,&after)<0)
            {
                fprintf(stderr,"error when get time \n");
                exit(1);
            }
            lock_time[threadid]+=1000000000*(after.tv_sec-before.tv_sec);
            lock_time[threadid]+=after.tv_nsec;
            lock_time[threadid]-=before.tv_nsec;
            SortedList_insert(list_list[hash_index].own_list,&elements[i]);
            pthread_mutex_unlock(&list_list[hash_index].mutex_lock);
        }
        else if(sync_opt=='s')
        {
            if(clock_gettime(CLOCK_MONOTONIC,&before)<0)
            {
                fprintf(stderr,"error when get time \n");
                exit(1);
            }
            while(__sync_lock_test_and_set(&list_list[hash_index].spin_lock,1))
                ;
            if(clock_gettime(CLOCK_MONOTONIC,&after)<0)
            {
                fprintf(stderr,"error when get time \n");
                exit(1);
            }
            lock_time[threadid]+=1000000000*(after.tv_sec-before.tv_sec);
            lock_time[threadid]+=after.tv_nsec;
            lock_time[threadid]-=before.tv_nsec;
            SortedList_insert(list_list[hash_index].own_list,&elements[i]);
            __sync_lock_release(&list_list[hash_index].spin_lock);
        }
    }
    int list_length = 0;
    for(i=0;i<list_num;i++)
    {
        if(!sync_flag)
            list_length += SortedList_length(list_list[i].own_list);
        else if(sync_opt=='m')
        {
            if(clock_gettime(CLOCK_MONOTONIC,&before)<0)
            {
                fprintf(stderr,"error when get time \n");
                exit(1);
            }
            pthread_mutex_lock(&list_list[i].mutex_lock);
            if(clock_gettime(CLOCK_MONOTONIC,&after)<0)
            {
                fprintf(stderr,"error when get time \n");
                exit(1);
            }
            lock_time[threadid]+=1000000000*(after.tv_sec-before.tv_sec);
            lock_time[threadid]+=after.tv_nsec;
            lock_time[threadid]-=before.tv_nsec;
            list_length += SortedList_length(list_list[i].own_list);
            pthread_mutex_unlock(&list_list[i].mutex_lock);
        }
        else if(sync_opt=='s')
        {
            if(clock_gettime(CLOCK_MONOTONIC,&before)<0)
            {
                fprintf(stderr,"error when get time \n");
                exit(1);
            }
            while(__sync_lock_test_and_set(&list_list[i].spin_lock,1))
                ;
           if(clock_gettime(CLOCK_MONOTONIC,&after)<0)
            {
                fprintf(stderr,"error when get time \n");
                exit(1);
            }
            lock_time[threadid]+=1000000000*(after.tv_sec-before.tv_sec);
            lock_time[threadid]+=after.tv_nsec;
            lock_time[threadid]-=before.tv_nsec;
            list_length += SortedList_length(list_list[i].own_list);
            __sync_lock_release(&list_list[i].spin_lock);
        }
    }
    i  = index;
    for(;i<index+iterations;i++)
    {
        int hash_index = hash(elements[i].key);
        if(!sync_flag)
        {
            SortedListElement_t* temp = SortedList_lookup(list_list[hash_index].own_list,elements[i].key);
            if(temp==NULL)
            {
                fprintf(stderr,"error: element not in list \n");
                free_all();
                exit(2);
            }
            if(SortedList_delete(temp))
            {
                fprintf(stderr,"error when deleting element \n");
                free_all();
                exit(2);
            }
        }
        else if(sync_opt =='m')
        {
            if(clock_gettime(CLOCK_MONOTONIC,&before)<0)
            {
                fprintf(stderr,"error when get time \n");
                exit(1);
            }
            pthread_mutex_lock(&list_list[hash_index].mutex_lock);
            if(clock_gettime(CLOCK_MONOTONIC,&after)<0)
            {
                fprintf(stderr,"error when get time \n");
                exit(1);
            }
            lock_time[threadid]+=1000000000*(after.tv_sec-before.tv_sec);
            lock_time[threadid]+=after.tv_nsec;
            lock_time[threadid]-=before.tv_nsec;
            SortedListElement_t* temp = SortedList_lookup(list_list[hash_index].own_list,elements[i].key);
            if(temp==NULL)
            {
                fprintf(stderr,"error: element not in list \n");
                free_all();
                exit(2);
            }
            if(SortedList_delete(temp))
            {
                fprintf(stderr,"error when deleting element \n");
                free_all();
                exit(2);
            }
            pthread_mutex_unlock(&list_list[hash_index].mutex_lock);
        }
        else if(sync_opt=='s')
        {
            if(clock_gettime(CLOCK_MONOTONIC,&before)<0)
            {
                fprintf(stderr,"error when get time \n");
                exit(1);
            }
            while(__sync_lock_test_and_set(&list_list[hash_index].spin_lock,1))
                ;
            if(clock_gettime(CLOCK_MONOTONIC,&after)<0)
            {
                fprintf(stderr,"error when get time \n");
                exit(1);
            }
            lock_time[threadid]+=1000000000*(after.tv_sec-before.tv_sec);
            lock_time[threadid]+=after.tv_nsec;
            lock_time[threadid]-=before.tv_nsec;
            SortedListElement_t* temp = SortedList_lookup(list_list[hash_index].own_list,elements[i].key);
            if(temp==NULL)
            {
                fprintf(stderr,"error: element not in list \n");
                free_all();
                exit(2);
            }
            if(SortedList_delete(temp))
            {
                fprintf(stderr,"error when deleting element \n");
                free_all();
                exit(2);
            }
            __sync_lock_release(&list_list[hash_index].spin_lock);
        }
    }   
    if(list_length<0)
    {
        fprintf(stderr,"list length error\n");
        free_all();
        exit(1);
    } 
    return NULL;
}

void segment_handler()
{
    fprintf(stderr,"catch segmentation fault\n");
    exit(2);
}
int main(int argc,char** argv)
{
    signal(SIGSEGV,segment_handler);
    int c;
    static struct option long_options[]={
    {"threads",required_argument,0,'t'},
    {"iterations",required_argument,0,'i'},
    {"yield",required_argument,0,'y'},
    {"sync",required_argument,0,'s'},
    {"lists",required_argument,0,'l'}
    };
    while(1)
    {
        c = getopt_long(argc,argv,"t:i:y:",long_options,NULL);
        if(c==-1)
            break;
        else if(c=='t')
            thread_num = atoi(optarg);
        else if(c=='i')
            iterations = atoi(optarg);
        else if(c=='y'){
            yield_flag = 1;
            opt_yield = 1;
            if(strlen(optarg)>3)
            {
                fprintf(stderr,"invald option for --yield\n");
                exit(1);
            }
            size_t i=0;
            for(;i<strlen(optarg);i++)
            {
                char yield_option = optarg[i];
                if(yield_option=='i'){
                    insert_flag = 1;
                    opt_yield |= INSERT_YIELD;
                }
                else if(yield_option=='d'){
                    delete_flag = 1;
                    opt_yield |= DELETE_YIELD;
                }
                else if(yield_option=='l'){
                    lookup_flag = 1;
                    opt_yield |= LOOKUP_YIELD;
                }
                else{
                    fprintf(stderr,"invalid option for --yield\n");
                    exit(1);
                }
            }
        }
        else if(c=='s')
        {
            sync_flag = 1;
            sync_opt = optarg[0];
        }
        else if(c=='l')
        {
            list_num  = atoi(optarg);
        }
        else{
            fprintf(stderr,"invalid argument\n");
            exit(1);
        }
    }
    /* initializes an empty list.*/
    /*creates and initializes (with random keys) the required number (threads x iterations) of list elements*/
    element_num  = thread_num*iterations;
    elements = (SortedListElement_t*)malloc(sizeof(SortedListElement_t)*element_num);
    int i = 0;
    for(;i<element_num;i++)
    {
        elements[i].key=random_string();
        elements[i].prev = &elements[i];
        elements[i].next = &elements[i];
    }
    /*notes the (high resolution) starting time for the run (using clock_gettime(3)).*/
    struct timespec start_time;
    int getstart_result = clock_gettime(CLOCK_MONOTONIC,&start_time);
    if(getstart_result<0)
    {
        fprintf(stderr,"gettime error\n");
        free_all();
        exit(1);
    }
    /*starts the specified number of threads.
    each thread:
    starts with a set of pre-allocated and initialized elements (--iterations=#)
    inserts them all into a (single shared-by-all-threads) list
    gets the list length
    looks up and deletes each of the keys it had previously inserted
    exits to re-join the parent thread*/
    list_list = (sublist_t*)malloc(list_num*sizeof(sublist_t));
    if(list_list==NULL)
    {
        fprintf(stderr,"error when malloc list_list\n");
        free(elements);
        exit(1);
    }
    
    for(i=0;i<list_num;i++)
    {
        list_list[i].own_list = (SortedList_t*)malloc(sizeof(SortedList_t));
        list_list[i].own_list->prev = list_list[i].own_list;
        list_list[i].own_list->next = list_list[i].own_list;
        list_list[i].own_list->key = NULL;
        pthread_mutex_init(&list_list[i].mutex_lock,NULL);
        list_list[i].spin_lock = 0;
    }

    pthread_t* thread_list = (pthread_t*)malloc(sizeof(pthread_t)*thread_num);
    if(thread_list==NULL)
    {
        fprintf(stderr,"error when malloc threads\n");
        free_all();
        exit(1);
    }
    i = 0;
    int index[thread_num];
    for(;i<thread_num;i++)
        index[i] = i*iterations;
    i = 0;
    for(;i<thread_num;i++)
    {
        int temp_result = pthread_create(&thread_list[i],NULL,thread_init,(void*)&index[i]);
        if(temp_result<0)
        {
            fprintf(stderr,"error when create thread\n");
            free_all();
            free(thread_list);
            exit(1);
        }
    }
    i = 0;
    for(;i<thread_num;i++)
    {
        int join_result = pthread_join(thread_list[i],NULL);
        if(join_result<0)
        {
            fprintf(stderr,"error when join thread\n");
            free_all();
            free(thread_list);
            exit(1);
        }
    }
    /*waits for all threads to complete, and notes the (high resolution) ending time for the run.*/
    struct timespec end_time;
    int getend_result = clock_gettime(CLOCK_MONOTONIC,&end_time);
    if(getend_result<0)
    {
        fprintf(stderr,"gettime error\n");
        free_all();
        free(thread_list);
        exit(1);
    }
    /*checks the length of the list to confirm that it is zero.*/
    int length_end =  0;    
    for(i=0;i<list_num;i++)
    {
        length_end += SortedList_length(list_list[i].own_list);
    }
    if(length_end>0)
    {
        fprintf(stderr,"length not zero but %d\n",length_end);
        free_all();
        free(thread_list);
        exit(2);
    }
    if(length_end<0)
    {
        fprintf(stderr,"inconsistence in list\n");
        free_all();
        free(thread_list);
        exit(2);        
    }
    /*the name of the test, which is of the form: list-yieldopts-syncopts: where
    yieldopts = {none, i,d,l,id,il,dl,idl}
    syncopts = {none,s,m}*/
    char tag[15];
    char* base = "list-";
    strcpy(tag,base);
    if(yield_flag)
    {
        if(insert_flag)
            strcat(tag,"i");
        if(delete_flag)
            strcat(tag,"d");
        if(lookup_flag)
            strcat(tag,"l");
        strcat(tag,"-");
    }
    else
        strcat(tag,"none-");
    if(sync_flag)
    {
        if(sync_opt=='s')
            strcat(tag,"s");
        else if(sync_opt=='m')
            strcat(tag,"m");
    }
    else
    {
        char* temp = "none";
        strcat(tag,temp);
    }
    long long total_locktime = 0;
    for(i=0;i<thread_num;i++)
    {
        total_locktime+=lock_time[i];
    }
    long long run_time = 1000000000*(end_time.tv_sec - start_time.tv_sec);
    int operation_num = thread_num*iterations*3;
    int lock_operation_num  = thread_num*(iterations*2+1);
    run_time +=end_time.tv_nsec;
    run_time -= start_time.tv_nsec;
    int average = run_time / operation_num;
    long long average_locktime  =total_locktime/lock_operation_num;
    printf("%s,%d,%d,%d,%d,%lld,%d,%lld\n",tag,thread_num,iterations,list_num,operation_num,run_time,average,average_locktime);
    free_all();
    free(thread_list);
    exit(0);
}
