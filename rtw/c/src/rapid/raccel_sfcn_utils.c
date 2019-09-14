/******************************************************************
 *  File: raccel_sfcn_utils.c
 *
 *  Abstract:
 *      - functions for dynamically loading s-function mex files
 *
 * Copyright 2007-2017, The MathWorks, Inc.
 ******************************************************************/

#include  <stdio.h>
#include  <stdlib.h>
#include  <stdarg.h>
#include  <string.h>
#include  <math.h>
#include  <float.h>
#include  <ctype.h>
#include  <setjmp.h>

#ifdef _WIN32
#include <windows.h>
#else
#include  <dlfcn.h>
#endif

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

#include "sfcn_loader_c_api.h"
#include "raccel_sfcn_utils.h"
#include "dt_info.h"
#include "sl_datatype_access.h"

extern jmp_buf gblRapidAccelJmpBuf;
extern const char* gblSFcnInfoFileName;
extern const char* gblErrorStatus;

/* ////////////////////////////////////////////////////////////////////////////////////////// */
/* long jumper */
void
raccelSFcnLongJumper(const char* errMsg)
{
    gblErrorStatus = errMsg;
    
    longjmp(gblRapidAccelJmpBuf, 1);
}

/* ////////////////////////////////////////////////////////////////////////////////////////// */
/* mex api handling */
void
raccelSFcnMexHandler(const char* mexFcn)
{
    const char* name =
        sfcnLoader_getCurrentSFcnName();

    const char* errTemplate =
        "<diag_root><diag id=\"Simulink:tools:rapidAccelSFcnMex\"><arguments><arg type = \"string\">%s</arg><arg type = \"string\">%s</arg></arguments></diag></diag_root>";                

    char* message =
        (char *) calloc(1024, sizeof(char));
    
    sprintf(
        message,
        errTemplate,
        name,
        mexFcn
        );

    raccelSFcnLongJumper((const char*) message);
}

bool
raccelMexIsLocked()
{
    raccelSFcnMexHandler("mexIsLocked");
    return false;
}

int
raccelMexPutVar(
    const char* c1,
    const char* c2,
    const mxArray* mx)
{
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(c2);
    UNUSED_PARAMETER(mx);
    raccelSFcnMexHandler("mexPutVar");
    return 0;
}

const mxArray *
raccelMexGetVarPtr(
    const char* c1,
    const char* c2)
{
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(c2);
    raccelSFcnMexHandler("mexGetVarPtr");
    return NULL;    
}

mxArray *
raccelMexGetVar(
    const char* c1,
    const char* c2)
{
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(c2);
    raccelSFcnMexHandler("mexGetVar");
    return NULL;    
}

void
raccelMexLock()
{
    raccelSFcnMexHandler("mexLock");
}

void
raccelMexUnlock()
{
    raccelSFcnMexHandler("mexUnlock");
}

const char *
raccelMexFunctionName()
{
    raccelSFcnMexHandler("mexFunctionName");
    return NULL;
}

int
raccelMexEvalString(
    const char * c1)
{
    UNUSED_PARAMETER(c1);    
    raccelSFcnMexHandler("mexEvalString");
    return 0;
}

mxArray *
raccelMexEvalStringWithTrap(
    const char * c1)
{
    UNUSED_PARAMETER(c1);    
    raccelSFcnMexHandler("mexEvalStringWithTrap");
    return NULL;
}

int
raccelMexSet(
    double d,
    const char * c,
    mxArray * m)
{
    UNUSED_PARAMETER(d);    
    UNUSED_PARAMETER(c);    
    UNUSED_PARAMETER(m);    
    raccelSFcnMexHandler("mexSet");
    return 0;
}

const mxArray *
raccelMexGet(
    double d,
    const char * c)
{
    UNUSED_PARAMETER(d);    
    UNUSED_PARAMETER(c);
    raccelSFcnMexHandler("mexGet");
    return NULL;
}

void
raccelMexSignalDebugger()
{
    raccelSFcnMexHandler("mexSignalDebugger");
}

