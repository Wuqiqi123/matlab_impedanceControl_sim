/* Copyright 2003-2014 The MathWorks, Inc. */

/**
 * Utility functions to traverse and access information from ModelMappingInfo
 *
 */

#ifdef SL_INTERNAL

# include "version.h"
# include "util.h"
# include "simstruct/simstruc_types.h"
# include "simulinkcoder_capi/rtw_modelmap.h"

#else

# include <stdlib.h>
# include <assert.h>

# define  utFree(arg)    if (arg) free(arg)
# define  utMalloc(arg)  malloc(arg)
# define  utAssert(exp)  assert(exp)

/*
 * UNUSED_PARAMETER(x)
 *   Used to specify that a function parameter (argument) is required but not
 *   accessed by the function body.
 */
#ifndef UNUSED_PARAMETER
# if defined(__LCC__)
#   define UNUSED_PARAMETER(x)  /* do nothing */
# else
/*
 * This is the semi-ANSI standard way of indicating that a
 * unused function parameter is required.
 */
#   define UNUSED_PARAMETER(x) (void) (x)
# endif
#endif

# include "builtin_typeid_types.h"
# include "rtwtypes.h"
# include "rtw_modelmap.h"

#endif

#include <string.h>

/* Logical definitions */
#if (!defined(__cplusplus))
#  ifndef false
#   define false                       (0U)
#  endif
#  ifndef true
#   define true                        (1U)
#  endif
#endif

static const char_T* rtwCAPI_mallocError = "Memory Allocation Error";

/** Function: rtwCAPI_EncodePath ===============================================
 *  Abstract:
 *     Escape all '|' characters in bpath. For examples 'aaa|b' will become
 *     'aaa~|b'. The caller is responsible for freeing the returned string
 *
 *
 * NOTE: returned string can be NULL in two cases:
 *     (1) string passed in was NULL
 *     (2) a memory allocation error occurred
 * In the second case, the caller need to report the error
 */
char* rtwCAPI_EncodePath(const char* path)
{
    char* encodedPath     = NULL;
    size_t pathLen        = (path==NULL) ? 0:strlen(path) + 1;
    size_t encodedPathLen = pathLen;
    unsigned i;
    unsigned j = 0;

    if (path == NULL) return NULL;

    for (i = 0; i < pathLen; ++i) {
        if (path[i] == '|' || path[i] == '~') ++encodedPathLen;
    }

    encodedPath = (char_T*)utMalloc(encodedPathLen*sizeof(char_T));
    if (encodedPath == NULL) return encodedPath;

    for (i = 0; i < pathLen; ++i) {
        char ch = path[i];
        if (ch == '~' || ch == '|') encodedPath[j++] = '~';
        encodedPath[j++] = ch;
    }
    utAssert(j == encodedPathLen);
    utAssert(encodedPath[j-1] == '\0');

    return encodedPath;

} /* rtwCAPI_EncodePath */

/** Function: rtwCAPI_GetSigAddrFromMap ========================================
 *
 */
void rtwCAPI_GetSigAddrFromMap(uint8_T      isPointer,
                               int_T*       sigComplexity,
                               int_T*       sigDataType,
                               void**       sigDataAddr,
                               int_T*       sigIdx,
                               uint_T       mapIdx,
                               void**       dataAddrMap)
{
    if (isPointer) {
        /* Dereference pointer and cache the address */

        /* Imported Pointers cannot be complex - Assert */
        utAssert(sigComplexity[*sigIdx] != 1);
        UNUSED_PARAMETER(sigComplexity);

        /* Check for data type and dereference accordingly */
        switch (sigDataType[*sigIdx]) {
          case SS_DOUBLE:
            sigDataAddr[*sigIdx] = \
                (void*) *((real_T **) dataAddrMap[mapIdx]);
            break;
          case SS_SINGLE:
            sigDataAddr[*sigIdx] = \
                (void*) *((real32_T **) dataAddrMap[mapIdx]);
            break;
          case SS_UINT32:
            sigDataAddr[*sigIdx] = \
                (void*) *((uint32_T **) dataAddrMap[mapIdx]);
            break;
          case SS_INT32:
            sigDataAddr[*sigIdx] = \
                (void*) *((int32_T **) dataAddrMap[mapIdx]);
            break;
          case SS_UINT16:
            sigDataAddr[*sigIdx] = \
                (void*) *((uint16_T **) dataAddrMap[mapIdx]);
            break;
          case SS_INT16:
            sigDataAddr[*sigIdx] = \
                (void*) *((int16_T **) dataAddrMap[mapIdx]);
            break;
          case SS_UINT8:
            sigDataAddr[*sigIdx] = \
                (void*) *((uint8_T **) dataAddrMap[mapIdx]);
            break;
          case SS_INT8:
            sigDataAddr[*sigIdx] = \
                (void*) *((int8_T **) dataAddrMap[mapIdx]);
            break;
          case SS_BOOLEAN:
            sigDataAddr[*sigIdx] = \
                (void*) *((boolean_T **) dataAddrMap[mapIdx]);
            break;
          default:
            sigDataAddr[*sigIdx] = \
                (void*) *((real_T **) dataAddrMap[mapIdx]);
            break;
        }  /* end switch */
    } else {
        /* if Data is not a pointer store the address directly */
        sigDataAddr[*sigIdx] = dataAddrMap[mapIdx];
    }

} /* rtwCAPI_GetSigAddrFromMap */


