/* WINCVT
 * =======
 * This software is Copyright (c) 2006-14 Malcolm Smith.
 * No warranty is provided, including but not limited to
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * This code is licenced subject to the GNU General
 * Public Licence (GPL).  See the COPYING file for more.
 */

#define WINVER 0x400
#define _WIN32_IE 0x200

#include <windows.h>
#include <commctrl.h>
#include <prsht.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef MINICRT
#include <minicrt.h>
#endif

#include "wincvt.h"
#include "wizdlg.h"
#include "wcassert.h"

HINSTANCE hInst;
WINCVT_CVT_LIST AllImportConverters = NULL;
WINCVT_CVT_LIST AllExportConverters = NULL;
WINCVT_CVT_LIST MatchingImportConverters = NULL;

WINCVT_CVT_CLASS ImportClass = NULL;
WINCVT_CVT_CLASS ExportClass = NULL;

BOOLEAN RedrawImportList = FALSE;

CHAR szSourceFile[MAX_PATH];
CHAR szDestFile[MAX_PATH];

// Compatibility hack: compile with MSVC5/4 headers
#ifndef OFN_ENABLESIZING
#define OFN_ENABLESIZING 0x00800000
#endif

#ifndef _WIN64
#ifndef DWLP_MSGRESULT
#define DWLP_MSGRESULT DWL_MSGRESULT
#endif
#endif

#define product "WinCvt Converter Wizard"

VOID UpdateOutputFileData()
{
	LPSTR a;
	strcpy(szDestFile, szSourceFile);
	a=strrchr(szDestFile, '.');
	if (a) {
		a++;
		*a='\0';
	}

	if (ExportClass) {
		LPCSTR szExtensions;

		szExtensions = WinCvtGetClassExtensions(ExportClass);
		a = strchr(szExtensions,' ');
		if (a) {
			//
			// There are multiple extensions.  We pick the first
			// one by default.
			//
			strncat(szDestFile, szExtensions, a-szExtensions);
		} else {
			strcat(szDestFile, szExtensions);
		}
		
	} else {
		strcat(szDestFile, "rtf");
	}
}

VOID UpdateInputFileData()
{
	if (MatchingImportConverters != NULL) {
		WinCvtFreeConverterList(MatchingImportConverters);
		MatchingImportConverters = NULL;
	}
	// If we can't load anything here, this is left NULL.
	// That's okay, we'll handle it.
	WinCvtGetImportConverterList(&MatchingImportConverters,
		NULL,
		szSourceFile,
		szSourceFile);

	RedrawImportList = TRUE;

	UpdateOutputFileData();
}


