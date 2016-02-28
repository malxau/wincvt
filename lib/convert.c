/* WINCVT
 * =======
 * This software is Copyright (c) 2006-2007 Malcolm Smith.
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

// FIXME - Need to synchronise this.
PNONOPAQUE_CVT_CONTEXT ActiveConverter = NULL;
_WinCvtProgressCallback ConvertFileProgress = NULL;
WORD ConvertFilePhase = 0;

/*
 * CONVERT TO RTF
 *
 * These functions are to support the main plumbing of implementing
 * native to RTF conversion.
 */

LONG WINAPI
ReadCvtCallback(LONG a, LONG b)
{
	LPSTR txt;
	LONG  err;
	DWORD ActuallyWritten = 0;
	ASSERT(ActiveConverter != NULL);
	ASSERT(ActiveConverter->hBuffer != NULL);
	ASSERT(ActiveConverter->hFile != NULL);

	//
	// Percent complete notifications are export only.
	//
	ASSERT(ActiveConverter->hPercentComplete == NULL);

	txt = GlobalLock(ActiveConverter->hBuffer);
	ASSERT(txt != NULL);
	//txt[a]='\0';
	if (!WriteFile(ActiveConverter->hFile, txt, a, &ActuallyWritten, NULL)) {
		return WINCVT_ERROR_WRITE;
	}
	GlobalUnlock(ActiveConverter->hBuffer);

	ActiveConverter->dwFileOffset += ActuallyWritten;

	if (ActiveConverter->ProgressRoutine) {
		err = ActiveConverter->ProgressRoutine(a, b);
		if (err != WINCVT_SUCCESS) {
			return err;
		}
	}

	return WINCVT_SUCCESS;
}

static WINCVT_STATUS
WinCvtIntConvertToRtf(PNONOPAQUE_CVT_CONTEXT Context, LPCSTR szFileName)
{
	HANDLE hFileName = NULL;
	HANDLE hClass = NULL;
	LPSTR  szTmp;

	SHORT  Ret;
	WINCVT_STATUS SavedError = WINCVT_SUCCESS; // If we forget to set it, this will assert

	ASSERT(Context != NULL);

	ASSERT(Context->hConverter != NULL);
	ASSERT(Context->hBuffer != NULL);
	ASSERT(Context->hFile != NULL);
	ASSERT(Context->dwFileOffset == 0);

	ASSERT(szFileName != NULL);

	//
	// FIXME? - Don't handle strings >4Gb
	//
	hFileName = WinCvtGlobalAlloc(GMEM_MOVEABLE,
		(DWORD)strlen(szFileName)+1);
	if (hFileName == NULL) {
		SavedError = WINCVT_ERROR_NO_MEMORY;
		goto abort;
	}
	szTmp = GlobalLock(hFileName);
	strcpy(szTmp, szFileName);
	GlobalUnlock(hFileName);

	// A converter will reallocate this if the space isn't big enough.
	hClass = WinCvtGlobalAlloc(GMEM_MOVEABLE, 256);
	if (hClass == NULL) {
		SavedError = WINCVT_ERROR_NO_MEMORY;
		goto abort;
	}

	Ret = Context->IsFormatCorrect(hFileName, hClass);
	if (Ret != 1) {
		if (Ret < 0)
			SavedError = Ret;
		else
			SavedError = WINCVT_ERROR_WRONG_TYPE;

		goto abort;
	}

	// FIXME - Let's assume we're all single threaded...
	ASSERT(ActiveConverter == NULL);
	ActiveConverter = Context;

	Ret = Context->ForeignToRtf(hFileName,
		NULL,
		Context->hBuffer,
		NULL,
		NULL,
		ReadCvtCallback);

	ASSERT(ActiveConverter == Context);
	ActiveConverter = NULL;
	if (Ret != 0) {
		SavedError = Ret;
		goto abort;
	}

	Context->dwFileOffset = 0;

	// Success.  Teardown now.

	WinCvtGlobalFree(hFileName);
	WinCvtGlobalFree(hClass);

	return WINCVT_SUCCESS;

abort:
	ASSERT(SavedError != WINCVT_SUCCESS);
	Context->dwFileOffset = 0;
	if (hFileName) {
		WinCvtGlobalFree(hFileName);
	}
	if (hClass) {
		WinCvtGlobalFree(hClass);
	}
	return SavedError;
}

