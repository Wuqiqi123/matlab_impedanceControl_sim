/*
 * Abstract:
 *   Main program for the Rapid Simulation target.
 *
 * Compiler specified defines:
 *      MODEL=modelName - Required.
 *	SAVEFILE        - Optional (non-quoted) name of MAT-file to create.
 *			  Default is <MODEL>.mat
 *      MULTITASKING    - Simulate multitasking mode.
 *
 * Copyright 1994-2018 The MathWorks, Inc.
 */


#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "tmwtypes.h"
#include "simstruc.h"
#include "rt_logging.h"
#include "rtmodel.h"
#ifdef UseMMIDataLogging
#include "rt_logging_mmi.h"
#endif
#include "rt_nonfinite.h"
#include "rsim.h"
#include "rsim_sup.h"

#include "ext_work.h"

#ifdef RSIM_WITH_SL_SOLVER
#ifdef __cplusplus

extern "C" {

#endif

# include "sl_solver_rtw.h"

#ifdef __cplusplus

}
#endif

#else
# include "rt_sim.h"
#endif

/*=========*
 * Defines *
 *=========*/

#ifndef TRUE
#define FALSE (0)
#define TRUE  (1)
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE  1
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS  0
#endif

#ifndef MODEL
# error "must define MODEL"
#endif

#ifndef RSIM_WITH_SL_SOLVER
# ifndef NUMST
#  error "must define number of sample times, NUMST"
# endif
# ifndef NCSTATES
#  error "must define NCSTATES"
# endif
#endif

#define GOTO_EXIT_IF_ERROR(msg, cond)         \
        if (cond) {                           \
            errorPrefix = msg;                \
            goto EXIT_POINT;                  \
        }

#define ERROR_EXIT(msg, cond)                 \
        if (cond) {                           \
            (void)fprintf(stderr, msg, cond); \
            return(EXIT_FAILURE);             \
        }

/*====================*
 * External functions *
 *====================*/

#ifdef __cplusplus

extern "C" {

#endif

extern SimStruct *MODEL(void);

extern void MdlInitializeSizes(void);
extern void MdlInitializeSampleTimes(void);
extern void MdlStart(void);
extern void MdlOutputs(int_T tid);
extern void MdlUpdate(int_T tid);
extern void MdlTerminate(void);

#ifdef __cplusplus

}
#endif

#ifndef RSIM_WITH_SL_SOLVER
# if NCSTATES > 0
#ifdef __cplusplus

extern "C" {

#endif
   extern void rt_CreateIntegrationData(SimStruct *S);
   extern void rt_UpdateContinuousStates(SimStruct *S);
#ifdef __cplusplus

}
#endif

# else
#  define rt_CreateIntegrationData(S)  ssSetSolverName(S,"FixedStepDiscrete");
#  define rt_UpdateContinuousStates(S) ssSetT(S,ssGetSolverStopTime(S));
# endif
#endif

#ifdef EXT_MODE
#  define rtExtModeSingleTaskUpload(S)                          \
   {                                                            \
        int stIdx;                                              \
        rtExtModeUploadCheckTrigger(ssGetNumSampleTimes(S));    \
        for (stIdx=0; stIdx<ssGetNumSampleTimes(S); stIdx++) {  \
            if (ssIsSampleHit(S, stIdx, 0 /*unused*/)) {        \
                rtExtModeUpload(stIdx,ssGetTaskTime(S,stIdx));  \
            }                                                   \
        }                                                       \
   }
#else
#  define rtExtModeSingleTaskUpload(S) /* Do nothing */
#endif


#define SL_MAX(A, B)   (((A) > (B)) ? (A) : (B))


/*=============*
 * Global data *
 *=============*/

const char gblModelName[] = QUOTE(MODEL);

#ifdef __cplusplus

extern "C" {

#endif
extern int gblTimeLimit;

extern const char* gblSlvrJacPatternFileName;
extern const char* gblInportFileName;

#ifdef __cplusplus

}

#endif



#ifdef RSIM_WITH_SL_SOLVER
#ifdef RSIM_WITH_SOLVER_MULTITASKING /* MULTITASKING WITH FIXED STEP SOLVERS */


# if TID01EQ == 1
#  define FIRST_TID 1
# else
#  define FIRST_TID 0
# endif

