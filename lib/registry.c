/* WINCVT
 * =======
 * This software is Copyright (c) 2006 Malcolm Smith.
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

static BOOL
WinCvtIntAttemptToGetRegValue(HKEY Key, LPCSTR Value, LPSTR Target)
{
	DWORD dwType;
	CHAR RegBuffer[512];
	DWORD dwRegBuffer = sizeof(RegBuffer);
	if (RegQueryValueEx(Key,
		(LPSTR)Value, // MSVC2 headers don't declare this const.  It is.
		NULL,
		&dwType,
		RegBuffer,
		&dwRegBuffer) != ERROR_SUCCESS) {

		return FALSE;
	}
	if (!(dwType == REG_SZ || dwType == REG_MULTI_SZ || dwType == REG_EXPAND_SZ)) {
		return FALSE;
	}
	if (dwType == REG_EXPAND_SZ) {
		if (!ExpandEnvironmentStrings(RegBuffer, Target, 512))
			return FALSE;
	} else {
		strcpy(Target, RegBuffer);
	}
	return TRUE;
}


/*
 * REGISTRY QUERY FUNCTIONS
 *
 * These functions are to support querying the registry for a list of converters.
 */
WINCVT_STATUS static
WinCvtIntLoadConvertersFromHive(PNONOPAQUE_CVT_LIST* List,
  LPCSTR szWantClass,
  LPCSTR szExtension,
  LPCSTR szFileNameToVerify,
  PNONOPAQUE_CVT_CLASS *FinalEntry,
  HKEY hKey,
  LPCSTR szSubKey)
{
	HKEY ConverterKey, SubKey;
	LONG Err;
	LONG Index = 0;
	PNONOPAQUE_CVT_CLASS Next = NULL;
	PNONOPAQUE_CVT_CLASS Prev = NULL;
	PNONOPAQUE_CVT_CLASS First = NULL;
	CHAR szClass[512];
	DWORD dwClassSize;
	HANDLE hVerifyFileName, hVerifyClass;

	NONOPAQUE_CVT_CONTEXT Context;

	if (szWantClass) {
		if (strlen(szWantClass)>=sizeof(szClass)) {
			return WINCVT_ERROR_BAD_PARAM_2;
		}
	}

	memset(&Context, 0, sizeof(Context));
	ASSERT( (*FinalEntry) == NULL || (*FinalEntry)->Next == NULL );
	Prev = *FinalEntry;

	if (szFileNameToVerify) {
		LPSTR szTmp;

		//
		//  FIXME? - don't safely handle string >4Gb
		//
		hVerifyFileName = WinCvtGlobalAlloc(GMEM_MOVEABLE,
			(DWORD)strlen(szFileNameToVerify)+1);
		if (hVerifyFileName == NULL) {
			return WINCVT_ERROR_NO_MEMORY;
		}
		szTmp = GlobalLock(hVerifyFileName);
		strcpy(szTmp, szFileNameToVerify);
		GlobalUnlock(hVerifyFileName);

		// A converter will reallocate this if the space isn't big enough.
		hVerifyClass = WinCvtGlobalAlloc(GMEM_MOVEABLE, 256);
		if (hVerifyClass == NULL) {
			WinCvtGlobalFree(hVerifyFileName);
			return WINCVT_ERROR_NO_MEMORY;
		}

	}

	Err = RegOpenKeyEx(hKey, szSubKey, 0, KEY_READ, &ConverterKey); 

	if (Err != ERROR_SUCCESS) {
		if (szFileNameToVerify) {
			WinCvtGlobalFree(hVerifyFileName);
			WinCvtGlobalFree(hVerifyClass);
		}
		return WINCVT_ERROR_REGISTRY;
	}

	while (TRUE) {
		if (szWantClass == NULL) {
			dwClassSize = sizeof(szClass);
			Err = RegEnumKeyEx(ConverterKey,
				Index,
				szClass,
				&dwClassSize,
				NULL,
				NULL,
				NULL,
				NULL);

			if (Err != ERROR_SUCCESS) break;

			Err = RegOpenKeyEx(ConverterKey,
				szClass,
				0,
				KEY_READ,
				&SubKey);

			// eek, we can't read a particular converter.  Keep going,
			// hopefully we'll get something.
			if (Err != ERROR_SUCCESS) {
				Index++;
				continue;
			}
		} else {
			if (Index != 0) {
				// We've been asked to move to the next converter,
				// but we were only asked for one.
				break;
			}
			// We know exactly what we want - if we fail, it's over.

			Err = RegOpenKeyEx(ConverterKey,
				szWantClass,
				0,
				KEY_READ,
				&SubKey);
			if (Err != ERROR_SUCCESS) break;

			// Yukky.  We length checked this before.
			strcpy(szClass, szWantClass);
		}

		Next = WinCvtMalloc(sizeof(*Next));
		if (Next) {
			memset(Next, 0, sizeof(*Next));
		}

		if (Next == NULL) {
			if (First)
				WinCvtIntFreeConverterClassChain(First);
			if (*FinalEntry) {
				(*FinalEntry)->Next = NULL;
			} else {
				(*List)->FirstClass = NULL;
			}
			if (szFileNameToVerify) {
				WinCvtGlobalFree(hVerifyFileName);
				WinCvtGlobalFree(hVerifyClass);
			}
			RegCloseKey(ConverterKey);
			return WINCVT_ERROR_NO_MEMORY;
		}


		if (Prev) {
			Prev->Next = Next;
		} else {
			(*List)->FirstClass = Next;
		}

		Next->szClassName = WinCvtMalloc(512);
		Next->szExtensions = WinCvtMalloc(512);
		Next->szDescription = WinCvtMalloc(512);
		Next->dwFlags = 0;
		Next->szFileName = WinCvtMalloc(512);

		if (Next->szClassName == NULL ||
			Next->szExtensions == NULL ||
			Next->szDescription == NULL ||
			Next->szFileName == NULL) {
			// Memory failure.  Roll back everything this function has
			// done and fail.
			if (First)
				WinCvtIntFreeConverterClassChain(First);
			else
				WinCvtIntFreeConverterClassChain(Next);
			if (*FinalEntry) {
				(*FinalEntry)->Next = NULL;
			} else {
				(*List)->FirstClass = NULL;
			}
			if (szFileNameToVerify) {
				WinCvtGlobalFree(hVerifyFileName);
				WinCvtGlobalFree(hVerifyClass);
			}
			RegCloseKey(ConverterKey);
			return WINCVT_ERROR_NO_MEMORY;
		}

		if (!(WinCvtIntAttemptToGetRegValue(SubKey, "Extensions", Next->szExtensions) &&
		      WinCvtIntAttemptToGetRegValue(SubKey, "Name", Next->szDescription) &&
		      WinCvtIntAttemptToGetRegValue(SubKey, "Path", Next->szFileName))) {
			// We're skipping this entry due to malformed registry.  Remove
			// the node we just created and keep going.
			if (Prev) 
				Prev->Next = NULL;
			else
				(*List)->FirstClass = NULL;
			WinCvtIntFreeConverterClassChain(Next);
			RegCloseKey(SubKey);
			Index++;
			continue;
		}
		RegCloseKey(SubKey);

		if (szExtension) {
			CHAR szExtensionCopy[512];
			LPSTR szThisExtension;
			BOOL bHaveMatch = FALSE;

			// Take a copy of the extension list - we're about
			// to modify it.
			strcpy(szExtensionCopy, Next->szExtensions);

			szThisExtension = strtok(szExtensionCopy, " ");
			while (szThisExtension) {
				if (stricmp(szThisExtension, szExtension)==0 || stricmp(szThisExtension, "*")==0) {
					bHaveMatch = TRUE;
					break;
				}
				szThisExtension = strtok(NULL, " ");
			}
			if (!bHaveMatch) {
				// We're skipping this entry, because it doesn't
				// support this extension according to the registry.
				if (Prev) 
					Prev->Next = NULL;
				else
					(*List)->FirstClass = NULL;
				WinCvtIntFreeConverterClassChain(Next);
				Index++;
				continue;
			}
			if (szFileNameToVerify) {
				LONG Ret;
				if (WinCvtIntInitializeConverter(&Context, Next->szFileName) != WINCVT_SUCCESS) {
					// If we can't load the converter, verification
					// fails.  Move to next converter.
					if (Prev) 
						Prev->Next = NULL;
					else
						(*List)->FirstClass = NULL;
					WinCvtIntFreeConverterClassChain(Next);
					Index++;
					continue;
				}
				Ret = Context.IsFormatCorrect(hVerifyFileName, hVerifyClass);
				if (Ret != 1) {
					// The converter didn't like our file.  Continue
					// to the next one.
					if (Prev) 
						Prev->Next = NULL;
					else
						(*List)->FirstClass = NULL;
					WinCvtIntFreeConverterClassChain(Next);
					Index++;
					WinCvtIntUninitializeConverter(&Context);
					continue;
				}
				// If we're still here, this is a match - add this to the
				// list and go to the next one.
				WinCvtIntUninitializeConverter(&Context);
			}

		}

		strcpy(Next->szClassName, szClass);
		Prev = Next;

		// Record the first entry from this registry key in case we need to roll
		// back.
		if (First == NULL) {
			First = Next;
		}

		(*List)->NumberOfClasses++;
		Index++;
	}

	RegCloseKey(ConverterKey);

	if (szFileNameToVerify) {
		WinCvtGlobalFree(hVerifyFileName);
		WinCvtGlobalFree(hVerifyClass);
	}

	*FinalEntry = Prev;

	if (szWantClass && Prev == NULL) {
		return WINCVT_ERROR_NO_CONVERTERS;
	}

	return WINCVT_SUCCESS;

}

