/* Copyright 1990-2017 The MathWorks, Inc. */

/******************************************************************************
 *                                                                            *
 * File    : RTWCG_FMU1_target.c                                              *
 * Abstract:                                                                  *
 *      Wrapper functions to dynamic call libraries of FMU 1.0                *
 *      This File define functions called by CGIR code                        *
 *      Also handle errors, and logger                                        *
 *                                                                            *
 * Author : Brayden Xia, Nov/2017                                             *
 *                                                                            *
 * TODO:                                                                      *
 *      Support String Parameters                                             *
 *      test all parameters, test err conditions                              *
 *      Support FMU ME                                                        *
 *      *Model reference                                                      *
 ******************************************************************************/

#include "RTWCG_FMU1_target.h"
#define FMU1_MESSAGE_SIZE 1024

/*
  will do nothing but return a error status
  Whenever a default function is called, it means a functions is called without successful load,
  return a fmiError;
*/
static fmiStatus defaultfcn1(fmiComponent c, ...){
    if(c != NULL){ return fmiFatal;}
    return fmiError;
}

static fmiBoolean CheckStatus(struct FMU1_CS_RTWCG * fmustruct, fmiStatus status, fmiString fcnName) {

    SimStruct* ss = fmustruct->ssPtr;
    if(status == fmiError || status == fmiFatal){
        void * diagnostic = CreateDiagnosticAsVoidPtr("FMUBlock:FMU:FMUSimFunctionErrorDebugToDisplayOn", 2,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fcnName,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fmustruct->fmuname);
        fmustruct->FMUErrorStatus = status;
        rt_ssSet_slErrMsg(ss, diagnostic);
        ssSetStopRequested(ss, 1);
    }else if(status == fmiPending){
        void * diagnostic = CreateDiagnosticAsVoidPtr("FMUBlock:FMU:FMUSimPendingNotAllowed", 2,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fcnName,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fmustruct->fmuname);
        fmustruct->FMUErrorStatus = status;
        rt_ssSet_slErrMsg(ss, diagnostic);
        ssSetStopRequested(ss, 1);
    }
    
    if(status == fmiOK)
        return fmiTrue;
    else
        return fmiFalse;
}
/*This function is required by FMI standard
  FMU loggin is currently not enabled, so it will not be called
  reportasInfo API is currently not avaiable for Rapid accelerator
*/
void fmu1Logger(fmiComponent c, fmiString instanceName, fmiStatus status,
                fmiString category, fmiString message, ...) {
    (void) c, instanceName, status, category, message;
    /*
    int prefixLength;
    va_list args;
    void * diagnostic;
    
    static const char* strStatus[] = {
        "fmiOK", "fmiWarning", "fmiDiscard", "fmiError", "fmiFatal", "fmiPending" };
    static char translatedMsg[FMU1_MESSAGE_SIZE];
    static char temp[FMU1_MESSAGE_SIZE];
    
    prefixLength = snprintf(translatedMsg, FMU1_MESSAGE_SIZE, "Log from FMU: [category:%s, status:%s] ",
                            strStatus[status], category); 
    va_start (args, message);
    vsnprintf(temp, FMU1_MESSAGE_SIZE, message, args);
    va_end(args);
    
    strncat(translatedMsg, temp, FMU1_MESSAGE_SIZE-prefixLength - 1);
    diagnostic = CreateDiagnosticAsVoidPtr("SL_SERVICES:utils:PRINTFWRAPPER", 1,
                                           CODEGEN_SUPPORT_ARG_STRING_TYPE, translatedMsg);
    rt_ssReportDiagnosticAsWarning(NULL, diagnostic);*/
}

