/*
 * Abstract:
 *   Main program for the Rapid Acceleration target.
 *
 * Copyright 2007-2017 The MathWorks, Inc.
 */

#define IN_RACCEL_MAIN

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <setjmp.h>

#include "tmwtypes.h"
#include "simstruc.h"
#include "rt_logging.h"
#include "rtmodel.h"
#include "rt_nonfinite.h"
#include "raccel.h"
#include "raccel_sup.h"

#include "ext_work.h"

#include "sl_solver_rtw.h"
#include "slexec_parallel.h"

#include "sl_AsyncioQueue/AsyncioQueueCAPI.h"
#include "simlogCIntrf.h"

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
extern SimStruct *raccel_register_model(void);
extern void raccel_setup_MMIStateLog(SimStruct*);

extern void MdlInitializeSizes(void);
extern void MdlInitializeSampleTimes(void);
extern void MdlStart(void);
extern void MdlOutputs(int_T tid);
extern void MdlOutputsParameterSampleTime(int_T tid);
extern void MdlUpdate(int_T tid);
extern void MdlTerminate(void);

#if defined(TGTCONN)
    extern void TgtConnTerm();
    extern void TgtConnPreStep(int_T tid);
    extern void TgtConnPostStep(int_T tid);
#else
    #define TgtConnTerm()                /* do nothing */
    #define TgtConnPreStep(tid)          /* do nothing */
    #define TgtConnPostStep(tid)         /* do nothing */
#endif

#define SL_MAX(A, B)   (((A) > (B)) ? (A) : (B))


/*=============*
 * Global data *
 *=============*/
boolean_T          gblInstalledSigHandlers;
double             configsetInitialStepSize = 0.0;
int_T              gblVerboseFlag = 0;
boolean_T          gblExtModeStartPktReceived = false;

extern boolean_T gbl_raccel_isMultitasking;
extern boolean_T gbl_raccel_tid01eq;
extern boolean_T decoupledContinuousIntegration;
extern boolean_T optimalSolverResetCausedByZc;
extern boolean_T gblExtModePortCheckOnly;
extern boolean_T gblExtModeEnabled;
extern boolean_T   gblSetParamPktReceived;
extern const char* gblErrorStatus;
extern const char* gblInportFileName;
extern const char* gblSlvrJacPatternFileName;
extern const char* configSetSolver;
extern const char* gblSFcnInfoFileName;
extern double stiffnessThreshold;
extern double *specifiedTimesForRuntimeSolverSwitch;
extern int_T gbl_raccel_NumST;
extern int_T numStatesForStiffnessChecking;
extern int_T gblTimeLimit;
extern const int_T gblParameterTuningTid;
extern ParallelExecutionOptions parallelExecutionOptions;
extern unsigned int autoSolverStatusFlags; 
extern unsigned int numSpecifiedTimesForRuntimeSolverSwitch;
extern void* gblISigstreamManager;

jmp_buf gblRapidAccelJmpBuf;
#if MODEL_HAS_DYNAMICALLY_LOADED_SFCNS
extern size_t gblNumMexFileSFcns;
#endif

#  define rtExtModeSingleTaskUpload(S)                          \
   if (gblExtModeEnabled) {                                     \
        int stIdx;                                              \
        rtExtModeUploadCheckTrigger(ssGetNumSampleTimes(S));    \
        for (stIdx=0; stIdx<ssGetNumSampleTimes(S); stIdx++) {  \
            if (ssIsSampleHit(S, stIdx, 0 /*unused*/)) {        \
                rtExtModeUpload(stIdx,ssGetTaskTime(S,stIdx));  \
            }                                                   \
        }                                                       \
   }


/* Function: ParameterSampleTimeOutputs ========================================
 *   
 *      Wrapper function for MdlOutputsParameterSampleTime. Runs and logs
 *      Parameter-Sample-Time blocks if a set_param packet was received by
 *      external mode.
 */