BOOL CALLBACK
Page1Dlg(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HANDLE hSourceFile;
	OPENFILENAME ofn;
	char szTemp[MAX_PATH];

	switch(Msg) {
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_BROWSE:
					memset(&ofn, 0, sizeof(ofn));
					strcpy(szSourceFile, "");

					GetDlgItemText(hWnd,
						IDC_FILENAME,
						szSourceFile,
						sizeof(szSourceFile));

					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hWnd;
					ofn.hInstance = hInst;
					ofn.lpstrFilter = "All Files (*.*)\0*.*\0\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFile = szSourceFile;
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_ENABLESIZING |
						OFN_EXPLORER |
						OFN_FILEMUSTEXIST |
						OFN_HIDEREADONLY |
						OFN_PATHMUSTEXIST;

					if (!GetOpenFileName(&ofn)) {
						DWORD Err;

						Err = CommDlgExtendedError();

						ASSERT(Err == 0);
					} else {
						SetDlgItemText(hWnd,
							IDC_FILENAME,
							szSourceFile);

						UpdateInputFileData();
					}
					break;
				default:
					break;
			}
			return TRUE;
			break;
		case WM_INITDIALOG:

			//
			// If we're loading the dialog and we have a source file
			// already, it must have been user specified on the
			// command line.  Load that into the user field.
			//
			if (szSourceFile[0] != '\0') {
				SetDlgItemText(hWnd,
					IDC_FILENAME,
					szSourceFile);

				UpdateInputFileData();

			}
			return TRUE;
			break;
		case WM_NOTIFY:
			switch(((NMHDR FAR *)lParam)->code) {
				/* Set buttons when this page is activated; remember, it could
 				 * be activated in (nearly) any order */
				case PSN_SETACTIVE:
					PropSheet_SetWizButtons(GetParent(hWnd),PSWIZB_NEXT);
					break;
				case PSN_WIZNEXT:

					GetDlgItemText(hWnd,
						IDC_FILENAME,
						szTemp,
						sizeof(szTemp));

					if (strcmp(szTemp, szSourceFile)!=0) {
						// If the file name changes, redraw the list
						strcpy(szSourceFile, szTemp);
						UpdateInputFileData();
					}
					if (strcmp(szSourceFile, "")==0) {

						MessageBox(hWnd,
							"Please select a file to convert.",
							product,
							48);

						SetWindowLong(hWnd,
							DWLP_MSGRESULT,
							TRUE);
						return TRUE;
					}
					hSourceFile = CreateFile(szSourceFile,
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);
					if (hSourceFile == INVALID_HANDLE_VALUE) {
						// FIXME use FormatMessage for this
						MessageBox(hWnd,
							"Cannot open the file to convert.",
							NULL,
							48);
						SetWindowLong(hWnd,
							DWLP_MSGRESULT,
							TRUE);
						return TRUE;
					}
					CloseHandle(hSourceFile);

					PropSheet_SetWizButtons(GetParent(hWnd),
						PSWIZB_BACK|PSWIZB_NEXT);
					break;
			}
			return TRUE;
			break;
		default:
			return FALSE;
	}
	return FALSE;
}