static _fmi_default_fcn_type LoadFMUFcn(struct FMU1_CS_RTWCG * fmustruct, const char * fcnName)
{
    _fmi_default_fcn_type fcn = NULL;

    static char fullFcnName[FULL_FCN_NAME_MAX_LEN];
    memset(fullFcnName, 0, FULL_FCN_NAME_MAX_LEN);
    strncpy(fullFcnName, fmustruct->fmuname, sizeof(fullFcnName));
    strncat(fullFcnName, "_", 1);
    strncat(fullFcnName, fcnName, sizeof(fullFcnName)-sizeof(fcnName)-1);
        
#ifdef _WIN32
    fcn = (_fmi_default_fcn_type)LOAD_FUNCTION(fmustruct->Handle, fullFcnName);
#else
    *((void **)(&fcn)) = LOAD_FUNCTION(fmustruct->Handle, fullFcnName);
#endif

    if (fcn == NULL) {
        void * diagnostic = CreateDiagnosticAsVoidPtr("FMUBlock:FMU:FMULoadLibFunctionError", 2,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fcnName,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fmustruct->fmuname);
        fmustruct->FMUErrorStatus = fmiWarning;
        /*A loading failure will cause a warning, ANY CALL to defualt Fcn will result in an Error and Stop*/
        rt_ssReportDiagnosticAsWarning(fmustruct->ssPtr, diagnostic);
        fcn = (_fmi_default_fcn_type) defaultfcn1; 
    }
    return fcn;
}
void* FMU1_fmuInitializeCS( const char * lib, fmiString instanceName, fmiString fmuGUID, fmiString fmuLocation, void* ssPtr){
    return FMU1_fmuInitialize(lib, instanceName, fmuGUID, fmuLocation, fmiCS, ssPtr);
}
void* FMU1_fmuInitializeME( const char * lib, fmiString instanceName, fmiString fmuLocation, fmiString fmuGUID, void* ssPtr){
    return FMU1_fmuInitialize(lib, instanceName, fmuGUID, fmuLocation, fmiME, ssPtr);
}
/* fmuPrefix is instanceName */
void* FMU1_fmuInitialize( const char * lib, fmiString instanceName, fmiString fmuGUID, fmiString fmuLocation, fmiTypes fmuType, void* ssPtr) {

    struct FMU1_CS_RTWCG * fmustruct;

    /*parameters to instantiateSlave()*/
    fmiCallbackFunctions callbacks;
    fmiReal timeout;           /* wait period in milli seconds, 0 for unlimited wait period"*/
    fmiBoolean visible;        /* no simulator user interface*/
    fmiBoolean interactive;    /* simulation run without user interaction*/
    const char* mimeType = "application/x-fmu-sharedlibrary";

    fmustruct = (struct FMU1_CS_RTWCG *)calloc(1, sizeof(struct FMU1_CS_RTWCG));
    fmustruct->ssPtr = (SimStruct*) ssPtr;
    fmustruct->fmuname = (char *)instanceName;
    fmustruct->dllfile = (char *)lib;
    fmustruct->FMUErrorStatus = fmiOK;
    
    if (strlen(instanceName)+ FCN_NAME_MAX_LEN + 1 >= FULL_FCN_NAME_MAX_LEN){
        /*FMU name is longer than 200+, rarely happens*/
        void * diagnostic = CreateDiagnosticAsVoidPtr("SL_SERVICES:utils:PRINTFWRAPPER", 1,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, "FMU Name is too long.");
        fmustruct->FMUErrorStatus = fmiFatal;
        rt_ssReportDiagnosticAsWarning(fmustruct->ssPtr, diagnostic);
        ssSetStopRequested(fmustruct->ssPtr, 1);
        return NULL;
    }

    fmustruct->Handle = LOAD_LIBRARY(lib);
    if (NULL == fmustruct->Handle) {
        void * diagnostic = CreateDiagnosticAsVoidPtr("FMUBlock:FMU:FMULoadLibraryError", 2,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fmustruct->dllfile,
                                                      CODEGEN_SUPPORT_ARG_STRING_TYPE, fmustruct->fmuname);
        /*loading lib failure will cause an Fatal and Stop*/
        fmustruct->FMUErrorStatus = fmiFatal;
        rt_ssReportDiagnosticAsWarning(fmustruct->ssPtr, diagnostic);
        ssSetStopRequested(fmustruct->ssPtr, 1);
        CLOSE_LIBRARY(fmustruct->Handle);
        return NULL;
    }

    fmustruct->getTypesPlatform   = (_fmiGetTypesPlatform)               LoadFMUFcn(fmustruct, "fmiGetTypesPlatform"); 
    fmustruct->instantiateSlave   = (_fmiInstantiateSlave)               LoadFMUFcn(fmustruct, "fmiInstantiateSlave");
    fmustruct->initializeSlave    = (_fmiInitializeSlave)                LoadFMUFcn(fmustruct, "fmiInitializeSlave");    
    fmustruct->terminateSlave     = (_fmiTerminateSlave)                 LoadFMUFcn(fmustruct, "fmiTerminateSlave");
    fmustruct->resetSlave         = (_fmiResetSlave)                     LoadFMUFcn(fmustruct, "fmiResetSlave");
    fmustruct->freeSlaveInstance  = (_fmiFreeSlaveInstance)              LoadFMUFcn(fmustruct, "fmiFreeSlaveInstance");
    fmustruct->setRealInputDerivatives = (_fmiSetRealInputDerivatives)   LoadFMUFcn(fmustruct, "fmiSetRealInputDerivatives");
    fmustruct->getRealOutputDerivatives = (_fmiGetRealOutputDerivatives) LoadFMUFcn(fmustruct, "fmiGetRealOutputDerivatives");
    fmustruct->cancelStep              = (_fmiCancelStep)                LoadFMUFcn(fmustruct, "fmiCancelStep");
    fmustruct->doStep                  = (_fmiDoStep)                    LoadFMUFcn(fmustruct, "fmiDoStep");
    fmustruct->getStatus               = (_fmiGetStatus)                 LoadFMUFcn(fmustruct, "fmiGetStatus");
    fmustruct->getRealStatus           = (_fmiGetRealStatus)             LoadFMUFcn(fmustruct, "fmiGetRealStatus");
    fmustruct->getIntegerStatus        = (_fmiGetIntegerStatus)          LoadFMUFcn(fmustruct, "fmiGetIntegerStatus");
    fmustruct->getBooleanStatus        = (_fmiGetBooleanStatus)          LoadFMUFcn(fmustruct, "fmiGetBooleanStatus");
    fmustruct->getStringStatus         = (_fmiGetStringStatus)           LoadFMUFcn(fmustruct, "fmiGetStringStatus");

    fmustruct->getVersion              = (_fmiGetVersion)                LoadFMUFcn(fmustruct, "fmiGetVersion");
    fmustruct->setDebugLogging         = (_fmiSetDebugLogging)           LoadFMUFcn(fmustruct, "fmiSetDebugLogging");
    fmustruct->setReal                 = (_fmiSetReal)                   LoadFMUFcn(fmustruct, "fmiSetReal");
    fmustruct->setInteger              = (_fmiSetInteger)                LoadFMUFcn(fmustruct, "fmiSetInteger");
    fmustruct->setBoolean              = (_fmiSetBoolean)                LoadFMUFcn(fmustruct, "fmiSetBoolean");
    fmustruct->setString               = (_fmiSetString)                 LoadFMUFcn(fmustruct, "fmiSetString");
    fmustruct->getReal                 = (_fmiGetReal)                   LoadFMUFcn(fmustruct, "fmiGetReal");
    fmustruct->getInteger              = (_fmiGetInteger)                LoadFMUFcn(fmustruct, "fmiGetInteger");
    fmustruct->getBoolean              = (_fmiGetBoolean)                LoadFMUFcn(fmustruct, "fmiGetBoolean");
    fmustruct->getString               = (_fmiGetString)                 LoadFMUFcn(fmustruct, "fmiGetString");

        
    /*instantiateSlave()*/
    timeout = 0;            /* wait period in milli seconds, 0 for unlimited wait period"*/
    visible = fmiFalse;     /* no simulator user interface*/
    interactive = fmiFalse; /* simulation run without user interaction*/

    callbacks.logger = fmu1Logger;
    callbacks.allocateMemory = calloc;
    callbacks.freeMemory = free;
    callbacks.stepFinished = NULL;

    if(fmuType == fmiCS){
        fmustruct->mFMIComp = fmustruct->instantiateSlave(instanceName,
                                                          fmuGUID,
                                                          fmuLocation,
                                                          mimeType,
                                                          timeout,
                                                          visible,
                                                          interactive,
                                                          callbacks,
                                                          fmiFalse);                      /*Logging feature OFF*/
    }else if(fmuType == fmiME){
        /*TODO: ME mode is currently not enabled
         */
    }
    
    if (NULL == fmustruct->mFMIComp ){
        CheckStatus(fmustruct, fmiError, "fmiInstantiateSlave");
        return NULL;
    }
    return (void *) fmustruct;
}