/* Function: rsimOneStep =======================================================
 *
 *      Perform one step of the model.
 *      Errors are set in the SimStruct's ErrorStatus, NULL means no errors.
 */
static void rsimOneStep(SimStruct *S)
{
    int_T  i;
    int_T* sampleHit = ssGetSampleHitPtr(S);

    ssSetSimTimeStep(S, MAJOR_TIME_STEP);

    /* Clear the flag that blocks set in their major time step methods (output,
       update, etc.) to flag that they have changed their state. If this flag is
       true then we need to run model outputs again at the first minor step
       (i.e., at the same time as the last major step).*/
    ssClearContTimeOutputInconsistentWithStateAtMajorStep(S);
    ssClearBlkStateChange(S);

#ifdef DEBUG_TIMING
    rsimDisplayTimingData(S,
                          sizeof(struct SimStruct_tag),
                          sizeof(struct _ssMdlInfo));
#endif
    rtExtModeOneStep(ssGetRTWExtModeInfo(S),
                     ssGetNumSampleTimes(S),
                     (boolean_T*)&ssGetStopRequested(S));

    /* Setup the task times and sample hit flags for the discrete rates */
    rsimUpdateDiscreteTaskTimesAndSampleHits(S);
    if (ssGetErrorStatus(S) != NULL) return;

    ssSetLogOutput(S,TRUE);

    /* Do output, log, update for the base rate */
    MdlOutputs(FIRST_TID);

    rtExtModeUploadCheckTrigger(ssGetNumSampleTimes(S));
    rtExtModeUpload(FIRST_TID, ssGetTaskTime(S, FIRST_TID));

    if (ssGetLogOutput(S)) {
        (void)rt_UpdateTXYLogVars(ssGetRTWLogInfo(S), ssGetTPtr(S));
        if (ssGetErrorStatus(S) != NULL) return;
    }

    MdlUpdate(FIRST_TID);
    if (ssGetErrorStatus(S) != NULL) return;

    /* Do not log outputs during minor time steps */
    ssSetLogOutput(S, FALSE);
    ssSetTimeOfLastOutput(S,ssGetT(S));

    /* Call the solver push time forward based on the continuous dynamics */
    if (ssGetSampleTime(S,0) == CONTINUOUS_SAMPLE_TIME &&
        !ssGetStopRequested(S) ) {
        rsimUpdateSolverStopTimeForFixedStepMultiTaskingSim(S);
        if (!ssGetStopRequested(S)) rsimAdvanceSolver(S);
    }

    /* Do output and update for the remaining rates */
    ssSetLogOutput(S, TRUE);
    for (i=FIRST_TID+1; i<NUMST; i++) {
        if ( !sampleHit[i] ) continue;
        MdlOutputs(i);
        rtExtModeUpload(i, ssGetTaskTime(S,i));
        MdlUpdate(i);
    }
    if (ssGetErrorStatus(S) != NULL) return;
    ssSetLogOutput(S, FALSE);

    rtExtModeCheckEndTrigger();

    /* Update the timing engine and determine the solver stop time */
    rsimUpdateTimingEngineAndSolverStopTime(S);
    if (ssGetErrorStatus(S) != NULL) return;

} /* rsimOneStep */

#else /* SINGLE-TASKING WITH SOLVER MODULE */

/* Function: rsimOutputLogUpdate ===============================================
 *
 */
