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
 * DEBUG CONDITIONALS
 *
 * These are dual implementations of macros and functions depending on whether
 * this is a debug build or not.
 */

#ifdef DEBUG_WINCVT
ULONG WinCvtMallocs = 0;
ULONG WinCvtGlobalAllocs = 0;
#define MEM_FAILURE_PROB 0


PVOID WinCvtMalloc(size_t x)
{
	PVOID ret;
	if (WinCvtMallocs == 0) {
		//
		// We get warnings about data loss here.  Safe to ignore, since
		// the whole point is to feed a random (inexact) number in
		// anyway.
		//
		srand((unsigned int)time(NULL));
	}
	if (MEM_FAILURE_PROB && (rand()%MEM_FAILURE_PROB==0)) {
		return NULL;
	}
	ret = malloc(x);
	if (ret) {
		InterlockedIncrement(&WinCvtMallocs);
	}
	return ret;
}

HANDLE WinCvtGlobalAlloc(DWORD flags, DWORD x)
{
	HANDLE ret;
	if (WinCvtGlobalAllocs == 0) {
		//
		// We get warnings about data loss here.  Safe to ignore, since
		// the whole point is to feed a random (inexact) number in
		// anyway.
		//
		srand((unsigned int)time(NULL));
	}
	if (MEM_FAILURE_PROB && (rand()%MEM_FAILURE_PROB==0)) {
		return NULL;
	}
	ret = GlobalAlloc(flags, x);
	if (ret) {
		InterlockedIncrement(&WinCvtGlobalAllocs);
	}
	return ret;
}

VOID WinCvtFree(PVOID x)
{
	ASSERT(WinCvtMallocs > 0);
	free(x);
	InterlockedDecrement(&WinCvtMallocs);
}

VOID WinCvtGlobalFree(HANDLE x)
{
	ASSERT(WinCvtGlobalAllocs > 0);
	GlobalFree(x);
	InterlockedDecrement(&WinCvtGlobalAllocs);
}

#endif


VOID
WinCvtIntFreeConverterClassChain(PNONOPAQUE_CVT_CLASS Target)
{
	PNONOPAQUE_CVT_CLASS Current, Next;

	Next = Target;
	Current = Target;
	while (Current) {
		Next=Current->Next;
		if (Current->szClassName) WinCvtFree(Current->szClassName);
		if (Current->szExtensions) WinCvtFree(Current->szExtensions);
		if (Current->szDescription) WinCvtFree(Current->szDescription);
		if (Current->szFileName) WinCvtFree(Current->szFileName);
		WinCvtFree(Current);
		Current=Next;
	}
}

/*
 * MISC EXPORT FUNCTIONS
 *
 * These are small peripheral support functions which are also exported to
 * applications.
 */

