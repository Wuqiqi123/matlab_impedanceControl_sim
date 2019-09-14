# Copyright 1994-2017 The MathWorks, Inc.
#
# File    : lcctools.mak   
# Abstract:
#	Setup LCC tools for gmake.

CC     = $(LCC)\bin\lcc64
LD     = $(LCC)\bin\lcclnk64
LIB    = $(LCC)\lib64
LIBCMD = $(LCC)\bin\lcclib64
BUILDLIB = 

DEFAULT_OPT_OPTS = 
DEFAULT_CFLAGS = -nodeclspec -c

#------------------------------------#
# Setup INCLUDES, DSP_MEX source dir #
#------------------------------------#

# DSP Blockset non-TLC S-fcn source path
# and additional file include paths
DSP_MEX      = $(MATLAB_ROOT)\toolbox\dspblks\dspmex
DSP_SIM      = $(MATLAB_ROOT)\toolbox\dspblks\src\sim
DSP_RT       = $(MATLAB_ROOT)\toolbox\dspblks\src\rt
DSP_INCLUDES = \
	-I$(DSP_SIM) \
	-I$(DSP_RT)

BLOCKSET_INCLUDES = $(DSP_INCLUDES) \
                   -I$(MATLAB_ROOT)\toolbox\commblks\commmex

COMPILER_INCLUDES = -I$(LCC)\include64

INCLUDES = -I. -I.. $(BLOCKSET_INCLUDES) \
           $(COMPILER_INCLUDES) $(USER_INCLUDES)