void ParameterSampleTimeOutputs(SimStruct* S, boolean_T logOutput) 
{
    if (gblSetParamPktReceived) {
        ssSetLogOutput(S, TRUE);
        MdlOutputsParameterSampleTime(gblParameterTuningTid);
        ssSetLogOutput(S, logOutput);
    }
}


/* Function: rsimOutputLogUpdate ===============================================
 *
 */
static void rsimOutputLogUpdate(SimStruct* S)
{
    double currTime  = ssGetT(S);
    boolean_T   logOutput = !ssGetOutputTimesOnly(S);

#ifdef DEBUG_TIMING
    rsimDisplayTimingData(S,
                          sizeof(struct SimStruct_tag),
                          sizeof(struct _ssMdlInfo));
#endif

    if (gblExtModeEnabled) {
        rtExtModeOneStep(ssGetRTWExtModeInfo(S),
                         ssGetNumSampleTimes(S),
                         (boolean_T*)&ssGetStopRequested(S));
        if (ssGetStopRequested(S)) return;
    }

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
        if (currTime == nextOutputTime) {
            uint_T idx = ssGetOutputTimesIndex(S);
            ssSetOutputTimesIndex(S, idx+1);
            logOutput = 1; /* this is one of the specified output times */
        }
    }
    ssSetLogOutput(S, logOutput);

    TgtConnPreStep(0); /* Target connectivity Pre-MdlOutputs */

    ParameterSampleTimeOutputs(S, logOutput);

    MdlOutputs(0);

    TgtConnPostStep(0); /* Target connectivity Post-MdlOutputs */

    if (gblExtModeEnabled) {
        rtExtModeSingleTaskUpload(S);
    }

    if (ssGetLogOutput(S)) {
        (void)rt_UpdateTXXFYLogVars(
            ssGetRTWLogInfo(S), ssGetTPtr(S),           
            rtwTimeInLoggingInterval(
                rtliGetLoggingInterval(ssGetRTWLogInfo(S)),
                *ssGetTPtr(S)));
        if (ssGetErrorStatus(S) != NULL) return;
    }

    MdlUpdate(0);
    if (ssGetErrorStatus(S) != NULL) return;

    ssSetLogOutput(S, FALSE);
    ssSetTimeOfLastOutput(S, currTime);

    /* Update the timing engine and determine the solver stop time */
    rsimUpdateTimingEngineAndSolverStopTime(S);
    if (ssGetErrorStatus(S) != NULL) return;

    if (gblExtModeEnabled) {
        rtExtModeCheckEndTrigger();
    }
    return;

} /* rsimOutputLogUpdate */

bool rsimShouldStiffBeCalcForRunTimeSlvrSwitch( SimStruct *S ) 
{
    static const unsigned int SL_CS_STATUS_CONST_AUTO = 0x100U;
    static const unsigned int SL_CS_STATUS_RUNTIME_AUTO = 0x200U;
    static const unsigned int SL_CS_STATUS_AUTO_PRMC = 0x400U;
    static const unsigned int SL_CS_STATUS_AUTO_TIME = 0x800U;

    bool autoSolverRunTimeSwitching = ( ( autoSolverStatusFlags & SL_CS_STATUS_RUNTIME_AUTO ) > 0);
    /* If current solver is not the ode45/ode15s branch, we skip stiffness check.*/
    if( ( autoSolverStatusFlags & SL_CS_STATUS_CONST_AUTO ) > 0 ) return false; 

    if( ssGetTimeOfLastOutput(S) == ssGetTStart(S) ) return true;

    if( autoSolverRunTimeSwitching ) { 
        static unsigned int autoSolverNextTimeIdx = 0; 
        if( gblSetParamPktReceived && 
              ( ( autoSolverStatusFlags & SL_CS_STATUS_AUTO_PRMC )>0) ) { 
            return true; 
        } 

        /* Switching times is a time vector. Auto solver will calculate stiffness
        * at time >= the specified time and change solver if needed.
        */

        if(( autoSolverStatusFlags & SL_CS_STATUS_AUTO_TIME)>0 ) { 
          time_T currentTime = ssGetT( S ); 

          if( ( autoSolverNextTimeIdx < numSpecifiedTimesForRuntimeSolverSwitch ) &&
              ( currentTime >= specifiedTimesForRuntimeSolverSwitch[ autoSolverNextTimeIdx ] ) ) { 
             
              while( ( autoSolverNextTimeIdx < numSpecifiedTimesForRuntimeSolverSwitch ) && 
                  ( currentTime >= specifiedTimesForRuntimeSolverSwitch[ autoSolverNextTimeIdx ] ) ) {
                  ++autoSolverNextTimeIdx; 
              } 
              return true;  
          } 
        }
    } 

    return false; 
} 


