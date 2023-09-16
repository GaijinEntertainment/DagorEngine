// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.

#ifndef __ATLCORE_H__
#define __ATLCORE_H__

#pragma once

#ifdef _ATL_ALL_WARNINGS
#pragma warning( push )
#endif

#pragma warning(disable: 4786) // identifier was truncated in the debug information

#include <atldef.h>
#include <windows.h>

#include <limits.h>
#include <tchar.h>

#include <atlexcept.h>
#include <atlsimpcoll.h>


namespace ATL
{
/////////////////////////////////////////////////////////////////////////////
// Verify that a null-terminated string points to valid memory
inline BOOL AtlIsValidString(LPCWSTR psz, size_t nMaxLength = INT_MAX)
{
#if defined(_DEBUG)
	// Implement ourselves because ::IsBadStringPtrW() isn't implemented on Win9x.
	if ((psz == NULL) || (nMaxLength == 0))
		return FALSE;

	LPCWSTR pch;
	LPCWSTR pchEnd;
	__try
	{
		wchar_t ch;

		pch = psz;
		pchEnd = psz+nMaxLength-1;
		ch = *(volatile wchar_t*)pch;
		while ((ch != L'\0') && (pch != pchEnd))
		{
			pch++;
			ch = *(volatile wchar_t*)pch;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return FALSE;
	}

	return TRUE;
#else
	(nMaxLength);
	return (psz != NULL);
#endif	
}

// Verify that a null-terminated string points to valid memory
inline BOOL AtlIsValidString(LPCSTR psz, size_t nMaxLength = UINT_MAX)
{
#if defined(_DEBUG)
	if (psz == NULL)
		return FALSE;
	return ::IsBadStringPtrA(psz, nMaxLength) == 0;
#else
	(nMaxLength);
	return (psz != NULL);
#endif
}

// Verify that a pointer points to valid memory
inline BOOL AtlIsValidAddress(const void* p, size_t nBytes,
	BOOL bReadWrite = TRUE)
{
#if defined(_DEBUG)
	return ((p != NULL) && !IsBadReadPtr(p, nBytes) &&
		(!bReadWrite || !IsBadWritePtr(const_cast<LPVOID>(p), nBytes)));
#else
	nBytes,bReadWrite;
	return (p != NULL);
#endif
}

template<typename T>
inline void AtlAssertValidObject(const T *pOb)
{
	ATLASSERT(pOb);
	ATLASSERT(AtlIsValidAddress(pOb, sizeof(T)));
	if(pOb)
		pOb->AssertValid();
}
#ifdef _DEBUG
#define ATLASSERT_VALID(x) ATL::AtlAssertValidObject(x)
#else
#define ATLASSERT_VALID(x) __noop;
#endif

// COM Sync Classes
class CComCriticalSection
{
public:
	CComCriticalSection() throw()
	{
		memset(&m_sec, 0, sizeof(CRITICAL_SECTION));
	}
	HRESULT Lock() throw()
	{
		EnterCriticalSection(&m_sec);
		return S_OK;
	}
	HRESULT Unlock() throw()
	{
		LeaveCriticalSection(&m_sec);
		return S_OK;
	}
	HRESULT Init() throw()
	{
		HRESULT hRes = S_OK;
		__try
		{
			InitializeCriticalSection(&m_sec);
		}
		// structured exception may be raised in low memory situations
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			if (STATUS_NO_MEMORY == GetExceptionCode())
				hRes = E_OUTOFMEMORY;
			else
				hRes = E_FAIL;
		}
		return hRes;
	}

	HRESULT Term() throw()
	{
		DeleteCriticalSection(&m_sec);
		return S_OK;
	}	
	CRITICAL_SECTION m_sec;
};

class CComAutoCriticalSection : public CComCriticalSection
{
public:
	CComAutoCriticalSection()
	{
		HRESULT hr = CComCriticalSection::Init();
		if (FAILED(hr))
			AtlThrow(hr);
	}
	~CComAutoCriticalSection() throw()
	{
		CComCriticalSection::Term();
	}
private :
	HRESULT Init();	// Not implemented. CComAutoCriticalSection::Init should never be called
	HRESULT Term(); // Not implemented. CComAutoCriticalSection::Term should never be called
};

class CComFakeCriticalSection
{
public:
	HRESULT Lock() throw() { return S_OK; }
	HRESULT Unlock() throw() { return S_OK; }
	HRESULT Init() throw() { return S_OK; }
	HRESULT Term() throw() { return S_OK; }
};

/////////////////////////////////////////////////////////////////////////////
// Module 

// Used by any project that uses ATL
struct _ATL_BASE_MODULE70
{
	UINT cbSize;
	HINSTANCE m_hInst;
	HINSTANCE m_hInstResource;
	bool m_bNT5orWin98;
	DWORD dwAtlBuildVer;
	const GUID* pguidVer;
	CComCriticalSection m_csResource;
	CSimpleArray<HINSTANCE> m_rgResourceInstance;
};
typedef _ATL_BASE_MODULE70 _ATL_BASE_MODULE;

class CAtlBaseModule : public _ATL_BASE_MODULE
{
public :
	static bool m_bInitFailed;
	CAtlBaseModule() throw();
	~CAtlBaseModule() throw ();

