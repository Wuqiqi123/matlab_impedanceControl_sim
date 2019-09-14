/* File    : rt_matrx.c
 * Abstract:
 *      Implements stand alone matrix access and creation routines.
 *	There are two types of MATLAB objects which can be "passed" to
 *	the generated code, a 2D real matrix and a string. Strings are
 *	passed as 2D real matrices. The first two elements of an S-function
 *	parameters are the row and column (m and n) dimensions respectively.
 *	These are followed by the matrix data.
 */



/*
 * Copyright 1994-2015 The MathWorks, Inc.
 */

/*==========*
 * Includes *
 *==========*/

#if defined(MDL_REF_SIM_TGT)
#undef MATLAB_MEX_FILE
#endif

#if defined(MATLAB_MEX_FILE)
# error "rt_matrix cannot be used within a mex file. It is for codegen only."
#endif

#include <stdlib.h>    /* needed for malloc, calloc, free, realloc */
#include <string.h>    /* needed for strlen                        */
#include "rtwtypes.h"  /* needed for real_T                        */
#include "rt_mxclassid.h" /* needed for mxClassID                     */
#include "rt_matrx.h"

#include <stddef.h> /* needed for size_t and NULL */
#include <float.h>  /* needed for definition of eps */

/*==========*
 * Typedefs *
 *==========*/

#ifndef rt_typedefs_h
#define rt_typedefs_h

#if !defined(TYPEDEF_MX_ARRAY)
# define TYPEDEF_MX_ARRAY
  typedef real_T mxArray;
#endif

typedef real_T mxChar;

#if !defined(TMW_NAME_LENGTH_MAX)
#define TMW_NAME_LENGTH_MAX 64
#endif
#define mxMAXNAM  TMW_NAME_LENGTH_MAX	/* maximum name length */

typedef enum {
    mxREAL,
    mxCOMPLEX
} mxComplexity;

#endif /* rt_typedefs_h */

/*==================*
 * Extern variables *
 *==================*/

#ifdef __cplusplus
extern "C" {
#endif

extern real_T rtInf;
extern real_T rtMinusInf;
extern real_T rtNaN;

#ifdef __cplusplus
}
#endif

/*=======================================*
 * Defines for mx Routines and constants *
 *=======================================*/


#define mxCalloc(n,size) \
        calloc(n,size)

#define mxCreateCharArray(ndim, dims) \
        mxCreateNumericArray(ndim, dims, mxCHAR_CLASS);

#define mxDestroyArray(pa) \
        if (pa) free(pa)

/* NOTE: You cannot mxFree(mxGetPr(pa)) !!! */
#define mxFree(ptr) \
        if(ptr)free(ptr)

#define mxGetClassID(pa) \
        mxDOUBLE_CLASS

/* NOTE: mxGetClassName(pa) returns "double" even on a character array */
#define mxGetClassName(pa) \
        "double"

#define mxGetData(pa) \
        ((void *)(&((pa)[2])))

#define mxGetElementSize(pa) \
        (sizeof(real_T))

#define mxGetInf() \
        rtInf

#define mxGetM(pa) \
        ((size_t) ((pa)[0]))
#define mxGetN(pa) \
        ((size_t) ((pa)[1]))

#define mxGetNaN() \
        rtNaN

#define mxGetNumberOfDimensions(pa) \
        (2)
#define mxGetNumberOfElements(pa) \
        (mxGetM(pa)*mxGetN(pa))

/* NOTE: mxGetPr() of an empty matrix does NOT return NULL */
#define mxGetPr(pa) \
        ( &((pa)[2]) )

#define mxGetScalar(pa) \
        ((pa)[2])

#define mxIsComplex(pa) \
        false

#define mxIsDouble(pa) \
        true

#define mxIsEmpty(pa) \
        (mxGetM(pa)==0 || mxGetN(pa)==0)

#define mxIsFinite(r) \
        ((r)>rtMinusInf && (r)<rtInf)

