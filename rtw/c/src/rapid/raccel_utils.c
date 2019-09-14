/******************************************************************
 *
 *  File: raccel_utils.c
 *
 *
 *  Abstract:
 *      - provide utility functions for rapid accelerator 
 *
 * Copyright 2007-2016 The MathWorks, Inc.
 ******************************************************************/

/* INCLUDES */
#include  <stdio.h>
#include  <stdlib.h>

#include  <string.h>
#include  <math.h>
#include  <float.h>
#include  <ctype.h>
#include <setjmp.h>

/*
 * We want access to the real mx* routines in this file and not their RTW
 * variants in rt_matrx.h, the defines below prior to including simstruc.h
 * accomplish this.
 */
#include  "mat.h"
#define TYPEDEF_MX_ARRAY
#define rt_matrx_h
#include "simstruc.h"
#undef rt_matrx_h
#undef TYPEDEF_MX_ARRAY

#include "dt_info.h"
#include "rtw_capi.h"
#include "rtw_modelmap.h"
#include "common_utils.h"
#include "raccel_utils.h"
#include "slsv_diagnostic_codegen_c_api.h"

/* external variables */
extern const char  *gblParamFilename;
extern const char  *gblInportFileName;
extern const int_T gblNumToFiles;
extern FNamePair   *gblToFNamepair;
extern const int_T gblNumFrFiles;
extern FNamePair   *gblFrFNamepair;
extern int_T         gblParamCellIndex;
extern jmp_buf gblRapidAccelJmpBuf;
/* for tuning struct params using the C-API */
extern rtwCAPI_ModelMappingInfo* rt_modelMapInfoPtr;

/* global variables */
void* gblLoggingInterval = NULL;
static PrmStructData gblPrmStruct;
dl_logger_sid_t gblDiagnosticLogger = NULL;
dl_logger_sid_t gblBlockPathDB = NULL;


/*==================*
 * NON-Visible routines *
 *==================*/

/* Function: rt_FreeParamStructs ===========================================
 * Abstract:
 *      Free and NULL the fields of all 'PrmStructData' structures.
 *      PrmStructData contains an array of ParamInfo structures, with one such
 *      structure for each non-struct data type, and one such structure for 
 *      each leaf of every struct parameter. 
 */
void
rt_FreeParamStructs(PrmStructData *paramStructure)
{
    if (paramStructure != NULL)
    {

        size_t nNonStructDataTypes =
            paramStructure->nNonStructDataTypes;
        size_t nStructLeaves =
            paramStructure->nStructLeaves;
        ParamInfo *paramInfo =
            paramStructure->paramInfo;

        {
            size_t i;
            if (paramInfo != NULL)
            {
                for (i=0; i < nNonStructDataTypes+nStructLeaves; i++)
                {
                    /*
                     * Must free "stolen" parts of matrices with
                     * mxFree (they are allocated with mxCalloc).
                     */
#if defined(MX_HAS_INTERLEAVED_COMPLEX)
                    mxFree(paramInfo[i].vals);
#else
                    mxFree(paramInfo[i].rVals);
                    mxFree(paramInfo[i].iVals);
#endif
                }
                free(paramInfo);
            }
        }

        paramStructure->nTrans = 0;
        paramStructure->nNonStructDataTypes = 0;
        paramStructure->nStructLeaves = 0;
        paramStructure->paramInfo = NULL;
    }
} /* end rt_FreeParamStructs */



/* Function: rt_GetNumStructLeaves ============================
 * Abstract:
 *  Recursive function to traverse a parameter structure and return the
 *  number of leaves needing a 'paramInfo'.  Initially called by
 *  rt_GetNumStructLeavesAndNumNonStructDataTypes() whenever a struct 
 *  parameter is encountered in the rtp MATFile, then recursively called 
 *  while processing the structure to find all of the leaf elements.
 */
size_t
rt_GetNumStructLeaves(
    uint16_T dTypeMapIdx, 
    uint16_T dimMapIdx,
    uint16_T fixPtIdx)
{
    size_t locNParamStructs = 0;
    size_t  numArrayElements = 1;

    const rtwCAPI_ModelMappingInfo *mmi =
        rt_modelMapInfoPtr;
    
    const rtwCAPI_DataTypeMap *dTypeMap =
        rtwCAPI_GetDataTypeMap(mmi);
    
    uint16_T dTypeMapNumElems =
        rtwCAPI_GetDataTypeNumElements(
            dTypeMap,
            dTypeMapIdx);

    /* found either a non-struct leaf (dTypeMapNumElems == 0) or a struct leaf that is a
     * fixed-point value */
    if (dTypeMapNumElems==0 || fixPtIdx > 0) 
        return(1);

    /* found a non-fixed-point struct field; recurse */
    {
        int loopIdx;
        uint16_T elDTypeMapIdx;
        uint16_T elDimMapIdx;
        uint16_T elFixPtIdx;
        
        const rtwCAPI_ElementMap *elemMap =
            rtwCAPI_GetElementMap(mmi);
        
        uint16_T dTypeMapElemMapIdx =
            rtwCAPI_GetDataTypeElemMapIndex(
                dTypeMap,
                dTypeMapIdx);

        for (loopIdx=0;
             loopIdx < dTypeMapNumElems;
             loopIdx++)
        {
            elDTypeMapIdx =
                rtwCAPI_GetElementDataTypeIdx(
                    elemMap,
                    dTypeMapElemMapIdx+loopIdx);
            
            elDimMapIdx =
                rtwCAPI_GetElementDimensionIdx(
                    elemMap,
                    dTypeMapElemMapIdx+loopIdx);
            
            elFixPtIdx =
                rtwCAPI_GetElementFixPtIdx(
                    elemMap,
                    dTypeMapElemMapIdx+loopIdx);
            
            locNParamStructs +=
                rt_GetNumStructLeaves(
                    elDTypeMapIdx,
                    elDimMapIdx,
                    elFixPtIdx);
        }
    }

    /* at this point, locNParamStructs will contain the number of leaves in a single element of 
     * the current struct array. multiplying locNParamStructs by the number of elements in the
     * current struct array will give the total number of leaves. */
    {
        const rtwCAPI_DimensionMap *dimMap =
            rtwCAPI_GetDimensionMap(mmi);
        
        const uint_T *dimArray =
            rtwCAPI_GetDimensionArray(mmi);
        
        int numDims =
            rtwCAPI_GetNumDims(
                dimMap,
                dimMapIdx);
        
        uint_T dimArrayIdx =
            rtwCAPI_GetDimArrayIndex(
                dimMap,
                dimMapIdx);

        int loopIdx;        
        for (loopIdx=0;
             loopIdx < numDims;
             ++loopIdx)
        {
            numArrayElements *= dimArray[dimArrayIdx + loopIdx];
        }
    }
    
    return(locNParamStructs * numArrayElements);
} /* end rt_GetNumStructLeaves */




