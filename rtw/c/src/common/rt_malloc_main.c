/*
 * Copyright 2012-2018 The MathWorks, Inc.
 *
 * File    : rt_malloc_main.c
 *
 * Abstract:
 *      A Generic "Real-Time (single tasking or pseudo-multitasking,
 *      dynamically allocated data)" main that runs under most 
 *      operating systems.
 *
 *      This file may be a useful starting point when targeting a new 
 *      processor or microcontroller.
 *
 *
 * Compiler specified defines:
 *      MODEL=modelname - Required.
 *	NUMST=#         - Required. Number of sample times.
 *      TID01EQ=1 or 0  - Optional. Only define to 1 if sample time task
 *                        id's 0 and 1 have equal rates.
 *      MULTITASKING    - Optional. (use MT for a synonym).
 *	SAVEFILE        - Optional (non-quoted) name of .mat file to create. 
 *			  Default is <MODEL>.mat
 */

/*==================*
 * Required defines *
 *==================*/

#ifndef MODEL
# error Must specify a model name.  Define MODEL=name.
#else
/* create generic macros that work with any model */
# define EXPAND_CONCAT(name1,name2) name1 ## name2
# define CONCAT(name1,name2) EXPAND_CONCAT(name1,name2)
# define MODEL_INITIALIZE CONCAT(MODEL,_initialize)
# define MODEL_STEP       CONCAT(MODEL,_step)
# define MODEL_TERMINATE  CONCAT(MODEL,_terminate)
# define RT_MDL_TYPE      CONCAT(MODEL,_M_TYPE)
#endif

#ifndef NUMST
# error Must specify the number of sample times.  Define NUMST=number.
#endif

#if CLASSIC_INTERFACE == 1
# error "Classic call interface is not supported by rt_malloc_main.c."
#endif

#if ONESTEPFCN==0
#error Separate output and update functions are not supported by rt_malloc_main.c. \
You must update rt_malloc_main.c to suit your application needs, or select \
the 'Single output/update function' option.
#endif

#if TERMFCN==0
#error The terminate function is required by rt_malloc_main.c. \
You must update rt_malloc_main.c to suit your application needs, or select \
the 'Terminate function required' option.
#endif

#if ALLOCATIONFCN==0 
# error An allocation function is required by rt_malloc_main.c. \
You must update rt_malloc_main.c to allocate model instance data, or select \
the 'Use dynamic memory allocation for model initialization' option.
#endif

#define QUOTE1(name) #name
#define QUOTE(name) QUOTE1(name)    /* need to expand name    */

#ifndef SAVEFILE
# define MATFILE2(file) #file ".mat"
# define MATFILE1(file) MATFILE2(file)
# define MATFILE MATFILE1(MODEL)
#else
# define MATFILE QUOTE(SAVEFILE)
#endif

/*==========*
 * Includes *
 *==========*/

#include "rtwtypes.h"
#if !defined(INTEGER_CODE) || INTEGER_CODE == 0
# include <stdio.h>    /* optional for printf */
#else
#ifdef __cplusplus
extern "C" {
#endif
  extern int printf(const char *, ...); 
  extern int fflush(void *);
#ifdef __cplusplus
}
#endif
#endif
#include <string.h>  /* optional for strcmp */
#include "rtmodel.h" /* optional for automated builds */

#include "rt_logging.h"
#ifdef UseMMIDataLogging
#include "rt_logging_mmi.h"
#endif
#include "ext_work.h"

#ifdef MODEL_STEP_FCN_CONTROL_USED
#error The static version of rt_malloc_main.c does not support model step function prototype control.
#endif

#if ROOT_IO_FORMAT != 2
# error rt_malloc_main.c requires root-level I/O to be passed as part of model \
data structure.                                                                \
You must update rt_malloc_main.c to suit your application needs, or set        \
'Pass root-level I/O as' parameter to 'Part of model data structure'. 
#endif

/*========================* 
 * Setup for multitasking * 
 *========================*/

/* 
 * Let MT be synonym for MULTITASKING (to shorten command line for DOS) 
 */
#if defined(MT)
# if MT == 0
# undef MT
# else
# define MULTITASKING 1
# endif
#endif

#if defined(TID01EQ) && TID01EQ == 1
#define FIRST_TID 1
#else 
#define FIRST_TID 0
#endif

/*====================*
 * External functions *
 *====================*/
extern RT_MDL_TYPE *MODEL(void);
extern void MODEL_INITIALIZE(RT_MDL_TYPE *S);
extern void MODEL_TERMINATE(RT_MDL_TYPE  *S);

#if !defined(MULTITASKING)
extern void MODEL_STEP(RT_MDL_TYPE *S);       /* single-rate step function */
#else
extern void MODEL_STEP(RT_MDL_TYPE *S, int_T tid);  /* multirate step function */
#endif


