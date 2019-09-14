/*
 * Copyright 1994-2005 The MathWorks, Inc.
 *
 */

#ifndef __RSIM_SUP_H__
#define __RSIM_SUP_H__

#ifdef __cplusplus
extern "C" {
#endif

/* data */
extern int_T       gblFinalTimeChanged;
extern double      gblFinalTime;
extern const char* gblMatLoggingFilename;

/* functions */

extern int InstallRunTimeLimitHandler(void);
extern const char *ParseArgs(int_T argc, char_T *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* __RSIM_SUP_H__ */