BOOL CALLBACK
Page2Dlg(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{

	WINCVT_STATUS Status;
	WINCVT_CVT_CLASS Class;
	WINCVT_CVT_CLASS LookFor;
	BOOLEAN Match = FALSE;
	int i, selected;

	switch(Msg) {
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_BROWSE:
					break;
				default:
					break;
			}
			return TRUE;
			break;

		case WM_INITDIALOG:
			SendDlgItemMessage(hWnd,
				IDC_FILEDESTTYPE,
				LB_ADDSTRING,
				0,
				(LPARAM)"RTF");

			SendDlgItemMessage(hWnd,
				IDC_FILEDESTTYPE,
				LB_SETCURSEL,
				0,
				0);

			SendDlgItemMessage(hWnd,
				IDC_FILEDESTTYPE,
				WM_ENABLE,
				FALSE,
				0);

			Status = WinCvtGetImportConverterList(&AllImportConverters,
				NULL,
				NULL,
				NULL);

			if (Status == WINCVT_SUCCESS) {
				Class = WinCvtGetFirstClass(AllImportConverters);

				while (Class != NULL) {
					SendDlgItemMessage(hWnd,
						IDC_FILESRCTYPE,
						LB_ADDSTRING,
						0,
						(LPARAM)WinCvtGetClassDescription(Class));

					Class = WinCvtGetNextClass(Class);
				}
			}

			Status = WinCvtGetExportConverterList(&AllExportConverters,
				NULL,
				NULL);

			if (Status == WINCVT_SUCCESS) {
				Class = WinCvtGetFirstClass(AllExportConverters);

				while (Class != NULL) {
					SendDlgItemMessage(hWnd,
						IDC_FILEDESTTYPE,
						LB_ADDSTRING,
						0,
						(LPARAM)WinCvtGetClassDescription(Class));

					Class = WinCvtGetNextClass(Class);
				}
			}

			return TRUE;
			break;

		case WM_NOTIFY:
			switch(((NMHDR FAR *)lParam)->code) {
				/* Set buttons when this page is activated; remember, it could
 				 * be activated in (nearly) any order */
				case PSN_SETACTIVE:

					if (RedrawImportList && MatchingImportConverters != NULL) {
						LookFor = WinCvtGetFirstClass(MatchingImportConverters);

						while (LookFor != NULL && AllImportConverters != NULL) {
							Class = WinCvtGetFirstClass(AllImportConverters);
							i = 0;

							while (Class != NULL) {
								ASSERT(WinCvtGetClassName(Class) != NULL);
								ASSERT(WinCvtGetClassName(LookFor) != NULL);
								if (strcmp(WinCvtGetClassName(Class),WinCvtGetClassName(LookFor))==0) {
									Match = TRUE;
									break;
								}
								Class = WinCvtGetNextClass(Class);
								i++;
							}
							if (Match) break;

							LookFor = WinCvtGetNextClass(LookFor);
						}

						if (Match) {
							SendDlgItemMessage(hWnd,
								IDC_FILESRCTYPE,
								LB_SETCURSEL,
								i,
								0);
						}
						RedrawImportList = FALSE;
					}
					
					PropSheet_SetWizButtons(GetParent(hWnd),
						PSWIZB_BACK|PSWIZB_NEXT);
					break;
				case PSN_WIZNEXT:
					//
					//  First pass: find the import converter.
					//

					//
					// FIXME? - Don't handle >2bill items
					//
					selected = (int)SendDlgItemMessage(hWnd,
						IDC_FILESRCTYPE,
						LB_GETCURSEL,
						0,
						0);

					if (selected < 0) {
						MessageBox(hWnd,
							"Please select a file type to convert from.",
							product,
							0);

						SetWindowLong(hWnd,
							DWLP_MSGRESULT,
							TRUE);
						return TRUE;
					}

					Class = WinCvtGetFirstClass(AllImportConverters);
					i = 0;

					while (Class != NULL && i<selected) {
						Class = WinCvtGetNextClass(Class);
						i++;
					}

					if (Class && i==selected) {
						ImportClass = Class;
					} else {
						// The user selected something we don't have...?
						ASSERT(FALSE);
					}

					//
					//  Second pass: locate the export converter
					//

					//
					// FIXME? - Don't handle >2bill items
					//
					selected = (int)SendDlgItemMessage(hWnd,
						IDC_FILEDESTTYPE,
						LB_GETCURSEL,
						0,
						0);

					if (selected < 0) {
						MessageBox(hWnd,
							"Please select a file type to convert to.",
							product,
							0);

						SetWindowLong(hWnd,
							DWLP_MSGRESULT,
							TRUE);
						return TRUE;
					}

					LookFor = ExportClass;
					Class = WinCvtGetFirstClass(AllExportConverters);
					//
					// If selected == 0, we're exporting to RTF.
					// In that case, we set ExportClass to NULL.
					// We also start counting from 1 in this pass.
					//
					ExportClass = NULL;
					i = 1;

					while (Class != NULL && i<selected) {
						Class = WinCvtGetNextClass(Class);
						i++;
					}

					if (Class && i==selected) {
						ExportClass = Class;
					}

					if (ExportClass != LookFor) {
						// 
						// Our export class has changed, so
						// update destination filename.
						//
						UpdateOutputFileData();
					}
					
					PropSheet_SetWizButtons(GetParent(hWnd),
						PSWIZB_BACK|PSWIZB_NEXT);
					break;
			}
			return TRUE;
			break;
		default:
			return FALSE;
	}
	return FALSE;
}

HWND hWndProgress = NULL;

WINCVT_STATUS WINAPI
ProgressCallback(LONG Bytes, LONG Percent)
{
	if (hWndProgress) {

		SendDlgItemMessage(hWndProgress,
			IDC_PERCENT,
			PBM_SETPOS,
			Percent,
			0);

	}
	return WINCVT_SUCCESS;
}

