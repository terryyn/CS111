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

static int thread_num = 1;
static int iterations = 1;
int yield_flag = 0;
int insert_flag = 0;
int delete_flag = 0;
int lookup_flag = 0;
int opt_yield = 0;
int sync_flag = 0;
char sync_opt ;
static SortedList_t* list= NULL;
static SortedListElement_t * elements = NULL;
static int element_num;
static int max_length = 10;
static int lock = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void free_all()
{
    free(list);
    free(elements);
    return;
}

char* random_string()
{
    char available[62] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int length =  rand()%max_length;
    char* s = (char*)malloc(sizeof(char)*length);
    int i=0;
    for(;i<length;i++)
        s[i] = available[rand()%62];
    return s;
}

void* thread_init(void* a)
{
    int index = *((int*)a);
    int i = index;
    for(;i<index+iterations;i++)
    {
        if(!sync_flag)
            SortedList_insert(list,&elements[i]);
        else if(sync_opt =='m')
        {
            pthread_mutex_lock(&mutex);
            SortedList_insert(list,&elements[i]);
            pthread_mutex_unlock(&mutex);
        }
        else if(sync_opt=='s')
        {
            while(__sync_lock_test_and_set(&lock,1))
                ;
            SortedList_insert(list,&elements[i]);
            __sync_lock_release(&lock);
        }
    }
    int list_length = 0;
    if(!sync_flag)
        list_length = SortedList_length(list);
    else if(sync_opt=='m')
    {
        pthread_mutex_lock(&mutex);
        list_length = SortedList_length(list);
        pthread_mutex_unlock(&mutex);
    }
    else if(sync_opt=='s')
    {
        while(__sync_lock_test_and_set(&lock,1))
            ;
        list_length = SortedList_length(list);
        __sync_lock_release(&lock);
    }

    if(list_length<0)
    {
        fprintf(stderr,"list length error\n");
        exit(1);
    }
    i  = index;
    for(;i<index+iterations;i++)
    {
        if(!sync_flag)
        {
            SortedListElement_t* temp = SortedList_lookup(list,elements[i].key);
            if(temp==NULL)
            {
                fprintf(stderr,"error: element not in list \n");
                exit(2);
            }
            if(SortedList_delete(temp))
            {
                fprintf(stderr,"error when deleting element \n");
                
                exit(2);
            }
        }
        else if(sync_opt =='m')
        {
            pthread_mutex_lock(&mutex);
            SortedListElement_t* temp = SortedList_lookup(list,elements[i].key);
            if(temp==NULL)
            {
                fprintf(stderr,"error: element not in list \n");
                exit(2);
            }
            if(SortedList_delete(temp))
            {
                fprintf(stderr,"error when deleting element \n");
                
                exit(2);
            }
            pthread_mutex_unlock(&mutex);
        }
        else if(sync_opt=='s')
        {
            while(__sync_lock_test_and_set(&lock,1))
                ;
            SortedListElement_t* temp = SortedList_lookup(list,elements[i].key);
            if(temp==NULL)
            {
                fprintf(stderr,"error: element not in list \n");
                exit(2);
            }
            if(SortedList_delete(temp))
            {
                fprintf(stderr,"error when deleting element \n");
                
                exit(2);
            }
            __sync_lock_release(&lock);
        }
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
    {"sync",required_argument,0,'s'}
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
        else{
            fprintf(stderr,"invalid argument\n");
            exit(1);
        }
    }
    /* initializes an empty list.*/
    list = (SortedList_t*)malloc(sizeof(SortedList_t));
    list->prev = list;
    list->next = list;
    list->key = NULL;
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
    pthread_t* thread_list = (pthread_t*)malloc(sizeof(pthread_t)*thread_num);
    if(thread_list==NULL)
    {
        fprintf(stderr,"error when malloc threads\n");
        free_all();
        exit(1);
    }
    i = 0;
    int index[thread_num] ;
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
    int length_end =  SortedList_length(list);
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
    long long run_time = 1000000000*(end_time.tv_sec - start_time.tv_sec);
    int operation_num = thread_num*iterations*3;
    run_time +=end_time.tv_nsec;
    run_time -= start_time.tv_nsec;
    int average = run_time / operation_num;
    printf("%s,%d,%d,%d,%d,%lld,%d\n",tag,thread_num,iterations,1,operation_num,run_time,average);
    free_all();
    free(thread_list);
    exit(0);
}