/** Function: rtwCAPI_HasStates ================================================
 *
 */
boolean_T rtwCAPI_HasStates(const rtwCAPI_ModelMappingInfo* mmi)
{
    int_T i;
    int_T nCMMI;

    if (mmi == NULL) return(0U);

    if (rtwCAPI_GetNumStates(mmi) > 0) return(1U);

    nCMMI = rtwCAPI_GetChildMMIArrayLen(mmi);
    for (i = 0; i < nCMMI; ++i) {
        if (rtwCAPI_HasStates(rtwCAPI_GetChildMMI(mmi,i))) return(1U);
    }
    return(0U);

} /* rtwCAPI_HasStates */



/** Function: rtwCAPI_GetNumStateRecords =======================================
 *
 */
int_T rtwCAPI_GetNumStateRecords(const rtwCAPI_ModelMappingInfo* mmi)
{
    int_T i;
    int_T nRecs;
    int_T nCMMI;

    if (mmi == NULL) return(0);

    nRecs = rtwCAPI_GetNumStates(mmi);

    nCMMI = rtwCAPI_GetChildMMIArrayLen(mmi);
    for (i = 0; i < nCMMI; ++i) {
        const rtwCAPI_ModelMappingInfo* cMMI = rtwCAPI_GetChildMMI(mmi,i);
        nRecs += rtwCAPI_GetNumStateRecords(cMMI);
    }
    return(nRecs);

} /* rtwCAPI_GetNumStateRecords */


/** Function: rtwCAPI_GetNumStateRecordsForRTWLogging ==========================
 *
 */
int_T rtwCAPI_GetNumStateRecordsForRTWLogging(const rtwCAPI_ModelMappingInfo* mmi)
{
    int_T i;
    int_T nRecs = 0;
    int_T nStates;
    int_T nCMMI;
    const rtwCAPI_States *states;
    const rtwCAPI_DataTypeMap* dataTypeMap;

    if (mmi == NULL) return(0);

    nStates = rtwCAPI_GetNumStates(mmi);
    states = rtwCAPI_GetStates(mmi);
    dataTypeMap = rtwCAPI_GetDataTypeMap(mmi);

    for (i = 0; i < nStates; ++i) {
        if (rtwCAPI_CanLogStateToMATFile(dataTypeMap, states, i)) {
            ++nRecs;
        }
    }

    nCMMI = rtwCAPI_GetChildMMIArrayLen(mmi);
    for (i = 0; i < nCMMI; ++i) {
        const rtwCAPI_ModelMappingInfo* cMMI = rtwCAPI_GetChildMMI(mmi,i);
        nRecs += rtwCAPI_GetNumStateRecordsForRTWLogging(cMMI);
    }

    return(nRecs);

} /* rtwCAPI_GetNumStateRecordsForRTWLogging */


/** Function: rtwCAPI_GetNumContStateRecords ===================================
 *
 */
int_T rtwCAPI_GetNumContStateRecords(const rtwCAPI_ModelMappingInfo* mmi)
{
    int_T i;
    int_T nRecs;
    int_T nCMMI;
    int_T nCStateRecs;
    const rtwCAPI_States* states;
    const rtwCAPI_DataTypeMap* dataTypeMap;

    if (mmi == NULL) return 0;

    nCStateRecs = 0;
    states      = rtwCAPI_GetStates(mmi);
    dataTypeMap = rtwCAPI_GetDataTypeMap(mmi);

    nRecs = rtwCAPI_GetNumStates(mmi);
    for (i = 0; i < nRecs; i++) {
        if (rtwCAPI_IsAContinuousState(states,i)) {
            ++nCStateRecs;

            /* All continuous states should be able to be logged to MAT-File
             * so we do not need to skip any states here. */
            utAssert(rtwCAPI_CanLogStateToMATFile(dataTypeMap, states, i));
        }
    }

    nCMMI = rtwCAPI_GetChildMMIArrayLen(mmi);
    for (i = 0; i < nCMMI; ++i) {
        const rtwCAPI_ModelMappingInfo* cMMI = rtwCAPI_GetChildMMI(mmi,i);
        nCStateRecs += rtwCAPI_GetNumContStateRecords(cMMI);
    }
    return nCStateRecs;

} /* rtwCAPI_GetNumContStateRecords */