int
raccelMexCallMatlab(
    int n1,
    mxArray* m1[],
    int n2,
    mxArray* m2[],
    const char* c,
    bool b)
{
    UNUSED_PARAMETER(n1);
    UNUSED_PARAMETER(m1);
    UNUSED_PARAMETER(n2);
    UNUSED_PARAMETER(m2);
    UNUSED_PARAMETER(c);
    UNUSED_PARAMETER(b);
    raccelSFcnMexHandler("mexCallMATLAB");
    return 0;
}

mxArray*
raccelMexCallMatlabWithTrap(
    int n1,
    mxArray* m1[],
    int n2,
    mxArray* m2[],
    const char* c)
{
    UNUSED_PARAMETER(n1);
    UNUSED_PARAMETER(m1);
    UNUSED_PARAMETER(n2);
    UNUSED_PARAMETER(m2);
    UNUSED_PARAMETER(c);
    raccelSFcnMexHandler("mexCallMATLABWithTrap");
    return NULL;
}

mxArray *
raccelMexCreateSimpleFunctionHandle(
    mxFunctionPtr f)
{
    UNUSED_PARAMETER(f);
    raccelSFcnMexHandler("mexCreateSimpleFunctionHandle");
    return NULL;    
}

int
raccelMexAtExit()
{
    raccelSFcnMexHandler("mexAtExit");
    return 0;
}

void
raccelMexErrMsgIdAndTxt(
    const char* c1,
    const char* c2,
    va_list v
    )
{
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(c2);
    UNUSED_PARAMETER(v);
    raccelSFcnMexHandler("mexErrMsgIdAndTxt");    
}

void
raccelMexErrMsgTxt(
    const char* c1
    )
{
    UNUSED_PARAMETER(c1);
    raccelSFcnMexHandler("mexErrMsgTxt");
}


void
raccelMexWarnMsgTxt(
    const char* c1
    )
{
    UNUSED_PARAMETER(c1);
    raccelSFcnMexHandler("mexWarnMsgTxt");
}

void
raccelMexWarnMsgIdAndTxt(
    const char* c1,
    const char* c2,
    va_list v
    )
{
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(c2);
    UNUSED_PARAMETER(v);
    raccelSFcnMexHandler("mexWarnMsgIdAndTxt");    
}

bool
raccelMexSetMexTrapFlag()
{
    return false;
}

/* ////////////////////////////////////////////////////////////////////////////////////////// */
/* ports */
int_T
raccelSetRegNumInputPortsFcn(
    void* arg1,
    int_T nInputPorts) 
{
    SimStruct* child =
        (SimStruct*) arg1;
        
    child->sizes.in.numInputPorts =
        nInputPorts;

    return 1;
}

int_T
raccelSetRegNumOutputPortsFcn(
    void* arg1,
    int_T nOutputPorts) 
{
    SimStruct* child =
        (SimStruct*) arg1;
    
    child->sizes.out.numOutputPorts =
        nOutputPorts;
    
    return 1;
}


/* ////////////////////////////////////////////////////////////////////////////////////////// */
/* data type access handling */

void
raccelDataTypeAccessHandler(const char* method)
{
    const char* name =
        sfcnLoader_getCurrentSFcnName();

    const char* errTemplate =
        "<diag_root><diag id=\"Simulink:tools:rapidAccelSFcnDataTypeAccess\"><arguments><arg type = \"string\">%s</arg><arg type = \"string\">%s</arg></arguments></diag></diag_root>";
    

    char* message =
        (char *) calloc(1024, sizeof(char));
    
    sprintf(
        message,
        errTemplate,
        name,
        method
        );

    raccelSFcnLongJumper((const char*) message);
}

DTypeId
raccelDTARegisterDataType(
    void * v,
    const char_T * c1,
    const char_T * c2)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(c2);
    
    raccelDataTypeAccessHandler("ssRegisterDataType");
    return -1;
}