#define IsFatalUserReturnCode(Ret) \
	((Ret != WINCVT_SUCCESS) && (Ret != WINCVT_ERROR_REGISTRY) && (Ret != WINCVT_ERROR_NO_CONVERTERS))

WINCVT_STATUS
WinCvtIntIncludePseudoImportConverters(
  PNONOPAQUE_CVT_LIST* List,
  LPCSTR szClass,
  LPCSTR szExtension,
  LPCSTR szFileNameToVerify,
  PNONOPAQUE_CVT_CLASS *FinalEntry)
{
	BOOL IncludeRtf = TRUE;

	if (szClass != NULL &&
		stricmp(szClass, WINCVT_PSEUDO_RTF_CLASS) != 0) {
		//
		//  The caller has asked for a known class, and it's
		//  different to the RTF class.
		//
		IncludeRtf = FALSE;
	}


	if (szExtension != NULL &&
		stricmp(szExtension, "rtf") != 0) {
		//
		//  The caller has asked for a known extension, and it's
		//  different to the RTF class.
		//
		IncludeRtf = FALSE;
	}

	if (IncludeRtf && szFileNameToVerify != NULL) {
		HANDLE hFile;
		hFile = CreateFile(szFileNameToVerify,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hFile == INVALID_HANDLE_VALUE) {
			//
			//  Can't open file, so pretend it's not convertable.
			//
			IncludeRtf = FALSE;
		} else {
			CHAR szContents[10];
			DWORD BytesRead;
			if (ReadFile(hFile, szContents, 6, &BytesRead, NULL) && BytesRead == 6) {
				//
				//  If we can read 6 bytes, and it's not "{\rtf1"
				//  then Rtf can't convert it.
				//
				if (memcmp(szContents, "{\\rtf1", 6) != 0) {
					IncludeRtf = FALSE;
				}
			} else {
				//
				//  If we can't read from the file, it's not RTF.
				//
				IncludeRtf = FALSE;
			}
			CloseHandle(hFile);
		}
	}

	if (IncludeRtf) {
		PNONOPAQUE_CVT_CLASS RtfClass = NULL;

		RtfClass = WinCvtMalloc(sizeof(*RtfClass));
		if (RtfClass) {
			memset(RtfClass, 0, sizeof(*RtfClass));
		}

		if (RtfClass == NULL) {
			return WINCVT_ERROR_NO_MEMORY;
		}


		RtfClass->szClassName = WinCvtMalloc(strlen(WINCVT_PSEUDO_RTF_CLASS)+1);
		RtfClass->szExtensions = WinCvtMalloc(4);
		RtfClass->szDescription = WinCvtMalloc(4);
		RtfClass->dwFlags = 0;
		RtfClass->szFileName = NULL;

		if (RtfClass->szClassName == NULL ||
			RtfClass->szExtensions == NULL ||
			RtfClass->szDescription == NULL) {

			//
			// Memory failure.  Roll back everything this function has
			// done and fail.
			//
			WinCvtIntFreeConverterClassChain(RtfClass);
			return WINCVT_ERROR_NO_MEMORY;
		}

		strcpy(RtfClass->szClassName, WINCVT_PSEUDO_RTF_CLASS);
		strcpy(RtfClass->szExtensions, "rtf");
		strcpy(RtfClass->szDescription, "RTF");
		RtfClass->dwFlags = WINCVT_CVT_CLASS_FLAG_PSEUDO;

		if (*FinalEntry) {
			ASSERT( (*FinalEntry)->Next == NULL );
			RtfClass->Next = NULL;
			(*FinalEntry)->Next = RtfClass;
		} else {
			RtfClass->Next = (*List)->FirstClass;
			(*List)->FirstClass = RtfClass;
		}

		//
		//  FIXME - may want to rework this, assumes RTF is the
		//  only one.
		//
		*FinalEntry = RtfClass;

		(*List)->NumberOfClasses++;
	}
	return WINCVT_SUCCESS;
}