#define mxIsInf(r) \
        ((r)==rtInf || (r)==rtMinusInf)

#define mxIsInt16(pa) \
        false

#define mxIsInt32(pa) \
        false

#define mxIsInt8(pa) \
        false

#define mxIsLogical(pa) \
        false

#define mxIsNumeric(pa) \
        true

#define mxIsSingle(pa) \
        false

#define mxIsSparse(pa) \
        false

#define mxIsStruct(pa) \
        false

#define mxIsUint16(pa) \
        false

#define mxIsUint32(pa) \
        false

#define mxIsUint8(pa) \
        false

#define mxMalloc(n) \
        malloc(n)

#define mxRealloc(p,n) \
        realloc(p,n)

/*==============*
 * Local macros *
 *==============*/
#define _mxSetM(pa,m) \
        (pa)[0] = ((int)(m))

#define _mxSetN(pa,n) \
        (pa)[1] = ((int)(n))


/*==========================*
 * Visible/extern functions *
 *=========================*/

/* Function: mxCreateCharMatrixFromStrings ====================================
 * Abstract:
 *	Create a string array initialized to the strings in str.
 */

#ifdef __cplusplus
extern "C" {
#endif

mxArray *rt_mxCreateCharMatrixFromStrings(int_T m, const char_T **str)
{
    int_T nchars;
    int_T i, n;
    mxArray *pa;

    n = 0;
    for (i = 0; i < m; ++i) {
	nchars = (int_T)strlen(str[i]);
	if (nchars > n) {
	    n = nchars;
	}
    }
    /*LINTED E_PASS_INT_TO_SMALL_INT*/
    pa = (mxArray *)malloc((m*n+2)*sizeof(real_T));
    if(pa!=NULL) {
	mxChar *chars;
	int_T  j;
	_mxSetM(pa, m);
	_mxSetN(pa, n);
	chars = mxGetPr(pa);
	for (j = 0; j < m; ++j) {
	    const char_T *src  = str[j];
	    mxChar *dest = chars + j;

	    nchars = (int_T)strlen(src);
	    i = nchars;
	    while (i--) {
		*dest = *src++;
		 dest += m;
	    }
	    i = n - nchars;
	    while (i--) {
		*dest = 0.0;
		dest += m;
	    }
	}
    }
    return pa;
} /* end mxCreateCharMatrixFromStrings */

#ifdef __cplusplus
}
#endif

/* Function: mxCreateString ===================================================
 * Abstract:
 *	Create a 1-by-n string array initialized to null terminated string
 *	where n is the length of the string.
 */
#ifdef __cplusplus
extern "C" {
#endif

mxArray *rt_mxCreateString(const char *str)
{
    int_T   len = (int_T)strlen(str);
    /*LINTED E_PASS_INT_TO_SMALL_INT*/
    mxArray *pa = (mxArray *)malloc((len+2)*sizeof(real_T));

    if(pa!=NULL) {
	real_T *pr;
	const unsigned char *ustr_ptr = (const unsigned char *) str;

	_mxSetM(pa, 1);
	_mxSetN(pa, len);
	pr = mxGetPr(pa);
	while (len--) {
            *pr++ = (real_T)*ustr_ptr++;
        }
    }
    return(pa);

} /* end mxCreateString */

#ifdef __cplusplus
}
#endif

/* Function: mxCreateDoubleMatrix =============================================
 * Abstract:
 *	Create a two-dimensional array to hold real_T data,
 *	initialize each data element to 0.
 */
/*LINTLIBRARY*/

#ifdef __cplusplus
extern "C" {
#endif

mxArray *rt_mxCreateDoubleMatrix(int m, int n, mxComplexity flag)
{
    if (flag == mxREAL) {
        mxArray *pa = (mxArray *)calloc(m*n+2, sizeof(real_T));
        if(pa!=NULL) {
            _mxSetM(pa, m);
            _mxSetN(pa, n);
        }
        return(pa);
    } else {
        return(NULL);
    }

} /* end mxCreateDoubleMatrix */

#ifdef __cplusplus
}
#endif