int_T
raccelDTAGetNumDataTypes(void * v)
{
    UNUSED_PARAMETER(v);
    
    raccelDataTypeAccessHandler("ssGetNumDataTypes");    
    return -1;
}

DTypeId
raccelDTAGetDataTypeId(
    void* v,
    const char_T* c1
    )
{

    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    
    raccelDataTypeAccessHandler("ssGetDataTypeId");
    return INVALID_DTYPE_ID;    
}

int_T
raccelDTAGetGenericDTAIntProp(
    void * v,
    const char_T * c1,
    DTypeId d,
    GenDTAIntPropType g)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(d);
    UNUSED_PARAMETER(g);
    
    raccelDataTypeAccessHandler("dtaGetGenericDTAIntProp");
    return -1;
}

int_T
raccelDTASetGenericDTAIntProp(
    void * v,
    const char_T * c1,
    DTypeId d,
    int_T i,
    GenDTAIntPropType g)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(d);
    UNUSED_PARAMETER(i);    
    UNUSED_PARAMETER(g);
    
    raccelDataTypeAccessHandler("dtaSetGenericDTAIntProp");
    return -1;
}

const void*
raccelDTAGetGenericDTAVoidProp(
    void * v,
    const char_T * c1,
    DTypeId d,
    GenDTAVoidPropType g)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(d);
    UNUSED_PARAMETER(g);
    
    raccelDataTypeAccessHandler("dtaGetGenericDTAVoidProp");
    return NULL;
}

int_T
raccelDTASetGenericDTAVoidProp(
    void * v1,
    const char_T * c1,
    DTypeId d,
    const void * v2,
    GenDTAVoidPropType g)
{
    UNUSED_PARAMETER(v1);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(d);
    UNUSED_PARAMETER(v2);    
    UNUSED_PARAMETER(g);
    
    raccelDataTypeAccessHandler("dtaSetGenericDTAVoidProp");
    return -1;
}

GenericDTAUnaryFcn
raccelDTAGetGenericDTAUnaryFcnGW(
    void * v,
    const char_T * c1,
    DTypeId d,
    GenDTAUnaryFcnType g)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(d);
    UNUSED_PARAMETER(g);
    
    raccelDataTypeAccessHandler("dtaGetGenericDTAUnaryFcnGW");
    return NULL;
}

int_T
raccelDTASetGenericDTAUnaryFcnGW(
    void * v,
    const char_T * c1,
    DTypeId d,
    GenericDTAUnaryFcn f,
    GenDTAUnaryFcnType g)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(d);
    UNUSED_PARAMETER(f);        
    UNUSED_PARAMETER(g);
    
    raccelDataTypeAccessHandler("dtaSetGenericDTAUnaryFcnGW");
    return -1;
}

GenericDTABinaryFcn
raccelDTAGetGenericDTABinaryFcnGW(
    void * v,
    const char_T * c1,
    DTypeId d,
    GenDTABinaryFcnType g)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(d);
    UNUSED_PARAMETER(g);
    
    raccelDataTypeAccessHandler("dtaGetGenericDTABinaryFcnGW");
    return NULL;
}

int_T
raccelDTASetGenericDTABinaryFcnGW(
    void * v,
    const char_T * c1,
    DTypeId d,
    GenericDTABinaryFcn f,
    GenDTABinaryFcnType g)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(d);
    UNUSED_PARAMETER(f);        
    UNUSED_PARAMETER(g);
    
    raccelDataTypeAccessHandler("dtaSetGenericDTABinaryFcnGW");
    return -1;
}

ConvertBetweenFcn
raccelDTAGetConvertBetweenFcn(
    void * v,
    const char_T * c1,
    DTypeId d)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(d);
    raccelDataTypeAccessHandler("dtaGetConvertBetweenFcn");
    return NULL;
}

int_T
raccelDTASetConvertBetweenFcn(
    void * v,
    const char_T * c1,
    DTypeId d,
    ConvertBetweenFcn f)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(d);
    UNUSED_PARAMETER(f);
    
    raccelDataTypeAccessHandler("dtaSetConvertBetweenFcn");
    return -1;
}

