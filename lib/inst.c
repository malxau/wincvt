/* WINCVT
 * =======
 * This software is Copyright (c) 2006-07 Malcolm Smith.
 * No warranty is provided, including but not limited to
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * This code is licenced subject to the GNU General
 * Public Licence (GPL).  See the COPYING file for more.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#include "wincvt.h"
#include "wcassert.h"

#include "common.h"


/*
 * INSTALLATION FUNCTIONS
 *
 * These functions are used to install converters.
 */
WINCVT_STATUS
WinCvtInstallConverter(
  LPCSTR szConverterName,
  BOOL bUserInstall)
{
	WINCVT_STATUS SavedError, TempError;
	NONOPAQUE_CVT_CONTEXT Context;
	HKEY ConverterKey = NULL;
	HKEY ClassKey = NULL;
	HKEY ImpExpKey = NULL;
	DWORD Disp;
	LONG Err;
	PNONOPAQUE_CVT_LIST Output = NULL;
	PNONOPAQUE_CVT_CLASS Class;
	DWORD Pass = 0;
	LPSTR szExpandedConverterName = NULL;
	LPSTR szFilePart;
	DWORD nExpandedConverterNameLength = 0;

	BOOL ConverterInitialized = FALSE;
	memset(&Context, 0, sizeof(Context));

	APP_ASSERT(szConverterName != NULL);
	if (szConverterName == NULL) {
		return WINCVT_ERROR_BAD_PARAM_1;
	}

	Err = RegCreateKeyEx(
		bUserInstall?HKEY_CURRENT_USER:HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Shared Tools",
		0,
		NULL,
		0,
		KEY_WRITE,
		NULL,
		&ConverterKey,
		&Disp); 

	if (Err != ERROR_SUCCESS) {
		return WINCVT_ERROR_REGISTRY;
	}

	//
	//  We need to expand the converter name specified to give it
	//  a full path.  This call is to find how long that path will
	//  be.
	//
	nExpandedConverterNameLength = GetFullPathName( szConverterName,
		0,
		NULL,
		NULL);

	if (nExpandedConverterNameLength == 0) {
		//
		//  We can't find how long the path will be.  Darn.
		//
		ASSERT( nExpandedConverterNameLength != 0 );
		RegCloseKey(ConverterKey);
		return WINCVT_ERROR_BAD_PATH;
	}

	szExpandedConverterName = WinCvtMalloc(nExpandedConverterNameLength + 1);
	if (szExpandedConverterName == NULL) {
		RegCloseKey(ConverterKey);
		return WINCVT_ERROR_NO_MEMORY;
	}

	Disp = GetFullPathName( szConverterName,
		nExpandedConverterNameLength,
		szExpandedConverterName,
		&szFilePart);

	if ((Disp >= nExpandedConverterNameLength) || (Disp == 0)) {
		//
		//  The return value is chars copied without the NULL or
		//  length required.  Thus, greater than or equal is the
		//  failure case.  This should never happen unless the
		//  current directory has changed out from underneath us.
		//
		ASSERT( FALSE );
		RegCloseKey(ConverterKey);
		WinCvtFree( szExpandedConverterName );
		return WINCVT_ERROR_BAD_PATH;
	}

	SavedError = WinCvtIntInitializeConverter(&Context,
		szExpandedConverterName );
	if (SavedError != WINCVT_SUCCESS) {
		RegCloseKey(ConverterKey);
		WinCvtFree( szExpandedConverterName );
		return SavedError;
	}

	ConverterInitialized = TRUE;

	if (Context.RegisterConverter != NULL) {
		// If the converter can do this itself, let it.
		SavedError = Context.RegisterConverter(ConverterKey);
		if (SavedError == 1) {
			//
			// This function returns 1 for success.
			//
			SavedError = WINCVT_SUCCESS;
			goto abort;
		}
		// We didn't succeed the easy eay, so let's try
		// ourselves.
	}
	// Okay, we do things the hard way.

	// Close and reopen at different level
	RegCloseKey(ConverterKey);
	ConverterKey = NULL;

	Err = RegCreateKeyEx(
		bUserInstall?HKEY_CURRENT_USER:HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Shared Tools\\Text Converters",
		0,
		NULL,
	   	0,
	   	KEY_WRITE,
		NULL,
		&ConverterKey,
		&Disp); 

	if (Err != ERROR_SUCCESS) {
		SavedError = WINCVT_ERROR_REGISTRY;
		goto abort;
	}

	while (Pass < 2) {
		SavedError = WinCvtIntGetConverterCapabilityList(&Context,
			&Output,
			Pass);
		if (SavedError != WINCVT_SUCCESS) {
			goto abort;
		}

		Class = WinCvtGetFirstClass(Output);
		Err = RegCreateKeyEx(ConverterKey,
				Pass?"Export":"Import",
				0,
				NULL,
				0,
				KEY_ALL_ACCESS,
				NULL,
				&ImpExpKey,
				&Disp);

		if (Err != ERROR_SUCCESS) {
			SavedError = WINCVT_ERROR_REGISTRY;
			goto abort;
		}
		while(Class) {
			Err = RegCreateKeyEx(ImpExpKey,
					Class->szClassName,
					0,
					NULL,
					0,
					KEY_ALL_ACCESS,
					NULL,
					&ClassKey,
					&Disp);

			if (Err != ERROR_SUCCESS) {
				SavedError = WINCVT_ERROR_REGISTRY;
				goto abort;
			}

			//
			// FIXME? - Don't handle strings >4Gb
			//
			Err = RegSetValueEx(ClassKey,
					"Extensions",
					0,
					REG_SZ,
					Class->szExtensions,
					(DWORD)strlen(Class->szExtensions));

			if (Err != ERROR_SUCCESS) {
				SavedError = WINCVT_ERROR_REGISTRY;
				goto abort;
			}

			//
			// FIXME? - Don't handle strings >4Gb
			//
			Err = RegSetValueEx(ClassKey,
					"Name",
					0,
					REG_SZ,
					Class->szDescription,
					(DWORD)strlen(Class->szDescription));

			if (Err != ERROR_SUCCESS) {
				SavedError = WINCVT_ERROR_REGISTRY;
				goto abort;
			}

			//
			// FIXME? - Don't handle strings >4Gb
			//
			Err = RegSetValueEx(ClassKey,
					"Path",
					0,
					REG_SZ,
					Class->szFileName,
					(DWORD)strlen(Class->szFileName));

			if (Err != ERROR_SUCCESS) {
				SavedError = WINCVT_ERROR_REGISTRY;
				goto abort;
			}
			RegCloseKey(ClassKey);
			ClassKey = NULL;

			Class = WinCvtGetNextClass(Class);
		}
		RegCloseKey(ImpExpKey);
		ImpExpKey = NULL;
		if (Output) {
			TempError = WinCvtFreeConverterList(Output);
			ASSERT(TempError == WINCVT_SUCCESS);
		}
		Output = NULL;
		Pass++;
	}

abort:
	if (szExpandedConverterName) {
		WinCvtFree( szExpandedConverterName );
	}
	if (Output) {
		TempError = WinCvtFreeConverterList(Output);
		ASSERT(TempError == WINCVT_SUCCESS);
	}
	if (ConverterInitialized) {
		WinCvtIntUninitializeConverter(&Context);
		ConverterInitialized = FALSE;
	}
	if (ClassKey) {
		RegCloseKey(ClassKey);
	}
	if (ImpExpKey) {
		RegCloseKey(ImpExpKey);
	}
	if (ConverterKey) {
		RegCloseKey(ConverterKey);
	}
	return SavedError;
}