/* Function: mxCreateNumericArray =============================================
 * Abstract:
 *	Create a numeric array and initialize all its data elements to 0.
 */

#ifdef __cplusplus
extern "C" {
#endif

mxArray *rt_mxCreateNumericArray(int_T ndims, const mwSize *dims, 
                                        mxClassID classid, mxComplexity flag)
{
    if (ndims == 2 && classid==mxDOUBLE_CLASS) {
        return(rt_mxCreateDoubleMatrix(dims[0], dims[1], flag));
    } else {
        return(NULL);
    }

} /* end mxCreateNumericArray */

#ifdef __cplusplus
}
#endif

/* Function: mxDuplicateArray =================================================
 * Abstract:
 *	Make a deep copy of an array, return a pointer to the copy.
 */
#ifdef __cplusplus
extern "C" {
#endif


mxArray *rt_mxDuplicateArray(const mxArray *pa)
{
    /*LINTED E_ASSIGN_INT_TO_SMALL_INT*/
    size_t   nbytes = (mxGetNumberOfElements(pa)+2)*mxGetElementSize(pa);
    mxArray *pcopy = (mxArray *)malloc(nbytes);

    if (pcopy!=NULL) {
	(void)memcpy(pcopy, pa, nbytes);
    }
    return(pcopy);

} /* end mxDuplicateArray */


#ifdef __cplusplus
}
#endif


/* Function: mxGetDimensions ==================================================
 * Abstract:
 *	Get pointer to dimension array
 * 	NOTE: This routine is not reentrant.
 */


#ifdef __cplusplus
extern "C" {
#endif


const mwSize *rt_mxGetDimensions(const mxArray *pa)
{
    static mwSize dims[2];
    dims[0] = mxGetM(pa);
    dims[1] = mxGetN(pa);
    return dims;
} /* end mxGetDimensions */


#ifdef __cplusplus
}
#endif


/* Function: mxGetEps =========================================================
 * Abstract:
 *	Return eps, the difference between 1.0 and the least value
 *	greater than 1.0 that is representable as a real_T.
 *	NOTE: Assumes real_T is either double or float.
 */


#ifdef __cplusplus
extern "C" {
#endif


real_T rt_mxGetEps(void)
{
    return (sizeof(double)==sizeof(real_T)) ? DBL_EPSILON : FLT_EPSILON;
}


#ifdef __cplusplus
}
#endif


/* Function: mxGetString ======================================================
 * Abstract:
 *	Converts a string array to a C-style string.
 */


#ifdef __cplusplus
extern "C" {
#endif


int_T rt_mxGetString(const mxArray *pa, char_T *buf, int_T buflen)
{
    int_T        nchars;
    const real_T *pr;
    char_T       *pc;
    int_T        truncate = 0;

    nchars = (int_T)mxGetNumberOfElements(pa);
    if (nchars >= buflen) {
	/* leave room for null byte */
	nchars = buflen - 1;
	truncate = 1;
    }
    pc = buf;
    pr = mxGetPr(pa);
    while (nchars--) {
	*pc++ = (char) (*pr++ + .5);
    }
    *pc = '\0';
    return truncate;
} /* end mxGetString */


#ifdef __cplusplus
}
#endif


#define mxCreateCharMatrixFromStrings(m, str) \
        rt_mxCreateCharMatrixFromStrings(m, str)

#define mxCreateString(str) \
        rt_mxCreateString(str) 

#define mxCreateDoubleMatrix(m, n, flag) \
        rt_mxCreateDoubleMatrix(m, n, flag)

#define mxCreateNumericArray(ndims, dims, classid, flag) \
        rt_mxCreateNumericArray(ndims, dims, classid, flag)

