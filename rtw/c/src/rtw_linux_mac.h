#ifndef RTW_LINUX_MAC_H
#define RTW_LINUX_MAC_H

/*
 * Copyright 2011 The MathWorks, Inc.
 *
 * File: rtw_linux_mac.h
 *
 * Abstract:
 *  Function prototypes and defines pthread mutex/semaphores
 */
#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>

#ifndef __USE_UNIX98
#define __USE_UNIX98
#endif

extern void rtw_pthread_mutex_init_mac( void** mutexDW );

#define rtw_pthread_mutex_lock_mac( mutexDW )                   \
    pthread_mutex_lock((pthread_mutex_t *)(mutexDW));

#define rtw_pthread_mutex_unlock_mac( mutexDW )                 \
    pthread_mutex_unlock((pthread_mutex_t *)(mutexDW));

#define rtw_pthread_mutex_destroy_mac( mutexDW )                \
    pthread_mutex_destroy((pthread_mutex_t *)(mutexDW));        \
    free(mutexDW);

extern void rtw_pthread_sem_create_mac( void** semaphoreDW1, 
                                 void** semaphoreDW2, 
                                 long initVal );


#define rtw_pthread_sem_wait_mac( semaphoreDW ) \
    sem_wait(semaphoreDW);

#define rtw_pthread_sem_post_mac( semaphoreDW ) \
    sem_post(semaphoreDW);

#define rtw_pthread_sem_destroy_mac( semaphoreDW ) \
    sem_unlink((char*)(semaphoreDW));              \
    free(semaphoreDW);


#ifdef __cplusplus
}
#endif

#endif /* RTW_LINUX_MAC_H */

/* LocalWords:  pthread
 */
