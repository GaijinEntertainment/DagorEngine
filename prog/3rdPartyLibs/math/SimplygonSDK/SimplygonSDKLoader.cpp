#include <stdio.h>
#include <stdlib.h> 
#include <string>
#include <vector>

#ifdef _WIN32
#include <tchar.h>
#include <windows.h>
#include <shlobj.h>
#include <process.h>
#include <io.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <iostream>
#include <dlfcn.h>
#include <pthread.h>
#include <limits.h>
#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif
#endif

#ifdef SAAS
#include "SimplygonSDKCloudLoader.h"
#else
#include "SimplygonSDKLoader.h"
#endif


static std::vector<std::basic_string<TCHAR> > AdditionalSearchPaths;

#ifdef _WIN32 

typedef int (CALLBACK* LPINITIALIZESIMPLYGONSDK)( const char* license_data , SimplygonSDK::ISimplygonSDK **pInterfacePtr );
typedef void (CALLBACK* LPDEINITIALIZESIMPLYGONSDK)();
typedef void (CALLBACK* LPGETINTERFACEVERSIONSIMPLYGONSDK)( char *deststring );
typedef int (CALLBACK* LPPOLLLOGSIMPLYGONSDK)( char *destbuffer , int max_length );

#define PUBLICBUILD

#endif


