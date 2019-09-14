/* Copyright 1994-2012 The MathWorks, Inc.
 *
 * File    : rt_sim.c     
 * Abstract:
 *   Performs one time step of a real-time single tasking or multitasking
 *   system for statically or dynamically allocated timing data.
 *
 *   The tasking mode is controlled by the MULTITASKING #define.
 *
 *   The data allocation type is controlled by the RT_MALLOC #define.
 */


#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stddef.h>
#include "tmwtypes.h"
#ifdef USE_RTMODEL
# include "simstruc_types.h"
#else
# include "simstruc.h"
#endif
#include "rt_sim.h"

/*==========*
 * Struct's *
 *==========*/

/*
 * TimingData
 */

#ifdef RT_MALLOC

/* dynamically allocate data */
typedef struct TimingData_Tag {
    real_T *period;       /* Task periods in seconds                   */
    real_T *offset;       /* Task offsets in seconds                   */
    real_T *clockTick;    /* Flint task time tick counter              */
    int_T  *taskTick;     /* Counter for determining task hits         */
    int_T  *nTaskTicks;   /* Number base rate ticks for a task hit     */
    int_T  firstDiscIdx;  /* First discrete task index                 */
} TimingData;

#else

/* statically allocate data */
typedef struct TimingData_Tag {
    real_T period[NUMST];       /* Task periods in seconds                   */
    real_T offset[NUMST];       /* Task offsets in seconds                   */
    real_T clockTick[NUMST];    /* Flint task time tick counter              */
    int_T  taskTick[NUMST];     /* Counter for determining task hits         */
    int_T  nTaskTicks[NUMST];   /* Number base rate ticks for a task hit     */
    int_T  firstDiscIdx;        /* First discrete task index                 */
} TimingData;

#endif

/*=========================*
 * Data local to this file *
 *=========================*/

#if defined(RT_MALLOC) || defined(USE_RTMODEL)
/* "td" will be a pointer to a dynamically allocated struct */
#else
/* statically allocate the struct. "td" will point to this static struct */
static TimingData td_struct;
#endif

/*==================*
 * Visible routines *
 *==================*/

/* Function: rt_SimInitTimingEngine ============================================
 * Abstract:
 *      This function is for use with single tasking or multitasking
 *      real-time systems.  
 *
 *      Initializes the timing engine for a fixed-step real-time system. 
 *      It is assumed that start time is 0.0.
 *
 * Returns:
 *      NULL     - success
 *      non-NULL - error string
 */
