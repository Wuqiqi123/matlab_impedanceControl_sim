# Copyright 1994-2018 The MathWorks, Inc.
#
#
# Abstract:
#       Template makefile for building a Windows-based stand-alone embedded
#       real-time version of Simulink model using generated C code and the
#          Microsoft Visual C/C++ compiler for x64.
#
#       Note that this template is automatically customized by the build 
#       procedure to create "<model>.mk"
#
#       The following defines can be used to modify the behavior of the
#       build:
#         OPT_OPTS       - Optimization option. See DEFAULT_OPT_OPTS in
#                          vctools.mak for default.
#         OPTS           - User specific options.
#         CPP_OPTS       - C++ compiler options.
#         USER_SRCS      - Additional user sources, such as files needed by
#                          S-functions.
#         USER_INCLUDES  - Additional include paths
#                          (i.e. USER_INCLUDES="-Iwhere-ever -Iwhere-ever2")
#
#       To enable debugging:
#         set DEBUG_BUILD = 1, which will trigger OPTS=-Zi (may vary with
#                               compiler version, see compiler doc) 
#
#       This template makefile is designed to be used with a system target
#       file that contains 'rtwgensettings.BuildDirSuffix' see ert.tlc


#------------------------ Macros read by make_rtw -----------------------------
#
# The following macros are read by the build procedure:
#
#  MAKECMD         - This is the command used to invoke the make utility
#  HOST            - What platform this template makefile is targeted for
#                    (i.e. PC or UNIX)
#  BUILD           - Invoke make from the build procedure (yes/no)?
#  SYS_TARGET_FILE - Name of system target file.

MAKECMD         = nmake
HOST            = PC
BUILD           = yes
SYS_TARGET_FILE = any
BUILD_SUCCESS	= ^#^#^# Created
COMPILER_TOOL_CHAIN = vcx64

# Opt in to simplified format by specifying compatible Toolchain
TOOLCHAIN_NAME = [\
    "Microsoft Visual C++ 2017 v15.0 | nmake (64-bit Windows)", \
    "Microsoft Visual C++ 2015 v14.0 | nmake (64-bit Windows)", \
    "Microsoft Visual C++ 2013 v12.0 | nmake (64-bit Windows)"]

#---------------------- Tokens expanded by make_rtw ---------------------------
#
# The following tokens, when wrapped with "|>" and "<|" are expanded by the
# build procedure.
#
#  MODEL_NAME          - Name of the Simulink block diagram
#  MODEL_MODULES       - Any additional generated source modules
#  MAKEFILE_NAME       - Name of makefile created from template makefile <model>.mk
#  MATLAB_ROOT         - Path to where MATLAB is installed.
#  MATLAB_BIN          - Path to MATLAB executable.
#  S_FUNCTIONS         - List of additional S-function modules.
#  S_FUNCTIONS_LIB     - List of S-functions libraries to link.
#  NUMST               - Number of sample times
#  NCSTATES            - Number of continuous states
#  BUILDARGS           - Options passed in at the command line.
#  MULTITASKING        - yes (1) or no (0): Is solver mode multitasking
#  INTEGER_CODE        - yes (1) or no (0): Is generated code purely integer
#  MAT_FILE            - yes (1) or no (0): Should mat file logging be done,
#                        if 0, the generated code runs indefinitely
#  MULTI_INSTANCE_CODE - Is the generated code multi instantiable (1/0)?
#  MODELREFS           - List of referenced models
#  SHRLIBTARGET        - Is this build intended for generation of a shared library instead 
#                        of executable (1/0)?
#  MAKEFILEBUILDER_TGT - Is this build performed by the MakefileBuilder class
#                        e.g. to create a PIL executable?
#  STANDALONE_SUPPRESS_EXE - Build the standalone target but only create object code modules 
#                            and do not build an executable