	HINSTANCE GetModuleInstance() throw()
	{
		return m_hInst;
	}
	HINSTANCE GetResourceInstance() throw()
	{
		return m_hInstResource;
	}
	HINSTANCE SetResourceInstance(HINSTANCE hInst) throw()
	{
		return static_cast< HINSTANCE >(InterlockedExchangePointer((void**)&m_hInstResource, hInst));
	}

	bool AddResourceInstance(HINSTANCE hInst) throw();
	bool RemoveResourceInstance(HINSTANCE hInst) throw();
	HINSTANCE GetHInstanceAt(int i) throw();
};

__declspec(selectany) bool CAtlBaseModule::m_bInitFailed = false;
extern CAtlBaseModule _AtlBaseModule;

/////////////////////////////////////////////////////////////////////////////
// String resource helpers

#pragma warning(push)
#pragma warning(disable: 4200)
	struct ATLSTRINGRESOURCEIMAGE
	{
		WORD nLength;
		WCHAR achString[];
	};
#pragma warning(pop)	// C4200

inline const ATLSTRINGRESOURCEIMAGE* _AtlGetStringResourceImage( HINSTANCE hInstance, HRSRC hResource, UINT id ) throw()
{
	const ATLSTRINGRESOURCEIMAGE* pImage;
	const ATLSTRINGRESOURCEIMAGE* pImageEnd;
	ULONG nResourceSize;
	HGLOBAL hGlobal;
	UINT iIndex;

	hGlobal = ::LoadResource( hInstance, hResource );
	if( hGlobal == NULL )
	{
		return( NULL );
	}

	pImage = (const ATLSTRINGRESOURCEIMAGE*)::LockResource( hGlobal );
	if( pImage == NULL )
	{
		return( NULL );
	}

	nResourceSize = ::SizeofResource( hInstance, hResource );
	pImageEnd = (const ATLSTRINGRESOURCEIMAGE*)(LPBYTE( pImage )+nResourceSize);
	iIndex = id&0x000f;

	while( (iIndex > 0) && (pImage < pImageEnd) )
	{
		pImage = (const ATLSTRINGRESOURCEIMAGE*)(LPBYTE( pImage )+(sizeof( ATLSTRINGRESOURCEIMAGE )+(pImage->nLength*sizeof( WCHAR ))));
		iIndex--;
	}
	if( pImage >= pImageEnd )
	{
		return( NULL );
	}
	if( pImage->nLength == 0 )
	{
		return( NULL );
	}

	return( pImage );
}

inline const ATLSTRINGRESOURCEIMAGE* AtlGetStringResourceImage( HINSTANCE hInstance, UINT id ) throw()
{
	HRSRC hResource;

	hResource = ::FindResource( hInstance, MAKEINTRESOURCE( ((id>>4)+1) ), RT_STRING );
	if( hResource == NULL )
	{
		return( NULL );
	}

	return _AtlGetStringResourceImage( hInstance, hResource, id );
}

inline const ATLSTRINGRESOURCEIMAGE* AtlGetStringResourceImage( HINSTANCE hInstance, UINT id, WORD wLanguage ) throw()
{
	HRSRC hResource;

	hResource = ::FindResourceEx( hInstance, RT_STRING, MAKEINTRESOURCE( ((id>>4)+1) ), wLanguage );
	if( hResource == NULL )
	{
		return( NULL );
	}

	return _AtlGetStringResourceImage( hInstance, hResource, id );
}

inline const ATLSTRINGRESOURCEIMAGE* AtlGetStringResourceImage( UINT id ) throw()
{
	const ATLSTRINGRESOURCEIMAGE* p = NULL;
	HINSTANCE hInst = _AtlBaseModule.GetHInstanceAt(0);

	for (int i = 1; hInst != NULL && p == NULL; hInst = _AtlBaseModule.GetHInstanceAt(i++))
	{
		p = AtlGetStringResourceImage(hInst, id);
	}
	return p;
}

inline const ATLSTRINGRESOURCEIMAGE* AtlGetStringResourceImage( UINT id, WORD wLanguage ) throw()
{
	const ATLSTRINGRESOURCEIMAGE* p = NULL;
	HINSTANCE hInst = _AtlBaseModule.GetHInstanceAt(0);

	for (int i = 1; hInst != NULL && p == NULL; hInst = _AtlBaseModule.GetHInstanceAt(i++))
	{
		p = AtlGetStringResourceImage(hInst, id, wLanguage);
	}
	return p;
}

inline int AtlLoadString(UINT nID, LPTSTR lpBuffer, int nBufferMax) throw()
{
	HINSTANCE hInst = _AtlBaseModule.GetHInstanceAt(0);
	int nRet = 0;

	for (int i = 1; hInst != NULL && nRet == 0; hInst = _AtlBaseModule.GetHInstanceAt(i++))
	{
		nRet = LoadString(hInst, nID, lpBuffer, nBufferMax);
	}
	return nRet;
}

inline HINSTANCE AtlFindResourceInstance(LPCTSTR lpName, LPCTSTR lpType, WORD wLanguage = 0) throw()
{
	ATLASSERT(lpType != RT_STRING);	// Call AtlFindStringResourceInstance to find the string
	if (lpType == RT_STRING)
		return NULL;

	if (IS_INTRESOURCE(lpType))
	{
		if (lpType == RT_ICON)
		{
			lpType = RT_GROUP_ICON;
		}
		else if (lpType == RT_CURSOR)
		{
			lpType = RT_GROUP_CURSOR;
		}
	}

	HINSTANCE hInst = _AtlBaseModule.GetHInstanceAt(0);
	HRSRC hResource = NULL;

	for (int i = 1; hInst != NULL; hInst = _AtlBaseModule.GetHInstanceAt(i++))
	{
		hResource = ::FindResourceEx(hInst, lpType, lpName, wLanguage);
		if (hResource != NULL)
		{
			return hInst;
		}
	}

	return NULL;
}

inline HINSTANCE AtlFindResourceInstance(UINT nID, LPCTSTR lpType, WORD wLanguage = 0) throw()
{
	return AtlFindResourceInstance(MAKEINTRESOURCE(nID), lpType, wLanguage);
}

inline HINSTANCE AtlFindStringResourceInstance(UINT nID, WORD wLanguage = 0) throw()
{
	const ATLSTRINGRESOURCEIMAGE* p = NULL;
	HINSTANCE hInst = _AtlBaseModule.GetHInstanceAt(0);

	for (int i = 1; hInst != NULL && p == NULL; hInst = _AtlBaseModule.GetHInstanceAt(i++))
	{
		p = AtlGetStringResourceImage(hInst, nID, wLanguage);
		if (p != NULL)
			return hInst;
	}

	return NULL;
}

}	// namespace ATL

#ifdef _ATL_ALL_WARNINGS
#pragma warning( pop )
#endif

#endif	// __ATLCORE_H__
