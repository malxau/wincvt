!include "Sections.nsh"

; The name of the installer
Name "WinCvt"
SetCompressor LZMA
RequestExecutionLevel user

!define /date BUILDDATE "%Y%m%d"

!IFDEF SHIPPDB
!define DEBUGTAG "-debug"
!ELSE
!define DEBUGTAG ""
!ENDIF

; The file to write
OutFile "bin\wincvt-win32-installer${DEBUGTAG}-${BUILDDATE}.exe"

; The default installation directory
InstallDir $PROGRAMFILES\WinCvt

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\WinCvt" "Install_Dir"

;--------------------------------

; Pages

Page license
Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

LicenseText "This software is licensed under the GPL, which attaches some conditions to modification and distribution (but not to use.)  Please read the full text for these conditions."
LicenseData "COPYING"

;--------------------------------

; The stuff to install
Section "WinCvt (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "bin\wincvt.dll"
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\WinCvt  "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WinCvt" "DisplayName" "WinCvt"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WinCvt" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WinCvt" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WinCvt" "NoRepair" 1
  WriteUninstaller "uninstall.exe"

  CreateDirectory "$SMPROGRAMS\WinCvt"
  CreateShortCut "$SMPROGRAMS\WinCvt\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

Section "Converter wizard"
  SetOutPath "$INSTDIR"
  File "bin\cvtwiz.exe"
  CreateShortCut "$SMPROGRAMS\WinCvt\Converter Wizard.lnk" "$INSTDIR\cvtwiz.exe" "" "$INSTDIR\cvtwiz.exe" 0
  CreateShortCut "$SENDTO\Change Format (WinCvt).lnk" "$INSTDIR\cvtwiz.exe" "" "$INSTDIR\cvtwiz.exe" 0
  WriteRegStr HKCR "Applications\cvtwiz.exe\shell\edit\command" "" '$INSTDIR\cvtwiz.exe "%1"'
  WriteRegStr HKCR "*\OpenWithList\cvtwiz.exe" "" ""
SectionEnd

Section "Document viewer"
  SetOutPath "$INSTDIR"
  File "bin\cvtview.exe"
  CreateShortCut "$SMPROGRAMS\WinCvt\Document Viewer.lnk" "$INSTDIR\cvtview.exe" "" "$INSTDIR\cvtview.exe" 0
SectionEnd

Section "Console tools"
  SetOutPath "$INSTDIR"
  File "bin\cvtfile.exe"
  File "bin\cvtinst.exe"
  File "bin\cvtopen.exe"
  File "bin\cvtquery.exe"
  File "bin\cvttype.exe"
  File "bin\cvtver.exe"
SectionEnd

Section "Documentation"
  SetOutPath $INSTDIR
  File "help\wincvt.chm"
  CreateShortCut "$SMPROGRAMS\WinCvt\Documentation.lnk" "$INSTDIR\wincvt.chm" "" "$INSTDIR\wincvt.chm" 0
SectionEnd

Section "Developer Support"
  SetOutPath $INSTDIR
  File "bin\wincvt.lib"
  File "include\wincvt.h"
SectionEnd

!IFDEF SHIPPDB
Section "Debugging Support"
  SetOutPath $INSTDIR
  File "bin\*.pdb"
SectionEnd
!ENDIF

;--------------------------------

; Uninstaller

Section "Uninstall"
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WinCvt"
  DeleteRegKey HKLM SOFTWARE\WinCvt
  DeleteRegKey HKCR "Applications\cvtwiz.exe" 
  DeleteRegKey HKCR "*\OpenWithList\cvtwiz.exe"

  ; Remove files and uninstaller
  Delete $INSTDIR\wincvt.dll
  Delete $INSTDIR\wincvt.h
  Delete $INSTDIR\wincvt.lib
  Delete $INSTDIR\wincvt.chm
  Delete $INSTDIR\cvtfile.exe
  Delete $INSTDIR\cvtinst.exe
  Delete $INSTDIR\cvtopen.exe
  Delete $INSTDIR\cvtquery.exe
  Delete $INSTDIR\cvttype.exe
  Delete $INSTDIR\cvtver.exe
  Delete $INSTDIR\cvtview.exe
  Delete $INSTDIR\cvtwiz.exe
  Delete $INSTDIR\uninstall.exe
  Delete "$INSTDIR\*.pdb"

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\WinCvt\*.*"
  Delete "$SENDTO\Change Format (WinCvt).lnk"

  ; Remove directories used
  RMDir "$SMPROGRAMS\WinCvt"
  RMDir "$INSTDIR"

SectionEnd

