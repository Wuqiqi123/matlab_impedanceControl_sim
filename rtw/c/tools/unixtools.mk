# Copyright 1994-2016 The MathWorks, Inc.
#
# File    : unixtools.mk   
# Abstract:
#	Setup Unix tools for GNU make

ARCH := $(shell echo "$(COMPUTER)" | tr '[A-Z]' '[a-z]')
OS:=$(shell uname)

#
# Modify the following macros to reflect the tools you wish to use for
# compiling and linking your code.
#


DEFAULT_OPT_OPTS = -O0
ANSI_OPTS        =
CPP_ANSI_OPTS    = 
CPP              = c++
LD               = $(CPP)
SYSLIBS          =
LDFLAGS          =
ARCH_SPECIFIC_LDFLAGS =
SHRLIBLDFLAGS    =
AR               = ar

# Override based on platform if needed

GCC_WALL_FLAG     :=
GCC_WALL_FLAG_MAX :=
ifeq ($(COMPUTER),GLNXA64)
  MATLAB_ARCH_BIN = $(MATLAB_ROOT)/bin/glnxa64
  CC  = gcc
  CPP = g++
  DEFAULT_OPT_OPTS = -O0
  SHRLIBLDFLAGS = -shared -Wl,--no-undefined -Wl,--version-script,
  COMMON_ANSI_OPTS = -fwrapv -fPIC
  ifneq ($(NOT_PEDANTIC), 1)
    PEDANTICFLAG = -pedantic
  else
    PEDANTICFLAG = 
  endif
  ANSI_OPTS        = -ansi $(PEDANTICFLAG) -Wno-long-long $(COMMON_ANSI_OPTS)
  CPP_ANSI_OPTS    = -std=c++98 $(PEDANTICFLAG) -Wno-long-long $(COMMON_ANSI_OPTS)
  # Allow ISO-C functions like fmin to be called
  ifeq ($(TGT_FCN_LIB),ISO_C)
    ANSI_OPTS     = -std=c99 $(PEDANTICFLAG) $(COMMON_ANSI_OPTS)
    CPP_ANSI_OPTS = $(COMMON_ANSI_OPTS)
  else
    ifeq ($(TGT_FCN_LIB),GNU)
      ANSI_OPTS     = -std=gnu99 $(PEDANTICFLAG) $(COMMON_ANSI_OPTS)
      CPP_ANSI_OPTS = $(COMMON_ANSI_OPTS)
    else
      ifeq ($(SINGLE_LINE_COMMENTS),1)
	  ANSI_OPTS     = $(COMMON_ANSI_OPTS)
      endif	
      ifneq ($(TGT_FCN_LIB),ISO_C++)
        ifeq ($(NON_ANSI_TRIG_FCN), 1)
          ANSI_OPTS     = 
          CPP_ANSI_OPTS = 
        endif
      endif	
    endif
  endif
  # These definitions are used by targets that have the WARN_ON_GLNX option
  GCC_WARN_OPTS     := -Wall -W -Wwrite-strings -Winline \
                       -Wpointer-arith -Wcast-align

  # Command line options valid for C/ObjC but not for C++
  ifneq ($(TARGET_LANG_EXT),cpp)
    GCC_WARN_OPTS := $(GCC_WARN_OPTS) -Wstrict-prototypes -Wnested-externs 
  endif

  # if TGT_FCN_LIB is C89/90 add -Wno-long-long flag. This flag will stop
  # gcc from throwing warning: ISO C90 does not support 'long long'.
  ifneq ($(NON_ANSI_TRIG_FCN), 1)
    GCC_WARN_OPTS     := $(GCC_WARN_OPTS) -Wno-long-long
  endif

  GCC_WARN_OPTS_MAX := $(GCC_WARN_OPTS) -Wcast-qual -Wshadow

  ifeq ($(WARN_ON_GLNX), 1)
    GCC_WALL_FLAG     := $(GCC_WARN_OPTS)
    GCC_WALL_FLAG_MAX := $(GCC_WARN_OPTS_MAX)
  endif
endif

ifeq ($(COMPUTER),MACI64)
  MATLAB_ARCH_BIN = $(MATLAB_ROOT)/bin/maci64
  DEFAULT_OPT_OPTS := -O0
  XCODE_SDK_VER   := $(shell perl $(MATLAB_ROOT)/rtw/c/tools/macsdkver.pl)
  XCODE_SDK       := MacOSX$(XCODE_SDK_VER).sdk
  XCODE_DEVEL_DIR := $(shell xcode-select -print-path)
  MW_SDK_ROOT  := $(XCODE_DEVEL_DIR)/Platforms/MacOSX.platform/Developer/SDKs/$(XCODE_SDK) 
  
  # architecture support x86_64 
  ARCHS = x86_64
  CC  = xcrun clang -arch $(ARCHS) -isysroot $(MW_SDK_ROOT)
  CPP = xcrun clang++ -arch $(ARCHS) -isysroot $(MW_SDK_ROOT)
  AR  = xcrun ar
  
  ANSI_OPTS = -fno-common -fexceptions 

  ARCH_SPECIFIC_LDFLAGS = -Wl,-rpath,$(MATLAB_ROOT)/bin/$(ARCH) -Wl,-rpath,@executable_path

  #instead of using -bundle, use -dynamiclib flag to make the lib dlopen compatible
  SHRLIBLDFLAGS = \
    -dynamiclib -install_name @rpath/$(notdir $(PRODUCT)) \
    -Wl,$(LD_NAMESPACE) $(LD_UNDEFS) -Wl,-exported_symbols_list,
endif