/* Function: rt_GetNumStructLeavesAndNumNonStructDataTypes ===================================
 * Abstract:
 *   Count the number of non-struct data types, and the number of leaves of all struct
 *   parameters.
 */
void
rt_GetNumStructLeavesAndNumNonStructDataTypes(
    const mxArray *paParamStructs,
    size_t *numStructLeaves,
    size_t *numNonStructDataTypes,
    const char **result)
{
    size_t nTrans = mxGetNumberOfElements(paParamStructs);

    *numStructLeaves       = 0;
    *numNonStructDataTypes = 0;

    {
        rtwCAPI_ModelMappingInfo *mmi =
            rt_modelMapInfoPtr;
        
        const rtwCAPI_BlockParameters *blkPrms =
            rtwCAPI_GetBlockParameters(mmi);
        
        const rtwCAPI_ModelParameters *mdlPrms =
            rtwCAPI_GetModelParameters(mmi);

        int loopIdx;
        for (loopIdx=0;
             loopIdx<nTrans;
             loopIdx++)
        {
            mxArray *valueMat =
                NULL;
            mxArray *structParamInfo =
                NULL;

            valueMat = mxGetField(
                paParamStructs,
                loopIdx,
                "values");
        
            /* 
             * If the value field of a parameter in the RTP is empty, count the parameter
             * as a non-struct datatype (struct parameters must have non-empty value fields).
             * For struct / bus parameters, and only for those types of parameters, 
             * rtp.parameters.values is a cell array.
             */
            if (valueMat == NULL || !mxIsStruct(valueMat))
            {
                /* new non-struct data type found */
                (*numNonStructDataTypes)++;
                continue;
            }

            /* struct parameter found; count all leaves */
            /* structParamInfo (contains c-api index) */
            structParamInfo = mxGetField(
                paParamStructs,
                loopIdx,
                "structParamInfo");
            
            if (structParamInfo == NULL)
            {
                *(result) = "The RTP entry for struct parameters must have a nonempty structParamInfo field.";
            }

            /* each struct transition contains exactly 1 parameter */
            if (mxGetN(valueMat) != 1 && mxGetN(structParamInfo) != 1)
            {
                *(result) = "Invalid rtp: only one struct parameter per data-type transition is allowed";
            }

            {
                double *pr = NULL;
                mxArray *mat = NULL;
                bool modelParam = false;
                size_t capiIdx = 0;
                uint16_T dTypeMapIdx = 0;
                uint16_T dimMapIdx = 0;
                uint16_T fixPtIdx = 0;

                /* parameter type (model or block) */
                mat = mxGetField(
                    structParamInfo,
                    0,
                    "ModelParam");
                
                if (mat == NULL)
                {
                    *(result) = "The structParamInfo field for struct parameters must have a nonempty modelParam field.";
                    return;
                }                
                pr  = mxGetPr(mat);
                modelParam = (bool)pr[0];

                /* c-api index */
                mat = mxGetField(
                    structParamInfo,
                    0,
                    "CAPIIdx");
                
                if (mat == NULL)
                {
                    *(result) = "The structParamInfo field for struct parameters must have a nonempty CAPIIndex field.";
                    return;
                }
                pr  = mxGetPr(mat);
                capiIdx = (size_t)pr[0];

                /* c-api datatype map index and dimension map index */
                dTypeMapIdx = (modelParam) ?
                    rtwCAPI_GetModelParameterDataTypeIdx(mdlPrms, capiIdx) :
                    rtwCAPI_GetBlockParameterDataTypeIdx(blkPrms, capiIdx);

                dimMapIdx = (modelParam) ?
                    rtwCAPI_GetModelParameterDimensionIdx(mdlPrms, capiIdx) :
                    rtwCAPI_GetBlockParameterDimensionIdx(blkPrms, capiIdx);
                
                fixPtIdx = (modelParam) ? 
                    rtwCAPI_GetModelParameterFixPtIdx(mdlPrms, capiIdx) :
                    rtwCAPI_GetBlockParameterFixPtIdx(blkPrms, capiIdx);

                (*numStructLeaves) += rt_GetNumStructLeaves(dTypeMapIdx, dimMapIdx, fixPtIdx);
            }
        }
    }
} /* end rt_GetNumStructLeavesAndNumNonStructDataTypes */



