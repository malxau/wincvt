/* WINCVT
 * =======
 * This software is Copyright (c) 2006-14 Malcolm Smith.
 * No warranty is provided, including but not limited to
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * This code is licenced subject to the GNU General
 * Public Licence (GPL).  See the COPYING file for more.
 */

#ifdef MINICRT
#include <minicrt.h>
#endif

typedef LONG    (WINAPI * _ReadCvtCallback)(LONG, LONG);

//typedef LONG    (WINAPI * _FetchError)(LONG, LPSTR, LONG);
typedef SHORT   (WINAPI * _ForeignToRtf)(HANDLE, PVOID, HANDLE, HANDLE, HANDLE, _ReadCvtCallback);
typedef VOID    (WINAPI * _GetReadNames)(HANDLE, HANDLE, HANDLE);
typedef VOID    (WINAPI * _GetWriteNames)(HANDLE, HANDLE, HANDLE);
typedef LONG    (WINAPI * _InitConverter)(HANDLE, LPSTR);
typedef SHORT   (WINAPI * _IsFormatCorrect)(HANDLE, HANDLE);
typedef HGLOBAL (WINAPI * _RegisterApp)(LONG, PVOID);
typedef LONG    (WINAPI * _RegisterConverter)(HANDLE);
typedef SHORT   (WINAPI * _RtfToForeign)(HANDLE, PVOID, HANDLE, HANDLE, _ReadCvtCallback);
typedef VOID    (WINAPI * _UninitConverter)();

#define WINCVT_CVT_CLASS_FLAG_PSEUDO 0x00000001

typedef struct _NONOPAQUE_CVT_CLASS {
	LPSTR szClassName;
	LPSTR szExtensions;
	LPSTR szDescription;
	LPSTR szFileName;
	DWORD dwFlags;
	struct _NONOPAQUE_CVT_CLASS * Next;
} NONOPAQUE_CVT_CLASS, *PNONOPAQUE_CVT_CLASS;

typedef struct _NONOPAQUE_CVT_LIST {
	DWORD NumberOfClasses;
	PNONOPAQUE_CVT_CLASS FirstClass;
} NONOPAQUE_CVT_LIST, *PNONOPAQUE_CVT_LIST;

typedef struct _NONOPAQUE_CVT_CONTEXT {
	// Pointers to functions in converter.  Should be NULL if no
	// converter loaded.  Note that some are optional, and need
	// to be checked before using.
//	_FetchError          FetchError;
	_ForeignToRtf        ForeignToRtf;
	_GetReadNames        GetReadNames;
	_GetWriteNames       GetWriteNames;
	_InitConverter       InitConverter;
	_IsFormatCorrect     IsFormatCorrect;
	_RegisterApp         RegisterApp;
	_RegisterConverter   RegisterConverter;
	_RtfToForeign        RtfToForeign;
	_UninitConverter     UninitConverter;

	// Pointer to a memory buffer for use during conversion.
	// Should be NULL when conversion not taking place.
	HANDLE hBuffer;

	// Handle to the converter DLL.  Should be NULL if no converter is
	// loaded.
	HANDLE hConverter;

	// Handle to file.  Should be NULL if conversion is not taking
	// place.
	HANDLE hFile;

	// File offset.  FIXME - should this be larger?
	DWORD dwFileOffset;

	// Pointer to the converter pathname.  Should be NULL if no converter
	// is loaded.
	LPSTR  szConverterFileName;

	// Handle to percentage complete notifications.
	HANDLE hPercentComplete;

	// Pointer to a callback function to use during conversion to indicate
	// progress.
	_WinCvtProgressCallback ProgressRoutine;
} NONOPAQUE_CVT_CONTEXT, *PNONOPAQUE_CVT_CONTEXT;

#ifdef DEBUG_WINCVT
PVOID WinCvtMalloc(size_t x);
HANDLE WinCvtGlobalAlloc(DWORD flags, DWORD x);
VOID WinCvtFree(PVOID x);
VOID WinCvtGlobalFree(HANDLE x);
#else
#define WinCvtMalloc(x) malloc(x)
#define WinCvtGlobalAlloc(x, y) GlobalAlloc(x, y)
#define WinCvtFree(x) free(x)
#define WinCvtGlobalFree(x) GlobalFree(x)
#endif

#ifdef DEBUG_WINCVT_APP
#define APP_ASSERT(x) ASSERT(x)
#else
#define APP_ASSERT(x)
#endif

#define WINCVT_PSEUDO_RTF_CLASS "WinCvt_RTF"

extern PNONOPAQUE_CVT_CONTEXT ActiveConverter;

// support.c
VOID WinCvtIntFreeConverterClassChain(PNONOPAQUE_CVT_CLASS Target);

// context.c
VOID WinCvtIntUninitializeConverter(PNONOPAQUE_CVT_CONTEXT Context);
WINCVT_STATUS WinCvtIntInitializeConverter(PNONOPAQUE_CVT_CONTEXT Context, LPCSTR szConverter);

// capabil.c
WINCVT_STATUS WinCvtIntGetConverterCapabilityList(PNONOPAQUE_CVT_CONTEXT Context, PNONOPAQUE_CVT_LIST* Output, BOOL bExport);