fmiBoolean FMU1_initializeSlave(void **fmuv, fmiReal tStart){
    struct FMU1_CS_RTWCG * fmustruct = (struct FMU1_CS_RTWCG *)(*fmuv);
    fmiStatus fmiFlag = fmustruct->initializeSlave(fmustruct->mFMIComp, 0, fmiFalse, 0);
    return CheckStatus(fmustruct, fmiFlag, "fmiInitializeSlave");
}

fmiBoolean FMU1_terminateSlave(void **fmuv){
    struct FMU1_CS_RTWCG * fmustruct = (struct FMU1_CS_RTWCG *)(*fmuv);
    if(fmustruct->FMUErrorStatus != fmiFatal){
        fmiStatus fmiFlag = fmustruct->terminateSlave(fmustruct->mFMIComp);
        CheckStatus(fmustruct, fmiFlag, "fmiTerminateSlave");
        if(fmustruct->FMUErrorStatus != fmiError)
            fmustruct->freeSlaveInstance(fmustruct->mFMIComp);
    }
    CLOSE_LIBRARY(fmustruct->Handle);
    free(fmustruct);
    return fmiTrue;
}

fmiStatus FMU1_doStep(void **fmuv, fmiReal currentCommunicationPoint, fmiReal communicationStepSize){
    struct FMU1_CS_RTWCG * fmustruct = (struct FMU1_CS_RTWCG *)(*fmuv);
    fmiStatus fmiFlag = fmustruct->doStep(fmustruct->mFMIComp, currentCommunicationPoint,communicationStepSize, fmiTrue);
    return CheckStatus(fmustruct, fmiFlag, "fmiDoStep");
}

