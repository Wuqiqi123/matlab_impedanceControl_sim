/* 
 *
 * Copyright 1994-2014 The MathWorks, Inc.
 *
 * File: rt_logging_mmi.c
 *
 * Abstract:
 */

#ifndef rt_logging_c
#define rt_logging_c

#include <stdlib.h>
#include <stddef.h>
#include "rtwtypes.h"
#include "builtin_typeid_types.h"
#include "rtw_matlogging.h"
#include "rtw_modelmap.h"

/* Logical definitions */
#if (!defined(__cplusplus))
#  ifndef false
#   define false                       (0U)
#  endif
#  ifndef true
#   define true                        (1U)
#  endif
#endif

static const char_T rtMemAllocError[] = "Memory allocation error";
#define FREE(m) if (m != NULL) free(m)

#define ACCESS_C_API_FOR_RTW_LOGGING true

/* Function: rt_FillStateSigInfoFromMMI =======================================
 * Abstract:
 *
 * Returns:
 *	== NULL  => success.
 *	~= NULL  => failure, the return value is a pointer to the error
 *                           message, which is also set in the simstruct.
 */
const char_T * rt_FillStateSigInfoFromMMI(RTWLogInfo   *li,
                                                 const char_T **errStatus)
{
    int_T                  i;
    int_T                  nSignals     = 0;
    int_T                  *dims        = NULL;
    BuiltInDTypeId         *dTypes      = NULL;
    int_T                  *cSgnls      = NULL;
    char_T                 **labels     = NULL;
    char_T                 **blockNames = NULL;
    char_T                 **stateNames = NULL;
    boolean_T              *crossMdlRef = NULL;
    void                   **sigDataAddr = NULL;
    RTWLoggingFcnPtr       *RTWLoggingPtrs = NULL;
    int_T                  *logDataType = NULL;
    boolean_T              *isVarDims   = NULL;


    const rtwCAPI_ModelMappingInfo *mmi = (const rtwCAPI_ModelMappingInfo *)rtliGetMMI(li);

    int_T                  sigIdx       = 0;

    RTWLogSignalInfo *     sigInfo;
    /* reset error status */
    *errStatus = NULL;

    sigInfo = (RTWLogSignalInfo *)calloc(1,sizeof(RTWLogSignalInfo));
    if (sigInfo == NULL) goto ERROR_EXIT;

    nSignals = rtwCAPI_GetNumStateRecordsForRTWLogging(mmi);

    if (nSignals >0) {
        /* These are all freed before exiting this function */
        dims        = (int_T *)calloc(nSignals,sizeof(int_T));
        if (dims == NULL) goto ERROR_EXIT;
        dTypes      = (BuiltInDTypeId *)calloc(nSignals,sizeof(BuiltInDTypeId));
        if (dTypes == NULL) goto ERROR_EXIT;
        cSgnls      = (int_T *)calloc(nSignals,sizeof(int_T));
        if (cSgnls == NULL) goto ERROR_EXIT;
        labels      = (char_T **)calloc(nSignals, sizeof(char_T*));
        if (labels == NULL) goto ERROR_EXIT;
        blockNames  = (char_T**)calloc(nSignals, sizeof(char_T*));
        if (blockNames == NULL) goto ERROR_EXIT;
        stateNames  = (char_T**)calloc(nSignals, sizeof(char_T*));
        if (stateNames == NULL) goto ERROR_EXIT;
        crossMdlRef  = (boolean_T*)calloc(nSignals, sizeof(boolean_T));
        if (crossMdlRef == NULL) goto ERROR_EXIT;
        logDataType = (int_T *)calloc(nSignals,sizeof(int_T));
        if (logDataType == NULL) goto ERROR_EXIT;
        /* Allocate memory for isVarDims pointer and set all elements to 0's */
        isVarDims = (boolean_T *)calloc(nSignals,sizeof(boolean_T));
        if (isVarDims == NULL) goto ERROR_EXIT;

        /* These are freed in stopDataLogging (they're needed in the meantime) */
        sigDataAddr = (void **)calloc(nSignals,sizeof(void *));
        if (sigDataAddr == NULL) goto ERROR_EXIT;
        RTWLoggingPtrs = (RTWLoggingFcnPtr *)calloc(nSignals, sizeof(RTWLoggingFcnPtr));
        if (RTWLoggingPtrs == NULL) goto ERROR_EXIT;

        *errStatus = rtwCAPI_GetStateRecordInfo(mmi,
                                                (const char_T**) blockNames,
                                                (const char_T**) labels,
                                                (const char_T**) stateNames,
                                                dims,
                                                (int_T*)dTypes,
                                                logDataType,
                                                cSgnls,
                                                sigDataAddr,
                                                RTWLoggingPtrs,
                                                crossMdlRef,
                                                NULL, /* sigInProtectedMdl */
                                                NULL,
                                                NULL, /* sigSampleTime */
                                                NULL, /* sigHierInfoIdx */
                                                NULL, /* sigFlatElemIdx */
                                                NULL, /* sigMMI */
                                                &sigIdx,
                                                false, /* crossingModel */
                                                false, /* isInProtectedMdl */
                                                NULL,  /* stateDerivVector */
                                                ACCESS_C_API_FOR_RTW_LOGGING);

        if (*errStatus != NULL) goto ERROR_EXIT;

        rtliSetLogXSignalPtrs(li,(LogSignalPtrsType)sigDataAddr);
    }

    sigInfo->numSignals = nSignals;
    sigInfo->numCols = dims;
    sigInfo->numDims = NULL;
    sigInfo->dims = dims;
    sigInfo->dataTypes = dTypes;
    sigInfo->complexSignals = cSgnls;
    sigInfo->frameData = NULL;
    sigInfo->preprocessingPtrs = (RTWPreprocessingFcnPtr*) RTWLoggingPtrs;
    sigInfo->labels.ptr = labels;
    sigInfo->titles = NULL;
    sigInfo->titleLengths = NULL;
    sigInfo->plotStyles = NULL;
    sigInfo->blockNames.ptr = blockNames;
    sigInfo->stateNames.ptr = stateNames;
    sigInfo->crossMdlRef = crossMdlRef;
    sigInfo->dataTypeConvert = NULL;

    sigInfo->isVarDims = isVarDims;
    sigInfo->currSigDims = NULL;

    rtliSetLogXSignalInfo(li,sigInfo);

    /* Free logDataType it's not needed any more,
     * the rest of them will be freed later */
    FREE(logDataType);
    return(NULL); /* NORMAL_EXIT */

  ERROR_EXIT:
    if (*errStatus == NULL) {
        *errStatus = rtMemAllocError;
    }
    /* Free local stuff that was allocated. It is no longer needed */
    for (i = 0; i < nSignals; ++i) utFree(blockNames[i]);
    FREE(blockNames);
    for (i = 0; i < nSignals; ++i) utFree(stateNames[i]);
    FREE(stateNames);
    FREE(labels);
    FREE(dims);
    FREE(dTypes);
    FREE(logDataType);
    FREE(cSgnls);
    FREE(isVarDims);
    return(*errStatus);

} /* end rt_InitSignalsStruct */

void rt_CleanUpForStateLogWithMMI(RTWLogInfo *li)
{
    int_T i;
    RTWLogSignalInfo *sigInfo = _rtliGetLogXSignalInfo(li); /* get the non-const ptr */
    int_T nSignals = sigInfo->numSignals;

    if ( nSignals > 0 ) {

        for (i = 0; i < nSignals; ++i) utFree(sigInfo->blockNames.ptr[i]);
        FREE(sigInfo->blockNames.ptr);
        FREE(sigInfo->labels.ptr);
        FREE(sigInfo->crossMdlRef);
        FREE(sigInfo->dims);
        FREE(sigInfo->dataTypes);
        FREE(sigInfo->complexSignals);
        FREE(sigInfo->isVarDims);

        FREE(sigInfo);
        rtliSetLogXSignalInfo(li, NULL);

        FREE(_rtliGetLogXSignalPtrs(li)); /* get the non-const ptr */
        rtliSetLogXSignalPtrs(li,NULL);
    }
}

#endif /*  rt_logging_mmi_c */

/* LocalWords:  Hier Deriv
 */
