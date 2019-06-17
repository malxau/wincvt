

!IF "$(MINICRT)"!="1"
MINICRT=0
!ENDIF
DEBUG=0
BUILD=$(MAKE) -nologo MINICRT=$(MINICRT) DEBUG=$(DEBUG)

all:
	@if not exist bin mkdir bin
	@cd lib
	@echo Building all in lib...
	@$(BUILD)
	@cd ..\tools
	@echo Building all in tools...
	@$(BUILD)
	@cd ..\help
	@echo Building all in help...
	@$(BUILD)
	@cd ..

clean:
	@cd lib
	@echo Cleaning in lib...
	@$(BUILD) clean
	@cd ..\tools
	@echo Cleaning in tools...
	@$(BUILD) clean
	@cd ..\help
	@echo Cleaning in help...
	@$(BUILD) clean
	@cd ..
	@if exist bin rd /s/q bin
	@if exist *~ erase *~

help:
	@rem echo "DEBUG=[0|1] - If set, will compile debug support"
	@rem echo.
	@rem echo "MINICRT=[0|1] - If set, will use minicrt rather than msvcrt"
	@rem echo.


SHIPPDB=
!IF $(DEBUG)>0
SHIPPDB=/DSHIPPDB
!ENDIF

distribution: all
	@echo Making self-installer...
	@makensis /V2 $(SHIPPDB) wincvt.nsi
	@cd bin
	@yori -c ypm -c wincvt-win32.cab wincvt 0.4.1 win32 -filelist ..\wincvt.lst -minimumosbuild 1381 -upgradepath http://www.malsmith.net/download/?obj=wincvt/latest-stable/wincvt-win32.cab -sourcepath http://www.malsmith.net/download/?obj=wincvt/latest-stable/wincvt-source.cab
	@cd ..
	@yori -c ypm -cs bin\wincvt-source.cab wincvt-source 0.4.1 -filepath .
	@if exist bin\wincvt-win32-installer* echo Distribution built successfully!