/** Function: rtwCAPI_FreeFullPaths ============================================
 *
 */
void rtwCAPI_FreeFullPaths(rtwCAPI_ModelMappingInfo* mmi)
{
    int_T   i;
    int_T   nCMMI;
    char_T* fullPath;

    if (mmi == NULL) return;

    fullPath = rtwCAPI_GetFullPath(mmi);
    utAssert(fullPath != NULL);
    utFree(fullPath);
    rtwCAPI_SetFullPath(*mmi, NULL);

    nCMMI = rtwCAPI_GetChildMMIArrayLen(mmi);
    for (i = 0; i < nCMMI; ++i) {
        rtwCAPI_ModelMappingInfo* cMMI = rtwCAPI_GetChildMMI(mmi,i);
        rtwCAPI_FreeFullPaths(cMMI);
    }

} /* rtwCAPI_FreeFullPaths */


/** Function: rtwCAPI_UpdateFullPaths =========================================*
 *
 */
const char_T* rtwCAPI_UpdateFullPaths(rtwCAPI_ModelMappingInfo* mmi,
                                      const char_T* path,
                                      boolean_T isCalledFromTopModel)
{
    int_T         i;
    int_T         nCMMI;
    size_t        pathLen;
    char_T*       mmiPath;
    size_t        mmiPathLen;
    char_T*       relMMIPath;
    size_t        relMMIPathLen;

    if (mmi == NULL) return NULL;

    utAssert(path != NULL);
    utAssert( rtwCAPI_GetFullPath(mmi) == NULL );

    pathLen = strlen(path)+1;

    if (isCalledFromTopModel) {
        /* If called from top model - FullPath is same as path */
        mmiPath = (char_T*)utMalloc(pathLen*sizeof(char_T));
        (void)memcpy(mmiPath, path, pathLen*sizeof(char_T));
    }
    else {        
        relMMIPath = rtwCAPI_EncodePath(rtwCAPI_GetPath(mmi));
        if ( (relMMIPath== NULL) && (rtwCAPI_GetPath(mmi) != NULL)) {
            return rtwCAPI_mallocError;
        }
        relMMIPathLen = relMMIPath ? (strlen(relMMIPath) + 1) : 0;
        
        mmiPathLen = pathLen + relMMIPathLen;
        
        mmiPath = (char_T*)utMalloc(mmiPathLen*sizeof(char_T));
        if (mmiPath == NULL) return rtwCAPI_mallocError;
        (void)memcpy(mmiPath, path, pathLen*sizeof(char_T));
        utAssert(mmiPath[pathLen-1] == '\0');
        
        if (relMMIPath) {
            /* mmiPath = path + | + relMMIPath + '\0' */
            mmiPath[pathLen-1] = '|';
            (void)memcpy(&(mmiPath[pathLen]),
                         relMMIPath, relMMIPathLen*sizeof(char_T));
            utAssert(mmiPath[mmiPathLen-1] == '\0');
            utFree(relMMIPath);
        }
    }
    rtwCAPI_SetFullPath(*mmi, mmiPath);

    nCMMI = rtwCAPI_GetChildMMIArrayLen(mmi);
    for (i = 0; i < nCMMI; ++i) {
        rtwCAPI_ModelMappingInfo* cMMI = rtwCAPI_GetChildMMI(mmi,i);
        const char_T* errstr = rtwCAPI_UpdateFullPaths(cMMI, mmiPath, 0);
        if (errstr != NULL) return errstr;
    }
    return NULL;

} /* rtwCAPI_UpdateFullPaths */


/** Function: rtwCAPI_GetFullStateBlockPath ===================================
 *
 */