namespace SimplygonSDK
	{

#ifdef _WIN32
	template<typename T> void safe_delete_ptr(T*& a) { delete a; a = nullptr; }
	template<typename T> void safe_delete_array(T*& a) { delete [] a; a = nullptr; }

	static LPINITIALIZESIMPLYGONSDK InitializeSimplygonSDKPtr = NULL; 
	static LPDEINITIALIZESIMPLYGONSDK DeinitializeSimplygonSDKPtr = NULL;
	static LPGETINTERFACEVERSIONSIMPLYGONSDK GetInterfaceVersionSimplygonSDKPtr = NULL;
	static LPPOLLLOGSIMPLYGONSDK PollLogSimplygonSDKPtr = NULL;
	static HINSTANCE hDLL = NULL; // Handle to SimplygonSDK DLL
	static int LoadError = SG_ERROR_INVALIDPARAM; // if the load failed, this contains the error

	// critical sections, process-local mutexes
	class rcriticalsection
		{
		private:
			CRITICAL_SECTION cs;

		public:
			rcriticalsection() { ::InitializeCriticalSection(&cs); }
			~rcriticalsection() { ::DeleteCriticalSection(&cs); }

			void Enter() { EnterCriticalSection(&cs); }
			void Leave() { LeaveCriticalSection(&cs); }
		};

	static int GetStringFromRegistry( LPCTSTR keyid , LPCTSTR valueid , LPTSTR dest )
		{
		HKEY reg_key;
		if( RegOpenKey( HKEY_LOCAL_MACHINE , keyid , &reg_key ) != ERROR_SUCCESS )
			return SimplygonSDK::SG_ERROR_NOLICENSE; 

		// read the value from the key
		DWORD path_size = MAX_PATH;
		if( RegQueryValueEx( reg_key , valueid , NULL ,	NULL , (unsigned char *)dest , &path_size ) != ERROR_SUCCESS )
			return SimplygonSDK::SG_ERROR_NOLICENSE;
		dest[path_size] = 0;

		// close the key
		RegCloseKey( reg_key );
		return SimplygonSDK::SG_ERROR_NOERROR;
		}

	inline LPTSTR ConstCharPtrToLPTSTR(const char * stringToConvert) 
		{
		size_t newsize = strlen(stringToConvert)+1;
		LPTSTR returnString = (LPTSTR)malloc(newsize * sizeof(TCHAR));
#ifndef _UNICODE
		memcpy( returnString , stringToConvert , newsize );	
#else
		size_t convertedChars = 0;
		mbstowcs_s(&convertedChars, returnString, newsize, stringToConvert, _TRUNCATE);
#endif // _UNICODE
		return returnString;
		}

	inline const char* LPCTSTRToConstCharPtr(LPCTSTR stringToConvert) 
		{
#ifndef _UNICODE
		size_t wlen = _tcslen(stringToConvert);
		char* standardChar = (char*)malloc((wlen+1) * sizeof(char)); 
		memcpy(standardChar,stringToConvert,wlen);
		standardChar[wlen] = 0;
		return standardChar;
#else
		size_t wlen = _tcslen(stringToConvert)+1;
		char* standardChar = (char*)malloc(wlen * sizeof(char)); 
		size_t convertedChars = 0;
		wcstombs_s(&convertedChars, standardChar, wlen, stringToConvert,_TRUNCATE);
		return standardChar;
#endif
		}

	static bool FileExists( LPCTSTR path )
		{
		DWORD v = ::GetFileAttributes( path );
		if( v == INVALID_FILE_ATTRIBUTES )
			{
			return false;
			}
		return true;
		}


	static std::basic_string<TCHAR> GetInstallationPath()
		{
		TCHAR InstallationPath[MAX_PATH];
#ifdef SAAS
		// get the installation path string from the registry
		if( GetStringFromRegistry( _T("Software\\DonyaLabs\\SimplygonSDKCloud") , _T("InstallationPath") , InstallationPath ) != 0 )
			{
			return std::basic_string<TCHAR>(_T(""));
			}
#else
		if( GetStringFromRegistry( _T("Software\\DonyaLabs\\SimplygonSDK") , _T("InstallationPath") , InstallationPath ) != 0 )
			{
			return std::basic_string<TCHAR>(_T(""));
			}

#endif

		size_t t = _tcslen(InstallationPath);
		if( InstallationPath[t] != _T('\\') )
			{
			InstallationPath[t] = _T('\\');
			InstallationPath[t+1] = _T('\0');
			}

		// append a backslash
		return InstallationPath;
		}

	static std::basic_string<TCHAR> GetCommonAppPath()
		{
		TCHAR AppDataPath[MAX_PATH];

		// Get the common appdata path
		if( SHGetFolderPath( NULL, CSIDL_COMMON_APPDATA, NULL, 0, AppDataPath ) != 0 )
			{
			return std::basic_string<TCHAR>(_T(""));
			}

		// append the path to Simplygon SDK
		return std::basic_string<TCHAR>(AppDataPath) + _T("\\DonyaLabs\\SimplygonSDK\\");
		}

	static std::basic_string<TCHAR> GetAppDataPath()
		{
		TCHAR AppDataPath[MAX_PATH];

		// Get the common appdata path
		if( SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA, NULL, 0, AppDataPath ) != 0 )
			{
			return std::basic_string<TCHAR>(_T(""));
			}

		// append the path to Simplygon SDK
		return std::basic_string<TCHAR>(AppDataPath) + _T("\\DonyaLabs\\SimplygonSDK\\");
		}

	static std::basic_string<TCHAR> GetApplicationPath()
		{
		TCHAR Path[MAX_PATH];
		TCHAR drive[_MAX_DRIVE];
		TCHAR dir[_MAX_DIR];
		TCHAR fname[_MAX_FNAME];
		TCHAR ext[_MAX_EXT];

		if( GetModuleFileName( NULL , Path , MAX_PATH ) == NULL )
			{	
			return std::basic_string<TCHAR>(_T(""));
			}

#if _MSC_VER >= 1400
		_tsplitpath_s( Path, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT );
		_tmakepath_s( Path, _MAX_PATH, drive, dir, _T(""), _T("") );
#else
		_tsplitpath( Path, drive, dir, fname, ext );
		_tmakepath( Path, drive, dir, _T(""), _T("") );
#endif

		return Path;
		}

	static std::basic_string<TCHAR> GetWorkingDirectory()
		{
		TCHAR dir[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, dir);
		return dir;
		}

	static std::basic_string<TCHAR> GetLibraryDirectory( std::basic_string<TCHAR> path )
		{
		TCHAR Directory[_MAX_PATH];
		TCHAR drive[_MAX_DRIVE];
		TCHAR dir[_MAX_DIR];
		TCHAR fname[_MAX_FNAME];
		TCHAR ext[_MAX_EXT];

#if _MSC_VER >= 1400
		_tsplitpath_s( path.c_str(), drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT );
		_tmakepath_s( Directory, _MAX_PATH, drive, dir, _T(""), _T("") );
#else
		_tsplitpath( path.c_str(), drive, dir, fname, ext );
		_tmakepath( Directory, drive, dir, _T(""), _T("") );
#endif

		return Directory;
		}

	static bool DynamicLibraryIsLoaded()
		{
		if(hDLL == NULL)
			{
			return false;
			}	
		return true;
		}

	static void UnloadDynamicLibrary()
		{
		if( !DynamicLibraryIsLoaded() )
			return;

		FreeLibrary(hDLL);
		hDLL = NULL;	

		InitializeSimplygonSDKPtr = NULL;
		DeinitializeSimplygonSDKPtr = NULL;
		GetInterfaceVersionSimplygonSDKPtr = NULL;
		PollLogSimplygonSDKPtr = NULL;
		}

	static bool LoadOSLibrary( std::basic_string<TCHAR> path )
		{
		// if no file exists, just return
		if( !FileExists( path.c_str() ) )
			{
			LoadError = SG_ERROR_FILENOTFOUND;
			return false;
			}
		
		// get the lib directory, it should be initialized in its directory
		std::basic_string<TCHAR> lib_dir = GetLibraryDirectory( path );
		std::basic_string<TCHAR> curr_dir = GetWorkingDirectory();

		// move to the dir where the library is 
		SetCurrentDirectory( lib_dir.c_str() );

		// try loading the library
		hDLL = LoadLibrary( path.c_str() );

		// move back to the previous dir
		SetCurrentDirectory( curr_dir.c_str() );

		// now, check if the library was loaded
		if( !DynamicLibraryIsLoaded() )
			{
			// load failed, probably a missing dependency
			TCHAR buffer[400];
			DWORD err = GetLastError();

#if _MSC_VER >= 1400
			_stprintf_s(buffer,MAX_PATH,_T("Loading the Simplygon DLL failed, which is in most cases because of a missing dependency, are all dependencies installed? Please try reinstalling Simplygon. GetLastError returns %d\n"), err);
#else
			_stprintf(buffer,_T("Loading the Simplygon DLL failed, which is in most cases because of a missing dependency, are all dependencies installed? Please try reinstalling Simplygon. GetLastError returns %d\n"), err);
#endif

			OutputDebugString(buffer);
			// set the error
			LoadError = SimplygonSDK::SG_ERROR_LOADFAILED;
			return false;
			}

		// setup the pointers
		InitializeSimplygonSDKPtr = (LPINITIALIZESIMPLYGONSDK)GetProcAddress(hDLL,"InitializeSimplygonSDK");
		DeinitializeSimplygonSDKPtr = (LPDEINITIALIZESIMPLYGONSDK)GetProcAddress(hDLL,"DeinitializeSimplygonSDK");
		GetInterfaceVersionSimplygonSDKPtr = (LPGETINTERFACEVERSIONSIMPLYGONSDK)GetProcAddress(hDLL,"GetInterfaceVersionSimplygonSDK");
		PollLogSimplygonSDKPtr = (LPPOLLLOGSIMPLYGONSDK)GetProcAddress(hDLL,"PollLogSimplygonSDK");
		if( InitializeSimplygonSDKPtr==NULL || 
			DeinitializeSimplygonSDKPtr==NULL ||
			GetInterfaceVersionSimplygonSDKPtr==NULL )
			{
			// load failed, invalid version
			TCHAR buffer[400];
#if _MSC_VER >= 1400
			_stprintf_s(buffer,MAX_PATH,_T("The Simplygon DLL loaded, but the .dll functions could not be retrieved, this is probably not a Simplygon dll file, or it is corrupted. Please reinstall Simplygon\n"));
#else
			_stprintf(buffer,_T("The Simplygon DLL loaded, but the .dll functions could not be retrieved, this is probably not a Simplygon dll file, or it is corrupted. Please reinstall Simplygon\n"));
#endif
			OutputDebugString(buffer);
			// set error
			LoadError = SimplygonSDK::SG_ERROR_WRONGVERSION;
			UnloadDynamicLibrary();
			return false;
			}

		// check the version string
		char versionstring[200];
		GetInterfaceVersionSimplygonSDKPtr( versionstring );
		const SimplygonSDK::rchar *header_version = SimplygonSDK::GetInterfaceVersionHash();
		if( strcmp (versionstring,header_version) != 0 )
			{
			// load failed, invalid version
			TCHAR buffer[400];
			LPTSTR pheader_version = ConstCharPtrToLPTSTR(header_version);
			LPTSTR pversionstring = ConstCharPtrToLPTSTR(versionstring);

#if _MSC_VER >= 1400
			_stprintf_s(buffer,MAX_PATH,_T("The Simplygon DLL loaded, but the interface version of the header is not the same as the library.\n\tHeader: %s\n\tLibrary: %s\n"),pheader_version,pversionstring);
#else
			_stprintf(buffer,_T("The Simplygon DLL loaded, but the interface version of the header is not the same as the library.\n\tHeader: %s\n\tLibrary: %s\n"),pheader_version,pversionstring);
#endif

			OutputDebugString(buffer);
			free(pheader_version);
			free(pversionstring);
			// set error
			LoadError = SimplygonSDK::SG_ERROR_WRONGVERSION;
			UnloadDynamicLibrary();
			return false;
			}

		// loaded successfully
		TCHAR buffer[400];
		LPTSTR pversionstring = ConstCharPtrToLPTSTR(versionstring);
#if _MSC_VER >= 1400
		_stprintf_s(buffer,MAX_PATH,_T("The Simplygon DLL was found and loaded successfully!\n\tInterface hash: %s \n"),versionstring);
#else
		_stprintf(buffer,_T("The Simplygon DLL was found and loaded successfully!\n\tInterface hash: %s \n"),versionstring);
#endif
		OutputDebugString(buffer);
		free(pversionstring);
		
		LoadError = SimplygonSDK::SG_ERROR_NOERROR;
		return true;
		}

	static bool FindNamedDynamicLibrary( std::basic_string<TCHAR> DLLName )
		{
		std::basic_string<TCHAR> InstallationPath = GetInstallationPath();
	
		std::basic_string<TCHAR> ApplicationPath = GetApplicationPath();
		std::basic_string<TCHAR> WorkingPath = GetWorkingDirectory() + _T("/");

		// try additional paths first, so they are able to override
		for( size_t i=0; i<AdditionalSearchPaths.size(); ++i )
			{
			if( LoadOSLibrary( AdditionalSearchPaths[i] + DLLName ) )
				{
				return true;
				}
			}

		// try local application path first
		if( LoadOSLibrary( ApplicationPath + DLLName ) )
			{
			return true;
			}

		// next try installation path
		if( !InstallationPath.empty() )
			{	
			if( LoadOSLibrary( InstallationPath + DLLName ) )
				{
				return true;
				}
			}

		// try working directory as well
		if( !WorkingPath.empty() )
			{
			if( LoadOSLibrary( WorkingPath + DLLName ) )
				{
				return true;
				}
			}
		
		return false;
		}

	static bool FindDynamicLibrary( LPCTSTR SDKPath )
		{
		std::basic_string<TCHAR> DLLName;

		if( SDKPath != NULL )
			{
			// load from explicit place, fail if not found
			if( LoadOSLibrary( std::basic_string<TCHAR>(SDKPath) ) )
				{
				return true;
				}
			return false;
			}

		// if debug version, try debug build first
#ifdef _DEBUG

#ifdef _WIN64
#ifdef SAAS
		DLLName = _T("SimplygonSDKCloudDebugx64.dll");
#else
		DLLName = _T("SimplygonSDKRuntimeDebugx64.dll");
#endif
#else
		DLLName = _T("SimplygonSDKRuntimeDebugWin32.dll");
#endif

		if( FindNamedDynamicLibrary( DLLName ) )
			{
			return true;
			}

#endif//_DEBUG

		// if debugrelease version, try that
#ifdef REDEBUG
#ifdef _WIN64
		DLLName = _T("SimplygonSDKRuntimeDebugReleasex64.dll");
#else
		DLLName = _T("SimplygonSDKRuntimeDebugReleaseWin32.dll");
#endif
		if( FindNamedDynamicLibrary( DLLName ) )
			{
			return true;
			}
#endif//REDEBUG

		// next (or if release) try release builds
#ifdef _WIN64
#ifdef SAAS
		DLLName = _T("SimplygonSDKCloudReleasex64.dll");
#else
		DLLName = _T("SimplygonSDKRuntimeReleasex64.dll");
#endif
#else
		DLLName = _T("SimplygonSDKRuntimeReleaseWin32.dll");
#endif
		
		if( FindNamedDynamicLibrary( DLLName ) )
			{
			return true;
			}

		LoadError = SimplygonSDK::SG_ERROR_FILENOTFOUND;

		return false;
		}

	static bool LoadDynamicLibrary( LPCTSTR SDKPath )
		{
		LoadError = SimplygonSDK::SG_ERROR_NOERROR;

		if( !FindDynamicLibrary(SDKPath) )
			{
			return false;
			}

		return true;
		}

	static std::basic_string<TCHAR> FindNamedProcess( std::basic_string<TCHAR> EXEName )
		{
		std::basic_string<TCHAR> InstallationPath = GetInstallationPath();
		std::basic_string<TCHAR> ApplicationPath = GetApplicationPath();
		std::basic_string<TCHAR> WorkingPath = GetWorkingDirectory();

		// order of running batch process
		std::basic_string<TCHAR> *dirs[] = {
			&ApplicationPath,
			&InstallationPath,
			&WorkingPath,
			NULL
			};

		// try the different paths
		int i=0;
		while( dirs[i] != NULL )
			{
			if( !dirs[i]->empty() )
				{
				std::basic_string<TCHAR> path = (*dirs[i]) + EXEName;
				if( FileExists( path.c_str() ) )
					{
					return path;
					}
				}
			++i;
			}

		return _T("");
		}


	// executes the batch process. redirects the stdout to the handle specified
	// places the handle of the process into the variable that phProcess points at
	static bool ExecuteProcess( const TCHAR *exepath , const TCHAR *ini_file , HANDLE *phProcess )
		{
		PROCESS_INFORMATION piProcInfo; 
		STARTUPINFO siStartInfo;
		BOOL bFuncRetn = FALSE; 

		// Set up members of the PROCESS_INFORMATION structure. 
		ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

		// Set up members of the STARTUPINFO structure. 
		ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
		siStartInfo.cb = sizeof(STARTUPINFO); 

		// create a command line string
		TCHAR *tmp_cmdline = new TCHAR[MAX_PATH*2];
#if _MSC_VER >= 1400
		if( ini_file != NULL )
			_stprintf_s(tmp_cmdline,MAX_PATH*2,_T("\"%s\" %s"),exepath,ini_file);
		else
			_stprintf_s(tmp_cmdline,MAX_PATH*2,_T("\"%s\" "),exepath);
#else
		if( ini_file != NULL )
			_stprintf(tmp_cmdline,_T("\"%s\" %s"),exepath,ini_file);
		else
			_stprintf(tmp_cmdline,_T("\"%s\" "),exepath);
#endif

		// Create the child process. 
		bFuncRetn = CreateProcess( 
			NULL,			// exe file path
			tmp_cmdline,    // command line 
			NULL,          	// process security attributes 
			NULL,          	// primary thread security attributes 
			TRUE,          	// handles are inherited 
			0,             	// creation flags 
			NULL,          	// use parent's environment 
			NULL,          	// use parent's current directory 
			&siStartInfo,  	// STARTUPINFO pointer 
			&piProcInfo		// receives PROCESS_INFORMATION 
			);  

		delete [] tmp_cmdline;

		if(bFuncRetn == 0) 
			{
			return false;
			}

		// function succeeded, return handle to process, release handles we will not use anymore
		*phProcess = piProcInfo.hProcess;
		CloseHandle(piProcInfo.hThread);
		return true;
		}

	// waits for process to end, and returns exit value
	// the process handle is also released by the function
	static DWORD WaitForProcess( HANDLE &hProcess )
		{
		DWORD exitcode;

		for(;;)
			{
			// check if process has ended
			GetExitCodeProcess( hProcess , &exitcode );
			if( exitcode != STILL_ACTIVE )
				{
				break;
				}

			// wait for it to signal. 
			WaitForSingleObject( hProcess , 1000 );
			}

		CloseHandle(hProcess);
		hProcess = INVALID_HANDLE_VALUE;
		return exitcode;
		}