/* Function: rt_CreateParamInfoForNonStructDataType ===========================
 * Abstract:
 *  Create a ParamInfo struct for a non-struct data type.
 */
void
rt_CreateParamInfoForNonStructDataType(
    mxArray* mat, 
    const mxArray* paParamStructs,   
    const size_t paramStructIndex,
    PrmStructData *paramStructure,
    size_t paramInfoIndex)
{
    ParamInfo *paramInfo = NULL;
    double *pr = NULL;
    
    paramInfo = &paramStructure->paramInfo[paramInfoIndex];
    paramInfo->structLeaf = false;
    paramInfo->elSize = mxGetElementSize(mat);
    paramInfo->nEls   = mxGetNumberOfElements(mat);

#if defined(MX_HAS_INTERLEAVED_COMPLEX)
    paramInfo->vals  = mxGetData(mat);
    mxSetData(mat,NULL);
#else
    paramInfo->rVals  = mxGetData(mat);
    mxSetData(mat,NULL);

    if (mxIsNumeric(mat))
    {
        paramInfo->iVals  = mxGetImagData(mat);
        mxSetImagData(mat,NULL);
    }  
#endif

    /* Grab the datatype id. */
    mat = mxGetField(
        paParamStructs,
        paramStructIndex,
        "dataTypeId");
    
    pr  = mxGetPr(mat);
    paramInfo->dataType = (int)pr[0];

    /* Grab the complexity. */
    mat = mxGetField(
        paParamStructs,
        paramStructIndex,
        "complex");
    
    pr  = mxGetPr(mat);
    paramInfo->complex = (bool)pr[0];

    /* Grab the dtTransIdx */
    mat = mxGetField(
        paParamStructs,
        paramStructIndex,
        "dtTransIdx");
    
    pr  = mxGetPr(mat);
    paramInfo->dtTransIdx = (int)pr[0];
} /* end rt_CreateParamInfoForNonStructDataType */




/* Function: rt_CopyStructFromStructArray ======================================
 * Abstract:
 *   Given an mxArray of that is an array of structs, and given an index, 
 *   create a copy of the struct in the array that's at the given index.  
 *   This function assumes that the argument structArray is an mxStruct.
 */
void
rt_CopyStructFromStructArray(
    mxArray *structArray,
    mxArray **destination,
    size_t structArrayIndex)
{
    int numFields
        = mxGetNumberOfFields(structArray);
    
    const char **fieldNames =
        (const char **) malloc(sizeof(const char *) * numFields);
    
    {
        int fieldIndex;
        for (fieldIndex = 0;
             fieldIndex < numFields;
             ++fieldIndex)
        {
            fieldNames[fieldIndex] =
                mxGetFieldNameByNumber(
                    structArray,
                    fieldIndex);
        }

        *destination = mxCreateStructMatrix(
            1,
            1,
            numFields,
            fieldNames);
    
        for (fieldIndex = 0;
             fieldIndex < numFields;
             ++fieldIndex)
        {
            mxArray *field = mxGetField(
                structArray, 
                structArrayIndex, 
                fieldNames[fieldIndex]);
            
            mxSetField(
                *destination, 
                0, 
                fieldNames[fieldIndex],
                field);
        }
    }
}


/* Function: rt_GetStructLeafInfo ================================================
 * Abstract:
 *  Recursive function to traverse the fields of a single struct parameter with
 *  value valueMat, and to create a ParamInfo structure for each leaf of the
 *  parameter.
 */
