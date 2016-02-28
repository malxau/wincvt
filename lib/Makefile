

all: wincvt.dll

DEBUG=0
UNICODE=0
MSVCRT_DLL=1

LINK=link.exe -nologo -incremental:no
MAKE=nmake.exe -nologo

CFLAGS=-nologo -W3 -Gs9999 -I.
LDFLAGS=-fixed -subsystem:console
LIBS=kernel32.lib advapi32.lib

!IF $(DEBUG)==1
CFLAGS=$(CFLAGS) -Zi -Od -DDEBUG_WINCVT -DDEBUG_WINCVT_APP
LDFLAGS=$(LDFLAGS) -DEBUG
LIBS=$(LIBS) user32.lib
!ELSE
CFLAGS=$(CFLAGS) -Gy -O1syi
!ENDIF

#
# Set the correct entrypoint depending on whether we're
# ANSI or Unicode.
#

!IF $(UNICODE)==1
CFLAGS=$(CFLAGS) -DUNICODE -D_UNICODE
!ENDIF

#
# Include and link to the desired CRT.
#

!IF "$(MINICRT)"=="1"
CFLAGS=$(CFLAGS) -DMINICRT
LDFLAGS=$(LDFLAGS) -nodefaultlib
CRTLIB=minicrt.lib
!ELSE
!IF $(MSVCRT_DLL)==1
CFLAGS=$(CFLAGS) -MD
CRTLIB=msvcrt.lib
!ELSE
CFLAGS=$(CFLAGS) -MT
CRTLIB=libcmt.lib
!ENDIF
!ENDIF

#
# Probe for more esoteric aspects of compiler behavior.
# Apparently pragma doesn't cut it for -GS-, and this is
# the most critical one for Minicrt.  MP is a perf
# optimization that only exists on newer compilers, so
# skip the probe on old ones.
#

!IF [$(CC) -GS- 2>&1 | find "unknown" >NUL]>0
CFLAGS_GS=-GS-
!IF [$(CC) -? 2>&1 | find "/MP" >NUL]==0
CFLAGS_MP=-MP
!ENDIF
!ENDIF
CFLAGS_GF=-GF

#
# Visual C 2003 really wants this and isn't satisfied
# with a pragma, so yet another probe
#

!IF [$(LINK) -OPT:NOWIN98 2>&1 | find "NOWIN98" >NUL]>0
LDFLAGS=$(LDFLAGS) -OPT:NOWIN98
!ENDIF

#
# We could also probe for -incremental:no,
# although they typically are only warnings and
# probes are expensive, so we don't.
#

CFLAGS=$(CFLAGS) $(CFLAGS_GS) $(CFLAGS_MP) $(CFLAGS_GF)
DEPS=ver.obj

ver.obj: ..\include\ver.rc
	@if exist ver.obj erase ver.obj
	@$(RC) /d_DLL=1 /fover.res $(RCFLAGS) ..\include\ver.rc
	@if not exist ver.obj ren ver.res ver.obj

OBJS=capabil.obj context.obj convert.obj debug.obj inst.obj registry.obj support.obj 

wincvt.dll: $(DEPS) $(OBJS) 
	@$(LINK) /DLL /DEF:wincvt.def $(LDFLAGS) $(OBJS) $(LIBS) $(CRTLIB) /out:..\bin\wincvt.dll


!IFDEF _NMAKE_VER
.c.obj::
!ELSE
.c.obj:
!ENDIF
	@$(CC) $(CFLAGS) -I..\include -c $<

clean:
	@if exist *.dll erase *.dll
	@if exist *.obj erase *.obj
	@if exist *.pdb erase *.pdb
	@if exist *.lib erase *.lib
	@if exist *.exp erase *.exp
	@if exist *~ erase *~
	@if exist *.manifest erase *.manifest

