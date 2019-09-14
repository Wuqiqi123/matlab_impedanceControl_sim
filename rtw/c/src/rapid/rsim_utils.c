/******************************************************************
 *
 *  File: rsim_utils.c
 *
 *
 *  Abstract:
 *      - provide utility functions for the RSim target
 *
 * Copyright 2007-2014 The MathWorks, Inc.
 ******************************************************************/

/* INCLUDES */
#include  <stdio.h>
#include  <stdlib.h>

#include  <string.h>
#include  <math.h>
#include  <float.h>
#include  <ctype.h>

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
#include "common_utils.h"
#include  "rsim_utils.h"

/* external variables */
extern const char   *gblParamFilename;
extern int_T         gblParamCellIndex;

static PrmStructData gblPrmStruct;


/*==================    *
 * NON-Visible routines *
 *==================    */


/* Function: rt_FreeParamStructs ===========================================
 * Abstract:
 *      Free and NULL the fields of all 'PrmStructData' structures.
 */
void rt_FreeParamStructs(PrmStructData *paramStructure)
{
    if (paramStructure != NULL) {
        size_t i;
        size_t nTrans       = paramStructure->nTrans;
        DTParamInfo *dtParamInfo = paramStructure->dtParamInfo;

        if (dtParamInfo != NULL) {
            for (i=0; i<nTrans; i++) {
                /*
                 * Must free "stolen" parts of matrices with
                 * mxFree (they are allocated with mxCalloc).
                 */
#if defined(MX_HAS_INTERLEAVED_COMPLEX)
                mxFree(dtParamInfo[i].vals);
#else
                mxFree(dtParamInfo[i].rVals);
                mxFree(dtParamInfo[i].iVals);
#endif
            }
            free(dtParamInfo);
        }

        paramStructure->nTrans      = 0;
        paramStructure->dtParamInfo = NULL;
    }
} /* end rt_FreeParamStructs */

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
const char *rt_ReadParamStructMatFile(PrmStructData **prmStructOut,
                                         int           cellParamIndex)
{
    size_t nTrans;
    size_t i;
    MATFile       *pmat              = NULL;
    mxArray       *pa                = NULL;
    const mxArray *paParamStructs    = NULL;
    PrmStructData *paramStructure    = NULL;
    const char    *result            = NULL; /* assume success */

    paramStructure = &gblPrmStruct;

    /**************************************************************************
     * Open parameter MAT-file, read checksum, swap rtP data for type Double *
     **************************************************************************/

    if ((pmat=matOpen(gblParamFilename,"r")) == NULL) {
        result = "could not find MAT-file containing new parameter data";
        goto EXIT_POINT;
    }

    /*
     * Read the param variable. The variable name must be passed in
     * from the generated code.
     */
    if ((pa=matGetNextVariable(pmat,NULL)) == NULL ) {
        result = "error reading new parameter data from MAT-file "
            "(matGetNextVariable)";
        goto EXIT_POINT;
    }

    /* Should be 1x1 structure */
    if (!mxIsStruct(pa) ||
        mxGetM(pa) != 1 || mxGetN(pa) != 1 ) {
        result = "parameter variables must be a 1x1 structure";
        goto EXIT_POINT;
    }

    /* look for modelChecksum field */
    {
        const double  *newChecksum;
        const mxArray *paModelChecksum;

        if ((paModelChecksum = mxGetField(pa, 0, "modelChecksum")) == NULL) {
            result = "parameter variable must contain a modelChecksum field";
            goto EXIT_POINT;
        }

        /* check modelChecksum field */
        if (!mxIsDouble(paModelChecksum) || mxIsComplex(paModelChecksum) ||
            mxGetNumberOfDimensions(paModelChecksum) > 2 ||
            mxGetM(paModelChecksum) < 1 || mxGetN(paModelChecksum) !=4 ) {
            result = "invalid modelChecksum in parameter MAT-file";
            goto EXIT_POINT;
        }

        newChecksum = mxGetPr(paModelChecksum);

        paramStructure->checksum[0] = newChecksum[0];
        paramStructure->checksum[1] = newChecksum[1];
        paramStructure->checksum[2] = newChecksum[2];
        paramStructure->checksum[3] = newChecksum[3];
    }

    /*
     * Get the "parameters" field from the structure.  It is an
     * array of structures.
     */
    if ((paParamStructs = mxGetField(pa, 0, "parameters")) == NULL) {
        goto EXIT_POINT;
    }

    /*
     * If the parameters field is a cell array then pick out the cell
     * array pointed to by the cellParamIndex
     */
    if ( mxIsCell(paParamStructs) ) {
        /* check that cellParamIndex is in range */
        size_t size = mxGetM(paParamStructs) * mxGetN(paParamStructs);
        if (cellParamIndex > 0 && cellParamIndex <= size){
            paParamStructs = mxGetCell(paParamStructs, cellParamIndex-1);
        }else{
            result = "Invalid index into parameter cell array";
            goto EXIT_POINT;
        }
        if (paParamStructs == NULL) {
            result = "Invalid parameter field in parameter structure";
            goto EXIT_POINT;
        }
    }

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

        if ((dum = mxGetField(paParamStructs, 0, "dataTypeName")) == NULL) {
            result = "parameters struct must contain a dataTypeName field";
            goto EXIT_POINT;
        }

        if ((dum = mxGetField(paParamStructs, 0, "dataTypeId")) == NULL) {
            result = "parameters struct must contain a dataTypeId field";
            goto EXIT_POINT;
        }

        if ((dum = mxGetField(paParamStructs, 0, "complex")) == NULL) {
            result = "parameters struct must contain a complex field";
            goto EXIT_POINT;
        }

        if ((dum = mxGetField(paParamStructs, 0, "dtTransIdx")) == NULL) {
            result = "parameters struct must contain a dtTransIdx field";
            goto EXIT_POINT;
        }

    }

    /*
     * Allocate the DTParamInfo's.
     */
    paramStructure->dtParamInfo = (DTParamInfo *)
        calloc(nTrans,sizeof(DTParamInfo));
    if (paramStructure->dtParamInfo == NULL) {
        result = "Memory allocation error";
        goto EXIT_POINT;
    }

    paramStructure->nTrans = nTrans;

    /*
     * Get the new parameter data for each data type.
     */
    paramStructure->numParams = 0;
    for (i=0; i<nTrans; i++) {
        double      *pr;
        mxArray     *mat;
        DTParamInfo *dtprmInfo = &paramStructure->dtParamInfo[i];

        /*
         * Grab the datatype id.
         */
        mat = mxGetField(paParamStructs,i,"dataTypeId");
        pr  = mxGetPr(mat);

        dtprmInfo->dataType = (int)pr[0];

        /*
         * Grab the complexity.
         */
        mat = mxGetField(paParamStructs,i,"complex");
        pr  = mxGetPr(mat);

        dtprmInfo->complex = (bool)pr[0];

        /*
         * Grab the data type transition index.
         */
        mat = mxGetField(paParamStructs,i,"dtTransIdx");
        pr  = mxGetPr(mat);

        dtprmInfo->dtTransIdx = (int)pr[0];

        /*
         * Grab the data and any attributes.  We "steal" the data
         * from the mxArray.
         */
        mat = mxGetField(paParamStructs,i,"values");
        if (mat) {
            dtprmInfo->elSize = mxGetElementSize(mat);
            dtprmInfo->nEls   = mxGetNumberOfElements(mat);

#if defined(MX_HAS_INTERLEAVED_COMPLEX)
            dtprmInfo->vals  = mxGetData(mat);
            mxSetData(mat,NULL);
#else
            dtprmInfo->rVals  = mxGetData(mat);
            dtprmInfo->iVals  = mxGetImagData(mat);
            mxSetData(mat,NULL);
            mxSetImagData(mat,NULL);
#endif
        } else {
            dtprmInfo->nEls   = 0;
            dtprmInfo->elSize = 0;
#if defined(MX_HAS_INTERLEAVED_COMPLEX)
            dtprmInfo->vals  = NULL;
#else
            dtprmInfo->rVals  = NULL;
            dtprmInfo->iVals  = NULL;
#endif
        }

        /*
         * Increment total element count.
         */
        paramStructure->numParams += dtprmInfo->nEls;
    }

