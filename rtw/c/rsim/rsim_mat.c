/******************************************************************
 *
 *  File: rsim_mat.c
 *
 *
 *  Abstract:
 *      - provide matfile handling for reading and writing matfiles
 *        for use with rsim stand-alone, non-real-time simulation
 *      - provide functions for swapping rtP vector for parameter tuning
 *
 * Copyright 1994-2009 The MathWorks, Inc.
 ******************************************************************/

/*
 * This file is still using the old 32-bit mxArray API
 */
#define MX_COMPAT_32

/* INCLUDES */
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <math.h>
#include  <float.h>

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

#include  "rsim.h"
 
extern mxClassID rt_GetMxIdFromDTypeId(BuiltInDTypeId dTypeID); 

/* external variables */

extern const char  *gblParamFilename;
/*
static PrmStructData gblPrmStruct;
*/
extern const char* gblSolverOptsFilename;
extern int         gblSolverOptsArrIndex;

extern FNamePair   *gblToFNamepair;
extern int         gblNumToFiles;

extern FNamePair   *gblFrFNamepair;
extern int          gblNumFrFiles;


/*==================*
 * Visible routines *
 *==================*/

/* Function: rt_RSimRemapToFileName ============================================
 * Abstract:
 *      Used by the Rapid Simulation Target (rsim) to remap during model
 *      start, the MAT-file from which this block writes to. The list
 *      of all to file names is assumed to be unique.  This routine will
 *      search the list that maps the original to file name specified in
 *      the model to a new output name. When found, the fileName pointer
 *      is updated to point to this location.
 */
void rt_RSimRemapToFileName(char *fileName)
{
    int_T i;
    for (i=0; i<gblNumToFiles; i++) {
	if (gblToFNamepair[i].newName != NULL &&
            strcmp(fileName, gblToFNamepair[i].oldName)==0) {
            (void)strcpy(fileName, gblToFNamepair[i].newName); /* remap */
            gblToFNamepair[i].remapped = 1;
        }
    }
} /* end rt_RSimRemapToFileName */


/* Function: rt_RSimRemapFromFileName ==========================================
 * Abstract:
 *      Used by the Rapid Simulation Target (rsim) to remap during model
 *      start, the MAT-file from which this block reads from. The list
 *      of all from file names is assumed to be unique.  This routine will
 *      search the list that maps the original from file name specified in
 *      the model to a new input name. When found, the fileName pointer
 *      is updated to point to this location.
 */
void rt_RSimRemapFromFileName(char *fileName)
{
    int_T i;
    for (i=0; i<gblNumFrFiles; i++) {
	if (gblFrFNamepair[i].newName != NULL &&
            strcmp(fileName, gblFrFNamepair[i].oldName)==0) {
            (void)strcpy(fileName,gblFrFNamepair[i].newName); /* remap */
            gblFrFNamepair[i].remapped = 1;
            break;
        }
    }
} /* end rt_RSimRemapFromFileName */




/*******************************************************************************
 *
 * Rountine to load solver options.
 * The options listed below can be changed as
 * the start of execution using the -S flag
 *
 *            Solver
 *            RelTol
 *            MinStep
 *            MaxStep
 *            InitialStep
 *            Refine
 *            MaxOrder
 *              ExtrapolationOrder        --  used by ODE14x
 *              NumberNewtonIterations    --  used by ODE14x
 *              SolverResetMethod -- used by ODE15s, ODE23t, ODE23tb
 */
