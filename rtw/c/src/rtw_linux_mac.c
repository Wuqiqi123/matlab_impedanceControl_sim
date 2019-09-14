/* Copyright 2011-2016 The MathWorks, Inc. */
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef __USE_UNIX98
#define __USE_UNIX98
#endif

extern int pthread_mutexattr_setprotocol(pthread_mutexattr_t *, int);

extern int pthread_mutexattr_settype(pthread_mutexattr_t *, int);

void rtw_pthread_mutex_init_mac( void** mutexDW )
{
    pthread_mutexattr_t attr;       
    pthread_mutexattr_setprotocol(&attr, 1);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    *(mutexDW) = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init((pthread_mutex_t *)(*(mutexDW)), &attr);
}

void rtw_pthread_sem_create_mac( void** semaphoreDW1, void** semaphoreDW2, long initVal )
{
    static int semcount = 0;
    char* semName = malloc(sizeof(char)*32);
    sprintf(semName, "sem_sync_%x%x", semcount++, getpid());
    
    *semaphoreDW1 = sem_open(semName, O_CREAT | O_EXCL, 0777, (int)initVal);
    *semaphoreDW2 = (void*)semName;
}

/* LocalWords:  PTHREAD PRIO DW
 */