MODEL                   = PMSM_VC_current_loop
MODULES                 = 
MAKEFILE                = PMSM_VC_current_loop.mk
MATLAB_ROOT             = G:\MATLAB
ALT_MATLAB_ROOT         = G:\MATLAB
MATLAB_BIN              = G:\MATLAB\bin
ALT_MATLAB_BIN          = G:\MATLAB\bin
MASTER_ANCHOR_DIR       = 
START_DIR               = G:\MATLAB\SCARA
S_FUNCTIONS             = 
S_FUNCTIONS_LIB         = 
NUMST                   = 4
NCSTATES                = 0
BUILDARGS               = 
MULTITASKING            = 0
INTEGER_CODE            = 0
MAT_FILE                = 0
ALLOCATIONFCN           = 0
ONESTEPFCN              = 0
TERMFCN                 = 1
ENABLE_SLEXEC_SSBRIDGE  = 0
MULTI_INSTANCE_CODE     = 0
CLASSIC_INTERFACE       = 1
MODELREFS               = 
SHARED_SRC              = 
SHARED_SRC_DIR          = 
SHARED_BIN_DIR          = 
SHARED_LIB              = 
GEN_SAMPLE_MAIN         = 0
TARGET_LANG_EXT         = c
COMPUTER                = PCWIN64
SHRLIBTARGET            = 0
MAKEFILEBUILDER_TGT     = 0
STANDALONE_SUPPRESS_EXE = 0
VISUAL_VER              = 15.0
OPTIMIZATION_FLAGS      = /Od /Oy- /DNDEBUG
ADDITIONAL_LDFLAGS      = 
INTERLEAVED_COMPLEX_FLAGS = -R2018a
DEFINES_CUSTOM          = 
SYSTEM_LIBS             = /LIBPATH:"G:\MATLAB\extern\lib\win64\microsoft" libmwipp.lib libut.lib libmwmathutil.lib libmwsl_simtarget_instrumentation.lib libmwsl_simtarget_core.lib libmwsl_fileio.lib libmwsigstream.lib libmwslexec_simlog.lib libmwsl_AsyncioQueue.lib sf_runtime.lib libmwsimulink.lib libmwslexec_simbridge.lib libmwstringutil.lib libmwslio_core.lib libmwslio_clients.lib libmwsl_services.lib
MODEL_HAS_DYNAMICALLY_LOADED_SFCNS = 0

# To enable debugging:
# set DEBUG_BUILD = 1
DEBUG_BUILD             = 0

#--------------------------- Model and reference models -----------------------
MODELLIB                  = PMSM_VC_current_looplib.lib
MODELREF_LINK_LIBS        = 
MODELREF_LINK_RSPFILE     = PMSM_VC_current_loop_ref.rsp
MODELREF_INC_PATH         = 
COMPILER_COMMAND_FILE     = PMSM_VC_current_loop_comp.rsp
RELATIVE_PATH_TO_ANCHOR   = ..\..\..
MODELREF_TARGET_TYPE      = SIM
ISPROTECTINGMODEL         = NOTPROTECTING
PROT_CAPIC_SUFFIX         = _capi.c
PROT_CAPIO_SUFFIX         = _capi_host.obj

!if "$(MATLAB_ROOT)" != "$(ALT_MATLAB_ROOT)"
MATLAB_ROOT = $(ALT_MATLAB_ROOT)
!endif
!if "$(MATLAB_BIN)" != "$(ALT_MATLAB_BIN)"
MATLAB_BIN = $(ALT_MATLAB_BIN)
!endif
MATLAB_ARCH_BIN = $(MATLAB_BIN)\win64

#--------------------------- Tool Specifications ------------------------------

CPU = AMD64
!include $(MATLAB_ROOT)\rtw\c\tools\vctools.mak
APPVER = 5.02

CMD_FILE             = $(MODEL).rsp

#------------------------------ Include/Lib Path ------------------------------


BUILDINFO_INCLUDES = $(BUILDINFO_INCLUDES);$(MATLAB_ROOT)\SCARA
BUILDINFO_INCLUDES = $(BUILDINFO_INCLUDES);$(MATLAB_ROOT)\SCARA\slprj\accel\PMSM_VC_current_loop
BUILDINFO_INCLUDES = $(BUILDINFO_INCLUDES);$(MATLAB_ROOT)\Motor_controltest\双周期控制仿真
BUILDINFO_INCLUDES = $(BUILDINFO_INCLUDES);$(MATLAB_ROOT)\extern\include
BUILDINFO_INCLUDES = $(BUILDINFO_INCLUDES);$(MATLAB_ROOT)\simulink\include
BUILDINFO_INCLUDES = $(BUILDINFO_INCLUDES);$(MATLAB_ROOT)\rtw\c\src
BUILDINFO_INCLUDES = $(BUILDINFO_INCLUDES);$(MATLAB_ROOT)\toolbox\physmod\powersys\facts\facts
BUILDINFO_INCLUDES = $(BUILDINFO_INCLUDES);$(MATLAB_ROOT)\toolbox\physmod\powersys\DR\DR