#define mxDuplicateArray(pa) \
        rt_mxDuplicateArray(pa)

#define mxGetDimensions(pa) \
        rt_mxGetDimensions(pa)

#define mxGetEps() \
        rt_mxGetEps()

#define mxGetString(pa, buf, buflen) \
        rt_mxGetString(pa, buf, buflen)

/*=========================*
 * Unsupported mx Routines *
 *=========================*/

#define mxCalcSingleSubscript(pa,nsubs,subs) \
        mxCalcSingleSubscript_is_not_supported_in_Simulink_Coder

#define mxCreateCellArray(ndim,dims) \
        mxCreateCellArray_is_not_supported_in_Simulink_Coder

#define mxCreateCellMatrix(m,n) \
        mxCreateCellMatrix_is_not_supported_in_Simulink_Coder

#define mxCreateSparse(pm,pn,pnzmax,pcmplx_flg) \
        mxCreateSparse_is_not_supported_in_Simulink_Coder

#define mxCreateStructArray(ndim,dims,nfields,fieldnames) \
        mxCreateStructArray_is_not_supported_in_Simulink_Coder

#define mxCreateStructMatrix(m,n,nfields,fieldnames) \
        mxCreateStructMatrix_is_not_supported_in_Simulink_Coder

#define mxGetCell(pa,i) \
        mxGetCell_is_not_supported_in_Simulink_Coder

#define mxGetField(pa,i,fieldname) \
        mxGetField_is_not_supported_in_Simulink_Coder

#define mxGetFieldByNumber(s,i,fieldnum) \
        mxGetFieldByNumber_is_not_supported_in_Simulink_Coder

#define mxGetFieldNameByNumber(pa,n) \
        mxGetFieldNameByNumber_is_not_supported_in_Simulink_Coder

#define mxGetFieldNumber(pa,fieldname) \
        mxGetFieldNumber_is_not_supported_in_Simulink_Coder

#define mxGetImagData(pa) \
        mxGetImagData_is_not_supported_in_Simulink_Coder

#define mxGetIr(ppa) \
        mxGetIr_is_not_supported_in_Simulink_Coder

#define mxGetJc(ppa) \
        mxGetJc_is_not_supported_in_Simulink_Coder

#define mxGetNumberOfFields(pa) \
        mxGetNumberOfFields_is_not_supported_in_Simulink_Coder

#define mxGetNzmax(pa) \
        mxGetNzmax_is_not_supported_in_Simulink_Coder

#define mxGetPi(pa) \
        mxGetPi_is_not_supported_in_Simulink_Coder

#define mxIsFromGlobalWS(pa) \
        mxIsFromGlobalWS_is_not_supported_in_Simulink_Coder

#define mxIsNaN(r) \
        mxIsNaN_is_not_supported_in_Simulink_Coder

#define mxIsChar(pa) \
        mxIsChar_is_not_supported_in_Simulink_Coder

#define mxIsClass(pa,class) \
        mxIsClass_is_not_supported_in_Simulink_Coder

#define mxIsCell(pa) \
        mxIsCell_is_not_supported_in_Simulink_Coder

#define mxSetCell(pa,i,value) \
        mxSetCell_is_not_supported_in_Simulink_Coder

#define mxSetClassName(pa,classname) \
        mxSetClassName_is_not_supported_in_Simulink_Coder

#define mxSetData(pa,pr) \
        mxSetData_is_not_supported_in_Simulink_Coder

#define mxSetDimensions(pa, size, ndims) \
        mxSetDimensions_is_not_supported_in_Simulink_Coder

#define mxSetField(pa,i,fieldname,value) \
        mxSetField_is_not_supported_in_Simulink_Coder

#define mxSetFieldByNumber(pa, index, fieldnum, value) \
        mxSetFieldByNumber_is_not_supported_in_Simulink_Coder

#define mxSetFromGlobalWS(pa,global) \
        mxSetFromGlobalWS_is_not_supported_in_Simulink_Coder