WINCVT_STATUS
WinCvtGetErrorString(
  LONG Error,
  LPSTR szBuffer,
  DWORD BufLen)
{
	APP_ASSERT(szBuffer!=NULL);
	APP_ASSERT(BufLen>0);
	switch(Error) {
		// Converter codes
		case WINCVT_SUCCESS:
			_snprintf(szBuffer, BufLen, "The operation completed successfully.");
			break;
		case WINCVT_ERROR_OPEN:
			_snprintf(szBuffer, BufLen, "The converter could not open the specified file.");
			break;
		case WINCVT_ERROR_READ:
			_snprintf(szBuffer, BufLen, "The converter could not read from the specified file.");
			break;
		case WINCVT_ERROR_WRITE:
			_snprintf(szBuffer, BufLen, "The converter could not write to the specified file.");
			break;
		case WINCVT_ERROR_DATA:
			_snprintf(szBuffer, BufLen, "The converter cannot convert this file because it contains invalid data for this format.");
			break;
		case WINCVT_ERROR_NO_MEMORY:
			_snprintf(szBuffer, BufLen, "There is not enough memory to complete this operation.");
			break;
		case WINCVT_ERROR_DISK_FULL:
			_snprintf(szBuffer, BufLen, "The converter could not write to the disk because it is full.");
			break;
		case WINCVT_ERROR_CREATE:
			_snprintf(szBuffer, BufLen, "The converter could not create the specified file.");
			break;
		case WINCVT_ERROR_CANCEL:
			// Note - we don't yet support cancel.  This can only happen by a buggy converter.
			_snprintf(szBuffer, BufLen, "The operation was cancelled by the user.");
			break;
		case WINCVT_ERROR_WRONG_TYPE:
			_snprintf(szBuffer, BufLen, "The converter cannot convert this file because it is in an unsupported format.");
			break;
		case WINCVT_ERROR_BAD_IMAGE:
			_snprintf(szBuffer, BufLen, "The converter cannot be loaded because it has not been installed correctly.");
			break;

		// WINCVT custom codes
		case WINCVT_ERROR_BAD_PARAM_1:
		case WINCVT_ERROR_BAD_PARAM_2:
		case WINCVT_ERROR_BAD_PARAM_3:
		case WINCVT_ERROR_BAD_PARAM_4:
		case WINCVT_ERROR_BAD_PARAM_5:
		case WINCVT_ERROR_BAD_PARAM_6:
		case WINCVT_ERROR_BAD_PARAM_7:
		case WINCVT_ERROR_BAD_PARAM_8:
		case WINCVT_ERROR_BAD_PARAM_9:
			_snprintf(szBuffer, BufLen, "Parameter %i passed to function is invalid.  This is an application bug.", Error-WINCVT_ERROR_CUSTOM_START);
			break;
		case WINCVT_ERROR_BAD_ENTRY:
			_snprintf(szBuffer, BufLen, "Functionality not supported by converter.");
			break;
		case WINCVT_ERROR_REGISTRY:
			_snprintf(szBuffer, BufLen, "The registry does not contain required information for this operation.");
			break;
		case WINCVT_ERROR_CONVERTER_VETO:
			_snprintf(szBuffer, BufLen, "The converter declined to proceed with this operation.");
			break;
		case WINCVT_ERROR_NO_CONVERTERS:
			_snprintf(szBuffer, BufLen, "The system does not contain converters that can convert this file.");
			break;

		case WINCVT_ERROR_BAD_PATH:
			_snprintf(szBuffer, BufLen, "The specified path could not be expanded to a full path.");
			break;

		default:
			_snprintf(szBuffer, BufLen, "Error string not implemented\n");
			ASSERT(FALSE);
			break;
		break;
	}
	return WINCVT_SUCCESS;
}

WINCVT_STATUS
WinCvtFreeConverterList(PNONOPAQUE_CVT_LIST Target)
{
	APP_ASSERT(Target != NULL);
	if (Target == NULL) {
		return WINCVT_ERROR_BAD_PARAM_1;
	}

	if (Target->FirstClass) {
		WinCvtIntFreeConverterClassChain(Target->FirstClass);
	}
	WinCvtFree(Target);
	return WINCVT_SUCCESS;
}

WINCVT_CVT_CLASS
WinCvtGetFirstClass(PNONOPAQUE_CVT_LIST Capability)
{
	APP_ASSERT(Capability != NULL);
	if (Capability == NULL) {
		return NULL;
	}

	return (WINCVT_CVT_CLASS)Capability->FirstClass;
}

WINCVT_CVT_CLASS
WinCvtGetNextClass(PNONOPAQUE_CVT_CLASS Class)
{
	APP_ASSERT(Class != NULL);
	if (Class == NULL) {
		return NULL;
	}

	ASSERT( !IsBadReadPtr( Class, sizeof(*Class) ) );

	return (WINCVT_CVT_CLASS)Class->Next;
}

LPCSTR
WinCvtGetClassFileName(PNONOPAQUE_CVT_CLASS Class)
{
	APP_ASSERT(Class != NULL);
	if (Class == NULL) {
		return NULL;
	}

	ASSERT( !IsBadReadPtr( Class, sizeof(*Class) ) );

	return Class->szFileName;
}

