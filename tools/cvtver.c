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

LPCSTR szConverter = NULL;

static void usage(LPCSTR szAppName)
{
	fprintf(stderr, "Usage: %s\n", szAppName);
	exit(EXIT_FAILURE);
}

int main(int argc, char * argv[])
{
	BOOL Success = FALSE;
	DWORD Version;

	if (argc != 1) usage(argv[0]);

	//
	// Note - this app does not sanity check versions before running.  The following APIs are
	// there from the beginning, and unlikely to change.  Thus, if we got far enough to run
	// (no unresolved externals), then this is probably right.
	//

	Version = WinCvtLibraryVersion();
	printf("Running on %i.%i.%i\n",
		WINCVT_GET_MAJOR_VERSION(Version),
		WINCVT_GET_MINOR_VERSION(Version),
		WINCVT_GET_MICRO_VERSION(Version));

	Version = WinCvtHeaderVersion();
	printf("Compiled against %i.%i.%i\n",
		WINCVT_GET_MAJOR_VERSION(Version),
		WINCVT_GET_MINOR_VERSION(Version),
		WINCVT_GET_MICRO_VERSION(Version));

	WinCvtFinalTeardown();

	return EXIT_SUCCESS;
}