char* rtwCAPI_GetFullStateBlockPath(const char* stateBlockPath,
                                           const char* mmiPath,
                                           size_t      mmiPathLen,
                                           boolean_T   crossingModel)
{
    char_T* blockPath          = NULL;
    char_T* fullStateBlockPath = NULL;
    size_t     fullStateBlockPathLen;

    if (stateBlockPath == NULL) goto EXIT_POINT;

    /* fullStateBlockPath = mmiPath + | + blockPath + '\0' */
    /* If crossing a model boundary encode, otherwise do not */

    if (crossingModel) {
        blockPath = rtwCAPI_EncodePath(stateBlockPath);
        if (blockPath == NULL) goto EXIT_POINT;
    } else {
        size_t len = strlen(stateBlockPath)+1;
        blockPath = (char*)utMalloc(len*sizeof(char));
        if (blockPath == NULL) goto EXIT_POINT;
        (void)strcpy(blockPath,stateBlockPath);
    }
    utAssert(blockPath != NULL);
    fullStateBlockPathLen = ( (mmiPath==NULL) ?
                              strlen(blockPath) + 1 :
                              mmiPathLen + strlen(blockPath) + 2 );
    fullStateBlockPath    = (char*)utMalloc(fullStateBlockPathLen*sizeof(char));
    if (fullStateBlockPath == NULL) goto EXIT_POINT;

    if (mmiPath != NULL) {
        (void)strcpy(fullStateBlockPath, mmiPath);
        fullStateBlockPath[mmiPathLen]   = '|';
        fullStateBlockPath[mmiPathLen+1] =  '\0';
        (void)strcat(fullStateBlockPath, blockPath);
    } else {
        (void)strcpy(fullStateBlockPath, blockPath);
        fullStateBlockPath[fullStateBlockPathLen-1] =  '\0';
    }
    utAssert(fullStateBlockPath[fullStateBlockPathLen-1] == '\0');

  EXIT_POINT:
    utFree(blockPath);
    return fullStateBlockPath; /* caller is responsible for free */
}

uint_T rtwCAPI_GetStateWidth(const rtwCAPI_DimensionMap* dimMap,
                             const uint_T*               dimArray,
                             const rtwCAPI_States*       states,
                             uint_T                      iState)
{
    uint_T mapIdx = rtwCAPI_GetStateDimensionIdx(states, iState);
    utAssert( rtwCAPI_GetNumDims(dimMap,mapIdx) == 2 );
    mapIdx = rtwCAPI_GetDimArrayIndex(dimMap, mapIdx);
    return (dimArray[mapIdx] * dimArray[mapIdx+1]);
}



/** Function: rtwCAPI_GetStateRecordInfo =======================================
 *
 */
