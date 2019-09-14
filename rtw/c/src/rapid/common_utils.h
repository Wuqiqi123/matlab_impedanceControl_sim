/*
 * Copyright 2007-2016 The MathWorks, Inc.
 *
 * File: common_utils.h
 *
 *
 * Abstract:
 *	Data structures used in rapid accelerator AND rsim, for To and From File
 *      blocks, and for parameter handling.
 *
 * Requires include files
 *	tmwtypes.h
 *	simstruc_type.h
 * Note including simstruc.h before rsim.h is sufficient because simstruc.h
 * includes both tmwtypes.h and simstruc_types.h.
 */

#ifndef __COMMON_UTILS_H__
#define __COMMON_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

    /*=========*
     * Defines *
     *=========*/

#define RUN_FOREVER -1.0

#define QUOTE1(name) #name
#define QUOTE(name) QUOTE1(name)    /* need to expand name    */

#ifndef SAVEFILE
# define MATFILE2(file) #file ".mat"
# define MATFILE1(file) MATFILE2(file)
# define MATFILE MATFILE1(MODEL)
#else
# define MATFILE QUOTE(SAVEFILE)
#endif

#ifndef MAXSTRLEN
# define MAXSTRLEN 509
#endif

#include <math.h>


    /*==========*
     * Typedefs *
     *==========*/

    /* Blockath and filename pairs for To File, From File blocks */
    typedef struct {
    char blockpath[MAXSTRLEN];
    char filename[MAXSTRLEN];
} FileInfo;

    /* File Name Pairs for To File, From File blocks */
    typedef struct {
    const char *inputString;
    char       *oldName;
    const char *newName;
    int_T      remapped; /* Used to verify that name was remapped */
} FNamePair;


    /* From File Info (one per from file block) */
    typedef struct {
    const char  *origFileName;
    const char  *newFileName;
    int         originalWidth;
    int         nptsTotal;
    int         nptsPerSignal;
    double      *tuDataMatrix;
} FrFInfo;


    /* From Workspace Info (one per from workspace block) */
    typedef struct {
    const char *origWorkspaceVarName;
    DTypeId    origDataTypeId;
    int        origIsComplex;
    int        origWidth;
    int        origElSize;
    int        nDataPoints;
    double     *time;    /* time vector must be double */
    void       *data;    /* data vector can be any type including complex */
    double     *valDims; /* valueDimensions vector is stored in double */
} FWksInfo;

    typedef struct {
    void   *ur;                /* columns of inputs: real part        */
    void   *ui;                /* columns of inputs: imag part        */   
    double *time;              /* time vector of each row             */           
    int_T   nTimePoints;        
    int     uDataType;          
    int     complex; 
    int     currTimeIdx;       /* for interpolation */
    bool    isPeriodicFcnCall; /* Should the TU table be interpreted as a
                                * periodic function call specification */
} rtInportTUtable;

#define NUM_DATA_TYPES (9)



    /* consult Foundation Libraries before using mxIsIntVectorWrapper G978320 */
    extern bool mxIsIntVectorWrapper(const mxArray *pm);

    extern const char *rt_RapidReadFromFileBlockMatFile(const char *origFileName, 
                                                        int originalWidth,
                                                        FrFInfo *frFInfo);

    extern void *rt_GetISigstreamManager(void);

    extern void *rt_GetOSigstreamManager(void);

    extern void **rt_GetOSigstreamManagerAddr(void);

    extern void *rt_slioCatalogue(void);

    extern void **rt_slioCatalogueAddr(void);

    extern const char *rt_RapidReadInportsMatFile(const char* inportFileName,
                                                  int* matFileFormat,
                                                  int isRaccel);              

    extern void rt_Interpolate_Datatype(void   *x1, void   *x2, void   *yout,
                                        real_T t,   real_T t1,  real_T t2,
                                        int    outputDType);

    extern int_T rt_getTimeIdx(real_T *timePtr, real_T t, int_T numTimePoints, 
                               int_T preTimeIdx, boolean_T interp, boolean_T timeHitOnly);

    extern void rt_RapidFreeGbls(int);

    extern const char *rt_RapidCheckRemappings(void);

    extern const char *rt_GetMatSigstreamLoggingFileName(void);

    extern const char *rt_GetMatSigLogSelectorFileName(void);

    extern bool rt_IsPeriodicSampleHit(rtInportTUtable* inportTUtables,
                                       int_T idx,
                                       real_T t);
    
    extern void rt_WriteSimMetadata(const char*, const SimStruct*);

    extern boolean_T slIsRapidAcceleratorSimulating(void);

#ifdef __cplusplus
}
#endif

#endif /* __COMMON_UTILS_H__ */

/* LocalWords:  raccel RSIM rsim Blockath
 */