#ifdef PUBLICBUILD

	class LicenseData
		{
		private:
			std::string License;
			std::string Application;
			std::string Platform;
			std::string Version;
			std::string LicenseKey;
			std::string Type;
			std::string ValidToDate;
			
			static std::string ExtractString( const char *text , const char *start_tag , const char *end_tag );

		public:
			LicenseData();
			~LicenseData();

			bool ReadLicense( std::basic_string<TCHAR> LicensePath );
			bool SetFromLicenseDataText( LPCTSTR LicenseDataText );
			void Clear();

			const char *GetLicense() const { return this->License.c_str(); }
			const char *GetApplication() const { return this->Application.c_str(); }
			const char *GetPlatform() const { return this->Platform.c_str(); }
			const char *GetVersion() const { return this->Version.c_str(); }
			const char *GetLicenseKey() const { return this->LicenseKey.c_str(); }
			const char *GetType() const { return this->Type.c_str(); }
			const char *GetValidToDate() const { return this->ValidToDate.c_str(); }

			bool IsLicenseForPlatform() const; // checks if the license is valid for this platform/application. 
			bool IsLicenseNodelocked() const; // checks for a nodelocked license
			bool IsValidDate() const; // checks if the date is a valid date
		};

	LicenseData::LicenseData()
		{
		}

	LicenseData::~LicenseData()
		{
		this->Clear();
		}
	
	void LicenseData::Clear()
		{
		this->License.clear();
		this->Application.clear();
		this->Platform.clear();
		this->Version.clear();
		this->LicenseKey.clear();
		this->Type.clear();
		this->ValidToDate.clear();
		}

	std::string LicenseData::ExtractString( const char *text , const char *start_tag , const char *end_tag )
		{
		if( text==NULL || start_tag==NULL || end_tag==NULL )
			return std::string("");

		// find beginning of data
		const char *pstart = strstr(text,start_tag);
		if( pstart==NULL )
			return std::string("");
		pstart += strlen(start_tag);

		// find end of data
		const char *pend = strstr(pstart,end_tag);
		if( pend==NULL )
			return std::string("");

		// extract the data
		std::string ret;
		ret.insert(0,pstart,(pend-pstart));
		return ret;
		}

	bool LicenseData::ReadLicense( std::basic_string<TCHAR> LicensePath )
		{
		FILE *fp = nullptr;
		this->Clear();

#if _MSC_VER >= 1400
		if( _tfopen_s(&fp,LicensePath.c_str(),_T("rb")) != 0 )
			{
			return false;
			}
#else
		fp = _tfopen( LicensePath.c_str(),_T("rb") );
		if( fp == nullptr )
			{
			return false;
			}
#endif

		int len = _filelength(fp->_file);
		char *dest = (char*)new char[len+1];
		if( fread(dest,1,len,fp) != len )
			{
			fclose(fp);
			delete[] dest;
			return false;
			}
		dest[len] = '\0';
		fclose(fp);

		// extract the data
		this->License = std::string(dest);
		this->Application = ExtractString( dest , "<Application>" , "</Application>" );
		this->Platform = ExtractString( dest , "<Platform>" , "</Platform>" );
		this->Version = ExtractString( dest , "<Version>" , "</Version>" );
		this->LicenseKey = ExtractString( dest , "<LicenseKey>" , "</LicenseKey>" );
		this->Type = ExtractString( dest , "<Type>" , "</Type>" );
		this->ValidToDate = ExtractString( dest , "<ValidToDate>" , "</ValidToDate>" );

		delete [] dest;
		return true;	
		}

	bool LicenseData::SetFromLicenseDataText( LPCTSTR LicenseDataText )
		{
		const char *dest = LPCTSTRToConstCharPtr( LicenseDataText );

		// extract the data
		this->License = std::string(dest);
		this->Application = ExtractString( dest , "<Application>" , "</Application>" );
		this->Platform = ExtractString( dest , "<Platform>" , "</Platform>" );
		this->Version = ExtractString( dest , "<Version>" , "</Version>" );
		this->LicenseKey = ExtractString( dest , "<LicenseKey>" , "</LicenseKey>" );
		this->Type = ExtractString( dest , "<Type>" , "</Type>" );
		this->ValidToDate = ExtractString( dest , "<ValidToDate>" , "</ValidToDate>" );

		free( (void*)dest );
		return true;	
		}
	
	bool LicenseData::IsLicenseForPlatform() const
		{
		if( this->Application != std::string("Simplygon") )
			return false;
		if( this->Platform != std::string("SDK") )
			return false;
		if( this->Version != std::string("5.0") )
			return false;
		return true;
		}


	bool LicenseData::IsLicenseNodelocked() const
		{
		return( this->Type == std::string("Nodelocked") );
		}

	static bool LoadLicense( bool only_common_folder , LicenseData &lic )
		{
		std::basic_string<TCHAR> LicensePath;

		// try the seach paths then local folder first, check if we have a license that is for this platform
		lic.Clear();
		if( !only_common_folder )
			{

			LicensePath = GetApplicationPath() + _T("Simplygon_5_license.dat");
			if( lic.ReadLicense( LicensePath ) )
				{
				if( lic.IsLicenseForPlatform() )
					{
					OutputDebugString( _T("Valid License Found:") );
					OutputDebugString( LicensePath.c_str() );
					return true;
					}
				}
			}

		// next, try the appdata folder
		lic.Clear();
		LicensePath = GetAppDataPath() + _T("Simplygon_5_license.dat");
		if( lic.ReadLicense( LicensePath ) )
			{
			if( lic.IsLicenseForPlatform() )
				{
				OutputDebugString( _T("Valid License Found:") );
				OutputDebugString( LicensePath.c_str() );
				return true;
				}
			}

		// next, try the common application folder
		lic.Clear();
		LicensePath = GetCommonAppPath() + _T("Simplygon_5_license.dat");
		if( lic.ReadLicense( LicensePath ) )
			{
			if( lic.IsLicenseForPlatform() )
				{
				OutputDebugString( _T("Valid License Found:") );
				OutputDebugString( LicensePath.c_str() );
				return true;
				}
			}

		// next, try the installation folder
		lic.Clear();
		LicensePath = GetInstallationPath() + _T("Simplygon_5_license.dat");
		if( lic.ReadLicense( LicensePath ) )
			{
			if( lic.IsLicenseForPlatform() )
				{
				OutputDebugString( _T("Valid License Found:") );
				OutputDebugString( LicensePath.c_str() );
				return true;
				}
			}

		//next try the additional search paths set in the loader
		lic.Clear();
		for( size_t i=0; i<AdditionalSearchPaths.size(); ++i )
			{
			LicensePath = AdditionalSearchPaths[i] + _T("Simplygon_5_license.dat");
			if( lic.ReadLicense( LicensePath ) )
				{
				if( lic.IsLicenseForPlatform() )
					{
					OutputDebugString( _T("Valid License Found:") );
					OutputDebugString( LicensePath.c_str() );
					return true;
					}
				}
			}

		// no license found
		return false;
		}

