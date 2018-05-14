#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "SortedList.h"


void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
    if (list == NULL || element == NULL || list->key != NULL)
        return;
    SortedList_t*  iterator = list->next;
    while ( iterator != list && strcmp( iterator->key, element->key) <= 0) {
        if (opt_yield & INSERT_YIELD)
            sched_yield();
         iterator =  iterator->next;
    }
    if (opt_yield & INSERT_YIELD)
        sched_yield();
    iterator =  iterator->prev;
    element->next =  iterator->next;
    element->prev =  iterator;
    iterator->next = element;
    element->next->prev = element;
    
    return;
}

int SortedList_delete( SortedListElement_t *element)
{
    /*check if element->next and element->prev are NULL??  nvm it is doubly linked circular list*/
    if(element==NULL||element->next->prev!=element||element->prev->next!=element)
        return 1;
    if(opt_yield &DELETE_YIELD)
        sched_yield();
    element->next->prev = element->prev;
    element->prev->next = element->next;
    return 0;
}


SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
    if(list==NULL||key==NULL||list->key!=NULL)
        return NULL;
    SortedListElement_t *iterator = list->next;
    while(iterator!=list)
    {
        const char* tempstr = iterator->key;
        if(strcmp(tempstr,key)==0)
            return iterator;
        if(opt_yield &LOOKUP_YIELD)
            sched_yield();
        iterator = iterator->next;
    }
    return NULL;
}

int SortedList_length(SortedList_t *list)
{
    if(list==NULL||list->key!=NULL)
        return -1;
    SortedList_t* iterator = list->next;
    int result = 0;
    while(iterator!=list)
    {
        if(iterator->next->prev!=iterator || iterator->prev->next!=iterator)
            return -1;
        result ++;
        if(opt_yield &LOOKUP_YIELD)
            sched_yield();
        iterator = iterator->next;
    }
    return result;
}