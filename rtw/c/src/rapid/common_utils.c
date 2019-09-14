/******************************************************************
 *
 *  File: common_utils.c
 *
 *
 *  Abstract:
 *      - provide utility functions common to rapid accelerator and RSim
 *        target
 *
 * Copyright 2007-2017 The MathWorks, Inc.
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

#include "sigstream_rtw.h"
#include "common_utils.h"

extern mxClassID rt_GetMxIdFromDTypeIdForRSim(BuiltInDTypeId dTypeID); 
extern mxClassID rt_GetMxIdFromDTypeId(BuiltInDTypeId dTypeID); 

/* external variables */
extern const char  *gblFromWorkspaceFilename;
extern const char  *gblParamFilename;
extern const char  *gblInportFileName;

extern const int_T gblNumToFiles;
extern FNamePair   *gblToFNamepair;

extern const int_T gblNumFrFiles;
extern FNamePair   *gblFrFNamepair;

extern int_T         gblParamCellIndex;

/* total signal width of root inports */
extern int_T gblNumModelInputs;
/* total number of root inports */
extern int_T gblNumRootInportBlks;        
/* inport dimensions */
extern int_T gblInportDims[];
/* inport signal is real or complex */
extern int_T gblInportComplex[];
/* inport interpolation flag */
extern int_T gblInportInterpoFlag[];
/* simulink data type idx of input port signal */
extern int_T gblInportDataTypeIdx[];
/* inport block smaple time */
extern int_T gblInportContinuous[];

rtInportTUtable *gblInportTUtables = NULL;
char  *gblMatSigstreamLoggingFilename = NULL;
char  *gblMatSigLogSelectorFilename = NULL;
void  *gblISigstreamManager = NULL;
void  *gblOSigstreamManager = NULL;
void  *slioCatalogue = NULL;

#define INVALID_DTYPE_ID   (-10)
#define SINGLEVAR_MATRIX   (0)
#define SINGLEVAR_STRUCT   (1)
#define MULTIPLEVAR_LIST (2) 


# define Interpolate(v1,v2,f1,f2) \
	    (((v1)==(v2))?((real_T)(v1)):\
                          (((f1)*((real_T)(v1)))+((f2)*((real_T)(v2)))))
# define InterpRound(v) (((v) >= 0) ? floor(((real_T)(v)) + 0.5): \
				       ceil(((real_T)(v)) - 0.5))


/*==================*
 * NON-Visible routines *
 *==================*/

/*
  Used to turn interleaved complex back into de-interleaved for parts of code
  that still need the de-interleaved representation and cannot be changed
  at this time.  

  TODO: watch out for performance impact, and do work to remove this.
*/
static void deInterleaveComplex(char* realDst, char* imagDst, char* src, 
                                size_t halfElementSize, size_t M, size_t N) {
    size_t i, j;
    for (j=0;j < N;j++)
    {
        for (i=0;i < M;i++)
        {
            /* Copy real part. */
            (void)memcpy(realDst,src,halfElementSize);
            src += halfElementSize;
            realDst += halfElementSize;

            /* Copy imag part. */
            (void)memcpy(imagDst,src,halfElementSize);
            src += halfElementSize;
            imagDst += halfElementSize;
        }
    }
}

/*
  This is taken from rt_matrx with an added multiplication factor
 */
static real_T rapid_eps(real_T t)
{
    return(((sizeof(double)==sizeof(real_T)) ? DBL_EPSILON:FLT_EPSILON)*fabs(t)*16);
}

/* Function: rt_VerifyTimeMonotone ============================================
 * Abstract:
 * Verify the time vector of inport MAT-file is monotone increase
 * Input argument: 1. Time vector head pointer  
 * 2. number of time point 3. timeData pointer Offset  4 file name  
 *  
 * Returns:
 *	NULL    : success, is monotone
 *	non-NULL: error string
 */
const char* rt_VerifyTimeMonotone(const double* timeDataPtr, 
                                  size_t numOfTimePoints,
                                  int_T timeDataPtrOffset,
                                  const char* inportFileName )
{    
    size_t i;
    static char  errmsg[1024];
    errmsg[0] = '\0'; /* assume success */
        
    /* 1. Verify that numOfTimePoints must >=1 */
    if (numOfTimePoints < 1){
        (void)sprintf(errmsg,"Time vector in MAT-file "
                      " '%s' must have at least one element.\n ",
                      inportFileName);
    }
    
    /*
     * Verify that the time vector is monotonically increasing.
     */    
    for (i=1; i<numOfTimePoints; i++) {
        if (timeDataPtr[i*timeDataPtrOffset] 
            < timeDataPtr[(i-1)*timeDataPtrOffset]) {
            (void)sprintf(errmsg,"Time in MAT-file "
                          " '%s' must be monotonically increasing.\n",
                          inportFileName);
        }
    }
    
    return (errmsg[0] != '\0'? errmsg: NULL);   
}/*rt_VerifyTimeMonotone*/


/* Function: rt_CheckMatFileWithStructVar ======================================
 * Abstract:
 * Check the struct format variable 
 * 
 *  
 * Returns:
 *	NULL    : success, variable is valid
 *	non-NULL: error string
 */
const char *rt_CheckMatFileWithStructVar( mxArray *inputTimePtr,
                                          mxArray *mxInportSignalDimensions,
                                          mxArray *mxInportSignalValues,
                                          int inportIdx)
                                      
{     
    static char  errmsg[1024];   
    boolean_T isOk; 
    size_t numDims;
    mwSize numDimsValues; 
    size_t  numOfTimePoints, numTimePts;   
       
    const mwSize *dimsValues;    
    double *dims; 

    errmsg[0] = '\0'; /* assume success */

    isOk = false;
    numDims = 0;  

    numDimsValues    = mxGetNumberOfDimensions(mxInportSignalValues); 
    dimsValues       = mxGetDimensions(mxInportSignalValues); 

    numTimePts = (numDimsValues == 2) ? 
        mxGetM(mxInportSignalValues) : (size_t)dimsValues[numDimsValues-1];
   
    numOfTimePoints = mxGetNumberOfElements(inputTimePtr);
    
    /* 1. Signal dimension field match signal value dimension, if the field 
     * exists 
     */
     
    if (mxInportSignalDimensions != NULL) {
        /* for signal, there is a field called dimensions. check how many 
         * element in the dimension matrix
         */
        
        numDims = mxGetNumberOfElements(mxInportSignalDimensions);    
        
        /* the dimension is either a scaler or a 1X2 vector */
        /* consult Foundation Libraries before using mxIsIntVectorWrapper G978320 */
        isOk = mxIsIntVectorWrapper(mxInportSignalDimensions) && 
            (numDims == 1 || numDims == 2);
        
	if (isOk) {           
            /* signal dimension value */           
            dims = mxGetPr(mxInportSignalDimensions);   
            if(numDims == 1) {
                /*if signal dimension is 1 then the signal values must be a 
                 * vector TxV and the signal dimension value = V.              
                 */
                isOk    = numDimsValues == 2 && dimsValues[1] == dims[0];
            } else { /* numDims == 2 */
                isOk = (numDimsValues == 3  || numDimsValues == 2) &&
		    (dimsValues[0] == dims[0]) && (dimsValues[1] == dims[1]);
            }
	}
    } else { 
        /* If  no dimension field  */
        
        isOk    = (numDimsValues == 2 || numDimsValues == 3);
    }
    
    if (!isOk) {
        (void)sprintf(errmsg, "Invalid structure-format variable specified in " 
                      " the matfile.  The structure's 'dimensions' field must "
                      " be a scalar or a vector with 2 elements. In addition, "
                      " this field must be compatible with the dimensions of "
                      " input signal stored in the 'values' field.\n");
        goto EXIT_POINT;
    }
                    
    /* 2. Time and data have the same number of rows if have time */
    
    if(numOfTimePoints > 0){
        if(numOfTimePoints != numTimePts){
            (void)sprintf(errmsg, "Invalid structure-format variable. Time "
                          " and data values must have the same number of rows.\n");
            goto EXIT_POINT;
        }
    }
 
    /* 3. Signal value dimension must equal to inport file dimension  
     * numDimsValues must >= 2 
     */
    
    if (numDimsValues == 2){
        if (dimsValues[1] != gblInportDims[inportIdx*2]){
            (void)sprintf(errmsg,"Dimension of signal #%d from MAT-file not " 
                          "equal the dimension of the corresponding inports "
                          "specified in the model.\n", inportIdx+1);
            goto EXIT_POINT;                             
        }
    }
    
    if (numDimsValues == 3){
      if (dimsValues[0] != gblInportDims[inportIdx*2] 
            ||dimsValues[1] != gblInportDims[inportIdx*2+1]){
            (void)sprintf(errmsg,"Dimension of signal #%d from MAT-file not " 
                          "equal the dimension of the corresponding inports "
                          "in specified in the model.\n", inportIdx+1);
            goto EXIT_POINT;                              
        }
    }
    
    /* 4. Signal data type must equal the inport data type */
    {
        int_T            dTypeID = gblInportDataTypeIdx[inportIdx]; 
        mxClassID  mxIDfromModel = rt_GetMxIdFromDTypeIdForRSim(dTypeID);
        mxClassID  mxIDfromFile  = mxGetClassID(mxInportSignalValues);
        
        if((mxIDfromModel != mxUNKNOWN_CLASS) &&
           (mxIDfromModel != mxIDfromFile)){
            (void)sprintf(errmsg,"Data type of signal #%d from MAT-file not "
                          "equal the Data type of the corresponding inports "
                          "specified in the model.\n", inportIdx+1);
            goto EXIT_POINT;                              
        }
    }
    
    /* 5. Check for port complexity mismatch */
    {
        boolean_T isComplexFromModel = mxIsComplex(mxInportSignalValues);
        boolean_T isComplexFromFile  = (bool)gblInportComplex[inportIdx];
        
        if(isComplexFromModel != isComplexFromFile){
            printf("*** Warning: Signal complexity of signal #%d from MAT-file "
                   "is not as same as the signal complexity of the corresponding inports. "
                   "The complex part is ignored. ***\n ", inportIdx+1);
        }
    }
    
EXIT_POINT:
    return (errmsg[0] != '\0'? errmsg: NULL);
} /*rt_CheckMatFileWithStructVar */