void
rt_GetStructLeafInfo(
    uint16_T dTypeMapIdx,
    uint16_T fixPtIdx,
    void *baseAddr,
    mxArray *valueMat,
    boolean_T modelParam,
    PrmStructData *paramStructure,
    size_t *paramInfoIndex,
    const char** result)
{
    rtwCAPI_ModelMappingInfo  *mmi =
        rt_modelMapInfoPtr;
    
    rtwCAPI_DataTypeMap const *dTypeMap
        = rtwCAPI_GetDataTypeMap(mmi);
    
    uint16_T dTypeMapNumElems =
        rtwCAPI_GetDataTypeNumElements(
            dTypeMap,
            dTypeMapIdx);

    /* fixPtIdx will be > 0 if and only if the parameter currently being inspected is a fixed-point
     * parameter. */
    if (dTypeMapNumElems == 0 || fixPtIdx > 0)
    {
        /* This is a leaf of a struct. Increment paramInfoIndex. */
        ParamInfo *paramInfo = &paramStructure->paramInfo[(*paramInfoIndex)++];
        paramInfo->structLeaf = true;
        paramInfo->modelParam = modelParam;
        paramInfo->dataType = dTypeMapIdx;
        paramInfo->complex = (boolean_T) rtwCAPI_GetDataIsComplex(dTypeMap, dTypeMapIdx);
        paramInfo->prmAddr = baseAddr;

        /* Grab the data and any attributes.  We "steal" the data from the mxArray. */
        if (valueMat)
        {
            paramInfo->elSize = mxGetElementSize(valueMat);
            paramInfo->nEls   = mxGetNumberOfElements(valueMat);
#if defined(MX_HAS_INTERLEAVED_COMPLEX)
            paramInfo->vals  = mxGetData(valueMat);
            mxSetData(valueMat,NULL);
#else
            paramInfo->rVals  = mxGetData(valueMat);
            mxSetData(valueMat,NULL);

            if (mxIsNumeric(valueMat))
            {
                paramInfo->iVals  = mxGetImagData(valueMat);
                mxSetImagData(valueMat,NULL);
            }
#endif
        } else {
            paramInfo->nEls   = 0;
            paramInfo->elSize = 0;
#if defined(MX_HAS_INTERLEAVED_COMPLEX)
            paramInfo->vals  = NULL;
#else
            paramInfo->rVals  = NULL;
            paramInfo->iVals  = NULL;
#endif
        }

        return;
    } else {
        /* This is a struct, possibly a struct array; recurse over its fields */     
        uint16_T dTypeMapElemMapIdx =
            rtwCAPI_GetDataTypeElemMapIndex(
                dTypeMap,
                dTypeMapIdx);
        
        rtwCAPI_ElementMap const *elemMap =
            rtwCAPI_GetElementMap(mmi);
        
        rtwCAPI_DimensionMap const *dimMap =
            rtwCAPI_GetDimensionMap(mmi);

        {
            int loopIdx;
            for (loopIdx=0;
                 loopIdx < dTypeMapNumElems;
                 loopIdx++)
            {       
                uint16_T elDTypeMapIdx =
                    rtwCAPI_GetElementDataTypeIdx(
                        elemMap,
                        dTypeMapElemMapIdx+loopIdx);
                
                uint32_T elOffset =
                    rtwCAPI_GetElementOffset(
                        elemMap,
                        dTypeMapElemMapIdx+loopIdx);
                
                uint16_T elFixPtIdx =
                    rtwCAPI_GetElementFixPtIdx(
                        elemMap,
                        dTypeMapElemMapIdx+loopIdx);
                
                uint16_T elDimMapIdx =
                    rtwCAPI_GetElementDimensionIdx(
                        elemMap,
                        dTypeMapElemMapIdx+loopIdx);
                
                const char_T* fieldName =
                    rtwCAPI_GetElementName(
                        elemMap,
                        dTypeMapElemMapIdx+loopIdx);
                
                mxArray *subMat =
                    mxGetField(
                        valueMat,
                        0,
                        fieldName);
                
                uint8_T elNumDims =
                    rtwCAPI_GetNumDims(
                        dimMap,
                        elDimMapIdx);
                
                uint_T elDimArrayIdx =
                    rtwCAPI_GetDimArrayIndex(
                        dimMap,
                        elDimMapIdx);
                
                const uint_T *dimArray =
                    rtwCAPI_GetDimensionArray(mmi);
                
                uint16_T elNumElems =
                    rtwCAPI_GetDataTypeNumElements(
                        dTypeMap,
                        elDTypeMapIdx);
                
                uint_T numArrayElements = 1;
                int dimensionLoopCounter;

                for (dimensionLoopCounter=0;
                     dimensionLoopCounter < elNumDims;
                     ++dimensionLoopCounter)
                {
                    numArrayElements *= dimArray[elDimArrayIdx + dimensionLoopCounter];
                }
            
                if (numArrayElements == 1 ||
                    elFixPtIdx > 0 ||
                    elNumElems == 0)
                {
                    /* this field is either not a struct, or is a 1x1 struct array */
                    rt_GetStructLeafInfo(
                        elDTypeMapIdx, 
                        elFixPtIdx,
                        (unsigned char *) baseAddr+elOffset,
                        subMat,
                        modelParam,
                        paramStructure,
                        paramInfoIndex,
                        result);
                } else {
                    /* non 1x1 struct array */
                    void* localBaseAddr =
                        baseAddr;

                    size_t arrayIdx;
                    for (arrayIdx=0; arrayIdx < numArrayElements; ++arrayIdx)
                    {
                        mxArray* subMatElement =
                            NULL;
                        
                        uint16_T elStructSize  =
                            rtwCAPI_GetDataTypeSize(
                                dTypeMap,
                                elDTypeMapIdx);
                        
                        rt_CopyStructFromStructArray(
                            subMat,
                            &subMatElement,
                            arrayIdx);
                        
                        rt_GetStructLeafInfo(
                            elDTypeMapIdx, 
                            elFixPtIdx, 
                            (unsigned char *)localBaseAddr+elOffset,
                            subMatElement,
                            modelParam,
                            paramStructure,
                            paramInfoIndex,
                            result);

                        localBaseAddr = (void*)((char*)localBaseAddr + elStructSize);                    
                    }
                }
            }
        }
    }
} /* end rt_GetStructLeafInfo */



/* Function: rt_GetInfoFromOneStructParam ==================================
 * Abstract:
 *   Create ParamInfo structures for each leaf of a struct parameter. 
 */