const char_T* rtwCAPI_GetStateRecordInfo(const rtwCAPI_ModelMappingInfo* mmi,
                                         const char_T**    sigBlockName,
                                         const char_T**    sigLabel,
                                         const char_T**    sigName,
                                         int_T*            sigWidth,
                                         int_T*            sigDataType,
                                         int_T*            logDataType,
                                         int_T*            sigComplexity,
                                         void**            sigDataAddr,
                                         RTWLoggingFcnPtr* RTWLoggingPtrs,
                                         boolean_T*        sigCrossMdlRef,
                                         boolean_T*        sigInProtectedMdl,
                                         const char_T**    sigPathAlias,
                                         real_T*           sigSampleTime,
                                         int_T*            sigHierInfoIdx,
                                         uint_T*           sigFlatElemIdx,                                         
                                         const rtwCAPI_ModelMappingInfo** sigMMI,
                                         int_T*            sigIdx,
                                         boolean_T         crossingModel,
                                         boolean_T         isInProtectedMdl,
                                         real_T*           contStateDeriv,
                                         boolean_T         rtwLogging)
{
    int_T               i;
    int_T               nCMMI;
    int_T               nStates;
    const char_T*       mmiPath;
    size_t              mmiPathLen;
    const rtwCAPI_States*  states;
    const rtwCAPI_DimensionMap* dimMap;
    const uint_T*       dimArray;
    const rtwCAPI_DataTypeMap*  dataTypeMap;
    void**              dataAddrMap;
    RTWLoggingFcnPtr* RTWLoggingPtrsMap;
    const char_T*       errstr = NULL;
    uint8_T             isPointer = 0;

    if (mmi == NULL) goto EXIT_POINT;
    isInProtectedMdl = isInProtectedMdl || rtwCAPI_IsProtectedModel(mmi);
    nCMMI = rtwCAPI_GetChildMMIArrayLen(mmi);
    for (i = 0; i < nCMMI; ++i) {
        rtwCAPI_ModelMappingInfo* cMMI = rtwCAPI_GetChildMMI(mmi,i);
        real_T* childContStateDeriv = NULL;

        if (cMMI == NULL) continue;

        if (contStateDeriv) {
            int idx;

            idx = rtwCAPI_MMIGetContStateStartIndex(cMMI);
            if(idx == -1) continue;
            
            childContStateDeriv = &contStateDeriv[idx];
        }
        errstr = rtwCAPI_GetStateRecordInfo(cMMI,
                                            sigBlockName,
                                            sigLabel,
                                            sigName,
                                            sigWidth,
                                            sigDataType,
                                            logDataType,
                                            sigComplexity,
                                            sigDataAddr,
                                            RTWLoggingPtrs,
                                            sigCrossMdlRef,
                                            sigInProtectedMdl,
                                            sigPathAlias,
                                            sigSampleTime,
                                            sigHierInfoIdx,
                                            sigFlatElemIdx,
                                            sigMMI,
                                            sigIdx,
                                            0x1, /* true, */
                                            isInProtectedMdl,
                                            childContStateDeriv,
                                            rtwLogging);
        if (errstr != NULL) goto EXIT_POINT;
    }

    nStates = rtwCAPI_GetNumStates(mmi);
    if (nStates < 1) goto EXIT_POINT;

    mmiPath     = rtwCAPI_GetFullPath(mmi);
    mmiPathLen  = (mmiPath==NULL)? 0 : strlen(mmiPath);
    states      = rtwCAPI_GetStates(mmi);
    dimMap      = rtwCAPI_GetDimensionMap(mmi);
    dimArray    = rtwCAPI_GetDimensionArray(mmi);
    dataTypeMap = rtwCAPI_GetDataTypeMap(mmi);
    dataAddrMap = rtwCAPI_GetDataAddressMap(mmi);
    RTWLoggingPtrsMap = rtwCAPI_GetRTWLoggingPtrsMap(mmi);

    for (i = 0; i < nStates; ++i) {
        uint_T      mapIdx;

        /* If collecting continuous states, skip non-continuous states */
        if (contStateDeriv && !rtwCAPI_IsAContinuousState(states,i)) continue;

        /* For RTW logging, skip states that cannot be logged to MAT-File. */
        if ((rtwLogging) &&
            (rtwCAPI_CanLogStateToMATFile(dataTypeMap, states, i) == false)) continue;

        /* BlockPath (caller is responsible for free) */
        sigBlockName[*sigIdx] =
            rtwCAPI_GetFullStateBlockPath(rtwCAPI_GetStateBlockPath(states,i),
                                          mmiPath, mmiPathLen, crossingModel);
        if (sigBlockName[*sigIdx] == NULL) {
            errstr = rtwCAPI_mallocError;
            goto EXIT_POINT;
        }

        /* Label */
        if (rtwCAPI_IsAContinuousState(states,i)){
            sigLabel[*sigIdx] = "CSTATE";
        } else {
            sigLabel[*sigIdx] = "DSTATE";
        }
        sigName[*sigIdx] = rtwCAPI_GetStateName(states, i);

        /* Width */
        sigWidth[*sigIdx] = rtwCAPI_GetStateWidth(dimMap, dimArray, states, i);

        /* DataType and logDataType */
        mapIdx = rtwCAPI_GetStateDataTypeIdx(states, i);
        sigDataType[*sigIdx] = rtwCAPI_GetDataTypeSLId(dataTypeMap, mapIdx);
        /* this mimics code in simulink.dll:DtGetDataTypeLoggingId() */
        if (logDataType) {
            switch (sigDataType[*sigIdx]) {
              case SS_DOUBLE:
              case SS_SINGLE:
              case SS_INT8:
              case SS_UINT8:
              case SS_INT16:
              case SS_UINT16:
              case SS_INT32:
              case SS_UINT32:
              case SS_BOOLEAN:
                logDataType[*sigIdx] = sigDataType[*sigIdx];
                break;
              case SS_ENUM_TYPE:
                logDataType[*sigIdx] = SS_INT32;
                break;
              default:
                logDataType[*sigIdx] = SS_DOUBLE;
                break;
            }
        }

        /* Complexity */
        sigComplexity[*sigIdx] = rtwCAPI_GetDataIsComplex(dataTypeMap, mapIdx);

        /* Data Access - Pointer or Direct*/
        isPointer = ((uint8_T)rtwCAPI_GetDataIsPointer(dataTypeMap, mapIdx));

        /* Address */
        if (contStateDeriv) {
            int_T csvIdx = rtwCAPI_GetContStateStartIndex(states,i);
            utAssert(csvIdx >= 0);
            sigDataAddr[*sigIdx] = &contStateDeriv[csvIdx];
        } else {
            mapIdx = rtwCAPI_GetStateAddrIdx(states,i);
            rtwCAPI_GetSigAddrFromMap(isPointer, sigComplexity, sigDataType,
                                      sigDataAddr, sigIdx, mapIdx, dataAddrMap);
        }

        /* Logging function pointer */
        if (RTWLoggingPtrs) {
            if (contStateDeriv || !RTWLoggingPtrsMap) {
                RTWLoggingPtrs[*sigIdx] = NULL;
            }
            else {
                mapIdx = rtwCAPI_GetStateAddrIdx(states, i);
                RTWLoggingPtrs[*sigIdx] = RTWLoggingPtrsMap[mapIdx];
            }
        }

        /* CrossingModelBoundary */
        sigCrossMdlRef[*sigIdx] = crossingModel;

        if (sigInProtectedMdl)
        {
            sigInProtectedMdl[*sigIdx] = isInProtectedMdl;
        }

        if (sigPathAlias && 
            rtwCAPI_GetStatePathAlias(states,i) != NULL && 
            rtwCAPI_GetStatePathAlias(states,i)[0] != '\0') {
            sigPathAlias[*sigIdx] =  
                rtwCAPI_GetFullStateBlockPath(rtwCAPI_GetStatePathAlias(states,i),
                                              mmiPath, mmiPathLen, crossingModel);
        }
        
        /* Sample Time */
        if (sigSampleTime) {
            const rtwCAPI_SampleTimeMap* tsMap = rtwCAPI_GetSampleTimeMap(mmi);
            int_T TID;
            mapIdx = rtwCAPI_GetStateSampleTimeIdx(states, i);
            TID = rtwCAPI_GetSampleTimeTID(tsMap, mapIdx);
            if (TID >= 0) {
                sigSampleTime[2*(*sigIdx)] =
                    *((const real_T*)rtwCAPI_GetSamplePeriodPtr(tsMap,mapIdx));
                sigSampleTime[2*(*sigIdx)+1] =
                    *((const real_T*)rtwCAPI_GetSampleOffsetPtr(tsMap,mapIdx));
            } else { /* triggered */
                utAssert(TID==-1); 
                sigSampleTime[2*(*sigIdx)]   = -1.0;
                sigSampleTime[2*(*sigIdx)+1] = -1.0;
            }
        }

        /* HierInfoIdx and FlatElemIdx */
        if (sigHierInfoIdx && sigFlatElemIdx)
        {
            sigHierInfoIdx[*sigIdx] = rtwCAPI_GetStateHierInfoIdx(states, i);
            sigFlatElemIdx[*sigIdx] = rtwCAPI_GetStateFlatElemIdx(states, i);
        }      

        /* MMI for each state */
        if (sigMMI)
        {
            sigMMI[*sigIdx] = mmi;
        }        

        ++(*sigIdx);
    }

  EXIT_POINT:
    return(errstr);

} /* rtwCAPI_GetStateRecordInfo */