/* Function: rt_CheckMatFileStructTimeField ======================================
 * Abstract:
 * Check the struct time field
 * 
 *  
 * Returns:
 *	NULL    : success, Time field is valid
 *	non-NULL: error string
 */
const char* rt_CheckMatFileStructTimeField( mxArray *inputTimePtr, 
                                            int_T inportIdx,
                                            const char* inportFileName)
{                                          
    static char errmsg[1024];
    const char* result;
    int_T i;
    double *timeDataPtr;
    int_T numOfTimePoints;

    int timeDataPtrOffset = 1;
    
    if (inputTimePtr != NULL) {
        numOfTimePoints = (int_T) mxGetNumberOfElements(inputTimePtr);
        /* if time vector is empty, must be discrete sample time 
         * interpolation must be set to off                    */
        
        if (numOfTimePoints == 0) {
            if(inportIdx < 0) {
                for (i = 0; i < gblNumRootInportBlks; ++i) {
                    if (gblInportInterpoFlag[i] != 0) {
                        (void)sprintf(errmsg, 
                                      "The MAT-file %s does not specify any "
                                      "time values and interpolation flag of "
                                      "all root inport blocks must set to "
                                      "off.\n", inportFileName );
                        goto EXIT_POINT; 
                    }                        
                
                    if (gblInportContinuous[i] == 1) {
                        (void)sprintf(errmsg, 
                                      "Invalid sample time for inport "
                                      "blocks. Since the variable in the MAT "
                                      "file %s does not specify any time "
                                      "values, the sample time of all root "
                                      "inport blocks must be discrete.\n", 
                                      inportFileName);
                        goto EXIT_POINT;       
                    }
                }
            } else {
                if (gblInportInterpoFlag[inportIdx] != 0){
                    (void)sprintf(errmsg, 
                                  "Invalid interpolation flag for inport %d. "
                                  "Since the variable in MAT-file %s "
                                  "corresponding to this inport does not "
                                  "specify any time values, the interpolation "
                                  "flag  must set to off.\n", 
                                  inportIdx+1, inportFileName);
                    goto EXIT_POINT; 
                }
                
                if (gblInportContinuous[inportIdx] == 1) {
                    (void)sprintf(errmsg, 
                                  "Invalid sample time for inport %d. Since "
                                  "the variable in MAT-file %s corresponding "
                                  "to the inport does not specify any time "
                                  "values, the sample time must be "
                                  "discrete.\n", inportIdx+1, inportFileName);
                    goto EXIT_POINT; 
                }   
            }               
        }else{
            timeDataPtr = mxGetPr(inputTimePtr);               
            result = rt_VerifyTimeMonotone(timeDataPtr, 
                                           numOfTimePoints, 
                                           timeDataPtrOffset, 
                                           inportFileName);
            if(result !=NULL){
                (void)strcpy(errmsg, result);
                goto EXIT_POINT;
            }
        }
    } else {
        /* time field is not present, error out*/
        (void)sprintf(errmsg, 
                      "Invalid structure-format variable specified as external "
                      "input in MAT-file %s. The variable must include 'time' "
                      "and 'signals' fields. Also, the 'signals' field must be "
                      "a structure with a 'values' field.\n", inportFileName);
        goto EXIT_POINT;        
    }   
    
EXIT_POINT:
    return (errmsg[0] != '\0'? errmsg: NULL);
}/*  rt_CheckMatFileStructTimeField */



/* Function: rt_VerifyInportsMatFile ======================================
 * Abstract:
 * Verify the MAT-file to drive the inport block 
 * 
 *  
 * Returns:
 *	NULL    : success, file is valid
 *	non-NULL: error string
 */