void
rt_GetInfoFromOneStructParam(
    mxArray *valueMat,
    PrmStructData *paramStructure,
    size_t *paramInfoIndex,
    boolean_T modelParam,
    int capiIdx,
    const char** result)
{
    if (valueMat == NULL)
    {
        *(result) = "the value field of an RTP entry for some struct param is empty (rt_GetInfoFromOneStructParam)";
        return;
    }

    if (!mxIsStruct(valueMat))
    {
        /* This function should deal only with struct params */
        *(result) = "non-struct param found where only struct params were expected (rt_GetInfoFromOneStructParam)";
        return;
    } else {
        uint16_T structSizeInBytes;
        uint16_T fixPtIdx;
        uint16_T dTypeMapIdx;
        uint16_T dimMapIdx;
        
        int addrIdx = -1;
        void *baseAddr = NULL;
        
        rtwCAPI_ModelMappingInfo *mmi =
            rt_modelMapInfoPtr;
        
        rtwCAPI_BlockParameters const *blkPrms =
            rtwCAPI_GetBlockParameters(mmi);
        
        rtwCAPI_ModelParameters const *mdlPrms =
            rtwCAPI_GetModelParameters(mmi);
        
        void ** addrMap =
            rtwCAPI_GetDataAddressMap(mmi);
        
        const rtwCAPI_DataTypeMap *dTypeMap =
            rtwCAPI_GetDataTypeMap(mmi);

        addrIdx  = (modelParam) ?
            rtwCAPI_GetModelParameterAddrIdx(mdlPrms, capiIdx) :
            rtwCAPI_GetBlockParameterAddrIdx(blkPrms, capiIdx);
        
        baseAddr = rtwCAPI_GetDataAddress(
            addrMap,
            addrIdx);

        dTypeMapIdx = (modelParam) ?
            rtwCAPI_GetModelParameterDataTypeIdx(mdlPrms, capiIdx) :
            rtwCAPI_GetBlockParameterDataTypeIdx(blkPrms, capiIdx);

        dimMapIdx = (modelParam) ?
            rtwCAPI_GetModelParameterDimensionIdx(mdlPrms, capiIdx) :
            rtwCAPI_GetBlockParameterDimensionIdx(blkPrms, capiIdx);

        structSizeInBytes = rtwCAPI_GetDataTypeSize(
            dTypeMap,
            dTypeMapIdx);

        fixPtIdx = modelParam ?
            rtwCAPI_GetModelParameterFixPtIdx(mdlPrms, capiIdx) : 
            rtwCAPI_GetBlockParameterFixPtIdx(blkPrms, capiIdx);
        
        if (fixPtIdx != 0)
        {
            *(result) = "Fixed-point index of struct params should be 0. (rt_GetInfoFromOneStructParam)";
            return;
        }

        /* Recursively traverse the struct param, creating a new ParamInfo structure for each leaf of the parameter. */
        {
            size_t numArrayElements =
                mxGetNumberOfElements(valueMat);
            
            if (numArrayElements == 1)
            {
                rt_GetStructLeafInfo(
                    dTypeMapIdx, 
                    fixPtIdx,
                    baseAddr,
                    valueMat,
                    modelParam,
                    paramStructure,
                    paramInfoIndex,
                    result);
            } else {
                void* localBaseAddr =
                    baseAddr;

                int loopIdx;
                for (loopIdx = 0;
                     loopIdx < numArrayElements;
                     loopIdx++)
                {
                    mxArray* valueMatElement = NULL;
                
                    rt_CopyStructFromStructArray(
                        valueMat,
                        &valueMatElement, 
                        loopIdx);

                    rt_GetStructLeafInfo(
                        dTypeMapIdx, 
                        fixPtIdx,
                        localBaseAddr,
                        valueMatElement,
                        modelParam,
                        paramStructure,
                        paramInfoIndex,
                        result);

                    localBaseAddr = (void*)((char*)localBaseAddr+ structSizeInBytes);
                }
            }
        }
    }
}  /* end rt_GetInfoFromOneStructParam */


/* Function: rt_PrepareToCreateParamInfosForStructParam ===========================
 * Abstract:
 *  Create a ParamInfo struct for a non-struct data type.
 */
const char *
rt_PrepareToCreateParamInfosForStructParam(
    mxArray* valueMat, 
    const mxArray* paParamStructs,
    size_t paramStructIndex,
    PrmStructData *paramStructure,
    size_t *paramInfoIndex)
{
    const mxArray *structParamInfo = NULL;
    const mxArray *temp = NULL;                
    int capiIndex = -1;
    int structParamInfoIndex = 0;
    double *pr = NULL;
    boolean_T modelParam = false;
    const char *result = NULL;

    if (valueMat == NULL)
    {
        return result;
    } else if (!mxIsStruct(valueMat)) {
        result = "A non-struct value was found in the rtp where a struct value was expected.";
        return result;
    }

    structParamInfo = mxGetField(
        paParamStructs,
        paramStructIndex,
        "structParamInfo");

    if (structParamInfo == NULL)
    {
        result = "Some struct parameter has an empty structParamInfo field.";
        return result;
    }

    if (mxGetN(valueMat) != 1 && mxGetN(structParamInfo) != 1)
    {
        result = "Invalid rtp: only one struct parameter per data-type transition is allowed";
        return result;
    }
                
    temp = mxGetField(structParamInfo, structParamInfoIndex, "ModelParam");
    if (temp)
    {
        pr = mxGetPr(temp);
        modelParam = (boolean_T)pr[0];
    } else {
        modelParam = 0;
    }

    temp = mxGetField(structParamInfo, structParamInfoIndex, "CAPIIdx");
    if (temp)
    {
        pr = mxGetPr(temp);
        capiIndex = (int) pr[0];
    } else {
        capiIndex = -1;
    }             

    rt_GetInfoFromOneStructParam(
        valueMat,
        paramStructure,
        paramInfoIndex,
        modelParam,
        capiIndex,
        &result);

    if (result != NULL) 
        return result;

    return result;
} /* end rt_PrepareToCreateParamInfosForStructParam */



