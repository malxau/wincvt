/* WINCVT
 * =======
 * This software is Copyright (c) 2006-2015 Malcolm Smith.
 * No warranty is provided, including but not limited to
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * This code is licenced subject to the GNU General
 * Public Licence (GPL).  See the COPYING file for more.
 */

#ifndef _WINCVT_H
#define _WINCVT_H

#ifdef __cplusplus
extern "C" {
#endif

#define WINCVT_VERSION_MAJOR  0
#define WINCVT_VERSION_MINOR  4
#define WINCVT_VERSION_MICRO  1

#define WINCVT_STRING_INT(str) #str
#define WINCVT_STRING(str) WINCVT_STRING_INT(str)

#define WINCVT_VER_STRING \
	WINCVT_STRING(WINCVT_VERSION_MAJOR) "." \
	WINCVT_STRING(WINCVT_VERSION_MINOR) "." \
	WINCVT_STRING(WINCVT_VERSION_MICRO)


#define WINCVT_NAME "WinCvt Windows Converter Toolkit"

#define WINCVT_COPYRIGHT "Copyright (c) 2006-2015"

#if !defined(RC_INVOKED)

// File Conversion Errors
#define WINCVT_SUCCESS                0

#define WINCVT_ERROR_OPEN           (-1)
#define WINCVT_ERROR_READ           (-2)
#define WINCVT_ERROR_WRITE          (-4)
#define WINCVT_ERROR_DATA           (-5)
#define WINCVT_ERROR_NO_MEMORY      (-8)
#define WINCVT_ERROR_DISK_FULL      (-10)
#define WINCVT_ERROR_CREATE         (-12)
#define WINCVT_ERROR_CANCEL         (-13)
#define WINCVT_ERROR_WRONG_TYPE     (-14)
#define WINCVT_ERROR_BAD_IMAGE      (-15)

#define WINCVT_ERROR_CUSTOM_START   (-1000)
#define WINCVT_ERROR_BAD_PARAM_1    (WINCVT_ERROR_CUSTOM_START+1)
#define WINCVT_ERROR_BAD_PARAM_2    (WINCVT_ERROR_CUSTOM_START+2)
#define WINCVT_ERROR_BAD_PARAM_3    (WINCVT_ERROR_CUSTOM_START+3)
#define WINCVT_ERROR_BAD_PARAM_4    (WINCVT_ERROR_CUSTOM_START+4)
#define WINCVT_ERROR_BAD_PARAM_5    (WINCVT_ERROR_CUSTOM_START+5)
#define WINCVT_ERROR_BAD_PARAM_6    (WINCVT_ERROR_CUSTOM_START+6)
#define WINCVT_ERROR_BAD_PARAM_7    (WINCVT_ERROR_CUSTOM_START+7)
#define WINCVT_ERROR_BAD_PARAM_8    (WINCVT_ERROR_CUSTOM_START+8)
#define WINCVT_ERROR_BAD_PARAM_9    (WINCVT_ERROR_CUSTOM_START+9)
#define WINCVT_ERROR_BAD_ENTRY      (WINCVT_ERROR_CUSTOM_START+10)
#define WINCVT_ERROR_REGISTRY       (WINCVT_ERROR_CUSTOM_START+11)
#define WINCVT_ERROR_CONVERTER_VETO (WINCVT_ERROR_CUSTOM_START+12)
#define WINCVT_ERROR_NO_CONVERTERS  (WINCVT_ERROR_CUSTOM_START+13)
#define WINCVT_ERROR_BAD_PATH       (WINCVT_ERROR_CUSTOM_START+14)

typedef PVOID WINCVT_CVT_CLASS;
typedef PVOID WINCVT_CVT_LIST;
typedef LONG  WINCVT_STATUS;

#define WINCVT_GET_MAJOR_VERSION(x) \
	((x>>16)&0xff)

#define WINCVT_GET_MINOR_VERSION(x) \
	((x>>8)&0xff)

#define WINCVT_GET_MICRO_VERSION(x) \
	(x&0xff)

typedef WINCVT_STATUS (WINAPI * _WinCvtProgressCallback)(LONG, LONG);

BOOL
WinCvtAreVersionsCompatible(
  DWORD HeaderVersion
);

WINCVT_STATUS
WinCvtConvertFile(
  LPCSTR szSourceFile,
  LPCSTR szSourceClass,
  LPCSTR szDestFile,
  LPCSTR szDestClass,
  PVOID Reserved
);

WINCVT_STATUS
WinCvtConvertToNative(
  LPCSTR szFileName,
  LPCSTR szConverter,
  LPCSTR szClass,
  HANDLE Input,
  PVOID Reserved
);

WINCVT_STATUS
WinCvtConvertToRtf(
  LPCSTR FileName,
  LPCSTR Converter,
  LPCSTR szClass,
  HANDLE Output,
  _WinCvtProgressCallback ProgressRoutine
);

VOID
WinCvtFinalTeardown();

WINCVT_STATUS
WinCvtFreeConverterList(
  WINCVT_CVT_LIST List
);

LPCSTR
WinCvtGetClassDescription(
  WINCVT_CVT_CLASS Item
);

LPCSTR
WinCvtGetClassExtensions(
  WINCVT_CVT_CLASS Item
);

LPCSTR
WinCvtGetClassFileName(
  WINCVT_CVT_CLASS Item
);

LPCSTR
WinCvtGetClassName(
  WINCVT_CVT_CLASS Item
);

WINCVT_STATUS
WinCvtGetConverterExportCapabilityList(
  LPCSTR szConverter,
  WINCVT_CVT_LIST* List
);

WINCVT_STATUS
WinCvtGetConverterImportCapabilityList(
  LPCSTR szConverter,
  WINCVT_CVT_LIST* List
);

WINCVT_STATUS
WinCvtGetErrorString(
  WINCVT_STATUS Error,
  LPSTR szBuffer,
  DWORD BufferLength
);

WINCVT_STATUS
WinCvtGetExportConverterList(
  WINCVT_CVT_LIST* List,
  LPCSTR szClass,
  LPCSTR szExtension
);

WINCVT_CVT_CLASS
WinCvtGetFirstClass(
  WINCVT_CVT_LIST List
);

WINCVT_STATUS
WinCvtGetImportConverterList(
  WINCVT_CVT_LIST* List,
  LPCSTR szClass,
  LPCSTR szExtension,
  LPCSTR szFilenameToVerify
);

WINCVT_CVT_CLASS
WinCvtGetNextClass(
  WINCVT_CVT_CLASS Item
);

#define WinCvtHeaderVersion() \
  ((WINCVT_VERSION_MAJOR<<16) + (WINCVT_VERSION_MINOR<<8) + WINCVT_VERSION_MICRO)


WINCVT_STATUS
WinCvtInstallConverter(
  LPCSTR szConverter,
  BOOL bUserOnly
);

#define WinCvtIsLibraryCompatible() \
  WinCvtAreVersionsCompatible(WinCvtHeaderVersion())

DWORD
WinCvtLibraryVersion();

WINCVT_STATUS
WinCvtUninstallConverter(
  LPCSTR szConverter,
  BOOL bUserOnly
);

#endif /* RC_INVOKED */

#ifdef __cplusplus
}
#endif
#endif