const char* rt_VerifyInportsMatFile(const char* inportFileName, 
                                    int* matFileDataFormat,
                                    int isRaccel,
                                    mxLogical* periodicFunctionCallInports)
{   
    static char  errmsg[1024];
    const char *result; 
    MATFile *pmat= NULL; 
    int nrows = 0;
    int ncols = 0;
    mxClassID  mxIDfromFile;

    int_T numOfTimePoints;
    const char  *inportVariableName;   

    const double *timeDataPtr;
    mxArray* inputTimePtr = NULL;
    mxArray* mxInputPtr= NULL;

    /* inport structure signals */
    mxArray *mxInportSignal = NULL;
    /* signal dimension array */
    mxArray *mxInportSignalDimensions= NULL;
    /* signal value array */
    mxArray *mxInportSignalValues= NULL;
     
    int i = 0;
    int inportIdx;
    int valuesFieldIdx;

    int_T timeDataPtrOffset = 1;
    int_T numVarInMatFile = 0;      
    bool   isStruct = true;     


    if (gblNumRootInportBlks == 0) {
        return NULL;
    } else {
        if (inportFileName == NULL) {
            printf("*** Warning: -i command is not used and there are "
                   "inports block in the model. All inports will be "
                   "set to zero. ***\n ");
            goto EXIT_POINT;
        }
    }
    
    if ((pmat=matOpen(inportFileName,"r")) == NULL) {
        (void)sprintf(errmsg,"could not open MAT-file '%s' containing "
                      "Inport Block data", inportFileName);
        goto EXIT_POINT;
    }
    
    while((mxInputPtr = matGetNextVariable(pmat, &inportVariableName)) != NULL) {
        ++numVarInMatFile;
    }
    matClose(pmat);
    pmat = NULL;
    
    if (numVarInMatFile == 0) {
        (void)sprintf(errmsg,"could not locate any variables in MAT-file '%s'",
                      inportFileName);
        goto EXIT_POINT;
    }
    
    pmat = matOpen(inportFileName,"r");
    mxInputPtr = matGetNextVariable(pmat, &inportVariableName);

    if (isRaccel) {        
        /* If we are in raccel the first variable is the 
         * externalInputIsInDatasetFormat flag.
         */
        mxInputPtr = matGetNextVariable(pmat, &inportVariableName);

        /* In order for the remainder of the parsing to be able to ignore this
         * leading variable we fix the number of variables flag here. */
        numVarInMatFile--;

        /* If we are in raccel the second variable should be a list of flags
         * indicating periodic function call inputs. */
        nrows = (int) mxGetM(mxInputPtr);
        ncols = (int) mxGetN(mxInputPtr);
        if (nrows != 1 || ncols != gblNumRootInportBlks) {
            (void)sprintf(errmsg,"Array dimensions of periodic function call inport "
                          "specification in MAT-file %s was not correct!\n" 
                          "Dimensions must be 1 by the number of root inports.\n",
                          inportFileName);
            goto EXIT_POINT;
        }
        
        mxIDfromFile  = mxGetClassID(mxInputPtr);
        if (mxIDfromFile != mxLOGICAL_CLASS) {
            (void)sprintf(errmsg,"Data type of periodic function call inport "
                          "specification in MAT-file %s was not correct!\n" 
                          "Data type should be boolean.\n", inportFileName);
            goto EXIT_POINT;
        }

        /* Now we can grab the flags from the array and store them in our array for
         * later use */
        (void)memcpy(periodicFunctionCallInports, mxGetLogicals(mxInputPtr),
                     sizeof(mxLogical)*gblNumRootInportBlks);

        /* In order for the remainder of the parsing to be able to ignore this
         * leading variable we fix the number of variables flag here. */
        numVarInMatFile--;
        /* If we are in raccel the third variable is the 
         * hasUnconstrainedPartitionHitTimes flag.
         */
        mxInputPtr = matGetNextVariable(pmat, &inportVariableName);

        /* In order for the remainder of the parsing to be able to ignore this
         * leading variable we fix the number of variables flag here. */
        numVarInMatFile--;
        
        mxInputPtr = matGetNextVariable(pmat, &inportVariableName);
    } else {
        /* We are running an rsim target, and thus should mark all inputs as
         * non-periodic. */
        for (i=0; i<gblNumRootInportBlks; i++) {
            periodicFunctionCallInports[i] = false;
        }
    }

    if (numVarInMatFile == 1 && gblNumRootInportBlks > 0 && 
        (gblInportDataTypeIdx[0] != SS_FCN_CALL)) {
        isStruct = mxIsStruct(mxInputPtr);
        if (!isStruct) {
            /*
             * 1. TU matrix, data type double,  notice that the matrix format 
             *  nCols = gblNumRootInputSignals + 1, nRows = numOfTimePoints.
             */  
            nrows = (int) mxGetM(mxInputPtr);
            ncols = (int) mxGetN(mxInputPtr);    
            mxIDfromFile  = mxGetClassID(mxInputPtr);
            if (mxIsEmpty(mxInputPtr)) goto EXIT_POINT;

            if(mxIDfromFile != rt_GetMxIdFromDTypeId(SS_DOUBLE)){
                (void)sprintf(errmsg,"Data type of matrix variable from "
                              "MAT-file %s is not double!\n" , inportFileName);
                goto EXIT_POINT;
            }

            if (ncols < 2) {
                (void)sprintf(errmsg, "Matrix variable from MAT-file "
                              "'%s' must contain at least 2 columns.\n",
                              inportFileName);
                goto EXIT_POINT;
            }
        
            if (ncols != gblNumModelInputs + 1) {
                (void)sprintf(errmsg,
                              "The number of columns of matrix variable from "
                              "MAT-file '%s' must equal %d, which is the "
                              "number of root inports plus 1.\n ", 
                              inportFileName, gblNumModelInputs + 1);
                goto EXIT_POINT;
            }
        
            for (i = 0; i < gblNumRootInportBlks; ++i) {
                /* check complexity, only real */         
                if (gblInportComplex[i] == 1) {
                    printf("*** Warning: Signal type of matrix variable from "
                           "MAT-file %s can only be real while signal type "
                           "of inport %d is set to complex. The imaginary part " 
                           "is ignored. ***\n", inportFileName, i);
                }
                
                /* check data type, only double */         
                if (gblInportDataTypeIdx[i] != SS_DOUBLE) {  
                    (void)sprintf(errmsg,"Data type of matrix variable from "
                                  "MAT-file %s can only be double while data "
                                  "type of inport %d is not set to double.\n", 
                                  inportFileName, i);
                    goto EXIT_POINT;
                }
            
                /* Check dimension, only vector is supported */
            
                if(gblInportDims[2*i + 1] != 1){
                    (void)sprintf(errmsg, 
                                  "The dimensions of inport %d require that "
                                  "the input variable must be a structure, "
                                  "while the variable in the file %s is in TU "
                                  "matrix format. This format can only be used "
                                  "if all the input signals are real, double "
                                  "vectors.\n", i, inportFileName);
                    goto EXIT_POINT; 
                }
            
            }
        
            timeDataPtr = mxGetPr(mxInputPtr);
            numOfTimePoints     = nrows; 
            result = rt_VerifyTimeMonotone(timeDataPtr, 
                                           numOfTimePoints,
                                           timeDataPtrOffset,
                                           inportFileName);    
            if(result != NULL){
                (void)strcpy(errmsg, result);
                goto EXIT_POINT;
            }

            *matFileDataFormat = SINGLEVAR_MATRIX;
            goto EXIT_POINT;
        
        } else {
            /*
             * Verify that the time vector is monotonically increasing.
             * only one time vector needs to be checked.
             * Check of time value is given, if not interpolation must be off
             * and sample time must be discrete
             */
            int_T  numInportSignals;
            inputTimePtr  = mxGetField(mxInputPtr, 0, "time");        
            result = rt_CheckMatFileStructTimeField(inputTimePtr, 
                                                    -1, 
                                                    inportFileName);
            if(result != NULL){
                (void)strcpy(errmsg, result);
                goto EXIT_POINT;
            }

            mxInportSignal = mxGetField(mxInputPtr, 0, "signals");
        
            if(mxInportSignal == NULL){
                (void)sprintf(errmsg, "Variable %s in MAT-file '%s' contains "
                              "no 'signals' field ", 
                              inportVariableName, inportFileName);
                goto EXIT_POINT;
            }

            numInportSignals = (int_T) mxGetN(mxInportSignal);
        
            if(numInportSignals != gblNumRootInportBlks){
                (void)sprintf(errmsg, 
                              "MAT-file '%s' contains %d signals while model "
                              "contains %d inports, they must be equal.\n ",
                              inportFileName, numInportSignals, 
                              gblNumRootInportBlks);
                goto EXIT_POINT;
            }
        }
        *matFileDataFormat = SINGLEVAR_STRUCT;

        /*
         * check each signal's dimension information, data type information  
         */
        for (inportIdx = 0; inportIdx < gblNumRootInportBlks; ++inportIdx){
            
            mxInportSignalDimensions = mxGetField(mxInportSignal, inportIdx,
                                                  "dimensions"); 
            valuesFieldIdx = mxGetFieldNumber(mxInportSignal, "values");
            if (valuesFieldIdx == -1) {
                (void)sprintf(errmsg, 
                              "External inputs structure must contain "
                              "'signals.values' field");
                goto EXIT_POINT;
            }
            mxInportSignalValues = mxGetFieldByNumber(mxInportSignal, inportIdx, 
                                                      valuesFieldIdx);
            if (mxInportSignalValues == NULL) {
                (void)sprintf(errmsg, 
                              "The data type of the 'values' field of 'signals' "
                              "structure for the input port '%d' is not "
                              "supported in rapid accelerator", (inportIdx+1));
                goto EXIT_POINT;
            }
        
            result = rt_CheckMatFileWithStructVar(inputTimePtr, 
                                                  mxInportSignalDimensions,
                                                  mxInportSignalValues,
                                                  inportIdx);
            if(result != NULL){
                (void)strcpy(errmsg, result);
                goto EXIT_POINT;
            }
        }
    } else if (numVarInMatFile != gblNumRootInportBlks) {
        (void)sprintf(errmsg,"MAT-file '%s' contains %d signals "
                      "while model contain %d inports. ",
                      inportFileName, numVarInMatFile, gblNumRootInportBlks);
        goto EXIT_POINT;               
    } else {
        *matFileDataFormat = MULTIPLEVAR_LIST;
        /*
         * check each signal's dimension information, data type information  
         */
        for (inportIdx = 0; inportIdx < gblNumRootInportBlks; ++inportIdx){

            if (!mxIsStruct(mxInputPtr)) {
                int_T portWidth  = 
                    gblInportDims[inportIdx*2]*gblInportDims[inportIdx*2 + 1];
                nrows = (int) mxGetM(mxInputPtr);
                ncols = (int) mxGetN(mxInputPtr);    
                mxIDfromFile  = mxGetClassID(mxInputPtr);

                if (mxIsEmpty(mxInputPtr)){

                    mxInputPtr = matGetNextVariable(pmat, &inportVariableName);
                    continue;
                }

                if(mxIDfromFile != rt_GetMxIdFromDTypeId(SS_DOUBLE)){
                    (void)sprintf(errmsg,"Data type of matrix variable for "
                                  "inport  %d is not double!\n" , 
                                  inportIdx);
                    goto EXIT_POINT;
                }

                if ((gblInportDataTypeIdx[inportIdx] != SS_FCN_CALL &&
                     ncols != portWidth + 1) ||
                    (gblInportDataTypeIdx[inportIdx] == SS_FCN_CALL &&
                     ncols != portWidth)) {
                    (void)sprintf(errmsg,
                                  "The number of columns of matrix variable for "
                                  "inport %d must equal %d ", 
                                  inportIdx,
                                  (gblInportDataTypeIdx[inportIdx]==SS_FCN_CALL)
                                  ? portWidth:portWidth + 1);
                    goto EXIT_POINT;
                }
        
                if (gblInportComplex[inportIdx] == 1) {
                    printf("*** Warning: Signal type of matrix variable from "
                           "MAT-file %s can only be real while signal type "
                           "of inport %d is set to complex. The imaginary part " 
                           "is ignored. ***\n", inportFileName, inportIdx);
                }
                
                /* check data type, only double or function call*/         
                if (gblInportDataTypeIdx[inportIdx] != SS_DOUBLE &&
                    gblInportDataTypeIdx[inportIdx] != SS_FCN_CALL) {  
                    (void)sprintf(errmsg,"Data type of matrix variable from "
                                  "MAT-file %s can only be double while data "
                                  "type of inport %d is not set to double.\n", 
                                  inportFileName, inportIdx);
                    goto EXIT_POINT;
                }
                
                if (periodicFunctionCallInports[inportIdx] == true)
                {
                    if (nrows != 2) {
                        (void)sprintf(errmsg,"The number of rows of matrix "
                                      "variable for inport %d must equal 2.\n",
                                      inportIdx);
                        goto EXIT_POINT;
                    }
                } else {
                    
                    timeDataPtr = mxGetPr(mxInputPtr);
                    numOfTimePoints     = nrows; 
                    result = rt_VerifyTimeMonotone(timeDataPtr, 
                                                   numOfTimePoints,
                                                   timeDataPtrOffset,
                                                   inportFileName);    
                    if(result != NULL){
                        (void)strcpy(errmsg, result);
                        goto EXIT_POINT;
                    }
                }

            } else {
                if (gblInportDataTypeIdx[inportIdx] == SS_FCN_CALL) {
                    (void)sprintf(errmsg, "inport %d is configured to output a "
                                  "function call and the data should be in a "
                                  "column vector of monotonically increasing "
                                  "time values ", inportIdx);
                    goto EXIT_POINT;
                }
                
                mxInportSignal = mxGetField(mxInputPtr, 0, "signals"); 
                if (mxInportSignal == NULL) {
                    (void)sprintf(errmsg, "Variable '%s' in  MAT-file '%s' "
                                  "contains no 'signals' field ",
                                  inportVariableName, inportFileName);
                    goto EXIT_POINT;
                }
                inputTimePtr = mxGetField(mxInputPtr, 0, "time");
                result = rt_CheckMatFileStructTimeField(inputTimePtr, 
                                                        inportIdx,
                                                        inportFileName);
                if(result != NULL){
                    (void)strcpy(errmsg, result);
                    goto EXIT_POINT;
                } 
                
                mxInportSignalDimensions = mxGetField(mxInportSignal, 0,
                                                      "dimensions"); 
                
                valuesFieldIdx = mxGetFieldNumber(mxInportSignal, "values");
                if (valuesFieldIdx == -1) {
                    (void)sprintf(errmsg, 
                                  "External inputs structure must contain "
                                  "'signals.values' field");
                    goto EXIT_POINT;
                }
                mxInportSignalValues = mxGetFieldByNumber(mxInportSignal, 0, 
                                                          valuesFieldIdx);
                if (mxInportSignalValues == NULL) {
                    (void)sprintf(errmsg, 
                                  "The data type of the 'values' field of "
                                  "'signals' structure for the input port '%d' "
                                  "is not supported in rapid accelerator", 
                                  (inportIdx+1));
                    goto EXIT_POINT;
                }
                
                result = rt_CheckMatFileWithStructVar(inputTimePtr, 
                                                      mxInportSignalDimensions,
                                                      mxInportSignalValues,
                                                      inportIdx);
                if(result != NULL){
                    (void)strcpy(errmsg, result);
                    goto EXIT_POINT;
                }
            }
            mxInputPtr = matGetNextVariable(pmat, &inportVariableName); 
        }
    }



EXIT_POINT:
    
    if (pmat!=NULL) {
        matClose(pmat); pmat= NULL;
    }
    
    if (mxInputPtr != NULL) {
        mxDestroyArray(mxInputPtr);
    }
    
    return (errmsg[0] != '\0'? errmsg: NULL);   

}/*  const char* rt_VerifyInportsMatFile */