static void rsimOutputLogUpdate(SimStruct* S)
{
    double currTime  = ssGetT(S);
    bool   logOutput = !ssGetOutputTimesOnly(S);

#ifdef DEBUG_TIMING
    rsimDisplayTimingData(S,
                          sizeof(struct SimStruct_tag),
                          sizeof(struct _ssMdlInfo));
#endif

    rtExtModeOneStep(ssGetRTWExtModeInfo(S),
                     ssGetNumSampleTimes(S),
                     (boolean_T*)&ssGetStopRequested(S));

    /* Setup the task times and sample hit flags for the discrete rates */
    rsimUpdateDiscreteTaskTimesAndSampleHits(S);
    if (ssGetErrorStatus(S) != NULL) return;

    /*
     * See if we are at an outputTime, and if so set logOutput to true and
     * increment the next output time index to point to the next entry in
     * the outputTimes array.
     */
    if ( ssGetNumOutputTimes(S) > 0 &&
         ssGetOutputTimesIndex(S) < ssGetNumOutputTimes(S) ) {
        time_T nextOutputTime = ssGetNextOutputTime(S);
        /* utAssert(currTime <= nextOutputTime); */
        if (currTime == nextOutputTime) {
            uint_T idx = ssGetOutputTimesIndex(S);
            ssSetOutputTimesIndex(S, idx+1);
            logOutput = 1; /* this is one of the specified output times */
        }
    }
    ssSetLogOutput(S, logOutput);

    MdlOutputs(0);

    rtExtModeSingleTaskUpload(S);

    if (ssGetLogOutput(S)) {
        (void)rt_UpdateTXYLogVars(ssGetRTWLogInfo(S), ssGetTPtr(S));
        if (ssGetErrorStatus(S) != NULL) return;
    }

    MdlUpdate(0);
    if (ssGetErrorStatus(S) != NULL) return;

    ssSetLogOutput(S, FALSE);
    ssSetTimeOfLastOutput(S, currTime);

    /* Update the timing engine and determine the solver stop time */
    rsimUpdateTimingEngineAndSolverStopTime(S);
    if (ssGetErrorStatus(S) != NULL) return;

    rtExtModeCheckEndTrigger();

    return;

} /* rsimOutputLogUpdate */


/* Function: rsimOneStep =======================================================
 *
 *      Perform one step of the model.
 *      Errors are set in the SimStruct's ErrorStatus, NULL means no errors.
 */
static void rsimOneStep(SimStruct *S)
{
    ssSetSimTimeStep(S, MAJOR_TIME_STEP);

    /* Check if the solver found ZC event.
     *  if yes, this is a major step at right post */
    if ( ssGetSolverFoundContZcEvents(S) ){ 
        ssSetSolverIsAtRightPostOfContZcEvent(S, true);
    }
    /* Clear the flag that blocks set in their major time step methods (output,
       update, etc.) to flag that they have changed their state. If this flag is
       true then we need to run model outputs again at the first minor step
       (i.e., at the same time as the last major step).*/
    ssClearContTimeOutputInconsistentWithStateAtMajorStep(S);
    ssClearBlkStateChange(S);

    rsimOutputLogUpdate(S);

    /* Done with the step. Set the right post flag to false
     * It is immaterial whether this step was really a right post step */
    ssSetSolverIsAtRightPostOfContZcEvent(S, false);

    if (ssGetErrorStatus(S) != NULL) return;

    /* Call rsimAdvanceSolver only if a stop has not been requested
       else assertions occur later in solver code due to zero stepsize (540586) */
    if (!ssGetStopRequested(S)) rsimAdvanceSolver(S);

} /* rsimOneStep */

#endif
#else

#if !defined(MULTITASKING)  /* SINGLETASKING */

/* Function: rsimOneStep =======================================================
 *
 *      Perform one step of the model.
 *      Errors are set in the SimStruct's ErrorStatus, NULL means no errors.
 */
static void rsimOneStep(SimStruct *S)
{
    real_T tnext;

    if (ssGetErrorStatus(S) != NULL) return;

    ssSetSimTimeStep(S, MAJOR_TIME_STEP);
    
    /* Clear the flag that blocks set in their major time step methods (output,
       update, etc.) to flag that they have changed their state. If this flag is
       true then we need to run model outputs again at the first minor step
       (i.e., at the same time as the last major step). */
    ssClearContTimeOutputInconsistentWithStateAtMajorStep(S);
    ssClearBlkStateChange(S);

    rtExtModeOneStep(ssGetRTWExtModeInfo(S),
                     ssGetNumSampleTimes(S),
                     (boolean_T *)&ssGetStopRequested(S));

#ifdef RT_MALLOC
    tnext = rt_GetNextSampleHit(S);
#else
    tnext = rt_GetNextSampleHit();
#endif
    ssSetSolverStopTime(S,tnext);

    MdlOutputs(0);

    rtExtModeSingleTaskUpload(S);

    (void)rt_UpdateTXYLogVars(ssGetRTWLogInfo(S), ssGetTPtr(S));
    if (ssGetErrorStatus(S) != NULL) return;

    MdlUpdate(0);
    if (ssGetErrorStatus(S) != NULL) return;
    rt_UpdateDiscreteTaskSampleHits(S);

    if (ssGetSampleTime(S,0) == CONTINUOUS_SAMPLE_TIME) { 
        
        int nx = ssGetNumContStates(S);
        if ( (ssGetBlkStateChange(S) ||
              ssGetContTimeOutputInconsistentWithStateAtMajorStep(S) )
             && (nx > 0) ) {
            ssSetSimTimeStep(S, MINOR_TIME_STEP);
            MdlOutputs(0);
            ssSetSimTimeStep(S, MAJOR_TIME_STEP);
        }
        
        rt_UpdateContinuousStates(S);
    }
    
    rtExtModeCheckEndTrigger();

} /* rsimOneStep */

