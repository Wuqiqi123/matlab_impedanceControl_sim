/*
 * Copyright 1994-2013 The MathWorks, Inc.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tmwtypes.h"
#include "simstruc.h"

#include "raccel.h"
#include "raccel_sup.h"
#include "rt_nonfinite.h"

#include "ext_work.h"
#include "ext_svr.h"

#if defined (UNIX)
# include <unistd.h> /* getpid() */
#else 
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
/* win32 and win64 for all compilers */
# include <process.h>	/* _getpid */
# define getpid _getpid
# include <io.h> /* _access */
# define access _access
# define F_OK   0 /* exists     */
#endif

#if defined(TGTCONN)
    extern const char *TgtConnInit(int_T argc, char_T *argv[]);
#else
    #define TgtConnInit(argc, argv) NULL /* do nothing */
#endif

/*=============*
 * Global data *
 *=============*/
/* Version information */
extern const char_T *gbl_raccel_Version;
/* Name of token to indicate that the program is running */
static char gblProgramActiveToken[1024];
static char gblErrorFile[1024];

/*
 * File name mapping pairs (old=new) for To File, From File blocks.
 */
FNamePair *gblFrFNamepair = NULL;
FNamePair *gblToFNamepair = NULL;

/*
 * Global data for command line arguments
 */
static const char gblFromWorkspaceFilenameDefault[] = "from_workspace.mat";
static int_T      gblVerbose                        = 0;
       
boolean_T gblExtModeEnabled = 0U;
boolean_T gblRunningInParallel = 0U;

const char *gblErrorStatus = NULL;
const char   *gblFromWorkspaceFilename = gblFromWorkspaceFilenameDefault;
const char   *gblMatLoggingFilename    = NULL;
const char   *gblSimMetadataFileName    = NULL;
const char   *gblSimDataRepoFilename    = NULL;
const char *gblSFcnInfoFileName = NULL;
const char *gblInportFileName = NULL;
const char *gblSolverOptsFilename = NULL;
const char *gblParamFilename = NULL;
const char *gblToFileSuffix = NULL;
/* Memory allocation error is used by blocks in generated code */
const char *RT_MEMORY_ALLOCATION_ERROR = "memory allocation error";
double gblFinalTime = 0;  /* not specified */
int_T gblParamCellIndex = -1;
int_T gblSolverOptsArrIndex = -1;
int_T gblFinalTimeChanged = 0;
int_T gblTimeLimit = -1; /* not specified */


extern const char   *gblMatSigstreamLoggingFilename;
extern const char   *gblMatSignalLoggingSelectorFilename;
extern const char   *gblSFcnInfoFileName;
/* Version information */
extern const char_T *gbl_raccel_Version;
/*extern rtInportTUtable *gblInportTUtables;*/
extern int_T gblNumRootInportBlks;
extern SimStruct *const rtS;

/*=================================*
 * External data setup by rsim.tlc *
 *=================================*/
extern const int_T   gblNumFrFiles;
extern const int_T   gblNumToFiles;
extern const int_T   gblNumFrWksBlocks;
extern       int_T   gblVerboseFlag;

void raccel_set_checksum();

/*===========*
 * Constants *
 *===========*/

static const char_T UsageMsgPart1[] =
" Captive Simulation Target usage: modelname [switches]\n"
"  switches:\n"
"    -c compare checksum to see if it matches"
"    -f <originalFromFile.mat=newFromFile.mat>\n"
"            Name of original input MAT-file and replacement input MAT-file\n"
"            containing TU matrix for \"From File\" block.\n"
"    -i <InportsFile.mat>\n"
"            Name of the MAT-file containing structure format for \"Root Inport\" block.\n";

static const char_T UsageMsgPart2[] =
"    -o <results.mat>\n"
"            Name of output MAT-file for MAT-file logging of simulation data\n"
"            (time,states,root outports).\n"
"    -p <parameters.mat>\n"
"            Name of new \"Parameter\" MAT-file containing new block parameter\n"
"            vector \"rtP\".\n"
"    -s,-tf <stopTime>\n"
"            Final time value to end simulation.\n"
"    -S <solver_options.mat>\n"
"            Load new solver options (e.g., Solver, RelTol, AbsTol, etc)\n";

static const char_T UsageMsgPart3[] =
"    -L timeLimit\n"
"            Exit if run time (in seconds) of the program exceeds timeLimit\n"
"    -t <originalToFile.mat=newToFile.mat>\n"
"            Name of original destination file and new destination file\n"
"            for results from a \"To File\" block.\n"
"    -w      Waits for Simulink to start model in External Mode.\n"
"    -port <TCPport>\n"
"            Overrides 17725 default port for External Mode,\n"
"            valid range 256 to 65535.\n";

