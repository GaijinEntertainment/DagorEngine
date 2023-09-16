#ifndef __SIMPLYGONSDKLOADER_H__
#define __SIMPLYGONSDKLOADER_H__

#include "SimplygonSDK.h"

#ifdef _WIN32
#include <tchar.h>
#include <WTypes.h>
#elif defined(__linux__) || defined(__APPLE__)
#define TCHAR   char
#define LPTSTR  char *
#define LPCTSTR const char *
#define _T(x)   x
#endif


namespace SimplygonSDK
	{
#ifdef _WIN32
	/// Adds a location to look for the DLL in. This method must be called before calling Initialize()
	void AddSearchPath( LPCTSTR search_path );

	/// Clears the additional search locations to look for the DLL in.
	void ClearAdditionalSearchPaths();
#endif

	/// Loads and initializes the SDK
	int Initialize( ISimplygonSDK **pInterfacePtr , LPCTSTR SDKPath = NULL , LPCTSTR LicenseDataText = NULL );

	/// Deinitializes the SDK, releases the DLL and all allocated memory
	void Deinitialize();

	/// Retrieves the error string of the error code.
	LPCTSTR GetError( int error_code );

#ifdef _WIN32
	/// Retrieves the license system log, for when initializing
	int PollLog( LPTSTR dest , int max_len_dest );

	/// Run the license wizard process, to ease license integration
	int RunLicenseWizard( LPCTSTR batch_file = NULL );
#endif
	};
	
#endif //__SIMPLYGONSDKLOADER_H__
