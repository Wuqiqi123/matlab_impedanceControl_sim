/* Copyright 2011 The MathWorks, Inc. */
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>

#ifndef __USE_UNIX98
#define __USE_UNIX98
#endif

extern int pthread_mutexattr_setprotocol(pthread_mutexattr_t *, int);
extern int pthread_mutexattr_settype(pthread_mutexattr_t *, int);

void rtw_pthread_mutex_init( void** mutexDW )
{
    pthread_mutexattr_t attr; 
    pthread_mutexattr_setprotocol(&attr, 1); /* PTHREAD_PRIO_INHERIT */ 
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    *mutexDW = malloc(sizeof(pthread_mutex_t)); 
    pthread_mutex_init((pthread_mutex_t *)(*mutexDW), &attr);
}

/* LocalWords:  PTHREAD PRIO
 */