static const char* setGblInportTUtableElement(int_T inportIdx,
                                              size_t numOfTimePoints,
                                              double* inportTimeDataPtr,
                                              size_t elementSize,
                                              bool isComplex,
                                              bool isPeriodicFcnCall,
                                              char* matDataRe,
                                              char* matDataIm) {
    int_T portWidth  = 
        gblInportDims[inportIdx*2]*gblInportDims[inportIdx*2 + 1];
    gblInportTUtables[inportIdx].ur = NULL;
    gblInportTUtables[inportIdx].ui = NULL;
    gblInportTUtables[inportIdx].time = NULL; 
    
    gblInportTUtables[inportIdx].complex = isComplex ? 1 : 0;    
    gblInportTUtables[inportIdx].isPeriodicFcnCall = isPeriodicFcnCall;
        
    /* set TU Table attribute */
    gblInportTUtables[inportIdx].nTimePoints = (int_T) numOfTimePoints;        
    gblInportTUtables[inportIdx].uDataType = 
        gblInportDataTypeIdx[inportIdx]; 
    /* Periodic function call ports do not use the time data in the same
     * way. See rt_RapidReadInportsMatFile for details. */
    gblInportTUtables[inportIdx].currTimeIdx =
        (inportTimeDataPtr && !isPeriodicFcnCall) ? 0 : -1;  

    if (numOfTimePoints == 0) return NULL;

    gblInportTUtables[inportIdx].time = inportTimeDataPtr;

    if (elementSize > 0) {
        /* allocate memory */
        gblInportTUtables[inportIdx].ur = 
            (char*)calloc(numOfTimePoints*portWidth,elementSize);     
        
        if (gblInportTUtables[inportIdx].ur == NULL) {
            return "Memory allocation error";
        }            
        
        if(gblInportTUtables[inportIdx].complex == 1){
            gblInportTUtables[inportIdx].ui = 
                (char*)calloc(numOfTimePoints*portWidth,elementSize);     
            if (gblInportTUtables[inportIdx].ui == NULL) {
                return "Memory allocation error"; 
            }
        }          
        
        /* now we can steal the data from the mxArray and 
         * generate tu table */
    
        (void)memcpy(gblInportTUtables[inportIdx].ur, matDataRe, 
                     elementSize*numOfTimePoints*portWidth);
    
        if (isComplex){                    
            (void)memcpy(gblInportTUtables[inportIdx].ui, matDataIm, 
                         elementSize*numOfTimePoints*portWidth);
        }
    } else {
        gblInportTUtables[inportIdx].ur = matDataRe;
        gblInportTUtables[inportIdx].ui = matDataIm;
    }
    return NULL;
}


/* Function:rt_ConvertInportsMatDatatoTUtable==================================
 * Abstract:
 * Convert the MatData to TUtable in memory
 * 
 *  
 * Returns:
 *	NULL    : success, Data is in memory
 *	non-NULL: error string
 */