#endif//PUBLICBUILD

#endif//_WIN32

#if defined(__linux__) || defined(__APPLE__)

	typedef int (*LPINITIALIZESIMPLYGONSDK)( LPCTSTR license_data , SimplygonSDK::ISimplygonSDK **pInterfacePtr );
	typedef void (*LPDEINITIALIZESIMPLYGONSDK)();
	typedef void (*LPGETINTERFACEVERSIONSIMPLYGONSDK)( char *deststring );

	static LPINITIALIZESIMPLYGONSDK InitializeSimplygonSDKPtr; 
	static LPDEINITIALIZESIMPLYGONSDK DeinitializeSimplygonSDKPtr;
	static LPGETINTERFACEVERSIONSIMPLYGONSDK GetInterfaceVersionSimplygonSDKPtr;
	static void *hSO = NULL; // Handle to SimplygonSDK SO
	static int LoadError = SG_ERROR_INVALIDPARAM; // if the load failed, this contains the error

	// critical sections, process-local mutexes
	class rcriticalsection
		{
		private:
			pthread_mutex_t mutex;

		public:
			rcriticalsection() 
				{ 
				::pthread_mutexattr_t mutexattr;
				::pthread_mutexattr_init(&mutexattr);
#if defined(__linux__)
				::pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
#elif defined(__APPLE__)
				::pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
#endif
				::pthread_mutex_init(&mutex, &mutexattr);
				::pthread_mutexattr_destroy(&mutexattr);
				}
			~rcriticalsection() 
				{ 
				::pthread_mutex_destroy(&mutex); 
				}

			void Enter() { pthread_mutex_lock(&mutex); }
			void Leave() { pthread_mutex_unlock(&mutex); }
		};

	static bool dlErrorHandler()
		{
		char *dlError = dlerror();
		if(dlError) 
			{
			fprintf(stderr,"dlErrorHandler(): Error with dl function: %s\n", dlError);
			return false;
			}
		return true; // all ok
		}

	static bool LoadDynamicLibrary( LPCTSTR SDKPath )
		{
		if( SDKPath != NULL )
			{
			hSO = dlopen(SDKPath, RTLD_NOW);
			}
		else
			{
			hSO = dlopen("./libSimplygonSDK.so", RTLD_NOW);
			}

		if( !dlErrorHandler() )
			{
			return false;
			}

		// setup the function pointers
		InitializeSimplygonSDKPtr = (LPINITIALIZESIMPLYGONSDK)dlsym(hSO,"InitializeSimplygonSDK");
		DeinitializeSimplygonSDKPtr = (LPDEINITIALIZESIMPLYGONSDK)dlsym(hSO,"DeinitializeSimplygonSDK");
		GetInterfaceVersionSimplygonSDKPtr = (LPGETINTERFACEVERSIONSIMPLYGONSDK)dlsym(hSO,"GetInterfaceVersionSimplygonSDK");
		if( InitializeSimplygonSDKPtr==NULL || 
			DeinitializeSimplygonSDKPtr==NULL ||
			GetInterfaceVersionSimplygonSDKPtr==NULL )
			{
			dlErrorHandler();
			fprintf(stderr,"LoadDynamicLibrary(): Failed to retrieve all pointers.\n");
			return false;
			}
		return true;
		}

	static bool DynamicLibraryIsLoaded()
		{
		if(hSO == NULL)
			{
			return false;
			}	
		return true;
		}

	static void UnloadDynamicLibrary()
		{
		dlclose(hSO);
		hSO = NULL;	
		}