/*==================================*
 * Global data local to this module *
 *==================================*/
#ifndef MULTITASKING
static boolean_T OverrunFlags[1];    /* ISR overrun flags */
static boolean_T eventFlags[1];      /* necessary for overlapping preemption */
#else
static boolean_T OverrunFlags[NUMST];
static boolean_T eventFlags[NUMST]; 
#endif

const char *RT_MEMORY_ALLOCATION_ERROR = "memory allocation error"; 

/*=================*
 * Local functions *
 *=================*/

#if !defined(MULTITASKING) /* single task */

/* Function: rtOneStep ========================================================
 *
 * Abstract:
 *   Perform one step of the model.  This function is modeled such that
 *   it could be called from an interrupt service routine (ISR) with minor
 *   modifications.
 */
static void rt_OneStep(RT_MDL_TYPE *S)
{
    /* Disable interrupts here */

    /***********************************************
     * Check and see if base step time is too fast *
     ***********************************************/
    if (OverrunFlags[0]++) {
        rtmSetErrorStatus(S, "Overrun");
    }

    /*************************************************
     * Check and see if an error status has been set *
     * by an overrun or by the generated code.       *
     *************************************************/
    if (rtmGetErrorStatus(S) != NULL) {
        return;
    }

    /* Save FPU context here (if necessary) */
    /* Reenable interrupts here */
    /* Set model inputs here */

    /**************
     * Step model *
     **************/
    MODEL_STEP(S);

    /* Get model outputs here */

    /**************************
     * Decrement overrun flag *
     **************************/
    OverrunFlags[0]--;

    rtExtModeCheckEndTrigger();

    /* Disable interrupts here */
    /* Restore FPU context here (if necessary) */
    /* Reenable interrupts here */

} /* end rtOneStep */

#else /* multitask */

/* Function: rtOneStep ========================================================
 *
 * Abstract:
 *      Perform one step of the model. This function is modeled such that
 *      it could be called from an interrupt service routine (ISR) with minor
 *      modifications.
 *
 *      This routine is modeled for use in a multitasking environment and
 *	therefore needs to be fully re-entrant when it is called from an
 *	interrupt service routine.
 *
 * Note:
 *      Error checking is provided which will only be used if this routine
 *      is attached to an interrupt.
 *
 */
static void rt_OneStep(RT_MDL_TYPE *S)
{
    int_T i;

    /* Disable interrupts here */

    /***********************************************
     * Check and see if base step time is too fast *
     ***********************************************/
    if (OverrunFlags[0]++) {
        rtmSetErrorStatus(S, "Overrun");
    }

    /*************************************************
     * Check and see if an error status has been set *
     * by an overrun or by the generated code.       *
     *************************************************/
    if (rtmGetErrorStatus(S) != NULL) {
        return;
    }

    /* Save FPU context here (if necessary) */
    /* Reenable interrupts here */
    
    /*************************************************
     * Update EventFlags and check subrate overrun   *
     *************************************************/
    for (i = FIRST_TID+1; i < NUMST; i++) {
        if (rtmStepTask(S,i) && eventFlags[i]++) {
            OverrunFlags[0]--;
            OverrunFlags[i]++;
            /* Sampling too fast */
            rtmSetErrorStatus(S, "Overrun");
            return;
        }
        if (++rtmTaskCounter(S,i) == rtmCounterLimit(S,i))
            rtmTaskCounter(S, i) = 0;
    }

    /* Set model inputs associated with base rate here */

    /*******************************************
     * Step the model for the base sample time *
     *******************************************/
    MODEL_STEP(S,0);

    /* Get model outputs associated with base rate here */

    /************************************************************************
     * Model step complete for base sample time, now it is okay to          *
     * re-interrupt this ISR.                                               *
     ************************************************************************/
    OverrunFlags[0]--;

    /*********************************************************
     * Step the model for any other sample times (subrates) *
     *********************************************************/
    for (i = FIRST_TID+1; i < NUMST; i++) {
        /*************************************************************
         * If task "i" is running, don't run any lower priority task *
         *************************************************************/
        if (OverrunFlags[i]) return; 

        if (eventFlags[i]) {
            OverrunFlags[i]++;

            /* Set model inputs associated with subrate here */

            /******************************************
             * Step the model for sample time "i" *
             ******************************************/
            MODEL_STEP(S,i);

            /* Get model outputs associated with subrate here */
            
            /**********************************************
             * Indicate task complete for sample time "i" *
             **********************************************/
            OverrunFlags[i]--;
            eventFlags[i]--;
        }
    }

    rtExtModeCheckEndTrigger();
    /* Disable interrupts here */
    /* Restore FPU context here (if necessary) */
    /* Enable interrupts here */

} /* end rtOneStep */

#endif /* MULTITASKING */

/* Function: rt_InitModel ====================================================
 * 
 * Abstract: 
 *   Initialized the model and the overrun flags
 *
 */
