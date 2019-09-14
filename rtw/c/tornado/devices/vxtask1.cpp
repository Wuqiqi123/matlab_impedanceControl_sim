/*
 * $Revision $
 *
 * Copyright 2003-2016 The MathWorks, Inc.
 */

#define S_FUNCTION_NAME vxtask1
#define S_FUNCTION_LEVEL 2

#define TASK_NAME       (ssGetSFcnParam(S,0))
#define PRIORITY        (ssGetSFcnParam(S,1))
#define STACK_SIZE      (ssGetSFcnParam(S,2))
#define PARENT_MANAGET  (ssGetSFcnParam(S,3))
#define TICK_RES        (ssGetSFcnParam(S,4))
#define TICK_LENGTH     (ssGetSFcnParam(S,5))

#include "simstruc.h"

#ifndef MATLAB_MEX_FILE
/* Since we have a target file for this S-function, declare an error here
 * so that, if for some reason this file is being used (instead of the
 * target file) for code generation, we can trap this problem at compile
 * time. */
#  error This_file_can_be_used_only_during_simulation_inside_Simulink
#endif

/*====================*
 * S-function methods *
 *====================*/

#define MDL_CHECK_PARAMETERS
static void mdlCheckParameters(SimStruct *S)
{
    int_T priority    = (int_T) (*(mxGetPr(PRIORITY)));
    int_T stackSize   = (int_T) (*(mxGetPr(STACK_SIZE)));
    
    /* Check priority */
    if( (priority < 0) | (priority > 255) ) {
        ssSetErrorStatus(S, "Priority must be 0-255.");
	return;
    }
    /* Check stack size */
    if( stackSize <= 0 ) {
        ssSetErrorStatus(S, "Stack size must be >= 0");
	return;
    }
    /* Check tick resolution */
    if (!mxIsDouble(TICK_RES)) {
        ssSetErrorStatus(S,"Must specify timer tick size as type 'double'.");
        return;
    } else if (((int_T) (mxGetNumberOfElements(TICK_RES))) != 1) {
        ssSetErrorStatus(S,"Must specify one value for tick size.");
        return;
    }
}

static void mdlInitializeSizes(SimStruct *S)
{
    int_T priority;

    ssSetNumSFcnParams(       S, 6);
    if (ssGetNumSFcnParams(S) == ssGetSFcnParamsCount(S)) {
        mdlCheckParameters(S);
        if (ssGetErrorStatus(S) != NULL) {
            return;
        }
    } else {
        return; /* Simulink will report a parameter mismatch error */
    }
    ssSetSFcnParamNotTunable( S, 0);
    ssSetSFcnParamNotTunable( S, 1);
    ssSetSFcnParamNotTunable( S, 2);
    ssSetSFcnParamNotTunable( S, 3);
    ssSetSFcnParamNotTunable( S, 4);
    ssSetSFcnParamNotTunable( S, 5);
    ssSetNumInputPorts(       S, 0);
    ssSetNumOutputPorts(      S, 1);
    ssSetOutputPortWidth(     S, 0, 1);
    /* All output elements are function-call, so we can set the data type of the
     * entire port to be function-call. */
    ssSetOutputPortDataType(  S, 0, SS_FCN_CALL);
    ssSetNumIWork(            S, 1);
    ssSetNumRWork(            S, 0);
    ssSetNumPWork(            S, 1);
    ssSetNumSampleTimes(      S, 1);
    ssSetNumContStates(       S, 0);
    ssSetNumDiscStates(       S, 0);
    ssSetNumModes(            S, 0);
    ssSetNumNonsampledZCs(    S, 0);

    /* Block has not internal states to save/restore in Simulation. PWork and IWork
     * is used for codegen only. No need to save/restore as SimState.
     * Need change SimStateCompliance setting
     * if new state is added */
    ssSetSimStateCompliance(S,HAS_NO_SIM_STATE);

    if ((int_T) (*(mxGetPr(PARENT_MANAGET)))) {
        /* Maintain this async rates timer by using callers timer */
        ssSetTimeSource(S, SS_TIMESOURCE_CALLER);
    } else {
        /* This async rate maintains it's own timer. */
        ssSetTimeSource(S, SS_TIMESOURCE_SELF);
        ssSetAsyncTimerAttributes(S,mxGetPr(TICK_RES)[0]);

        switch((int_T)(mxGetPr(TICK_LENGTH)[0])) {
          case 1: 
            ssSetAsyncTimerDataType(S, SS_UINT32);
            break;
          case 2:
            ssSetAsyncTimerDataType(S, SS_UINT16);
            break;
          case 3:
            ssSetAsyncTimerDataType(S, SS_UINT8);
            break;
          case 4:
            /* Automatically set by simulink */
            ssSetAsyncTimerDataType(S, -1);
            break;
          default:
            ssSetErrorStatus(S, "Invalid Timer size setting");
            return;
        }
    }

    /* An alternate approach to manage time for this asynchronous task is to
     * utilize the models base rate counter (provided the resolution of the base
     * rate is sufficient). The base rate counter will be double bufferred to
     * ensured data integrity if this block sets a priority. You can explicitly
     * set 'BASE_RATE' using ssSetTimeSource(S, BASE_RATE); which is also the
     * default value. */

    priority  = (int_T) (*(mxGetPr(PRIORITY)));
    ssSetAsyncTaskPriorities(S,1,&priority);

    ssSetOptions(S, (SS_OPTION_EXCEPTION_FREE_CODE |
                     SS_OPTION_ASYNCHRONOUS |
                                  SS_OPTION_DISALLOW_CONSTANT_SAMPLE_TIME));
}

static void mdlInitializeSampleTimes(SimStruct *S)
{
    ssSetSampleTime(S, 0, INHERITED_SAMPLE_TIME);
    ssSetOffsetTime(S, 0, 0.0);
    ssSetCallSystemOutput(S, 0);
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
    ssCallSystemWithTid(S, 0, tid);
}

static void mdlTerminate(SimStruct *S) {}

#define MDL_RTW
static void mdlRTW(SimStruct *S)
{
 
    char *str = mxArrayToString(TASK_NAME);

    /* Write out the parameters for this block.*/
    if (!ssWriteRTWParamSettings(S, 4, 
                                 SSWRITE_VALUE_QSTR,"TaskName",str,
                                 SSWRITE_VALUE_NUM,"Priority",
                                 (real_T) (*(mxGetPr(PRIORITY))),
                                 SSWRITE_VALUE_NUM,"StackSize",
                                 (real_T) (*(mxGetPr(STACK_SIZE))),
                                 SSWRITE_VALUE_NUM,"ParentManageT",
                                 (real_T) (*(mxGetPr(PARENT_MANAGET)))
                                 )) {
        mxFree(str);
        return; /* An error occurred which will be reported by SL */
    }
    mxFree(str);
    
    /* Write out names for the IWork vectors.*/
    if (!ssWriteRTWWorkVect(S, "IWork", 1, "TaskID", 1)) {
        return; /* An error occurred which will be reported by SL */
    }
    
    /* Write out names for the PWork vectors.*/
    if (!ssWriteRTWWorkVect(S, "PWork", 1, "SemID", 1)) {
        return; /* An error occurred which will be reported by SL */
    }
}

/*=============================*
 * Required S-function trailer *
 *=============================*/

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif


/* EOF: vxtask1.c*/