WINCVT_STATUS
WinCvtIntPseudoImport(
  LPCSTR szFileName,
  PNONOPAQUE_CVT_CLASS Class,
  HANDLE hOutput,
  _WinCvtProgressCallback ProgressRoutine)
{
	LPCSTR szClassName;
	BOOL bHandled = FALSE;
	ASSERT( Class != NULL );
	ASSERT( szFileName != NULL );

	szClassName = WinCvtGetClassName( Class );

	ASSERT( szClassName != NULL );
	ASSERT( Class->dwFlags & WINCVT_CVT_CLASS_FLAG_PSEUDO );
	ASSERT( WinCvtGetClassFileName( Class ) == NULL );

	if (strcmp( szClassName, WINCVT_PSEUDO_RTF_CLASS ) == 0) {
		WINCVT_STATUS ProgressResult;
		HANDLE hFile;
		LPSTR  szBuffer;
		DWORD  ReadLength = (1024 * 1024 * 10);
		DWORD  BytesRead;
		DWORD  TotalBytes;
		DWORD  dwFileOffset = 0; // FIXME - larger?

		szBuffer = WinCvtMalloc(ReadLength);
		if (szBuffer == NULL) {
			return WINCVT_ERROR_NO_MEMORY;
		}

		hFile = CreateFile(szFileName,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hFile == INVALID_HANDLE_VALUE) {
			WinCvtFree( szBuffer );
			return WINCVT_ERROR_OPEN;
		}

		if (hOutput == NULL) {
			hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
			if (hOutput == INVALID_HANDLE_VALUE) {
				WinCvtFree( szBuffer );
				CloseHandle( hFile );
				return WINCVT_ERROR_BAD_PARAM_4;
			}
		}

		TotalBytes = GetFileSize(hFile, NULL);

		while (ReadFile(hFile,
			szBuffer,
			ReadLength,
			&BytesRead,
			NULL) &&
			(BytesRead > 0)) {

			dwFileOffset += BytesRead;

			if (ProgressRoutine) {
				//
				//  If we're doing progress notifications, notify
				//  and prepare to be cancelled.
				//
				ProgressResult = ProgressRoutine( dwFileOffset,
					MulDiv(dwFileOffset, 100, TotalBytes) );

				if (ProgressResult != WINCVT_SUCCESS) {
					WinCvtFree( szBuffer );
					CloseHandle( hFile );
					return WINCVT_ERROR_CANCEL;
				}
			}

			if (!WriteFile(hOutput,
				szBuffer,
				BytesRead,
				&BytesRead,
				NULL)) {

				WinCvtFree( szBuffer );
				CloseHandle( hFile );
				return WINCVT_ERROR_WRITE;
			}
		}

		if (GetLastError() != ERROR_HANDLE_EOF) {
			WinCvtFree( szBuffer );
			CloseHandle( hFile );
			return WINCVT_ERROR_READ;
		}

		WinCvtFree( szBuffer );
		CloseHandle( hFile );
		bHandled = TRUE;
	}

	if (!bHandled) {
		return WINCVT_ERROR_WRONG_TYPE;
	}
	return WINCVT_SUCCESS;
}

