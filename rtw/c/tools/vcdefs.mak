
APPVER = 5.02
NMAKE_WINVER = 0x0502
_WIN32_IE = 0x0600


!IF "$(PROCESSOR_ARCHITECTURE)" == ""
CPU=i386
PROCESSOR_ARCHITECTURE=x86
!endif

!IF !DEFINED(CPU) || "$(CPU)" == ""
CPU = $(PROCESSOR_ARCHITECTURE)
!ENDIF # CPU

!IF ( "$(CPU)" == "X86" ) || ( "$(CPU)" == "x86" )
CPU = i386
!ENDIF # CPU == X86


# cdebug ----------------------------------------------------------------------
!IFDEF NODEBUG
cdebug = -Ox -DNDEBUG 
!ELSE
cdebug = -Zi -Od -DDEBUG
!ENDIF

# cflags ----------------------------------------------------------------------
ccommon = -c -DCRTAPI1=_cdecl -DCRTAPI2=_cdecl -nologo -GS

!IF "$(CPU)" == "i386"
cflags = $(ccommon) -D_X86_=1 -DWIN32 -D_WIN32 -W3
!ELSEIF "$(CPU)" == "AMD64"
cflags = $(ccommon) -D_AMD64_=1 -DWIN64 -D_WIN64 -DWIN32 -D_WIN32 -W4
!ENDIF

winntdefs =  -D_WINNT -D_WIN32_WINNT=$(NMAKE_WINVER) -DNTDDI_VERSION=$(NMAKE_WINVER)0000
cflags = $(cflags) $(winntdefs) -D_WIN32_IE=$(_WIN32_IE) -DWINVER=$(NMAKE_WINVER)

# cvars cvarsmt cvarsdll ------------------------------------------------------
# for Windows applications that use the C Run-Time libraries
!IFDEF NODEBUG
cvarsmt    = -D_MT -MT
cvars      = $(cvarsmt)
cvarsdll   = -D_MT -D_DLL -MD
!ELSE
cvarsmt    = -D_MT -MTd
cvars      = $(cvarsmt)
cvarsdll   = -D_MT -D_DLL -MDd
!ENDIF

# ldebug ----------------------------------------------------------------------
!IFDEF NODEBUG
ldebug = /RELEASE
!ELSE
ldebug = /DEBUG /DEBUGTYPE:cv
!ENDIF


# conflags( same as conlflags) ------------------------------------------------
lflags  = $(lflags) /INCREMENTAL:NO /NOLOGO
conflags = $(lflags) -subsystem:console,$(APPVER)

# conlibs, conlibsmt, conlibsdll ----------------------------------------------
winsocklibs = ws2_32.lib mswsock.lib
baselibs    = kernel32.lib $(winsocklibs) advapi32.lib

# these values are always the same
conlibs     = $(baselibs)
conlibsmt   = $(conlibs)
conlibsdll  = $(conlibs)