#define mxSetImagData(pa,pv) \
        mxSetImagData_is_not_supported_in_Simulink_Coder

#define mxSetIr(ppa,ir) \
        mxSetIr_is_not_supported_in_Simulink_Coder

#define mxSetJc(ppa,jc) \
        mxSetJc_is_not_supported_in_Simulink_Coder

#define mxSetM(pa, m) \
        mxSetM_is_not_supported_in_Simulink_Coder

#define mxSetN(pa, m) \
        mxSetN_is_not_supported_in_Simulink_Coder

#define mxSetPr(pa,pr) \
        mxSetPr_is_not_supported_in_Simulink_Coder

#define mxSetNzmax(pa,nzmax) \
        mxSetNzmax_is_not_supported_in_Simulink_Coder

#define mxSetPi(pa,pv) \
        mxSetPi_is_not_supported_in_Simulink_Coder



/*==========================*
 * Unsupported mex routines *
 *==========================*/

#define mexPrintAssertion(test,fname,linenum,message) \
        mexPrintAssertion_is_not_supported_by_Simulink_Coder

#define mexEvalString(str) \
        mexEvalString_is_not_supported_by_Simulink_Coder

#define mexErrMsgTxt(str) \
        mexErrMsgTxt_is_not_supported_by_Simulink_Coder

#define mexWarnMsgTxt(warning_msg) \
        mexWarnMsgTxt_is_not_supported_by_Simulink_Coder

#define mexPrintf \
        mexPrintf_is_not_supported_by_Simulink_Coder

#define mexMakeArrayPersistent(pa) \
        mexMakeArrayPersistent_is_not_supported_by_Simulink_Coder

#define mexMakeMemoryPersistent(ptr) \
        mexMakeMemoryPersistent_is_not_supported_by_Simulink_Coder

#define mexLock() \
        mexLock_is_not_supported_by_Simulink_Coder

#define mexUnlock() \
        mexUnlock_is_not_supported_by_Simulink_Coder

#define mexFunctionName() \
        mexFunctionName_is_not_supported_by_Simulink_Coder

#define mexIsLocked() \
        mexIsLocked_is_not_supported_by_Simulink_Coder

#define mexGetFunctionHandle() \
        mexGetFunctionHandle_is_not_supported_by_Simulink_Coder

#define mexCallMATLABFunction() \
        mexCallMATLABFunction_is_not_supported_by_Simulink_Coder

#define mexRegisterFunction() \
        mexRegisterFunction_is_not_supported_by_Simulink_Coder

#define mexSet(handle,property,value) \
        mexSet_is_not_supported_by_Simulink_Coder

#define mexGet(handle,property) \
        mexGet_is_not_supported_by_Simulink_Coder

#define mexCallMATLAB(nlhs,plhs,nrhs,prhs,fcn) \
        mexCallMATLAB_is_not_supported_by_Simulink_Coder

#define mexSetTrapFlag(flag) \
        mexSetTrapFlag_is_not_supported_by_Simulink_Coder

#define mexUnlink(a) \
        mexUnlink_is_not_supported_by_Simulink_Coderw

#define mexSubsAssign(plhs,sub,nsubs,prhs) \
        mexSubsAssign_is_not_supported_by_Simulink_Coder

#define mexSubsReference(prhs,subs,nsubs) \
        mexSubsReference_is_not_supported_by_Simulink_Coder

#define mexPrintAssertion(test,fname,linenum,message) \
        mexPrintAssertion_is_not_supported_by_Simulink_Coder

#define mexAddFlops(count) \
        mexAddFlops_is_not_supported_by_Simulink_Coder

#define mexIsGlobal(pa) \
        mexIsGlobal_is_not_supported_by_Simulink_Coder

#define mexAtExit(fcn) \
        mexAtExit_is_not_supported_by_Simulink_Coder

/* [EOF] rt_matrx.c */