#else /* MULTITASKING */

# if TID01EQ == 1
#  define FIRST_TID 1
# else
#  define FIRST_TID 0
# endif

/* Function: rsimOneStep =======================================================
 *
 *      Perform one step of the model in "simulated" multitasking mode
 *      Errors are set in the SimStruct's ErrorStatus, NULL means no errors.
 */
static void rsimOneStep(SimStruct* S)
{
    int_T  i;
    real_T tnext;
    int_T* sampleHit = ssGetSampleHitPtr(S);

    ssSetSimTimeStep(S, MAJOR_TIME_STEP);

    /* Clear the flag that blocks set in their major time step methods (output,
       update, etc.) to flag that they have changed their state. If this flag is
       true then we need to run model outputs again at the first minor step
       (i.e., at the same time as the last major step). */
    ssClearContTimeOutputInconsistentWithStateAtMajorStep(S);
    ssClearBlkStateChange(S);

    rtExtModeOneStep(ssGetRTWExtModeInfo(S),
                     ssGetNumSampleTimes(S),
                     (boolean_T*)&ssGetStopRequested(S));

    /* Update discrete events */
    tnext = rt_UpdateDiscreteEvents(S);
    ssSetSolverStopTime(S, tnext);

    /* Do output, log, update for the base rate */

    MdlOutputs(FIRST_TID);

    rtExtModeUploadCheckTrigger(ssGetNumSampleTimes(S));
    rtExtModeUpload(FIRST_TID, ssGetTaskTime(S, FIRST_TID));
    
    (void)rt_UpdateTXYLogVars(ssGetRTWLogInfo(S), ssGetTPtr(S));
    if (ssGetErrorStatus(S) != NULL) return;
    
    MdlUpdate(FIRST_TID);
    if (ssGetErrorStatus(S) != NULL) return;
    
    if (ssGetSampleTime(S,0) == CONTINUOUS_SAMPLE_TIME) {
        int nx = ssGetNumContStates(S);
        if ( (ssGetBlkStateChange(S) ||
              ssGetContTimeOutputInconsistentWithStateAtMajorStep(S) )
             && (nx > 0) ) {
            /* Do not log outputs during minor time steps */
            ssSetLogOutput(S, FALSE);
            ssSetTimeOfLastOutput(S,ssGetT(S));
            
            ssSetSimTimeStep(S, MINOR_TIME_STEP);
            MdlOutputs(FIRST_TID);
            ssSetSimTimeStep(S, MAJOR_TIME_STEP);
            ssSetLogOutput(S, TRUE);
        }
        rt_UpdateContinuousStates(S);
    } else {
        rt_UpdateDiscreteTaskTime(S,0);
    }

#if FIRST_TID == 1
    rt_UpdateDiscreteTaskTime(S,1);
#endif

    /* Do output and update for the remaining rates */

    for (i=FIRST_TID+1; i<NUMST; i++) {
        if ( !sampleHit[i] ) continue;

        MdlOutputs(i);
        rtExtModeUpload(i, ssGetTaskTime(S,i));
        MdlUpdate(i);
        rt_UpdateDiscreteTaskTime(S, i);
    }
    if (ssGetErrorStatus(S) != NULL) return;

    rtExtModeCheckEndTrigger();

} /* end rsimOneStep */

#endif /* MULTITASKING */

#endif /* RSIM_WITH_SL_SOLVER */


/* Function: WriteResultsToMatFile =============================================
 *
 *     This function is called from main for normal exit or from the
 *     signal handler in case of abnormal exit (^C, time out etc).
 */
void WriteResultsToMatFile(SimStruct* S)
{
    rt_StopDataLogging(gblMatLoggingFilename,ssGetRTWLogInfo(S));
}

/* Function: main ==============================================================
 *
 *      Execute model on a generic target such as a workstation.
 */