static char_T UsageMsg[sizeof(UsageMsgPart1) + sizeof(UsageMsgPart2) + sizeof(UsageMsgPart3)];

static const char ObsoleteMsg[] =
"The -s switch is now obsolete and will be removed from a future version.\n"
"Please use the -tf switch in place of -s.\n";

/* Return TRUE if file 'fileName' exists */
bool FileExists(const char *fileName)
{
#if defined (UNIX)
    if(access(gblProgramActiveToken,F_OK) == 0)
        return true;
    else
        return false;
#else
    DWORD       fileAttr;
    fileAttr = GetFileAttributes(fileName);
    if (0xFFFFFFFF == fileAttr)
        return false;
    else
        return true;
#endif
}

/* Return current process ID */
#if defined (UNIX)
int GetMyPID()
{
    return getpid();
}
#else
DWORD GetMyPID()
{
    return GetCurrentProcessId();
}
#endif

/* Function: AtExitFcn =========================================================
 *
 *   Delete the program active token
 */
void AtExitFcn(void)
{
    /*if (access(gblProgramActiveToken,F_OK) == 0) {*/ /* token exists? */
    if(FileExists(gblProgramActiveToken)) { /* token exists? */
        if (remove(gblProgramActiveToken) == -1) { /* remove failed? */
            fprintf(stderr,"** Unable to remove '%s'\n", gblProgramActiveToken);
            fflush(stderr);
        }
    }
        /* ssGetErrorStatus will only be valid after raccel_register_model
           has been called. To check if raccel_register_model has been called
           we check if root simstruct is set (ssGetRootSS)
        */
        if ((ssGetRootSS(rtS) && ssGetErrorStatus(rtS)) || gblErrorStatus) {
            FILE* fh;
        fh = fopen(gblErrorFile, "w");
            if (fh) {
                if (ssGetRootSS(rtS) && ssGetErrorStatus(rtS)) {
                    (void)fprintf(fh, "%s", ssGetErrorStatus(rtS));
                } else {
                   (void)fprintf(fh, "%s", gblErrorStatus);
                }
                fclose(fh);
            }
        }
    }






/* Function: SplitFileNamepair =================================================
 *
 * Abstract:
 *           This function splits an input string into two parts.
 *           The input file-name-pair would appear as follows:
 *
 *                origname.mat=replacement.mat
 *
 *           The first  portion of the string contains the original file name.
 *           This is followed by an "=" equal sign, no spaces allowed!
 *           Then, the string following the equal sign will contain
 *           the replacement file name.
 *
 * Inputs:   struct FNamePair fileNamePair.inputString
 * Outputs:  returns struct "fileNames" with fields containing:
 *           - fileNamePair.oldName
 *           - fileNamePair.newName
 *
 */
static const char *SplitFileNamepair(FNamePair *fileNamePair)
{
    const char *pairStr = fileNamePair->inputString;
    const char *eq      = strchr(pairStr, '=');
    size_t     oldLen;

    if (eq == NULL) {
        return("invalid file remapping argument specified (no \"=\" found)");
    }

    /*LINTED E_CAST_INT_TO_SMALL_INT */
    oldLen = (size_t)(eq - pairStr);

    if ((fileNamePair->oldName = (char*)malloc(oldLen+1)) == NULL) {
        return("memory allocation error (SplitFileNamepair)");
    }

    (void)strncpy(fileNamePair->oldName,pairStr,oldLen);
    fileNamePair->oldName[oldLen] = '\0';

    fileNamePair->newName = eq+1;

    if (fileNamePair->newName[0] == '\0') {
        return("invalid file remapping argument specified (no new name found)");
    }

    return(NULL);

}  /* end SplitFileNamepair */


