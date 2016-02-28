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
#include <richedit.h>
#include <stdio.h>

#ifdef MINICRT
#include <minicrt.h>
#endif

#include <wincvt.h>
#include <wcassert.h>
#include "viewdlg.h"

HINSTANCE hInst;
HWND hWndMain;
HWND hWndCli;
HWND hWndStatus;

#define product "WinCvt Document Viewer"

LRESULT APIENTRY MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


HINSTANCE GethInst()
{
	return hInst;
}

HWND GethWndMain()
{
	return hWndMain;
}

HWND GethWndCli()
{
	return hWndCli;
}

HWND GethWndStatus()
{
	return hWndStatus;
}

#ifndef RICHEDIT_CLASS
//
//  If building with a compiler that doesn't natively have support
//  for RichEdV2.  We can only hope that if the compiler has no
//  knowledge of v2, the binary will still run on a v2 system.
//
#define RICHEDIT_CLASS "RichEdit20A"
#endif

HWND
InitInstance(
  HINSTANCE hInstance,
  int nCmdShow)
{
	HFONT hFont;
	HDC hDC;
	RECT Rect;
	BOOLEAN RichEdV2;

	hInst = hInstance;  

	InitCommonControls();
	if (LoadLibrary("RichEd20.dll")) {
		RichEdV2 = TRUE;
	} else {
		LoadLibrary("RichEd32.dll");
		RichEdV2 = FALSE;
	}

	hWndMain = CreateWindowEx(
		0,
		"WinCvtView",
		product,
		WS_OVERLAPPEDWINDOW|WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		(HWND)NULL,
		(HMENU)NULL,
		(HINSTANCE)hInstance,
		(LPVOID)NULL
	);

	hWndCli = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		RichEdV2?RICHEDIT_CLASS:"RichEdit",
		"",
		WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_READONLY,
		0,0,32,32,
		(HWND)hWndMain,
		(HMENU)NULL,
		(HINSTANCE)hInstance,
		NULL
	);

	hWndStatus = CreateWindowEx(
		0,
		STATUSCLASSNAME,
		"",
		WS_CHILD|WS_VISIBLE|SBARS_SIZEGRIP,
		0,0,100,20,
		(HWND)hWndMain,
		(HMENU)NULL,
		(HINSTANCE)hInstance,
		(LPVOID)NULL);

	hDC = GetDC(hWndCli);
	hFont = CreateFont(-MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72),
			0,0,0,0,0,0,0,
			ANSI_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			0,
			"MS Sans Serif");

	ReleaseDC(hWndCli,hDC);
	SendMessage(hWndCli,
		WM_SETFONT,
		(WPARAM)hFont,
		MAKELPARAM(TRUE,0));

	GetClientRect(hWndMain, &Rect);

	SendMessage(hWndMain,
		WM_SIZE,
		0,
		MAKELPARAM(Rect.right,Rect.bottom));

	SendMessage(hWndCli,
		EM_SETLIMITTEXT,
		(1024*1024*16), // 16Mb instead of default 64/32K
		0);

	return hWndMain;
}


/*
This structure is used to record data when it's being streamed into the
RichEdit control.  Because we only have one pointer (dwCookie) that gets
sent into the callback routine, we need to package everything into a
structure.
*/
typedef struct RTFdata
{
	HANDLE hFile;
} RTFdata;

#ifndef _WIN64
#ifndef DWORD_PTR
typedef DWORD DWORD_PTR;
#endif
#endif

#ifndef OFN_ENABLESIZING
#define OFN_ENABLESIZING 0x00800000
#endif

/* This is the callback routine that sends things into the RichEdit control */
DWORD CALLBACK
EditStreamInputCallback(
  DWORD_PTR dwCookie,  // our data
  LPBYTE buf,          // destination buffer
  LONG cb,             // number of bytes to read or write
  LONG * pcb)          // number actually read or written
{
	RTFdata * rtf;
	DWORD gle;

	rtf=(RTFdata *)dwCookie;

	if (ReadFile(rtf->hFile, buf, cb, pcb, NULL)) {
		return 0;
	}

	*pcb = 0;
	gle = GetLastError();

	if (gle == ERROR_HANDLE_EOF) {
		return 0;
	}

	return gle;
}

#ifdef WINCVT_PRINT_SUPPORT

#define INCH_TWIPS 1440

#define MARGIN (INCH_TWIPS/2) 