#endif //__linux__


	int InitializeSimplygonSDK( SimplygonSDK::ISimplygonSDK **pInterfacePtr , LPCTSTR SDKPath , LPCTSTR LicenseDataText )
		{
#ifdef _WIN32
		LicenseData lic;
#endif

		// load the library
		if( !LoadDynamicLibrary(SDKPath) )
			{
			return LoadError;
			}	

		// read the license file from the installation path
#if defined(_WIN32) && defined(PUBLICBUILD)
		bool license_load_succeeded = false;
		if( LicenseDataText )
			{
			license_load_succeeded = lic.SetFromLicenseDataText( LicenseDataText );
			}
		else
			{
			license_load_succeeded = LoadLicense(false,lic);
			}
#endif

#ifdef _WIN32
		// call the initialization function
		int retval = InitializeSimplygonSDKPtr(lic.GetLicense(),pInterfacePtr);
		
		if( retval == 0 )
			{
			TCHAR buffer[400];
			LPTSTR versionstring = ConstCharPtrToLPTSTR((*pInterfacePtr)->GetVersion());
#if _MSC_VER >= 1400
			_stprintf_s(buffer,MAX_PATH,_T("Simplygon initialized and running.\n\tVersion: %s\n"),versionstring);
#else
			_stprintf(buffer,_T("Simplygon initialized and running.\n\tVersion: %s\n"),versionstring);
#endif
			OutputDebugString(buffer);
			free(versionstring);
			}
		
#elif defined(__linux__) || defined(__APPLE__)
		int retval = InitializeSimplygonSDKPtr("",pInterfacePtr);
		if( retval == 0 )
			{
			char buffer[400];
			sprintf(buffer, _T("Simplygon initialized and running.\n\tVersion: %s\n"), (*pInterfacePtr)->GetVersion());
			std::cerr << buffer << std::endl;
			}
#endif
		
		// return retval
		return retval;
		}

	void DeinitializeSimplygonSDK()
		{
		if( !DynamicLibraryIsLoaded() )
			{
			return;
			}
		DeinitializeSimplygonSDKPtr();
		UnloadDynamicLibrary();
		}

	////////////////////////////////////////////////////////////////////////////////////


	static rcriticalsection init_guard; // only one thread is allowed to run init/deinit at any time
	static int init_count = 0; // a reference count of the number of Init/Deinits
	static ISimplygonSDK *InterfacePtr; // the pointer to the only interface