const char *rt_SimInitTimingEngine(int_T       rtmNumSampTimes,
                                   real_T      rtmStepSize,
                                   real_T      *rtmSampleTimePtr,
                                   real_T      *rtmOffsetTimePtr,
                                   int_T       *rtmSampleHitPtr,
                                   int_T       *rtmSampleTimeTaskIDPtr,
                                   real_T      rtmTStart,
                                   SimTimeStep *rtmSimTimeStepPtr,
                                   void        **rtmTimingDataPtr)
{
#ifdef USE_RTMODEL

    /* In the USE_RTMODEL case this function does nothing */

    UNUSED_PARAMETER(rtmNumSampTimes);
    UNUSED_PARAMETER(rtmStepSize); 
    UNUSED_PARAMETER(rtmSampleTimePtr);
    UNUSED_PARAMETER(rtmOffsetTimePtr);
    UNUSED_PARAMETER(rtmSampleHitPtr);
    UNUSED_PARAMETER(rtmSampleTimeTaskIDPtr);
    UNUSED_PARAMETER(rtmSimTimeStepPtr);
    UNUSED_PARAMETER(rtmTimingDataPtr);

    if (rtmTStart != 0.0) {
        return("Start time must be zero for real-time systems.  For non-zero start times you must use the Simulink solver module");
    } else {
        return(NULL);
    }

#else /* must be !USE_RTMODEL */

    int_T     i;
    int       *tsMap     = rtmSampleTimeTaskIDPtr;
    real_T    *period    = rtmSampleTimePtr;
    real_T    *offset    = rtmOffsetTimePtr;
    int_T     *sampleHit = rtmSampleHitPtr;
    real_T    stepSize   = rtmStepSize;

#ifdef RT_MALLOC

    int_T numst = rtmNumSampTimes;

    /* In the dynamically allocated case, we allocate the data here */
    static const char_T *malloc_error;
    TimingData *td;
    malloc_error = "Memory allocation error";
    td = (TimingData *) malloc(sizeof(TimingData));
    if (!td) {
        return(malloc_error);
    }

    td->period = (real_T *) malloc(numst * sizeof(real_T));
    if (!td->period) {
        return(malloc_error);
    }

    td->offset = (real_T *) malloc(numst * sizeof(real_T));
    if (!td->offset) {
        return(malloc_error);
    }

    td->clockTick = (real_T *) malloc(numst * sizeof(real_T));
    if (!td->clockTick) {
        return(malloc_error);
    }

    td->taskTick = (int_T *) malloc(numst * sizeof(int_T));
    if (!td->taskTick) {
        return(malloc_error);
    }

    td->nTaskTicks = (int_T *) malloc(numst * sizeof(int_T));
    if (!td->nTaskTicks) {
        return(malloc_error);
    }
    if (rtmTStart != 0.0) {
        return("Start time must be zero for real-time systems.  For non-zero start times you must use the Simulink solver module");
    }

#else /* must be !RT_MALLOC */

    /* In the statically allocated case, we point to the static structure */
    TimingData *td;
    td = &td_struct;

    /* Also, we use the constant NUMST instead of what was passed in */
    rtmNumSampTimes = NUMST; 

#endif /* !RT_MALLOC */

    if (rtmTStart != 0.0) {
        return("Start time must be zero for real-time systems.  For non-zero start times you must use the Simulink solver module");
    }

    *rtmSimTimeStepPtr = MAJOR_TIME_STEP;

    *rtmTimingDataPtr = (void*)&td;

    for (i = 0; i < rtmNumSampTimes; i++) {
        tsMap[i]         = i;
        td->period[i]     = period[i];
        td->offset[i]     = offset[i];
        td->nTaskTicks[i] = (int_T)floor(period[i]/stepSize + 0.5);
        if (td->period[i] == CONTINUOUS_SAMPLE_TIME ||
            td->offset[i] == 0.0) {
            td->taskTick[i]  = 0;
            td->clockTick[i] = 0.0;
            sampleHit[i]    = 1;
        } else {
            td->taskTick[i]  = (int_T)floor((td->period[i]-td->offset[i]) /
                                            stepSize+0.5);
            td->clockTick[i] = -1.0;
            sampleHit[i]    = 0;
        }
    }

    /* Correct first sample time if continuous task */
    td->period[0]     = stepSize;
    td->nTaskTicks[0] = 1; 

    /* Set first discrete task index */
    if (rtmNumSampTimes == 1)
        td->firstDiscIdx = (int_T)(period[0] == CONTINUOUS_SAMPLE_TIME);
    else
        td->firstDiscIdx = ((int_T)(period[0] == CONTINUOUS_SAMPLE_TIME) + 
                          (int_T)(period[1] == CONTINUOUS_SAMPLE_TIME));

    return(NULL); /* success */

#endif /* ! USE_RTMODEL case */

} /* end rt_SimInitTimingEngine */

/* In the statically-allocated case, rt_SimDestroyTimingEngine is not needed. */
#ifdef RT_MALLOC

/* Dynamically allocated data */
/* Function: rt_SimDestroyTimingEngine ===========================================
 * Abstract:
 *      This function frees the timing engine data.
 */