/* Signal Logging functions */

/** Function: rtwCAPI_GetNumSigLogRecords ======================================
 *
 */
int_T rtwCAPI_GetNumSigLogRecords(const rtwCAPI_ModelMappingInfo* mmi)
{
    int_T i;
    int_T nRecs;
    int_T nCMMI;

    if (mmi == NULL) return(0);

    nRecs = rtwCAPI_GetNumSignals(mmi);

    nCMMI = rtwCAPI_GetChildMMIArrayLen(mmi);
    for (i = 0; i < nCMMI; ++i) {
        const rtwCAPI_ModelMappingInfo* cMMI = rtwCAPI_GetChildMMI(mmi,i);
        nRecs += rtwCAPI_GetNumSigLogRecords(cMMI);
    }
    return(nRecs);

} /* rtwCAPI_GetNumSigLogRecords */


/** Function: rtwCAPI_GetNumSigLogRecordsForRTWLogging =========================
 *
 */
int_T rtwCAPI_GetNumSigLogRecordsForRTWLogging(const rtwCAPI_ModelMappingInfo* mmi)
{
    int_T i;
    int_T nRecs = 0;
    int_T nSignals = 0;
    int_T nCMMI;
    const rtwCAPI_Signals *signals = NULL;
    const rtwCAPI_DataTypeMap* dataTypeMap;

    if (mmi == NULL) return(0);

    nSignals = rtwCAPI_GetNumSignals(mmi);
    signals = rtwCAPI_GetSignals(mmi);
    dataTypeMap = rtwCAPI_GetDataTypeMap(mmi);

    for (i = 0; i < nSignals; ++i) {
        if (rtwCAPI_CanLogSignalToMATFile(dataTypeMap, signals, i)) {
            ++nRecs;
        }
    }

    nCMMI = rtwCAPI_GetChildMMIArrayLen(mmi);
    for (i = 0; i < nCMMI; ++i) {
        const rtwCAPI_ModelMappingInfo* cMMI = rtwCAPI_GetChildMMI(mmi,i);
        nRecs += rtwCAPI_GetNumSigLogRecordsForRTWLogging(cMMI);
    }

    return(nRecs);

} /* rtwCAPI_GetNumSigLogRecords */


/** Function: rtwCAPI_GetSigLogRecordInfo ======================================
 *
 */