#ifdef _WIN32
	void AddSearchPath( LPCTSTR search_path )
		{
		TCHAR full[MAX_PATH];
		if( _tfullpath( full, search_path, MAX_PATH ) != NULL )
			{
			AdditionalSearchPaths.push_back( full );
			}

		}

	void ClearAdditionalSearchPaths()
		{
		AdditionalSearchPaths.clear();
#if _HAS_CPP0X
		// make it release its memory, so this does not give a false positive in memory leak detection
		AdditionalSearchPaths.shrink_to_fit();
#endif
		}
#endif //_WIN32

	int Initialize( ISimplygonSDK **pInterfacePtr , LPCTSTR SDKPath , LPCTSTR LicenseDataText )
		{
		int retval = SG_ERROR_NOERROR;

		// if the interface does not exist, init it
		if( InterfacePtr == NULL )
			{
			init_count = 0;
			retval = InitializeSimplygonSDK( &InterfacePtr , SDKPath , LicenseDataText );
			}

		// if everything is ok, add a reference
		if( retval == SG_ERROR_NOERROR )
			{
			++init_count;
			*pInterfacePtr = InterfacePtr;
			}

		return retval;
		}

	void Deinitialize()
		{
		// release a reference
		--init_count;

		// if the reference is less or equal to 0, release the interface 
		if( init_count <= 0 )
			{
			// only release if one exists
			if( InterfacePtr != NULL )
				{
				DeinitializeSimplygonSDK();
				InterfacePtr = NULL;
				}

			// make sure the init_count is 0
			init_count = 0;
			}
		}

	LPCTSTR GetError( int error_code )
		{
		switch( error_code ) 
			{
			case SG_ERROR_NOERROR:
				return _T("No error");
			case SG_ERROR_NOLICENSE:
				return _T("No license was found, or the license is not valid. Please install a valid license.");
			case SG_ERROR_NOTINITIALIZED:
				return _T("The SDK is not initialized, or no process object is loaded.");
			case SG_ERROR_ALREADYINITIALIZED:
				return _T("The SDK is already initialized. Please call Deinitialize() before calling Initialize() again.");
			case SG_ERROR_FILENOTFOUND:
				return _T("The specified file was not found.");
			case SG_ERROR_INVALIDPARAM:
				return _T("An invalid parameter was passed to the method");
			case SG_ERROR_WRONGVERSION:
				return _T("The Simplygon DLL and header file interface versions do not match");
			case SG_ERROR_LOADFAILED:
				return _T("the Simplygon DLL failed loading, probably because of a missing dependency");
			default:
				return _T("Extended Error Code. Contact support@simplygon.com");
			}
		}