VOID PrintRichText()
{
	PRINTDLG pd;

	pd.lStructSize = sizeof(PRINTDLG);
	pd.hwndOwner = GethWndMain();
	pd.hDevMode = (HANDLE)NULL;
	pd.hDevNames = (HANDLE)NULL;
	pd.nFromPage = 0;
	pd.nToPage = 0;
	pd.nMinPage = 0;
	pd.nMaxPage = 0;
	pd.nCopies = 0;
	pd.hInstance = hInst;
	pd.Flags = PD_RETURNDC|PD_NOPAGENUMS|PD_NOSELECTION;
	pd.lpfnSetupHook = (LPSETUPHOOKPROC)NULL;
	pd.lpfnPrintHook = (LPPRINTHOOKPROC)NULL;
	pd.lpPrintTemplateName = (LPTSTR)NULL;
	if (PrintDlg(&pd)) {
		FORMATRANGE fr;
		DOCINFO docInfo;
		LONG lTextOut, lTextAmt;

		fr.hdc = fr.hdcTarget = pd.hDC;
		fr.chrg.cpMin = 0;
		fr.chrg.cpMax = -1;
		fr.rcPage.top = fr.rcPage.left = 0;
		fr.rc.top = MARGIN;
		fr.rc.left = MARGIN;
		fr.rcPage.right = MulDiv(GetDeviceCaps(pd.hDC,PHYSICALWIDTH)-2*GetDeviceCaps(pd.hDC,PHYSICALOFFSETX),
			INCH_TWIPS,
			GetDeviceCaps(pd.hDC,LOGPIXELSX))-MARGIN;
		fr.rc.right = fr.rcPage.right - MARGIN;
		fr.rcPage.bottom = MulDiv(GetDeviceCaps(pd.hDC,PHYSICALHEIGHT)-2*GetDeviceCaps(pd.hDC,PHYSICALOFFSETY),
			INCH_TWIPS,
			GetDeviceCaps(pd.hDC,LOGPIXELSY))-MARGIN;
		fr.rc.bottom = fr.rcPage.bottom - MARGIN;
		
		docInfo.cbSize = sizeof(DOCINFO);
		docInfo.lpszDocName = "converted file";
		docInfo.lpszOutput = NULL;

		SetMapMode(pd.hDC, MM_TEXT);
		StartDoc(pd.hDC, &docInfo);
		StartPage(pd.hDC);
		lTextOut = 0;

		lTextAmt = SendMessage(GethWndCli(),
			WM_GETTEXTLENGTH,
			0,
			0);

		SendMessage(GethWndCli(),
			EM_FORMATRANGE,
			TRUE,
			(LPARAM)NULL);

		while (lTextOut < lTextAmt) {

			lTextOut = SendMessage(GethWndCli(),
				EM_FORMATRANGE,
				TRUE,
				(LPARAM)&fr);

			if (lTextOut < lTextAmt) {
				EndPage(pd.hDC);
				StartPage(pd.hDC);
				fr.chrg.cpMin = lTextOut;
				fr.chrg.cpMax = -1;
			}
		}
		SendMessage(GethWndCli(),
			EM_FORMATRANGE,
			TRUE,
			(LPARAM)NULL);

		EndPage(pd.hDC);
		EndDoc(pd.hDC);

		DeleteDC(pd.hDC);
	}

}

#endif


BOOL OpenSourceFile(LPSTR szFile)
{
	HANDLE hFile;
	HANDLE hTempFile;
	RTFdata rtf;
	EDITSTREAM eStream;
	CHAR szTempFile[256];
	CHAR szMessage[400];
	CHAR szFileName[20];
	WINCVT_STATUS Status;

	hFile = CreateFile(szFile,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		sprintf(szMessage, "Could not open %s", szFile);
		MessageBox(GethWndMain(), szMessage, product, 16);
		return FALSE;
	}
	CloseHandle(hFile);

	GetTempPath(sizeof(szTempFile)-20, szTempFile);

	sprintf(szFileName, "cvt%08x.rtf", GetCurrentProcessId());
	strcat(szTempFile, szFileName);

	hTempFile = CreateFile(szTempFile,
		GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_DELETE_ON_CLOSE,
		NULL);
	if (hTempFile == INVALID_HANDLE_VALUE) {
		sprintf(szMessage, "Could not create temporary file %s", szTempFile);
		MessageBox(GethWndMain(), szMessage, product, 16);
		return FALSE;
	}

	Status = WinCvtConvertToRtf(szFile,
		NULL,
		NULL,
		hTempFile,
		NULL);

	if (Status != WINCVT_SUCCESS) {
		CloseHandle(hTempFile);
		WinCvtGetErrorString(Status, szTempFile, sizeof(szTempFile));
		sprintf(szMessage, "Could not convert file (Reason: %s)", szTempFile);
		MessageBox(GethWndMain(), szMessage, product, 16);
		return FALSE;
	}	

	SetFilePointer(hTempFile, 0, 0, FILE_BEGIN);

	rtf.hFile = hTempFile;

	// FIXME - Will barf on Win64
	eStream.dwCookie = (DWORD)&rtf;
	eStream.pfnCallback = EditStreamInputCallback;
	eStream.dwError = 0;

	SendMessage(GethWndCli(),
		EM_STREAMIN,
		(WPARAM)SF_RTF,
		(LPARAM)&eStream);

	if (eStream.dwError != 0) {
		sprintf(szMessage, "An error occurred loading RTF to display (%i)", eStream.dwError);
		MessageBox(GethWndMain(), szMessage, product, 16);
		CloseHandle(hTempFile);
		return FALSE;
	}

	SendMessage(GethWndCli(),
		EM_SETMODIFY,
		TRUE,
		0);

	SendMessage(GethWndStatus(),
		SB_SETTEXT,
		0,
		(LPARAM)szFile);

	CloseHandle(hTempFile);

	return TRUE;
}


