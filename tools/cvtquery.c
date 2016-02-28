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

LPCSTR szConverterName  = NULL;
LPCSTR szConverterClass = NULL;
BOOL   bExport = FALSE;

static void usage(LPCSTR szAppName)
{
	fprintf(stderr, "Usage: %s [-e] [-c ConverterClass]|[ConverterPath]\n\nLists all conversions supported by the system or a particular converter\n -e Lists export conversions (RTF to native) instead of import converstions", szAppName);
	exit(EXIT_FAILURE);
}

void ParseParameters(int argc, char * argv[])
{
	BOOL bMatched = FALSE;
	int index = 1;
	while (argc > index) {
		bMatched = FALSE;
		if (strcmp(argv[index], "-e") == 0) {
			bMatched = TRUE;
			if (!bExport) 
				bExport = TRUE;
			else
				usage(argv[0]);
		}
		if (strcmp(argv[index], "-c") == 0) {
			bMatched = TRUE;
			if (szConverterName != NULL) usage(argv[0]);
			szConverterClass = argv[index+1];
			index++;
			if (szConverterClass == NULL) usage(argv[0]);
		}
		if (strcmp(argv[index], "-?") == 0 ||
			strcmp(argv[index], "/?") == 0) {
			usage( argv[0] );
		}

		if (!bMatched) {
			if (szConverterName == NULL && szConverterClass == NULL) {
				szConverterName = argv[index];
			} else usage(argv[0]);
		}
		index++;
	}
}

int main(int argc, char * argv[])
{
	WINCVT_CVT_LIST  Output = NULL;
	WINCVT_CVT_CLASS CvtClass = NULL;
	WINCVT_STATUS    Status;
	BOOL Success = FALSE;
	BOOL Initialized = FALSE;
	CHAR szErr[256];

	if (!WinCvtIsLibraryCompatible()) {
		fprintf(stderr, "This application is not compatible with this version of WinCvt.dll.\n");
		return EXIT_FAILURE;
	}

	ParseParameters(argc, argv);

	if (szConverterName) {
		if (bExport) {
			Status = WinCvtGetConverterExportCapabilityList(szConverterName, &Output);
		} else {
			Status = WinCvtGetConverterImportCapabilityList(szConverterName, &Output);
		}
		if (Status != WINCVT_SUCCESS) {
			WinCvtGetErrorString(Status, szErr, sizeof(szErr));
			fprintf(stderr, "Cannot fetch converter capabilities - %s (%i)\n", szErr, Status);
			goto abort;
		}
	} else {
		if (bExport) {
			Status = WinCvtGetExportConverterList(&Output, szConverterClass, NULL);
		} else {
			Status = WinCvtGetImportConverterList(&Output, szConverterClass, NULL, NULL);
		}
		if (Status != WINCVT_SUCCESS) {
			WinCvtGetErrorString(Status, szErr, sizeof(szErr));
			if (szConverterClass == NULL) {
				fprintf(stderr, "Cannot fetch converter list - %s (%i)\n", szErr, Status);
			} else {
				fprintf(stderr, "Cannot fetch converter class - %s (%i)\n", szErr, Status);
			}
			goto abort;
		}
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
