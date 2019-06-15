

DEBUG=0
UNICODE=0
MSVCRT_DLL=1

LINK=link.exe -nologo -incremental:no
MAKE=nmake.exe -nologo

CFLAGS=-nologo -W3 -Gs9999 -I.
LDFLAGS=-fixed
LD_CONSOLEAPP=
LD_GUIAPP=
LIBS=kernel32.lib

!IF $(DEBUG)==1
CFLAGS=$(CFLAGS) -Zi -Od -DDEBUG_WINCVT -DDEBUG_WINCVT_APP
LDFLAGS=$(LDFLAGS) -DEBUG
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
CFLAGS=$(CFLAGS) -GS-
CFLAGS=$(CFLAGS) -GF
!IF [$(CC) -? 2>&1 | find "/MP" >NUL]==0
CFLAGS=$(CFLAGS) -MP
!ENDIF
!ELSE
!IF [$(CC) -GF 2>&1 | find "unknown" >NUL]>0
CFLAGS=$(CFLAGS) -GF
!ELSE
CFLAGS=$(CFLAGS) -Gf
!ENDIF
!ENDIF

#
# Visual C 2003 really wants this and isn't satisfied
# with a pragma, so yet another probe
#

!IF [$(LINK) -OPT:NOWIN98 2>&1 | find "NOWIN98" >NUL]>0
LDFLAGS=$(LDFLAGS) -OPT:NOWIN98
!ENDIF

!IF "$(MINICRT)"=="1"

#
# Try to detect which architecture the compiler is generating.  This is done
# to help the subsystem version detection, below.  If we don't guess it's
# not fatal, the linker will figure it out in the end.
#

!IF [$(CC) 2>&1 | find "for x86" >NUL]==0
LDFLAGS=$(LDFLAGS) -MACHINE:X86
!ELSE
!IF [$(CC) 2>&1 | find "for x64" >NUL]==0
LDFLAGS=$(LDFLAGS) -MACHINE:X64
!ENDIF
!ENDIF

#
# Look for the oldest subsystem version the linker is willing to generate
# for us.  This app with minicrt will run back to NT 3.1, but requires OS
# support added in 4.0, but typically the primary limit is the linker.
# Without minicrt we're at the mercy of whatever CRT is there, so just
# trust the defaults.  For execution efficiency, these checks are
# structured as a tree:
#
#                             5.0?
#                           /      \
#                         /          \
#                       /              \
#                   4.0?                5.2?
#                 /      \            /      \
#              4.0         5.0    5.1?         6.0?
#                                /    \       /    \
#                              5.1    5.2   6.0   default
#

!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,5.0 2>&1 | find "default subsystem" >NUL]>0
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,4.0 2>&1 | find "default subsystem" >NUL]>0
LD_CONSOLEAPP=-SUBSYSTEM:CONSOLE,4.0
LD_GUIAPP=-SUBSYSTEM:WINDOWS,4.0
!ELSE  # !4.0
LD_CONSOLEAPP=-SUBSYSTEM:CONSOLE,5.0
LD_GUIAPP=-SUBSYSTEM:WINDOWS,5.0
!ENDIF # 4.0
!ELSE  # !5.0
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,5.2 2>&1 | find "default subsystem" >NUL]>0
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,5.1 2>&1 | find "default subsystem" >NUL]>0
LD_CONSOLEAPP=-SUBSYSTEM:CONSOLE,5.1
LD_GUIAPP=-SUBSYSTEM:WINDOWS,5.1
!ELSE  # !5.1
LD_CONSOLEAPP=-SUBSYSTEM:CONSOLE,5.2
LD_GUIAPP=-SUBSYSTEM:WINDOWS,5.2
!ENDIF # 5.1
!ELSE  # !5.2
!IF [$(LINK) $(LDFLAGS) -SUBSYSTEM:CONSOLE,6.0 2>&1 | find "default subsystem" >NUL]>0
LD_CONSOLEAPP=-SUBSYSTEM:CONSOLE
LD_GUIAPP=-SUBSYSTEM:WINDOWS
!ENDIF # 6.0
!ENDIF # 5.2
!ENDIF # 5.0
!ELSE  # MINICRT
LD_CONSOLEAPP=-SUBSYSTEM:CONSOLE
LD_GUIAPP=-SUBSYSTEM:WINDOWS
!ENDIF


#
# We could also probe for -incremental:no,
# although they typically are only warnings and
# probes are expensive, so we don't.
#

GUI_TARGETS= \
	..\bin\cvtopen.exe \
	..\bin\cvtview.exe \
	..\bin\cvtwiz.exe

CONS_TARGETS= \
	..\bin\cvtfile.exe \
	..\bin\cvtinst.exe \
	..\bin\cvtquery.exe \
	..\bin\cvttype.exe \
	..\bin\cvtver.exe

TARGETS=$(CONS_TARGETS) $(GUI_TARGETS)

OBJS=ver.obj

MYLIBS =..\bin\wincvt.lib

all: $(TARGETS)

clean: 
	@if exist *.obj erase *.obj
	@if exist *.res erase *.res
	@if exist *.exe erase *.exe
	@if exist *.dll erase *.dll
	@if exist *.exp erase *.exp
	@if exist *.lib erase *.lib
	@if exist *.aps erase *.aps
	@if exist *.pch erase *.pch
	@if exist *.pdb erase *.pdb
	@if exist *.ilk erase *.ilk
	@if exist *.map erase *.map
	@if exist *~ erase *~
	@for %i in ($(TARGETS)) do if exist %i erase %i
	@if exist ..\bin\*.ilk erase ..\bin\*.ilk

.SUFFIXES: .cpp .rc .res .obj .exe

!IFDEF _NMAKE_VER
.c.obj::
!ELSE
.c.obj:
!ENDIF
	@$(CC) $(CFLAGS) -I..\include -DWIN32_NO_CONFIG_H -c $<

.rc.obj:
	@echo $(**F)
	@if exist $@ erase $@
	@$(RC) -DMANIFEST /fo$(@B).res $(RCLAGS) $**
	@if not exist $@ ren $(@B).res $@

# Yes, this sucks.  It can't find ver.rc since that's in a different
# directory.
ver.obj: ..\include\ver.rc
	@echo $(**F)
	@if exist $@ erase $@
	@$(RC) /fo$(@B).res $(RCLAGS) $**
	@if not exist $@ ren $(@B).res $@

$(CONS_TARGETS): $(@B).obj $(OBJS)
	@echo $(@F)
	@$(LINK) $(LDFLAGS) $(LD_CONSOLEAPP) $** /out:$@ $(LIBS) $(CRTLIB) $(MYLIBS) 

..\bin\cvtopen.exe: $(@B).obj
	@echo $(@F)
	@$(LINK) $(LDFLAGS) $(LD_CONSOLEAPP) $** /out:$@ $(LIBS) $(CRTLIB) $(MYLIBS) shell32.lib

..\bin\cvtwiz.exe: $(@B).obj wizdlg.obj
	@echo $(@F)
	@$(LINK) $(LDFLAGS) $(LD_GUIAPP) $** /out:$@ $(LIBS) $(CRTLIB) $(MYLIBS) comctl32.lib comdlg32.lib user32.lib

..\bin\cvtview.exe: $(@B).obj viewdlg.obj
	@echo $(@F)
	@$(LINK) $(LDFLAGS) $(LD_GUIAPP) $** /out:$@ $(LIBS) $(CRTLIB) $(MYLIBS) comctl32.lib comdlg32.lib user32.lib gdi32.lib