WINCVT_STATUS
WinCvtIntDeleteKey(
  HKEY hParentKey,
  LPCSTR szChildKey)
{
	LONG Err;
	HKEY hChildKey = NULL;
	CHAR szName[256];
	DWORD dwNameSize = sizeof(szName);
	FILETIME LastWrite;

	Err = RegOpenKeyEx(
		hParentKey,
		szChildKey,
	   	0,
	   	KEY_ALL_ACCESS,
		&hChildKey); 

	if (Err != ERROR_SUCCESS) {
		return WINCVT_ERROR_REGISTRY;
	}

	Err = RegEnumKeyEx(
		hChildKey,
		0,
		szName,
		&dwNameSize,
		NULL,
		NULL,
		NULL,
		&LastWrite);

	while (Err == ERROR_SUCCESS) {
		Err = RegDeleteKey( hChildKey,
			szName);

		if (Err != ERROR_SUCCESS) {
			RegCloseKey(hChildKey);
			return WINCVT_ERROR_REGISTRY;
		}

		Err = RegEnumKeyEx(
			hChildKey,
			0,
			szName,
			&dwNameSize,
			NULL,
			NULL,
			NULL,
			&LastWrite);
	}

	RegCloseKey(hChildKey);

	Err = RegDeleteKey(hParentKey, szChildKey);
	if (Err != ERROR_SUCCESS) {
		return WINCVT_ERROR_REGISTRY;
	}
	return WINCVT_SUCCESS;
}