const char* rt_ConvertInportsMatDatatoTUtable(int_T matFileDataFormat,
                                              const char* inportFileName,
                                              int isRaccel,
                                              const mxLogical* periodicFunctionCallInports
                                              )
{
    /* if we get here , the file is valid, need to read in the file to 
     * inportUTtable. Each inport has its own tu table, it is because
     * we assume the user will change the MAT-file format after the 
     * code is generated and don't want to recompile 
     */
    
    static char  errmsg[1024];
    const char* result = NULL;
    char *matDataRe = NULL;
    char *matDataIm = NULL;
    double* inportTimeDataPtr = NULL;
    bool isComplex = false;    
    size_t numOfTimePoints = 0; /* invalid value */
    int_T  portWidth;
    int_T  inportIdx;
    size_t elementSize = 0; /* invalid value */
#if defined(MX_HAS_INTERLEAVED_COMPLEX)
    void *matData = NULL;
#endif
  
    mxArray *mxInportSignal;
    const double* timeDataPtr = NULL;       
    double *dataPtr;

    mxArray* inputTimePtr;
    mxArray* mxInportSignalValues = NULL;
    
  
    MATFile *pmat= matOpen(inportFileName,"r");   
    mxArray *mxInputPtr = matGetNextVariable(pmat, NULL); 
    if (isRaccel) {
        /* If we are in raccel the first variable is the 
         * externalInputIsInDatasetFormat flag.
         */
        mxInputPtr = matGetNextVariable(pmat, NULL);

        /* The second variable is the set of periodic function call flags. */
        mxInputPtr = matGetNextVariable(pmat, NULL);

        /* The third variable is hasUnconstrainedPartitionHitTimes flag */
        mxInputPtr = matGetNextVariable(pmat, NULL);
    }

    errmsg[0] = '\0'; /* assume success */
     
    /* allocate memory for tu table */
    
    gblInportTUtables = (rtInportTUtable*)
        malloc(sizeof(rtInportTUtable)*gblNumRootInportBlks);
    
    if (gblInportTUtables == NULL) {
        (void)sprintf(errmsg,"Memory allocation error"); 
        goto EXIT_POINT;
    }

    if (matFileDataFormat == SINGLEVAR_MATRIX) {
        numOfTimePoints = mxGetM(mxInputPtr);                

        if (numOfTimePoints > 0) {                
            timeDataPtr  = mxGetPr(mxInputPtr);         
            elementSize  = sizeof(double);
            dataPtr = mxGetPr(mxInputPtr); 
            matDataRe = (char*)dataPtr + numOfTimePoints*elementSize;
            
            inportTimeDataPtr =  
                (double*)calloc(numOfTimePoints,sizeof(double));
            if (inportTimeDataPtr == NULL) {
                (void)sprintf(errmsg,"Memory allocation error"); 
                goto EXIT_POINT;
            }
            (void)memcpy(inportTimeDataPtr, timeDataPtr, 
                         sizeof(double)*numOfTimePoints);
            
        } else {
            inportTimeDataPtr = NULL;
            elementSize  = 0;
            matDataRe = NULL;
        }
        for (inportIdx = 0; inportIdx < gblNumRootInportBlks; ++inportIdx) {
            result = setGblInportTUtableElement(inportIdx, numOfTimePoints, 
                                                inportTimeDataPtr, elementSize, 
                                                false, false, matDataRe, NULL);
            if (result != NULL){
                (void)strcpy(errmsg, result);
                goto EXIT_POINT;
            }

            portWidth  = 
                gblInportDims[inportIdx*2]*gblInportDims[inportIdx*2 + 1];
            matDataRe = matDataRe + portWidth*numOfTimePoints*elementSize;
        }
    } else if (matFileDataFormat == SINGLEVAR_STRUCT) {

        inputTimePtr  = mxGetField(mxInputPtr, 0, "time");
        mxInportSignal = mxGetField(mxInputPtr, 0 , "signals");
        numOfTimePoints = mxGetNumberOfElements(inputTimePtr);
        timeDataPtr = numOfTimePoints == 0 ? NULL : mxGetPr(inputTimePtr);

        if (timeDataPtr) {
            inportTimeDataPtr =  
                (double*)calloc(numOfTimePoints,sizeof(double));

            if (inportTimeDataPtr == NULL) {
                (void)sprintf(errmsg,"Memory allocation error"); 
                goto EXIT_POINT;
            }
            (void)memcpy(inportTimeDataPtr, timeDataPtr, 
                         sizeof(double)*numOfTimePoints);
        }

        for (inportIdx = 0; inportIdx < gblNumRootInportBlks; ++inportIdx) {

            mwSize numDimsValues;
            const mwSize *dimsValues;       
            mxInportSignalValues = mxGetField(mxInportSignal, inportIdx, 
                                              "values");
            numOfTimePoints = mxGetNumberOfElements(inputTimePtr);
            numDimsValues   = mxGetNumberOfDimensions(mxInportSignalValues);
            dimsValues      = mxGetDimensions(mxInportSignalValues); 
            if(numOfTimePoints == 0){                
                numOfTimePoints = ((numDimsValues == 2) ? 
                                   mxGetM(mxInportSignalValues) : 
                                   (size_t)dimsValues[numDimsValues-1]);
            }
                              
            isComplex = ((gblInportComplex[inportIdx] != 
                          mxIsComplex(mxInportSignalValues))? 
                         false : (bool) gblInportComplex[inportIdx]);  
            elementSize = mxGetElementSize(mxInportSignalValues);
#if defined(MX_HAS_INTERLEAVED_COMPLEX)
            /* Conversion to non-interleaved. */
            matData = mxGetData(mxInportSignalValues);
            if(isComplex) {
                size_t halfElementSize = elementSize/2;
                size_t ncols = mxGetN(mxInportSignalValues);
                matDataRe = (char*)calloc(numOfTimePoints*ncols,halfElementSize);
                matDataIm = (char*)calloc(numOfTimePoints*ncols,halfElementSize);
                deInterleaveComplex(matDataRe, matDataIm, matData, 
                                    halfElementSize, numOfTimePoints, ncols);
            } else {
                matDataRe = matData;
            }
            /* Stay with old non-interleaved model beyond this point for now. */
#else
            matDataRe = mxGetData(mxInportSignalValues);
            matDataIm = isComplex ? mxGetImagData(mxInportSignalValues) : NULL;
#endif
            result = setGblInportTUtableElement(inportIdx, numOfTimePoints, 
                                                inportTimeDataPtr, elementSize, 
                                                isComplex, false, matDataRe, matDataIm);
#if defined(MX_HAS_INTERLEAVED_COMPLEX)
            /* setGblInportTUtableElement() has made a copy. */
            if(isComplex) {
                /* In the real case, we are just using the original matrix directly.
                   This is already getting freed.  In the complex case, we calloc()'ed
                   additional arrays, free them now. */
                free(matDataRe);
                free(matDataIm);
            }
#endif
            if (result != NULL){
                (void)strcpy(errmsg, result);
                goto EXIT_POINT;
            }
        }
    } else {

        for (inportIdx = 0; inportIdx < gblNumRootInportBlks; ++inportIdx) {

            if (!mxIsStruct(mxInputPtr)) {
                numOfTimePoints = mxGetM(mxInputPtr);
                if (numOfTimePoints > 0) {
                    timeDataPtr  = mxGetPr(mxInputPtr);         
                    if (gblInportDataTypeIdx[inportIdx] != SS_FCN_CALL) {
                        elementSize  = sizeof(double);
                        dataPtr = mxGetPr(mxInputPtr); 
                        matDataRe = (char*)dataPtr + numOfTimePoints*elementSize;
                        inportTimeDataPtr =  
                            (double*)calloc(numOfTimePoints,sizeof(double));
                        if (inportTimeDataPtr == NULL) {
                            (void)sprintf(errmsg,"Memory allocation error"); 
                            goto EXIT_POINT;
                        }
                        (void)memcpy(inportTimeDataPtr, timeDataPtr, 
                                     sizeof(double)*numOfTimePoints);
                    } else if (periodicFunctionCallInports[inportIdx]) {
                        elementSize = 0;
                        matDataRe = NULL;
                        inportTimeDataPtr =
                            (double*) calloc(numOfTimePoints,sizeof(double));
                        inportTimeDataPtr[0] = timeDataPtr[0];
                        inportTimeDataPtr[1] = timeDataPtr[1];
                    } else {
                        int timeDataIdx = 0;
                        int aggTimeIdx = 0;
                        uint_T *matDataReFcnCall= NULL;
                        int initialNumOfTimePoints= (int) numOfTimePoints;
                        /* First loop to figure out how many time points we
                         * actually have: take out duplicates due to
                         * multiple function calls. */
                        for (timeDataIdx = 0; timeDataIdx < initialNumOfTimePoints; 
                             ++timeDataIdx) {
                            if (timeDataPtr[timeDataIdx] == 
                                timeDataPtr[timeDataIdx-1]) {
                                /* This means just one less data point in the total since
                                 *  this is just an increment of the previous.
                                 */
                                --numOfTimePoints; 
                            }
                        }
                        /* Restore the time data index (for the next loop). */
                        timeDataIdx= 0;
                        /* Allocate the arrays for time and data based on the
                           new count taking into account duplicates. */
                        inportTimeDataPtr =  
                            (double*)calloc(numOfTimePoints,sizeof(double));
                        matDataReFcnCall =
                            (uint_T*)calloc(numOfTimePoints,sizeof(uint_T));
                        /* Set element size to zero, this will be handled as a 
                         * special case for the function call inputs in
                         * setGblInportTUtableElement(). 
                         */
                        elementSize = 0;
                        /* Handle the first data point separately. */
                        inportTimeDataPtr[aggTimeIdx] = timeDataPtr[timeDataIdx];
                        matDataReFcnCall[aggTimeIdx] = 1U;
                        /* Loop over the rest of the data points. */
                        for (timeDataIdx = 1; timeDataIdx < initialNumOfTimePoints; 
                             ++timeDataIdx) {
                            if (timeDataPtr[timeDataIdx] == 
                                timeDataPtr[timeDataIdx-1]) {
                                /* Multiple function call case. */
                                ++matDataReFcnCall[aggTimeIdx]; 
                            } else {
                                /* New time data point. */
                                ++aggTimeIdx;
                                inportTimeDataPtr[aggTimeIdx] = 
                                    timeDataPtr[timeDataIdx];
                                matDataReFcnCall[aggTimeIdx] = 1U;
                            }
                        }
                        /* Cast the uint_T pointer we used into the 
                           "standard" char pointer we use. */
                        matDataRe= (char *)matDataReFcnCall;
                    }
                } else {
                    inportTimeDataPtr = NULL;
                    elementSize  = 0;
                    matDataRe = NULL;
                }

                result = setGblInportTUtableElement(
                    inportIdx, numOfTimePoints, inportTimeDataPtr, 
                    elementSize, false, periodicFunctionCallInports[inportIdx],
                    matDataRe, NULL);
                if (result != NULL) {
                    (void)strcpy(errmsg, result);
                    goto EXIT_POINT;
                }
                
            } else {
                mwSize numDimsValues;
                const mwSize* dimsValues;       
                inputTimePtr  = mxGetField(mxInputPtr, 0, "time");
                mxInportSignal = mxGetField(mxInputPtr, 0 , "signals");
                numOfTimePoints = mxGetNumberOfElements(inputTimePtr);
                timeDataPtr = (numOfTimePoints == 0 ? NULL : 
                               mxGetPr(inputTimePtr));
                if (timeDataPtr) {
                    inportTimeDataPtr =  
                        (double*)calloc(numOfTimePoints,sizeof(double));
                    if (inportTimeDataPtr == NULL) {
                        (void)sprintf(errmsg,"Memory allocation error"); 
                        goto EXIT_POINT;
                    }
                    (void)memcpy(inportTimeDataPtr, timeDataPtr, 
                                 sizeof(double)*numOfTimePoints);
                } else {
                    inportTimeDataPtr = NULL;
                }

                mxInportSignalValues = mxGetField(mxInportSignal, 0, "values");
                numDimsValues   = mxGetNumberOfDimensions(mxInportSignalValues);
                dimsValues      = mxGetDimensions(mxInportSignalValues); 
                if(numOfTimePoints == 0){                
                    numOfTimePoints = ((numDimsValues == 2) ? 
                                       mxGetM(mxInportSignalValues) : 
                                       (size_t)dimsValues[numDimsValues-1]);
                }
                              
                isComplex = ((gblInportComplex[inportIdx] != 
                              mxIsComplex(mxInportSignalValues))? 
                             false : (bool) gblInportComplex[inportIdx]);  
                elementSize = mxGetElementSize(mxInportSignalValues);
#if defined(MX_HAS_INTERLEAVED_COMPLEX)
                /* Conversion to non-interleaved. */
                matData = mxGetData(mxInportSignalValues);
                if(isComplex) {
                    size_t halfElementSize = elementSize/2;
                    size_t ncols = mxGetN(mxInportSignalValues);
                    matDataRe = (char*)calloc(numOfTimePoints*ncols,halfElementSize);
                    matDataIm = (char*)calloc(numOfTimePoints*ncols,halfElementSize);
                    deInterleaveComplex(matDataRe, matDataIm, matData, 
                                        halfElementSize, numOfTimePoints, ncols);
                } else {
                    matDataRe = matData;
                }
                /* Stay with old non-interleaved model beyond this point for now. */
#else
                matDataRe = mxGetData(mxInportSignalValues);
                matDataIm = isComplex ? mxGetImagData(mxInportSignalValues) : NULL;
#endif
                result = setGblInportTUtableElement(
                    inportIdx, numOfTimePoints, inportTimeDataPtr, elementSize,
                    isComplex, false, matDataRe, matDataIm);

#if defined(MX_HAS_INTERLEAVED_COMPLEX)
            /* setGblInportTUtableElement() has made a copy. */
            if(isComplex) {
                /* In the real case, we are just using the original matrix directly.
                   This is already getting freed.  In the complex case, we calloc()'ed
                   additional arrays, free them now. */
                free(matDataRe);
                free(matDataIm);
            }
#endif          
                if (result != NULL){
                    (void)strcpy(errmsg, result);
                    goto EXIT_POINT;
                }
            }
            mxInputPtr = matGetNextVariable(pmat, NULL);
        }

    }

EXIT_POINT:
    
    if (pmat!=NULL) {
        matClose(pmat); pmat = NULL;
    }
    
    if (mxInputPtr != NULL) {
        mxDestroyArray(mxInputPtr);
    }
    
    return (errmsg[0] != '\0'? errmsg: NULL);   
} /* rt_ConvertInportsMatDatatoTUtable */        