int_T
raccelDTAGetGenericDTADiagnostic(
    void * v,
    const char_T * c1,
    GenDTADiagnosticType g,
    BDErrorValue * b)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(g);
    UNUSED_PARAMETER(b);
    
    raccelDataTypeAccessHandler("dtaGetGenericDTADiagnostic");
    return -1;
}

void *
raccelDTARegisterDataTypeWithCheck(
    void * v,
    const char_T * c1,
    const char_T * c2,
    DTypeId * d)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(c2);
    UNUSED_PARAMETER(d);
    
    raccelDataTypeAccessHandler("dtaRegisterDataTypeWithCheck");
    return NULL;
}

int_T
raccelDTAGetGenericDTAIntElemProp(
    void * v,
    const char_T * c1,
    DTypeId d,
    int_T i,
    GenDTAIntElemPropType g)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(d);
    UNUSED_PARAMETER(i);        
    UNUSED_PARAMETER(g);
    
    raccelDataTypeAccessHandler("dtaGetGenericDTAIntElemProp");
    return -1;
}

int_T
raccelDTASetGenericDTAIntElemProp(
    void * v,
    const char_T * c1 ,
    DTypeId d,
    int_T i,
    int_T j,
    GenDTAIntElemPropType g)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(d);
    UNUSED_PARAMETER(i);
    UNUSED_PARAMETER(j);    
    UNUSED_PARAMETER(g);
    
    raccelDataTypeAccessHandler("dtaSetGenericDTAIntElemProp");
    return -1;
}

const void*
raccelDTAGetGenericDTAVoidElemProp(
    void * v,
    const char_T * c1,
    DTypeId d,
    int_T i,
    GenDTAVoidElemPropType g)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(d);
    UNUSED_PARAMETER(i);        
    UNUSED_PARAMETER(g);
    
    raccelDataTypeAccessHandler("dtaGetGenericDTAVoidElemProp");
    return NULL;
}

int_T
raccelDTASetGenericDTAVoidElemProp(
    void * v1,
    const char_T * c1,
    DTypeId d,
    int_T i,
    const void * v2,
    GenDTAVoidElemPropType g)
{
    UNUSED_PARAMETER(v1);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(d);
    UNUSED_PARAMETER(i);    
    UNUSED_PARAMETER(v2);        
    UNUSED_PARAMETER(g);
    
    raccelDataTypeAccessHandler("dtaSetGenericDTAVoidElemProp");
    return -1;
}

real_T
raccelDTAGetGenericDTARealElemProp(
    void * v,
    const char_T * c1,
    DTypeId d,
    int_T i,
    GenDTARealElemPropType g)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(d);            
    UNUSED_PARAMETER(i);
    UNUSED_PARAMETER(g);
    
    raccelDataTypeAccessHandler("dtaGetGenericDTARealElemProp");
    return -1;
}

int_T
raccelDTASetGenericDTARealElemProp(
    void * v,
    const char_T * c1,
    DTypeId d,
    int_T i,
    int_T j,
    GenDTARealElemPropType g)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(c1);
    UNUSED_PARAMETER(d);            
    UNUSED_PARAMETER(i);
    UNUSED_PARAMETER(j);
    UNUSED_PARAMETER(g);
    
    raccelDataTypeAccessHandler("dtaSetGenericDTARealElemProp");
    return -1;
}


/* ////////////////////////////////////////////////////////////////////////////////////////// */
typedef enum DataTypeInfoTableSearchMethod_T
{
    SEARCH_DATATYPEINFOTABLE_BY_NAME,
    SEARCH_DATATYPEINFOTABLE_BY_DATATYPEID
} DataTypeInfoTableSearchMethod;