WINCVT_STATUS
WinCvtConvertToRtf(
  LPCSTR szFileName,
  LPCSTR szConverterPath,
  LPCSTR szClass,
  HANDLE hOutput,
  _WinCvtProgressCallback ProgressRoutine)
{
	PNONOPAQUE_CVT_LIST Converters = NULL;
	PNONOPAQUE_CVT_CLASS Class = NULL;

	WINCVT_STATUS SavedError = WINCVT_SUCCESS; // If we forget to set it, this will assert
	WINCVT_STATUS TempError;

	BOOL   ConverterInitialized = FALSE;
	BOOL   Retry = FALSE;


	NONOPAQUE_CVT_CONTEXT Context;
	memset(&Context, 0, sizeof(Context));

	APP_ASSERT(szFileName != NULL);
	if (szFileName == NULL) {
		return WINCVT_ERROR_BAD_PARAM_1;
	}

	if (szConverterPath == NULL) {

		SavedError = WinCvtGetImportConverterList(&Converters,
			szClass,    // User specified class, may be null
			szFileName, // Extension to filter
			szFileName);// File to verify against

		if (SavedError != WINCVT_SUCCESS) {
			return SavedError;
		}
		ASSERT(Converters != NULL);

		if (WinCvtGetFirstClass(Converters) == NULL) {
			ASSERT(Converters->NumberOfClasses == 0);

			SavedError = WINCVT_ERROR_NO_CONVERTERS;
			goto abort;
		}

		Class = WinCvtGetFirstClass(Converters);

		szConverterPath = WinCvtGetClassFileName(Class);
		ASSERT(szConverterPath != NULL ||
			(Class->dwFlags & WINCVT_CVT_CLASS_FLAG_PSEUDO));
	}

retry:

	if (Converters && (Class->dwFlags & WINCVT_CVT_CLASS_FLAG_PSEUDO)) {
		//
		//  We were asked to detect converter path, and found a pseudoconverter
		//  (a converter implemented by WinCvt, not an external .cvt file.
		//  For this to happen, we must have a detected class (it would be
		//  impossible to specify the path to a pseudoconverter.)
		//
		SavedError = WinCvtIntPseudoImport(szFileName,
			Class,
			hOutput,
			ProgressRoutine);

	} else {

		SavedError = WinCvtIntInitializeConverter(&Context, szConverterPath);
		ASSERT( Context.dwFileOffset == 0 );
		if (SavedError != WINCVT_SUCCESS) {
			Retry = TRUE;
			goto abort;
		}

		ConverterInitialized = TRUE;
		Context.ProgressRoutine = ProgressRoutine;

		Context.hBuffer = WinCvtGlobalAlloc(GMEM_MOVEABLE, 1024*1024); /* 1Mb */
		if (Context.hBuffer == NULL) {
			SavedError = WINCVT_ERROR_NO_MEMORY;
			goto abort;
		}

		if (hOutput) {
			Context.hFile = hOutput;
		} else {
			Context.hFile = GetStdHandle(STD_OUTPUT_HANDLE);
			if (Context.hFile == INVALID_HANDLE_VALUE) {
				Context.hFile = NULL;
				SavedError = WINCVT_ERROR_BAD_PARAM_4;
				goto abort;
			}
		}

		SavedError = WinCvtIntConvertToRtf(&Context, szFileName);
		if (SavedError != WINCVT_SUCCESS) {
			Retry = TRUE;
			goto abort;
		}
	}

	// Success.  Teardown now.
	if (Converters) {
		SavedError = WinCvtFreeConverterList(Converters);
		ASSERT(SavedError == WINCVT_SUCCESS);
	}

	if (Context.hBuffer != NULL) {
		WinCvtGlobalFree(Context.hBuffer);
		Context.hBuffer = NULL;
	}

	if (ConverterInitialized) {
		Context.ProgressRoutine = NULL;
		WinCvtIntUninitializeConverter(&Context);
	}
	return WINCVT_SUCCESS;

abort:
	ASSERT(SavedError != WINCVT_SUCCESS);
	if (Context.hBuffer) {
		WinCvtGlobalFree(Context.hBuffer);
		Context.hBuffer = NULL;
	}
	if (ConverterInitialized) {
		Context.ProgressRoutine = NULL;
		WinCvtIntUninitializeConverter(&Context);
		ConverterInitialized = FALSE;
	}
	if (Retry && Converters) {
		Retry = FALSE;
		Class=Class->Next;
		if (Class) {
			ASSERT(Class->szFileName!=NULL);
			szConverterPath = Class->szFileName;
			goto retry;
		}
	}
	if (Converters) {
		TempError = WinCvtFreeConverterList(Converters);
		ASSERT(TempError == WINCVT_SUCCESS);
	}
	return SavedError;
}

/*
 * CONVERT TO NATIVE
 *
 * These functions are to support the main plumbing of implementing
 * RTF to native conversion.
 */

#ifdef DEBUG_WINCVT
// 
// Use something absurdly small to test logic.
//
#define WINCVT_EXPORT_MAX_TRANSFER_CHUNK (256)
#else
#define WINCVT_EXPORT_MAX_TRANSFER_CHUNK (10*1024*1024)
#endif

//
// FIXME - We need to support percentage complete notification for
// parity with conversion to RTF.
//
typedef struct _CVT_COMPLETE {
	USHORT StructureSize;
	USHORT StructureVersion;
	USHORT AppPercentComplete;
	USHORT CvtPercentComplete;
} CVT_COMPLETE, *PCVT_COMPLETE;