const char_T* rtwCAPI_GetSigLogRecordInfo(const rtwCAPI_ModelMappingInfo* mmi,
                                          const char_T**    sigBlockName,
                                          const char_T**    sigLabel,
                                          int_T*            sigWidth,
                                          int_T*            sigDataType,
                                          int_T*            logDataType,
                                          int_T*            sigComplexity,
                                          void**            sigDataAddr,
                                          boolean_T*        sigCrossMdlRef,
                                          int_T*            sigIdx,
                                          boolean_T         crossingModel,
                                          boolean_T         rtwLogging)
{
    int_T               i;
    int_T               nCMMI;
    int_T               nSignals;
    const char_T*       mmiPath;
    size_t              mmiPathLen;
    const rtwCAPI_Signals*  signals;
    const rtwCAPI_DimensionMap* dimMap;
    const uint_T*       dimArray;
    const rtwCAPI_DataTypeMap*  dataTypeMap;
    void**              dataAddrMap;
    const char_T*       errstr = NULL;
    uint8_T             isPointer = 0;
    char*               blockPath = NULL;

    if (mmi == NULL) goto EXIT_POINT;

    nCMMI = rtwCAPI_GetChildMMIArrayLen(mmi);
    for (i = 0; i < nCMMI; ++i) {
        rtwCAPI_ModelMappingInfo* cMMI = rtwCAPI_GetChildMMI(mmi,i);

        errstr = rtwCAPI_GetSigLogRecordInfo(cMMI,
                                             sigBlockName,
                                             sigLabel,
                                             sigWidth,
                                             sigDataType,
                                             logDataType,
                                             sigComplexity,
                                             sigDataAddr,
                                             sigCrossMdlRef,
                                             sigIdx,
                                             true,
                                             rtwLogging);
        if (errstr != NULL) goto EXIT_POINT;
    }

    nSignals = rtwCAPI_GetNumSignals(mmi);
    if (nSignals < 1) goto EXIT_POINT;

    mmiPath     = rtwCAPI_GetFullPath(mmi);
    mmiPathLen  = (mmiPath==NULL)? 0 : strlen(mmiPath);
    signals     = rtwCAPI_GetSignals(mmi);
    dimMap      = rtwCAPI_GetDimensionMap(mmi);
    dimArray    = rtwCAPI_GetDimensionArray(mmi);
    dataTypeMap = rtwCAPI_GetDataTypeMap(mmi);
    dataAddrMap = rtwCAPI_GetDataAddressMap(mmi);

    for (i = 0; i < nSignals; ++i) {
        uint_T mapIdx;
        size_t sigPathLen;
        char*  sigPath;

        /* For RTW logging, skip states that cannot be logged to MAT-File. */
        if ((rtwLogging) &&
            (rtwCAPI_CanLogSignalToMATFile(dataTypeMap, signals, i) == false)) continue;

        /* sigBlockPath = mmiPath + | + BlockPath + '\0' */
        /* If crossing a model boundary encode, otherwise do not */

        if (crossingModel) {
            blockPath = rtwCAPI_EncodePath(rtwCAPI_GetSignalBlockPath(signals, i));
            if ( (blockPath == NULL) &&
                 (rtwCAPI_GetSignalBlockPath(signals, i) != NULL)) {
                errstr = rtwCAPI_mallocError;
                goto EXIT_POINT;
            }
        } else {
            const char* constBlockPath = rtwCAPI_GetSignalBlockPath(signals, i);
            blockPath = (char*)utMalloc((strlen(constBlockPath)+1)*sizeof(char));
            (void)strcpy(blockPath, constBlockPath);
        }
        utAssert(blockPath != NULL);
        sigPathLen = ( (mmiPath==NULL) ?
                                   strlen(blockPath) + 1 :
                                   mmiPathLen + strlen(blockPath) + 2 );
        sigPath    = (char*)utMalloc(sigPathLen*sizeof(char));
        if (sigPath == NULL) {
            errstr = rtwCAPI_mallocError;
            goto EXIT_POINT;
        }
        if (mmiPath != NULL) {
            (void)strcpy(sigPath, mmiPath);
            sigPath[mmiPathLen]   = '|';
            sigPath[mmiPathLen+1] =  '\0';
            (void)strcat(sigPath, blockPath);
        } else {
            (void)strcpy(sigPath, blockPath);
            sigPath[sigPathLen-1] =  '\0';
        }
       /* need to free for every iteration of the loop, but also have
        * the free below EXIT_POINT in case of error */
        utFree(blockPath);
        blockPath = NULL;
        utAssert(sigPath[sigPathLen-1] == '\0');
        sigBlockName[*sigIdx] = sigPath; /* caller is responsible for free */

        /* Label */
        sigLabel[*sigIdx] = rtwCAPI_GetSignalName(signals, i);

        /* Width */
        mapIdx = rtwCAPI_GetSignalDimensionIdx(signals, i);
        utAssert( rtwCAPI_GetNumDims(dimMap,mapIdx) == 2 );
        mapIdx = rtwCAPI_GetDimArrayIndex(dimMap, mapIdx);
        sigWidth[*sigIdx] = dimArray[mapIdx] * dimArray[mapIdx+1];

        /* DataType and logDataType */
        mapIdx = rtwCAPI_GetSignalDataTypeIdx(signals, i);
        sigDataType[*sigIdx] = rtwCAPI_GetDataTypeSLId(dataTypeMap, mapIdx);
        /* this mimics code in simulink.dll:mapSigDataTypeToLogDataType */
        switch (sigDataType[*sigIdx]) {
          case SS_DOUBLE:
          case SS_SINGLE:
          case SS_INT8:
          case SS_UINT8:
          case SS_INT16:
          case SS_UINT16:
          case SS_INT32:
          case SS_UINT32:
          case SS_BOOLEAN:
            logDataType[*sigIdx] = sigDataType[*sigIdx];
            break;
          case SS_ENUM_TYPE:
            logDataType[*sigIdx] = SS_INT32;
            break;
          default:
            logDataType[*sigIdx] = SS_DOUBLE;
            break;
        }

        /* Complexity */
        sigComplexity[*sigIdx] = rtwCAPI_GetDataIsComplex(dataTypeMap, mapIdx);

        /* Data Access - Pointer or Direct*/
        isPointer = ((uint8_T)rtwCAPI_GetDataIsPointer(dataTypeMap, mapIdx));

        /* Address */
        mapIdx = rtwCAPI_GetSignalAddrIdx(signals, i);

        rtwCAPI_GetSigAddrFromMap(isPointer, sigComplexity, sigDataType,
                                  sigDataAddr, sigIdx, mapIdx, dataAddrMap);

        /* CrossingModelBoundary */
        sigCrossMdlRef[*sigIdx] = crossingModel;

        ++(*sigIdx);
    }

  EXIT_POINT:
    utFree(blockPath);
    return(errstr);

} /* rtwCAPI_GetSigLogRecordInfo */


