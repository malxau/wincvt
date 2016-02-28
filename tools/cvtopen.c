/* WINCVT
 * =======
 * This software is Copyright (c) 2006-14 Malcolm Smith.
 * No warranty is provided, including but not limited to
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * This code is licenced subject to the GNU General
 * Public Licence (GPL).  See the COPYING file for more.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef MINICRT
#include <minicrt.h>
#endif

#include "wincvt.h"
#include "wcassert.h"

LPCSTR szSourceFile  = NULL;
LPCSTR szApplication = NULL;

static void usage(LPCSTR AppName)
{
	printf("%s: File [Application]\n\nOpen or edit a file with an editor\n",
		AppName);
	exit(EXIT_FAILURE);
}

void ParseParameters(int argc, char * argv[])
{
	BOOL bMatched = FALSE;
	int index = 1;
	while (argc > index) {
		if (strcmp(argv[index], "-?") == 0 ||
			strcmp(argv[index], "/?") == 0) {
			usage( argv[0] );
		}
		bMatched = FALSE;
		if (!bMatched) {
			if (szSourceFile == NULL) {
				szSourceFile = argv[index];
			} else if (szApplication == NULL) {
				szApplication = argv[index];
			} else usage(argv[0]);
		}
		index++;
	}
	if (szSourceFile == NULL) usage(argv[0]);
}

typedef struct _CVTOPEN_FILE_VERIFY_DATA {
	LONGLONG llModifyTime;
	DWORD dwFileSizeLow;
} CVTOPEN_FILE_VERIFY_DATA, *PCVTOPEN_FILE_VERIFY_DATA;

BOOL CollectFileVerifyData( HANDLE hFile, PCVTOPEN_FILE_VERIFY_DATA FileData )
{
	BY_HANDLE_FILE_INFORMATION fileinfo;

	if (!GetFileInformationByHandle( hFile, &fileinfo )) {
		FileData->llModifyTime = 0;
		FileData->dwFileSizeLow = 0;
		return FALSE;
	}

	FileData->llModifyTime = ((LARGE_INTEGER *)&fileinfo.ftLastWriteTime)->QuadPart;
	FileData->dwFileSizeLow = fileinfo.nFileSizeLow;
	return TRUE;
}

BOOL VerifyFiles( PCVTOPEN_FILE_VERIFY_DATA File1, PCVTOPEN_FILE_VERIFY_DATA File2 )
{
	printf("%016llx %08x\n",
			File1->llModifyTime,
			File1->dwFileSizeLow );

	printf("%016llx %08x\n",
			File2->llModifyTime,
			File2->dwFileSizeLow );
	
	if (File1->llModifyTime <= File2->llModifyTime)
		return FALSE;

	if (File1->dwFileSizeLow != File2->dwFileSizeLow)
		return FALSE;
	return TRUE;
}

int main(int argc, char * argv[])
{
	WINCVT_STATUS Status;
	CHAR szErr[256];
	CHAR szTempFile[256];
	CHAR szFileName[20];
	SHELLEXECUTEINFO ExecInfo;
	HANDLE hTemp;
	DWORD ret;
	BOOL Success = TRUE;
	BOOL bTemp;

	CVTOPEN_FILE_VERIFY_DATA FileWeWrote;
	CVTOPEN_FILE_VERIFY_DATA FileAfterAppClosed;

	WINCVT_CVT_LIST ImportConverters = NULL;
	WINCVT_CVT_CLASS ImportConverter;

	if (!WinCvtIsLibraryCompatible()) {
		fprintf(stderr, "This application is not compatible with this version of WinCvt.dll.\n");
		Success = FALSE;
		goto abort;
	}

	ParseParameters(argc, argv);

	Status = WinCvtGetImportConverterList( &ImportConverters,
		NULL,
		szSourceFile,
		szSourceFile);

	if (Status != WINCVT_SUCCESS) {
		WinCvtGetErrorString(Status, szErr, sizeof(szErr));
		fprintf(stderr,
			"Could not fetch converter for %s\n%s (%i)\n",
			szSourceFile,
			szErr,
			Status);
		Success = FALSE;
		goto abort;
	}

	ImportConverter = WinCvtGetFirstClass( ImportConverters );

	if (ImportConverter == NULL) {
		fprintf(stderr,
			"Could not fetch converter for %s\n",
			szSourceFile);
		Success = FALSE;
		goto abort;
	}

	GetTempPath(sizeof(szTempFile) - sizeof(szFileName), szTempFile);

	sprintf(szFileName, "cvt%08x.rtf", GetCurrentProcessId());
	strcat(szTempFile, szFileName);

	hTemp = CreateFile( szTempFile,
			GENERIC_WRITE,
			0,
			NULL,
			CREATE_NEW,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

	if (hTemp == INVALID_HANDLE_VALUE) {
		fprintf(stderr,
			"Could not create temporary file %s\n",
			szTempFile);
		Success = FALSE;
		goto abort;
	}

	Status = WinCvtConvertToRtf(szSourceFile,
		WinCvtGetClassFileName( ImportConverter ),
		WinCvtGetClassName( ImportConverter ),
		hTemp,
		NULL);

	bTemp = CollectFileVerifyData( hTemp, &FileWeWrote );
	ASSERT( bTemp );
	//
	// Add a one second fudge factor.  This happens because of filetime
	// granularity issues.
	//
	FileWeWrote.llModifyTime += 1*1000*1000*10;

	CloseHandle(hTemp);

	if (Status != WINCVT_SUCCESS) {
		WinCvtGetErrorString(Status, szErr, sizeof(szErr));
		fprintf(stderr,
			"Could not convert %s to temporary file %s\n%s (%i)\n",
			szSourceFile,
			szTempFile,
			szErr,
			Status);
		Success = FALSE;
		goto abort;
	}

	printf("Created %s from %s\n", szTempFile, WinCvtGetClassDescription(ImportConverter));

	memset(&ExecInfo, 0, sizeof(ExecInfo));
	ExecInfo.cbSize = sizeof(ExecInfo);
	ExecInfo.nShow = SW_SHOWDEFAULT;
	if (szApplication != NULL) {
		ExecInfo.lpFile = szApplication;
		ExecInfo.lpParameters = szTempFile;
	} else {
		ExecInfo.lpFile = szTempFile;
	}
	ExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;

	if (!ShellExecuteEx( &ExecInfo )) {

		fprintf(stderr,
			"Could not launch %s\n",
			szApplication?szApplication:szTempFile);
		DeleteFile(szTempFile);
		Success = FALSE;
		goto abort;

	}

	ret = WaitForSingleObject( ExecInfo.hProcess, INFINITE );
	ASSERT( ret == WAIT_OBJECT_0 );
	printf("Application terminated\n");

	ret = CloseHandle( ExecInfo.hProcess );
	ASSERT( ret );

	hTemp = CreateFile( szTempFile,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL );

	if (hTemp == INVALID_HANDLE_VALUE) {
		fprintf(stderr,
			"Could not open temporary file %s\n",
			szTempFile);
		DeleteFile( szTempFile );
		Success = FALSE;
		goto abort;
	}

	bTemp = CollectFileVerifyData( hTemp, &FileAfterAppClosed );
	ASSERT( bTemp );

	bTemp = VerifyFiles( &FileWeWrote, &FileAfterAppClosed );
	
	if (!bTemp) {

		printf("Files have been changed\n");

		Status = WinCvtConvertToNative(szSourceFile,
			WinCvtGetClassFileName( ImportConverter ),
			WinCvtGetClassName( ImportConverter ),
			hTemp,
			NULL);

		if (Status != WINCVT_SUCCESS) {
			WinCvtGetErrorString( Status, szErr, sizeof(szErr) );
			fprintf(stderr,
				"Could not convert %s to %s\n%s (%i)\nLeaving temporary file around for manual recovery.",
				szTempFile,
				szSourceFile,
				szErr,
				Status);
			Success = FALSE;
			goto abort;
		} else {

			printf("Converted file %s successfully.\n", szSourceFile);
		}


	} else {

		printf("Files have not been changed\n");

	}

	CloseHandle( hTemp );

	DeleteFile(szTempFile);

abort:
	ASSERT( (!Success) || (Status == WINCVT_SUCCESS) );

	if (ImportConverters) {
		WinCvtFreeConverterList( ImportConverters );
	}
	
	WinCvtFinalTeardown();

	return (Success)?EXIT_SUCCESS:EXIT_FAILURE;
}