LONG WINAPI
WriteCvtCallback(LONG Flags, LONG b)
{
	LPSTR szBuffer;

	LONG  err;

	DWORD BytesToRead, BytesRead = 0;
	DWORD FileSize; // FIXME - make this bigger?
	USHORT PercentComplete;

	ASSERT(ActiveConverter != NULL);
	ASSERT(ActiveConverter->hBuffer != NULL);

	//
	// We must have this allocated.  If we haven't got one and
	// the converter asks for it, it hurts.  Plus, if we haven't
	// got one and the caller asks, it's presumably chewing on
	// crap...?
	//
	ASSERT(ActiveConverter->hPercentComplete != NULL);

	//
	// This field is reserved.  We don't know what to do with it.
	//
	ASSERT(b == 0);

	//
	// We better have a source to read data from.
	//
	ASSERT(ActiveConverter->hFile != NULL);

	FileSize = GetFileSize(ActiveConverter->hFile, &BytesRead);
	BytesToRead = FileSize;
	if (BytesToRead == 0xFFFFFFFF || BytesRead != 0) {
		//
		// Either this failed or the file size happens to be really
		// big.  In either case, attempt to read max transfer chunk;
		// failures can happen here because the input stream may
		// not be a file.
		//
		BytesToRead = WINCVT_EXPORT_MAX_TRANSFER_CHUNK;
	}

	if (BytesToRead > WINCVT_EXPORT_MAX_TRANSFER_CHUNK) {
		BytesToRead = WINCVT_EXPORT_MAX_TRANSFER_CHUNK;
	}

	szBuffer = GlobalLock(ActiveConverter->hBuffer);

	// 
	// FIXME? We're reading the file size, not the file position.
	// So, if a file is 20K and we've read 10K, we issue a 20K read.
	// (Capped by WINCVT_EXPORT_MAX_TRANSFER_CHUNK, of course.)
	// This should be harmless, but seems ugly.
	//
	if (!ReadFile(ActiveConverter->hFile, szBuffer, BytesToRead, &BytesRead, NULL)) {
		DWORD GLE;
		GLE = GetLastError();

		if (GLE == ERROR_HANDLE_EOF) {
			//
			// All done!
			//
			GlobalUnlock(ActiveConverter->hBuffer);
			ASSERT( BytesRead == 0 );
			BytesRead = 0;

		} else {
			//
			// We failed.
			//
			GlobalUnlock(ActiveConverter->hBuffer);
			return WINCVT_ERROR_READ;
		}
	}

	ActiveConverter->dwFileOffset += BytesRead;
	PercentComplete = MulDiv(ActiveConverter->dwFileOffset, 100, FileSize);

	//
	// If the caller has asked for percentage complete notifications,
	// do that work now.
	//
	if (ActiveConverter->ProgressRoutine) {
		err = ActiveConverter->ProgressRoutine(BytesRead, PercentComplete);
		if (err != WINCVT_SUCCESS) {
			return err;
		}
	}

	//
	// If the converter has asked for percentage complete notifications,
	// do that work now.
	// 
	if (Flags & 4) {
		PCVT_COMPLETE pCvtPercentComplete;

		pCvtPercentComplete = GlobalLock( ActiveConverter->hPercentComplete );

		pCvtPercentComplete->AppPercentComplete = PercentComplete;

		GlobalUnlock( ActiveConverter->hPercentComplete );
	}

	//
	// If we get success without data, that happily "just works" :)
	//

	return BytesRead;
}