BOOL CALLBACK
Page3Dlg(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	OPENFILENAME ofn;
	WINCVT_STATUS Status;
	switch(Msg) {
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_BROWSE:
					memset(&ofn, 0, sizeof(ofn));
					GetDlgItemText(hWnd,
						IDC_FILENAME,
						szDestFile,
						sizeof(szDestFile));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hWnd;
					ofn.hInstance = hInst;
					ofn.lpstrFilter = "All Files (*.*)\0*.*\0\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFile = szDestFile;
					ofn.nMaxFile = MAX_PATH;

					ofn.Flags = OFN_ENABLESIZING |
						OFN_EXPLORER |
						OFN_FILEMUSTEXIST |
						OFN_HIDEREADONLY |
						OFN_PATHMUSTEXIST;

					if (!GetSaveFileName(&ofn)) {
						DWORD Err;

						Err = CommDlgExtendedError();

						ASSERT(Err == 0);
					} else {
						SetDlgItemText(hWnd,
							IDC_FILENAME,
							szDestFile);
					}
					break;
				default:
					break;
			}
			return TRUE;
			break;
		case WM_INITDIALOG:
			return TRUE;
			break;
		case WM_NOTIFY:
			switch(((NMHDR FAR *)lParam)->code) {
				/* Set buttons when this page is activated; remember, it could
 				 * be activated in (nearly) any order */
				case PSN_SETACTIVE:
					ASSERT(strcmp(szDestFile, "")!=0);

					SetDlgItemText(hWnd,
						IDC_FILENAME,
						szDestFile);

					PropSheet_SetWizButtons(GetParent(hWnd),
						PSWIZB_BACK|PSWIZB_FINISH);
					break;
				case PSN_KILLACTIVE:
					GetDlgItemText(hWnd,
						IDC_FILENAME,
						szDestFile,
						sizeof(szDestFile));
					PropSheet_SetWizButtons(GetParent(hWnd),
						PSWIZB_BACK|PSWIZB_NEXT);
					break;
				case PSN_WIZFINISH:
					GetDlgItemText(hWnd,
						IDC_FILENAME,
						szDestFile,
						sizeof(szDestFile));

					ASSERT(ImportClass != NULL);
					ASSERT(WinCvtGetClassName(ImportClass) != NULL);

					//
					// Move to the next page (zero-based.)
					//
					PropSheet_SetCurSel(GetParent(hWnd), NULL, 3);
					PropSheet_CancelToClose(GetParent(hWnd));
					PropSheet_SetWizButtons(GetParent(hWnd), 0);

					//
					// The export class can be NULL, if we're
					// converting to RTF.
					//

					Status = WinCvtConvertFile(szSourceFile,
						WinCvtGetClassName(ImportClass),
						szDestFile,
						ExportClass?WinCvtGetClassName(ExportClass):NULL,
						ProgressCallback);

					if (Status!=WINCVT_SUCCESS) {
						CHAR szTemp[256];
						CHAR szTemp2[400];

						WinCvtGetErrorString(Status,
							szTemp,
							sizeof(szTemp));
						
						sprintf(szTemp2,
							"Could not convert file.  Reason: %s",
							szTemp);

						MessageBox(hWnd,
							szTemp2,
							NULL,
							0);
					} else {
						MessageBox(hWnd,
							"File converted successfully.",
							product,
							0);

					}

					break;
			}
			return TRUE;
			break;
		default:
			return FALSE;
	}
	return FALSE;
}

BOOL CALLBACK
Page4Dlg(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg) {
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				default:
					break;
			}
			return TRUE;
			break;
		case WM_INITDIALOG:
			return TRUE;
			break;
		case WM_NOTIFY:
			switch(((NMHDR FAR *)lParam)->code) {
				/* When this page is activated, we've passed the point of no
				 * return */
				case PSN_SETACTIVE:
					hWndProgress = hWnd;
					break;
				case PSN_KILLACTIVE:
					break;
				case PSN_WIZFINISH:
					break;
			}
			return TRUE;
			break;
		default:
			return FALSE;
	}
	return FALSE;
}