void rt_SimDestroyTimingEngine(void *rtmTimingData)
{
#ifdef USE_RTMODEL
    UNUSED_PARAMETER(rtmTimingData);
#else /* must be !USE_RTMODEL */
    TimingData *td;
    td = (TimingData *)rtmTimingData;

    if (td) {
        if (td->period) {
            free(td->period);
        }
        
        if (td->offset) {
            free(td->offset);
        }
        
        if (td->clockTick) {
            free(td->clockTick);
        }
        
        if (td->taskTick) {
            free(td->taskTick);
        }
        
        if (td->nTaskTicks) {
            free(td->nTaskTicks);
        }
        free(td);
    }
#endif /* !USE_RTMODEL */
} /* end rt_SimDestroyTimingEngine */

#endif /* RT_MALLOC */

#if !defined(MULTITASKING)

/*###########################################################################*/
/*########################### SINGLE TASKING ################################*/
/*###########################################################################*/

/* Function: rt_SimGetNextSampleHit ============================================
 * Abstract:
 *      For a single tasking real-time system, return time of next sample hit.
 */
/* This function has a different signature in the RT_MALLOC case */
#ifdef RT_MALLOC
time_T rt_SimGetNextSampleHit(void   *rtmTimingData,
                              int_T   rtmNumSampTimes)
#else
time_T rt_SimGetNextSampleHit(void)
#endif
{
#ifdef USE_RTMODEL

    /* The USE_RTMODEL version of this function does nothing */

#ifdef RT_MALLOC
    UNUSED_PARAMETER(rtmTimingData);
    UNUSED_PARAMETER(rtmNumSampTimes);
#endif
    return -1;

#else /* must be !USE_RTMODEL */

    time_T timeOfNextHit;
#ifdef RT_MALLOC
    TimingData *td;
    td = (TimingData *)rtmTimingData;
#else
    TimingData *td;
    int_T rtmNumSampTimes;
    td = &td_struct;
    rtmNumSampTimes = NUMST; /* it's not passed in, in this case */
#endif
    td->clockTick[0] += 1;
    timeOfNextHit = td->clockTick[0] * td->period[0];

    if(rtmNumSampTimes > 1) {
        int i;
        for (i = 1; i < rtmNumSampTimes; i++) {
            if (++td->taskTick[i] == td->nTaskTicks[i]) {
                td->taskTick[i] = 0;
                td->clockTick[i]++;
            }
        }
    }

    return(timeOfNextHit);

#endif /* !USE_RTMODEL */
} /* end rt_SimGetNextSampleHit */



/* Function: rt_SimUpdateDiscreteTaskSampleHits ================================
 * Abstract:
 *      This function is for use with single tasking real-time systems.  
 *      
 *      If the number of sample times is greater than one, then we need to 
 *      update the discrete task sample hits for the next time step. Note, 
 *      task 0 always has a hit since it's sample time is the fundamental 
 *      sample time.
 */
void rt_SimUpdateDiscreteTaskSampleHits(int_T  rtmNumSampTimes,
                                        void   *rtmTimingData,
                                        int_T  *rtmSampleHitPtr,
                                        real_T *rtmTPtr)
{
#ifdef USE_RTMODEL

    /* The USE_RTMODEL version of this function does nothing */
    UNUSED_PARAMETER(rtmNumSampTimes);
    UNUSED_PARAMETER(rtmTimingData);
    UNUSED_PARAMETER(rtmSampleHitPtr);
    UNUSED_PARAMETER(rtmTPtr);
    return;

#else /* must be !USE_RTMODEL */

    int   i;
    int_T *sampleHit;

#ifdef RT_MALLOC
    TimingData *td;
    td = (TimingData *)rtmTimingData;
#else
    TimingData *td;
    UNUSED_PARAMETER(rtmTimingData);
    td = &td_struct;
    rtmNumSampTimes = NUMST;
#endif

    sampleHit = rtmSampleHitPtr;

    for (i = td->firstDiscIdx; i < rtmNumSampTimes; i++) {
        int_T hit = (td->taskTick[i] == 0);
        if (hit) {
            rttiSetTaskTime(rtmTPtr, i,
                            td->clockTick[i]*td->period[i] + td->offset[i]);
        }
        sampleHit[i] = hit;
    }
#endif /* !USE_RTMODEL */
} /* rt_SimUpdateDiscreteTaskSampleHits */