static WINCVT_STATUS
WinCvtIntConvertToNative(
  PNONOPAQUE_CVT_CONTEXT Context,
  LPCSTR szFileName,
  LPCSTR szClass)
{
	HANDLE  hFileName = NULL;
	HANDLE  hClass = NULL;
	HANDLE  hPercentComplete = NULL;
	LPSTR   szTmp, szFileComponent;
	DWORD   FileNameLen, LengthRequired;
	PCVT_COMPLETE pCvtComplete;
	PHANDLE phTmp;

	SHORT   Ret;
	WINCVT_STATUS SavedError = WINCVT_SUCCESS; // If we forget to set it, this will assert

	ASSERT(Context != NULL);

	ASSERT(Context->hConverter != NULL);
	ASSERT(Context->hBuffer != NULL);
	ASSERT(Context->hFile != NULL);
	ASSERT(Context->dwFileOffset == 0);

	//
	// FIXME? - Don't handle strings >4Gb
	//
	FileNameLen = (DWORD)strlen(szFileName) + 1;
	if (FileNameLen<MAX_PATH) {
		//
		// If the filename is smaller than MAX_PATH, we increase the
		// buffer size to MAX_PATH.
		//
		FileNameLen = MAX_PATH;
	}

	hFileName = WinCvtGlobalAlloc(GMEM_MOVEABLE, FileNameLen);
	if (hFileName == NULL) {
		SavedError = WINCVT_ERROR_NO_MEMORY;
		goto abort;
	}
	szTmp = GlobalLock(hFileName);
	//
	// Call GetFullPathName.  The reason for this is that converters seem
	// to have a habit of ignoring the current directory when interpreting
	// relative paths and putting files in the user's home directory
	// instead.  This hack is to maintain consistency - no path == current
	// directory.  So we give the converter a full path instead.
	//
	LengthRequired = GetFullPathName(szFileName,
		FileNameLen,
		szTmp,
		&szFileComponent);

	if (LengthRequired >= FileNameLen) {
		//
		// If it needed more space than we had.  The = is because the
		// return value doesn't include the NULL, but FileNameLen does.
		//
		strcpy(szTmp, szFileName);
	}
	GlobalUnlock(hFileName);

	//
	// FIXME? - Don't handle strings >4Gb
	//
	hClass = WinCvtGlobalAlloc(GMEM_MOVEABLE, (DWORD)strlen(szClass)+1);
	if (hClass == NULL) {
		SavedError = WINCVT_ERROR_NO_MEMORY;
		goto abort;
	}
	szTmp = GlobalLock(hClass);
	strcpy(szTmp, szClass);
	GlobalUnlock(hClass);

	//
	// Percentage complete seems to be a nasty hack.  We have to allocate
	// and initialize memory, and pass it in with the first buffer.  We
	// also keep this around in our own structures (and presumably the
	// converter will too.)  Then we talk based on a shared memory space.
	//
	hPercentComplete = WinCvtGlobalAlloc(GMEM_MOVEABLE, sizeof(CVT_COMPLETE));
	if (hPercentComplete == NULL) {
		SavedError = WINCVT_ERROR_NO_MEMORY;
		goto abort;
	}
	pCvtComplete = GlobalLock(hPercentComplete);
	pCvtComplete->StructureSize = sizeof(CVT_COMPLETE);
	pCvtComplete->StructureVersion = 1;
	pCvtComplete->AppPercentComplete = pCvtComplete->CvtPercentComplete = 0;
	GlobalUnlock(hPercentComplete);

	Context->hPercentComplete = hPercentComplete;
	phTmp = GlobalLock( Context->hBuffer );
	*phTmp = hPercentComplete;
	GlobalUnlock( Context->hBuffer );

	//
	// If the destination file already exists, delete it.  This is because
	// we don't know how converters will handle this case - some may fail.
	// This gives us consistency.  We do it last because we want to minimise
	// any potential data loss.  If it fails, ignore it.
	//
	DeleteFile(szFileName);

	// FIXME - Let's assume we're all single threaded...
	ASSERT(ActiveConverter == NULL);
	ActiveConverter = Context;
	Ret = Context->RtfToForeign(hFileName,
		NULL,
		Context->hBuffer,
		hClass,
		WriteCvtCallback);

	ASSERT(ActiveConverter == Context);
	ActiveConverter = NULL;
	Context->dwFileOffset = 0;
	if (Ret != 0) {
		//
		// If we got this far, the previous file was already junked.
		// Delete anything we just created.
		//
		DeleteFile(szFileName);
		SavedError = Ret;
		goto abort;
	}

	// Success.  Teardown now.
	ASSERT( Context->hPercentComplete == hPercentComplete && hPercentComplete != NULL );

	WinCvtGlobalFree(hFileName);
	WinCvtGlobalFree(hClass);
	WinCvtGlobalFree(hPercentComplete);
	Context->hPercentComplete = NULL;

	return WINCVT_SUCCESS;

abort:
	ASSERT(SavedError != WINCVT_SUCCESS);
	ASSERT( Context->hPercentComplete == NULL || Context->hPercentComplete == hPercentComplete );
	Context->hPercentComplete = NULL;
	if (hPercentComplete) {
		WinCvtGlobalFree(hPercentComplete);
	}
	if (hFileName) {
		WinCvtGlobalFree(hFileName);
	}
	if (hClass) {
		WinCvtGlobalFree(hClass);
	}
	return SavedError;
}