#define MAX_PAGES 4

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR szCmdLine, int nCmdShow)
{
	PROPSHEETHEADER PropSheetHeader;
	PROPSHEETPAGE   PropSheetPage[MAX_PAGES];

	int i;

	if (!WinCvtIsLibraryCompatible()) {
		MessageBox(NULL,
			"This application is not compatible with this version of WinCvt.dll.",
			NULL,
			MB_ICONSTOP);
		return EXIT_FAILURE;
	}

	hInst = hInstance;

	memset(&PropSheetHeader, 0, sizeof(PropSheetHeader));
	memset(&PropSheetPage, 0, sizeof(PropSheetPage));

	PropSheetHeader.dwSize = sizeof(PropSheetHeader);
	PropSheetHeader.dwFlags = PSH_NOAPPLYNOW |
		PSH_WIZARD |
		PSH_PROPSHEETPAGE |
		PSH_USECALLBACK;
	PropSheetHeader.hInstance = hInst;
	PropSheetHeader.pszCaption = "File Conversion Wizard";
	PropSheetHeader.nPages = MAX_PAGES;
	PropSheetHeader.ppsp = PropSheetPage;

	for (i=0;i<MAX_PAGES;i++) {
		memset(&PropSheetPage[i], 0, sizeof(PROPSHEETPAGE));
		PropSheetPage[i].dwSize = sizeof(PROPSHEETPAGE);
		PropSheetPage[i].hInstance = hInst;
	}

	//
	// FIXME - On Win64, DLGPROC returns 8bytes (INT_PTR).
	// Need to devise compatible way to support Win64 in
	// this case while working on old compilers.
	//
	PropSheetPage[0].pfnDlgProc = (DLGPROC)Page1Dlg;
	PropSheetPage[0].pszTemplate = MAKEINTRESOURCE(IDD_WIZ1);

	PropSheetPage[1].pfnDlgProc = (DLGPROC)Page2Dlg;
	PropSheetPage[1].pszTemplate = MAKEINTRESOURCE(IDD_WIZ2);

	PropSheetPage[2].pfnDlgProc = (DLGPROC)Page3Dlg;
	PropSheetPage[2].pszTemplate = MAKEINTRESOURCE(IDD_WIZ3);

	PropSheetPage[3].pfnDlgProc = (DLGPROC)Page4Dlg;
	PropSheetPage[3].pszTemplate = MAKEINTRESOURCE(IDD_WIZ4);

	//
	// If a parameter was specified, load it into the source file
	// field.
	//
	strcpy(szDestFile, "");
	strcpy(szSourceFile, "");

	if (szCmdLine != NULL && szCmdLine[0] != '\0') {
		if (strlen(szCmdLine) < sizeof(szSourceFile)) {
			LPSTR EndOfMeaningfulSection;

			while (szCmdLine[0]=='"'||szCmdLine[0]=='\''||szCmdLine[0]==' ')
				szCmdLine++;

			EndOfMeaningfulSection = strchr(szCmdLine, '"');
			if (EndOfMeaningfulSection) *EndOfMeaningfulSection = '\0';

			EndOfMeaningfulSection = strchr(szCmdLine, '\'');
			if (EndOfMeaningfulSection) *EndOfMeaningfulSection = '\0';

			strcpy(szSourceFile, szCmdLine);
			//
			// Start from page 2 since we don't need to input a file
			// in this case.
			//
			PropSheetHeader.nStartPage = 1;

			UpdateInputFileData();
		}
	}

	PropertySheet(&PropSheetHeader);

	if (AllImportConverters) {
		WinCvtFreeConverterList(AllImportConverters);
		AllImportConverters = NULL;
	}

	if (AllExportConverters) {
		WinCvtFreeConverterList(AllExportConverters);
		AllExportConverters = NULL;
	}

	if (MatchingImportConverters != NULL) {
		WinCvtFreeConverterList(MatchingImportConverters);
		MatchingImportConverters = NULL;
	}
	WinCvtFinalTeardown();

	return EXIT_SUCCESS;
}