#else /* defined(MULTITASKING) */

/*###########################################################################*/
/*############################## MULTITASKING ###############################*/
/*###########################################################################*/


#ifndef USE_RTMODEL
/* This function is not defined here if USE_RTMODEL is set */

/* Function: rt_SimUpdateDiscreteEvents ========================================
 * Abstract:
 *      This function is for use with multitasking real-time systems. 
 *
 *      This function updates the status of the RT_MODEL sampleHits
 *      flags and the perTaskSampleHits matrix which is used to determine 
 *      when special sample hits occur.
 *
 *      The RT_MODEL contains a matrix, called perTaskSampleHits. 
 *      This matrix is used by the ssIsSpecialSampleHit macro. The row and 
 *      column indices are both task id's (equivalent to the root RT_MODEL 
 *      sample time indices). This is a upper triangle matrix. This routine 
 *      only updates the slower task hits (kept in column j) for row
 *      i if we have a sample hit in row i.
 *
 *                       column j
 *           tid   0   1   2   3   4   5  
 *               -------------------------
 *             0 |   | X | X | X | X | X |
 *         r     -------------------------
 *         o   1 |   |   | X | X | X | X |      This matrix(i,j) answers:
 *         w     -------------------------      If we are in task i, does
 *             2 |   |   |   | X | X | X |      slower task j have a hit now?
 *         i     -------------------------
 *             3 |   |   |   |   | X | X |
 *               -------------------------
 *             4 |   |   |   |   |   | X |      X = 0 or 1
 *               -------------------------
 *             5 |   |   |   |   |   |   |
 *               -------------------------
 *
 *      How macros index this matrix:
 *
 *          ssSetSampleHitInTask(S, j, i, X)   => matrix(i,j) = X
 *
 *          ssIsSpecialSampleHit(S, my_sti, promoted_sti, tid) => 
 *              (tid_for(promoted_sti) == tid && !minor_time_step &&
 *               matrix(tid,tid_for(my_sti)) 
 *              )
 *
 *            my_sti       = My (the block's) original sample time index.
 *            promoted_sti = The block's promoted sample time index resulting
 *                           from a transition via a ZOH from a fast to a 
 *                           slow block or a transition via a unit delay from 
 *                           a slow to a fast block.
 *
 *      The perTaskSampleHits array, of dimension n*n, is accessed using 
 *      perTaskSampleHits[j + i*n] where n is the total number of sample
 *      times, 0 <= i < n, and 0 <= j < n.  The C language stores arrays in 
 *      row-major order, that is, row 0 followed by row 1, etc.
 * 
 */
time_T rt_SimUpdateDiscreteEvents(int_T  rtmNumSampTimes,
                                  void   *rtmTimingData,
                                  int_T  *rtmSampleHitPtr,
                                  int_T  *rtmPerTaskSampleHits)
{
    int   i, j;
    int_T *sampleHit;
#ifdef RT_MALLOC
    TimingData *td;
    td = (TimingData *)rtmTimingData;
    rtmNumSampTimes = NUMST;
#else    
    TimingData *td;
    UNUSED_PARAMETER(rtmTimingData);
    td = &td_struct;
#endif
    sampleHit = rtmSampleHitPtr;
    
    /*
     * Run this loop in reverse so that we do lower priority events first.
     */
    i = rtmNumSampTimes;
    while (--i >= 0) {
        if (td->taskTick[i] == 0) {
            /* 
             * Got a sample hit, reset the counter, and update the clock
             * tick counter.
             */
            sampleHit[i] = 1;
            td->clockTick[i]++;

            /*
             * Record the state of all "slower" events 
             */
            for (j = i + 1; j < rtmNumSampTimes; j++) {
                rttiSetSampleHitInTask(rtmPerTaskSampleHits, rtmNumSampTimes,
                                       j, i, sampleHit[j]);
            }
        } else {
            /*
             * no sample hit, increment the counter 
             */
            sampleHit[i] = 0;
        }

        if (++td->taskTick[i] == td->nTaskTicks[i]) { /* update for next time */
            td->taskTick[i] = 0;
        }
    }

    return(td->clockTick[0]*td->period[0]);
    
} /* rt_SimUpdateDiscreteEvents */