#define AddToPtr(P, O) (((LPSTR)P) + O)

WINCVT_STATUS
WinCvtConvertToNative(
  LPCSTR szFileName,
  LPCSTR szConverterPath,
  LPCSTR szClass,
  HANDLE hInput,
  _WinCvtProgressCallback ProgressRoutine)
{
	WINCVT_STATUS SavedError = WINCVT_SUCCESS; // If we forget to set it, this will assert
	WINCVT_STATUS TempError = WINCVT_SUCCESS;
	PNONOPAQUE_CVT_LIST Converters = NULL;

	LPSTR  szTmp = NULL;

	BOOL   ConverterInitialized = FALSE;

	DWORD  FileSizeHigh;
	DWORD  FileSizeLow;

	NONOPAQUE_CVT_CONTEXT Context;
	memset(&Context, 0, sizeof(Context));

	APP_ASSERT(szFileName != NULL);
	if (szFileName == NULL) {
		return WINCVT_ERROR_BAD_PARAM_1;
	}
	APP_ASSERT(szClass != NULL);
	if (szClass == NULL) {
		return WINCVT_ERROR_BAD_PARAM_3;
	}

	if (szConverterPath == NULL) {
		WINCVT_CVT_CLASS Class;
		SavedError = WinCvtGetExportConverterList(&Converters, szClass, NULL);
		if (SavedError != WINCVT_SUCCESS) {
			return SavedError;
		}
		ASSERT(Converters != NULL);

		Class = WinCvtGetFirstClass(Converters);

		if (Class == NULL) {
			ASSERT(Converters->NumberOfClasses == 0);

			SavedError = WINCVT_ERROR_NO_CONVERTERS;
			goto abort;
		}

		//
		// For export converters, class names should be unique.
		//
		ASSERT(Converters->NumberOfClasses == 1);

		szConverterPath = WinCvtGetClassFileName(Class);
		ASSERT(szConverterPath != NULL);
	}

	if (hInput == NULL) {
		hInput = GetStdHandle(STD_INPUT_HANDLE);
	}

	SavedError = WinCvtIntInitializeConverter(&Context, szConverterPath);
	if (SavedError != WINCVT_SUCCESS) {
		goto abort;
	}

	//
	// Tear this down now.  If we're here we're finished with it, and it's
	// just chewing up memory before we start a memory-intensive operation.
	//
	if (Converters) {
		TempError = WinCvtFreeConverterList(Converters);
		ASSERT(TempError == WINCVT_SUCCESS);
		Converters = NULL;
	}

	ConverterInitialized = TRUE;
	Context.ProgressRoutine = ProgressRoutine;
	Context.hFile = hInput;

	FileSizeLow = GetFileSize(hInput, &FileSizeHigh);
	if ((FileSizeHigh!=0)||(FileSizeLow>WINCVT_EXPORT_MAX_TRANSFER_CHUNK)) {
		//
		// Either the user has asked to convert a really
		// large file, or we're unable to determine the
		// length of the file.  We'll allocate a 10Mb
		// buffer in either case (yes, this is arbitary.)
		// If the file is really large, we'll end up
		// taking multiple goes at it.
		//
		FileSizeLow = WINCVT_EXPORT_MAX_TRANSFER_CHUNK;
		FileSizeHigh = 0;
	}

	Context.hBuffer = WinCvtGlobalAlloc(GMEM_MOVEABLE, FileSizeLow + sizeof(CVT_COMPLETE));

	if (Context.hBuffer == NULL) {
		SavedError = WINCVT_ERROR_NO_MEMORY;
		goto abort;
	}

	SavedError = WinCvtIntConvertToNative(&Context, szFileName, szClass);
	ASSERT( Context.dwFileOffset == 0 );
	if (SavedError != WINCVT_SUCCESS) {
		goto abort;
	}

	// Success.  Teardown now.

	if (Context.hBuffer != NULL) {
		WinCvtGlobalFree(Context.hBuffer);
		Context.hBuffer = NULL;
	}

	if (ConverterInitialized) {
		Context.ProgressRoutine = NULL;
		WinCvtIntUninitializeConverter(&Context);
	}
	return WINCVT_SUCCESS;

abort:
	ASSERT(SavedError != WINCVT_SUCCESS);
	if (Context.hBuffer) {
		WinCvtGlobalFree(Context.hBuffer);
		Context.hBuffer = NULL;
	}
	if (ConverterInitialized) {
		Context.ProgressRoutine = NULL;
		WinCvtIntUninitializeConverter(&Context);
		ConverterInitialized = FALSE;
	}
	if (Converters) {
		TempError = WinCvtFreeConverterList(Converters);
		ASSERT(TempError == WINCVT_SUCCESS);
	}
	return SavedError;
}