/* Function: rt_ReadParamStructMatFile=======================================
 * Abstract:
 *  Reads a matfile containing a new parameter structure.  It also reads the
 *  model checksum and compares this with the RTW generated code's checksum
 *  before inserting the new parameter structure.
 *
 * Returns:
 *	NULL    : success
 *	non-NULL: error string
 */
const char *
rt_ReadParamStructMatFile(
    PrmStructData **prmStructOut,
    const SimStruct * S,
    int cellParamIndex)
{
    size_t nTrans = 0;
    MATFile *pmat = NULL;
    mxArray *pa = NULL;
    const mxArray *paParamStructs = NULL;
    PrmStructData *paramStructure = NULL;
    const char *result = NULL; /* assume success */

    paramStructure = &gblPrmStruct;

    /**************************************************************************
     * Open parameter MAT-file, read checksum, swap rtP data for type Double *
     **************************************************************************/

    if ((pmat=matOpen(gblParamFilename,"r")) == NULL)
    {
        result = "could not find MAT-file containing new parameter data";
        goto EXIT_POINT;
    }

    /*
     * Read the param variable. The variable name must be passed in
     * from the generated code.
     */
    if ((pa=matGetNextVariable(pmat,NULL)) == NULL )
    {
        result = "error reading RTP from MAT-file "
            "(matGetNextVariable)";
        goto EXIT_POINT;
    }

    /* Should be 1x1 structure */
    if (!mxIsStruct(pa) ||
        mxGetM(pa) != 1 ||
        mxGetN(pa) != 1 )
    {
        result = "RTP must be a 1x1 structure";
        goto EXIT_POINT;
    }

    /* look for modelChecksum field */
    {
        const double  *newChecksum;
        const mxArray *paModelChecksum;

        if ((paModelChecksum = mxGetField(pa, 0, "modelChecksum")) == NULL)
        {
            result = "parameter variable must contain a modelChecksum field";
            goto EXIT_POINT;
        }

        /* check modelChecksum field */
        if (!mxIsDouble(paModelChecksum) ||
            mxIsComplex(paModelChecksum) ||
            mxGetNumberOfDimensions(paModelChecksum) > 2 ||
            mxGetM(paModelChecksum) < 1 ||
            mxGetN(paModelChecksum) !=4 )
        {
            result = "invalid modelChecksum in parameter MAT-file";
            goto EXIT_POINT;
        }

        newChecksum = mxGetPr(paModelChecksum);

        paramStructure->checksum[0] = newChecksum[0];
        paramStructure->checksum[1] = newChecksum[1];
        paramStructure->checksum[2] = newChecksum[2];
        paramStructure->checksum[3] = newChecksum[3];
    }

    /* be sure checksums all match */
    if (paramStructure->checksum[0] != ssGetChecksum0(S) ||
        paramStructure->checksum[1] != ssGetChecksum1(S) ||
        paramStructure->checksum[2] != ssGetChecksum2(S) ||
        paramStructure->checksum[3] != ssGetChecksum3(S) )
    {
        result = "model checksum mismatch - incorrect parameter data "
            "specified";
        goto EXIT_POINT;
    }


    /*
     * Get the "parameters" field from the structure.  It is an
     * array of structures.
     */
    if ((paParamStructs = mxGetField(pa, 0, "parameters")) == NULL)
    {
        goto EXIT_POINT;
    }

    /*
     * If the parameters field is a cell array then pick out the cell
     * array pointed to by the cellParamIndex
     */
    if ( mxIsCell(paParamStructs) )
    {
        /* check that cellParamIndex is in range */
        size_t size =
            mxGetM(paParamStructs) * mxGetN(paParamStructs);
        
        if (cellParamIndex > 0 &&
            cellParamIndex <= (int) size)
        {
            paParamStructs = mxGetCell(
                paParamStructs,
                cellParamIndex-1);
        } else{
            result = "Invalid index into parameter cell array";
            goto EXIT_POINT;
        }
        
        if (paParamStructs == NULL)
        {
            result = "Invalid parameter field in parameter structure";
            goto EXIT_POINT;
        }
    }

    /* the number of data-types in the RTP */
    nTrans = mxGetNumberOfElements(paParamStructs);
    if (nTrans == 0) goto EXIT_POINT;

    /*
     * Validate the array fields - only check the first element of the
     * array since all elements of a structure array have the same
     * fields.
     *
     * It is assumed that if the proper data fields exists, that the
     * data is correct.
     */
    {
        mxArray *dum;

        if ((dum = mxGetField(paParamStructs, 0, "dataTypeName")) == NULL)
        {
            result = "parameters struct must contain a dataTypeName field";
            goto EXIT_POINT;
        }

        if ((dum = mxGetField(paParamStructs, 0, "dataTypeId")) == NULL)
        {
            result = "parameters struct must contain a dataTypeId field";
            goto EXIT_POINT;
        }

        if ((dum = mxGetField(paParamStructs, 0, "complex")) == NULL)
        {
            result = "parameters struct must contain a complex field";
            goto EXIT_POINT;
        }

        if ((dum = mxGetField(paParamStructs, 0, "dtTransIdx")) == NULL)
        {
            result = "parameters struct must contain a dtTransIdx field";
            goto EXIT_POINT;
        }
    }

    /* 
     * Calculate the total number of ParamInfo structures needed. Each non-struct
     * data type is given one ParamInfo, and each leaf of every struct 
     * parameter is given one ParamInfo.
     */ 
    {
        size_t nStructLeaves = 0;
        size_t nNonStructDataTypes = 0;
        size_t nParamInfos = 0;
        
        rt_GetNumStructLeavesAndNumNonStructDataTypes(
            paParamStructs, 
            &nStructLeaves,
            &nNonStructDataTypes,
            &result);

        if (result != NULL) goto EXIT_POINT;

        paramStructure->nTrans =
            nTrans;
        paramStructure->nStructLeaves =
            nStructLeaves;
        paramStructure->nNonStructDataTypes =
            nNonStructDataTypes;

        /*
         * Allocate the ParamInfo's.
         * The total number of ParamInfos needed is nStructLeaves + nNonStructDataTypes,
         */
        nParamInfos =
            nStructLeaves + nNonStructDataTypes;
        
        paramStructure->paramInfo =
            (ParamInfo *) calloc(nParamInfos, sizeof(ParamInfo));
    }
    
    if (paramStructure->paramInfo == NULL)
    {
        result = "Memory allocation error";
        goto EXIT_POINT;
    }

    /* Get the new parameter data for each data type. */
    {
        size_t paramStructIndex = 0;
        size_t paramInfoIndex   = 0;

        for (paramStructIndex=0;
             paramStructIndex < nTrans;
             paramStructIndex++) 
        {
            mxArray *mat = NULL;
            
            mat = mxGetField(
                paParamStructs,
                paramStructIndex,
                "values");

            if (mat == NULL)
            {
                ParamInfo *paramInfo =
                    &paramStructure->paramInfo[paramInfoIndex];
                paramInfo->nEls = 0;
                paramInfo->elSize = 0;
#if defined(MX_HAS_INTERLEAVED_COMPLEX)
                paramInfo->vals = NULL;
#else
                paramInfo->rVals = NULL;
                paramInfo->iVals = NULL;
#endif
                paramInfoIndex++;
                continue;
            } 
      
            if (!mxIsStruct(mat))
            {
                /* Non-struct data type */
                rt_CreateParamInfoForNonStructDataType(
                    mat,
                    paParamStructs,
                    paramStructIndex,
                    paramStructure, 
                    paramInfoIndex
                    );
                
                paramInfoIndex++;
            } else {
                /* This is a struct param; grab information from rtp.parameters(i).structParamInfo 
                 * paramInfoIndex is incremented inside rt_CreateParamInfosForStructOrBusType
                 */
                result = rt_PrepareToCreateParamInfosForStructParam(
                    mat, 
                    paParamStructs,
                    paramStructIndex,
                    paramStructure,
                    &paramInfoIndex);
            }
        }
    } 

EXIT_POINT:
    mxDestroyArray(pa);

    if (pmat != NULL)
    {
        matClose(pmat); pmat = NULL;
    }

    if (result != NULL)
    {
        rt_FreeParamStructs(paramStructure);
        paramStructure = NULL;
    }
    
    *prmStructOut = paramStructure;
    return(result);
} /* end rt_ReadParamStructMatFile */