#ifdef _WIN32
	int PollLog( LPTSTR dest , int max_len_dest )
		{
#if _MSC_VER >= 1400
		if( dest == nullptr || max_len_dest == 0 || PollLogSimplygonSDKPtr == nullptr )
			{
			return 0;
			}
#else
		if( dest == NULL || max_len_dest == 0 || PollLogSimplygonSDKPtr == NULL )
			{
			return 0;
			}
#endif
		int sz;

#ifdef UNICODE
		char *tmp = new char[max_len_dest];
		PollLogSimplygonSDKPtr(tmp,max_len_dest);
		size_t cnv_sz;
		mbstowcs_s( 
			&cnv_sz,
			dest,
			max_len_dest,
			tmp,
			_TRUNCATE
			);
		delete [] tmp;
		sz = (int)cnv_sz;
#else
		sz = PollLogSimplygonSDKPtr(dest,max_len_dest);
#endif//UNICODE	

		return sz;
		}

	int RunLicenseWizard( LPCTSTR batch_file )
		{
		// find process
		std::basic_string<TCHAR> path = FindNamedProcess( _T("LicenseApplication.exe") );
		if( path.empty() )
			{
			return SG_ERROR_FILENOTFOUND;
			}

		// run process
		HANDLE hProcess;
		bool succ = ExecuteProcess(path.c_str(),batch_file,&hProcess);
		if( !succ )
			{
			return SG_ERROR_LOADFAILED;
			}

		// wait for it to end
		return WaitForProcess(hProcess);
		}

#endif //_WIN32

	// end of namespace
	};