INCLUDE = .;$(RELATIVE_PATH_TO_ANCHOR);;$(BUILDINFO_INCLUDES);$(INCLUDE)

!if "$(SHARED_SRC_DIR)" != ""
INCLUDE = $(INCLUDE);$(SHARED_SRC_DIR)
!endif


#----------------- Compiler and Linker Options --------------------------------

# Optimization Options
OPT_OPTS = $(DEFAULT_OPT_OPTS)

# General User Options
!if "$(DEBUG_BUILD)" == "0"
DBG_FLAG =
!else
#   Set OPT_OPTS=-Zi and any additional flags for debugging
DBG_FLAG = -Zi
!endif

!if "$(MODELREF_TARGET_TYPE)" == "SIM"
MDLREF_FLAG = -DMDL_REF_SIM_TGT=1 
!if "$(ENABLE_SLEXEC_SSBRIDGE)" != "0"
MDLREF_FLAG = $(MDLREF_FLAG) -DENABLE_SLEXEC_SSBRIDGE=$(ENABLE_SLEXEC_SSBRIDGE)
!endif
!else
MDLREF_FLAG =
!endif

!if "$(OPTIMIZATION_FLAGS)" != ""
CC_OPTS = $(OPTS) $(OPTIMIZATION_FLAGS)
!else
CC_OPTS = $(OPT_OPTS) $(OPTS) 
!endif

CPP_REQ_DEFINES = -DMODEL=$(MODEL) -DNUMST=$(NUMST) -DNCSTATES=$(NCSTATES) \
		  -DMAT_FILE=$(MAT_FILE) -DINTEGER_CODE=$(INTEGER_CODE) \
		  -DONESTEPFCN=$(ONESTEPFCN) -DTERMFCN=$(TERMFCN) \
		  -DHAVESTDIO -DMULTI_INSTANCE_CODE=$(MULTI_INSTANCE_CODE) \
		  -DCLASSIC_INTERFACE=$(CLASSIC_INTERFACE) \
		  -DALLOCATIONFCN=$(ALLOCATIONFCN) \
		  $(DEFINES_CUSTOM)

!if "$(MODELREF_TARGET_TYPE)" == "SIM"
CPP_REQ_DEFINES = $(CPP_REQ_DEFINES) $(MDLREF_FLAG)
!else
CPP_REQ_DEFINES = $(CPP_REQ_DEFINES) -DMT=$(MULTITASKING)
!endif


CPP_REQ_DEFINES = $(CPP_REQ_DEFINES) -DMODEL_HAS_DYNAMICALLY_LOADED_SFCNS=$(MODEL_HAS_DYNAMICALLY_LOADED_SFCNS)

CVARSFLAG = $(cvarsdll)

CFLAGS = $(cflags) @$(COMPILER_COMMAND_FILE) $(CVARSFLAG) \
	 -D_CRT_SECURE_NO_WARNINGS $(DBG_FLAG) $(CC_OPTS) $(CPP_REQ_DEFINES) \
	 $(USER_INCLUDES)

CPPFLAGS = $(cflags) @$(COMPILER_COMMAND_FILE) $(CVARSFLAG) \
	   -EHs -D_CRT_SECURE_NO_WARNINGS $(DBG_FLAG) $(CPP_OPTS) $(CC_OPTS) \
	   $(CPP_REQ_DEFINES) $(USER_INCLUDES)

LDFLAGS = $(ldebug) $(conflags) $(conlibs) $(ADDITIONAL_LDFLAGS)

#----------------------------- Source Files -----------------------------------

ADD_SRCS =

REQ_SRCS = $(ADD_SRCS) $(MODULES) $(USER_SRCS)