static void rt_InitModel(RT_MDL_TYPE  *S)
{
#if defined(MULTITASKING)
    int i;
    for(i=0; i < NUMST; i++) {
        OverrunFlags[i] = 0;
        eventFlags[i] = 0;
    }
#else
    OverrunFlags[0] = 0;
    eventFlags[0] = 0;
#endif

    /************************
     * Initialize the model *
     ************************/
    MODEL_INITIALIZE(S);
}

/* Function: rt_TermModel ====================================================
 * 
 * Abstract:
 *   Terminates the model and prints the error status
 *
 */
static int_T rt_TermModel(RT_MDL_TYPE  *S)
{
    const char_T *errStatus = (const char_T *) (rtmGetErrorStatus(S));
    int_T i = 0;
    
    if (errStatus != NULL && strcmp(errStatus, "Simulation finished")) {
        (void)printf("%s\n", errStatus);
#if defined(MULTITASKING)
        for (i = 0; i < NUMST; i++) {
            if (OverrunFlags[i]) {
                (void)printf("ISR overrun - sampling rate too"
                             "fast for sample time index %d.\n", i);
            }
        }
#else
        if (OverrunFlags[i]) { 
            (void)printf("ISR overrun - base sampling rate too fast.\n");
        }
#endif
        MODEL_TERMINATE(S);
        return(1);
    }
    
    MODEL_TERMINATE(S);
    return(0);
}

/* Function: main =============================================================
 *
 * Abstract:
 *   Execute model on a generic target such as a workstation.
 */
int_T main(int_T argc, const char *argv[])
{
    RT_MDL_TYPE  *S;
    const char_T *errmsg;
    int_T ret;

    /* External mode */
    rtParseArgsForExtMode(argc, argv);
 
    /*******************************************
     * warn if the model will run indefinitely *
     *******************************************/
#if MAT_FILE==0 && EXT_MODE==0
    printf("warning: the simulation will run with no stop time; "
           "to change this behavior select the 'MAT-file logging' option\n");
    fflush(NULL);
#endif

    /************************
     * Initialize the model *
     ************************/
    
    S = MODEL();
    if (S == NULL) {
        (void)fprintf(stderr,"Memory allocation error during model "
                      "registration");
        return(1);
    }
    errmsg = (const char_T *) (rtmGetErrorStatus(S));
    if (errmsg != NULL) {
        (void)fprintf(stderr,"Error during model registration: %s\n", errmsg);
        MODEL_TERMINATE(S);
        return(1);
    }

#ifdef UseMMIDataLogging
    rt_FillStateSigInfoFromMMI(rtmGetRTWLogInfo(S), &rtmGetErrorStatus(S));
#endif
    errmsg = rt_StartDataLogging(rtmGetRTWLogInfo(S), 
                                 rtmGetTFinal(S),
                                 rtmGetStepSize(S),
                                 &rtmGetErrorStatus(S));

    if (errmsg != NULL) {
        (void)fprintf(stderr,"Error starting data logging: %s\n",errmsg);
        MODEL_TERMINATE(S);
        return(1);
    }
    
    /* External mode */
    rtSetTFinalForExtMode(&rtmGetTFinal(S));
    rtExtModeCheckInit(NUMST);
    rtExtModeWaitForStartPkt(rtmGetRTWExtModeInfo(S),
                             NUMST,
                             (boolean_T *)&rtmGetStopRequested(S));

    (void)printf("\n** starting the model **\n");

    rt_InitModel(S);

    /***********************************************************************
     * Execute (step) the model.  You may also attach rtOneStep to an ISR, *
     * in which case you replace the call to rtOneStep with a call to a    *
     * background task.  Note that the generated code sets error status    *
     * to "Simulation finished" when MatFileLogging is specified in TLC.   *
     ***********************************************************************/
    while (rtmGetErrorStatus(S) == NULL &&
           !rtmGetStopRequested(S)) {

        rtExtModePauseIfNeeded(rtmGetRTWExtModeInfo(S),
                               NUMST,
                               (boolean_T *)&rtmGetStopRequested(S));

        if (rtmGetStopRequested(S)) break;

        /* external mode */
        rtExtModeOneStep(rtmGetRTWExtModeInfo(S),
                         NUMST,
                         (boolean_T *)&rtmGetStopRequested(S));
        
        rt_OneStep(S);        
    }

    /********************
     * Cleanup and exit *
     ********************/

#ifdef UseMMIDataLogging
    rt_CleanUpForStateLogWithMMI(rtmGetRTWLogInfo(S));
#endif
    rt_StopDataLogging(MATFILE,rtmGetRTWLogInfo(S));

    ret = rt_TermModel(S);

    rtExtModeShutdown(NUMST);

    return ret;
}

/* EOF: rt_malloc_main.c */