WINCVT_STATUS
WinCvtGetImportConverterList(
  PNONOPAQUE_CVT_LIST* List,
  LPCSTR szClass,
  LPCSTR szExtension,
  LPCSTR szFileNameToVerify)
{

	PNONOPAQUE_CVT_CLASS FinalEntry;
	LPCSTR szRealExtension = NULL;
	WINCVT_STATUS Ret;

	*List = WinCvtMalloc(sizeof(NONOPAQUE_CVT_LIST));
	if (*List == NULL) {
		return WINCVT_ERROR_NO_MEMORY;
	}

	(*List)->NumberOfClasses = 0;
	(*List)->FirstClass = NULL;

	FinalEntry = NULL;

	if (szExtension) {
		szRealExtension = strrchr(szExtension, '.');
		if (szRealExtension) {
			// The user passed a filename, so we jump to after the final '.'.
			szRealExtension++;
		} else {
			// There is no . - the user just passed in the extension.
			szRealExtension = szExtension;
		}
	}

	Ret = WinCvtIntIncludePseudoImportConverters(
			List,
			szClass,
			szRealExtension,
			szFileNameToVerify,
			&FinalEntry);

	ASSERT( Ret != WINCVT_SUCCESS ||
		szClass != NULL ||
		szRealExtension != NULL ||
		szFileNameToVerify != NULL || 
		((*List)->NumberOfClasses>0 && FinalEntry!=NULL));

	if (Ret != WINCVT_SUCCESS) {
		//
		//  Note: this assumes the list is empty.
		//
		WinCvtFree( *List );
		*List = NULL;
		return Ret;
	}

	Ret = WinCvtIntLoadConvertersFromHive(
			List,
			szClass,
			szRealExtension,
			szFileNameToVerify,
			&FinalEntry,
			HKEY_CURRENT_USER,
			"SOFTWARE\\Microsoft\\Shared Tools\\Text Converters\\Import");

	if (IsFatalUserReturnCode(Ret)) {
		WinCvtFreeConverterList( *List );
		*List = NULL;
		return Ret;
	}

	Ret = WinCvtIntLoadConvertersFromHive(
			List,
			szClass,
			szRealExtension,
			szFileNameToVerify,
			&FinalEntry,
			HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Shared Tools\\Text Converters\\Import");

	if (Ret != WINCVT_SUCCESS) {
		WinCvtFreeConverterList( *List );
		*List = NULL;
	}
	return Ret;
}

WINCVT_STATUS
WinCvtGetExportConverterList(
  PNONOPAQUE_CVT_LIST* List,
  LPCSTR szClass,
  LPCSTR szExtension)
{

	PNONOPAQUE_CVT_CLASS FinalEntry;
	LPCSTR szRealExtension = NULL;
	WINCVT_STATUS Ret;

	*List = WinCvtMalloc(sizeof(NONOPAQUE_CVT_LIST));
	if (*List == NULL) {
		return WINCVT_ERROR_NO_MEMORY;
	}

	(*List)->NumberOfClasses = 0;
	(*List)->FirstClass = NULL;

	FinalEntry = NULL;

	if (szExtension) {
		szRealExtension = strrchr(szExtension, '.');
		if (szRealExtension) {
			// The user passed a filename, so we jump to after the final '.'.
			szRealExtension++;
		} else {
			// There is no . - the user just passed in the extension.
			szRealExtension = szExtension;
		}
	}
	Ret = WinCvtIntLoadConvertersFromHive(
			List,
			szClass,
			szRealExtension,
			NULL,
			&FinalEntry,
			HKEY_CURRENT_USER,
			"SOFTWARE\\Microsoft\\Shared Tools\\Text Converters\\Export");

	if (IsFatalUserReturnCode(Ret)) {
		//
		//  This assumes the list is empty at this point.
		//
		WinCvtFree(*List);
		*List = NULL;
		return Ret;
	}

	if (szClass != NULL &&
		(*List)->NumberOfClasses == 1 &&
		Ret == WINCVT_SUCCESS) {
		//
		// Export converters guarantee uniqueness, ie., there
		// should only ever be one match for a particular class
		// string.  Here, we've found a match from the user
		// registry.  That takes precedence over the system
		// registry.  Return the match now and don't risk finding
		// a dupe.
		//
		return Ret;
	}

	Ret = WinCvtIntLoadConvertersFromHive(
			List,
			szClass,
			szRealExtension,
			NULL,
			&FinalEntry,
			HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Shared Tools\\Text Converters\\Export");

	if (Ret != WINCVT_SUCCESS) {
		WinCvtFreeConverterList( *List );
		*List = NULL;
	}
	return Ret;
}


