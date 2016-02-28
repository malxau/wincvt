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
 * INTERNAL FUNCTIONS
 *
 * These are helper functions to support various aspects of wincvt.  They're
 * up here so they don't need to be declared in advance.  Everything here
 * must be 'static'.  There should be no APP_ASSERTs either.  Assume valid
 * data, crash if it isn't.
 */
VOID
WinCvtIntUninitializeConverter(PNONOPAQUE_CVT_CONTEXT Context)
{
	ASSERT(Context != NULL);

	ASSERT( !IsBadReadPtr( Context, sizeof(*Context) ) );

	if (Context->UninitConverter) {
		Context->UninitConverter();
	}

	ASSERT(Context->hConverter != NULL);
	FreeLibrary(Context->hConverter);

	ASSERT(Context->hBuffer == NULL);
	ASSERT(Context->ProgressRoutine == NULL);
	ASSERT(Context->dwFileOffset == 0);
	ASSERT(Context->hPercentComplete == NULL);

	ASSERT(Context->szConverterFileName != NULL);
	WinCvtFree(Context->szConverterFileName);

	memset(Context, 0, sizeof(NONOPAQUE_CVT_CONTEXT));
}

WINCVT_STATUS
WinCvtIntInitializeConverter(
  PNONOPAQUE_CVT_CONTEXT Context,
  LPCSTR szConverter)
{
	WINCVT_STATUS Status = WINCVT_SUCCESS;
	LONG InitError;

	ASSERT(Context != NULL);
	ASSERT( !IsBadReadPtr( Context, sizeof(*Context) ) );

	ASSERT(Context->hConverter == NULL);
	ASSERT(Context->hFile == NULL);
	ASSERT(Context->hBuffer == NULL);
	ASSERT(Context->szConverterFileName == NULL);
	ASSERT(Context->ProgressRoutine == NULL);
	ASSERT(Context->dwFileOffset == 0);
	ASSERT(Context->hPercentComplete == NULL);

	Context->hConverter = LoadLibrary(szConverter);
	if (Context->hConverter == NULL) {
		return WINCVT_ERROR_BAD_PARAM_2;
	}

	Context->szConverterFileName = WinCvtMalloc(strlen(szConverter)+1);
	if (Context->szConverterFileName == NULL) {
		FreeLibrary(Context->hConverter);
		Context->hConverter = NULL;
		return WINCVT_ERROR_NO_MEMORY;
	}
	strcpy(Context->szConverterFileName, szConverter);

	// FIXME - Add more, 16/32 support
//	ASSERT(Context->FetchError == NULL);
//	Context->FetchError = (_FetchError)GetProcAddress(Context->hConverter, "CchFetchLpszError");
	ASSERT(Context->ForeignToRtf == NULL);
	Context->ForeignToRtf = (_ForeignToRtf)GetProcAddress(Context->hConverter, "ForeignToRtf32");
	ASSERT(Context->RtfToForeign == NULL);
	Context->RtfToForeign = (_RtfToForeign)GetProcAddress(Context->hConverter, "RtfToForeign32");
	ASSERT(Context->GetReadNames == NULL);
	Context->GetReadNames = (_GetReadNames)GetProcAddress(Context->hConverter, "GetReadNames");
	ASSERT(Context->GetWriteNames == NULL);
	Context->GetWriteNames = (_GetWriteNames)GetProcAddress(Context->hConverter, "GetWriteNames");
	ASSERT(Context->InitConverter == NULL);
	Context->InitConverter = (_InitConverter)GetProcAddress(Context->hConverter, "InitConverter32");
	ASSERT(Context->IsFormatCorrect == NULL);
	Context->IsFormatCorrect = (_IsFormatCorrect)GetProcAddress(Context->hConverter, "IsFormatCorrect32");
	ASSERT(Context->RegisterApp == NULL);
	Context->RegisterApp = (_RegisterApp)GetProcAddress(Context->hConverter, "RegisterApp");
	ASSERT(Context->RegisterConverter == NULL);
	Context->RegisterConverter = (_RegisterConverter)GetProcAddress(Context->hConverter, "FRegisterConverter");
	ASSERT(Context->UninitConverter == NULL);
	Context->UninitConverter = (_UninitConverter)GetProcAddress(Context->hConverter, "UninitConverter");

	if (Context->InitConverter == NULL || Context->IsFormatCorrect == NULL || Context->ForeignToRtf == NULL) {
		// Don't call uninit since we haven't called init.
		Context->UninitConverter = NULL;
		WinCvtIntUninitializeConverter(Context);
		return WINCVT_ERROR_BAD_ENTRY;
	}

	InitError = Context->InitConverter(NULL, "WinCvt");
	if (InitError == 0) {
		WinCvtIntUninitializeConverter(Context);
		return WINCVT_ERROR_CONVERTER_VETO;
	}

	if (Context->RegisterApp) {
		HGLOBAL CvtPrefs;

		//
		// 1 == Support percentage complete notification in export;
		// 2 == Don't allow binary encoding in RTF (use hex);
		// 4 == Preview mode - no dialogs
		// 8 == ANSI filenames
		//
		CvtPrefs = Context->RegisterApp(1|2|4|8, NULL);
		if (CvtPrefs) {
			// We didn't allocate this one, so free it directly
			// instead of using the debugging counters.
			GlobalFree(CvtPrefs);
		}
	}
	return WINCVT_SUCCESS;
}