/* Function: ReplaceRtP ========================================================
 * Abstract
 *  Initialize the rtP structure using the parameters from the specified
 *  'paramStructure'.  The 'paramStructure' contains parameter info that was
 *  read from a mat file (see raccel_mat.c/rt_ReadParamStructMatFile).
 */
static const char *
ReplaceRtP(
    const SimStruct *S,
    const PrmStructData *paramStructure)
{
    const char *errStr = NULL;
    const ParamInfo *paramInfo =
        paramStructure->paramInfo;
    size_t nStructLeaves =
        paramStructure->nStructLeaves;
    size_t nNonStructDataTypes =
        paramStructure->nNonStructDataTypes;
    const DataTypeTransInfo *dtInfo =
        (const DataTypeTransInfo *)ssGetModelMappingInfo(S);
    DataTypeTransitionTable *dtTable =
        dtGetParamDataTypeTrans(dtInfo);
    uint_T *dataTypeSizes =
        dtGetDataTypeSizes(dtInfo);
    rtwCAPI_ModelMappingInfo *mmi =
        rt_modelMapInfoPtr;
    rtwCAPI_DataTypeMap const *dTypeMap =
        rtwCAPI_GetDataTypeMap(mmi);

    {
        size_t loopIdx;
        for (loopIdx=0;
             loopIdx < nStructLeaves+nNonStructDataTypes;
             loopIdx++)
        {
            bool structLeaf =
                paramInfo[loopIdx].structLeaf;
            bool complex =
                (bool)paramInfo[loopIdx].complex;        
            int dtTransIdx =
                paramInfo[loopIdx].dtTransIdx;
            int dataType =
                paramInfo[loopIdx].dataType;
            int dtSize = 0;
            size_t nEls = 0;
            size_t elSize = 0;
            size_t nParams = 0;
            char *address = NULL;

#if !defined(MX_HAS_INTERLEAVED_COMPLEX)
            char *dst = NULL;
#endif

            dtSize = structLeaf ?
                rtwCAPI_GetDataTypeSize(dTypeMap, dataType) :
                (int)dataTypeSizes[dataType];

            /*
             * The datatype-size table (dataTypeSizes) contains only real data types 
             * whereas the c-api includes both real and complex datatypes (cint32_T, for 
             * example). If dtSize was obtained from the c-api, then it must be divided
             * by two for nParams to be correct.
             */
            dtSize = (complex && structLeaf) ?
                (dtSize / 2) :
                dtSize;

#if defined(MX_HAS_INTERLEAVED_COMPLEX)
            if(complex) { dtSize*= 2; } /* Added for complex-interleaved conversion. */
#endif

            nEls = paramInfo[loopIdx].nEls;
            elSize = paramInfo[loopIdx].elSize;
            nParams = (elSize*nEls)/dtSize;

            if (!nEls)
                continue;

            if (!structLeaf)
            {
                address = dtTransGetAddress(
                    dtTable,
                    dtTransIdx);
                /*
                 * Check for consistent element size.  paramInfo->elSize is the size
                 * as stored in the parameter mat-file.  This should match the size
                 * used by the generated code (i.e., stored in the SimStruct).
                 */
                if ((dataType <= 13 && elSize != dtSize) ||
                    (dataType > 13 && (dtSize % elSize != 0)))
                {
                    errStr = "Parameter data type sizes in MAT-file not same "
                        "as data type sizes in RTW generated code";
                    goto EXIT_POINT;
                }
            } else{
                address = paramInfo[loopIdx].prmAddr;
            }

#if defined(MX_HAS_INTERLEAVED_COMPLEX)            
            (void)memcpy(
                address,
                paramInfo[loopIdx].vals,
                nParams*dtSize);
#else
            if (!complex)
            {
                (void)memcpy(
                    address,
                    paramInfo[loopIdx].rVals,
                    nParams*dtSize);
            } else {
                /*
                 * Must interleave the real and imaginary parts.  Simulink style.
                 */
                size_t elIdx;
                const char *realSrc =
                    (const char *)paramInfo[loopIdx].rVals;
                const char *imagSrc =
                    (const char *)paramInfo[loopIdx].iVals;
                dst = address;

                for (elIdx=0;
                     structLeaf ?
                         elIdx<nEls :
                         elIdx<nParams;
                     elIdx++)
                {
                    /* Copy real part. */
                    (void)memcpy(dst,realSrc,dtSize);
                    dst += dtSize;
                    realSrc += dtSize;

                    /* Copy imag part. */
                    (void)memcpy(dst,imagSrc,dtSize);
                    dst += dtSize;
                    imagSrc += dtSize;
                }
            }
#endif
        }        
    }        

  EXIT_POINT:
    return(errStr);
} /* end ReplaceRtP */