/* Function: ParseArgs ========================================================
 * Abstract:
 *	Parse the command-line arguments: valid args:
 *       -c     = compare checksum to see if it matches
 *       -f     = Fromfile block name pair for reading "from file" data
 *       -i     = Inport block name pair for reading "Inport" data
 *       -o     = Output file for matfile logging
 *       -m     = Output file for simulation metadata
 *       -p     = Param file for new rtP vector
 *       -s,-tf = Stop time for ending simulation
 *       -t     = Tofile name pair so saving "to file" data
 *       -d     = File containing s-function info
 *
 *  Example:
 *       f14 -p mydata -o myoutfile -s 10.1 -f originfile.mat=newinfile.mat
 *           -t origtofile=newtofile.mat -i inport.mat
 *
 *  This results as follows:
 *       - sets data structures to run model f14
 *       - "-p" option loads new rtP param vector from "mydata.mat"
 *       - "-o" saves stand-alone simulation results to "mydata.mat"
 *              instead of saving to "f14.mat"
 *       - "-s" results in simulation stopping at t=10.1 seconds.
 *       - "-f" replaces the input matfile name for all instances of  fromfile
 *              blocks that had the original name: "originfile.mat" with the
 *              replacement name: "newinfile.mat".
 *       - "-i" read in the inport.mat file that drive the inport blocks
 *       - "-t" similar to -f option except swapping names of to file blocks.
 *
 *  Returns:
 *	NULL     - success
 *	non-NULL - error message
 */