void
getDataTypeInfo(
    SimStruct* S,
    const void* searchData,
    DataTypeInfoTableSearchMethod searchMethod,    
    const DataTypeInfo** dataTypeInfo
    )
{
    SimStruct* rootSS =
        NULL;

    const DataTypeTransInfo* dtInfo =
        NULL;

    const DataTypeInfo* dataTypeInfoTable =
        NULL;

    rootSS = ssGetRootSS(S);
    
    /* set error status? */
    if (rootSS == NULL)
    {
        return;
    }

    dtInfo = ssGetModelMappingInfo(rootSS);    

    /* set error status? */
    if (dtInfo == NULL)
    {
        return;
    }

    dataTypeInfoTable = dtInfo->dataTypeInfoTable;

    /* set error status? */
    if (dataTypeInfoTable == NULL)
    {
        return;
    }

    {
        int numDataTypes =
            dtInfo->numDataTypes;

        int idx = 0;

        switch(searchMethod)
        {
            
        case SEARCH_DATATYPEINFOTABLE_BY_NAME:
        {
            const char* dataTypeName =
                (const char*)(searchData);
            
            for (
                idx = 0;
                idx < numDataTypes;
                ++idx
                )
            {
            
                if (strcmp(dataTypeInfoTable[idx].name, dataTypeName) == 0)
                {
                    *dataTypeInfo = &(dataTypeInfoTable[idx]);
                    return;
                }
            }

            break;
        }

        case SEARCH_DATATYPEINFOTABLE_BY_DATATYPEID:
        {
            const DTypeId dataTypeId =
                *((const DTypeId*) searchData);
            
            for (
                idx = 0;
                idx < numDataTypes;
                ++idx
                )
            {
            
                if (dataTypeInfoTable[idx].dataTypeId == dataTypeId)
                {
                    *dataTypeInfo = &(dataTypeInfoTable[idx]);
                    return;
                }
            }

            break;            
        }
        
        }
    }
}


/* ////////////////////////////////////////////////////////////////////////////////////////// */
/* generic function handling */
int_T
raccelGenericFcn(
    SimStruct * S,
    GenFcnType genFcnType,
    int_T arg1,
    void * arg2)
{
    switch(genFcnType)
    {

    case GEN_FCN_SET_CURR_OUTPUT_DIMS:
    {
        /* _ssVarDimsIdxVal declared in simstruc.h */
        struct _ssVarDimsIdxVal_tag dimIdxVal =
            *((struct _ssVarDimsIdxVal_tag *) arg2);

        int dimIdx =
            dimIdxVal.dIdx;
            
        int dimVal =
            dimIdxVal.dVal;
            
        S->blkInfo.blkInfo2->portInfo2->outputs[arg1].portVarDims[dimIdx] 
            = dimVal;

        break;
    }

    case GEN_FCN_REGISTER_TYPE_FROM_NAMED_OBJECT:
    {

        /* ssRegisterTypeFromNameType is defined in simstruc_implement.h, but I can't include that
         * from some reason */
        typedef struct _ssRegisterTypeFromNameType_tag
        {
            const char_T* name;
            int_T* dataTypeId;
        } ssRegisterTypeFromNameType;
        
        ssRegisterTypeFromNameType* info =
            (ssRegisterTypeFromNameType*) arg2;

        const DataTypeInfo* dataTypeInfo =
            NULL;

        if (info == NULL)
        {
            return 0;
        }

        getDataTypeInfo(
            S,
            info->name,
            SEARCH_DATATYPEINFOTABLE_BY_NAME,            
            &dataTypeInfo
            );

        if (dataTypeInfo != NULL)
        {
            *(info->dataTypeId) = dataTypeInfo->dataTypeId;
        }
        
        break;
        
    }
    
    }

    return 0;    
}