/*==================*
 * Visible routines *
 *==================*/

/* Function: rt_RapidReadMatFileAndUpdateParams ========================================
 *
 */
void
rt_RapidReadMatFileAndUpdateParams(const SimStruct *S)
{
    const char* result = NULL;
    PrmStructData* paramStructure = NULL;

    if (gblParamFilename == NULL)
        goto EXIT_POINT;

    /* checksum comparison is performed in rt_ReadParamStructMatFile */
    result = rt_ReadParamStructMatFile(
        &paramStructure,
        S,
        gblParamCellIndex);
    
    if (result != NULL)
        goto EXIT_POINT;

    /* Replace the rtP structure */
    result = ReplaceRtP(
        S,
        paramStructure);
    
    if (result != NULL)
        goto EXIT_POINT;

  EXIT_POINT:
    if (paramStructure != NULL)
    {
        rt_FreeParamStructs(paramStructure);
    }

    if (result)
    {
        ssSetErrorStatus(S, result);
    }
    
    return;

} /* rt_RapidReadMatFileAndUpdateParams */


void rt_ssGetBlockPath(SimStruct* S, int_T sysIdx, int_T blkIdx, char_T **path) {
    (void)(S);
    if (!gblBlockPathDB)
    {
        gblBlockPathDB = dl_init_objpath(1000);
    }
    *path = dl_get_object_path(gblBlockPathDB, sysIdx, blkIdx);
}
void rt_ssSet_slErrMsg(SimStruct* S, void* diag) {
    if (gblDiagnosticLogger)
    {
        dl_report_from_diagnostic(gblDiagnosticLogger, CODEGEN_SUPPORT_DIAGNOSTIC_ERROR, diag);
    }
    else 
    {
        _ssSet_slErrMsg(S, diag);
    }
    longjmp(gblRapidAccelJmpBuf, 1);
}
void rt_ssReportDiagnosticAsWarning(SimStruct* S, void* diag) {
    if (gblDiagnosticLogger)
    {
        dl_report_from_diagnostic(gblDiagnosticLogger, CODEGEN_SUPPORT_DIAGNOSTIC_WARNING, diag);
    }
    else 
    {
        _ssReportDiagnosticAsWarning(S, diag);
    }
}


void rt_RapidInitDiagLoggerDB(const char* dbhome, size_t sid)
{
    dl_set_home(dbhome);
    gblDiagnosticLogger = dl_init(sid);
}


extern void rt_RapidReleaseDiagLoggerDB()
{
    if (gblDiagnosticLogger)
    {
        dl_reset(gblDiagnosticLogger);
        gblDiagnosticLogger = NULL;
    }
}

/* EOF raccel_utils.c */

/* LocalWords:  RSim matrx smaple matfile rb scaler Tx gbl tu Datato TUtable
 * LocalWords:  UTtable FName FFname raccel el fromfile npts npoints nchannels
 * LocalWords:  curr tfinal timestep Gbls Remappings DType rtp CAPI rsim
 */