BOOL
ProcessCmd(
  LPSTR lpCmdLine)
{
	if (lpCmdLine != NULL && lpCmdLine[0]!='\0') {
		return OpenSourceFile(lpCmdLine);
	}

	// Succeeded at doing nothing.
	return TRUE;
}

BOOL
InitApplication(
  HINSTANCE hInstance)
{
	WNDCLASS  wc;

	if (!WinCvtIsLibraryCompatible()) {
		MessageBox(NULL,
			"This application is not compatible with this version of WinCvt.dll.",
			NULL,
			16);
		return FALSE;
	}

	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(VIEWICON));
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
	wc.lpszMenuName = "CvtViewMenu";
	wc.lpszClassName = "WinCvtView";
	return RegisterClass(&wc);
}


int WINAPI
WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
	MSG msg;

	if (!WinCvtIsLibraryCompatible()) {
		MessageBox(NULL,
			"This application is not compatible with this version of WinCvt.dll.",
			NULL,
			MB_ICONSTOP);
		return EXIT_FAILURE;
	}

	if (!InitApplication(hInstance)) return FALSE;
	if (!InitInstance(hInstance, nCmdShow)) return FALSE;

	ShowWindow(hWndMain, nCmdShow);
	UpdateWindow(hWndMain);
	ProcessCmd(lpCmdLine);	
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return EXIT_SUCCESS;
}

BOOL OpenFileBySelection(HWND hWnd)
{
	OPENFILENAME ofn;
	CHAR szFile[MAX_PATH];

	memset(&ofn, 0, sizeof(ofn));
	strcpy(szFile, "");

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = hInst;
	ofn.lpstrFilter = "All Files (*.*)\0*.*\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szFile;
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
		return FALSE;
	} else {
		return OpenSourceFile(szFile);
	}
}



LRESULT APIENTRY
MainWndProc(
  HWND hWnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam)
{
	static CHAR szOpenFile[MAX_PATH];

	switch (message) {
 	case WM_COMMAND:
		switch(LOWORD(wParam)) {
			case IDM_OPEN:
				OpenFileBySelection(hWnd);
				break;
#ifdef WINCVT_PRINT_SUPPRT
			case IDM_PRINT:
				PrintRichText();
				break;
#endif
			case IDM_EXIT:
				DestroyWindow(hWnd);
				PostQuitMessage(0);
				break;
			case IDM_COPY:
				SendMessage(GethWndCli(),
					WM_COPY,
					0,
					0);
				break;
			case IDM_ABOUT:
				{
					CHAR szTemp[300];
					sprintf(szTemp, "%s, %lu.%lu.%lu."
#ifdef PRERELEASE
						" Prerelease build, built on %s."
#endif
						"  "
						WINCVT_COPYRIGHT
						".  "
						"This software is licensed under the terms of the GNU Public License, "
						"version 2 or any later version.",
						product,
						WINCVT_GET_MAJOR_VERSION(WinCvtHeaderVersion()),
						WINCVT_GET_MINOR_VERSION(WinCvtHeaderVersion()),
						WINCVT_GET_MICRO_VERSION(WinCvtHeaderVersion())
#ifdef PRERELEASE
						, __DATE__
#endif
						);
					MessageBox(hWnd, szTemp, product, 64);
					break;
				}
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
				break;
			}
		break;
	case WM_CREATE:
		break;
#ifdef WM_MOUSEWHEEL
	case WM_MOUSEWHEEL:
		PostMessage(GethWndCli(), WM_MOUSEWHEEL, wParam, lParam);
		break;
#endif
	case WM_CLOSE:
		DestroyWindow(hWnd);
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		{
			RECT Rect;
			int WinHeight;
			GetWindowRect(GethWndStatus(),&Rect);
			WinHeight = Rect.bottom - Rect.top;
			MoveWindow(GethWndCli(),
				0,
				0,
				(int)LOWORD(lParam),
				(int)(HIWORD(lParam)-WinHeight),
				TRUE);	
			MoveWindow(GethWndStatus(),
				0,
				HIWORD(lParam)-WinHeight,
				LOWORD(lParam),
				WinHeight,
				TRUE);
			RedrawWindow(GethWndStatus(),
				NULL,
				NULL,
				RDW_ERASE|RDW_INVALIDATE);
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return (0);
}

