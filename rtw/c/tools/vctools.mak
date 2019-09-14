# Copyright 1994-2000 The MathWorks, Inc.
#
# File    : vctools.mak   
# Abstract:
#	Setup Visual tools for nmake.

!ifndef DEBUG_BUILD
DEBUG_BUILD = 0
!endif

!if "$(DEBUG_BUILD)" == "0"
NODEBUG = 1
!endif

!if "$(VISUAL_VER)" == "11.0" || "$(VISUAL_VER)" == "12.0" || "$(VISUAL_VER)" == "14.0" || "$(VISUAL_VER)" == "15.0"
!include $(MATLAB_ROOT)\rtw\c\tools\vcdefs.mak
!else
!include <ntwin32.mak>
APPVER = 5.0
!endif

CC     = cl
LD     = link
LIBCMD = lib

CFLAGS_VERSPECIFIC = /wd4996 /fp:precise
CPPFLAGS_VERSPECIFIC = $(CFLAGS_VERSPECIFIC) /EHsc-

# Setting mex architecture argument depends on computer type
MEX_ARCH = -win32
ML_ARCH = win32
!if "$(COMPUTER)" == "PCWIN64"
MEX_ARCH = -win64
ML_ARCH = win64
!endif

#
# Default optimization options.
#
DEFAULT_OPT_OPTS = -Od

#[eof] vctools.mak