WINCVT_STATUS WINAPI
WinCvtConvertFileProgressRoutine(LONG Bytes, LONG Percent)
{
	if (ConvertFileProgress) {
		LONG NewPercent;
		switch (ConvertFilePhase) {
			case 0:
				NewPercent = Percent;
				break;
			case 1:
				NewPercent = Percent / 2;
				break;
			case 2:
				NewPercent = Percent / 2 + 50;
				break;
			default:
				ASSERT( FALSE );
		}
		ASSERT( NewPercent >= 0 );
		ASSERT( NewPercent <= 100 );
		return ConvertFileProgress(Bytes, NewPercent);
	}
	return WINCVT_SUCCESS;
}

WINCVT_STATUS
WinCvtConvertFile(
  LPCSTR szSourceFile,
  LPCSTR szSourceClass,
  LPCSTR szDestFile,
  LPCSTR szDestClass,
  _WinCvtProgressCallback ProgressRoutine)
{
	HANDLE hDestFile = NULL;
	WINCVT_STATUS Status;
	CHAR szTempFile[256];
	LPCSTR szImportResultFile;

	APP_ASSERT(szSourceFile != NULL);
	if (szSourceFile == NULL) {
		return WINCVT_ERROR_BAD_PARAM_1;
	}

	if (szDestClass != NULL) {
		char szFileName[40];
		APP_ASSERT(szDestFile != NULL);
		if (szDestFile == NULL) {
			return WINCVT_ERROR_BAD_PARAM_3;
		}
		//
		// If we're performing export conversion, we need to import
		// to a temporary file, then export to the destination.
		//
		GetTempPath(sizeof(szTempFile)-20, szTempFile);

		sprintf(szFileName, "cvt%08x.rtf", GetCurrentProcessId());
		strcat(szTempFile, szFileName);
		szImportResultFile = szTempFile;
	} else {
		szImportResultFile = szDestFile;
	}

	if (szImportResultFile) {
		//
		// Regardless of whether we're importing to RTF or importing
		// and re-exporting, this file will hold RTF.  If we're re-
		// exporting, it's temporary.
		//
		// Note that we're attempting to overwrite the file if it
		// already exists.
		//

		hDestFile = CreateFile(szImportResultFile,
			GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL|(szDestClass?FILE_FLAG_DELETE_ON_CLOSE:0),
			NULL);

		if (hDestFile == INVALID_HANDLE_VALUE) {
			Status = WINCVT_ERROR_OPEN;
			goto abort;
		}
	}

	// FIXME - Single threaded only
	ASSERT( ConvertFileProgress == NULL );
	ConvertFileProgress = ProgressRoutine;

	ASSERT( ConvertFilePhase == 0 );
	if (szDestClass != NULL) {
		//
		// We're going to do export, so the import is half way.
		//
		ConvertFilePhase = 1;
	}

	//
	// Perform native to RTF conversion.  We need to do this in all cases.
	//
	Status = WinCvtConvertToRtf(szSourceFile,
		NULL,
		szSourceClass,
		hDestFile,
		WinCvtConvertFileProgressRoutine);

	if (Status != WINCVT_SUCCESS) {
		goto abort;
	}

	if (szDestClass != NULL) {
		ASSERT( ConvertFilePhase == 1 );
		ConvertFilePhase = 2;
		//
		// We're re-exporting to a native format.  Go back to the beginning
		// of the temporary file, and convert again.
		//
		SetFilePointer(hDestFile, 0, 0, FILE_BEGIN);

		ASSERT(szDestFile != NULL);
		ASSERT(szDestClass != NULL);

		Status = WinCvtConvertToNative(szDestFile,
			NULL,
			szDestClass,
			hDestFile,
			WinCvtConvertFileProgressRoutine);

		if (Status != WINCVT_SUCCESS) {
			goto abort;
		}
	}

abort:
	ConvertFileProgress = NULL;
	ConvertFilePhase = 0;

	if (hDestFile) {
		CloseHandle(hDestFile);
	}

	return Status;
}