!if "$(MODELREF_TARGET_TYPE)" == "NONE"
!if $(SHRLIBTARGET) == 1
PRODUCT   = $(RELATIVE_PATH_TO_ANCHOR)\$(MODEL)_win64.dll
!else
!if $(MAKEFILEBUILDER_TGT) == 1
PRODUCT   = $(MODEL).exe
PREBUILT_SRCS  = $(MODULES)
PREBUILT_OBJS_CPP_UPPER = $(PREBUILT_SRCS:.CPP=.obj)
PREBUILT_OBJS_CPP_LOWER = $(PREBUILT_OBJS_CPP_UPPER:.cpp=.obj)
PREBUILT_OBJS_C_UPPER = $(PREBUILT_OBJS_CPP_LOWER:.C=.obj)
PREBUILT_OBJS = $(PREBUILT_OBJS_C_UPPER:.c=.obj)
!else
!if $(STANDALONE_SUPPRESS_EXE) == 1
#  Build object code only for top level model
PRODUCT            = "ObjectModulesOnly"
!else
PRODUCT   = $(RELATIVE_PATH_TO_ANCHOR)\$(MODEL).exe
!endif
!endif
!endif
!else
# Generate a library
PRODUCT   = $(MODELLIB)
!endif

!if "$(MODELREF_TARGET_TYPE)" == "SIM"
SRCS = $(REQ_SRCS)
!else
SRCS = $(REQ_SRCS) $(S_FUNCTIONS)
!endif


USER_SRCS =


OBJS_CPP_UPPER = $(SRCS:.CPP=.obj)
OBJS_CPP_LOWER = $(OBJS_CPP_UPPER:.cpp=.obj)
OBJS_C_UPPER = $(OBJS_CPP_LOWER:.C=.obj)
OBJS = $(OBJS_C_UPPER:.c=.obj)
PROT_CAPIC  = $(MODEL)$(PROT_CAPIC_SUFFIX)
PROT_CAPIO  = $(MODEL)$(PROT_CAPIO_SUFFIX)
SHARED_OBJS = $(SHARED_BIN_DIR)\*.obj

#-------------------------- Additional Libraries -------------------------------

LIBS = 


LIBS = $(LIBS) $(S_FUNCTIONS_LIB)

#--------------------------------- Rules --------------------------------------
!if "$(ISPROTECTINGMODEL)" == "PROTECTING"
!if "$(MODELREF_TARGET_TYPE)" == "SIM"
all: set_environment_variables $(PRODUCT) $(PROT_CAPIO)
!else
all: set_environment_variables $(PRODUCT)
!endif
!else
all: set_environment_variables $(PRODUCT)
!endif


!if "$(MODELREF_TARGET_TYPE)" == "NONE"
#--- Shared library target (.dll) ---
!if $(SHRLIBTARGET)==1
$(PRODUCT) : $(LIBS) $(OBJS) $(SHARED_LIB) $(LIBS) $(MODELREF_LINK_LIBS)
	@cmd /C "echo ### Linking ..."
	$(LD) $(LDFLAGS) $(SHARED_LIB) $(LIBS) \
    @$(CMD_FILE) @$(MODELREF_LINK_RSPFILE) -dll -def:$(MODEL).def -out:$@
#--- Comment out the next line to retain .lib and .exp files ---
	@del $(RELATIVE_PATH_TO_ANCHOR)\$(MODEL)_win64.lib $(RELATIVE_PATH_TO_ANCHOR)\$(MODEL)_win64.exp
	@cmd /C "echo $(BUILD_SUCCESS) dynamically linked library  $(MODEL)_win64.dll"
!else
!if $(MAKEFILEBUILDER_TGT)==1
$(PRODUCT) : $(LIBS) $(PREBUILT_OBJS) $(OBJS) $(MODELLIB) $(SHARED_LIB) $(LIBS) $(MODELREF_LINK_LIBS)
	@cmd /C "echo ### Linking ..."

	@cmd /C "echo $(BUILD_SUCCESS) executable $(MODEL).exe"
!else
!if $(STANDALONE_SUPPRESS_EXE)==1
.PHONY: $(PRODUCT)
$(PRODUCT) : $(LIBS) $(OBJS) $(SHARED_LIB) $(LIBS)
	@cmd /C "echo $(BUILD_SUCCESS) executable $(PRODUCT)"