WINCVT_STATUS
WinCvtUninstallConverter(
  LPCSTR szConverterName,
  BOOL bUserInstall)
{
	WINCVT_STATUS SavedError, TempError;
	NONOPAQUE_CVT_CONTEXT Context;
	HKEY ConverterKey = NULL;
	HKEY ImpExpKey = NULL;
	LONG Err;
	PNONOPAQUE_CVT_LIST Output = NULL;
	PNONOPAQUE_CVT_CLASS Class;
	DWORD Pass = 0;

	BOOL ConverterInitialized = FALSE;
	memset(&Context, 0, sizeof(Context));

	APP_ASSERT(szConverterName != NULL);
	if (szConverterName == NULL) {
		return WINCVT_ERROR_BAD_PARAM_1;
	}

	SavedError = WinCvtIntInitializeConverter(&Context, szConverterName);
	if (SavedError != WINCVT_SUCCESS) {
		RegCloseKey(ConverterKey);
		return SavedError;
	}

	ConverterInitialized = TRUE;

	// We have to do things the hard way for uninstall, since converters
	// don't provide that functionality.

	Err = RegOpenKeyEx(
		bUserInstall?HKEY_CURRENT_USER:HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Shared Tools\\Text Converters",
	   	0,
	   	KEY_WRITE,
		&ConverterKey); 

	if (Err != ERROR_SUCCESS) {
		SavedError = WINCVT_ERROR_REGISTRY;
		goto abort;
	}

	while (Pass < 2) {
		SavedError = WinCvtIntGetConverterCapabilityList(&Context,
			&Output,
			Pass);
		if (SavedError != WINCVT_SUCCESS) {
			goto abort;
		}

		Class = WinCvtGetFirstClass(Output);
		Err = RegOpenKeyEx(ConverterKey,
				Pass?"Export":"Import",
				0,
				KEY_ALL_ACCESS,
				&ImpExpKey);

		if (Err != ERROR_SUCCESS) {
			SavedError = WINCVT_ERROR_REGISTRY;
			goto abort;
		}
		while(Class) {
			//
			// FIXME? - We're assuming this converter was correctly
			// installed, and removing all its classes.  However,
			// it's possible that the classes it provides are
			// actually being handled by another converter.  If
			// this happens, what should we do?  The current
			// code is the simple answer.
			//
			SavedError = WinCvtIntDeleteKey(ImpExpKey,
					Class->szClassName);

			if (SavedError != WINCVT_SUCCESS) {
				goto abort;
			}

			Class = WinCvtGetNextClass(Class);
		}
		RegCloseKey(ImpExpKey);
		ImpExpKey = NULL;
		if (Output) {
			TempError = WinCvtFreeConverterList(Output);
			ASSERT(TempError == WINCVT_SUCCESS);
		}
		Output = NULL;
		Pass++;
	}

abort:
	if (Output) {
		TempError = WinCvtFreeConverterList(Output);
		ASSERT(TempError == WINCVT_SUCCESS);
	}
	if (ConverterInitialized) {
		WinCvtIntUninitializeConverter(&Context);
		ConverterInitialized = FALSE;
	}
	if (ImpExpKey) {
		RegCloseKey(ImpExpKey);
	}
	if (ConverterKey) {
		RegCloseKey(ConverterKey);
	}
	return SavedError;
}