/* Function: FreeFNamePairList ================================================
 * Abstract:
 *	Free name pair lists.
 */
static void FreeFNamePairList(FNamePair *list, int_T len)
{
    if (list != NULL) {
        int_T i;
        for (i = 0; i < len; i++) {
            if (list[i].oldName != NULL) free(list[i].oldName);
        }
        free(list);
    }

} /* end FreeFFnameList */


/*==================*
 * Visible routines *
 *==================*/

/* Function: rt_RapidReadFromFileBlockMatFile ============================================

 *
 * Abstract:
 *      This function opens a "fromfile" matfile containing a TU matrix.
 *      The first row of the TU matrix contains a time vector, while
 *      successive rows contain one or more U vectors. This function
 *      expects to find one and only one matrix in the
 *      matfile which must be named "TU".
 *
 *      originalWidth    = only the number of U channels (minimum is 1)
 *      nptsPerSignal    = the length of the T vector.
 *      nptsTotal        = total number of point in entire TU matrix.
 *                         npoints equals: nptsPerChannel * (nchannels + 1)
 *
 * Returns:
 *	NULL    : success
 *      non-NULL: error message
 */
const char *rt_RapidReadFromFileBlockMatFile(const char *origFileName,
                                   int originalWidth,
                                   FrFInfo * frFInfo)
{
    static char  errmsg[1024];
    MATFile      *pmat;
    mxArray      *tuData_mxArray_ptr = NULL;
    const double *matData;
    size_t       nbytes;
    int          nrows, ncols;
    int          rowIdx, colIdx;
    const char   *matFile;

    errmsg[0] = '\0'; /* assume success */

    /******************************************************************
     * Remap the "original" MAT-filename if told to do by user via a *
     * -f command line switch.                                        *
     ******************************************************************/
    {
        int_T i;

        frFInfo->origFileName  = origFileName;
        frFInfo->originalWidth = originalWidth;
        frFInfo->newFileName   = origFileName; /* assume */

        for (i=0; i<gblNumFrFiles; i++) {
            if (gblFrFNamepair[i].newName != NULL && \
                strcmp(origFileName, gblFrFNamepair[i].oldName)==0) {
                frFInfo->newFileName = gblFrFNamepair[i].newName; /* remap */
                gblFrFNamepair[i].remapped = 1;
                break;
            }
        }
    }

    if ((pmat=matOpen(matFile=frFInfo->newFileName,"r")) == NULL) {
        (void)sprintf(errmsg,"could not open MAT-file '%s' containing "
                      "From File Block data", matFile);
        goto EXIT_POINT;
    }

    if ( (tuData_mxArray_ptr=matGetNextVariable(pmat,NULL)) == NULL) {
        (void)sprintf(errmsg,"could not locate a variable in MAT-file '%s'",
                      matFile);
        goto EXIT_POINT;
    }

    nrows= (int) mxGetM(tuData_mxArray_ptr);
    if ( nrows<2 ) {
        (void)sprintf(errmsg,"\"From File\" matrix variable from MAT-file "
                      "'%s' must contain at least 2 rows", matFile);
        goto EXIT_POINT;
    }

    ncols= (int) mxGetN(tuData_mxArray_ptr);

    frFInfo->nptsPerSignal = ncols;
    frFInfo->nptsTotal     = nrows * ncols;

    /* Don't count Time as part of output vector width */
    if (frFInfo->originalWidth != nrows) {
        /* Note, origWidth is determined by fromfile.tlc */
        (void)sprintf(errmsg,"\"From File\" number of rows in MAT-file "
                      "'%s' must match original number of rows", matFile);
        goto EXIT_POINT;
    }

    matData = mxGetPr(tuData_mxArray_ptr);

    /*
     * Verify that the time vector is monotonically increasing.
     */
    {
        int i;
        for (i=1; i<ncols; i++) {
            if (matData[i*nrows] < matData[(i-1)*nrows]) {
                (void)sprintf(errmsg,"Time in \"From File\" MAT-file "
                              "'%s' must be monotonically increasing",
                              matFile);
                goto EXIT_POINT;
            }
        }
    }

    /*
     * It is necessary to have the same number of input signals as
     * in the original model. It is NOT necessary for the signals to
     * have the same signal length as in the original model. They
     * can be substantially larger if desired.
     */
    nbytes = (size_t)(nrows * ncols * (size_t)sizeof(double));

    if ((frFInfo->tuDataMatrix = (double*)malloc(nbytes)) == NULL) {
        (void)sprintf(errmsg,"memory allocation error "
                      "(rt_RapidReadFromFileBlockMatFile %s)", matFile);
        goto EXIT_POINT;
    }

    /* Copy and transpose data into "tuDataMatrix" */
    for (rowIdx=0; rowIdx<frFInfo->originalWidth; rowIdx++) {
        for (colIdx=0; colIdx<frFInfo->nptsPerSignal; colIdx++) {
            frFInfo->tuDataMatrix[colIdx + rowIdx*frFInfo->nptsPerSignal] =
                matData[rowIdx + colIdx*frFInfo->originalWidth];
        }
    }


EXIT_POINT:

    if (pmat!=NULL) {
        matClose(pmat);
        pmat = NULL;
    }

    if (tuData_mxArray_ptr != NULL) {
        mxDestroyArray(tuData_mxArray_ptr);
    }

    return (errmsg[0] != '\0'? errmsg: NULL);

} /* end rt_RapidReadFromFileBlockMatFile */


/* Function: rt_RapidReadInportsMatFile ============================================
 *
 * Abstract:
 *      This function opens a "Inport matfile" and read it to InportTUTable    
 *      
 *      
 *      
 * Returns:
 *	NULL    : success
 *      non-NULL: error message
 */
const char *rt_RapidReadInportsMatFile(const char* inportFileName, int* matFileFormat, int isRaccel)              
{
    static char  errmsg[1024];
    const  char* result;

    int_T fileDataFormat = -1; 
    
    MATFile      *pmat = NULL;
    mxArray      *inportData_mxArray_ptr = NULL;
    mxLogical    *periodicFunctionCallInports = NULL;
    bool          externalInputIsInDatasetFormat = false;
   
    errmsg[0] = '\0'; /* assume success */
    
    /* no root inports return */
    if (gblNumRootInportBlks == 0){
        return NULL;
    } else {
        if (inportFileName == NULL) {
            printf("*** Warning: -i command is not used and there are "
                   "inports block in the model. All inports will be "
                   "set to zero. ***\n ");
            goto EXIT_POINT;
        }
    }
    
    if (isRaccel) {
        void *pISigstreamManager = rt_GetISigstreamManager();
        rtwISigstreamManagerGetDatasetInputFromMatFile(pISigstreamManager, inportFileName);
        rtwISigstreamManagerGetInputIsInDatasetFormat(
            pISigstreamManager, 
            &externalInputIsInDatasetFormat
            );
        if (externalInputIsInDatasetFormat) {            
            goto EXIT_POINT;
        }
    }

    periodicFunctionCallInports = malloc(sizeof(mxLogical)*gblNumRootInportBlks);
    if (periodicFunctionCallInports == NULL) {
        (void)sprintf(errmsg,"Memory allocation error"); 
        goto EXIT_POINT;
    }

    *matFileFormat = fileDataFormat; 
    result = rt_VerifyInportsMatFile(inportFileName,
                                     matFileFormat,
                                     isRaccel,
                                     periodicFunctionCallInports);
                                         
    if (result != NULL){
        (void)strcpy(errmsg, result);
        goto EXIT_POINT;
    }
    
    /* reach here, data file is valid, steal data to TU table*/
    result  = rt_ConvertInportsMatDatatoTUtable(*matFileFormat,
                                                inportFileName,
                                                isRaccel,
                                                periodicFunctionCallInports);
    if (result != NULL){
        (void)strcpy(errmsg, result);
        goto EXIT_POINT;
    }
    /* Reach here, data is successfully loaded */
    printf(" *** %s is successfully loaded! ***\n", inportFileName);

EXIT_POINT:
    
    if (pmat!=NULL) {
        matClose(pmat); pmat = NULL;
    }
    
    if (inportData_mxArray_ptr != NULL) {
        mxDestroyArray(inportData_mxArray_ptr);
    }

    if( errmsg[0] != '\0'){
        gblInportFileName = NULL;
        return errmsg;
    }else{
        return NULL;
    }
} /* end rt_RapidReadInportsMatFile */


