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

WINCVT_STATUS
WinCvtIntGetConverterCapabilityList(
  PNONOPAQUE_CVT_CONTEXT Context,
  PNONOPAQUE_CVT_LIST* Output,
  BOOL bExport)
{
	PNONOPAQUE_CVT_CLASS Next = NULL;
	PNONOPAQUE_CVT_CLASS First = NULL;
	PNONOPAQUE_CVT_CLASS Prev = NULL;

	HANDLE hClasses, hDescriptions, hExtensions = NULL;
	LPSTR szClasses, szDescriptions, szExtensions;

	if ((!bExport) && (Context->GetReadNames == NULL)) {
		return WINCVT_ERROR_BAD_ENTRY;
	}
	if (bExport && (Context->GetWriteNames == NULL)) {
		return WINCVT_ERROR_BAD_ENTRY;
	}
	*Output = WinCvtMalloc(sizeof(NONOPAQUE_CVT_LIST));
	if (*Output == NULL) {
		return WINCVT_ERROR_NO_MEMORY;
	}

	(*Output)->NumberOfClasses = 0;
	First = WinCvtMalloc(sizeof(NONOPAQUE_CVT_CLASS));
	if (First == NULL) {
		WinCvtFree(*Output);
		*Output = NULL;
		return WINCVT_ERROR_NO_MEMORY;
	}

	hClasses = WinCvtGlobalAlloc(GMEM_MOVEABLE, 4096);
	hDescriptions = WinCvtGlobalAlloc(GMEM_MOVEABLE, 4096);
	hExtensions = WinCvtGlobalAlloc(GMEM_MOVEABLE, 4096);
	if (hClasses == NULL || hDescriptions == NULL || hExtensions == NULL)
	{
		if (hClasses) WinCvtGlobalFree(hClasses);
		if (hDescriptions) WinCvtGlobalFree(hDescriptions);
		if (hExtensions) WinCvtGlobalFree(hExtensions);
		WinCvtFree(First);
		WinCvtFree(*Output);
		*Output = NULL;
		return WINCVT_ERROR_NO_MEMORY;
	}

	// These need to be initialized as empty because GetReadNames/
	// GetWriteNames are void and the only way to detect failure
	// is if the buffers have not been modified
	szClasses = GlobalLock(hClasses);
	szDescriptions = GlobalLock(hDescriptions);
	szExtensions = GlobalLock(hExtensions);

	strcpy(szClasses, "");
	strcpy(szDescriptions, "");
	strcpy(szExtensions, "");

	GlobalUnlock(hClasses);
	GlobalUnlock(hDescriptions);
	GlobalUnlock(hExtensions);

	Next = First;
	memset(Next, 0, sizeof(*Next));

	if (bExport) {
		Context->GetWriteNames(hClasses, hDescriptions, hExtensions);
	} else {
		Context->GetReadNames(hClasses, hDescriptions, hExtensions);
	}
	szClasses = GlobalLock(hClasses);
	szDescriptions = GlobalLock(hDescriptions);
	szExtensions = GlobalLock(hExtensions);

	while (*szClasses) {
		Next->szClassName = WinCvtMalloc(strlen(szClasses)+1);
		Next->szExtensions = WinCvtMalloc(strlen(szExtensions)+1);
		Next->szDescription = WinCvtMalloc(strlen(szDescriptions)+1);
		Next->szFileName = WinCvtMalloc(strlen(Context->szConverterFileName)+1);
		Next->dwFlags = 0;

		if (Prev) {
			Prev->Next = Next;
		} else {
			ASSERT( Next == First );
		}
		Prev = Next;

		Next = WinCvtMalloc(sizeof(*Next));

		if (Prev->szClassName == NULL || Prev->szExtensions == NULL || Prev->szDescription == NULL || Prev->szFileName == NULL || Next == NULL) {
			WinCvtGlobalFree(hClasses);
			WinCvtGlobalFree(hDescriptions);
			WinCvtGlobalFree(hExtensions);
			WinCvtIntFreeConverterClassChain(First);
			WinCvtFree(*Output);
			if (Next)
				WinCvtFree(Next);
			*Output = NULL;
			return WINCVT_ERROR_NO_MEMORY;
		}
		memset(Next, 0, sizeof(*Next));
		strcpy(Prev->szFileName, Context->szConverterFileName);
		strcpy(Prev->szClassName, szClasses);
		strcpy(Prev->szExtensions, szExtensions);
		strcpy(Prev->szDescription, szDescriptions);

		(*Output)->NumberOfClasses++;

		szClasses+=strlen(szClasses);
		szDescriptions+=strlen(szDescriptions);
		szExtensions+=strlen(szExtensions);
	}

	if (Prev) {
		Prev->Next = NULL;
	} else {
		First = NULL;
	}
	WinCvtFree(Next);

	(*Output)->FirstClass = First;

	WinCvtGlobalFree(hClasses);
	WinCvtGlobalFree(hDescriptions);
	WinCvtGlobalFree(hExtensions);

	return WINCVT_SUCCESS;
}

WINCVT_STATUS
WinCvtGetConverterImportCapabilityList(
  LPCSTR szConverter,
  PNONOPAQUE_CVT_LIST* Output)
{
	WINCVT_STATUS Status;
	NONOPAQUE_CVT_CONTEXT Context;
	memset(&Context, 0, sizeof(Context));

	Status = WinCvtIntInitializeConverter(&Context, szConverter);
	if (Status != WINCVT_SUCCESS) {
		return Status;
	}
	
	Status = WinCvtIntGetConverterCapabilityList(&Context, Output, FALSE);
	WinCvtIntUninitializeConverter(&Context);
	return Status;
}

WINCVT_STATUS
WinCvtGetConverterExportCapabilityList(
  LPCSTR szConverter,
  PNONOPAQUE_CVT_LIST* Output)
{
	WINCVT_STATUS Status;
	NONOPAQUE_CVT_CONTEXT Context;
	memset(&Context, 0, sizeof(Context));

	Status = WinCvtIntInitializeConverter(&Context, szConverter);
	if (Status != WINCVT_SUCCESS) {
		return Status;
	}
	
	Status = WinCvtIntGetConverterCapabilityList(&Context, Output, TRUE);
	WinCvtIntUninitializeConverter(&Context);
	return Status;
}

