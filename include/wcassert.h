#ifdef DEBUG_WINCVT

#include <winbase.h>
#include <winuser.h>

extern VOID
DbgRealAssert( LPCSTR szCondition, 
	LPCSTR szFunction,
	LPCSTR szFile,
	DWORD dwLine );

//
//  __FUNCTION__ isn't defined until ~MSVC7, which isn't much use.  This is one
//  really great feature, but MSVC7 isn't my 'default' compiler yet, and
//  won't be for a ~really~ long time.
//

#ifndef __FUNCTION__
#define __FUNCTION__ ""
#endif

//
//  We output to three places: the debugger, stdout, and UI.  This is
//  because we can't know what type of application is running here:
//  this code may be in a DLL used by different types of applications.
//  Note also that we preserve LastError in this function, since
//  MessageBox et al will mess with it. 
//


#define ASSERT(x) \
	if (!(x)) { \
		DbgRealAssert( #x, __FUNCTION__, __FILE__, __LINE__ ); \
	} 

#else

#define ASSERT(x)

#endif