/* Function:  Interpolate_Datatype================================
 * Abstract:
 *      Performs Lagrange interpolation on a pair of data values of
 *      specified data type.
 *
 */

void rt_Interpolate_Datatype(void   *x1, void   *x2, void   *yout,
    				    real_T t,   real_T t1,  real_T t2,
                                    int    outputDType)
{
    real_T  out;
    real_T  f1 = (t2 - t) / (t2 - t1);
    real_T  f2 = 1.0 - f1;

    switch(outputDType){

      case SS_DOUBLE:
  	  out = Interpolate(*(real_T *)x1, *(real_T *)x2, f1, f2);
          *(real_T *) yout = out;
          break;

      case SS_SINGLE:
          out = Interpolate(*(real32_T *)x1, *(real32_T *)x2, f1, f2);
          *(real32_T *) yout = (real32_T) out;
          break;

      case SS_INT8:
          out = Interpolate(*(int8_T *)x1,*(int8_T *)x2, f1, f2);

          if (out >= MAX_int8_T) {
	      *(int8_T *)yout = MAX_int8_T;
	  } else if (out <= MIN_int8_T) {
	      *(int8_T *)yout = MIN_int8_T;
	  } else {
	      *(int8_T *)yout = (int8_T)InterpRound(out);
	  }
          break;

      case SS_UINT8:

          out = Interpolate(*(uint8_T *)x1,*(uint8_T *)x2, f1, f2);

	  if (out >= MAX_uint8_T) {
	      *(uint8_T *)yout = MAX_uint8_T;
	  } else if (out <= MIN_uint8_T) {
	      *(uint8_T *)yout = MIN_uint8_T;
	  } else {
	      *(uint8_T *)yout = (uint8_T)InterpRound(out);
	  }

          break;

      case SS_INT16:
          out = Interpolate(*(int16_T *)x1,*(int16_T *)x2, f1, f2);

          if (out >= MAX_int16_T) {
	      *(int16_T *)yout = MAX_int16_T;
	  } else if (out <= MIN_int16_T) {
	      *(int16_T *)yout = MIN_int16_T;
	  } else {
	      *(int16_T *)yout = (int16_T)InterpRound(out);
	  }
          break;

      case SS_UINT16:
          out = Interpolate(*(uint16_T *)x1,*(uint16_T *)x2, f1, f2);

	  if (out >= MAX_uint16_T) {
	      *(uint16_T *)yout = MAX_uint16_T;
	  } else if (out <= MIN_uint16_T) {
	      *(uint16_T *)yout = MIN_uint16_T;
	  } else {
	      *(uint16_T *)yout = (uint16_T)InterpRound(out);
	  }

          break;

      case SS_INT32:
          out = Interpolate(*(int32_T *)x1,*(int32_T *)x2, f1, f2);

          if (out >= MAX_int32_T) {
	      *(int32_T *)yout = MAX_int32_T;
	  } else if (out <= MIN_int32_T) {
	      *(int32_T *)yout = MIN_int32_T;
	  } else {
	      *(int32_T *)yout = (int32_T)InterpRound(out);
	  }

          break;

      case SS_UINT32:
          out = Interpolate(*(uint32_T *)x1,*(uint32_T *)x2, f1, f2);

	  if (out >= MAX_uint32_T) {
	      *(uint32_T *)yout = MAX_uint32_T;
	  } else if (out <= MIN_uint32_T) {
	      *(uint32_T *)yout = MIN_uint32_T;
	  } else {
	      *(uint32_T *)yout = (uint32_T)InterpRound(out);
	  }

          break;

      case SS_BOOLEAN:
          /*
           * For Boolean interpolation amounts to choosing the point that
           * is closest in time.
           */
          *(boolean_T *) yout = (fabs(t-t1) < fabs(t-t2)) ? (*(boolean_T *)x1) :
                                                            (*(boolean_T *)x2);
          break;

      default:  
          break;
    }
}    /* end rt_Interpolate_Datatype */


/* Function:  rt_isTimeHit =================================
 * Abstract:
 *      This function is used to compare a given time to
 *      a data point in a time sequence.  Compares for
 *      equality and returns true if equal, false if not.
 */
int_T rt_isTimeHit(real_T t, real_T timePoint) {
    if (fabs(t - timePoint) <= rapid_eps(t)) /* test for epsilon */
        return true;
    else 
        return false;
}

/* Function:  rt_getTimeIdx ================================
 * Abstract:
 *      Given a time array and time, get time index so
 *      that timePtr[currTimeIdx]<t< timePtr[currTimeIdx+1]
 *      Note that if interpolation is off, when t is 
 *      larger than last time point, use last time point.
 *
 *      -7 means use default/ground datatype values (0 in most cases)
 */

int_T rt_getTimeIdx(real_T *timePtr, real_T t, int_T numTimePoints, 
                    int_T preTimeIdx, boolean_T interp, 
                    boolean_T timeHitOnly)
{


    int_T currTimeIdx= preTimeIdx;

    if(timeHitOnly) {
        if(currTimeIdx == -7) currTimeIdx= 0;
        while(currTimeIdx < numTimePoints) {
            if(rt_isTimeHit(t, timePtr[currTimeIdx])) {
                    return currTimeIdx;
            }
            currTimeIdx++;
        }

        return -7;

    }
    
    /* For structure has no time value, when reach last point
     *  output should be 0. return currTimeIdx = -7.
     */
    if (timePtr == NULL) {
        if ((currTimeIdx == numTimePoints - 1) || currTimeIdx == -7) return -7;
        return currTimeIdx +1;
    }
    
    /* When interpolation is off, when t< t0 or t>tfinal, set
     * currTimeIdx = -7 to output 0.
     */

    if (t < timePtr[0]){
        if (interp) {
            currTimeIdx = 0;
        } else {
            currTimeIdx = -7;
        }
    } else if ((t > timePtr[numTimePoints - 1]) || 
              (fabs(t - timePtr[numTimePoints - 1]) <= rapid_eps(t))) {
        /* When reach last time point, if interpolation is on
         * use last two points to extrapolate, otherwise return -7.
         */
        if (interp) {
            currTimeIdx = numTimePoints == 1 ? 0 : numTimePoints - 2;
        } else {
            if(fabs(t - timePtr[numTimePoints - 1]) < rapid_eps(t)){
                currTimeIdx = numTimePoints - 1;
            } else {
                currTimeIdx = -7; 
            }
        }
    } else {
        /* If we arrive at this point and currTimeIdx is set to -7 
         * (outputting "held value" or 0), then we need to update currTimeIdx 
         * to the first sample.  This can happen if we were outputting -7
         * due to the first time being < timePtr[0] for instance, and the 
         * next timestep bringing us here because t was incremented by a
         * timestep.
         */
        if(currTimeIdx == -7) currTimeIdx= 0;
        if (t < timePtr[currTimeIdx]) {
            /* dead code? */
            while (t < timePtr[currTimeIdx]) {
                currTimeIdx--;
            }
        } else {
            /* Keep incrementing the current time index while current time t
               is greater than or equal to the next time point. */
            while (t >= timePtr[currTimeIdx + 1]) {
                currTimeIdx++;
            }
        }
    }
    return currTimeIdx;
}     /* end rt_getTimeIdx */

/* Function: rt_IsPeriodicSampleHit ========================================================
 * Abstract:
 *	Determine if a periodic sample time has been hit. This is used for periodic function
 *      calls.
 */
bool rt_IsPeriodicSampleHit(rtInportTUtable *inportTUtables,
                                  int_T idx,
                                  real_T t)
{
    bool isHit = false;

    real_T period = inportTUtables[idx].time[0];
    real_T offset = inportTUtables[idx].time[1];

    /*
     * In order to avoid special casing, we essentially measure all hits
     * relative to a "hit -1". This hit never actually happens, but it allows us
     * to use the timeSinceHit to detect the time of the first sample hit using
     * the same formula. The currTimeIdx is initialized to -1 for this reason.
     */
    real_T timeSinceHit = t - (offset +
                               inportTUtables[idx].currTimeIdx * period);
    if (timeSinceHit > (period - rapid_eps(period))) {
        /* This is a sample hit, update the current hit index */
        isHit = true;
        inportTUtables[idx].currTimeIdx++;
    }
    return isHit;
}

/* Function: rt_RapidFreeGbls ========================================================
 * Abstract:
 *	Free global memory prior to exit
 
*/
void rt_RapidFreeGbls(int matFileFormat)
{
    FreeFNamePairList(gblToFNamepair, gblNumToFiles);
    FreeFNamePairList(gblFrFNamepair, gblNumFrFiles);
    
    if(gblNumRootInportBlks>0){
        int i;
        if (gblInportTUtables!= NULL){
            for(i=0; i< gblNumRootInportBlks; i++){
                
                if(gblInportTUtables[i].time != NULL){
                    if(matFileFormat == 2){
                        free(gblInportTUtables[i].time);
                    }else{
                        if(i==0){
                            free(gblInportTUtables[0].time);
                        }
                    }                    
                    gblInportTUtables[i].time = NULL;
                }
                
                if(gblInportTUtables[i].ur != (char*)NULL){
                    free(gblInportTUtables[i].ur);
                    gblInportTUtables[i].ur = NULL;
                }
                if(gblInportTUtables[i].ui != (char*)NULL){
                    free(gblInportTUtables[i].ui);
                    gblInportTUtables[i].ui = NULL;
                }          
            }
            free(gblInportTUtables);
        }    
    }
} /* end rt_RapidFreeGbls */