#endif /* !USE_RTMODEL */

/* Function: rt_SimUpdateDiscreteTaskTime ======================================
 * Abstract:
 *      This function is for use with multitasking systems. 
 *
 *      After a discrete task output and update has been performed, this 
 *      function must be called to update the discrete task time for next 
 *      time around.
 */
void rt_SimUpdateDiscreteTaskTime(real_T *rtmTPtr,
                                  void   *rtmTimingData,
                                  int    tid)
{
#ifdef USE_RTMODEL
    /* This function is a no-op in USE_RTMODEL */
    UNUSED_PARAMETER(rtmTPtr);
    UNUSED_PARAMETER(rtmTimingData);
    UNUSED_PARAMETER(tid);
    return;

#else /* must be !USE_RTMODEL */

#ifdef RT_MALLOC
    TimingData *td;
    td = (TimingData *)rtmTimingData;
#else
    TimingData *td;
    UNUSED_PARAMETER(rtmTimingData);
    td = &td_struct;
#endif
    rttiSetTaskTime(rtmTPtr, tid,
                    td->clockTick[tid]*td->period[tid] + td->offset[tid]);

#endif /* !USE_RTMODEL */
}

#endif /* MULTITASKING */

/*
 *******************************************************************************
 * FUNCTIONS MAINTAINED FOR BACKWARDS COMPATIBILITY WITH THE SimStruct
 *******************************************************************************
 */
#ifndef USE_RTMODEL
const char *rt_InitTimingEngine(SimStruct *S)
{
    const char_T *retVal = rt_SimInitTimingEngine(
        ssGetNumSampleTimes(S),
        ssGetStepSize(S),
        ssGetSampleTimePtr(S),
        ssGetOffsetTimePtr(S),
        ssGetSampleHitPtr(S),
        ssGetSampleTimeTaskIDPtr(S),
        ssGetTStart(S),
        &ssGetSimTimeStep(S),
        &ssGetTimingData(S));
    return(retVal);
}

# if !defined(MULTITASKING)
void rt_UpdateDiscreteTaskSampleHits(SimStruct *S)
{
    rt_SimUpdateDiscreteTaskSampleHits(
        ssGetNumSampleTimes(S),
        ssGetTimingData(S),
        ssGetSampleHitPtr(S),
        ssGetTPtr(S));
}

#ifdef RT_MALLOC
time_T rt_GetNextSampleHit(SimStruct *S)
{
    return(rt_SimGetNextSampleHit(
        ssGetTimingData(S),
        ssGetNumSampleTimes(S)));
}
#else
time_T rt_GetNextSampleHit(void)
{
    return(rt_SimGetNextSampleHit());
}
#endif

# else /* MULTITASKING */

time_T rt_UpdateDiscreteEvents(SimStruct *S)
{
    return(rt_SimUpdateDiscreteEvents(
               ssGetNumSampleTimes(S),
               ssGetTimingData(S),
               ssGetSampleHitPtr(S),
               ssGetPerTaskSampleHitsPtr(S)));
}

void rt_UpdateDiscreteTaskTime(SimStruct *S, int tid)
{
    rt_SimUpdateDiscreteTaskTime(ssGetTPtr(S), ssGetTimingData(S), tid);
}

#endif /* MULTITASKING */
#endif /* USE_RTMODEL */

/* EOF: rt_sim.c */
