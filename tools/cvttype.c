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

BOOL bQuick = FALSE;
LPCSTR szSourceFile = NULL;

static void usage(LPCSTR szAppName)
{
	fprintf(stderr, "Usage: %s [-q] file\n\nDisplay a list of converters suitable for this document.\n -q - Quick (don't verify converters, trust registry.)", szAppName);
	exit(EXIT_FAILURE);
}

void ParseParameters(int argc, char * argv[])
{
	BOOL bMatched = FALSE;
	int index = 1;
	while (argc > index) {
		bMatched = FALSE;
		if (strcmp(argv[index], "-q") == 0) {
			bMatched = TRUE;
			bQuick = TRUE;
		}
		if (strcmp(argv[index], "-?") == 0 ||
			strcmp(argv[index], "/?") == 0) {
			usage( argv[0] );
		}
		if (!bMatched) {
			if (szSourceFile == NULL) {
				szSourceFile = argv[index];
			} else usage(argv[0]);
		}
		index++;
	}
	if (szSourceFile == NULL) usage(argv[0]);
}



int main(int argc, char * argv[])
{
	WINCVT_CVT_LIST  Output = NULL;
	WINCVT_CVT_CLASS CvtClass = NULL;
	BOOL Success = FALSE;
	WINCVT_STATUS Status;
	CHAR szErr[256];

	if (!WinCvtIsLibraryCompatible()) {
		fprintf(stderr, "This application is not compatible with this version of WinCvt.dll.\n");
		return EXIT_FAILURE;
	}

	ParseParameters(argc, argv);

	Status = WinCvtGetImportConverterList(&Output, NULL, szSourceFile, bQuick?NULL:szSourceFile);
	if (Status != WINCVT_SUCCESS) {
		WinCvtGetErrorString(Status, szErr, sizeof(szErr));
		printf("Cannot fetch converter list - %s (%i)\n", szErr, Status);
		goto abort;
	}
	CvtClass = WinCvtGetFirstClass(Output);
	while(CvtClass != NULL) {
		printf("%-6s %-16s %s %s\n",
				WinCvtGetClassExtensions(CvtClass),
				WinCvtGetClassName(CvtClass),
				WinCvtGetClassDescription(CvtClass),
				WinCvtGetClassFileName(CvtClass));
		CvtClass = WinCvtGetNextClass(CvtClass);
	}

	Success = TRUE;

abort:
	if (Output)
		WinCvtFreeConverterList(Output);

	WinCvtFinalTeardown();

	return Success?EXIT_SUCCESS:EXIT_FAILURE;
}