void rsimLoadSolverOpts(SimStruct* S)
{
    MATFile*    matf   = NULL;
    mxArray*    pa     = NULL;
    mxArray*    sa     = NULL;
    size_t      idx    = 0;
    const char* result = NULL;

    if (gblSolverOptsFilename == NULL) return;

    if ((matf=matOpen(gblSolverOptsFilename,"r")) == NULL) {
        result = "could not find MAT-file containing new parameter data";
        goto EXIT_POINT;
    }

    if ((pa=matGetNextVariable(matf,NULL)) == NULL ) {
        result = "error reading new solver options from MAT-file";
        goto EXIT_POINT;
    }

    /* Should be structure */
    if ( !mxIsStruct(pa) || (mxGetN(pa) > 1 && mxGetM(pa) > 1) ) {
        result = "solver options should be a vector of structures";
        goto EXIT_POINT;
    }

    if (gblSolverOptsArrIndex > 0) idx = gblSolverOptsArrIndex;

    if (idx > 0 && mxGetNumberOfElements(pa) <= idx) {
        result = "options array size is less than the specified array index";
        goto EXIT_POINT;
    }

    /* SolverName */
    {
        const char* opt = "SolverName";
        static char solver[256] = "\0";
        if ( (sa=mxGetField(pa,(int)idx,opt)) != NULL ) {
            if (mxGetString(sa, solver, 256) != 0) {
                result = "xxx later";
                goto EXIT_POINT;
            }
            ssSetSolverName(S, solver);
        }
    }

    /* RelTol */
    {
        const char* opt = "RelTol";
        if ( (sa=mxGetField(pa,(int)idx,opt)) != NULL ) {
            if ( !mxIsDouble(sa) || mxGetNumberOfElements(sa) != 1 ) {
                result = "error reading solver option RelTol";
                goto EXIT_POINT;
            }
            ssSetSolverRelTol(S, mxGetPr(sa)[0]);
        }
    }

    /* AbsTol */
    {
        const char* opt = "AbsTol";
        int  nx   = ssGetNumContStates(S);

        /* Load absolute tolerance if :
         * Is variable step solver AND 
         * there are continuous states in the model */

        if ( (ssIsVariableStepSolver(S)) && (nx > 0) && 
             (sa=mxGetField(pa,(int)idx,opt)) != NULL ) {
            double* aTol = ssGetAbsTolVector(S);
            const uint8_T* aTolControl = ssGetAbsTolControlVector(S);
            size_t   n    = mxGetNumberOfElements(sa);

            if (!mxIsDouble(sa) || (n != 1 && n != (int)nx) ) {
                result = "error reading solver option AbsTol";
                goto EXIT_POINT;
            }
            if (n == 1) {
                int i;
                for (i = 0; i < nx; i++) {
                    if (aTolControl[i] == SL_SOLVER_TOLERANCE_GLOBAL){
                        aTol[i] = mxGetPr(sa)[0];
                    }
                }
            } else {
                int i;
                for (i = 0; i < nx; i++) {
                    if (aTolControl[i] == SL_SOLVER_TOLERANCE_GLOBAL){
                        aTol[i] = mxGetPr(sa)[i];
                    }
                }
            }
        }
    }

    /* MinStep */
    {
        const char* opt = "MinStep";
        if ( (sa=mxGetField(pa,(int)idx,opt)) != NULL ) {
            if ( !mxIsDouble(sa) || mxGetNumberOfElements(sa) != 1 ) {
                result = "error reading solver option MinStep";
                goto EXIT_POINT;
            }
            ssSetMinStepSize(S, mxGetPr(sa)[0]);
        }
    }
    
    /* MaxConsecutiveMinStep */
    {
        const char* opt = "MaxConsecutiveMinStep";
        int         maxConsecutiveMinStep;
        if ( (sa=mxGetField(pa,(int)idx,opt)) != NULL ) {
            if ( !mxIsDouble(sa) || mxGetNumberOfElements(sa) != 1 ) {
                result = "error reading solver option MaxConsecutiveMinStep";
                goto EXIT_POINT;
            }
            maxConsecutiveMinStep = (int) (mxGetPr(sa)[0]);
            ssSetSolverMaxConsecutiveMinStep(S, maxConsecutiveMinStep);
        }
    }

    /* MaxStep */
    {
        const char* opt = "MaxStep";
        if ( (sa=mxGetField(pa,(int)idx,opt)) != NULL ) {
            if ( !mxIsDouble(sa) || mxGetNumberOfElements(sa) != 1 ) {
                result = "error reading solver option MaxStep";
                goto EXIT_POINT;
            }
            ssSetMaxStepSize(S, mxGetPr(sa)[0]);
        }
    }

    /* FixedStep */
    {
        const char* opt = "FixedStep";

        if ( (sa=mxGetField(pa,(int)idx,opt)) != NULL ) {
            double newFixedStepSize;

            if ( !mxIsDouble(sa) || mxGetNumberOfElements(sa) != 1 ) {
                result = "FixedStep solver option must be a positive finite scalar";
                goto EXIT_POINT;
            }
            newFixedStepSize = mxGetPr(sa)[0];
            if (ssGetFixedStepSize(S) != newFixedStepSize) {
                if (ssGetFixedStepSize(S) == 0) {
                    result = "error: cannot change the Fixed Step Size "
                        "when using a Variable Step Solver";
                    goto EXIT_POINT;
                }
                if (newFixedStepSize <= 0.0 || !mxIsFinite(newFixedStepSize))  {
                    result = "FixedStep must be a positive finite scalar";
                }
                if ( !( ssGetNumSampleTimes(S) == 1 && 
                        ssGetSampleTime(S,0) == 0.0 && 
                        ssGetOffsetTime(S,0) == 0.0) ) {
                    result = "error: cannot change the Fixed Step Size "
                        "for discrete or multi rate models";
                    goto EXIT_POINT;

                }
                ssSetStepSize(S, newFixedStepSize);
            }
        }
    }

    /* InitialStep */
    {
        const char* opt = "InitialStep";
        if ( (sa=mxGetField(pa,(int)idx,opt)) != NULL ) {
            if ( !mxIsDouble(sa) || mxGetNumberOfElements(sa) != 1 ) {
                result = "error reading solver option InitialStep";
                goto EXIT_POINT;
            }
            ssSetStepSize(S, mxGetPr(sa)[0]);
        }
    }

    /* MaxOrder */
    {
        const char* opt = "MaxOrder";
        int         maxOrder;
        if ( (sa=mxGetField(pa,(int)idx,opt)) != NULL ) {
            if ( !mxIsDouble(sa) || mxGetNumberOfElements(sa) != 1 ) {
                result = "error reading solver option MaxOrder";
                goto EXIT_POINT;
            }
            maxOrder = (int) (mxGetPr(sa)[0]);
            ssSetSolverMaxOrder(S, maxOrder);
        }
    }

     /* ConsecutiveZCsStepRelTol */
    {
        const char* opt = "ConsecutiveZCsStepRelTol";
        double      consecutiveZCsStepRelTol;
        if ( (sa=mxGetField(pa,(int)idx,opt)) != NULL ) {
            if ( !mxIsDouble(sa) || mxGetNumberOfElements(sa) != 1 ) {
                result = "error reading solver option ConsecutiveZCsStepRelTol";
                goto EXIT_POINT;
            }
            consecutiveZCsStepRelTol = (mxGetPr(sa)[0]);
            ssSetSolverConsecutiveZCsStepRelTol(S, consecutiveZCsStepRelTol);    
        }
    }
 
     /* MaxConsecutiveZCs */
    {
        const char* opt = "MaxConsecutiveZCs";
        int         maxConsecutiveZCs;
        if ( (sa=mxGetField(pa,(int)idx,opt)) != NULL ) {
            if ( !mxIsDouble(sa) || mxGetNumberOfElements(sa) != 1 ) {
                result = "error reading solver option MaxConsecutiveZCs";
                goto EXIT_POINT;
            }
            maxConsecutiveZCs = (int) (mxGetPr(sa)[0]);
            ssSetSolverMaxConsecutiveZCs(S, maxConsecutiveZCs);
        }
    }


    /* ExtrapolationOrder -- used by ODE14x, only */
    {
        const char* opt = "ExtrapolationOrder";
        int         extrapolationOrder;
        if ( (sa=mxGetField(pa,(int)idx,opt)) != NULL ) {
            if ( !mxIsDouble(sa) || mxGetNumberOfElements(sa) != 1 ) {
                result = "error reading solver option ExtrapolationOrder";
                goto EXIT_POINT;
            }
            extrapolationOrder = (int) (mxGetPr(sa)[0]);
            ssSetSolverExtrapolationOrder(S, extrapolationOrder);
        }
    }

    /* NumberNewtonIterations -- used by ODE14x, only */
    {
        const char* opt = "NumberNewtonIterations";
        int         numberIterations;
        if ( (sa=mxGetField(pa,(int)idx,opt)) != NULL ) {
            if ( !mxIsDouble(sa) || mxGetNumberOfElements(sa) != 1 ) {
                result = "error reading solver option ExtrapolationOrder";
                goto EXIT_POINT;
            }
            numberIterations = (int) (mxGetPr(sa)[0]);
            ssSetSolverNumberNewtonIterations(S, numberIterations);
        }
    }

    /* Refine */
    {
        const char* opt = "Refine";
        int         refine;
        if ( (sa=mxGetField(pa,(int)idx,opt)) != NULL ) {
            if ( !mxIsDouble(sa) || mxGetNumberOfElements(sa) != 1 ) {
                result = "error reading solver option Refine";
                goto EXIT_POINT;
            }
            refine = (int) (mxGetPr(sa)[0]);
            ssSetSolverRefineFactor(S, refine);
        }
    }

    /* SolverResetMethod: {'Fast'} | 'Robust' - used by ODE15s, ODE23t, ODE23tb */
    {
        const char* opt = "SolverResetMethod";
	const char* robustMethod = "Robust";
        static char resetMethod[256] = "\0";
        bool        isRobustResetMethod = false;
        if ( (sa=mxGetField(pa,(int)idx,opt)) != NULL ) {
            if (mxGetString(sa, resetMethod, 256) != 0) {
                result = "xxx later";
                goto EXIT_POINT;
            }
	    isRobustResetMethod = (strcmp(resetMethod,robustMethod) == 0);
	    ssSetSolverRobustResetMethod(S,(bool)isRobustResetMethod);
        }
    }

  EXIT_POINT:

    if (matf != NULL) {
        if(pa!=NULL) {
            mxDestroyArray(pa);
            pa = NULL;
        }
        matClose(matf); matf = NULL;
    }

    ssSetErrorStatus(S, result);
    return;

} /* rsimLoadSolverOpts */


/* EOF rsim_mat.c */