fmiStatus FMU1_setRealVal(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiReal dvalue){
    struct FMU1_CS_RTWCG * fmustruct = (struct FMU1_CS_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiReal value = dvalue;
    fmiStatus fmiFlag = fmustruct->setReal(fmustruct->mFMIComp, &vr, nvr, &value);
    return CheckStatus(fmustruct, fmiFlag, "fmiSetReal");
}

fmiStatus FMU1_setReal(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiReal value[]){
    struct FMU1_CS_RTWCG * fmustruct = (struct FMU1_CS_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiStatus fmiFlag = fmustruct->setReal(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmiFlag, "fmiSetReal");
}

fmiStatus FMU1_getReal(void **fmuv, const fmiValueReference dvr, size_t nvr, fmiReal value[]){
    struct FMU1_CS_RTWCG * fmustruct = (struct FMU1_CS_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiStatus fmiFlag = fmustruct->getReal(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmiFlag, "fmiGetReal");
}

fmiStatus FMU1_setIntegerVal(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiInteger dvalue){
    struct FMU1_CS_RTWCG * fmustruct = (struct FMU1_CS_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiInteger value = dvalue;
    fmiStatus fmiFlag = fmustruct->setInteger(fmustruct->mFMIComp, &vr, nvr, &value);
    return CheckStatus(fmustruct, fmiFlag, "fmiGetInteger");
}

fmiStatus FMU1_setInteger(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiInteger value[]){
    struct FMU1_CS_RTWCG * fmustruct = (struct FMU1_CS_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiStatus fmiFlag = fmustruct->setInteger(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmiFlag, "fmiSetInteger");

}

fmiStatus FMU1_getInteger(void **fmuv, const fmiValueReference dvr, size_t nvr, fmiInteger value[]){
    struct FMU1_CS_RTWCG * fmustruct = (struct FMU1_CS_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiStatus fmiFlag = fmustruct->getInteger(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmiFlag, "fmiGetInteger");
}

fmiStatus FMU1_setBooleanVal(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiBoolean dvalue){
    struct FMU1_CS_RTWCG * fmustruct = (struct FMU1_CS_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiBoolean value = dvalue;
    fmiStatus fmiFlag = fmustruct->setBoolean(fmustruct->mFMIComp, &vr, nvr, &value);
    return CheckStatus(fmustruct, fmiFlag, "fmiSetBoolean");
}

fmiStatus FMU1_setBoolean(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiBoolean value[]){
    struct FMU1_CS_RTWCG * fmustruct = (struct FMU1_CS_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiStatus fmiFlag = fmustruct->setBoolean(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmiFlag, "fmiSetBoolean");
}

fmiStatus FMU1_getBoolean(void **fmuv, const fmiValueReference dvr, size_t nvr, fmiBoolean value[]){
    struct FMU1_CS_RTWCG * fmustruct = (struct FMU1_CS_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiStatus fmiFlag = fmustruct->getBoolean(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmiFlag, "fmiGetBoolean");
}

fmiStatus FMU1_setStringVal(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiString dvalue){
    struct FMU1_CS_RTWCG * fmustruct = (struct FMU1_CS_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiString value = dvalue;
    fmiStatus fmiFlag = fmustruct->setString(fmustruct->mFMIComp, &vr, nvr, &value);
    return CheckStatus(fmustruct, fmiFlag, "fmiSetString");
}

fmiStatus FMU1_setString(void **fmuv, const fmiValueReference dvr, size_t nvr, const fmiString value[]){
    struct FMU1_CS_RTWCG * fmustruct = (struct FMU1_CS_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiStatus fmiFlag = fmustruct->setString(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmiFlag, "fmiSetString");
}

fmiStatus FMU1_getString(void **fmuv, const fmiValueReference dvr, size_t nvr, fmiString value[]){
    struct FMU1_CS_RTWCG * fmustruct = (struct FMU1_CS_RTWCG *)(*fmuv);
    fmiValueReference vr =dvr;
    fmiStatus fmiFlag = fmustruct->getString(fmustruct->mFMIComp, &vr, nvr, value);
    return CheckStatus(fmustruct, fmiFlag, "fmiGetString");
}
