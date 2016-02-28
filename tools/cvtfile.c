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
#include <stdio.h>
#include <stdlib.h>

#ifdef MINICRT
#include <minicrt.h>
#endif

#include "wincvt.h"
#include "wcassert.h"

LPCSTR szSourceFile = NULL;
LPCSTR szDestFile   = NULL;
LPCSTR szSrcClass   = NULL;
LPCSTR szDestClass  = NULL;
BOOL   bProgress    = FALSE;

static void usage(LPCSTR AppName)
{
	printf("%s: [-c ImportClass] [-e ExportClass] [-p] SourceFile DestFile\n\nConvert a file from one type to another\n", AppName);
	exit(EXIT_FAILURE);
}

void ParseParameters(int argc, char * argv[])
{
	BOOL bMatched = FALSE;
	int index = 1;
	while (argc > index) {
		bMatched = FALSE;
		if (strcmp(argv[index], "-c") == 0) {
			bMatched = TRUE;
			szSrcClass = argv[index+1];
			if (szSrcClass == NULL) usage(argv[0]);
			index++;
		}
		if (strcmp(argv[index], "-e") == 0) {
			bMatched = TRUE;
			if (szDestClass!=NULL) usage(argv[0]);
			if (argv[index+1] == NULL) usage(argv[0]);
			szDestClass = argv[index+1];
			index++;
		}
		if (strcmp(argv[index], "-p") == 0) {
			bMatched = TRUE;
			bProgress = TRUE;
		}
		if (strcmp(argv[index], "-?") == 0 ||
			strcmp(argv[index], "/?") == 0) {
			usage( argv[0] );
		}
		if (!bMatched) {
			if (szSourceFile == NULL) {
				szSourceFile = argv[index];
			} else if (szDestFile == NULL) {
				szDestFile = argv[index];
			} else usage(argv[0]);
		}
		index++;
	}
	if (szSourceFile == NULL) usage(argv[0]);
	if (szDestClass != NULL && szDestFile == NULL) usage(argv[0]);
	if (bProgress && szDestFile == NULL) usage(argv[0]);
}

WINCVT_STATUS WINAPI
Progress(LONG Bytes, LONG Percent)
{
	if (bProgress) {
		printf("%i%% percent complete\n", Percent);
	}
	return WINCVT_SUCCESS;
}

int main(int argc, char * argv[])
{
	WINCVT_STATUS Status;
	CHAR szErr[256];

	if (!WinCvtIsLibraryCompatible()) {
		fprintf(stderr, "This application is not compatible with this version of WinCvt.dll.\n");
		return EXIT_FAILURE;
	}

	ParseParameters(argc, argv);

	Status = WinCvtConvertFile(szSourceFile,
		szSrcClass,
		szDestFile,
		szDestClass,
		Progress);

	WinCvtGetErrorString(Status, szErr, sizeof(szErr));
	if (Status != WINCVT_SUCCESS) {
		fprintf(stderr, "Could not convert file - %s (%i)\n", szErr, Status);
	} else {
		if (szDestFile != NULL) {
			// 
			// Give a success message if we're not outputting to
			// stdout.
			//
			fprintf(stderr, "%s (%i)\n", szErr, Status);
		}
	}

	WinCvtFinalTeardown();

	return (Status==WINCVT_SUCCESS)?EXIT_SUCCESS:EXIT_FAILURE;
}

