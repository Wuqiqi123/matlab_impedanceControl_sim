/*
 * Copyright 2007-2013 The MathWorks, Inc.
 *
 */

#ifndef __RACCEL_SUP_H__
#define __RACCEL_SUP_H__

#ifdef __cplusplus
extern "C" {
#endif

/* data */
extern int_T       gblFinalTimeChanged;
extern double      gblFinalTime;
extern const char* gblMatLoggingFilename;
extern const char* gblSimMetadataFileName;
extern const char* gblMatSigstreamLoggingFilename;
extern const char* gblMatSigLogSelectorFilename;
extern const char* gblSimDataRepoFilename;

/* functions */

extern int InstallRunTimeLimitHandler(void);
extern const char *ParseArgs(int_T argc, char_T *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* __RACCEL_SUP_H__ */