/** Function: rtwCAPI_CountSysRan ==============================================
 *   Recursive function that counts the number of non-NULL pointers in the array
 *   of system ran dwork pointers, for the given MMI and below
 */
void rtwCAPI_CountSysRan(const rtwCAPI_ModelMappingInfo *mmi,
			 int                            *count)
{
    sysRanDType **sysRan;
    int numSys;
    int nCMMI;
    int numSysRan = 0;
    int i;

    if (mmi == NULL) return;

    sysRan = rtwCAPI_GetSystemRan(mmi);
    numSys    = rtwCAPI_GetNumSystems(mmi);
    nCMMI     = rtwCAPI_GetChildMMIArrayLen(mmi);

    /* Recurse over children */
    for (i = 0; i < nCMMI; i++) {
        rtwCAPI_CountSysRan(rtwCAPI_GetChildMMI(mmi,i), &numSysRan);
    }

    /* Count number of dworks in this MMI - skip root */
    for (i = 1; i< numSys; i++) {
        if (sysRan[i] != NULL) numSysRan++;
    }

    *count += numSysRan;

} /* end rtwCAPI_CountSysRan */


/** Function: rtwCAPI_FillSysRan ===============================================
 *   Recursive function that fills in the system ran dwork pointers and their
 *   corresponding tids in the given array, for the given MMI and below.  The
 *   array to be filled in must be allocated outside
 */
void rtwCAPI_FillSysRan(const rtwCAPI_ModelMappingInfo *mmi,
			sysRanDType                    **sysRan,
			int                            *sysTid,
                        int                            *fillIdx)
{
    int       numSys;
    sysRanDType **mmiSysRan;
    int       nCMMI;
    int       *mmiSysTid;
    int       idx         = *fillIdx;
    const int *mmiConSys;
    int i;

    if (mmi == NULL) return;

    numSys      = rtwCAPI_GetNumSystems(mmi);
    mmiSysRan = rtwCAPI_GetSystemRan(mmi);
    nCMMI       = rtwCAPI_GetChildMMIArrayLen(mmi);
    mmiSysTid  = rtwCAPI_GetSystemTid(mmi);
    mmiConSys  = rtwCAPI_GetContextSystems(mmi);

    /* Recurse over children */
    for (i = 0; i < nCMMI; i++) {
        rtwCAPI_FillSysRan(rtwCAPI_GetChildMMI(mmi,i), sysRan, sysTid, &idx);
    }

    /* Populate arrays with dwork pointers and tid - skip root */
    for (i = 1; i< numSys; i++) {
        if (mmiSysRan[i] != NULL) {
            idx++;
            sysRan[idx] = mmiSysRan[i];
            sysTid[idx] = mmiSysTid[mmiConSys[i]];
        }
    }

    *fillIdx = idx;

} /* end rtwCAPI_FillSysRan */

/* LocalWords:  CAPI bpath aaa Addr mmi CSTATE DSTATE Hier tids
 */