const char *ParseArgs(int_T argc, char_T *argv[])
{
    int_T      tvar;
    size_t     optLen           = 0;
    int_T      toFNamepairIdx   = 0;
    int_T      frFNamepairIdx   = 0;
    const char *result          = NULL; /* assume success */

    (void) strcpy(UsageMsg, UsageMsgPart1);
    (void) strcpy(&UsageMsg[strlen(UsageMsgPart1)],UsageMsgPart2);
    (void) strcpy(&UsageMsg[strlen(UsageMsgPart1) + strlen(UsageMsgPart2)],UsageMsgPart3);

 
      
    /*
     * Get Checksum:
     * --get-checksum
     */
    if (argc > 1 && argv[1] && strcmp(argv[1], "--get-checksum") == 0 ) {
        raccel_set_checksum(rtS);
        (void)printf("%u, %u, %u, %u, Version: '%s'", ssGetChecksumVal(rtS, 0),
                     ssGetChecksumVal(rtS, 1),ssGetChecksumVal(rtS, 2),
                     ssGetChecksumVal(rtS, 3), gbl_raccel_Version);
        exit(0);
    }

    /*
     * In TLC, when using RSim, we need to define variables:
     * gblNumToFiles and gblNumFrFiles -- even if the number of
     * such blocks is zero. This allows the parser to flag the user if
     * an attempt is made to provide a new file for a type of block
     * that is not used in the current model.
     */

    /*
     * If any "to file" or "from file" blocks exist, we must allocate
     * memory for "to file" and "from file" namepairs even if the
     * file names remain unchanged from the original file names.
     */
     if (gblNumToFiles>0) {
         gblToFNamepair = (FNamePair*)calloc(gblNumToFiles,sizeof(FNamePair));
         if (gblToFNamepair==NULL) {
             result = "memory allocation error (gblToFNamepair)";
             goto EXIT_POINT;
         }
     }

     if (gblNumFrFiles>0) {
         gblFrFNamepair = (FNamePair*)calloc(gblNumFrFiles,sizeof(FNamePair));
         if (gblFrFNamepair==NULL) {
             result = "memory allocation error (gblFrFNamepair)";
             goto EXIT_POINT;
         }
     }

     /*
      * Warn user about -s being obsoleted in a future version (it still gets
      * processed as normal for now).
      */
     for (tvar = 1; tvar < argc; tvar++) {

         if (argv[tvar] && strcmp(argv[tvar], "-s") == 0) {
             printf("%s", ObsoleteMsg);
         }
     }

     /*
      * Convert any -tf arguments to -s.
      */
     for (tvar = 1; tvar < argc; tvar++) {

         if (argv[tvar] && strcmp(argv[tvar], "-tf") == 0) {
             argv[tvar][1] = 's';
             argv[tvar][2] = '\0';
         }
     }

     for (tvar = 1; tvar < argc; tvar++) { /* check list of input args */

         if (argv[tvar] == NULL) continue;

         if (strcmp(argv[tvar],"-server_info_file") == 0) {
             /* create a program active token and write the pid in it. */
             FILE* fh;

             (void)sprintf(gblProgramActiveToken, "%s", argv[tvar+1]);

             fh = fopen(gblProgramActiveToken,"w");
             if (fh == NULL) {
                 result = "Unable to create active token";
                 goto EXIT_POINT;
             }
             /*(void)fprintf(fh, "Server PID: %d\n", getpid());*/
             (void)fprintf(fh, "Server PID: %d\n", (unsigned int)GetMyPID());
             fclose(fh);
             /* register a function to delete this token when program exists */
             if ( atexit(AtExitFcn) != 0 ) {
                 result = "unable set exit function";
                 goto EXIT_POINT;
             }
             /* We will not null these specific args as they will be used to return 
              * the server TCPIP port number. 
              */
             tvar = tvar+1; /* move to the next arg */
             continue;

         }   
        
         if (strcmp(argv[tvar],"-error_file") == 0) {
             (void)sprintf(gblErrorFile, "%s", argv[tvar+1]);
             argv[tvar++] = NULL;
             argv[tvar] = NULL;
             continue;
         }
        
         if(strcmp(argv[tvar],"-verbose") == 0) {
            if( strcmp(argv[tvar+1],"on") == 0) {
                gblVerboseFlag = 1;
            }
            argv[tvar] = NULL;
            argv[tvar+1] = NULL;       
            
            tvar = tvar+1; 
            continue;
         /*tvar = tvar+1;  move to the next arg */ 

         }

         if(strcmp(argv[tvar],"-ignore-arg") == 0) {
             /* This is currently used for sbruntests
                SBRUNTESTS_SESSION_ID support, for preventing rogue
                processes.
                Do NOT NULL out the arguments.  We want this argument
                to stick around for later.  External mode may see them
                as well but "-ignore-arg" is also handled by 
                external mode. */
             tvar = tvar+1; /* move to the next arg */
             continue;
         }
         
         if (argv[tvar][0] == '-') {       /* expect an option to follow "-" */
             optLen = strlen( argv[tvar] );
             if (optLen == 1) {
                 result = UsageMsg;
                 goto EXIT_POINT;
             }

             /*
              * Skip arguments which are greater than 2 chars in length.
              */
             if (optLen != 2) {
                 continue;
             }

             switch(argv[tvar][1]) {

               case 'f':  /* FromFile */
                 /*  Syntax:   -f oldfile.mat=newfile.mat
                  *
                  *  This allows a new input file "newName.mat" to replace the
                  *  original input file "oldName.mat" provided data dimensions
                  *  and data types are compatible.
                  *
                  *  If "oldname.mat" does not exist, we error out.
                  */
                 if( (tvar + 1 ) == argc || argv[tvar+1][0] == '-') {
                     result = UsageMsg;
                     goto EXIT_POINT;
                 }

                 if (frFNamepairIdx == gblNumFrFiles) {
                     result = "too many -f switches were specified";
                     goto EXIT_POINT;
                 }

                 gblFrFNamepair[frFNamepairIdx].inputString = argv[tvar+1];
                 if ((result =
                      SplitFileNamepair(&gblFrFNamepair[frFNamepairIdx])) !=
                     NULL) {
                     goto EXIT_POINT;
                 }
                 frFNamepairIdx++; /* another name pair */

                 /*
                  * Set the current argument to NULL, advance the pointer
                  * to the next argument, and NULL it out as well.  We need
                  * to NULL out processed arguments so external mode will
                  * ignore them.
                  */
                 argv[tvar++] = NULL;
                 argv[tvar]   = NULL;
                 break;

               case 'i':  /* Inport */
                 /*  Syntax:   -i inportfile.mat
                  *  If "inportfile.mat" does not exist, we error out.
                  */

                 if( gblNumRootInportBlks ==0){
                    (void)printf("No root inport in the model, external "
                        "inputs are ignored\n");
                    break;
                 }

                 if( (tvar + 1 ) == argc || argv[tvar+1][0] == '-') {
                     result = UsageMsg;
                     goto EXIT_POINT;
                 }

                 if (gblInportFileName != NULL) {
                     result = "only one -i switch is allowed\n";
                     goto EXIT_POINT;
                 }

                 gblInportFileName = argv[tvar+1];

                 /*
                  * Set the current argument to NULL, advance the pointer
                  * to the next argument, and NULL it out as well.  We need
                  * to NULL out processed arguments so external mode will
                  * ignore them.
                  */
                 argv[tvar++] = NULL;
                 argv[tvar]   = NULL;

                 break;

               case 'o':  /* OutputFile */
                 /* Syntax: -o filename.mat
                  *
                  * From argv, get OutputFile name for saving matfile
                  * logging data. The default output file name
                  * is <modelname>.mat.
                  *
                  * Only one -o switch is allowed.
                  */
                 if( (tvar + 1 ) == argc || argv[tvar+1][0] == '-') {
                     result = UsageMsg;
                     goto EXIT_POINT;
                 }
                 if (gblMatLoggingFilename != NULL) {
                     result = "only one -o switch is allowed\n";
                     goto EXIT_POINT;
                 }
                 
                 gblMatLoggingFilename = argv[tvar+1];
                 
                 /*
                  * Set the current argument to NULL, advance the pointer
                  * to the next argument, and NULL it out as well.  We need
                  * to NULL out processed arguments so external mode will
                  * ignore them.
                  */
                 argv[tvar++] = NULL;
                 argv[tvar]   = NULL;
                 break;

               case 'm':  /* SimMetadataFile */
                 /* Syntax: -m filename.mat
                  *
                  * From argv, get SimMetadataFile name for saving matfile
                  * sim metadata. 
                  *
                  * Only one -m switch is allowed.
                  */
                 if( (tvar + 1 ) == argc || argv[tvar+1][0] == '-') {
                     result = UsageMsg;
                     goto EXIT_POINT;
                 }
                 if (gblSimMetadataFileName != NULL) {
                     result = "only one -m switch is allowed\n";
                     goto EXIT_POINT;
                 }
                 
                 gblSimMetadataFileName = argv[tvar+1];


                 /*
                  * Set the current argument to NULL, advance the pointer
                  * to the next argument, and NULL it out as well.  We need
                  * to NULL out processed arguments so external mode will
                  * ignore them.
                  */
                 argv[tvar++] = NULL;
                 argv[tvar]   = NULL;
                 break;

               case 'd':  /* s-function info file name */
                 /* Syntax: -d filename.mat
                  *
                  * From argv, get file containing s-function info
                  *
                  * Only one -d switch is allowed.
                  */
                 if( (tvar + 1 ) == argc || argv[tvar+1][0] == '-') {
                     result = UsageMsg;
                     goto EXIT_POINT;
                 }
                 if (gblSFcnInfoFileName != NULL) {
                     result = "only one -d switch is allowed\n";
                     goto EXIT_POINT;
                 }
                 
                 gblSFcnInfoFileName = argv[tvar+1];

                 /*
                  * Set the current argument to NULL, advance the pointer
                  * to the next argument, and NULL it out as well.  We need
                  * to NULL out processed arguments so external mode will
                  * ignore them.
                  */
                 argv[tvar++] = NULL;
                 argv[tvar]   = NULL;
                 break;

        case 'l':  /* SigstreamFile */
                 /* Syntax: -l filename.mat
                  *
                  * From argv, get SigstreamFile name for saving matfile
                  * signal logging data. The default output file name
                  * is <modelname>_logsout.mat.
                  *
                  * Only one -l switch is allowed.
                  */
                 if( (tvar + 1 ) == argc || argv[tvar+1][0] == '-') {
                     result = UsageMsg;
                     goto EXIT_POINT;
                 }
                 if (gblMatSigstreamLoggingFilename != NULL) {
                     result = "only one -l switch is allowed\n";
                     goto EXIT_POINT;
                 }
                 
                 gblMatSigstreamLoggingFilename = argv[tvar+1];
                 
                 /*
                  * Set the current argument to NULL, advance the pointer
                  * to the next argument, and NULL it out as well.  We need
                  * to NULL out processed arguments so external mode will
                  * ignore them.
                  */
                 argv[tvar++] = NULL;
                 argv[tvar]   = NULL;
                 break;

               case 'e':  /* SigLogSelectorFile */
                 /* Syntax: -e filename.mat
                  *
                  * From argv, get SigLogSelectorFile name for loading
                  * SigLogSelector settings.
                  *
                  * Only one -e switch is allowed.
                  */
                 if( (tvar + 1 ) == argc || argv[tvar+1][0] == '-') {
                     result = UsageMsg;
                     goto EXIT_POINT;
                 }
                 if (gblMatSigLogSelectorFilename != NULL) {
                     result = "only one -e switch is allowed\n";
                     goto EXIT_POINT;
                 }
                 
                 gblMatSigLogSelectorFilename = argv[tvar+1];
                 
                 /*
                  * Set the current argument to NULL, advance the pointer
                  * to the next argument, and NULL it out as well.  We need
                  * to NULL out processed arguments so external mode will
                  * ignore them.
                  */
                 argv[tvar++] = NULL;
                 argv[tvar]   = NULL;
                 break;

               case 'R':  /* Simulation Data Repository file */
                   /* Syntax: -R filename.dmr
                   *
                   * From argv, get Simulation Data Repository name for streaming
                   *
                   * Only one -R switch is allowed.
                   */
                   if((tvar + 1) == argc || argv[tvar + 1][0] == '-') {
                       result = UsageMsg;
                       goto EXIT_POINT;
                   }
                   if(gblSimDataRepoFilename != NULL) {
                       result = "only one -R switch is allowed\n";
                       goto EXIT_POINT;
                   }

                   gblSimDataRepoFilename = argv[tvar + 1];

                   /*
                   * Set the current argument to NULL, advance the pointer
                   * to the next argument, and NULL it out as well.  We need
                   * to NULL out processed arguments so external mode will
                   * ignore them.
                   */
                   argv[tvar++] = NULL;
                   argv[tvar] = NULL;
                   break;

               case 'T':  /* ConcurrencyResolvingToFileSuffix */
                 /* Syntax: -T ToFileSuffix
                  *
                  * Only one -T switch is allowed.
                  */
                 if( (tvar + 1 ) == argc || argv[tvar+1][0] == '-') {
                     result = UsageMsg;
                     goto EXIT_POINT;
                 }
                 if (gblToFileSuffix != NULL) {
                     result = "only one ToFile suffix is allowed\n";
                     goto EXIT_POINT;
                 }
                 
                 gblToFileSuffix = argv[tvar+1];
                 
                 /*
                  * Set the current argument to NULL, advance the pointer
                  * to the next argument, and NULL it out as well.  We need
                  * to NULL out processed arguments so external mode will
                  * ignore them.
                  */
                 argv[tvar++] = NULL;
                 argv[tvar]   = NULL;
                 break;
                 
               case 'p':  /* Param File */
                 /*
                  * Syntax: -p paramfile.mat
                  *
                  *  From argv, get the input string containing Param file
                  *  name. This file should contain a new "rtP" parameter
                  *  vector with a completely new parameter vector.
                  *
                  *  Only one occurrence of -p paramfile.mat is valid.
                  */
                 
#if defined RSIM_PARAMETER_LOADING
                 
                 if( (tvar + 1 ) == argc || argv[tvar+1][0] == '-') {
                     result = UsageMsg;
                     goto EXIT_POINT;
                 }
                 
                 if (gblParamFilename != NULL) {
                     result = "only one -p switch is allowed\n";
                     goto EXIT_POINT;
                 }
                 
                 /*
                  * Put name of file containing parameter structure into
                  * a global variable. Once the name is non-NULL, RSim
                  * will plan on reading the MAT-file and replacing
                  * the rtP structure with a new parameter set
                  */
                 gblParamFilename = argv[tvar+1];
                                  
                 /*
                  * Set the current argument to NULL, advance the pointer
                  * to the next argument, and NULL it out as well.  We need
                  * to NULL out processed arguments so external mode will
                  * ignore them.
                  */
                 argv[tvar++] = NULL;
                 argv[tvar]   = NULL;
#else
                 /* If the parameter tuning infrastructure does not exist, do not
                  * try to attempt parsing the parameter structures
                  */
                 result = "unable to load parameters from a MAT-File."
                     "To fix this error regenerate the RSim executable with parameter loading option enabled\n";
                 goto EXIT_POINT;
#endif
                 break;
                 
               case 's':  /* Stop Time */
                 /* Syntax: -s <time>, -tf <time>
                  *
                  * From argv, get the final time value when the simulation
                  * will end.
                  */
                 if( (tvar + 1 ) == argc || ( argv[tvar+1][0] == '-')  ) {
                     result = UsageMsg;
                     goto EXIT_POINT;
                 }
                 
                 if (gblFinalTimeChanged) {
                     result = "only one -s or -tf switch is allowed\n";
                     goto EXIT_POINT;
                 }
                 
                 {
                     char_T str2[200], tmpstr[2];
                     
                     sscanf(argv[tvar+1],"%200s",str2);
                     gblFinalTimeChanged = 1;
                     if (strcmp(str2, "inf") == 0) {
                         gblFinalTime = rtInf;
                     } else {
                         if (sscanf(str2,"%lf%1s",&gblFinalTime,tmpstr) != 1 ) {
                             result = "invalid -s or -tf switch argument "
                                 "specified.  stop time must be a real "
                                 "value or inf\n";
                             goto EXIT_POINT;
                         }
                     }
                 }
                 
                 /*
                  * Set the current argument to NULL, advance the pointer
                  * to the next argument, and NULL it out as well.  We need
                  * to NULL out processed arguments so external mode will
                  * ignore them.
                  */
                 argv[tvar++] = NULL;
                 argv[tvar]   = NULL;
                 break;
                 
               case 'S':  /* Solver options */
                 /*
                  * Syntax: -S solver_opts.mat
                  *
                  *  From argv, get the input string containing solver options
                  *  file name.
                  *
                  *  Only one occurrences of -S solver_opts.mat is valid.
                  */
                 if( (tvar + 1 ) == argc || argv[tvar+1][0] == '-') {
                     result = UsageMsg;
                     goto EXIT_POINT;
                 }
                 
                 if (gblSolverOptsFilename != NULL) {
                     result = "only one -S switch is allowed\n";
                     goto EXIT_POINT;
                 }
                 
                 /*
                  * Put name of file containing parameter structure into
                  * a global variable. Once the name is non-NULL, RSim
                  * will plan on reading the MAT-file and replacing
                  * the rtP structure with a new parameter set
                  */
                 gblSolverOptsFilename = argv[tvar+1];
                 
                 
                 /*
                  * Set the current argument to NULL, advance the pointer
                  * to the next argument, and NULL it out as well.  We need
                  * to NULL out processed arguments so external mode will
                  * ignore them.
                  */
                 argv[tvar++] = NULL;
                 argv[tvar]   = NULL;
                 break;
                 
               case 'L':  /* Stop Time */
                 /* Syntax: -L <timeLimit>
                  *
                  */
                 if( (tvar + 1 ) == argc || ( argv[tvar+1][0] == '-')  ) {
                     result = UsageMsg;
                     goto EXIT_POINT;
                 }
                 
                 if (gblTimeLimit != -1.0) {
                     result = "only one -L switch is allowed\n";
                     goto EXIT_POINT;
                 }
                 
                 {
                     char_T str2[200], tmpstr[2];
                     
                     sscanf(argv[tvar+1],"%200s",str2);
                     if ( (sscanf(str2,"%d%1s",&gblTimeLimit,tmpstr) != 1) ||
                          (gblTimeLimit <= 0) ) {
                         result = "invalid -L switch argument specified.\n"
                             "Run time limit must be a positive integer\n";
                         goto EXIT_POINT;
                     }
                 }
                 
                 /*
                  * Set the current argument to NULL, advance the pointer
                  * to the next argument, and NULL it out as well.  We need
                  * to NULL out processed arguments so external mode will
                  * ignore them.
                  */
                 argv[tvar++] = NULL;
                 argv[tvar]   = NULL;
                 break;
                 
               case 't':  /* ToFile */
                 /*
                  * Get the input string containing a "To File" name pair, for
                  *  example:   -t oldfile.mat=newfile.mat
                  *  The "newfile.mat" will be used as a replacement for the
                  *  original output filename "oldname.mat" so multiple runs
                  *  can be made without overwriting output files.
                  */
                 if( (tvar + 1 ) == argc || argv[tvar+1][0] == '-') {
                     result = UsageMsg;
                     goto EXIT_POINT;
                 }
                 
                 if (toFNamepairIdx == gblNumToFiles) {
                     result = "too many -t switches were specified";
                     goto EXIT_POINT;
                 }
                 
                 gblToFNamepair[toFNamepairIdx].inputString = argv[tvar+1];
                 if ((result =
                      SplitFileNamepair(&gblToFNamepair[toFNamepairIdx])) !=
                     NULL) {
                     goto EXIT_POINT;
                 }
                 toFNamepairIdx++; /* another name pair */
                 
                 /*
                  * Set the current argument to NULL, advance the pointer
                  * to the next argument, and NULL it out as well.  We need
                  * to NULL out processed arguments so external mode will
                  * ignore them.
                  */
                 argv[tvar++] = NULL;
                 argv[tvar]   = NULL;
                 break;
                 
               case 'v': /* Verbose switch */
                 /* Syntax: -v
                  *
                  *  Turn on the global verbose flag.
                  */
                 if (gblVerbose != 0) {
                     result = "only one -v switch is allowed";
                     goto EXIT_POINT;
                 }
                 gblVerbose = 1;
                 
                 /*
                  * Set the current argument to NULL.  We need to NULL out
                  * processed arguments so external mode will ignore them.
                  */
                 argv[tvar] = NULL;
                 break;
                 
               case 'P': /* RunningInParallel switch */
                 /* Syntax: -P
                  *
                  *  Turn on the global RunningInParallel flag.
                  */
                 if (gblRunningInParallel) {
                     result = "only one -P switch is allowed";
                     goto EXIT_POINT;
                 }
                 gblRunningInParallel = 1U;
                 
                 /*
                  * Set the current argument to NULL.  We need to NULL out
                  * processed arguments so external mode will ignore them.
                  */
                 argv[tvar] = NULL;
                 break;
                 
               default:
                 break;
             }
         }
     } /* end parse loop */
     
     /*
      * Check for external mode arguments.
      */
     rtExtModeParseArgs(argc, (const char_T **) argv, NULL);

     gblExtModeEnabled = ExtWaitForStartPkt();

    /* Target connectivity initialization */
    result = TgtConnInit(argc, argv);
    if (result != NULL) return(result);

     /*
      * Check for unprocessed ("unhandled") args.
      */
     {
         int i;
         for (i=1; i<argc; i++) {
             if (argv[i] != NULL) {
                 result = UsageMsg;
                 goto EXIT_POINT;
             }
         }
     }

     if (gblMatLoggingFilename == NULL)  {
         gblMatLoggingFilename = MATFILE; /* default is model.mat */
     }
     /* Check that in parallel simulation with ToFile blocks the 
        ConcurrencyResolvingToFileSuffix is specified */
     {
         if (gblRunningInParallel && (gblNumToFiles > 0) && 
             (gblToFileSuffix == NULL)) {
             result = "'ToFile' blocks when running parallel simulations can cause concurrency issues. This can be resolved by specifying the ConcurrencyResolvingToFileSuffix in the simset options";
             goto EXIT_POINT;
         }
     }
    /* Check that none of the new "To File" output filenames
     * is the same as the output file name
     * Also check that none of the new "To File" output filenames
     * is mapped assigned to different old "To File" output filenames
     */

     {
         int i,j;
         for (i = 0; i < toFNamepairIdx; i++ ) {
             if(strcmp(gblMatLoggingFilename,gblToFNamepair[i].newName) == 0){
                 result = " 'To File' filename cannot be the same as output filename of the model";
                 goto EXIT_POINT;

             }
         }
         for (j = i+1; j <  toFNamepairIdx; j++ ){
             if(strcmp(gblToFNamepair[i].oldName, gblToFNamepair[j].oldName) == 0){
                 (void)printf("'To File' filename '%s' is replaced more than once with the -t option\n",
                              gblToFNamepair[j].oldName);
                 result = "Multiple replacement of 'To File' filenames is not allowed\n";
                 goto EXIT_POINT;
             }

             if(strcmp(gblToFNamepair[i].newName, gblToFNamepair[j].newName) == 0){
                 (void)printf("'To File' filename replacement '%s' is used more than once with the -t option\n",
                              gblToFNamepair[j].newName);
                 result = "All 'To File' filenames must be  unique\n";
                 goto EXIT_POINT;
             }
         }
     }

     if (gblVerbose) {
         int_T i;
         if (gblParamFilename != NULL) {
             (void)printf("** Reading parameters");
             if (gblParamCellIndex != -1) {
                 (void)printf(" at cell array index %d", gblParamCellIndex);
             }
             (void)printf(" from file \"%s\"\n", gblParamFilename);
         } else {
             (void)printf("** Using default model parameters (no parameter "
                          "file specified)\n");
         }

         if (gblSolverOptsFilename != NULL) {
             (void)printf("** Reading solver options");
             if (gblSolverOptsArrIndex != -1) {
                 (void)printf(" at array index %d",gblSolverOptsArrIndex);
             }
             (void)printf(" from file \"%s\"\n", gblSolverOptsFilename);
         } else {
             (void)printf("** Using solver options specified on the model "
                          "(no solver options file specified)\n");
         }

         if (gblFinalTimeChanged) {
             if (gblFinalTime == rtInf) {
                 (void)printf("** Setting stop time to infinity.\n");
             } else {
                 (void)printf("** Stop time = %.16g\n", gblFinalTime);
             }
         }

         (void)printf("** Output filename = %s\n", gblMatLoggingFilename);

         (void)printf("** Sigstream filename = %s\n", gblMatSigstreamLoggingFilename);

         (void)printf("** SigLogSelector filename = %s\n", gblMatSigLogSelectorFilename);

         (void)printf("** Simulation Data Repository filename = %s\n", gblSimDataRepoFilename);

         for (i=0; i < toFNamepairIdx; i++) {
             (void)printf("** Replacing ToFile \"%s\" with \"%s\"\n",
                          gblToFNamepair[i].oldName,
                          gblToFNamepair[i].newName);
         }

         for (i=0; i < frFNamepairIdx; i++) {
             (void)printf("** Replacing FromFile \"%s\" with \"%s\"\n",
                          gblFrFNamepair[i].oldName,
                          gblFrFNamepair[i].newName);
         }

     }

 EXIT_POINT:
     return(result);

} /* end ParseArgs */

/* LocalWords:  getpid TUtable gbl TUtables rsim modelname Pport Namepair RSim
 * LocalWords:  origname FName Fromfile matfile Tofile mydata myoutfile fh
 * LocalWords:  originfile newinfile origtofile newtofile fromfile namepairs
 * LocalWords:  FNamepair pid tvar oldfile newfile oldname inportfile Logsout
 * LocalWords:  logsout paramfile lf
 */