/* ////////////////////////////////////////////////////////////////////////////////////////// */
/* set up the regDataType field of the s-function simstruct */
/* ssRegisterTypeFromExpr, ssRegisterTypeFromExprNoError, ssRegisterTypeFromNamedObject and ssRegisterTypeFromParameter */
/* are not here because they are pound-defined in simstruc.h to do nothing */
void
raccelRegDataTypeHandler(const char* method)
{
    const char* name =
        sfcnLoader_getCurrentSFcnName();

    const char* errTemplate =
        "<diag_root><diag id=\"Simulink:tools:rapidAccelSFcnRegDataType\"><arguments><arg type = \"string\">%s</arg><arg type = \"string\">%s</arg></arguments></diag></diag_root>";        

    char* message =
        (char *) calloc(1024, sizeof(char));
    
    sprintf(
        message,
        errTemplate,
        name,
        method
        );

    raccelSFcnLongJumper((const char*) message);
}

DTypeId
raccelRDTRegisterFcn(
    void* rootSS_vptr,
    const char *dataTypeName
    )
{
    const DataTypeInfo* dataTypeInfo =
        NULL;

    getDataTypeInfo(
        (SimStruct*)(rootSS_vptr),
        dataTypeName,
        SEARCH_DATATYPEINFOTABLE_BY_NAME,
        &dataTypeInfo
        );

    if (dataTypeInfo != NULL)
    {
        return dataTypeInfo->dataTypeId;
    }
    else
    {
        return INVALID_DTYPE_ID;
    }    
}

int_T
raccelRDTSetDataTypeSize(
    void *v,
    DTypeId d,
    int_T i)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(d);
    UNUSED_PARAMETER(i);
    raccelRegDataTypeHandler("ssSetDataTypeSize");
}

int_T
raccelRDTGetDataTypeSize(
    void* rootSS_vptr,
    DTypeId d
    )
{
    const DataTypeInfo* dataTypeInfo =
        NULL;

    getDataTypeInfo(
        (SimStruct*)(rootSS_vptr),
        (void*)(&d),
        SEARCH_DATATYPEINFOTABLE_BY_DATATYPEID,
        &dataTypeInfo
        );

    if (dataTypeInfo != NULL)
    {
        return dataTypeInfo->size;
    }
    else
    {
        return INVALID_DTYPE_ID;
    }        
}

int_T
raccelRDTSetDataTypeZero(
    void *v1,
    DTypeId d,
    void *v2)
{
    UNUSED_PARAMETER(v1);
    UNUSED_PARAMETER(d);
    UNUSED_PARAMETER(v2);
    raccelRegDataTypeHandler("ssSetDataTypeZero");
}

const void *
raccelRDTGetDataTypeZero(
    void *v,
    DTypeId d)
{
    UNUSED_PARAMETER(v);
    UNUSED_PARAMETER(d);
    raccelRegDataTypeHandler("ssGetDataTypeZero");
}

const char_T *
raccelRDTGetDataTypeName(
    void* rootSS_vptr,
    DTypeId d
    )
{
    const DataTypeInfo* dataTypeInfo =
        NULL;

    getDataTypeInfo(
        (SimStruct*)(rootSS_vptr),
        (void*)(&d),
        SEARCH_DATATYPEINFOTABLE_BY_DATATYPEID,
        &dataTypeInfo
        );

    if (dataTypeInfo != NULL)
    {
        return dataTypeInfo->name;
    }
    else
    {
        return NULL;
    }    
}

DTypeId
raccelRDTGetDataTypeId(
    void* rootSS_vptr,
    const char_T* dataTypeName
    )
{
    const DataTypeInfo* dataTypeInfo =
        NULL;

    getDataTypeInfo(
        (SimStruct*)(rootSS_vptr),
        dataTypeName,
        SEARCH_DATATYPEINFOTABLE_BY_NAME,
        &dataTypeInfo
        );

    if (dataTypeInfo != NULL)
    {
        return dataTypeInfo->dataTypeId;
    }
    else
    {
        return INVALID_DTYPE_ID;
    }
}

int_T
raccelRDTSetNumDWorkFcn(
    SimStruct * s,
    int_T i)
{
    s->sizes.numDWork = i;
    /* UNUSED_PARAMETER(s); */
    /* UNUSED_PARAMETER(i); */
    /* raccelRegDataTypeHandler(); */
}


