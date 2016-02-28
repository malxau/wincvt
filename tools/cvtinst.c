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

LPCSTR szConverter = NULL;
BOOL   bUserOnly = FALSE;
BOOL   bUninstall = FALSE;

static void usage(LPCSTR szAppName)
{
	fprintf(stderr, "Usage: %s [-e] [-u] converter\n\nInstall or uninstall a converter into the system.\n", szAppName);
	exit(EXIT_FAILURE);
}

void ParseParameters(int argc, char * argv[])
{
	BOOL bMatched = FALSE;
	int index = 1;
	while (argc > index) {
		bMatched = FALSE;
		if (strcmp(argv[index], "-e") == 0) {
			if (bUninstall) usage(argv[0]);
			bUninstall = TRUE;
			bMatched = TRUE;
		}
		if (strcmp(argv[index], "-u") == 0) {
			if (bUserOnly) usage(argv[0]);
			bUserOnly = TRUE;
			bMatched = TRUE;
		}
		if (strcmp(argv[index], "-?") == 0 ||
			strcmp(argv[index], "/?") == 0) {
			usage( argv[0] );
		}
		if (!bMatched) {
			if (szConverter == NULL) {
				szConverter = argv[index];
			} else usage(argv[0]);
		}
		index++;
	}
	if (szConverter == NULL) usage(argv[0]);
}



int main(int argc, char * argv[])
{
	BOOL Success = FALSE;
	WINCVT_STATUS Status;
	CHAR szErr[256];

	if (!WinCvtIsLibraryCompatible()) {
		fprintf(stderr, "This application is not compatible with this version of WinCvt.dll.\n");
		return EXIT_FAILURE;
	}

	ParseParameters(argc, argv);

	if (bUninstall) {
		Status = WinCvtUninstallConverter(szConverter, bUserOnly);
	} else {
		Status = WinCvtInstallConverter(szConverter, bUserOnly);
	}
	if (Status != WINCVT_SUCCESS) {
		WinCvtGetErrorString(Status, szErr, sizeof(szErr));
		printf("Cannot install converter - %s (%i)\n", szErr, Status);
		goto abort;
	}

	Success = TRUE;

abort:

	WinCvtFinalTeardown();

	return Success?EXIT_SUCCESS:EXIT_FAILURE;
}