EXIT_POINT:
    mxDestroyArray(pa);

    if (pmat != NULL) {
        matClose(pmat); pmat = NULL;
    }

    if (result != NULL) {
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
static const char *ReplaceRtP(const SimStruct *S,
                              const PrmStructData *paramStructure)
{
    size_t                     i;
    const char              *errStr        = NULL;
    const DTParamInfo       *dtParamInfo   = paramStructure->dtParamInfo;
    size_t                  nTrans         = paramStructure->nTrans;
    const DataTypeTransInfo *dtInfo        = (const DataTypeTransInfo *)ssGetModelMappingInfo(S);
    DataTypeTransitionTable *dtTable       = dtGetParamDataTypeTrans(dtInfo);
    uint_T                  *dataTypeSizes = dtGetDataTypeSizes(dtInfo);

    for (i=0; i<nTrans; i++) {
        int  dataTransIdx  = dtParamInfo[i].dtTransIdx;
        char *transAddress = dtTransGetAddress(dtTable, dataTransIdx);
        bool complex       = (bool)dtParamInfo[i].complex;
        int  dataType      = dtParamInfo[i].dataType;
        int  dtSize        = (int)dataTypeSizes[dataType];
        size_t nEls          = dtParamInfo[i].nEls;
        size_t elSize      = dtParamInfo[i].elSize;
        size_t nParams       = (elSize*nEls)/dtSize;
        if (!nEls) continue;

#if defined(MX_HAS_INTERLEAVED_COMPLEX)
        if(complex) { dtSize*= 2; } /* Added for complex-interleaved conversion. */
#endif
        
        /*
         * Check for consistent element size.  dtParamInfo->elSize is the size
         * as stored in the parameter mat-file.  This should match the size
         * used by the generated code (i.e., stored in the SimStruct).
         */
        if ((dataType <= 13 && elSize != dtSize) ||
            (dataType > 13 && (dtSize % elSize != 0))){
            errStr = "Parameter data type sizes in MAT-file not same "
                "as data type sizes in RTW generated code";
            goto EXIT_POINT;
        }
#if defined(MX_HAS_INTERLEAVED_COMPLEX)
        (void)memcpy(transAddress,dtParamInfo[i].vals,nParams*dtSize);
#else
        if (!complex) {
            (void)memcpy(transAddress,dtParamInfo[i].rVals,nParams*dtSize);
        } else {
            /*
             * Must interleave the real and imaginary parts.  Simulink style.
             */
            int  j;
            char *dst     = transAddress;
            const char *realSrc = (const char *)dtParamInfo[i].rVals;
            const char *imagSrc = (const char *)dtParamInfo[i].iVals;

            for (j=0; j<nParams; j++) {
                /* Copy real part. */
                (void)memcpy(dst,realSrc,dtSize);
                dst     += dtSize;
                realSrc += dtSize;

                /* Copy imag part. */
                (void)memcpy(dst,imagSrc,dtSize);
                dst     += dtSize;
                imagSrc += dtSize;
            }
        }
#endif
        
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
void rt_RapidReadMatFileAndUpdateParams(const SimStruct *S)
{
    const char*    result         = NULL;
    PrmStructData* paramStructure = NULL;

    if (gblParamFilename == NULL) goto EXIT_POINT;

    result = rt_ReadParamStructMatFile(&paramStructure, gblParamCellIndex);
    if (result != NULL) goto EXIT_POINT;

    /* be sure checksums all match */
    if (paramStructure->checksum[0] != ssGetChecksum0(S) ||
        paramStructure->checksum[1] != ssGetChecksum1(S) ||
        paramStructure->checksum[2] != ssGetChecksum2(S) ||
        paramStructure->checksum[3] != ssGetChecksum3(S) ) {
        result = "model checksum mismatch - incorrect parameter data "
            "specified";
        goto EXIT_POINT;
    }

    /* Replace the rtP structure */
    result = ReplaceRtP(S, paramStructure);
    if (result != NULL) goto EXIT_POINT;

  EXIT_POINT:
    if (paramStructure != NULL) {
        rt_FreeParamStructs(paramStructure);
    }
    if (result) ssSetErrorStatus(S, result);
    return;

} /* rt_RapidReadMatFileAndUpdateParams */




/* EOF rsim_utils.c */

/* LocalWords:  RSim matrx smaple matfile rb scaler Tx gbl tu Datato TUtable
 * LocalWords:  UTtable FName FFname raccel el fromfile npts npoints nchannels
 * LocalWords:  curr tfinal timestep Gbls Remappings
 */