int_T main(int_T argc, char_T *argv[])
{
    SimStruct  *S                 = NULL;
    boolean_T  calledMdlStart     = FALSE;
    boolean_T  dataLoggingActive  = FALSE;
    boolean_T  initializedExtMode = FALSE;
    const char *result            = NULL;
    time_t     now;
    int matFileFormat             = 0;
    const char *errorPrefix       = NULL;


    
    /* No re-defining of data types allowed. */
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4127)
#endif
    
    if ((sizeof(real_T)   != 8) ||
        (sizeof(real32_T) != 4) ||
        (sizeof(int8_T)   != 1) ||
        (sizeof(uint8_T)  != 1) ||
        (sizeof(int16_T)  != 2) ||
        (sizeof(uint16_T) != 2) ||
        (sizeof(int32_T)  != 4) ||
        (sizeof(uint32_T) != 4) ||
        (sizeof(boolean_T)!= 1)) {
        ERROR_EXIT("Error: %s\n", "Re-defining data types such as REAL_T is not supported by RSIM");
    }

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    
    rt_InitInfAndNaN(sizeof(real_T));

    /* Parse arguments */
    result = ParseArgs(argc, argv);
    ERROR_EXIT("Error parsing input arguments: %s\n", result);

    /* Initialize the model */
    S = MODEL();
    ERROR_EXIT("Error during model registration: %s\n", ssGetErrorStatus(S));

    ssClearFirstInitCondCalled(S);
    /* Override StopTime */
    if (gblFinalTimeChanged) {
#ifdef RSIM_WITH_SL_SOLVER
        ssSetTFinal(S,gblFinalTime);
        if (gblFinalTime == rtInf) {
            printf("\n** May run forever. Model stop time set to infinity.\n");
        }
#else
        if (gblFinalTime == rtInf) {
            ssSetTFinal(S, RUN_FOREVER);
            printf("\n** May run forever. Model stop time set to infinity.\n");
        } else {
            ssSetTFinal(S,gblFinalTime);
        }
#endif
    }
    
    MdlInitializeSizes();
    MdlInitializeSampleTimes();

#ifdef RSIM_WITH_SL_SOLVER

    /* load solver options */
    rsimLoadSolverOpts(S);
    ERROR_EXIT("Error loading solver options: %s\n", ssGetErrorStatus(S));

# if defined(DEBUG_SOLVER)
  rsimEnableDebugOutput(sizeof(struct SimStruct_tag),sizeof(struct _ssMdlInfo));
# endif

    rsimInitializeEngine(S);
    ERROR_EXIT("Error initializing RSIM engine: %s\n", ssGetErrorStatus(S));

#else

    result = rt_InitTimingEngine(S);
    ERROR_EXIT("Error initializing sample time engine: %s\n", result);
    rt_CreateIntegrationData(S);
    ERROR_EXIT("Error creating integration data: %s\n", ssGetErrorStatus(S));

#endif /* RSIM_WITH_SL_SOLVER */

#ifdef UseMMIDataLogging
    rt_FillStateSigInfoFromMMI(ssGetRTWLogInfo(S),&ssGetErrorStatus(S));
#endif
    if (ssIsVariableStepSolver(S)) {
        (void)rt_StartDataLoggingWithStartTime(ssGetRTWLogInfo(S),
#ifdef RSIM_WITH_SL_SOLVER
                                               ssGetTStart(S),
#else
                                               0.0,
#endif /* RSIM_WITH_SL_SOLVER */
                                               ssGetTFinal(S),
                                               0.0,
                                               &ssGetErrorStatus(S));
    } else {
        (void)rt_StartDataLoggingWithStartTime(ssGetRTWLogInfo(S),
#ifdef RSIM_WITH_SL_SOLVER
                                               ssGetTStart(S),
#else
                                               0.0,
#endif /* RSIM_WITH_SL_SOLVER */
                                               ssGetTFinal(S),
                                               ssGetStepSize(S),
                                               &ssGetErrorStatus(S));
    }

    GOTO_EXIT_IF_ERROR("Error starting data logging: %s\n", result);
    dataLoggingActive = TRUE;

    rt_RapidReadMatFileAndUpdateParams(S);
    GOTO_EXIT_IF_ERROR("Error reading parameter data from mat-file: %s\n",
                       ssGetErrorStatus(S));

    /*******************
     * Start the model *
     *******************/
    rtExtModeCheckInit(ssGetNumSampleTimes(S));
    initializedExtMode = TRUE;
    rtExtModeWaitForStartPkt(ssGetRTWExtModeInfo(S),
                             ssGetNumSampleTimes(S),
                             (boolean_T *)&ssGetStopRequested(S));

    now = time(NULL);
    (void)printf("\n** Starting model '%s' @ %s", gblModelName, ctime(&now));