/* Function: rsimOneStepST =====================================================
 *
 *      Perform one step of the model.
 *      Errors are set in the SimStruct's ErrorStatus, NULL means no errors.
 */
static void rsimOneStepST(SimStruct *S)
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
    ssClearBlkStateChange(S); /* for backwards compatibility only */
    
    rsimOutputLogUpdate(S);

    /* Done with the step. Set the right post flag to false
     * It is immaterial whether this step was really a right post step */
    ssSetSolverIsAtRightPostOfContZcEvent(S, false);

    if (ssGetErrorStatus(S) != NULL) return;

    /* Call to recreate solver for auto solver with stiffness. */
    if( rsimShouldStiffBeCalcForRunTimeSlvrSwitch( S ) ) {
            
            rsimFinalizeAutoSolverForStiffModelsIfNeeded( S, 
                                            gblSlvrJacPatternFileName,
                                            numStatesForStiffnessChecking,
                                            stiffnessThreshold,
                                            configsetInitialStepSize );

            if (ssGetErrorStatus(S) != NULL) return;
    }

    /* Call rsimAdvanceSolver only if no stop was requested (540586) */
    if (!ssGetStopRequested(S)) rsimAdvanceSolver(S);

} /* rsimOneStepST */


/* Function: rsimOneStepMT =====================================================
 *
 *      Perform one step of the model.
 *      Errors are set in the SimStruct's ErrorStatus, NULL means no errors.
 */