!else
#--- Stand-alone model (.exe) ---
$(PRODUCT) : $(LIBS) $(OBJS) $(SHARED_LIB) $(LIBS) $(MODELREF_LINK_LIBS)
	@cmd /C "echo ### Linking ..."
	$(LD) $(LDFLAGS) $(SHARED_LIB) $(LIBS) @$(CMD_FILE) @$(MODELREF_LINK_RSPFILE) $(SYSTEM_LIBS) -out:$@
	@cmd /C "echo $(BUILD_SUCCESS) executable $(MODEL).exe"
!endif
!endif
!endif
!else
#--- Library target ---
$(PRODUCT) : $(LIBS) $(OBJS) $(SHARED_LIB)
	@cmd /C "echo ### Linking ..."


!if "$(ISPROTECTINGMODEL)" == "PROTECTING"
$(PROT_CAPIO) : $(PROT_CAPIC)
	$(CC) $(CFLAGS) -DHOST_CAPI_BUILD $(PROT_CAPIC) /Fo$(PROT_CAPIO)
!endif
!endif	


#-------------------------- Support for building modules ----------------------


!if $(GEN_SAMPLE_MAIN) == 0
!if "$(TARGET_LANG_EXT)" ==  "cpp"
{$(MATLAB_ROOT)\rtw\c\src\common}.cpp.obj :
	@cmd /C "echo ### Compiling $<"
	$(CC) /TP $(CPPFLAGS) $<

{$(MATLAB_ROOT)\rtw\c\src\common}.c.obj :
	@cmd /C "echo ### Compiling $<"
	$(CC) /TP $(CPPFLAGS) $<
!else
{$(MATLAB_ROOT)\rtw\c\src\common}.c.obj :
	@cmd /C "echo ### Compiling $<"
	$(CC) $(CFLAGS) $<
!endif
!endif


{$(MATLAB_ROOT)\rtw\c\src}.c.obj :
	@cmd /C "echo ### Compiling $<"
	$(CC) $(CFLAGS) $<

# Additional sources

{$(MATLAB_ROOT)\toolbox\physmod\powersys\powersys}.c.obj :
	@cmd /C "echo ### Compiling $<"
	$(CC) $(CFLAGS) $<

{$(MATLAB_ROOT)\rtw\c\src}.c.obj :
	@cmd /C "echo ### Compiling $<"
	$(CC) $(CFLAGS) $<

{$(MATLAB_ROOT)\simulink\src}.c.obj :
	@cmd /C "echo ### Compiling $<"
	$(CC) $(CFLAGS) $<



{$(MATLAB_ROOT)\toolbox\physmod\powersys\powersys}.cpp.obj :
	@cmd /C "echo ### Compiling $<"
	$(CC) $(CPPFLAGS) $<

{$(MATLAB_ROOT)\rtw\c\src}.cpp.obj :
	@cmd /C "echo ### Compiling $<"
	$(CC) $(CPPFLAGS) $<

{$(MATLAB_ROOT)\simulink\src}.cpp.obj :
	@cmd /C "echo ### Compiling $<"
	$(CC) $(CPPFLAGS) $<




# Put these rules last, otherwise nmake will check toolboxes first

{$(RELATIVE_PATH_TO_ANCHOR)}.c.obj :
	@cmd /C "echo ### Compiling $<"
	$(CC) $(CFLAGS) $<

{$(RELATIVE_PATH_TO_ANCHOR)}.cpp.obj :
	@cmd /C "echo ### Compiling $<"
	$(CC) $(CPPFLAGS) $<

.c.obj :
	@cmd /C "echo ### Compiling $<"
	$(CC) $(CFLAGS) $<

.cpp.obj :
	@cmd /C "echo ### Compiling $<"
	$(CC) $(CPPFLAGS) $<

!if "$(SHARED_LIB)" != ""
$(SHARED_LIB) : $(SHARED_SRC)
	@cmd /C "echo ### Creating $@"
	@$(CC) $(CFLAGS) -Fo$(SHARED_BIN_DIR)\ @<<
$?
<<
	@$(LIBCMD) /nologo /out:$@ $(SHARED_OBJS)
	@cmd /C "echo ### $@ Created"
!endif

set_environment_variables:
	@set INCLUDE=$(INCLUDE)
	@set LIB=$(LIB)

# Libraries:






#----------------------------- Dependencies -----------------------------------