/* ////////////////////////////////////////////////////////////////////////////////////////// */
static MexCallbacks mexCallbacks =
{
    &raccelMexIsLocked,
    &raccelMexPutVar,
    &raccelMexGetVarPtr,
    &raccelMexGetVar,
    &raccelMexLock,
    &raccelMexUnlock,
    &raccelMexFunctionName,
    &raccelMexEvalString,
    &raccelMexEvalStringWithTrap,
    &raccelMexSet,
    &raccelMexGet,
    &raccelMexCallMatlab,
    &raccelMexCallMatlabWithTrap,
    &raccelMexCreateSimpleFunctionHandle,
    &raccelMexAtExit,
    &raccelMexErrMsgIdAndTxt,
    &raccelMexErrMsgTxt,
    &raccelMexWarnMsgTxt,
    &raccelMexWarnMsgIdAndTxt,
    &raccelMexSetMexTrapFlag
};

static PortInfoFcns portInfoFcns =
{
    &raccelSetRegNumInputPortsFcn,
    &raccelSetRegNumOutputPortsFcn
};

static DataTypeAccessFcns dataTypeAccessFcns =
{
    &raccelDTARegisterDataType,
    &raccelDTAGetNumDataTypes,
    &raccelDTAGetDataTypeId,
    &raccelDTAGetGenericDTAIntProp,
    &raccelDTASetGenericDTAIntProp,
    &raccelDTAGetGenericDTAVoidProp,
    &raccelDTASetGenericDTAVoidProp,
    &raccelDTAGetGenericDTAUnaryFcnGW,
    &raccelDTASetGenericDTAUnaryFcnGW,
    &raccelDTAGetGenericDTABinaryFcnGW,
    &raccelDTASetGenericDTABinaryFcnGW,
    &raccelDTAGetConvertBetweenFcn,
    &raccelDTASetConvertBetweenFcn,
    &raccelDTAGetGenericDTADiagnostic,
    &raccelDTARegisterDataTypeWithCheck,
    &raccelDTAGetGenericDTAIntElemProp,
    &raccelDTASetGenericDTAIntElemProp,
    &raccelDTAGetGenericDTAVoidElemProp,
    &raccelDTASetGenericDTAVoidElemProp,
    &raccelDTAGetGenericDTARealElemProp,
    &raccelDTASetGenericDTARealElemProp
};

static GenericFcn genericFcn =
    &raccelGenericFcn;

static RegisterDataTypeFcns registerDataTypeFcns =
{
    &raccelRDTRegisterFcn,
    &raccelRDTSetDataTypeSize,
    &raccelRDTGetDataTypeSize,
    &raccelRDTSetDataTypeZero,
    &raccelRDTGetDataTypeZero,
    &raccelRDTGetDataTypeName,
    &raccelRDTGetDataTypeId,
    &raccelRDTSetNumDWorkFcn
};


/* ////////////////////////////////////////////////////////////////////////////////////////// */
/* get s-function info (names, paths, parameters etc.) */
void
raccelInitializeForMexSFcns()
{
    const char* errStr = NULL;

    sfcnLoader_initialize_for_rapid_accelerator(
        &gblErrorStatus,
        gblSFcnInfoFileName,
        &mexCallbacks,
        &portInfoFcns,
        &dataTypeAccessFcns,
        &genericFcn,
        &registerDataTypeFcns
        );

    if (gblErrorStatus != NULL)
    {
        return;
    }
    
}


/* ////////////////////////////////////////////////////////////////////////////////////////// */
/* dynamic loading */

void raccelLoadSFcnMexFile(
    const char* sFcnName,
    const char* blockSID,
    SimStruct* simstruct,
    size_t childIdx
    )
{
    
    sfcnLoader_callSFcn(
        sFcnName,
        blockSID,
        simstruct
        );
    
}


/* EOF raccel_sfcn_utils.c  */

/* LocalWords:  raccel_sfcn_utils.c */
