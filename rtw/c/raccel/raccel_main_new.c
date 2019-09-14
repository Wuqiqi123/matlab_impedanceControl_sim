/*
 * Abstract:
 *   Main program for the Rapid Acceleration target.
 *
 * 
 * Copyright 2007-2017 The MathWorks, Inc.
 */
#define IN_RACCEL_MAIN

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#define ERROR_EXIT(msg, cond)                   \
    if (cond) {                                 \
        (void)fprintf(stderr, msg, cond);       \
        return(EXIT_FAILURE);                   \
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
extern void MdlUpdate(int_T tid);
extern void MdlTerminate(void);
extern void MdlOutputsParameterSampleTime(int_T tid);

#define SL_MAX(A, B)   (((A) > (B)) ? (A) : (B))


/*=============*
 * Global data *
 *=============*/
int                gblVerboseFlag = 0;
boolean_T          gblExtModeStartPktReceived = false;

extern boolean_T   gbl_raccel_isMultitasking;
extern boolean_T   gbl_raccel_tid01eq;
extern boolean_T   gblExtModePortCheckOnly;
extern boolean_T   gblExtModeEnabled;
extern boolean_T   gblSetParamPktReceived;
extern const char* gblErrorStatus;
extern const char* gblInportFileName;
extern const char* gblSolverOptsFilename;
extern const char* gblSlvrJacPatternFileName;
extern const char* gblSFcnInfoFileName;
extern int         gblSolverOptsArrIndex;
extern int         gblTimeLimit;
extern int_T       gbl_raccel_NumST;
extern void*       gblISigstreamManager;

jmp_buf gblRapidAccelJmpBuf;
#if MODEL_HAS_DYNAMICALLY_LOADED_SFCNS
extern size_t      gblNumMexFileSFcns;
#endif

static ssBridgeExtModeCallbacks_T gblExtModeCallbacks = {
    rtExtModeCheckInit,
    rtExtModeWaitForStartPkt, 
    rtExtModeOneStep,
    rtExtModeUploadCheckTrigger,
    rtExtModeUpload,
    rtExtModeCheckEndTrigger,
    rtExtModePauseIfNeeded,
    rtExtModeShutdown,
    {NULL, NULL} /* assume */
};

#if defined(TGTCONN)
extern void TgtConnTerm();
extern void TgtConnPreStep(int_T tid);
extern void TgtConnPostStep(int_T tid);
#else
#define TgtConnTerm()                /* do nothing */
#endif

static struct {
    int matFileFormat;
} gblStatusFlags;

void raccel_setup_model( SimStruct *S );
void getRaccelExecutionInfo(SimStruct *S,  ssBridgeExecutionInfo_T* ex);

static ssBridgeSetupAndTerminationCallbacks_T gblSetupAndTerminationCallbacks = {
    getRaccelExecutionInfo
};

/* Function: WriteResultsToMatFile =============================================
 *
 *     This function is called from main for normal exit or from the
 *     signal handler in case of abnormal exit (^C, time out etc).
 */
void WriteResultsToMatFile(SimStruct* S)
{
    rt_StopDataLoggingImpl(gblMatLoggingFilename,ssGetRTWLogInfo(S),true);

}

/* Function types for replacement of tofile and fromfile filenames */
typedef void (*raccelToFileNameReplacementFcn)(const char *blockPath, char *fileName);
typedef void (*raccelFromFileNameReplacementFcn)(const char *blockPath, char *fileName);
raccelToFileNameReplacementFcn toFileNameReplacementFunction= NULL;
raccelFromFileNameReplacementFcn fromFileNameReplacementFunction= NULL;

#ifdef __LCC__
void sdiBindObserversAndStartStreamingEngine_wrapper(const char* modelName) 
{
    sdiBindObserversAndStartStreamingEngine(modelName);
}
#endif

/* Function: main ==============================================================
 *
 *      Execute model on a generic target such as a workstation.
 */
int_T main(int_T argc, char_T *argv[])
{
    volatile SimStruct  *S                 = NULL;

    const char *program           = NULL;

    const char *errorPrefix       = NULL;
    /* void *parforSchedulerInit     = NULL; */

    program = argv[0];

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
    gblErrorStatus = ParseArgs(argc, argv);
    ERROR_EXIT("Error parsing input arguments: %s\n", gblErrorStatus);

    /* Create ISigstreamManager instance */
    rtwISigstreamManagerCreateInstance(&gblISigstreamManager);

    /* Initialize SDI/HMI streaming engine */
    sdiInitializeForHostBasedTarget(gblExtModeEnabled, gblSimDataRepoFilename, gblMatSigLogSelectorFilename);

    /* 
     * Long-jump handling is required here because, for example, a dynamically loaded
     * s-function can long-jump out of its initialize-sizes method, which is called from 
     * raccel_register_model before the execution engine is created
     */
#ifndef __LCC__
    {
        int setJmpReturnValue =
            setjmp(gblRapidAccelJmpBuf);

        if (setJmpReturnValue != 0)
        {
            if(gblSimMetadataFileName && S)
            {
                ssWriteSimMetadata(                
                    (SimStruct*) S,
                    gblSimMetadataFileName
                    );
            }
            
            if (gblExtModeEnabled)
            {
                raccelForceExtModeShutdown();
            }

            return EXIT_FAILURE;
        }
    }
#endif

#if MODEL_HAS_DYNAMICALLY_LOADED_SFCNS        
    /* Dynamically loaded s-functions */
    raccelInitializeForMexSFcns();
    ERROR_EXIT("%s", gblErrorStatus);
#endif
    
    /* Initialize the model */
    S = raccel_register_model();
    ERROR_EXIT("Error during model registration: %s\n", ssGetErrorStatus(S));

    toFileNameReplacementFunction= rt_RAccelReplaceToFilename;
    fromFileNameReplacementFunction= rt_RAccelReplaceFromFilename;

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

    /* load additional options */
    rsimLoadOptionsFromMATFile((SimStruct*) S); 
    ERROR_EXIT("Error loading additional options: %s\n", ssGetErrorStatus(S));

    /* Run simulation */
    /* To add callbacks into generated/C code during simulation, see notes at
     * ExecutionEngine::run() implementation in slexec simbridge */
    ssRunSimulation((SimStruct*) S, &gblSetupAndTerminationCallbacks);

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


    /* Target connectivity termination */
    TgtConnTerm();
    
    /* Destroy ISigstreamManager instance */
    rtwISigstreamManagerDestroyInstance(gblISigstreamManager);

    /* Destroy LoggingInterval instance */
    rtwLoggingIntervalDestroyInstance(
        rtliGetLoggingInterval(ssGetRTWLogInfo(S)));

    rt_RapidReleaseDiagLoggerDB();

    rt_RapidFreeGbls(gblStatusFlags.matFileFormat);
    
    return ssGetErrorStatus(S) ? EXIT_FAILURE : EXIT_SUCCESS;

} /* end main */



void getRaccelExecutionInfo(SimStruct* S, ssBridgeExecutionInfo_T* exInfo)
{
    /* set the root simstruct */
    exInfo->simstruct_ = S; 

    /* Solver options */ 
    exInfo->solverOptions_.jacobianPatternFileName_ = gblSlvrJacPatternFileName;

    /* Simulation Options */
    exInfo->simulationOptions_.parameterFileName_ = gblSolverOptsFilename; 
    exInfo->simulationOptions_.parameterArrayIndex_ = gblSolverOptsArrIndex;
    exInfo->simulationOptions_.timeLimit_ = gblTimeLimit;
    exInfo->simulationOptions_.enableSLExecSSBridgeFeatureValue_ = ENABLE_SLEXEC_SSBRIDGE;
    exInfo->simulationOptions_.inportFileName_ = gblInportFileName;
    exInfo->simulationOptions_.matFileFormat_ = &(gblStatusFlags.matFileFormat);
    exInfo->simulationOptions_.simMetadataFilePath_ = gblSimMetadataFileName;
    
    /* model methods */
    exInfo->modelMethods_.start = &MdlStart;
    exInfo->modelMethods_.outputsParameterSampleTime = &MdlOutputsParameterSampleTime;
    exInfo->modelMethods_.terminate = &MdlTerminate;

    /* Runtime callbacks */
    exInfo->runtimeCallbacks_.externalModeCallbacks_ = gblExtModeEnabled ? &gblExtModeCallbacks : NULL;
    exInfo->runtimeCallbacks_.loggingFunction = &WriteResultsToMatFile;
    exInfo->runtimeCallbacks_.setupMMIStateLog = &raccel_setup_MMIStateLog;
    exInfo->runtimeCallbacks_.startDataLoggingWithStartTime = &rt_StartDataLoggingWithStartTime;
    exInfo->runtimeCallbacks_.rapidReadInportsMatFile = &rt_RapidReadInportsMatFile;
    exInfo->runtimeCallbacks_.rapidCheckRemappings = &rt_RapidCheckRemappings;
#ifdef __LCC__
    exInfo->runtimeCallbacks_.sdiBindObserversAndStartStreamingEngine = 
        &sdiBindObserversAndStartStreamingEngine_wrapper;
#else
    exInfo->runtimeCallbacks_.sdiBindObserversAndStartStreamingEngine = 
        &sdiBindObserversAndStartStreamingEngine;
#endif

    /* Runtime flags */
    exInfo->runtimeFlags_.parameterPacketReceived_ = &gblSetParamPktReceived;
    exInfo->runtimeFlags_.startPacketReceived_ = &gblExtModeStartPktReceived;

    /* Runtime objects */
#ifdef __LCC__
    exInfo->runtimeObjects_.longJumpBuffer_ = NULL;
#else
    exInfo->runtimeObjects_.longJumpBuffer_ = &gblRapidAccelJmpBuf;
#endif

    /* Parallel options */
#ifdef RACCEL_ENABLE_PARALLEL_EXECUTION
     exInfo->parallelExecution_.enabled_ = TRUE;
     exInfo->parallelExecution_.simulatorType_ = RACCEL_PARALLEL_SIMULATOR_TYPE;
     exInfo->parallelExecution_.options_.numberOfNodes = RACCEL_NUM_PARALLEL_NODES;
     exInfo->parallelExecution_.options_.numberOfThreads = RACCEL_PARALLEL_EXECUTION_NUM_THREADS;
     exInfo->parallelExecution_.options_.enableTiming = false;
     exInfo->parallelExecution_.options_.numberOfStepsToAnalyze = 0;
     exInfo->parallelExecution_.options_.timingOutputFilename = NULL;
     exInfo->parallelExecution_.options_.nodeExecutionModesFilename = NULL;
     exInfo->parallelExecution_.options_.parallelExecutionMode = PARALLEL_EXECUTION_OFF;
#else
     exInfo->parallelExecution_.enabled_ = FALSE;
#endif

     /* Target Connectivity */
#if defined(TGTCONN)
     gblExtModeCallbacks.targetConnectivityCallbacks_.TgtConnPreStep = TgtConnPreStep;
     gblExtModeCallbacks.targetConnectivityCallbacks_.TgtConnPostStep = TgtConnPostStep;
#endif

    return;
}



/* EOF */

/* LocalWords:  tofile fromfile RSIM rsim ISigstream slexec simbridge
 */