LPCSTR
WinCvtGetClassName(PNONOPAQUE_CVT_CLASS Class)
{
	APP_ASSERT(Class != NULL);
	if (Class == NULL) {
		return NULL;
	}

	ASSERT( !IsBadReadPtr( Class, sizeof(*Class) ) );

	return Class->szClassName;
}

LPCSTR
WinCvtGetClassDescription(PNONOPAQUE_CVT_CLASS Class)
{
	APP_ASSERT(Class != NULL);
	if (Class == NULL) {
		return NULL;
	}

	ASSERT( !IsBadReadPtr( Class, sizeof(*Class) ) );

	return Class->szDescription;
}

LPCSTR
WinCvtGetClassExtensions(PNONOPAQUE_CVT_CLASS Class)
{
	APP_ASSERT(Class != NULL);
	if (Class == NULL) {
		return NULL;
	}

	ASSERT( !IsBadReadPtr( Class, sizeof(*Class) ) );

	return Class->szExtensions;
}

DWORD WinCvtLibraryVersion()
{
	return WinCvtHeaderVersion();
}

VOID WinCvtFinalTeardown()
{
#ifdef DEBUG_WINCVT
	ASSERT(WinCvtMallocs == 0);
	ASSERT(WinCvtGlobalAllocs == 0);
	ASSERT(ActiveConverter == NULL);
#endif
}


BOOL
WinCvtAreVersionsCompatible(
  DWORD HeaderVersion)
{
	DWORD LibraryVersion;

	LibraryVersion = WinCvtLibraryVersion();
	if (WINCVT_GET_MAJOR_VERSION(LibraryVersion) != WINCVT_GET_MAJOR_VERSION(HeaderVersion)) {
		return FALSE;
	}

	if (WINCVT_GET_MAJOR_VERSION(LibraryVersion) == 0) {
		//
		//  We're a beta version, so the rules are a little different.
		//
		//  Minor versions in beta are not guaranteed to be compatible (0.2.0 not compatible
		//  with 0.4.0)
		//
		if (WINCVT_GET_MINOR_VERSION(LibraryVersion) != WINCVT_GET_MINOR_VERSION(HeaderVersion)) {
			return FALSE;
		}

		if (WINCVT_GET_MINOR_VERSION(LibraryVersion) & 1) {
			//
			//  We're running an unstable beta version, eg. 0.3.0.
			//  In this case, micro versions must match exactly.
			//
			if (WINCVT_GET_MICRO_VERSION(LibraryVersion) != WINCVT_GET_MICRO_VERSION(HeaderVersion)) {
				return FALSE;
			}
		} else {
			//
			//  We're running a stable beta version.  Should be
			//  upwardly compatible across mico versions.
			//

			if (WINCVT_GET_MICRO_VERSION(LibraryVersion) < WINCVT_GET_MICRO_VERSION(HeaderVersion)) {
				return FALSE;
			}
		}
	} else {
		//
		//  Release version.
		//
		//  Minor versions in release are guaranteed to be upward compatible (1.4.0 lib will 
		//  run 1.2.0 app)
		//
		if (WINCVT_GET_MINOR_VERSION(LibraryVersion) < WINCVT_GET_MINOR_VERSION(HeaderVersion)) {
			return FALSE;
		}

		//
		//  FIXME? - Is an app compiled against 1.2.2 guaranteed to run on 1.2.0?  Should be
		//  no new exports, but possibly behavior changes.
		//
		if ((WINCVT_GET_MINOR_VERSION(LibraryVersion) == WINCVT_GET_MINOR_VERSION(HeaderVersion)) &&
				(WINCVT_GET_MICRO_VERSION(LibraryVersion) < WINCVT_GET_MICRO_VERSION(HeaderVersion))) {
			return FALSE;
		}
	}
	return TRUE;
}