/* Function: rt_RapidCheckRemappings ==================================================
 * Abstract:
 *	Verify that the FromFile switches were used
 *
 * Returns:
 *	NULL     - success
 *	non-NULL - error message
 */
const char *rt_RapidCheckRemappings(void)
{
    int_T i;

    for (i = 0; i < gblNumFrFiles; i++) {
        if (gblFrFNamepair[i].oldName != NULL && gblFrFNamepair[i].remapped==0){
            return("one or more -f switches from file names do not exist in "
                   "the model");
        }
    }

    for (i = 0; i < gblNumToFiles; i++) {
        if (gblToFNamepair[i].oldName != NULL && gblToFNamepair[i].remapped==0){
            return("one or more -t switches from file names do not exist in "
                   "the model");
        }
    }
    return(NULL);

} /* end rt_RapidCheckRemappings */

/* Function: rt_GetISigstreamManager ============================================
 *
 * Abstract:
 *      This function gets the pointer to the single global ISigstreamManager
 *      instance.
 *      
 *      
 *      
 * Returns:
 *	void * pointer to the ISigstreamManager instance.
 */
void *rt_GetISigstreamManager(void)
{
    return gblISigstreamManager;
}

/* Function: rt_GetOSigstreamManager ============================================
 *
 * Abstract:
 *      This function gets the pointer to the single global OSigstreamManager
 *      instance.
 *      
 *      
 *      
 * Returns:
 *	void * pointer to the SigStreamManager instance.
 */
void *rt_GetOSigstreamManager(void)
{
    return gblOSigstreamManager;
}

/* Function: rt_GetOSigstreamManagerAddr ========================================
 *
 * Abstract:
 *      This function gets the address of the pointer to the single global 
 *      OSigstreamManager instance.
 *      
 *      
 *      
 * Returns:
 *	void ** containing the address of the pointer to the SigStreamManager 
 *      instance.
 */
void **rt_GetOSigstreamManagerAddr(void)
{
    return &gblOSigstreamManager;
}


/* Function: rt_slioCatalogue ============================================
 *
 * Abstract:
 *      This function gets the pointer to the single global SlioCatalogue
 *      instance.
 *      
 *      
 *      
 * Returns:
 *	void * pointer to the SigStreamManager instance.
 */
void *rt_slioCatalogue(void)
{
    return slioCatalogue;
}

/* Function: rt_slioCatalogueAddr ========================================
 *
 * Abstract:
 *      This function gets the address of the pointer to the single global 
 *      SlioCatalogue instance.
 *      
 *      
 *      
 * Returns:
 *	void ** containing the address of the pointer to the SlioCatalogue 
 *      instance.
 */
void **rt_slioCatalogueAddr(void)
{
    return &slioCatalogue;
}

/* Function: rt_GetMatSigstreamLoggingFileName =====================================
 * Abstract:
 *	Get the name of the file with sigstream logging results.
 */
const char *rt_GetMatSigstreamLoggingFileName(void)
{
    return gblMatSigstreamLoggingFilename;
}

/* Function: GetMatSigLogSelectorFileName =======================================
 * Abstract:
 *	Get the name of the file with SigLogSelector settings.
 */
const char *rt_GetMatSigLogSelectorFileName(void)
{
    return gblMatSigLogSelectorFilename;
}

void rt_WriteSimMetadata(const char *file, const SimStruct* S)
{
    MATFile *fptr = NULL;
    mxArray* mxDoubleVar = NULL;
    mxArray* mxStringVar = NULL;
    mxArray* mxStructVar = NULL;
    mxArray* mxLogicalVar = NULL;
    mxArray* mxUint32Var = NULL;

    static const char* fieldNames[] = {"StopTime",
                                "IsVariableStepSolver",
                                "SolverName",
                                "StepSize",
                                "StructuralChecksum0",
                                "StructuralChecksum1",
                                "StructuralChecksum2",
                                "StructuralChecksum3",
                                "StopRequested",
                                "ReachedStopTime",
                                "StartTime"
                                };

    if((file == NULL) || ((fptr = matOpen(file, "w")) == NULL)){
        (void)fprintf(stderr,"Could not open file for sim metadata\n");
        return;
    }


    mxStructVar = mxCreateStructMatrix(1, 1, 11, fieldNames);

    /* StopTime */
    mxDoubleVar = mxCreateDoubleMatrix(1, 1, mxREAL);
    *(mxGetPr(mxDoubleVar)) = ssGetT(S); 
    mxSetFieldByNumber(mxStructVar, 0, 0, mxDoubleVar);

    mxLogicalVar = mxCreateLogicalScalar((mxLogical) ssIsVariableStepSolver(S));
    mxSetFieldByNumber(mxStructVar, 0, 1, mxLogicalVar);

    mxStringVar = mxCreateString(ssGetSolverName(S));
    mxSetFieldByNumber(mxStructVar, 0, 2, mxStringVar);

    mxDoubleVar = mxCreateDoubleMatrix(1, 1, mxREAL);
    if(ssIsVariableStepSolver(S)) {
        *(mxGetPr(mxDoubleVar)) = ssGetMaxStepSize(S);
    } else {
        *(mxGetPr(mxDoubleVar)) = ssGetFixedStepSize(S);
    }
    mxSetFieldByNumber(mxStructVar, 0, 3, mxDoubleVar);

    mxUint32Var = mxCreateNumericMatrix(1, 1, mxUINT32_CLASS, mxREAL);
    *((uint32_T*)mxGetData(mxUint32Var)) = ssGetChecksum0(S);
    mxSetFieldByNumber(mxStructVar, 0, 4, mxUint32Var);

    mxUint32Var = mxCreateNumericMatrix(1, 1, mxUINT32_CLASS, mxREAL);
    *((uint32_T*)mxGetData(mxUint32Var)) = ssGetChecksum1(S);
    mxSetFieldByNumber(mxStructVar, 0, 5, mxUint32Var);

    mxUint32Var = mxCreateNumericMatrix(1, 1, mxUINT32_CLASS, mxREAL);
    *((uint32_T*)mxGetData(mxUint32Var)) = ssGetChecksum2(S);
    mxSetFieldByNumber(mxStructVar, 0, 6, mxUint32Var);

    mxUint32Var = mxCreateNumericMatrix(1, 1, mxUINT32_CLASS, mxREAL);
    *((uint32_T*)mxGetData(mxUint32Var)) = ssGetChecksum3(S);
    mxSetFieldByNumber(mxStructVar, 0, 7, mxUint32Var);

    mxLogicalVar = mxCreateLogicalScalar((mxLogical) ssGetStopRequested(S));
    mxSetFieldByNumber(mxStructVar, 0, 8, mxLogicalVar);

    mxLogicalVar = mxCreateLogicalScalar(ssGetT(S) == ssGetTFinal(S));
    mxSetFieldByNumber(mxStructVar, 0, 9, mxLogicalVar);
    
    mxDoubleVar = mxCreateDoubleMatrix(1, 1, mxREAL);
    *(mxGetPr(mxDoubleVar)) = ssGetTStart(S); 
    mxSetFieldByNumber(mxStructVar, 0, 10, mxDoubleVar);

    matPutVariable(fptr, "SimMetadataStruct", mxStructVar);

    if (mxStructVar) {
        mxDestroyArray(mxStructVar);
    }

    matClose(fptr);
}

/* BEGIN: Methods to allow the Model Reference SIM Target to be used */

boolean_T slIsRapidAcceleratorSimulating(void) {
    return true;
}

void slcsInvokeSimulinkFunction(SimStruct *S, const char *fcnName, 
                                _ssFcnCallExecArgInfo *args)
{
    (void)S;
    (void)fcnName;
    (void)args;
}

void slcsInvokeSimulinkFunctionVoidArgs(
    SimStruct* S, const char* fcnName, int_T numArgs, void* args[])
{
    (void)S;
    (void)fcnName;
    (void)numArgs;
    (void)args;
}

void slmrModelRefRegisterSimStateChecksum(
    SimStruct* S, const char* mdlName, const uint32_T* chksum)
{
  (void)S;
  (void)mdlName;
  (void)chksum;
}

void slmrSetForeachDimensions(
    void *pDatasetDesc, uint_T numForEachLevels, const uint_T *forEachDims)
{
    (void) pDatasetDesc;
    (void) numForEachLevels;
    (void) forEachDims;
}

void slmrLogStatesAfterStateWrite(
    SimStruct *S,  const char* writerSID)
{
    (void) S;
    (void) writerSID;
}


/* END: Methods to allow the Model Reference SIM Target to be used */

/* EOF common_utils.c */

/* LocalWords:  RSim matrx smaple matfile rb scaler Tx gbl tu Datato TUtable
 * LocalWords:  UTtable FName FFname raccel el fromfile npts npoints nchannels
 * LocalWords:  curr tfinal timestep Gbls Remappings slio Catalogue
 */