/*Enable logging in the MdlStart method*/
#ifdef RSIM_WITH_SL_SOLVER
    ssSetLogOutput(S,TRUE);
#endif

    /* Enable -i option to load inport data file */
    result = rt_RapidReadInportsMatFile(gblInportFileName, &matFileFormat, 0);
    if (result!= NULL) {
        ssSetErrorStatus(S,result);
        GOTO_EXIT_IF_ERROR("Error starting model: %s\n",  ssGetErrorStatus(S));
    }

    MdlStart();
#ifdef RSIM_WITH_SL_SOLVER
    ssSetLogOutput(S,FALSE);
#endif
    calledMdlStart = TRUE;
    GOTO_EXIT_IF_ERROR("Error starting model: %s\n", ssGetErrorStatus(S));

    result = rt_RapidCheckRemappings();
    ssSetErrorStatus(S,result);
    GOTO_EXIT_IF_ERROR("Error: %s\n", ssGetErrorStatus(S));

    /* Create solver data */
#ifdef RSIM_WITH_SL_SOLVER
    rsimCreateSolverData(S, gblSlvrJacPatternFileName);
    ERROR_EXIT("Error creating solver data: %s\n", ssGetErrorStatus(S));
#endif

    ssSetFirstInitCondCalled(S);

    /*********************
     * Execute the model *
     *********************/
        
#ifdef RSIM_WITH_SL_SOLVER
    /* Install Signal and Run time limit handlers */
    rsimInstallAllHandlers(S,WriteResultsToMatFile,gblTimeLimit);
    GOTO_EXIT_IF_ERROR("Error: %s\n", ssGetErrorStatus(S));
#endif
    
#ifdef RSIM_WITH_SL_SOLVER
    while ( ((ssGetTFinal(S)-ssGetT(S)) > (fabs(ssGetT(S))*DBL_EPSILON)) ) {
#else
    while ( (ssGetTFinal(S) == RUN_FOREVER) ||
            ((ssGetTFinal(S)-ssGetT(S)) > (fabs(ssGetT(S))*DBL_EPSILON)) ) {
#endif

        rtExtModePauseIfNeeded(ssGetRTWExtModeInfo(S),
                               ssGetNumSampleTimes(S),
                               (boolean_T *)&ssGetStopRequested(S));

        if (ssGetStopRequested(S)) break;

        rsimOneStep(S);
        if (ssGetErrorStatus(S)) break;
    }
    if (ssGetErrorStatus(S) == NULL && !ssGetStopRequested(S)) {
#ifdef RSIM_WITH_SL_SOLVER
        /* Do a major step at the final time */
#ifdef RSIM_WITH_SOLVER_MULTITASKING
        rsimOneStep(S);
#else
        rsimOutputLogUpdate(S);
#endif
#else
        rsimOneStep(S);
#endif
    }
    
  EXIT_POINT:
    /********************
     * Cleanup and exit *
     ********************/
#ifdef UseMMIDataLogging
    rt_CleanUpForStateLogWithMMI(ssGetRTWLogInfo(S));
#endif
    
    if (ssGetErrorStatus(S) != NULL) {
        if (errorPrefix) {
            (void)fprintf(stderr, errorPrefix, ssGetErrorStatus(S));
        } else {
            (void)fprintf(stderr, "Error: %s\n", ssGetErrorStatus(S));
        }
    }
#ifdef RSIM_WITH_SL_SOLVER
    rsimUninstallNonfatalHandlers();
#endif
    if (dataLoggingActive) WriteResultsToMatFile(S);

#ifdef RSIM_WITH_SL_SOLVER
    rsimTerminateEngine(S,1);
#endif

    if (calledMdlStart) {
        MdlTerminate();
    }
    if (initializedExtMode) {
        rtExtModeShutdown(ssGetNumSampleTimes(S));
    }
    rt_RapidFreeGbls(matFileFormat);
    return ssGetErrorStatus(S) ? EXIT_FAILURE : EXIT_SUCCESS;

} /* end main */

/* EOF */