static void rsimOneStepMT(SimStruct *S)
{
    int_T  i;
    const int_T FIRST_TID = gbl_raccel_tid01eq ? 1 : 0;
    int_T* sampleHit = ssGetSampleHitPtr(S);

    ssSetSimTimeStep(S, MAJOR_TIME_STEP);
    
    /* Clear the flag that blocks set in their major time step methods (output,
       update, etc.) to flag that they have changed their state. If this flag is
       true then we need to run model outputs again at the first minor step
       (i.e., at the same time as the last major step).*/    
    ssClearContTimeOutputInconsistentWithStateAtMajorStep(S);
    ssClearBlkStateChange(S); /* for backwards compatibility only */

#ifdef DEBUG_TIMING
    rsimDisplayTimingData(S,
                          sizeof(struct SimStruct_tag),
                          sizeof(struct _ssMdlInfo));
#endif
    if (gblExtModeEnabled) {
        rtExtModeOneStep(ssGetRTWExtModeInfo(S),
                         ssGetNumSampleTimes(S),
                         (boolean_T*)&ssGetStopRequested(S));
        if (ssGetStopRequested(S)) return;
    }
    /* Setup the task times and sample hit flags for the discrete rates */
    rsimUpdateDiscreteTaskTimesAndSampleHits(S);
    if (ssGetErrorStatus(S) != NULL) return;

    ssSetLogOutput(S,TRUE);

    TgtConnPreStep(FIRST_TID); /* Target connectivity Pre-MdlOutputs */

    ParameterSampleTimeOutputs(S, TRUE);

    /* Do output, log, update for the base rate */
    MdlOutputs(FIRST_TID);

    TgtConnPostStep(FIRST_TID); /* Target connectivity Post-MdlOutputs */

    if (gblExtModeEnabled) {
        rtExtModeUploadCheckTrigger(ssGetNumSampleTimes(S));
        rtExtModeUpload(FIRST_TID, ssGetTaskTime(S, FIRST_TID));
    }

    if (ssGetLogOutput(S)) {
        (void)rt_UpdateTXXFYLogVars(
            ssGetRTWLogInfo(S), ssGetTPtr(S), 
            rtwTimeInLoggingInterval(
                rtliGetLoggingInterval(ssGetRTWLogInfo(S)),
                *ssGetTPtr(S)));
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
    for (i = FIRST_TID+1; i < gbl_raccel_NumST; i++) {
        if ( !sampleHit[i] ) continue;
        TgtConnPreStep(i); /* Target connectivity Pre-MdlOutputs */
        MdlOutputs(i);
        TgtConnPostStep(i); /* Target connectivity Post-MdlOutputs */
        rtExtModeUpload(i, ssGetTaskTime(S,i));
        MdlUpdate(i);
    }
    if (ssGetErrorStatus(S) != NULL) return;
    ssSetLogOutput(S, FALSE);

    if (gblExtModeEnabled) {
        rtExtModeCheckEndTrigger();
    }

    /* Update the timing engine and determine the solver stop time */
    rsimUpdateTimingEngineAndSolverStopTime(S);
    if (ssGetErrorStatus(S) != NULL) return;

} /* rsimOneStepMT */

/* Function: WriteResultsToMatFile =============================================
 *
 *     This function is called from main for normal exit or from the
 *     signal handler in case of abnormal exit (^C, time out etc).
 */
void WriteResultsToMatFile(SimStruct* S)
{
    rt_StopDataLoggingImpl(gblMatLoggingFilename,ssGetRTWLogInfo(S),true);
}

/* Function: main ==============================================================
 *
 *      Execute model on a generic target such as a workstation.
 */
int_T main(int_T argc, char_T *argv[])
{
    volatile SimStruct  *S        = NULL;
    boolean_T  calledMdlStart     = FALSE;
    boolean_T  dataLoggingActive  = FALSE;
    boolean_T  initializedExtMode = FALSE;
    const char *result            = NULL;
    int matFileFormat             = 0;
    const char *errorPrefix       = NULL;
    boolean_T logOutput           = FALSE;
    time_t     now;

    gblInstalledSigHandlers = FALSE;

    /* No re-defining of data types allowed. 
     * The condition in the following if statement evaluates
     * to a constant and so can cause a warning in Windows.
     */
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
    gblErrorStatus = ParseArgs(argc, argv);
    ERROR_EXIT("Error parsing input arguments: %s\n", gblErrorStatus);

    /* Create ISigstreamManager instance */
    rtwISigstreamManagerCreateInstance(&gblISigstreamManager);

    /* Error handling: setup jmp buffer */
    {
        int setJmpReturnValue =
            setjmp(gblRapidAccelJmpBuf);

        if (setJmpReturnValue != 0)
        {
            if (gblExtModeEnabled)
            {
                raccelForceExtModeShutdown();
            }

            return EXIT_FAILURE;
        }
    }

#if MODEL_HAS_DYNAMICALLY_LOADED_SFCNS
    /* Dynamically loaded s-functions */
    raccelInitializeForMexSFcns();
    ERROR_EXIT("%s", gblErrorStatus);

#endif
    
    /* Initialize the model */
    S = raccel_register_model();
    ERROR_EXIT("Error during model registration: %s\n", ssGetErrorStatus(S));

    ssClearFirstInitCondCalled(S);
    /* Override StopTime */
    if (gblFinalTimeChanged) ssSetTFinal(S,gblFinalTime);

    MdlInitializeSizes();
    MdlInitializeSampleTimes();

    /* We don't have GOTO_EXIT_IF_ERROR here as engine is not initialized 
       via rsimInitializeEngine */
    rt_RapidReadMatFileAndUpdateParams((SimStruct*) S);
    ERROR_EXIT("Error reading parameter data from mat-file: %s\n",
              ssGetErrorStatus(S));

    /* load solver options */
    rsimLoadSolverOpts((SimStruct*) S);
    ERROR_EXIT("Error loading solver options: %s\n", ssGetErrorStatus(S));

    configsetInitialStepSize = ssGetStepSize( S );

# if defined(DEBUG_SOLVER)
    rsimEnableDebugOutput(sizeof(struct SimStruct_tag),
                          sizeof(struct _ssMdlInfo));
# endif

#ifdef RACCEL_ENABLE_PARALLEL_EXECUTION
    parallelExecutionOptions.numberOfNodes = RACCEL_NUM_PARALLEL_NODES;
    parallelExecutionOptions.numberOfThreads = 
        RACCEL_PARALLEL_EXECUTION_NUM_THREADS;
    initialize_parallel_execution(parallelExecutionOptions);
#endif

    rsimInitializeEngine((SimStruct*) S);
    ERROR_EXIT("Error initializing RSIM engine: %s\n", ssGetErrorStatus(S));

    /* initialize external model */
    if (gblExtModeEnabled) {
        rtExtModeCheckInit(ssGetNumSampleTimes(S));
        initializedExtMode = TRUE;       
    }

    /* Initialize SDI/HMI streaming engine */
    sdiInitializeForHostBasedTarget(gblExtModeEnabled, gblSimDataRepoFilename, gblMatSigLogSelectorFilename);

    raccel_setup_MMIStateLog((SimStruct*) S);

    if (gblExtModeEnabled) {
        /* If -w flag is specified wait here for connect signal from host */
        rtExtModeWaitForStartPkt(ssGetRTWExtModeInfo(S),
                                 ssGetNumSampleTimes(S),
                                 (boolean_T *)&ssGetStopRequested(S));
        if (ssGetStopRequested(S)) goto EXIT_POINT;

        gblExtModeStartPktReceived = true;
    }

    if (ssIsVariableStepSolver(S)) {
        (void)rt_StartDataLoggingWithStartTime(ssGetRTWLogInfo(S),
                                               ssGetTStart(S),
                                               ssGetTFinal(S),
                                               0.0,
                                               &ssGetErrorStatus(S));
    } else {
        (void)rt_StartDataLoggingWithStartTime(ssGetRTWLogInfo(S),
                                               ssGetTStart(S),
                                               ssGetTFinal(S),
                                               ssGetStepSize(S),
                                               &ssGetErrorStatus(S));
    }
    GOTO_EXIT_IF_ERROR("Error starting data logging: %s\n", 
                       ssGetErrorStatus(S));
    dataLoggingActive = TRUE;

    /* Start the model */
    now = time(NULL);
    if(gblVerboseFlag) {
        (void)printf("\n** Starting model @ %s", ctime(&now));
        }

    /* Enable logging in the MdlStart method */
    ssSetLogOutput(S,TRUE);

    /* Enable -i option to load inport data file */
    result = rt_RapidReadInportsMatFile(gblInportFileName, &matFileFormat, 1);
    if (result!= NULL) {
        ssSetErrorStatus(S,result);
        GOTO_EXIT_IF_ERROR("Error starting model: %s\n",  ssGetErrorStatus(S));
    }

    MdlStart();
    ssSetLogOutput(S,FALSE);

    calledMdlStart = TRUE;
    GOTO_EXIT_IF_ERROR("Error starting model: %s\n", ssGetErrorStatus(S));

    result = rt_RapidCheckRemappings();
    ssSetErrorStatus(S,result);
    GOTO_EXIT_IF_ERROR("Error: %s\n", ssGetErrorStatus(S));

    /* Create solver data */
    rsimCreateSolverData((SimStruct*) S, gblSlvrJacPatternFileName);
    GOTO_EXIT_IF_ERROR("Error creating solver data: %s\n", ssGetErrorStatus(S));

    rsimSetDecoupledContinuousIntegration((SimStruct*) S, decoupledContinuousIntegration);
    rsimSetOptimalSolverResetCausedByZc((SimStruct*) S, optimalSolverResetCausedByZc);

    ssSetFirstInitCondCalled(S);

    /* Create async observers */
    #if !defined(_WIN32) || !defined(__LCC__) || defined(__LCC64__)
        sdiBindObserversAndStartStreamingEngine(S->modelName);
    #endif
    
    /*********************
     * Execute the model *
     *********************/

    /* Install Signal and Run time limit handlers */
    
    rsimInstallAllHandlers((SimStruct*) S,WriteResultsToMatFile,gblTimeLimit);        
    
    gblInstalledSigHandlers = TRUE;
    GOTO_EXIT_IF_ERROR("Error: %s\n", ssGetErrorStatus(S));
    
    /* reset gblSetParamPktReceived in case it changed during initialization */
    gblSetParamPktReceived = false;

    /* run parameter-sample-time blocks */
    logOutput =  !ssGetOutputTimesOnly(S);
    ssSetLogOutput(S, TRUE);
    MdlOutputsParameterSampleTime(gblParameterTuningTid);
    ssSetLogOutput(S, logOutput);

    while ( ((ssGetTFinal(S)-ssGetT(S)) > (fabs(ssGetT(S))*DBL_EPSILON)) ) {

#ifdef RACCEL_ENABLE_PARALLEL_EXECUTION
        analyze_parallel_execution();
#endif

        if (gblExtModeEnabled) {
            rtExtModePauseIfNeeded(ssGetRTWExtModeInfo(S),
                                   ssGetNumSampleTimes(S),
                                   (boolean_T *)&ssGetStopRequested(S));
        }
        if (ssGetStopRequested(S)) break;

        if (gbl_raccel_isMultitasking) {
            rsimOneStepMT((SimStruct*) S);
        } else {
            rsimOneStepST((SimStruct*) S);
        }
        /* Clear flag for run time param change */
        gblSetParamPktReceived = FALSE; 
        if (ssGetErrorStatus(S)) break;
    }


    if (ssGetErrorStatus(S) == NULL && !ssGetStopRequested(S)) {
        /* Do a major step at the final time */
        if (gbl_raccel_isMultitasking) {
            rsimOneStepMT((SimStruct*) S);
        } else { 
            rsimOutputLogUpdate((SimStruct*) S);
        }
    }

  EXIT_POINT:
    /********************
     * Cleanup and exit *
     ********************/
    if (ssGetErrorStatus(S) != NULL) {
        if (errorPrefix) {
            (void)fprintf(stderr, errorPrefix, ssGetErrorStatus(S));
        } else {
            (void)fprintf(stderr, "Error: %s\n", ssGetErrorStatus(S));
        }
    }
    if (gblInstalledSigHandlers) {
        rsimUninstallNonfatalHandlers();
        gblInstalledSigHandlers = FALSE;
    }
    if (dataLoggingActive){
        WriteResultsToMatFile((SimStruct*) S);
    }
    rt_WriteSimMetadata(gblSimMetadataFileName, (SimStruct*) S);
 
    rsimTerminateEngine((SimStruct*) S,0);

    if (calledMdlStart) {
        MdlTerminate();
    }

    if (initializedExtMode) {
        rtExtModeShutdown(ssGetNumSampleTimes(S));
    }
    
    /* Target connectivity termination */
    TgtConnTerm();
    
    /* Destroy ISigstreamManager instance */
    rtwISigstreamManagerDestroyInstance(gblISigstreamManager);

    /* Destroy LoggingInterval instance */
    rtwLoggingIntervalDestroyInstance(
        rtliGetLoggingInterval(ssGetRTWLogInfo(S)));

    rt_RapidReleaseDiagLoggerDB();

    rt_RapidFreeGbls(matFileFormat);
    free( specifiedTimesForRuntimeSolverSwitch );   
    return ssGetErrorStatus(S) ? EXIT_FAILURE : EXIT_SUCCESS;

} /* end main */

/* EOF */

/* LocalWords:  rsim ut curr ZC Struct's ISigstream gbl async
 */
