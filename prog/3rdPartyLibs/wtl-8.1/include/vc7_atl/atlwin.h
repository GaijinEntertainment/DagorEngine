// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning (push)
#pragma warning(disable : 4668)	// is not defined as a preprocessor macro, replacing with '0' for '#if/#elif
#pragma warning(disable : 4820)	// padding added after member
#endif //!_ATL_NO_PRAGMA_WARNINGS

#ifndef __ATLWIN_H__
#define __ATLWIN_H__

#pragma once

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLBASE_H__
	#error atlwin.h requires atlbase.h to be included first
#endif

#include <commctrl.h>
#include <atlsimpstr.h>

// protect template members from windowsx.h macros
#ifdef _INC_WINDOWSX
#undef SubclassWindow
#endif //_INC_WINDOWSX

#ifdef SetWindowLongPtrA
#undef SetWindowLongPtrA
inline LONG_PTR SetWindowLongPtrA( HWND hWnd, int nIndex, LONG_PTR dwNewLong )
{
	return( ::SetWindowLongA( hWnd, nIndex, LONG( dwNewLong ) ) );
}
#endif

#ifdef SetWindowLongPtrW
#undef SetWindowLongPtrW
inline LONG_PTR SetWindowLongPtrW( HWND hWnd, int nIndex, LONG_PTR dwNewLong )
{
	return( ::SetWindowLongW( hWnd, nIndex, LONG( dwNewLong ) ) );
}
#endif

#ifdef GetWindowLongPtrA
#undef GetWindowLongPtrA
inline LONG_PTR GetWindowLongPtrA( HWND hWnd, int nIndex )
{
	return( ::GetWindowLongA( hWnd, nIndex ) );
}
#endif

#ifdef GetWindowLongPtrW
#undef GetWindowLongPtrW
inline LONG_PTR GetWindowLongPtrW( HWND hWnd, int nIndex )
{
	return( ::GetWindowLongW( hWnd, nIndex ) );
}
#endif

//#ifndef _ATL_DLL_IMPL
namespace ATL
{
//#endif

/////////////////////////////////////////////////////////////////////////////
// Dual argument helper classes

class _U_RECT
{
public:
	_U_RECT(LPRECT lpRect) : m_lpRect(lpRect)
	{ }
	_U_RECT(RECT& rc) : m_lpRect(&rc)
	{ }
	LPRECT m_lpRect;
};

class _U_MENUorID
{
public:
	_U_MENUorID(HMENU hMenu) : m_hMenu(hMenu)
	{ }
	_U_MENUorID(UINT nID) : m_hMenu((HMENU)(UINT_PTR)nID)
	{ }	
	HMENU m_hMenu;
};

class _U_STRINGorID
{
public:
	_U_STRINGorID(LPCTSTR lpString) : m_lpstr(lpString)
	{ }
	_U_STRINGorID(UINT nID) : m_lpstr(MAKEINTRESOURCE(nID))
	{ }
	LPCTSTR m_lpstr;
};

struct _ATL_WNDCLASSINFOA;
struct _ATL_WNDCLASSINFOW;

ATLAPI_(ATOM) AtlWinModuleRegisterWndClassInfoA(_ATL_WIN_MODULE* pWinModule, _ATL_BASE_MODULE* pBaseModule, _ATL_WNDCLASSINFOA* p, WNDPROC* pProc);
inline ATOM AtlModuleRegisterWndClassInfoA(_ATL_MODULE* /*pM*/, _ATL_WNDCLASSINFOA* p, WNDPROC* pProc)
{
	return AtlWinModuleRegisterWndClassInfoA(&_AtlWinModule, &_AtlBaseModule, p, pProc);
}

ATLAPI_(ATOM) AtlWinModuleRegisterWndClassInfoW(_ATL_WIN_MODULE* pWinModule, _ATL_BASE_MODULE* pBaseModule, _ATL_WNDCLASSINFOW* p, WNDPROC* pProc);
inline ATOM AtlModuleRegisterWndClassInfoW(_ATL_MODULE* /*pM*/, _ATL_WNDCLASSINFOW* p, WNDPROC* pProc)
{
	return AtlWinModuleRegisterWndClassInfoW(&_AtlWinModule, &_AtlBaseModule, p, pProc);
}

ATLAPI_(ATOM) AtlWinModuleRegisterClassExA(_ATL_WIN_MODULE* pWinModule, const WNDCLASSEXA *lpwc);
inline ATOM AtlModuleRegisterClassExA(_ATL_MODULE* /*pM*/, const WNDCLASSEXA *lpwc)
{
	return AtlWinModuleRegisterClassExA(&_AtlWinModule, lpwc);
}

ATLAPI_(ATOM) AtlWinModuleRegisterClassExW(_ATL_WIN_MODULE* pWinModule, const WNDCLASSEXW *lpwc);
inline ATOM AtlModuleRegisterClassExW(_ATL_MODULE* /*pM*/, const WNDCLASSEXW *lpwc)
{
	return AtlWinModuleRegisterClassExW(&_AtlWinModule, lpwc);
}


#ifdef UNICODE
#define AtlWinModuleRegisterWndClassInfo AtlWinModuleRegisterWndClassInfoW
#define AtlWinModuleRegisterClassEx AtlWinModuleRegisterClassExW
#define AtlModuleRegisterWndClassInfo AtlModuleRegisterWndClassInfoW
#define AtlModuleRegisterClassEx AtlModuleRegisterClassExW
#else
#define AtlWinModuleRegisterWndClassInfo AtlWinModuleRegisterWndClassInfoA
#define AtlWinModuleRegisterClassEx AtlWinModuleRegisterClassExA
#define AtlModuleRegisterWndClassInfo AtlModuleRegisterWndClassInfoA
#define AtlModuleRegisterClassEx AtlModuleRegisterClassExA
#endif


#define HIMETRIC_PER_INCH   2540
#define MAP_PIX_TO_LOGHIM(x,ppli)   MulDiv(HIMETRIC_PER_INCH, (x), (ppli))
#define MAP_LOGHIM_TO_PIX(x,ppli)   MulDiv((ppli), (x), HIMETRIC_PER_INCH)

ATLAPI_(HDC) AtlCreateTargetDC(HDC hdc, DVTARGETDEVICE* ptd);
ATLAPI_(void) AtlHiMetricToPixel(const SIZEL * lpSizeInHiMetric, LPSIZEL lpSizeInPix);
ATLAPI_(void) AtlPixelToHiMetric(const SIZEL * lpSizeInPix, LPSIZEL lpSizeInHiMetric);

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lParam)	((int)(short)LOWORD(lParam))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lParam)	((int)(short)HIWORD(lParam))
#endif


//#ifndef _ATL_DLL_IMPL
}; //namespace ATL
//#endif

namespace ATL
{

/////////////////////////////////////////////////////////////////////////////
// _ATL_MSG - extended MSG structure

struct _ATL_MSG : public MSG
{
public:
// Additional data members
	int cbSize;
	BOOL bHandled;

// Constructors
	_ATL_MSG() : cbSize(sizeof(_ATL_MSG)), bHandled(TRUE)
	{
		hwnd = NULL;
		message = 0;
		wParam = 0;
		lParam = 0;
		time = 0;
		pt.x = pt.y = 0;
	}
	_ATL_MSG(HWND hWnd, UINT uMsg, WPARAM wParamIn, LPARAM lParamIn, DWORD dwTime, POINT ptIn, BOOL bHandledIn) : cbSize(sizeof(_ATL_MSG)), bHandled(bHandledIn)
	{
		hwnd = hWnd;
		message = uMsg;
		wParam = wParamIn;
		lParam = lParamIn;
		time = dwTime;
		pt = ptIn;
	}
	_ATL_MSG(HWND hWnd, UINT uMsg, WPARAM wParamIn, LPARAM lParamIn, BOOL bHandledIn = TRUE) : cbSize(sizeof(_ATL_MSG)), bHandled(bHandledIn)
	{
		hwnd = hWnd;
		message = uMsg;
		wParam = wParamIn;
		lParam = lParamIn;
		time = 0;
		pt.x = pt.y = 0;
	}
	_ATL_MSG(MSG& msg, BOOL bHandledIn = TRUE) : cbSize(sizeof(_ATL_MSG)), bHandled(bHandledIn)
	{
		hwnd = msg.hwnd;
		message = msg.message;
		wParam = msg.wParam;
		lParam = msg.lParam;
		time = msg.time;
		pt = msg.pt;
	}
};

struct _ATL_WNDCLASSINFOA
{
	WNDCLASSEXA m_wc;
	LPCSTR m_lpszOrigName;
	WNDPROC pWndProc;
	LPCSTR m_lpszCursorID;
	BOOL m_bSystemCursor;
	ATOM m_atom;
	CHAR m_szAutoName[5+sizeof(void*)*CHAR_BIT];
	ATOM Register(WNDPROC* p)
	{
		return AtlWinModuleRegisterWndClassInfoA(&_AtlWinModule, &_AtlBaseModule, this, p);
	}
};

struct _ATL_WNDCLASSINFOW
{
	WNDCLASSEXW m_wc;
	LPCWSTR m_lpszOrigName;
	WNDPROC pWndProc;
	LPCWSTR m_lpszCursorID;
	BOOL m_bSystemCursor;
	ATOM m_atom;
	WCHAR m_szAutoName[5+sizeof(void*)*CHAR_BIT];
	ATOM Register(WNDPROC* p)
	{
		return AtlWinModuleRegisterWndClassInfoW(&_AtlWinModule, &_AtlBaseModule, this, p);
	}
};

};  // namespace ATL

namespace ATL
{

/////////////////////////////////////////////////////////////////////////////
// Forward declarations

class CWindow;
#ifndef _ATL_NO_HOSTING
template <class TBase = CWindow> class CAxWindowT;
template <class TBase = CWindow> class CAxWindow2T;
#endif //!_ATL_NO_HOSTING
class CMessageMap;
class CDynamicChain;
typedef _ATL_WNDCLASSINFOA CWndClassInfoA;
typedef _ATL_WNDCLASSINFOW CWndClassInfoW;
#ifdef UNICODE
#define CWndClassInfo CWndClassInfoW
#else
#define CWndClassInfo CWndClassInfoA
#endif
template <class T, class TBase = CWindow, class TWinTraits = CControlWinTraits> class CWindowImpl;
template <class T, class TBase = CWindow, class TWinTraits = CControlWinTraits> class CWindowWithReflectorImpl;
template <class T, class TBase = CWindow> class CDialogImpl;
#ifndef _ATL_NO_HOSTING
template <class T, class TBase = CWindow> class CAxDialogImpl;
#endif //!_ATL_NO_HOSTING
template <WORD t_wDlgTemplateID, BOOL t_bCenter = TRUE> class CSimpleDialog;
template <class TBase = CWindow, class TWinTraits = CControlWinTraits> class CContainedWindowT;

/////////////////////////////////////////////////////////////////////////////
// Helper functions for cracking dialog templates

class _DialogSplitHelper
{
public:
	// Constants used in DLGINIT resources for OLE control containers
	// NOTE: These are NOT real Windows messages they are simply tags
	// used in the control resource and are never used as 'messages'
	enum
	{
		ATL_WM_OCC_LOADFROMSTREAM = 0x0376,
		ATL_WM_OCC_LOADFROMSTORAGE = 0x0377,
		ATL_WM_OCC_INITNEW = 0x0378,
		ATL_WM_OCC_LOADFROMSTREAM_EX = 0x037A,
		ATL_WM_OCC_LOADFROMSTORAGE_EX = 0x037B,
		ATL_DISPID_DATASOURCE = 0x80010001,
		ATL_DISPID_DATAFIELD = 0x80010002,
	};

//local struct used for implementation
#pragma pack(push, 1)
	struct DLGINITSTRUCT
	{
		WORD nIDC;
		WORD message;
		DWORD dwSize;
	};
	struct DLGTEMPLATEEX
	{
		WORD dlgVer;
		WORD signature;
		DWORD helpID;
		DWORD exStyle;
		DWORD style;
		WORD cDlgItems;
		short x;
		short y;
		short cx;
		short cy;

		// Everything else in this structure is variable length,
		// and therefore must be determined dynamically

		// sz_Or_Ord menu;			// name or ordinal of a menu resource
		// sz_Or_Ord windowClass;	// name or ordinal of a window class
		// WCHAR title[titleLen];	// title string of the dialog box
		// short pointsize;			// only if DS_SETFONT is set
		// short weight;			// only if DS_SETFONT is set
		// short bItalic;			// only if DS_SETFONT is set
		// WCHAR font[fontLen];		// typeface name, if DS_SETFONT is set
	};
	struct DLGITEMTEMPLATEEX
	{
		DWORD helpID;
		DWORD exStyle;
		DWORD style;
		short x;
		short y;
		short cx;
		short cy;
		DWORD id;

		// Everything else in this structure is variable length,
		// and therefore must be determined dynamically

		// sz_Or_Ord windowClass;	// name or ordinal of a window class
		// sz_Or_Ord title;			// title string or ordinal of a resource
		// WORD extraCount;			// bytes following creation data
	};
#pragma pack(pop)

	static BOOL IsDialogEx(const DLGTEMPLATE* pTemplate)
	{
		return ((DLGTEMPLATEEX*)pTemplate)->signature == 0xFFFF;
	}

	inline static WORD& DlgTemplateItemCount(DLGTEMPLATE* pTemplate)
	{
		if (IsDialogEx(pTemplate))
			return reinterpret_cast<DLGTEMPLATEEX*>(pTemplate)->cDlgItems;
		else
			return pTemplate->cdit;
	}

	inline static const WORD& DlgTemplateItemCount(const DLGTEMPLATE* pTemplate)
	{
		if (IsDialogEx(pTemplate))
			return reinterpret_cast<const DLGTEMPLATEEX*>(pTemplate)->cDlgItems;
		else
			return pTemplate->cdit;
	}

	static DLGITEMTEMPLATE* FindFirstDlgItem(const DLGTEMPLATE* pTemplate)
	{
		BOOL bDialogEx = IsDialogEx(pTemplate);

		WORD* pw;
		DWORD dwStyle;
		if (bDialogEx)
		{
			pw = (WORD*)((DLGTEMPLATEEX*)pTemplate + 1);
			dwStyle = ((DLGTEMPLATEEX*)pTemplate)->style;
		}
		else
		{
			pw = (WORD*)(pTemplate + 1);
			dwStyle = pTemplate->style;
		}

		// Check for presence of menu and skip it if there is one
		// 0x0000 means there is no menu
		// 0xFFFF means there is a menu ID following
		// Everything else means that this is a NULL terminated Unicode string
		// which identifies the menu resource
		if (*pw == 0xFFFF)
			pw += 2;				// Has menu ID, so skip 2 words
		else
			while (*pw++);			// Either No menu, or string, skip past terminating NULL

		// Check for presence of class name string
		// 0x0000 means "Use system dialog class name"
		// 0xFFFF means there is a window class (atom) specified
		// Everything else means that this is a NULL terminated Unicode string
		// which identifies the menu resource
		if (*pw == 0xFFFF)
			pw += 2;				// Has class atom, so skip 2 words
		else
			while (*pw++);			// Either No class, or string, skip past terminating NULL

		// Skip caption string
		while (*pw++);

		// If we have DS_SETFONT, there is extra font information which we must now skip
		if (dwStyle & DS_SETFONT)
		{
			// If it is a regular DLGTEMPLATE there is only a short for the point size
			// and a string specifying the font (typefacename).  If this is a DLGTEMPLATEEX
			// then there is also the font weight, and bItalic which must be skipped
			if (bDialogEx)
				pw += 3;			// Skip font size, weight, (italic, charset)
			else
				pw += 1;			// Skip font size
			while (*pw++);			// Skip typeface name
		}

		// Dword-align and return
		return (DLGITEMTEMPLATE*)(((DWORD_PTR)pw + 3) & ~3);
	}

	// Given the current dialog item and whether this is an extended dialog
	// return a pointer to the next DLGITEMTEMPLATE*
	static DLGITEMTEMPLATE* FindNextDlgItem(DLGITEMTEMPLATE* pItem, BOOL bDialogEx)
	{
		WORD* pw;

		// First skip fixed size header information, size of which depends
		// if this is a DLGITEMTEMPLATE or DLGITEMTEMPLATEEX
		if (bDialogEx)
			pw = (WORD*)((DLGITEMTEMPLATEEX*)pItem + 1);
		else
			pw = (WORD*)(pItem + 1);

		if (*pw == 0xFFFF)			// Skip class name ordinal or string
			pw += 2; // (WORDs)
		else
			while (*pw++);

		if (*pw == 0xFFFF)			// Skip title ordinal or string
			pw += 2; // (WORDs)
		else
			while (*pw++);

		WORD cbExtra = *pw++;		// Skip extra data

		// cbExtra includes the size WORD in DIALOG resource.
		if (cbExtra != 0 && !bDialogEx)
			cbExtra -= 2;

		// Dword-align and return
		return (DLGITEMTEMPLATE*)(((DWORD_PTR)pw + cbExtra + 3) & ~3);
	}

	// Find the initialization data (Stream) for the control specified by the ID
	// If found, return the pointer into the data and the length of the data
	static DWORD FindCreateData(DWORD dwID, BYTE* pInitData, BYTE** pData)
	{
		while (pInitData)
		{
			// Read the DLGINIT header
			WORD nIDC = *((UNALIGNED WORD*)pInitData);
			pInitData += sizeof(WORD);
			BYTE* pTemp = pInitData;
			WORD nMsg = *((UNALIGNED WORD*)pInitData);
			pInitData += sizeof(WORD);
			DWORD dwLen = *((UNALIGNED DWORD*)pInitData);
			pInitData += sizeof(DWORD);

			// If the header is for the control specified get the other info
			if (nIDC == dwID)
			{
				if (nMsg == (WORD)ATL_WM_OCC_INITNEW)
				{
					ATLASSERT(dwLen == 0);
					return 0;
				}
				*pData = pTemp;
				return dwLen + sizeof(WORD) + sizeof(DWORD);
			}

			// It's not the right control, skip past data
			pInitData += dwLen;
		}
		return 0;
	}

	static bool IsActiveXControl(DLGITEMTEMPLATE* pItem, BOOL bDialogEx)
	{
		LPWSTR pszClassName;
		pszClassName = bDialogEx ?
			(LPWSTR)(((DLGITEMTEMPLATEEX*)pItem) + 1) :
			(LPWSTR)(pItem + 1);
		if (pszClassName[0] == L'{')
			return true;
		return false;
	}

	// Convert MSDEV (MFC) style DLGTEMPLATE with controls to regular DLGTEMPLATE
	static DLGTEMPLATE* SplitDialogTemplate(DLGTEMPLATE* pTemplate, BYTE* /*pInitData*/)
	{
		// Calculate the size of the DLGTEMPLATE for allocating the new one
		DLGITEMTEMPLATE* pFirstItem = FindFirstDlgItem(pTemplate);
		ULONG_PTR cbHeader = (BYTE*)pFirstItem - (BYTE*)pTemplate;
		ULONG_PTR cbNewTemplate = cbHeader;

		BOOL bDialogEx = IsDialogEx(pTemplate);

		int iItem;
		int nItems = (int)DlgTemplateItemCount(pTemplate);
		LPWSTR pszClassName;
		BOOL bHasOleControls = FALSE;

		// Make first pass through the dialog template.  On this pass, we're
		// interested in determining:
		//    1. Does this template contain any ActiveX Controls?
		//    2. If so, how large a buffer is needed for a template containing
		//       only the non-OLE controls?

		DLGITEMTEMPLATE* pItem = pFirstItem;
		DLGITEMTEMPLATE* pNextItem = pItem;
		for (iItem = 0; iItem < nItems; iItem++)
		{
			pNextItem = FindNextDlgItem(pItem, bDialogEx);

			pszClassName = bDialogEx ?
				(LPWSTR)(((DLGITEMTEMPLATEEX*)pItem) + 1) :
				(LPWSTR)(pItem + 1);

			// Check if the class name begins with a '{'
			// If it does, that means it is an ActiveX Control in MSDEV (MFC) format
			if (pszClassName[0] == L'{')
			{
				// Item is an ActiveX control.
				bHasOleControls = TRUE;
			}
			else
			{
				// Item is not an ActiveX Control: make room for it in new template.
				cbNewTemplate += (BYTE*)pNextItem - (BYTE*)pItem;
			}

			pItem = pNextItem;
		}

		// No OLE controls were found, so there's no reason to go any further.
		if (!bHasOleControls)
			return pTemplate;

		// Copy entire header into new template.
		BYTE* pNew = (BYTE*)GlobalAlloc(GMEM_FIXED, cbNewTemplate);
		DLGTEMPLATE* pNewTemplate = (DLGTEMPLATE*)pNew;
		memcpy(pNew, pTemplate, cbHeader);
		pNew += cbHeader;

		// Initialize item count in new header to zero.
		DlgTemplateItemCount(pNewTemplate) = 0;

		pItem = pFirstItem;
		pNextItem = pItem;

		// Second pass through the dialog template.  On this pass, we want to:
		//    1. Copy all the non-OLE controls into the new template.
		for (iItem = 0; iItem < nItems; iItem++)
		{
			pNextItem = FindNextDlgItem(pItem, bDialogEx);

			pszClassName = bDialogEx ?
				(LPWSTR)(((DLGITEMTEMPLATEEX*)pItem) + 1) :
				(LPWSTR)(pItem + 1);

			if (pszClassName[0] != L'{')
			{
				// Item is not an OLE control: copy it to the new template.
				ULONG_PTR cbItem = (BYTE*)pNextItem - (BYTE*)pItem;
				ATLASSERT(cbItem >= (bDialogEx ?
					sizeof(DLGITEMTEMPLATEEX) :
					sizeof(DLGITEMTEMPLATE)));
				memcpy(pNew, pItem, cbItem);
				pNew += cbItem;

				// Incrememt item count in new header.
				++DlgTemplateItemCount(pNewTemplate);
			}

			pItem = pNextItem;
		}
		return pNewTemplate;
	}

	static HRESULT ParseInitData(IStream* pStream, BSTR* pLicKey)
	{
		*pLicKey = NULL;
		if(pStream == NULL)
			return S_OK;	// nothing to do

		ULONG uRead;
		HRESULT hr;

		WORD nMsg;
		hr = pStream->Read(&nMsg, sizeof(WORD), &uRead);
		if (FAILED(hr))
			return hr;

		DWORD dwLen;
		hr = pStream->Read(&dwLen, sizeof(DWORD), &uRead);
		if (FAILED(hr))
			return hr;

		DWORD cchLicKey;
		hr = pStream->Read(&cchLicKey, sizeof(DWORD), &uRead);
		if (FAILED(hr))
			return hr;

		if (cchLicKey > 0)
		{
			CComBSTR bstr(cchLicKey);
			if (bstr.Length() == 0)
				return E_OUTOFMEMORY;
			memset(bstr.m_str, 0, (cchLicKey + 1) * sizeof(OLECHAR));
			hr = pStream->Read(bstr.m_str, cchLicKey * sizeof(OLECHAR), &uRead);
			if (FAILED(hr))
				return hr;
			*pLicKey = bstr.Detach();
		}

		// Extended (DATABINDING) stream format is not supported,
		// we reject databinding info but preserve other information
		if (nMsg == (WORD)ATL_WM_OCC_LOADFROMSTREAM_EX ||
			nMsg == (WORD)ATL_WM_OCC_LOADFROMSTORAGE_EX)
		{
			// Read the size of the section
			ULONG cbOffset;
			hr = pStream->Read(&cbOffset, sizeof(ULONG), &uRead);
			if (FAILED(hr))
				return hr;
			BYTE pTemp[1000];
			cbOffset -= sizeof(ULONG);

			while (cbOffset > 0)
			{
				pStream->Read(pTemp, (cbOffset < 1000 ? cbOffset : 1000), &uRead);
				cbOffset -= uRead;
			}
			return S_OK;
		}
		if (nMsg == (WORD)ATL_WM_OCC_LOADFROMSTREAM)
		{
			return S_OK;
		}
		return E_FAIL;
	}
};

/////////////////////////////////////////////////////////////////////////////
// CWindow - client side for a Windows window

class CWindow
{
public:
	static RECT rcDefault;
	HWND m_hWnd;

	CWindow(HWND hWnd = NULL) throw() :
		m_hWnd(hWnd)
	{
	}

	CWindow& operator=(HWND hWnd) throw()
	{
		m_hWnd = hWnd;
		return *this;
	}

	static LPCTSTR GetWndClassName() throw()
	{
		return NULL;
	}

	void Attach(HWND hWndNew) throw()
	{
		ATLASSERT(m_hWnd == NULL);
		ATLASSERT((hWndNew == NULL) || ::IsWindow(hWndNew));
		m_hWnd = hWndNew;
	}

	HWND Detach() throw()
	{
		HWND hWnd = m_hWnd;
		m_hWnd = NULL;
		return hWnd;
	}

	HWND Create(LPCTSTR lpstrWndClass, HWND hWndParent, _U_RECT rect = NULL, LPCTSTR szWindowName = NULL,
			DWORD dwStyle = 0, DWORD dwExStyle = 0,
			_U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL) throw()
	{
		ATLASSERT(m_hWnd == NULL);
		if(rect.m_lpRect == NULL)
			rect.m_lpRect = &rcDefault;
		m_hWnd = ::CreateWindowEx(dwExStyle, lpstrWndClass, szWindowName,
			dwStyle, rect.m_lpRect->left, rect.m_lpRect->top, rect.m_lpRect->right - rect.m_lpRect->left,
			rect.m_lpRect->bottom - rect.m_lpRect->top, hWndParent, MenuOrID.m_hMenu,
			_AtlBaseModule.GetModuleInstance(), lpCreateParam);
		return m_hWnd;
	}

	BOOL DestroyWindow() throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));

		if(!::DestroyWindow(m_hWnd))
			return FALSE;

		m_hWnd = NULL;
		return TRUE;
	}

// Attributes

	operator HWND() const throw()
	{ 
		return m_hWnd; 
	}

	DWORD GetStyle() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (DWORD)::GetWindowLong(m_hWnd, GWL_STYLE);
	}

	DWORD GetExStyle() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (DWORD)::GetWindowLong(m_hWnd, GWL_EXSTYLE);
	}

	LONG GetWindowLong(int nIndex) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetWindowLong(m_hWnd, nIndex);
	}

	LONG_PTR GetWindowLongPtr(int nIndex) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetWindowLongPtr(m_hWnd, nIndex);
	}

	LONG SetWindowLong(int nIndex, LONG dwNewLong) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetWindowLong(m_hWnd, nIndex, dwNewLong);
	}

	LONG_PTR SetWindowLongPtr(int nIndex, LONG_PTR dwNewLong) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetWindowLongPtr(m_hWnd, nIndex, dwNewLong);
	}

	WORD GetWindowWord(int nIndex) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetWindowWord(m_hWnd, nIndex);
	}

	WORD SetWindowWord(int nIndex, WORD wNewWord) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetWindowWord(m_hWnd, nIndex, wNewWord);
	}

// Message Functions

	LRESULT SendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SendMessage(m_hWnd,message,wParam,lParam);
	}

	BOOL PostMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::PostMessage(m_hWnd,message,wParam,lParam);
	}

	BOOL SendNotifyMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SendNotifyMessage(m_hWnd, message, wParam, lParam);
	}

	// support for C style macros
	static LRESULT SendMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) throw()
	{
		ATLASSERT(::IsWindow(hWnd));
		return ::SendMessage(hWnd, message, wParam, lParam);
	}

// Window Text Functions

	BOOL SetWindowText(LPCTSTR lpszString) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetWindowText(m_hWnd, lpszString);
	}

	int GetWindowText(LPTSTR lpszStringBuf, int nMaxCount) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetWindowText(m_hWnd, lpszStringBuf, nMaxCount);
	}
	int GetWindowText(CSimpleString& strText) const
	{
		int nLength;
		LPTSTR pszText;

		nLength = GetWindowTextLength();
		pszText = strText.GetBuffer(nLength+1);
		nLength = GetWindowText(pszText, nLength+1);
		strText.ReleaseBuffer(nLength);

		return nLength;
	}
	int GetWindowTextLength() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetWindowTextLength(m_hWnd);
	}

// Font Functions

	void SetFont(HFONT hFont, BOOL bRedraw = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(bRedraw, 0));
	}

	HFONT GetFont() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (HFONT)::SendMessage(m_hWnd, WM_GETFONT, 0, 0);
	}

// Menu Functions (non-child windows only)

	HMENU GetMenu() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (HMENU)::GetMenu(m_hWnd);
	}

	BOOL SetMenu(HMENU hMenu) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetMenu(m_hWnd, hMenu);
	}

	BOOL DrawMenuBar() throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::DrawMenuBar(m_hWnd);
	}

	HMENU GetSystemMenu(BOOL bRevert) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (HMENU)::GetSystemMenu(m_hWnd, bRevert);
	}

	BOOL HiliteMenuItem(HMENU hMenu, UINT uItemHilite, UINT uHilite) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::HiliteMenuItem(m_hWnd, hMenu, uItemHilite, uHilite);
	}

// Window Size and Position Functions

	BOOL IsIconic() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::IsIconic(m_hWnd);
	}

	BOOL IsZoomed() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::IsZoomed(m_hWnd);
	}

	BOOL MoveWindow(int x, int y, int nWidth, int nHeight, BOOL bRepaint = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::MoveWindow(m_hWnd, x, y, nWidth, nHeight, bRepaint);
	}

	BOOL MoveWindow(LPCRECT lpRect, BOOL bRepaint = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::MoveWindow(m_hWnd, lpRect->left, lpRect->top, lpRect->right - lpRect->left, lpRect->bottom - lpRect->top, bRepaint);
	}

	BOOL SetWindowPos(HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetWindowPos(m_hWnd, hWndInsertAfter, x, y, cx, cy, nFlags);
	}

	BOOL SetWindowPos(HWND hWndInsertAfter, LPCRECT lpRect, UINT nFlags) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetWindowPos(m_hWnd, hWndInsertAfter, lpRect->left, lpRect->top, lpRect->right - lpRect->left, lpRect->bottom - lpRect->top, nFlags);
	}

	UINT ArrangeIconicWindows() throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::ArrangeIconicWindows(m_hWnd);
	}

	BOOL BringWindowToTop() throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::BringWindowToTop(m_hWnd);
	}

	BOOL GetWindowRect(LPRECT lpRect) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetWindowRect(m_hWnd, lpRect);
	}

	BOOL GetClientRect(LPRECT lpRect) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetClientRect(m_hWnd, lpRect);
	}

	BOOL GetWindowPlacement(WINDOWPLACEMENT FAR* lpwndpl) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetWindowPlacement(m_hWnd, lpwndpl);
	}

	BOOL SetWindowPlacement(const WINDOWPLACEMENT FAR* lpwndpl) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetWindowPlacement(m_hWnd, lpwndpl);
	}

// Coordinate Mapping Functions

	BOOL ClientToScreen(LPPOINT lpPoint) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::ClientToScreen(m_hWnd, lpPoint);
	}

	BOOL ClientToScreen(LPRECT lpRect) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		if(!::ClientToScreen(m_hWnd, (LPPOINT)lpRect))
			return FALSE;
		return ::ClientToScreen(m_hWnd, ((LPPOINT)lpRect)+1);
	}

	BOOL ScreenToClient(LPPOINT lpPoint) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::ScreenToClient(m_hWnd, lpPoint);
	}

	BOOL ScreenToClient(LPRECT lpRect) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		if(!::ScreenToClient(m_hWnd, (LPPOINT)lpRect))
			return FALSE;
		return ::ScreenToClient(m_hWnd, ((LPPOINT)lpRect)+1);
	}

	int MapWindowPoints(HWND hWndTo, LPPOINT lpPoint, UINT nCount) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::MapWindowPoints(m_hWnd, hWndTo, lpPoint, nCount);
	}

	int MapWindowPoints(HWND hWndTo, LPRECT lpRect) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::MapWindowPoints(m_hWnd, hWndTo, (LPPOINT)lpRect, 2);
	}

// Update and Painting Functions

	HDC BeginPaint(LPPAINTSTRUCT lpPaint) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::BeginPaint(m_hWnd, lpPaint);
	}

	void EndPaint(LPPAINTSTRUCT lpPaint) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::EndPaint(m_hWnd, lpPaint);
	}

	HDC GetDC() throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetDC(m_hWnd);
	}

	HDC GetWindowDC() throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetWindowDC(m_hWnd);
	}

	int ReleaseDC(HDC hDC) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::ReleaseDC(m_hWnd, hDC);
	}

	void Print(HDC hDC, DWORD dwFlags) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, WM_PRINT, (WPARAM)hDC, dwFlags);
	}

	void PrintClient(HDC hDC, DWORD dwFlags) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, WM_PRINTCLIENT, (WPARAM)hDC, dwFlags);
	}

	BOOL UpdateWindow() throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::UpdateWindow(m_hWnd);
	}

	void SetRedraw(BOOL bRedraw = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, WM_SETREDRAW, (WPARAM)bRedraw, 0);
	}

	BOOL GetUpdateRect(LPRECT lpRect, BOOL bErase = FALSE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetUpdateRect(m_hWnd, lpRect, bErase);
	}

	int GetUpdateRgn(HRGN hRgn, BOOL bErase = FALSE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetUpdateRgn(m_hWnd, hRgn, bErase);
	}

	BOOL Invalidate(BOOL bErase = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::InvalidateRect(m_hWnd, NULL, bErase);
	}

	BOOL InvalidateRect(LPCRECT lpRect, BOOL bErase = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::InvalidateRect(m_hWnd, lpRect, bErase);
	}

	BOOL ValidateRect(LPCRECT lpRect) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::ValidateRect(m_hWnd, lpRect);
	}

	void InvalidateRgn(HRGN hRgn, BOOL bErase = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::InvalidateRgn(m_hWnd, hRgn, bErase);
	}

	BOOL ValidateRgn(HRGN hRgn) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::ValidateRgn(m_hWnd, hRgn);
	}

	BOOL ShowWindow(int nCmdShow) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::ShowWindow(m_hWnd, nCmdShow);
	}

	BOOL IsWindowVisible() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::IsWindowVisible(m_hWnd);
	}

	BOOL ShowOwnedPopups(BOOL bShow = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::ShowOwnedPopups(m_hWnd, bShow);
	}

	HDC GetDCEx(HRGN hRgnClip, DWORD flags) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetDCEx(m_hWnd, hRgnClip, flags);
	}

	BOOL LockWindowUpdate(BOOL bLock = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::LockWindowUpdate(bLock ? m_hWnd : NULL);
	}

	BOOL RedrawWindow(LPCRECT lpRectUpdate = NULL, HRGN hRgnUpdate = NULL, UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::RedrawWindow(m_hWnd, lpRectUpdate, hRgnUpdate, flags);
	}

// Timer Functions

	UINT_PTR SetTimer(UINT_PTR nIDEvent, UINT nElapse, void (CALLBACK* lpfnTimer)(HWND, UINT, UINT_PTR, DWORD) = NULL) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetTimer(m_hWnd, nIDEvent, nElapse, (TIMERPROC)lpfnTimer);
	}

	BOOL KillTimer(UINT_PTR nIDEvent) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::KillTimer(m_hWnd, nIDEvent);
	}

// Window State Functions

	BOOL IsWindowEnabled() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::IsWindowEnabled(m_hWnd);
	}

	BOOL EnableWindow(BOOL bEnable = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::EnableWindow(m_hWnd, bEnable);
	}

	HWND SetActiveWindow() throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetActiveWindow(m_hWnd);
	}

	HWND SetCapture() throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetCapture(m_hWnd);
	}

	HWND SetFocus() throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetFocus(m_hWnd);
	}

// Dialog-Box Item Functions

	BOOL CheckDlgButton(int nIDButton, UINT nCheck) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::CheckDlgButton(m_hWnd, nIDButton, nCheck);
	}

	BOOL CheckRadioButton(int nIDFirstButton, int nIDLastButton, int nIDCheckButton) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::CheckRadioButton(m_hWnd, nIDFirstButton, nIDLastButton, nIDCheckButton);
	}

	int DlgDirList(LPTSTR lpPathSpec, int nIDListBox, int nIDStaticPath, UINT nFileType) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::DlgDirList(m_hWnd, lpPathSpec, nIDListBox, nIDStaticPath, nFileType);
	}

	int DlgDirListComboBox(LPTSTR lpPathSpec, int nIDComboBox, int nIDStaticPath, UINT nFileType) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::DlgDirListComboBox(m_hWnd, lpPathSpec, nIDComboBox, nIDStaticPath, nFileType);
	}

	BOOL DlgDirSelect(LPTSTR lpString, int nCount, int nIDListBox) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::DlgDirSelectEx(m_hWnd, lpString, nCount, nIDListBox);
	}

	BOOL DlgDirSelectComboBox(LPTSTR lpString, int nCount, int nIDComboBox) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::DlgDirSelectComboBoxEx(m_hWnd, lpString, nCount, nIDComboBox);
	}

	UINT GetDlgItemInt(int nID, BOOL* lpTrans = NULL, BOOL bSigned = TRUE) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetDlgItemInt(m_hWnd, nID, lpTrans, bSigned);
	}

	UINT GetDlgItemText(int nID, LPTSTR lpStr, int nMaxCount) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetDlgItemText(m_hWnd, nID, lpStr, nMaxCount);
	}

	UINT GetDlgItemText(int nID, CSimpleString& strText) const
	{
		HWND hItem;

		ATLASSERT(::IsWindow(m_hWnd));
		hItem = GetDlgItem(nID);
		if (hItem != NULL)
		{
			int nLength;
			LPTSTR pszText;

			nLength = ::GetWindowTextLength(hItem);
			pszText = strText.GetBuffer(nLength+1);
			nLength = ::GetWindowText(hItem, pszText, nLength+1);
			strText.ReleaseBuffer(nLength);

			return nLength;
		}
		else
		{
			strText.Empty();

			return 0;
		}
	}
#ifdef _OLEAUTO_H_
	BOOL GetDlgItemText(int nID, BSTR& bstrText) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));

		HWND hWndCtl = GetDlgItem(nID);
		if(hWndCtl == NULL)
			return FALSE;

		return CWindow(hWndCtl).GetWindowText(bstrText);
	}
#endif // _OLEAUTO_H_
	CWindow GetNextDlgGroupItem(HWND hWndCtl, BOOL bPrevious = FALSE) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return CWindow(::GetNextDlgGroupItem(m_hWnd, hWndCtl, bPrevious));
	}

	CWindow GetNextDlgTabItem(HWND hWndCtl, BOOL bPrevious = FALSE) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return CWindow(::GetNextDlgTabItem(m_hWnd, hWndCtl, bPrevious));
	}

	UINT IsDlgButtonChecked(int nIDButton) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::IsDlgButtonChecked(m_hWnd, nIDButton);
	}

	LRESULT SendDlgItemMessage(int nID, UINT message, WPARAM wParam = 0, LPARAM lParam = 0) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SendDlgItemMessage(m_hWnd, nID, message, wParam, lParam);
	}

	BOOL SetDlgItemInt(int nID, UINT nValue, BOOL bSigned = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetDlgItemInt(m_hWnd, nID, nValue, bSigned);
	}

	BOOL SetDlgItemText(int nID, LPCTSTR lpszString) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetDlgItemText(m_hWnd, nID, lpszString);
	}

#ifndef _ATL_NO_HOSTING
	HRESULT GetDlgControl(int nID, REFIID iid, void** ppCtrl) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(ppCtrl != NULL);
		if (ppCtrl == NULL)
			return E_POINTER;
		*ppCtrl = NULL;
		HRESULT hr = HRESULT_FROM_WIN32(ERROR_CONTROL_ID_NOT_FOUND);
		HWND hWndCtrl = GetDlgItem(nID);
		if (hWndCtrl != NULL)
		{
			*ppCtrl = NULL;
			CComPtr<IUnknown> spUnk;
			hr = AtlAxGetControl(hWndCtrl, &spUnk);
			if (SUCCEEDED(hr))
				hr = spUnk->QueryInterface(iid, ppCtrl);
		}
		return hr;
	}
	HRESULT GetDlgHost(int nID, REFIID iid, void** ppHost) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(ppHost != NULL);
		if (ppHost == NULL)
			return E_POINTER;
		*ppHost = NULL;
		HRESULT hr = HRESULT_FROM_WIN32(ERROR_CONTROL_ID_NOT_FOUND);
		HWND hWndCtrl = GetDlgItem(nID);
		if (hWndCtrl != NULL)
		{
			CComPtr<IUnknown> spUnk;
			hr = AtlAxGetHost(hWndCtrl, &spUnk);
			if (SUCCEEDED(hr))
				hr = spUnk->QueryInterface(iid, ppHost);
		}
		return hr;
	}
#endif //!_ATL_NO_HOSTING

// Scrolling Functions

	int GetScrollPos(int nBar) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetScrollPos(m_hWnd, nBar);
	}

	BOOL GetScrollRange(int nBar, LPINT lpMinPos, LPINT lpMaxPos) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetScrollRange(m_hWnd, nBar, lpMinPos, lpMaxPos);
	}

	BOOL ScrollWindow(int xAmount, int yAmount, LPCRECT lpRect = NULL, LPCRECT lpClipRect = NULL) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::ScrollWindow(m_hWnd, xAmount, yAmount, lpRect, lpClipRect);
	}

	int ScrollWindowEx(int dx, int dy, LPCRECT lpRectScroll, LPCRECT lpRectClip, HRGN hRgnUpdate, LPRECT lpRectUpdate, UINT uFlags) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::ScrollWindowEx(m_hWnd, dx, dy, lpRectScroll, lpRectClip, hRgnUpdate, lpRectUpdate, uFlags);
	}

	int ScrollWindowEx(int dx, int dy, UINT uFlags, LPCRECT lpRectScroll = NULL, LPCRECT lpRectClip = NULL, HRGN hRgnUpdate = NULL, LPRECT lpRectUpdate = NULL) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::ScrollWindowEx(m_hWnd, dx, dy, lpRectScroll, lpRectClip, hRgnUpdate, lpRectUpdate, uFlags);
	}

	int SetScrollPos(int nBar, int nPos, BOOL bRedraw = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetScrollPos(m_hWnd, nBar, nPos, bRedraw);
	}

	BOOL SetScrollRange(int nBar, int nMinPos, int nMaxPos, BOOL bRedraw = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetScrollRange(m_hWnd, nBar, nMinPos, nMaxPos, bRedraw);
	}

	BOOL ShowScrollBar(UINT nBar, BOOL bShow = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::ShowScrollBar(m_hWnd, nBar, bShow);
	}

	BOOL EnableScrollBar(UINT uSBFlags, UINT uArrowFlags = ESB_ENABLE_BOTH) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::EnableScrollBar(m_hWnd, uSBFlags, uArrowFlags);
	}

// Window Access Functions

	CWindow ChildWindowFromPoint(POINT point) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return CWindow(::ChildWindowFromPoint(m_hWnd, point));
	}

	CWindow ChildWindowFromPointEx(POINT point, UINT uFlags) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return CWindow(::ChildWindowFromPointEx(m_hWnd, point, uFlags));
	}

	CWindow GetTopWindow() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return CWindow(::GetTopWindow(m_hWnd));
	}

	CWindow GetWindow(UINT nCmd) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return CWindow(::GetWindow(m_hWnd, nCmd));
	}

	CWindow GetLastActivePopup() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return CWindow(::GetLastActivePopup(m_hWnd));
	}

	BOOL IsChild(HWND hWnd) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::IsChild(m_hWnd, hWnd);
	}

	CWindow GetParent() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return CWindow(::GetParent(m_hWnd));
	}

	CWindow SetParent(HWND hWndNewParent) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return CWindow(::SetParent(m_hWnd, hWndNewParent));
	}

// Window Tree Access

	int GetDlgCtrlID() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetDlgCtrlID(m_hWnd);
	}

	int SetDlgCtrlID(int nID) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (int)::SetWindowLong(m_hWnd, GWL_ID, nID);
	}

	CWindow GetDlgItem(int nID) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return CWindow(::GetDlgItem(m_hWnd, nID));
	}

// Alert Functions

	BOOL FlashWindow(BOOL bInvert) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::FlashWindow(m_hWnd, bInvert);
	}

	int MessageBox(LPCTSTR lpszText, LPCTSTR lpszCaption = _T(""), UINT nType = MB_OK) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::MessageBox(m_hWnd, lpszText, lpszCaption, nType);
	}

// Clipboard Functions

	BOOL ChangeClipboardChain(HWND hWndNewNext) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::ChangeClipboardChain(m_hWnd, hWndNewNext);
	}

	HWND SetClipboardViewer() throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetClipboardViewer(m_hWnd);
	}

	BOOL OpenClipboard() throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::OpenClipboard(m_hWnd);
	}

// Caret Functions

	BOOL CreateCaret(HBITMAP hBitmap) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::CreateCaret(m_hWnd, hBitmap, 0, 0);
	}

	BOOL CreateSolidCaret(int nWidth, int nHeight) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::CreateCaret(m_hWnd, (HBITMAP)0, nWidth, nHeight);
	}

	BOOL CreateGrayCaret(int nWidth, int nHeight) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::CreateCaret(m_hWnd, (HBITMAP)1, nWidth, nHeight);
	}

	BOOL HideCaret() throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::HideCaret(m_hWnd);
	}

	BOOL ShowCaret() throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::ShowCaret(m_hWnd);
	}

#ifdef _INC_SHELLAPI
// Drag-Drop Functions
	void DragAcceptFiles(BOOL bAccept = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd)); ::DragAcceptFiles(m_hWnd, bAccept);
	}
#endif

// Icon Functions

	HICON SetIcon(HICON hIcon, BOOL bBigIcon = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (HICON)::SendMessage(m_hWnd, WM_SETICON, bBigIcon, (LPARAM)hIcon);
	}

	HICON GetIcon(BOOL bBigIcon = TRUE) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (HICON)::SendMessage(m_hWnd, WM_GETICON, bBigIcon, 0);
	}

// Help Functions

	BOOL WinHelp(LPCTSTR lpszHelp, UINT nCmd = HELP_CONTEXT, DWORD dwData = 0) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::WinHelp(m_hWnd, lpszHelp, nCmd, dwData);
	}

	BOOL SetWindowContextHelpId(DWORD dwContextHelpId) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetWindowContextHelpId(m_hWnd, dwContextHelpId);
	}

	DWORD GetWindowContextHelpId() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetWindowContextHelpId(m_hWnd);
	}

// Hot Key Functions

	int SetHotKey(WORD wVirtualKeyCode, WORD wModifiers) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (int)::SendMessage(m_hWnd, WM_SETHOTKEY, MAKEWORD(wVirtualKeyCode, wModifiers), 0);
	}

	DWORD GetHotKey() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (DWORD)::SendMessage(m_hWnd, WM_GETHOTKEY, 0, 0);
	}

// Misc. Operations

//N new
	BOOL GetScrollInfo(int nBar, LPSCROLLINFO lpScrollInfo) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetScrollInfo(m_hWnd, nBar, lpScrollInfo);
	}
	int SetScrollInfo(int nBar, LPSCROLLINFO lpScrollInfo, BOOL bRedraw = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetScrollInfo(m_hWnd, nBar, lpScrollInfo, bRedraw);
	}
	BOOL IsDialogMessage(LPMSG lpMsg) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::IsDialogMessage(m_hWnd, lpMsg);
	}

	void NextDlgCtrl() const throw()
		{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, WM_NEXTDLGCTL, 0, 0L);
	}
	void PrevDlgCtrl() const throw()
		{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, WM_NEXTDLGCTL, 1, 0L);
	}
	void GotoDlgCtrl(HWND hWndCtrl) const throw()
		{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, WM_NEXTDLGCTL, (WPARAM)hWndCtrl, 1L);
	}

	BOOL ResizeClient(int nWidth, int nHeight, BOOL bRedraw = TRUE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));

		RECT rcWnd;
		if(!GetClientRect(&rcWnd))
			return FALSE;

		if(nWidth != -1)
			rcWnd.right = nWidth;
		if(nHeight != -1)
			rcWnd.bottom = nHeight;

		if(!::AdjustWindowRectEx(&rcWnd, GetStyle(), (!(GetStyle() & WS_CHILD) && (GetMenu() != NULL)), GetExStyle()))
			return FALSE;

		UINT uFlags = SWP_NOZORDER | SWP_NOMOVE;
		if(!bRedraw)
			uFlags |= SWP_NOREDRAW;

		return SetWindowPos(NULL, 0, 0, rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top, uFlags);
	}

	int GetWindowRgn(HRGN hRgn) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetWindowRgn(m_hWnd, hRgn);
	}
	int SetWindowRgn(HRGN hRgn, BOOL bRedraw = FALSE) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SetWindowRgn(m_hWnd, hRgn, bRedraw);
	}
	HDWP DeferWindowPos(HDWP hWinPosInfo, HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::DeferWindowPos(hWinPosInfo, m_hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
	}
	DWORD GetWindowThreadID() throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::GetWindowThreadProcessId(m_hWnd, NULL);
	}
	DWORD GetWindowProcessID() throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		DWORD dwProcessID;
		::GetWindowThreadProcessId(m_hWnd, &dwProcessID);
		return dwProcessID;
	}
	BOOL IsWindow() const throw()
	{
		return ::IsWindow(m_hWnd);
	}
	BOOL IsWindowUnicode() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::IsWindowUnicode(m_hWnd);
	}
	BOOL IsParentDialog() throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		TCHAR szBuf[8]; // "#32770" + NUL character
		if (GetClassName(GetParent(), szBuf, sizeof(szBuf)/sizeof(TCHAR)) == 0)
			return FALSE;
		return lstrcmp(szBuf, _T("#32770")) == 0;
	}
	BOOL ShowWindowAsync(int nCmdShow) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::ShowWindowAsync(m_hWnd, nCmdShow);
	}

	CWindow GetDescendantWindow(int nID) const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));

		// GetDlgItem recursive (return first found)
		// breadth-first for 1 level, then depth-first for next level

		// use GetDlgItem since it is a fast USER function
		HWND hWndChild, hWndTmp;
		if((hWndChild = ::GetDlgItem(m_hWnd, nID)) != NULL)
		{
			if(::GetTopWindow(hWndChild) != NULL)
			{
				// children with the same ID as their parent have priority
				CWindow wnd(hWndChild);
				hWndTmp = wnd.GetDescendantWindow(nID);
				if(hWndTmp != NULL)
					return CWindow(hWndTmp);
			}
			return CWindow(hWndChild);
		}

		// walk each child
		for(hWndChild = ::GetTopWindow(m_hWnd); hWndChild != NULL;
			hWndChild = ::GetNextWindow(hWndChild, GW_HWNDNEXT))
		{
			CWindow wnd(hWndChild);
			hWndTmp = wnd.GetDescendantWindow(nID);
			if(hWndTmp != NULL)
				return CWindow(hWndTmp);
		}

		return CWindow(NULL);    // not found
	}

	void SendMessageToDescendants(UINT message, WPARAM wParam = 0, LPARAM lParam = 0, BOOL bDeep = TRUE) throw()
	{
		for(HWND hWndChild = ::GetTopWindow(m_hWnd); hWndChild != NULL;
			hWndChild = ::GetNextWindow(hWndChild, GW_HWNDNEXT))
		{
			::SendMessage(hWndChild, message, wParam, lParam);

			if(bDeep && ::GetTopWindow(hWndChild) != NULL)
			{
				// send to child windows after parent
				CWindow wnd(hWndChild);
				wnd.SendMessageToDescendants(message, wParam, lParam, bDeep);
			}
		}
	}

	BOOL CenterWindow(HWND hWndCenter = NULL) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));

		// determine owner window to center against
		DWORD dwStyle = GetStyle();
		if(hWndCenter == NULL)
		{
			if(dwStyle & WS_CHILD)
				hWndCenter = ::GetParent(m_hWnd);
			else
				hWndCenter = ::GetWindow(m_hWnd, GW_OWNER);
		}

		// get coordinates of the window relative to its parent
		RECT rcDlg;
		::GetWindowRect(m_hWnd, &rcDlg);
		RECT rcArea;
		RECT rcCenter;
		HWND hWndParent;
		if(!(dwStyle & WS_CHILD))
		{
			// don't center against invisible or minimized windows
			if(hWndCenter != NULL)
			{
				DWORD dwStyleCenter = ::GetWindowLong(hWndCenter, GWL_STYLE);
				if(!(dwStyleCenter & WS_VISIBLE) || (dwStyleCenter & WS_MINIMIZE))
					hWndCenter = NULL;
			}

			// center within screen coordinates
			::SystemParametersInfo(SPI_GETWORKAREA, NULL, &rcArea, NULL);
			if(hWndCenter == NULL)
				rcCenter = rcArea;
			else
				::GetWindowRect(hWndCenter, &rcCenter);
		}
		else
		{
			// center within parent client coordinates
			hWndParent = ::GetParent(m_hWnd);
			ATLASSERT(::IsWindow(hWndParent));

			::GetClientRect(hWndParent, &rcArea);
			ATLASSERT(::IsWindow(hWndCenter));
			::GetClientRect(hWndCenter, &rcCenter);
			::MapWindowPoints(hWndCenter, hWndParent, (POINT*)&rcCenter, 2);
		}

		int DlgWidth = rcDlg.right - rcDlg.left;
		int DlgHeight = rcDlg.bottom - rcDlg.top;

		// find dialog's upper left based on rcCenter
		int xLeft = (rcCenter.left + rcCenter.right) / 2 - DlgWidth / 2;
		int yTop = (rcCenter.top + rcCenter.bottom) / 2 - DlgHeight / 2;

		// if the dialog is outside the screen, move it inside
		if(xLeft < rcArea.left)
			xLeft = rcArea.left;
		else if(xLeft + DlgWidth > rcArea.right)
			xLeft = rcArea.right - DlgWidth;

		if(yTop < rcArea.top)
			yTop = rcArea.top;
		else if(yTop + DlgHeight > rcArea.bottom)
			yTop = rcArea.bottom - DlgHeight;

		// map screen coordinates to child coordinates
		return ::SetWindowPos(m_hWnd, NULL, xLeft, yTop, -1, -1,
			SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}

	BOOL ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));

		DWORD dwStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);
		DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;
		if(dwStyle == dwNewStyle)
			return FALSE;

		::SetWindowLong(m_hWnd, GWL_STYLE, dwNewStyle);
		if(nFlags != 0)
		{
			::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0,
				SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | nFlags);
		}

		return TRUE;
	}

	BOOL ModifyStyleEx(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0) throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));

		DWORD dwStyle = ::GetWindowLong(m_hWnd, GWL_EXSTYLE);
		DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;
		if(dwStyle == dwNewStyle)
			return FALSE;

		::SetWindowLong(m_hWnd, GWL_EXSTYLE, dwNewStyle);
		if(nFlags != 0)
		{
			::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0,
				SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | nFlags);
		}

		return TRUE;
	}

#ifdef _OLEAUTO_H_
	BOOL GetWindowText(BSTR* pbstrText) throw()
	{
		return GetWindowText(*pbstrText);
	}
	BOOL GetWindowText(BSTR& bstrText) throw()
	{
		USES_CONVERSION;
		ATLASSERT(::IsWindow(m_hWnd));
		if (bstrText != NULL)
		{
			SysFreeString(bstrText);
			bstrText = NULL;
		}

		int nLen = ::GetWindowTextLength(m_hWnd);
		if(nLen == 0)
		{
			bstrText = ::SysAllocString(OLESTR(""));
			return (bstrText != NULL) ? TRUE : FALSE;
		}

		CTempBuffer<TCHAR> lpszText;
		ATLTRY(lpszText.Allocate(nLen+1));
		if (lpszText == NULL)
			return FALSE;

		if(!::GetWindowText(m_hWnd, lpszText, nLen+1))
			return FALSE;

		bstrText = ::SysAllocString(T2OLE(lpszText));
		return (bstrText != NULL) ? TRUE : FALSE;
	}
#endif // _OLEAUTO_H_
	CWindow GetTopLevelParent() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));

		HWND hWndParent = m_hWnd;
		HWND hWndTmp;
		while((hWndTmp = ::GetParent(hWndParent)) != NULL)
			hWndParent = hWndTmp;

		return CWindow(hWndParent);
	}

	CWindow GetTopLevelWindow() const throw()
	{
		ATLASSERT(::IsWindow(m_hWnd));

		HWND hWndParent;
		HWND hWndTmp = m_hWnd;

		do
		{
			hWndParent = hWndTmp;
			hWndTmp = (::GetWindowLong(hWndParent, GWL_STYLE) & WS_CHILD) ? ::GetParent(hWndParent) : ::GetWindow(hWndParent, GW_OWNER);
		}
		while(hWndTmp != NULL);

		return CWindow(hWndParent);
	}
};

_declspec(selectany) RECT CWindow::rcDefault = { CW_USEDEFAULT, CW_USEDEFAULT, 0, 0 };

/////////////////////////////////////////////////////////////////////////////
// CAxWindow - client side for an ActiveX host window

#ifndef _ATL_NO_HOSTING

template <class TBase /* = CWindow */>
class CAxWindowT : public TBase
{
public:
// Constructors
	CAxWindowT(HWND hWnd = NULL) : TBase(hWnd)
	{
		AtlAxWinInit();
	}

	CAxWindowT< TBase >& operator=(HWND hWnd)
	{
		m_hWnd = hWnd;
		return *this;
	}

// Attributes
	static LPCTSTR GetWndClassName()
	{
		return _T(ATLAXWIN_CLASS);
	}

// Operations
	HWND Create(HWND hWndParent, _U_RECT rect = NULL, LPCTSTR szWindowName = NULL,
			DWORD dwStyle = 0, DWORD dwExStyle = 0,
			_U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL)
	{
		return CWindow::Create(GetWndClassName(), hWndParent, rect, szWindowName, dwStyle, dwExStyle, MenuOrID, lpCreateParam);
	}

	HRESULT CreateControl(LPCOLESTR lpszName, IStream* pStream = NULL, IUnknown** ppUnkContainer = NULL)
	{
		return CreateControlEx(lpszName, pStream, ppUnkContainer);
	}

	HRESULT CreateControl(DWORD dwResID, IStream* pStream = NULL, IUnknown** ppUnkContainer = NULL)
	{
		return CreateControlEx(dwResID, pStream, ppUnkContainer);
	}

	HRESULT CreateControlEx(LPCOLESTR lpszName, IStream* pStream = NULL, 
			IUnknown** ppUnkContainer = NULL, IUnknown** ppUnkControl = NULL,
			REFIID iidSink = IID_NULL, IUnknown* punkSink = NULL)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		// We must have a valid window!

		// Get a pointer to the container object connected to this window
		CComPtr<IAxWinHostWindow> spWinHost;
		HRESULT hr = QueryHost(&spWinHost);

		// If QueryHost failed, there is no host attached to this window
		// We assume that the user wants to create a new host and subclass the current window
		if (FAILED(hr))
			return AtlAxCreateControlEx(lpszName, m_hWnd, pStream, ppUnkContainer, ppUnkControl, iidSink, punkSink);

		// Create the control requested by the caller
		CComPtr<IUnknown> pControl;
		if (SUCCEEDED(hr))
			hr = spWinHost->CreateControlEx(lpszName, m_hWnd, pStream, &pControl, iidSink, punkSink);

		// Send back the necessary interface pointers
		if (SUCCEEDED(hr))
		{
			if (ppUnkControl)
				*ppUnkControl = pControl.Detach();

			if (ppUnkContainer)
			{
				hr = spWinHost.QueryInterface(ppUnkContainer);
				ATLASSERT(SUCCEEDED(hr)); // This should not fail!
			}
		}

		return hr;
	}

	HRESULT CreateControlEx(DWORD dwResID,  IStream* pStream = NULL, 
			IUnknown** ppUnkContainer = NULL, IUnknown** ppUnkControl = NULL,
			REFIID iidSink = IID_NULL, IUnknown* punkSink = NULL)
	{
		TCHAR szModule[MAX_PATH];
		DWORD dwFLen = GetModuleFileName(_AtlBaseModule.GetModuleInstance(), szModule, MAX_PATH);
		if( dwFLen == 0 )
			return AtlHresultFromLastError();
		else if( dwFLen == MAX_PATH )
			return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

		CComBSTR bstrURL(OLESTR("res://"));
		bstrURL.Append(szModule);
		bstrURL.Append(OLESTR("/"));
		TCHAR szResID[11];
		wsprintf(szResID, _T("%0d"), dwResID);
		bstrURL.Append(szResID);

		ATLASSERT(::IsWindow(m_hWnd));
		return CreateControlEx(bstrURL, pStream, ppUnkContainer, ppUnkControl, iidSink, punkSink);
	}

	HRESULT AttachControl(IUnknown* pControl, IUnknown** ppUnkContainer)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		// We must have a valid window!

		// Get a pointer to the container object connected to this window
		CComPtr<IAxWinHostWindow> spWinHost;
		HRESULT hr = QueryHost(&spWinHost);

		// If QueryHost failed, there is no host attached to this window
		// We assume that the user wants to create a new host and subclass the current window
		if (FAILED(hr))
			return AtlAxAttachControl(pControl, m_hWnd, ppUnkContainer);

		// Attach the control specified by the caller
		if (SUCCEEDED(hr))
			hr = spWinHost->AttachControl(pControl, m_hWnd);

		// Get the IUnknown interface of the container
		if (SUCCEEDED(hr) && ppUnkContainer)
		{
			hr = spWinHost.QueryInterface(ppUnkContainer);
			ATLASSERT(SUCCEEDED(hr)); // This should not fail!
		}

		return hr;
	}

	HRESULT QueryHost(REFIID iid, void** ppUnk)
	{
		ATLASSERT(ppUnk != NULL);
		if (ppUnk == NULL)
			return E_POINTER;
		HRESULT hr;
		*ppUnk = NULL;
		CComPtr<IUnknown> spUnk;
		hr = AtlAxGetHost(m_hWnd, &spUnk);
		if (SUCCEEDED(hr))
			hr = spUnk->QueryInterface(iid, ppUnk);
		return hr;
	}
	template <class Q>
	HRESULT QueryHost(Q** ppUnk)
	{
		return QueryHost(__uuidof(Q), (void**)ppUnk);
	}
	HRESULT QueryControl(REFIID iid, void** ppUnk)
	{
		ATLASSERT(ppUnk != NULL);
		if (ppUnk == NULL)
			return E_POINTER;
		HRESULT hr;
		*ppUnk = NULL;
		CComPtr<IUnknown> spUnk;
		hr = AtlAxGetControl(m_hWnd, &spUnk);
		if (SUCCEEDED(hr))
			hr = spUnk->QueryInterface(iid, ppUnk);
		return hr;
	}
	template <class Q>
	HRESULT QueryControl(Q** ppUnk)
	{
		return QueryControl(__uuidof(Q), (void**)ppUnk);
	}
	HRESULT SetExternalDispatch(IDispatch* pDisp)
	{
		HRESULT hr;
		CComPtr<IAxWinHostWindow> spHost;
		hr = QueryHost(__uuidof(IAxWinHostWindow), (void**)&spHost);
		if (SUCCEEDED(hr))
			hr = spHost->SetExternalDispatch(pDisp);
		return hr;
	}
	HRESULT SetExternalUIHandler(IDocHostUIHandlerDispatch* pUIHandler)
	{
		HRESULT hr;
		CComPtr<IAxWinHostWindow> spHost;
		hr = QueryHost(__uuidof(IAxWinHostWindow), (void**)&spHost);
		if (SUCCEEDED(hr))
			hr = spHost->SetExternalUIHandler(pUIHandler);
		return hr;
	}
};

typedef CAxWindowT<CWindow> CAxWindow;

template <class TBase /* = CWindow */>
class CAxWindow2T : public CAxWindowT<TBase>
{
public:
// Constructors
	CAxWindow2T(HWND hWnd = NULL) : CAxWindowT<TBase>(hWnd)
	{
	}

	CAxWindow2T< TBase >& operator=(HWND hWnd)
	{
		m_hWnd = hWnd;
		return *this;
	}

// Attributes
	static LPCTSTR GetWndClassName()
	{
		return _T(ATLAXWINLIC_CLASS);
	}

// Operations
	HWND Create(HWND hWndParent, _U_RECT rect = NULL, LPCTSTR szWindowName = NULL,
			DWORD dwStyle = 0, DWORD dwExStyle = 0,
			_U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL)
	{
		return CWindow::Create(GetWndClassName(), hWndParent, rect, szWindowName, dwStyle, dwExStyle, MenuOrID, lpCreateParam);
	}

	HRESULT CreateControlLic(LPCOLESTR lpszName, IStream* pStream = NULL, IUnknown** ppUnkContainer = NULL, BSTR bstrLicKey = NULL)
	{
		return CreateControlLicEx(lpszName, pStream, ppUnkContainer, NULL, IID_NULL, NULL, bstrLicKey);
	}

	HRESULT CreateControlLic(DWORD dwResID, IStream* pStream = NULL, IUnknown** ppUnkContainer = NULL, BSTR bstrLicKey = NULL)
	{
		return CreateControlLicEx(dwResID, pStream, ppUnkContainer, NULL, IID_NULL, NULL, bstrLicKey);
	}

	HRESULT CreateControlLicEx(LPCOLESTR lpszName, IStream* pStream = NULL, 
			IUnknown** ppUnkContainer = NULL, IUnknown** ppUnkControl = NULL,
			REFIID iidSink = IID_NULL, IUnknown* punkSink = NULL, BSTR bstrLicKey = NULL)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		// We must have a valid window!

		// Get a pointer to the container object connected to this window
		CComPtr<IAxWinHostWindowLic> spWinHost;
		HRESULT hr = QueryHost(&spWinHost);

		// If QueryHost failed, there is no host attached to this window
		// We assume that the user wants to create a new host and subclass the current window
		if (FAILED(hr))
			return AtlAxCreateControlLicEx(lpszName, m_hWnd, pStream, ppUnkContainer, ppUnkControl, iidSink, punkSink, bstrLicKey);

		// Create the control requested by the caller
		CComPtr<IUnknown> pControl;
		if (SUCCEEDED(hr))
			hr = spWinHost->CreateControlLicEx(lpszName, m_hWnd, pStream, &pControl, iidSink, punkSink, bstrLicKey);

		// Send back the necessary interface pointers
		if (SUCCEEDED(hr))
		{
			if (ppUnkControl)
				*ppUnkControl = pControl.Detach();

			if (ppUnkContainer)
			{
				hr = spWinHost.QueryInterface(ppUnkContainer);
				ATLASSERT(SUCCEEDED(hr)); // This should not fail!
			}
		}

		return hr;
	}

	HRESULT CreateControlLicEx(DWORD dwResID,  IStream* pStream = NULL, 
			IUnknown** ppUnkContainer = NULL, IUnknown** ppUnkControl = NULL,
			REFIID iidSink = IID_NULL, IUnknown* punkSink = NULL, BSTR bstrLickey = NULL)
	{
		TCHAR szModule[MAX_PATH];
		DWORD dwFLen = GetModuleFileName(_AtlBaseModule.GetModuleInstance(), szModule, MAX_PATH);
		if( dwFLen == 0 )
			return AtlHresultFromLastError();
		else if( dwFLen == MAX_PATH )
			return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

		CComBSTR bstrURL(OLESTR("res://"));
		bstrURL.Append(szModule);
		bstrURL.Append(OLESTR("/"));
		TCHAR szResID[11];
		wsprintf(szResID, _T("%0d"), dwResID);
		bstrURL.Append(szResID);

		ATLASSERT(::IsWindow(m_hWnd));
		return CreateControlLicEx(bstrURL, pStream, ppUnkContainer, ppUnkControl, iidSink, punkSink, bstrLickey);
	}
};

typedef CAxWindow2T<CWindow> CAxWindow2;


#endif //_ATL_NO_HOSTING

/////////////////////////////////////////////////////////////////////////////
// WindowProc thunks

class CWndProcThunk
{
public:
	_AtlCreateWndData cd;
	CStdCallThunk thunk;

	void Init(WNDPROC proc, void* pThis)
	{
		thunk.Init((DWORD_PTR)proc, pThis);
	}
	WNDPROC GetWNDPROC()
	{
		return (WNDPROC)thunk.GetCodeAddress();
	}
};

/////////////////////////////////////////////////////////////////////////////
// CMessageMap - abstract class that provides an interface for message maps

class ATL_NO_VTABLE CMessageMap
{ 
public:
	virtual BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		LRESULT& lResult, DWORD dwMsgMapID) = 0;
};

/////////////////////////////////////////////////////////////////////////////
// Message map

#define BEGIN_MSG_MAP(theClass) \
public: \
	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID = 0) \
	{ \
		BOOL bHandled = TRUE; \
		(hWnd); \
		(uMsg); \
		(wParam); \
		(lParam); \
		(lResult); \
		(bHandled); \
		switch(dwMsgMapID) \
		{ \
		case 0:

#define ALT_MSG_MAP(msgMapID) \
		break; \
		case msgMapID:

#define MESSAGE_HANDLER(msg, func) \
	if(uMsg == msg) \
	{ \
		bHandled = TRUE; \
		lResult = func(uMsg, wParam, lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define MESSAGE_RANGE_HANDLER(msgFirst, msgLast, func) \
	if(uMsg >= msgFirst && uMsg <= msgLast) \
	{ \
		bHandled = TRUE; \
		lResult = func(uMsg, wParam, lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define COMMAND_HANDLER(id, code, func) \
	if(uMsg == WM_COMMAND && id == LOWORD(wParam) && code == HIWORD(wParam)) \
	{ \
		bHandled = TRUE; \
		lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define COMMAND_ID_HANDLER(id, func) \
	if(uMsg == WM_COMMAND && id == LOWORD(wParam)) \
	{ \
		bHandled = TRUE; \
		lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define COMMAND_CODE_HANDLER(code, func) \
	if(uMsg == WM_COMMAND && code == HIWORD(wParam)) \
	{ \
		bHandled = TRUE; \
		lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define COMMAND_RANGE_HANDLER(idFirst, idLast, func) \
	if(uMsg == WM_COMMAND && LOWORD(wParam) >= idFirst  && LOWORD(wParam) <= idLast) \
	{ \
		bHandled = TRUE; \
		lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define COMMAND_RANGE_CODE_HANDLER(idFirst, idLast, code, func) \
	if(uMsg == WM_COMMAND && code == HIWORD(wParam) && LOWORD(wParam) >= idFirst  && LOWORD(wParam) <= idLast) \
	{ \
		bHandled = TRUE; \
		lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define NOTIFY_HANDLER(id, cd, func) \
	if(uMsg == WM_NOTIFY && id == ((LPNMHDR)lParam)->idFrom && cd == ((LPNMHDR)lParam)->code) \
	{ \
		bHandled = TRUE; \
		lResult = func((int)wParam, (LPNMHDR)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define NOTIFY_ID_HANDLER(id, func) \
	if(uMsg == WM_NOTIFY && id == ((LPNMHDR)lParam)->idFrom) \
	{ \
		bHandled = TRUE; \
		lResult = func((int)wParam, (LPNMHDR)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define NOTIFY_CODE_HANDLER(cd, func) \
	if(uMsg == WM_NOTIFY && cd == ((LPNMHDR)lParam)->code) \
	{ \
		bHandled = TRUE; \
		lResult = func((int)wParam, (LPNMHDR)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define NOTIFY_RANGE_HANDLER(idFirst, idLast, func) \
	if(uMsg == WM_NOTIFY && ((LPNMHDR)lParam)->idFrom >= idFirst && ((LPNMHDR)lParam)->idFrom <= idLast) \
	{ \
		bHandled = TRUE; \
		lResult = func((int)wParam, (LPNMHDR)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define NOTIFY_RANGE_CODE_HANDLER(idFirst, idLast, cd, func) \
	if(uMsg == WM_NOTIFY && cd == ((LPNMHDR)lParam)->code && ((LPNMHDR)lParam)->idFrom >= idFirst && ((LPNMHDR)lParam)->idFrom <= idLast) \
	{ \
		bHandled = TRUE; \
		lResult = func((int)wParam, (LPNMHDR)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define CHAIN_MSG_MAP(theChainClass) \
	{ \
		if(theChainClass::ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult)) \
			return TRUE; \
	}

#define CHAIN_MSG_MAP_MEMBER(theChainMember) \
	{ \
		if(theChainMember.ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult)) \
			return TRUE; \
	}

#define CHAIN_MSG_MAP_ALT(theChainClass, msgMapID) \
	{ \
		if(theChainClass::ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, msgMapID)) \
			return TRUE; \
	}

#define CHAIN_MSG_MAP_ALT_MEMBER(theChainMember, msgMapID) \
	{ \
		if(theChainMember.ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, msgMapID)) \
			return TRUE; \
	}

#define CHAIN_MSG_MAP_DYNAMIC(dynaChainID) \
	{ \
		if(CDynamicChain::CallChain(dynaChainID, hWnd, uMsg, wParam, lParam, lResult)) \
			return TRUE; \
	}

#define END_MSG_MAP() \
			break; \
		default: \
			ATLTRACE(ATL::atlTraceWindowing, 0, _T("Invalid message map ID (%i)\n"), dwMsgMapID); \
			ATLASSERT(FALSE); \
			break; \
		} \
		return FALSE; \
	}


// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);


// Empty message map macro

#define DECLARE_EMPTY_MSG_MAP() \
public: \
	BOOL ProcessWindowMessage(HWND, UINT, WPARAM, LPARAM, LRESULT&, DWORD = 0) \
	{ \
		return FALSE; \
	}

// Message forwarding and reflection macros

#define FORWARD_NOTIFICATIONS() \
	{ \
		bHandled = TRUE; \
		lResult = ForwardNotifications(uMsg, wParam, lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define REFLECT_NOTIFICATIONS() \
	{ \
		bHandled = TRUE; \
		lResult = ReflectNotifications(uMsg, wParam, lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define DEFAULT_REFLECTION_HANDLER() \
	if(DefaultReflectionHandler(hWnd, uMsg, wParam, lParam, lResult)) \
		return TRUE;

#define REFLECTED_COMMAND_HANDLER(id, code, func) \
	if(uMsg == OCM_COMMAND && id == LOWORD(wParam) && code == HIWORD(wParam)) \
	{ \
		bHandled = TRUE; \
		lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define REFLECTED_COMMAND_ID_HANDLER(id, func) \
	if(uMsg == OCM_COMMAND && id == LOWORD(wParam)) \
	{ \
		bHandled = TRUE; \
		lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define REFLECTED_COMMAND_CODE_HANDLER(code, func) \
	if(uMsg == OCM_COMMAND && code == HIWORD(wParam)) \
	{ \
		bHandled = TRUE; \
		lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define REFLECTED_COMMAND_RANGE_HANDLER(idFirst, idLast, func) \
	if(uMsg == OCM_COMMAND && LOWORD(wParam) >= idFirst  && LOWORD(wParam) <= idLast) \
	{ \
		bHandled = TRUE; \
		lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define REFLECTED_COMMAND_RANGE_CODE_HANDLER(idFirst, idLast, code, func) \
	if(uMsg == OCM_COMMAND && code == HIWORD(wParam) && LOWORD(wParam) >= idFirst  && LOWORD(wParam) <= idLast) \
	{ \
		bHandled = TRUE; \
		lResult = func(HIWORD(wParam), LOWORD(wParam), (HWND)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define REFLECTED_NOTIFY_HANDLER(id, cd, func) \
	if(uMsg == OCM_NOTIFY && id == ((LPNMHDR)lParam)->idFrom && cd == ((LPNMHDR)lParam)->code) \
	{ \
		bHandled = TRUE; \
		lResult = func((int)wParam, (LPNMHDR)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define REFLECTED_NOTIFY_ID_HANDLER(id, func) \
	if(uMsg == OCM_NOTIFY && id == ((LPNMHDR)lParam)->idFrom) \
	{ \
		bHandled = TRUE; \
		lResult = func((int)wParam, (LPNMHDR)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define REFLECTED_NOTIFY_CODE_HANDLER(cd, func) \
	if(uMsg == OCM_NOTIFY && cd == ((LPNMHDR)lParam)->code) \
	{ \
		bHandled = TRUE; \
		lResult = func((int)wParam, (LPNMHDR)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define REFLECTED_NOTIFY_RANGE_HANDLER(idFirst, idLast, func) \
	if(uMsg == OCM_NOTIFY && ((LPNMHDR)lParam)->idFrom >= idFirst && ((LPNMHDR)lParam)->idFrom <= idLast) \
	{ \
		bHandled = TRUE; \
		lResult = func((int)wParam, (LPNMHDR)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define REFLECTED_NOTIFY_RANGE_CODE_HANDLER(idFirst, idLast, cd, func) \
	if(uMsg == OCM_NOTIFY && cd == ((LPNMHDR)lParam)->code && ((LPNMHDR)lParam)->idFrom >= idFirst && ((LPNMHDR)lParam)->idFrom <= idLast) \
	{ \
		bHandled = TRUE; \
		lResult = func((int)wParam, (LPNMHDR)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

/////////////////////////////////////////////////////////////////////////////
// CDynamicChain - provides support for dynamic chaining

class CDynamicChain
{
public:
	struct ATL_CHAIN_ENTRY
	{
		DWORD m_dwChainID;
		CMessageMap* m_pObject;
		DWORD m_dwMsgMapID;
	};

	CSimpleArray<ATL_CHAIN_ENTRY*> m_aChainEntry;

	CDynamicChain()
	{ }

	~CDynamicChain()
	{
		for(int i = 0; i < m_aChainEntry.GetSize(); i++)
		{
			if(m_aChainEntry[i] != NULL)
				delete m_aChainEntry[i];
		}
	}

	BOOL SetChainEntry(DWORD dwChainID, CMessageMap* pObject, DWORD dwMsgMapID = 0)
	{
	// first search for an existing entry

		int i;
		for(i = 0; i < m_aChainEntry.GetSize(); i++)
		{
			if(m_aChainEntry[i] != NULL && m_aChainEntry[i]->m_dwChainID == dwChainID)
			{
				m_aChainEntry[i]->m_pObject = pObject;
				m_aChainEntry[i]->m_dwMsgMapID = dwMsgMapID;
				return TRUE;
			}
		}

	// create a new one

		ATL_CHAIN_ENTRY* pEntry = NULL;
		ATLTRY(pEntry = new ATL_CHAIN_ENTRY);

		if(pEntry == NULL)
			return FALSE;

		pEntry->m_dwChainID = dwChainID;
		pEntry->m_pObject = pObject;
		pEntry->m_dwMsgMapID = dwMsgMapID;

	// search for an empty one

		for(i = 0; i < m_aChainEntry.GetSize(); i++)
		{
			if(m_aChainEntry[i] == NULL)
			{
				m_aChainEntry[i] = pEntry;
				return TRUE;
			}
		}

	// add a new one

		BOOL bRet = m_aChainEntry.Add(pEntry);

		if(!bRet)
		{
			delete pEntry;
			return FALSE;
		}

		return TRUE;
	}

	BOOL RemoveChainEntry(DWORD dwChainID)
	{
		for(int i = 0; i < m_aChainEntry.GetSize(); i++)
		{
			if(m_aChainEntry[i] != NULL && m_aChainEntry[i]->m_dwChainID == dwChainID)
			{
				delete m_aChainEntry[i];
				m_aChainEntry[i] = NULL;
				return TRUE;
			}
		}

		return FALSE;
	}

	BOOL CallChain(DWORD dwChainID, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
	{
		for(int i = 0; i < m_aChainEntry.GetSize(); i++)
		{
			if(m_aChainEntry[i] != NULL && m_aChainEntry[i]->m_dwChainID == dwChainID)
				return (m_aChainEntry[i]->m_pObject)->ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, m_aChainEntry[i]->m_dwMsgMapID);
		}

		return FALSE;
	}
};

/////////////////////////////////////////////////////////////////////////////
// CWndClassInfo - Manages Windows class information

#define DECLARE_WND_CLASS(WndClassName) \
static ATL::CWndClassInfo& GetWndClassInfo() \
{ \
	static ATL::CWndClassInfo wc = \
	{ \
		{ sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, StartWindowProc, \
		  0, 0, NULL, NULL, NULL, (HBRUSH)(COLOR_WINDOW + 1), NULL, WndClassName, NULL }, \
		NULL, NULL, IDC_ARROW, TRUE, 0, _T("") \
	}; \
	return wc; \
}

#define DECLARE_WND_CLASS_EX(WndClassName, style, bkgnd) \
static ATL::CWndClassInfo& GetWndClassInfo() \
{ \
	static ATL::CWndClassInfo wc = \
	{ \
		{ sizeof(WNDCLASSEX), style, StartWindowProc, \
		  0, 0, NULL, NULL, NULL, (HBRUSH)(bkgnd + 1), NULL, WndClassName, NULL }, \
		NULL, NULL, IDC_ARROW, TRUE, 0, _T("") \
	}; \
	return wc; \
}

#define DECLARE_WND_SUPERCLASS(WndClassName, OrigWndClassName) \
static ATL::CWndClassInfo& GetWndClassInfo() \
{ \
	static ATL::CWndClassInfo wc = \
	{ \
		{ sizeof(WNDCLASSEX), 0, StartWindowProc, \
		  0, 0, NULL, NULL, NULL, NULL, NULL, WndClassName, NULL }, \
		OrigWndClassName, NULL, NULL, TRUE, 0, _T("") \
	}; \
	return wc; \
}

/////////////////////////////////////////////////////////////////////////////
// CWinTraits - Defines various default values for a window

template <DWORD t_dwStyle = 0, DWORD t_dwExStyle = 0>
class CWinTraits
{
public:
	static DWORD GetWndStyle(DWORD dwStyle)
	{
		return dwStyle == 0 ? t_dwStyle : dwStyle;
	}
	static DWORD GetWndExStyle(DWORD dwExStyle)
	{
		return dwExStyle == 0 ? t_dwExStyle : dwExStyle;
	}
};

typedef CWinTraits<WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0>					CControlWinTraits;
typedef CWinTraits<WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE>		CFrameWinTraits;
typedef CWinTraits<WS_OVERLAPPEDWINDOW | WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_MDICHILD>	CMDIChildWinTraits;

typedef CWinTraits<0, 0> CNullTraits;

template <DWORD t_dwStyle = 0, DWORD t_dwExStyle = 0, class TWinTraits = CControlWinTraits>
class CWinTraitsOR
{
public:
	static DWORD GetWndStyle(DWORD dwStyle)
	{
		return dwStyle | t_dwStyle | TWinTraits::GetWndStyle(dwStyle);
	}
	static DWORD GetWndExStyle(DWORD dwExStyle)
	{
		return dwExStyle | t_dwExStyle | TWinTraits::GetWndExStyle(dwExStyle);
	}
};

/////////////////////////////////////////////////////////////////////////////
// CWindowImpl - Implements a window

template <class TBase /* = CWindow */>
class ATL_NO_VTABLE CWindowImplRoot : public TBase, public CMessageMap
{
public:
	CWndProcThunk m_thunk;
	const _ATL_MSG* m_pCurrentMsg;
	DWORD m_dwState;

	enum { WINSTATE_DESTROYED = 0x00000001 };

// Constructor/destructor
	CWindowImplRoot() : m_pCurrentMsg(NULL), m_dwState(0)
	{ }

	virtual ~CWindowImplRoot()
	{
#ifdef _DEBUG
		if(m_hWnd != NULL)	// should be cleared in WindowProc
		{
			ATLTRACE(atlTraceWindowing, 0, _T("ERROR - Object deleted before window was destroyed\n"));
			ATLASSERT(FALSE);
		}
#endif //_DEBUG
	}

// Current message
	const _ATL_MSG* GetCurrentMessage() const
	{
		return m_pCurrentMsg;
	}

	// "handled" management for cracked handlers
	BOOL IsMsgHandled() const
	{
		const _ATL_MSG* pMsg = GetCurrentMessage();
		ATLASSERT(pMsg != NULL);
		ATLASSERT(pMsg->cbSize >= sizeof(_ATL_MSG));
		return pMsg->bHandled;
	}
	void SetMsgHandled(BOOL bHandled)
	{
		_ATL_MSG* pMsg = (_ATL_MSG*)GetCurrentMessage();	// override const
		ATLASSERT(pMsg != NULL);
		ATLASSERT(pMsg->cbSize >= sizeof(_ATL_MSG));
		pMsg->bHandled = bHandled;
	}

// Message forwarding and reflection support
	LRESULT ForwardNotifications(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT ReflectNotifications(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	static BOOL DefaultReflectionHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
};

template <class TBase>
LRESULT CWindowImplRoot< TBase >::ForwardNotifications(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LRESULT lResult = 0;
	switch(uMsg)
	{
	case WM_COMMAND:
	case WM_NOTIFY:
	case WM_PARENTNOTIFY:
	case WM_DRAWITEM:
	case WM_MEASUREITEM:
	case WM_COMPAREITEM:
	case WM_DELETEITEM:
	case WM_VKEYTOITEM:
	case WM_CHARTOITEM:
	case WM_HSCROLL:
	case WM_VSCROLL:
	case WM_CTLCOLORBTN:
	case WM_CTLCOLORDLG:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORMSGBOX:
	case WM_CTLCOLORSCROLLBAR:
	case WM_CTLCOLORSTATIC:
		lResult = GetParent().SendMessage(uMsg, wParam, lParam);
		break;
	default:
		bHandled = FALSE;
		break;
	}
	return lResult;
}

template <class TBase>
LRESULT CWindowImplRoot< TBase >::ReflectNotifications(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HWND hWndChild = NULL;

	switch(uMsg)
	{
	case WM_COMMAND:
		if(lParam != NULL)	// not from a menu
			hWndChild = (HWND)lParam;
		break;
	case WM_NOTIFY:
		hWndChild = ((LPNMHDR)lParam)->hwndFrom;
		break;
	case WM_PARENTNOTIFY:
		switch(LOWORD(wParam))
		{
		case WM_CREATE:
		case WM_DESTROY:
			hWndChild = (HWND)lParam;
			break;
		default:
			hWndChild = GetDlgItem(HIWORD(wParam));
			break;
		}
		break;
	case WM_DRAWITEM:
		if(wParam)	// not from a menu
			hWndChild = ((LPDRAWITEMSTRUCT)lParam)->hwndItem;
		break;
	case WM_MEASUREITEM:
		if(wParam)	// not from a menu
			hWndChild = GetDlgItem(((LPMEASUREITEMSTRUCT)lParam)->CtlID);
		break;
	case WM_COMPAREITEM:
		if(wParam)	// not from a menu
			hWndChild = GetDlgItem(((LPCOMPAREITEMSTRUCT)lParam)->CtlID);
		break;
	case WM_DELETEITEM:
		if(wParam)	// not from a menu
			hWndChild = GetDlgItem(((LPDELETEITEMSTRUCT)lParam)->CtlID);
		break;
	case WM_VKEYTOITEM:
	case WM_CHARTOITEM:
	case WM_HSCROLL:
	case WM_VSCROLL:
		hWndChild = (HWND)lParam;
		break;
	case WM_CTLCOLORBTN:
	case WM_CTLCOLORDLG:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORMSGBOX:
	case WM_CTLCOLORSCROLLBAR:
	case WM_CTLCOLORSTATIC:
		hWndChild = (HWND)lParam;
		break;
	default:
		break;
	}

	if(hWndChild == NULL)
	{
		bHandled = FALSE;
		return 1;
	}

	ATLASSERT(::IsWindow(hWndChild));
	return ::SendMessage(hWndChild, OCM__BASE + uMsg, wParam, lParam);
}

template <class TBase>
BOOL CWindowImplRoot< TBase >::DefaultReflectionHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	switch(uMsg)
	{
	case OCM_COMMAND:
	case OCM_NOTIFY:
	case OCM_PARENTNOTIFY:
	case OCM_DRAWITEM:
	case OCM_MEASUREITEM:
	case OCM_COMPAREITEM:
	case OCM_DELETEITEM:
	case OCM_VKEYTOITEM:
	case OCM_CHARTOITEM:
	case OCM_HSCROLL:
	case OCM_VSCROLL:
	case OCM_CTLCOLORBTN:
	case OCM_CTLCOLORDLG:
	case OCM_CTLCOLOREDIT:
	case OCM_CTLCOLORLISTBOX:
	case OCM_CTLCOLORMSGBOX:
	case OCM_CTLCOLORSCROLLBAR:
	case OCM_CTLCOLORSTATIC:
		lResult = ::DefWindowProc(hWnd, uMsg - OCM__BASE, wParam, lParam);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

template <class TBase = CWindow, class TWinTraits = CControlWinTraits>
class ATL_NO_VTABLE CWindowImplBaseT : public CWindowImplRoot< TBase >
{
public:
	WNDPROC m_pfnSuperWindowProc;

	CWindowImplBaseT() : m_pfnSuperWindowProc(::DefWindowProc)
	{}

	static DWORD GetWndStyle(DWORD dwStyle)
	{
		return TWinTraits::GetWndStyle(dwStyle);
	}
	static DWORD GetWndExStyle(DWORD dwExStyle)
	{
		return TWinTraits::GetWndExStyle(dwExStyle);
	}

	virtual WNDPROC GetWindowProc()
	{
		return WindowProc;
	}
	static LRESULT CALLBACK StartWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND Create(HWND hWndParent, _U_RECT rect, LPCTSTR szWindowName,
			DWORD dwStyle, DWORD dwExStyle, _U_MENUorID MenuOrID, ATOM atom, LPVOID lpCreateParam = NULL);
	BOOL DestroyWindow()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::DestroyWindow(m_hWnd);
	}
	BOOL SubclassWindow(HWND hWnd);
	HWND UnsubclassWindow(BOOL bForce = FALSE);

	LRESULT DefWindowProc()
	{
		const _ATL_MSG* pMsg = m_pCurrentMsg;
		LRESULT lRes = 0;
		if (pMsg != NULL)
			lRes = DefWindowProc(pMsg->message, pMsg->wParam, pMsg->lParam);
		return lRes;
	}

	LRESULT DefWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
#ifdef STRICT
		return ::CallWindowProc(m_pfnSuperWindowProc, m_hWnd, uMsg, wParam, lParam);
#else
		return ::CallWindowProc((FARPROC)m_pfnSuperWindowProc, m_hWnd, uMsg, wParam, lParam);
#endif
	}

	virtual void OnFinalMessage(HWND /*hWnd*/)
	{
		// override to do something, if needed
	}
};

typedef CWindowImplBaseT<CWindow>	CWindowImplBase;

template <class TBase, class TWinTraits>
LRESULT CALLBACK CWindowImplBaseT< TBase, TWinTraits >::StartWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CWindowImplBaseT< TBase, TWinTraits >* pThis = (CWindowImplBaseT< TBase, TWinTraits >*)_AtlWinModule.ExtractCreateWndData();
	ATLASSERT(pThis != NULL);
	pThis->m_hWnd = hWnd;
	pThis->m_thunk.Init(pThis->GetWindowProc(), pThis);
	WNDPROC pProc = pThis->m_thunk.GetWNDPROC();
	WNDPROC pOldProc = (WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)pProc);
#ifdef _DEBUG
	// check if somebody has subclassed us already since we discard it
	if(pOldProc != StartWindowProc)
		ATLTRACE(atlTraceWindowing, 0, _T("Subclassing through a hook discarded.\n"));
#else
	(pOldProc);	// avoid unused warning
#endif
	return pProc(hWnd, uMsg, wParam, lParam);
}

template <class TBase, class TWinTraits>
LRESULT CALLBACK CWindowImplBaseT< TBase, TWinTraits >::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CWindowImplBaseT< TBase, TWinTraits >* pThis = (CWindowImplBaseT< TBase, TWinTraits >*)hWnd;
	// set a ptr to this message and save the old value
	_ATL_MSG msg(pThis->m_hWnd, uMsg, wParam, lParam);
	const _ATL_MSG* pOldMsg = pThis->m_pCurrentMsg;
	pThis->m_pCurrentMsg = &msg;
	// pass to the message map to process
	LRESULT lRes;
	BOOL bRet = pThis->ProcessWindowMessage(pThis->m_hWnd, uMsg, wParam, lParam, lRes, 0);
	// restore saved value for the current message
	ATLASSERT(pThis->m_pCurrentMsg == &msg);
	pThis->m_pCurrentMsg = pOldMsg;
	// do the default processing if message was not handled
	if(!bRet)
	{
		if(uMsg != WM_NCDESTROY)
			lRes = pThis->DefWindowProc(uMsg, wParam, lParam);
		else
		{
			// unsubclass, if needed
			LONG_PTR pfnWndProc = ::GetWindowLongPtr(pThis->m_hWnd, GWLP_WNDPROC);
			lRes = pThis->DefWindowProc(uMsg, wParam, lParam);
			if(pThis->m_pfnSuperWindowProc != ::DefWindowProc && ::GetWindowLongPtr(pThis->m_hWnd, GWLP_WNDPROC) == pfnWndProc)
				::SetWindowLongPtr(pThis->m_hWnd, GWLP_WNDPROC, (LONG_PTR)pThis->m_pfnSuperWindowProc);
			// mark window as destryed
			pThis->m_dwState |= WINSTATE_DESTROYED;
		}
	}
	if((pThis->m_dwState & WINSTATE_DESTROYED) && pThis->m_pCurrentMsg == NULL)
	{
		// clear out window handle
		HWND hWnd = pThis->m_hWnd;
		pThis->m_hWnd = NULL;
		pThis->m_dwState &= ~WINSTATE_DESTROYED;
		// clean up after window is destroyed
		pThis->OnFinalMessage(hWnd);
	}
	return lRes;
}

template <class TBase, class TWinTraits>
HWND CWindowImplBaseT< TBase, TWinTraits >::Create(HWND hWndParent, _U_RECT rect, LPCTSTR szWindowName,
		DWORD dwStyle, DWORD dwExStyle, _U_MENUorID MenuOrID, ATOM atom, LPVOID lpCreateParam)
{
	ATLASSERT(m_hWnd == NULL);

	if(atom == 0)
		return NULL;

	_AtlWinModule.AddCreateWndData(&m_thunk.cd, this);

	if(MenuOrID.m_hMenu == NULL && (dwStyle & WS_CHILD))
		MenuOrID.m_hMenu = (HMENU)(UINT_PTR)this;
	if(rect.m_lpRect == NULL)
		rect.m_lpRect = &TBase::rcDefault;

	HWND hWnd = ::CreateWindowEx(dwExStyle, MAKEINTATOM(atom), szWindowName,
		dwStyle, rect.m_lpRect->left, rect.m_lpRect->top, rect.m_lpRect->right - rect.m_lpRect->left,
		rect.m_lpRect->bottom - rect.m_lpRect->top, hWndParent, MenuOrID.m_hMenu,
		_AtlBaseModule.GetModuleInstance(), lpCreateParam);

	ATLASSERT(m_hWnd == hWnd);

	return hWnd;
}

template <class TBase, class TWinTraits>
BOOL CWindowImplBaseT< TBase, TWinTraits >::SubclassWindow(HWND hWnd)
{
	ATLASSERT(m_hWnd == NULL);
	ATLASSERT(::IsWindow(hWnd));
	m_thunk.Init(GetWindowProc(), this);
	WNDPROC pProc = m_thunk.GetWNDPROC();
	WNDPROC pfnWndProc = (WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)pProc);
	if(pfnWndProc == NULL)
		return FALSE;
	m_pfnSuperWindowProc = pfnWndProc;
	m_hWnd = hWnd;
	return TRUE;
}

// Use only if you want to subclass before window is destroyed,
// WindowProc will automatically subclass when  window goes away
template <class TBase, class TWinTraits>
HWND CWindowImplBaseT< TBase, TWinTraits >::UnsubclassWindow(BOOL bForce /*= FALSE*/)
{
	ATLASSERT(m_hWnd != NULL);

	WNDPROC pOurProc = m_thunk.GetWNDPROC();
	WNDPROC pActiveProc = (WNDPROC)::GetWindowLongPtr(m_hWnd, GWLP_WNDPROC);

	HWND hWnd = NULL;
	if (bForce || pOurProc == pActiveProc)
	{
		if(!::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_pfnSuperWindowProc))
			return NULL;

		m_pfnSuperWindowProc = ::DefWindowProc;
		hWnd = m_hWnd;
		m_hWnd = NULL;
	}
	return hWnd;
}

template <class T, class TBase /* = CWindow */, class TWinTraits /* = CControlWinTraits */>
class ATL_NO_VTABLE CWindowImpl : public CWindowImplBaseT< TBase, TWinTraits >
{
public:
	DECLARE_WND_CLASS(NULL)

	static LPCTSTR GetWndCaption()
	{
		return NULL;
	}

	HWND Create(HWND hWndParent, _U_RECT rect = NULL, LPCTSTR szWindowName = NULL,
			DWORD dwStyle = 0, DWORD dwExStyle = 0,
			_U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL)
	{
		if (T::GetWndClassInfo().m_lpszOrigName == NULL)
			T::GetWndClassInfo().m_lpszOrigName = GetWndClassName();
		ATOM atom = T::GetWndClassInfo().Register(&m_pfnSuperWindowProc);

		dwStyle = T::GetWndStyle(dwStyle);
		dwExStyle = T::GetWndExStyle(dwExStyle);

		// set caption
		if (szWindowName == NULL)
			szWindowName = T::GetWndCaption();

		return CWindowImplBaseT< TBase, TWinTraits >::Create(hWndParent, rect, szWindowName,
			dwStyle, dwExStyle, MenuOrID, atom, lpCreateParam);
	}
};

template <class T, class TBase /* = CWindow */, class TWinTraits /* = CControlWinTraits */>
class ATL_NO_VTABLE CWindowWithReflectorImpl : public CWindowImpl< T, TBase, TWinTraits >
{
public:
	HWND Create(HWND hWndParent, _U_RECT rect = NULL, LPCTSTR szWindowName = NULL,
			DWORD dwStyle = 0, DWORD dwExStyle = 0,
			_U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL)
	{
		m_wndReflector.Create(hWndParent, rect, NULL, WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, Reflector::REFLECTOR_MAP_ID);
		RECT rcPos = { 0, 0, rect.m_lpRect->right, rect.m_lpRect->bottom };
		return CWindowImpl< T, TBase, TWinTraits >::Create(m_wndReflector, rcPos, szWindowName, dwStyle, dwExStyle, MenuOrID, lpCreateParam);
	}

// message map and handlers
	typedef CWindowWithReflectorImpl< T, TBase, TWinTraits >	thisClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_NCDESTROY, OnNcDestroy)
		MESSAGE_HANDLER(WM_WINDOWPOSCHANGING, OnWindowPosChanging)
	END_MSG_MAP()

	LRESULT OnNcDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		m_wndReflector.DestroyWindow();
		bHandled = FALSE;
		return 1;
	}
	LRESULT OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		WINDOWPOS* pWP = (WINDOWPOS*)lParam;
		m_wndReflector.SetWindowPos(m_wndReflector.GetParent(), pWP->x, pWP->y, pWP->cx, pWP->cy, pWP->flags);
		pWP->flags |= SWP_NOMOVE;
		pWP->x = 0;
		pWP->y = 0;
		return DefWindowProc(uMsg, wParam, lParam);
	}

	// reflector window stuff
	class Reflector : public CWindowImpl<Reflector>
	{
	public:
		enum { REFLECTOR_MAP_ID = 69 };
		DECLARE_WND_CLASS_EX(_T("ATLReflectorWindow"), 0, -1)
		BEGIN_MSG_MAP(Reflector)
			REFLECT_NOTIFICATIONS()
		END_MSG_MAP()
	} m_wndReflector;
};

/////////////////////////////////////////////////////////////////////////////
// CDialogImpl - Implements a dialog box

#define _ATL_RT_DLGINIT  MAKEINTRESOURCE(240)

template <class TBase /* = CWindow */>
class ATL_NO_VTABLE CDialogImplBaseT : public CWindowImplRoot< TBase >
{
public:
	virtual ~CDialogImplBaseT()
	{
	}
	virtual DLGPROC GetDialogProc()
	{
		return DialogProc;
	}
	static INT_PTR CALLBACK StartDialogProc(HWND hWnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL MapDialogRect(LPRECT lpRect)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::MapDialogRect(m_hWnd, lpRect);
	}
	virtual void OnFinalMessage(HWND /*hWnd*/)
	{
		// override to do something, if needed
	}
	// has no meaning for a dialog, but needed for handlers that use it
	LRESULT DefWindowProc()
	{
		return 0;
	}
	// initialize combobox and comboboxex from RT_DLGINIT resource if any
	BOOL ExecuteDlgInit(int iDlgID)
	{
		BOOL bSuccess = TRUE;
		HINSTANCE hInst = _AtlBaseModule.GetResourceInstance();
		HRSRC hrsrc = ::FindResource(hInst, MAKEINTRESOURCE(iDlgID), _ATL_RT_DLGINIT);
		if (hrsrc)
		{
			HGLOBAL hResData = ::LoadResource(hInst, hrsrc);
			if (hResData)
			{
				UNALIGNED WORD* pDlgInit = (UNALIGNED WORD*)::LockResource(hResData);
				if (pDlgInit)
				{
					USES_CONVERSION;
					while (bSuccess && NULL != *pDlgInit)
					{
						WORD wID = *pDlgInit++;
						WORD wMsg = *pDlgInit++;
						DWORD dwSize = *((UNALIGNED DWORD*&)pDlgInit)++;

						// CB_ADDSTRING is stored as 0x403
						if (0x403 == wMsg)
						{
							if (-1 == SendDlgItemMessage(wID, CB_ADDSTRING, 0, (LPARAM)(A2T((LPSTR)pDlgInit))))
								bSuccess = FALSE;
						}
						// CBEM_INSERTITEM is stored as 0x1234
						else if (0x1234 == wMsg)
						{
							COMBOBOXEXITEM item;
							item.mask = CBEIF_TEXT;
							item.iItem = -1;
							item.pszText = A2T(LPSTR(pDlgInit));
							if (-1 == SendDlgItemMessage(wID, CBEM_INSERTITEM, 0, (LPARAM)&item))
								bSuccess = FALSE;
						}
						pDlgInit = (LPWORD)((LPBYTE)pDlgInit + dwSize);
					}
				}
			}
		}
		return bSuccess;
	}
};

template <class TBase>
INT_PTR CALLBACK CDialogImplBaseT< TBase >::StartDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CDialogImplBaseT< TBase >* pThis = (CDialogImplBaseT< TBase >*)_AtlWinModule.ExtractCreateWndData();
	ATLASSERT(pThis != NULL);
	pThis->m_hWnd = hWnd;
	pThis->m_thunk.Init((WNDPROC)pThis->GetDialogProc(), pThis);
	DLGPROC pProc = (DLGPROC)pThis->m_thunk.GetWNDPROC();
	DLGPROC pOldProc = (DLGPROC)::SetWindowLongPtr(hWnd, DWLP_DLGPROC, (LONG_PTR)pProc);
#ifdef _DEBUG
	// check if somebody has subclassed us already since we discard it
	if(pOldProc != StartDialogProc)
		ATLTRACE(atlTraceWindowing, 0, _T("Subclassing through a hook discarded.\n"));
#else
	pOldProc;	// avoid unused warning
#endif
	return pProc(hWnd, uMsg, wParam, lParam);
}

template <class TBase>
INT_PTR CALLBACK CDialogImplBaseT< TBase >::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CDialogImplBaseT< TBase >* pThis = (CDialogImplBaseT< TBase >*)hWnd;
	// set a ptr to this message and save the old value
	_ATL_MSG msg(pThis->m_hWnd, uMsg, wParam, lParam);
	const _ATL_MSG* pOldMsg = pThis->m_pCurrentMsg;
	pThis->m_pCurrentMsg = &msg;
	// pass to the message map to process
	LRESULT lRes;
	BOOL bRet = pThis->ProcessWindowMessage(pThis->m_hWnd, uMsg, wParam, lParam, lRes, 0);
	// restore saved value for the current message
	ATLASSERT(pThis->m_pCurrentMsg == &msg);
	pThis->m_pCurrentMsg = pOldMsg;
	// set result if message was handled
	if(bRet)
	{
		switch (uMsg)
		{
		case WM_COMPAREITEM:
		case WM_VKEYTOITEM:
		case WM_CHARTOITEM:
		case WM_INITDIALOG:
		case WM_QUERYDRAGICON:
		case WM_CTLCOLORMSGBOX:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSCROLLBAR:
		case WM_CTLCOLORSTATIC:
			// return directly
			bRet = (BOOL)lRes;
			break;
		default:
			// return in DWL_MSGRESULT
			::SetWindowLongPtr(pThis->m_hWnd, DWLP_MSGRESULT, lRes);
			break;
		}
	}
	else if(uMsg == WM_NCDESTROY)
	{
		// mark dialog as destryed
		pThis->m_dwState |= WINSTATE_DESTROYED;
	}

	if((pThis->m_dwState & WINSTATE_DESTROYED) && pThis->m_pCurrentMsg == NULL)
	{
		// clear out window handle
		HWND hWnd = pThis->m_hWnd;
		pThis->m_hWnd = NULL;
		pThis->m_dwState &= ~WINSTATE_DESTROYED;
		// clean up after dialog is destroyed
		pThis->OnFinalMessage(hWnd);
	}
	return bRet;
}

typedef CDialogImplBaseT<CWindow>	CDialogImplBase;

template <class T, class TBase /* = CWindow */>
class ATL_NO_VTABLE CDialogImpl : public CDialogImplBaseT< TBase >
{
public:
#ifdef _DEBUG
	bool m_bModal;
	CDialogImpl() : m_bModal(false) { }
#endif //_DEBUG
	// modal dialogs
	INT_PTR DoModal(HWND hWndParent = ::GetActiveWindow(), LPARAM dwInitParam = NULL)
	{
		ATLASSERT(m_hWnd == NULL);
		_AtlWinModule.AddCreateWndData(&m_thunk.cd, (CDialogImplBaseT< TBase >*)this);
#ifdef _DEBUG
		m_bModal = true;
#endif //_DEBUG
		return ::DialogBoxParam(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(static_cast<T*>(this)->IDD),
					hWndParent, T::StartDialogProc, dwInitParam);
	}
	BOOL EndDialog(int nRetCode)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(m_bModal);	// must be a modal dialog
		return ::EndDialog(m_hWnd, nRetCode);
	}
	// modeless dialogs
	HWND Create(HWND hWndParent, LPARAM dwInitParam = NULL)
	{
		ATLASSERT(m_hWnd == NULL);
		_AtlWinModule.AddCreateWndData(&m_thunk.cd, (CDialogImplBaseT< TBase >*)this);
#ifdef _DEBUG
		m_bModal = false;
#endif //_DEBUG
		HWND hWnd = ::CreateDialogParam(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(static_cast<T*>(this)->IDD),
					hWndParent, T::StartDialogProc, dwInitParam);
		ATLASSERT(m_hWnd == hWnd);
		return hWnd;
	}
	// for CComControl
	HWND Create(HWND hWndParent, RECT&, LPARAM dwInitParam = NULL)
	{
		return Create(hWndParent, dwInitParam);
	}
	BOOL DestroyWindow()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(!m_bModal);	// must not be a modal dialog
		return ::DestroyWindow(m_hWnd);
	}
};

/////////////////////////////////////////////////////////////////////////////
// CAxDialogImpl - Implements a dialog box that hosts ActiveX controls

#ifndef _ATL_NO_HOSTING


template <class T, class TBase /* = CWindow */>
class ATL_NO_VTABLE CAxDialogImpl : public CDialogImplBaseT< TBase >
{
public:
#ifdef _DEBUG
	bool m_bModal;
	CAxDialogImpl() : m_bModal(false) { }
#endif //_DEBUG
	int GetIDD()
	{
		return( static_cast<T*>(this)->IDD );
	}
	virtual DLGPROC GetDialogProc()
	{
		return DialogProc;
	}
	static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	// modal dialogs
	INT_PTR DoModal(HWND hWndParent = ::GetActiveWindow(), LPARAM dwInitParam = NULL)
	{
		ATLASSERT(m_hWnd == NULL);
		_AtlWinModule.AddCreateWndData(&m_thunk.cd, (CDialogImplBaseT< TBase >*)this);
#ifdef _DEBUG
		m_bModal = true;
#endif //_DEBUG
		return AtlAxDialogBox(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(static_cast<T*>(this)->IDD),
					hWndParent, T::StartDialogProc, dwInitParam);
	}
	BOOL EndDialog(int nRetCode)
	{
		ATLASSERT(::IsWindow(m_hWnd));
#ifdef _DEBUG
		ATLASSERT(m_bModal);	// must be a modal dialog
#endif //_DEBUG
		return ::EndDialog(m_hWnd, nRetCode);
	}
	// modeless dialogs
	HWND Create(HWND hWndParent, LPARAM dwInitParam = NULL)
	{
		ATLASSERT(m_hWnd == NULL);
		_AtlWinModule.AddCreateWndData(&m_thunk.cd, (CDialogImplBaseT< TBase >*)this);
#ifdef _DEBUG
		m_bModal = false;
#endif //_DEBUG
		HWND hWnd = AtlAxCreateDialog(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(static_cast<T*>(this)->IDD),
					hWndParent, T::StartDialogProc, dwInitParam);
		ATLASSERT(m_hWnd == hWnd);
		return hWnd;
	}
	// for CComControl
	HWND Create(HWND hWndParent, RECT&, LPARAM dwInitParam = NULL)
	{
		return Create(hWndParent, dwInitParam);
	}
	BOOL DestroyWindow()
	{
		ATLASSERT(::IsWindow(m_hWnd));
#ifdef _DEBUG
		ATLASSERT(!m_bModal);	// must not be a modal dialog
#endif //_DEBUG
		return ::DestroyWindow(m_hWnd);
	}

// Event handling support and Message map
	HRESULT AdviseSinkMap(bool bAdvise)
	{
		if(!bAdvise && m_hWnd == NULL)
		{
			// window is gone, controls are already unadvised
			ATLTRACE(atlTraceControls, 1, _T("CAxDialogImpl::AdviseSinkMap called after the window was destroyed\n"));
			return S_OK;
		}
		HRESULT hRet = E_NOTIMPL;
		__if_exists(T::_GetSinkMapFinder)
		{
			T* pT = static_cast<T*>(this);
			hRet = AtlAdviseSinkMap(pT, bAdvise);
		}
		return hRet;
	}

	typedef CAxDialogImpl< T, TBase >	thisClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	END_MSG_MAP()

	virtual HRESULT CreateActiveXControls(UINT nID)
	{
		// Load dialog template and InitData
		HRSRC hDlgInit = ::FindResource(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(nID), (LPTSTR)_ATL_RT_DLGINIT);
		BYTE* pInitData = NULL;
		HGLOBAL hData = NULL;
		HRESULT hr = S_OK;
		if (hDlgInit != NULL)
		{
			hData = ::LoadResource(_AtlBaseModule.GetResourceInstance(), hDlgInit);
			if (hData != NULL)
				pInitData = (BYTE*) ::LockResource(hData);
		}

		HRSRC hDlg = ::FindResource(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(nID), (LPTSTR)RT_DIALOG);
		if (hDlg != NULL)
		{
			HGLOBAL hResource = ::LoadResource(_AtlBaseModule.GetResourceInstance(), hDlg);
			DLGTEMPLATE* pDlg = NULL;
			if (hResource != NULL)
			{
				pDlg = (DLGTEMPLATE*) ::LockResource(hResource);
				if (pDlg != NULL)
				{
					// Get first control on the template
					BOOL bDialogEx = _DialogSplitHelper::IsDialogEx(pDlg);
					WORD nItems = _DialogSplitHelper::DlgTemplateItemCount(pDlg);

					// Get first control on the dialog
					DLGITEMTEMPLATE* pItem = _DialogSplitHelper::FindFirstDlgItem(pDlg);
					HWND hWndPrev = GetWindow(GW_CHILD);

					// Create all ActiveX cotnrols in the dialog template and place them in the correct tab order (z-order)
					for (WORD nItem = 0; nItem < nItems; nItem++)
					{
						DWORD wID = bDialogEx ? ((_DialogSplitHelper::DLGITEMTEMPLATEEX*)pItem)->id : pItem->id;
						if (_DialogSplitHelper::IsActiveXControl(pItem, bDialogEx))
						{
							BYTE* pData = NULL;
							DWORD dwLen = _DialogSplitHelper::FindCreateData(wID, pInitData, &pData);
							CComPtr<IStream> spStream;
							if (dwLen != 0)
							{
								HGLOBAL h = GlobalAlloc(GHND, dwLen);
								if (h != NULL)
								{
									BYTE* pBytes = (BYTE*) GlobalLock(h);
									BYTE* pSource = pData; 
									memcpy(pBytes, pSource, dwLen);
									GlobalUnlock(h);
									CreateStreamOnHGlobal(h, TRUE, &spStream);
								}
								else
								{
									hr = E_OUTOFMEMORY;
									break;
								}
							}

							CComBSTR bstrLicKey;
							hr = _DialogSplitHelper::ParseInitData(spStream, &bstrLicKey.m_str);
							if (SUCCEEDED(hr))
							{
								CAxWindow2 wnd;
								// Get control caption.
								LPWSTR pszClassName = 
									bDialogEx ? 
										(LPWSTR)(((_DialogSplitHelper::DLGITEMTEMPLATEEX*)pItem) + 1) :
										(LPWSTR)(pItem + 1);
								// Get control rect.
								RECT rect;
								rect.left = 
									bDialogEx ? 
										((_DialogSplitHelper::DLGITEMTEMPLATEEX*)pItem)->x : 
										pItem->x;
								rect.top = 
									bDialogEx ? 
										((_DialogSplitHelper::DLGITEMTEMPLATEEX*)pItem)->y : 
										pItem->y;
								rect.right = rect.left + 
									(bDialogEx ? 
										((_DialogSplitHelper::DLGITEMTEMPLATEEX*)pItem)->cx : 
										pItem->cx);
								rect.bottom = rect.top + 
									(bDialogEx ? 
										((_DialogSplitHelper::DLGITEMTEMPLATEEX*)pItem)->cy : 
										pItem->cy);

								// Convert from dialog units to screen units
								MapDialogRect(&rect);

								// Create AxWindow with a NULL caption.
								wnd.Create(m_hWnd, 
									&rect, 
									NULL, 
									(bDialogEx ? 
										((_DialogSplitHelper::DLGITEMTEMPLATEEX*)pItem)->style : 
										pItem->style) | WS_TABSTOP, 
									bDialogEx ? 
										((_DialogSplitHelper::DLGITEMTEMPLATEEX*)pItem)->exStyle : 
										0,
									bDialogEx ? 
										((_DialogSplitHelper::DLGITEMTEMPLATEEX*)pItem)->id : 
										pItem->id,
									NULL);

								if (wnd != NULL)
								{
									// Set the Help ID
									if (bDialogEx && ((_DialogSplitHelper::DLGITEMTEMPLATEEX*)pItem)->helpID != 0)
										wnd.SetWindowContextHelpId(((_DialogSplitHelper::DLGITEMTEMPLATEEX*)pItem)->helpID);
									// Try to create the ActiveX control.
									hr = wnd.CreateControlLic(pszClassName, spStream, NULL, bstrLicKey);
									if (FAILED(hr))
										break;
									// Set the correct tab position.
									if (nItem == 0)
										hWndPrev = HWND_TOP;
									wnd.SetWindowPos(hWndPrev, 0,0,0,0,SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
									hWndPrev = wnd;
								}
								else
								{
									hr = AtlHresultFromLastError();
								}
							}
						}
						else
						{
							if (nItem != 0)
								hWndPrev = ::GetWindow(hWndPrev, GW_HWNDNEXT);
						}
						pItem = _DialogSplitHelper::FindNextDlgItem(pItem, bDialogEx);
					}
				}
				else
					hr = AtlHresultFromLastError();
			}
			else
				hr = AtlHresultFromLastError();
		}
		return hr;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		// initialize controls in dialog with DLGINIT resource section
		ExecuteDlgInit(static_cast<T*>(this)->IDD);
		AdviseSinkMap(true);
		bHandled = FALSE;
		return 1;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		AdviseSinkMap(false);
		bHandled = FALSE;
		return 1;
	}

// Accelerators handling - needs to be called from a message loop
	BOOL IsDialogMessage(LPMSG pMsg)
	{
		if((pMsg->message < WM_KEYFIRST || pMsg->message > WM_KEYLAST) &&
		   (pMsg->message < WM_MOUSEFIRST || pMsg->message > WM_MOUSELAST))
			return FALSE;

		// find a direct child of the dialog from the window that has focus
		HWND hWndCtl = ::GetFocus();
		if(IsChild(hWndCtl) && ::GetParent(hWndCtl) != m_hWnd)
		{
			do
			{
				hWndCtl = ::GetParent(hWndCtl);
			}
			while (::GetParent(hWndCtl) != m_hWnd);
		}
		// give controls a chance to translate this message
		if (::SendMessage(hWndCtl, WM_FORWARDMSG, 0, (LPARAM)pMsg) == 1)
			return TRUE;

		// do the Windows default thing
		return CDialogImplBaseT< TBase >::IsDialogMessage(pMsg);
	}
};

template <class T, class TBase>
INT_PTR CALLBACK CAxDialogImpl< T, TBase >::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CAxDialogImpl< T, TBase >* pThis = (CAxDialogImpl< T, TBase >*)hWnd;
	if (uMsg == WM_INITDIALOG)
	{
		HRESULT hr;
		if (FAILED(hr = pThis->CreateActiveXControls(pThis->GetIDD())))
		{
			pThis->DestroyWindow();
			SetLastError(hr & 0x0000FFFF);
			return FALSE;
		}
	}
	return CDialogImplBaseT< TBase >::DialogProc(hWnd, uMsg, wParam, lParam);
}
#endif //_ATL_NO_HOSTING

/////////////////////////////////////////////////////////////////////////////
// CSimpleDialog - Prebuilt modal dialog that uses standard buttons

template <WORD t_wDlgTemplateID, BOOL t_bCenter /* = TRUE */>
class CSimpleDialog : public CDialogImplBase
{
public:
	INT_PTR DoModal(HWND hWndParent = ::GetActiveWindow())
	{
		ATLASSERT(m_hWnd == NULL);
		_AtlWinModule.AddCreateWndData(&m_thunk.cd, (CDialogImplBase*)this);
		int nRet = ::DialogBox(_AtlBaseModule.GetResourceInstance(),
			MAKEINTRESOURCE(t_wDlgTemplateID), hWndParent, StartDialogProc);
		m_hWnd = NULL;
		return nRet;
	}

	typedef CSimpleDialog<t_wDlgTemplateID, t_bCenter>	thisClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_RANGE_HANDLER(IDOK, IDNO, OnCloseCmd)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// initialize controls in dialog with DLGINIT resource section
		ExecuteDlgInit(t_wDlgTemplateID);
		if(t_bCenter)
			CenterWindow(GetParent());
		return TRUE;
	}

	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		::EndDialog(m_hWnd, wID);
		return 0;
	}
};

/////////////////////////////////////////////////////////////////////////////
// CContainedWindow - Implements a contained window

template <class TBase /* = CWindow */, class TWinTraits /* = CControlWinTraits */>
class CContainedWindowT : public TBase
{
public:
	CWndProcThunk m_thunk;
	LPCTSTR m_lpszClassName;
	WNDPROC m_pfnSuperWindowProc;
	CMessageMap* m_pObject;
	DWORD m_dwMsgMapID;
	const _ATL_MSG* m_pCurrentMsg;

	// If you use this constructor you must supply
	// the Window Class Name, Object* and Message Map ID
	// Later to the Create call
	CContainedWindowT() : m_pCurrentMsg(NULL)
	{ }

	CContainedWindowT(LPTSTR lpszClassName, CMessageMap* pObject, DWORD dwMsgMapID = 0)
		: m_lpszClassName(lpszClassName),
		m_pfnSuperWindowProc(::DefWindowProc),
		m_pObject(pObject), m_dwMsgMapID(dwMsgMapID),
		m_pCurrentMsg(NULL)
	{ }

	CContainedWindowT(CMessageMap* pObject, DWORD dwMsgMapID = 0)
		: m_lpszClassName(TBase::GetWndClassName()),
		m_pfnSuperWindowProc(::DefWindowProc),
		m_pObject(pObject), m_dwMsgMapID(dwMsgMapID),
		m_pCurrentMsg(NULL)
	{ }

	void SwitchMessageMap(DWORD dwMsgMapID)
	{
		m_dwMsgMapID = dwMsgMapID;
	}

	const _ATL_MSG* GetCurrentMessage() const
	{
		return m_pCurrentMsg;
	}

	LRESULT DefWindowProc()
	{
		const _ATL_MSG* pMsg = m_pCurrentMsg;
		LRESULT lRes = 0;
		if (pMsg != NULL)
			lRes = DefWindowProc(pMsg->message, pMsg->wParam, pMsg->lParam);
		return lRes;
	}

	LRESULT DefWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
#ifdef STRICT
		return ::CallWindowProc(m_pfnSuperWindowProc, m_hWnd, uMsg, wParam, lParam);
#else
		return ::CallWindowProc((FARPROC)m_pfnSuperWindowProc, m_hWnd, uMsg, wParam, lParam);
#endif
	}
	static LRESULT CALLBACK StartWindowProc(HWND hWnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam)
	{
		CContainedWindowT< TBase >* pThis = (CContainedWindowT< TBase >*)_AtlWinModule.ExtractCreateWndData();
		ATLASSERT(pThis != NULL);
		pThis->m_hWnd = hWnd;
		pThis->m_thunk.Init(WindowProc, pThis);
		WNDPROC pProc = pThis->m_thunk.GetWNDPROC();
		WNDPROC pOldProc = (WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)pProc);
#ifdef _DEBUG
		// check if somebody has subclassed us already since we discard it
		if(pOldProc != StartWindowProc)
			ATLTRACE(atlTraceWindowing, 0, _T("Subclassing through a hook discarded.\n"));
#else
		pOldProc;	// avoid unused warning
#endif
		return pProc(hWnd, uMsg, wParam, lParam);
	}

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		CContainedWindowT< TBase >* pThis = (CContainedWindowT< TBase >*)hWnd;
		ATLASSERT(pThis->m_hWnd != NULL);
		ATLASSERT(pThis->m_pObject != NULL);
		// set a ptr to this message and save the old value
		_ATL_MSG msg(pThis->m_hWnd, uMsg, wParam, lParam);
		const _ATL_MSG* pOldMsg = pThis->m_pCurrentMsg;
		pThis->m_pCurrentMsg = &msg;
		// pass to the message map to process
		LRESULT lRes;
		BOOL bRet = pThis->m_pObject->ProcessWindowMessage(pThis->m_hWnd, uMsg, wParam, lParam, lRes, pThis->m_dwMsgMapID);
		// restore saved value for the current message
		ATLASSERT(pThis->m_pCurrentMsg == &msg);
		pThis->m_pCurrentMsg = pOldMsg;
		// do the default processing if message was not handled
		if(!bRet)
		{
			if(uMsg != WM_NCDESTROY)
				lRes = pThis->DefWindowProc(uMsg, wParam, lParam);
			else
			{
				// unsubclass, if needed
				LONG_PTR pfnWndProc = ::GetWindowLongPtr(pThis->m_hWnd, GWLP_WNDPROC);
				lRes = pThis->DefWindowProc(uMsg, wParam, lParam);
				if(pThis->m_pfnSuperWindowProc != ::DefWindowProc && ::GetWindowLongPtr(pThis->m_hWnd, GWLP_WNDPROC) == pfnWndProc)
					::SetWindowLongPtr(pThis->m_hWnd, GWLP_WNDPROC, (LONG_PTR)pThis->m_pfnSuperWindowProc);
				// clear out window handle
				pThis->m_hWnd = NULL;
			}
		}
		return lRes;
	}

	ATOM RegisterWndSuperclass()
	{
		ATOM atom = 0;
		LPTSTR szBuff = (LPTSTR)_alloca((lstrlen(m_lpszClassName) + 14) * sizeof(TCHAR));

		WNDCLASSEX wc;
		wc.cbSize = sizeof(WNDCLASSEX);

		// Try global class
		if(!::GetClassInfoEx(NULL, m_lpszClassName, &wc))
		{
			// try local class
			if(!::GetClassInfoEx(_AtlBaseModule.GetModuleInstance(), m_lpszClassName, &wc))
				return atom;
		}

		m_pfnSuperWindowProc = wc.lpfnWndProc;
		lstrcpy(szBuff, _T("ATL:"));
		lstrcat(szBuff, m_lpszClassName);

		WNDCLASSEX wc1;
		wc1.cbSize = sizeof(WNDCLASSEX);
		atom = (ATOM)::GetClassInfoEx(_AtlBaseModule.GetModuleInstance(), szBuff, &wc1);

		if(atom == 0)   // register class
		{
			wc.lpszClassName = szBuff;
			wc.lpfnWndProc = StartWindowProc;
			wc.hInstance = _AtlBaseModule.GetModuleInstance();
			wc.style &= ~CS_GLOBALCLASS;	// we don't register global classes

			atom = AtlWinModuleRegisterClassEx(&_AtlWinModule, &wc);
		}
		return atom;
	}

	HWND Create(HWND hWndParent, _U_RECT rect, LPCTSTR szWindowName = NULL,
		DWORD dwStyle = 0, DWORD dwExStyle = 0,
		_U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL)
	{
		ATLASSERT(m_hWnd == NULL);

		ATOM atom = RegisterWndSuperclass();
		if(atom == 0)
			return NULL;

		_AtlWinModule.AddCreateWndData(&m_thunk.cd, this);

		if(MenuOrID.m_hMenu == NULL && (dwStyle & WS_CHILD))
			MenuOrID.m_hMenu = (HMENU)(UINT_PTR)this;
		if(rect.m_lpRect == NULL)
			rect.m_lpRect = &TBase::rcDefault;

		dwStyle = TWinTraits::GetWndStyle(dwStyle);
		dwExStyle = TWinTraits::GetWndExStyle(dwExStyle);

		HWND hWnd = ::CreateWindowEx(dwExStyle, MAKEINTATOM(atom), szWindowName,
								dwStyle,
								rect.m_lpRect->left, rect.m_lpRect->top,
								rect.m_lpRect->right - rect.m_lpRect->left,
								rect.m_lpRect->bottom - rect.m_lpRect->top,
								hWndParent, MenuOrID.m_hMenu,
								_AtlBaseModule.GetModuleInstance(), lpCreateParam);
		ATLASSERT(m_hWnd == hWnd);
		return hWnd;
	}

	HWND Create(CMessageMap* pObject, DWORD dwMsgMapID, HWND hWndParent, _U_RECT rect,
		LPCTSTR szWindowName = NULL, DWORD dwStyle = 0, DWORD dwExStyle = 0,
		_U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL)
	{
		m_lpszClassName = TBase::GetWndClassName();
		m_pfnSuperWindowProc = ::DefWindowProc;
		m_pObject = pObject;
		m_dwMsgMapID = dwMsgMapID;
		return Create(hWndParent, rect, szWindowName, dwStyle, dwExStyle, MenuOrID, lpCreateParam);
	}

	HWND Create(LPCTSTR lpszClassName, CMessageMap* pObject, DWORD dwMsgMapID, HWND hWndParent, _U_RECT rect, LPCTSTR szWindowName = NULL,
		DWORD dwStyle = 0, DWORD dwExStyle = 0, _U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL)
	{
		m_lpszClassName = lpszClassName;
		m_pfnSuperWindowProc = ::DefWindowProc;
		m_pObject = pObject;
		m_dwMsgMapID = dwMsgMapID;
		return Create(hWndParent, rect, szWindowName, dwStyle, dwExStyle, MenuOrID, lpCreateParam);
	}

	BOOL SubclassWindow(HWND hWnd)
	{
		ATLASSERT(m_hWnd == NULL);
		ATLASSERT(::IsWindow(hWnd));

		m_thunk.Init(WindowProc, this);
		WNDPROC pProc = m_thunk.GetWNDPROC();
		WNDPROC pfnWndProc = (WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)pProc);
		if(pfnWndProc == NULL)
			return FALSE;
		m_pfnSuperWindowProc = pfnWndProc;
		m_hWnd = hWnd;
		return TRUE;
	}

	// Use only if you want to subclass before window is destroyed,
	// WindowProc will automatically subclass when  window goes away
	HWND UnsubclassWindow(BOOL bForce = FALSE)
	{
		ATLASSERT(m_hWnd != NULL);

		WNDPROC pOurProc = m_thunk.GetWNDPROC();
		WNDPROC pActiveProc = (WNDPROC)::GetWindowLongPtr(m_hWnd, GWLP_WNDPROC);

		HWND hWnd = NULL;
		if (bForce || pOurProc == pActiveProc)
		{
			if(!::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_pfnSuperWindowProc))
				return NULL;

			m_pfnSuperWindowProc = ::DefWindowProc;
			hWnd = m_hWnd;
			m_hWnd = NULL;
		}
		return hWnd;
	}
	LRESULT ReflectNotifications(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		HWND hWndChild = NULL;

		switch(uMsg)
		{
		case WM_COMMAND:
			if(lParam != NULL)	// not from a menu
				hWndChild = (HWND)lParam;
			break;
		case WM_NOTIFY:
			hWndChild = ((LPNMHDR)lParam)->hwndFrom;
			break;
		case WM_PARENTNOTIFY:
			switch(LOWORD(wParam))
			{
			case WM_CREATE:
			case WM_DESTROY:
				hWndChild = (HWND)lParam;
				break;
			default:
				hWndChild = GetDlgItem(HIWORD(wParam));
				break;
			}
			break;
		case WM_DRAWITEM:
			if(wParam)	// not from a menu
				hWndChild = ((LPDRAWITEMSTRUCT)lParam)->hwndItem;
			break;
		case WM_MEASUREITEM:
			if(wParam)	// not from a menu
				hWndChild = GetDlgItem(((LPMEASUREITEMSTRUCT)lParam)->CtlID);
			break;
		case WM_COMPAREITEM:
			if(wParam)	// not from a menu
				hWndChild = GetDlgItem(((LPCOMPAREITEMSTRUCT)lParam)->CtlID);
			break;
		case WM_DELETEITEM:
			if(wParam)	// not from a menu
				hWndChild = GetDlgItem(((LPDELETEITEMSTRUCT)lParam)->CtlID);
			break;
		case WM_VKEYTOITEM:
		case WM_CHARTOITEM:
		case WM_HSCROLL:
		case WM_VSCROLL:
			hWndChild = (HWND)lParam;
			break;
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORMSGBOX:
		case WM_CTLCOLORSCROLLBAR:
		case WM_CTLCOLORSTATIC:
			hWndChild = (HWND)lParam;
			break;
		default:
			break;
		}

		if(hWndChild == NULL)
		{
			bHandled = FALSE;
			return 1;
		}

		ATLASSERT(::IsWindow(hWndChild));
		return ::SendMessage(hWndChild, OCM__BASE + uMsg, wParam, lParam);
	}
};

typedef CContainedWindowT<CWindow>	CContainedWindow;

/////////////////////////////////////////////////////////////////////////////
// _DialogSizeHelper - helpers for calculating the size of a dialog template

class _DialogSizeHelper
{
public:
//local struct used for implementation
#pragma pack(push, 1)
	struct _ATL_DLGTEMPLATEEX
	{
		WORD dlgVer;
		WORD signature;
		DWORD helpID;
		DWORD exStyle;
		DWORD style;
		WORD cDlgItems;
		short x;
		short y;
		short cx;
		short cy;
	};
#pragma pack(pop)

	static void GetDialogSize(const DLGTEMPLATE* pTemplate, SIZE* pSize, bool bPropertyPage = false)
	{
		// If the dialog has a font we use it otherwise we default
		// to the system font.
		TCHAR szFace[LF_FACESIZE];
		WORD  wFontSize = 0;
		GetSizeInDialogUnits(pTemplate, pSize);
		BOOL bFont = GetFont(pTemplate, szFace, &wFontSize);
		if (bFont)
		{
			ConvertDialogUnitsToPixels(szFace, wFontSize, pSize, bPropertyPage);
		}
		else
		{
			ConvertDialogUnitsToPixels(NULL, 0, pSize, bPropertyPage);
		}
	}

	static void GetFontDimensions(LPCTSTR pszFontFace, WORD wFontSize, SIZE* pSizeChar, LONG *ptmHeight)
	{
		if (pszFontFace != NULL)
		{
			// Attempt to create the font to be used in the dialog box
			HDC hDC = ::GetDC(NULL);
			if (hDC != NULL)
			{
				LOGFONT lf;
				memset(&lf, 0, sizeof(LOGFONT));
				lf.lfHeight = -MulDiv(wFontSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
				lf.lfWeight = FW_NORMAL;
				lf.lfCharSet = DEFAULT_CHARSET;
				lstrcpyn(lf.lfFaceName, pszFontFace, sizeof(lf.lfFaceName) / sizeof(lf.lfFaceName[0]));

				HFONT hNewFont = CreateFontIndirect(&lf);
				if (hNewFont != NULL)
				{
					TEXTMETRIC  tm;
					SIZE        size;
					HFONT       hFontOld = (HFONT)SelectObject(hDC, hNewFont);
					GetTextMetrics(hDC, &tm);
					::GetTextExtentPoint(hDC,
						_T("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"), 52,
						&size);
					SelectObject(hDC, hFontOld);
					DeleteObject(hNewFont);
					*ptmHeight = tm.tmHeight;
					pSizeChar->cy = tm.tmHeight + tm.tmExternalLeading;
					pSizeChar->cx = (size.cx + 26) / 52;
					::ReleaseDC(NULL, hDC);
					return;
				}
				::ReleaseDC(NULL, hDC);
			}
		}
		// Could not create font or no font name was not specified
		LONG nDlgBaseUnits = GetDialogBaseUnits();
		pSizeChar->cx = LOWORD(nDlgBaseUnits);
		*ptmHeight = pSizeChar->cy = HIWORD(nDlgBaseUnits);
	}

// ID of the dialog template used for property sheet in comctl32.dll
#define IDD_PROPSHEET_ID   1006

	static void ConvertDialogUnitsToPixels(LPCTSTR pszFontFace, WORD wFontSize, SIZE* pSizePixel, bool bPropertyPage = false)
	{
		LONG tmHeight;
		SIZE sizeFontChar;
		GetFontDimensions(pszFontFace, wFontSize, &sizeFontChar, &tmHeight);
		if (bPropertyPage)
		{
			// Get the font used by the property sheet
			HINSTANCE hInst = LoadLibrary(_T("COMCTL32.DLL"));
			if (hInst != NULL)
			{
				HRSRC hResource = ::FindResource(hInst,
					MAKEINTRESOURCE(IDD_PROPSHEET_ID),
					RT_DIALOG);
				if (hResource != NULL)
				{
					HGLOBAL hTemplate = LoadResource(hInst, hResource);
					if (hTemplate != NULL)
					{
						TCHAR szFace[LF_FACESIZE];
						WORD wSize;
						BOOL bFont;
						bFont = _DialogSizeHelper::GetFont((DLGTEMPLATE*)hTemplate, szFace,	&wSize);
						FreeLibrary(hInst);
						if (bFont)
						{
							SIZE sizeSheetFontChar;
							LONG tmHeightSheetFont;
							GetFontDimensions(szFace, wSize, &sizeSheetFontChar, &tmHeightSheetFont);
							// Now translate to pixels compensating for the calculations made by OLEAUT32 and Dialog manager

							// Calculate the size of pixels using property sheet font.
							pSizePixel->cx = MulDiv(pSizePixel->cx, sizeFontChar.cx, 4);
							pSizePixel->cy = MulDiv(pSizePixel->cy, sizeSheetFontChar.cy, 8);

							// Inflate/Deflate the height to compensate for the correct font.
							pSizePixel->cy = MulDiv(pSizePixel->cy, tmHeight, tmHeightSheetFont);
							return ;
						}
					}
				}
			}
		}
		// Not property page or could not load Property sheet resource.
		// Translate dialog units to pixels
		pSizePixel->cx = MulDiv(pSizePixel->cx, sizeFontChar.cx, 4);
		pSizePixel->cy = MulDiv(pSizePixel->cy, sizeFontChar.cy, 8);
	}

	static BOOL IsDialogEx(const DLGTEMPLATE* pTemplate)
	{
		return ((_ATL_DLGTEMPLATEEX*)pTemplate)->signature == 0xFFFF;
	}

	static BOOL HasFont(const DLGTEMPLATE* pTemplate)
	{
		return (DS_SETFONT &
			(IsDialogEx(pTemplate) ?
				((_ATL_DLGTEMPLATEEX*)pTemplate)->style : pTemplate->style));
	}

	static BYTE* GetFontSizeField(const DLGTEMPLATE* pTemplate)
	{
		BOOL bDialogEx = IsDialogEx(pTemplate);
		WORD* pw;

		if (bDialogEx)
			pw = (WORD*)((_ATL_DLGTEMPLATEEX*)pTemplate + 1);
		else
			pw = (WORD*)(pTemplate + 1);

		if (*pw == (WORD)-1)        // Skip menu name string or ordinal
			pw += 2; // WORDs
		else
			while(*pw++);

		if (*pw == (WORD)-1)        // Skip class name string or ordinal
			pw += 2; // WORDs
		else
			while(*pw++);

		while (*pw++);          // Skip caption string

		return (BYTE*)pw;
	}

	static BOOL GetFont(const DLGTEMPLATE* pTemplate, TCHAR* pszFace, WORD* pFontSize)
	{
		USES_CONVERSION;
		if (!HasFont(pTemplate))
			return FALSE;

		BYTE* pb = GetFontSizeField(pTemplate);
		*pFontSize = *(WORD*)pb;
		// Skip over font attributes to get to the font name
		pb += sizeof(WORD) * (IsDialogEx(pTemplate) ? 3 : 1);

		_tcsncpy(pszFace, W2T((WCHAR*)pb), LF_FACESIZE);
		if (_tcslen(pszFace) >= LF_FACESIZE) 
		{	// NUL not appended
			pszFace[LF_FACESIZE-1] = _T('\0');
		}
		return TRUE;
	}

	static void GetSizeInDialogUnits(const DLGTEMPLATE* pTemplate, SIZE* pSize)
	{
		if (IsDialogEx(pTemplate))
		{
			pSize->cx = ((_ATL_DLGTEMPLATEEX*)pTemplate)->cx;
			pSize->cy = ((_ATL_DLGTEMPLATEEX*)pTemplate)->cy;
		}
		else
		{
			pSize->cx = pTemplate->cx;
			pSize->cy = pTemplate->cy;
		}
	}
};

inline void AtlGetDialogSize(const DLGTEMPLATE* pTemplate, SIZE* pSize, bool bPropertyPage = false)
{
	ATLASSERT(pTemplate != NULL);
	ATLASSERT(pSize != NULL);
	_DialogSizeHelper::GetDialogSize(pTemplate, pSize, bPropertyPage);
}

}; //namespace ATL

#ifndef _ATL_NO_HOSTING

#include <atlhost.h>

#endif

#endif // __ATLWIN_H__

//All exports go here
#ifndef _ATL_DLL

namespace ATL
{

class AtlModuleRegisterWndClassInfoParamA
{
public:
	typedef LPSTR				PXSTR;
	typedef LPCSTR				PCXSTR;
	typedef _ATL_WNDCLASSINFOA	_ATL_WNDCLASSINFO;
	typedef WNDCLASSEXA			WNDCLASSEX;

	static BOOL GetClassInfoEx(HINSTANCE hinst, PCXSTR lpszClass, WNDCLASSEX* lpwcx)
	{
		return ::GetClassInfoExA(hinst, lpszClass, lpwcx);
	}

	static void FormatWindowClassName(PXSTR szBuffer, void* unique)
	{
#if defined(_WIN64) // || or Windows 2000
		::wsprintfA(szBuffer, "ATL:%p", unique);
#else
		::wsprintfA(szBuffer, "ATL:%8.8X", reinterpret_cast<DWORD_PTR>(unique));
#endif
	}

	static HCURSOR LoadCursor(HINSTANCE hInstance, PCXSTR lpCursorName)
	{
		return ::LoadCursorA(hInstance, lpCursorName);
	}

	static ATOM RegisterClassEx(_ATL_WIN_MODULE* pWinModule, const WNDCLASSEX* lpwcx)
	{
		return AtlWinModuleRegisterClassExA(pWinModule, lpwcx);
	}
};

class AtlModuleRegisterWndClassInfoParamW
{
public:
	typedef LPWSTR				PXSTR;
	typedef LPCWSTR				PCXSTR;
	typedef _ATL_WNDCLASSINFOW	_ATL_WNDCLASSINFO;
	typedef WNDCLASSEXW			WNDCLASSEX;

	static BOOL GetClassInfoEx(HINSTANCE hinst, PCXSTR lpszClass, WNDCLASSEX* lpwcx)
	{
		return ::GetClassInfoExW(hinst, lpszClass, lpwcx);
	}

	static void FormatWindowClassName(PXSTR szBuffer, void* unique)
	{
#if defined(_WIN64) // || or Windows 2000
		::wsprintfW(szBuffer, L"ATL:%p", unique);
#else
		::wsprintfW(szBuffer, L"ATL:%8.8X", reinterpret_cast<DWORD_PTR>(unique));
#endif
	}

	static HCURSOR LoadCursor(HINSTANCE hInstance, PCXSTR lpCursorName)
	{
		return ::LoadCursorW(hInstance, lpCursorName);
	}

	static ATOM RegisterClassEx(_ATL_WIN_MODULE* pWinModule, const WNDCLASSEX* lpwcx)
	{
		return AtlWinModuleRegisterClassExW(pWinModule, lpwcx);
	}
};

ATLINLINE ATLAPI_(ATOM) AtlWinModuleRegisterClassExA(_ATL_WIN_MODULE* pWinModule, const WNDCLASSEXA *lpwc)
{
	if (pWinModule == NULL || lpwc == NULL)
		return 0;
	ATOM atom = ::RegisterClassExA(lpwc);
	BOOL bRet = pWinModule->m_rgWindowClassAtoms.Add(atom);
	ATLASSERT(bRet);
	(bRet);
	return atom;
}

ATLINLINE ATLAPI_(ATOM) AtlWinModuleRegisterClassExW(_ATL_WIN_MODULE* pWinModule, const WNDCLASSEXW *lpwc)
{
	if (pWinModule == NULL || lpwc == NULL)
		return 0;
	ATOM atom = ::RegisterClassExW(lpwc);
	BOOL bRet = pWinModule->m_rgWindowClassAtoms.Add(atom);
	ATLASSERT(bRet);
	(bRet);
	return atom;
}


template <class T>
ATLINLINE ATOM AtlModuleRegisterWndClassInfoT(_ATL_BASE_MODULE* pBaseModule, _ATL_WIN_MODULE* pWinModule, typename T::_ATL_WNDCLASSINFO* p, WNDPROC* pProc, T)
{
	if (pBaseModule == NULL || pWinModule == NULL || p == NULL || pProc == NULL)
	{
		ATLTRACE(atlTraceWindowing, 0, _T("ERROR : Invalid Arguments to AtlModuleRegisterWndClassInfoT\n"));
		ATLASSERT(0);
		return 0;
	}
		
	if (p->m_atom == 0)
	{
		ATL::CComCritSecLock<ATL::CComCriticalSection> lock(pWinModule->m_csWindowCreate, false);
		if (FAILED(lock.Lock()))
		{
			ATLTRACE(atlTraceWindowing, 0, _T("ERROR : Unable to lock critical section in AtlModuleRegisterWndClassInfoT\n"));
			ATLASSERT(0);
			return 0;
		}
		if(p->m_atom == 0)
		{
			if (p->m_lpszOrigName != NULL)
			{
				ATLASSERT(pProc != NULL);
				T::PCXSTR lpsz = p->m_wc.lpszClassName;
				WNDPROC proc = p->m_wc.lpfnWndProc;

				T::WNDCLASSEX wc;
				wc.cbSize = sizeof(T::WNDCLASSEX);
				// Try global class
				if(!T::GetClassInfoEx(NULL, p->m_lpszOrigName, &wc))
				{
					// try process local
					if(!T::GetClassInfoEx(pBaseModule->m_hInst, p->m_lpszOrigName, &wc))
					{
						ATLTRACE(atlTraceWindowing, 0, "ERROR : Could not obtain Window Class information for %s\n", p->m_lpszOrigName);
						return 0;
					}
				}
				p->m_wc = wc;
				p->pWndProc = p->m_wc.lpfnWndProc;
				p->m_wc.lpszClassName = lpsz;
				p->m_wc.lpfnWndProc = proc;
			}
			else
			{
				p->m_wc.hCursor = T::LoadCursor(p->m_bSystemCursor ? NULL : pBaseModule->m_hInstResource,
					p->m_lpszCursorID);
			}

			p->m_wc.hInstance = pBaseModule->m_hInst;
			p->m_wc.style &= ~CS_GLOBALCLASS;	// we don't register global classes
			if (p->m_wc.lpszClassName == NULL)
			{
				T::FormatWindowClassName(p->m_szAutoName, &p->m_wc);
				p->m_wc.lpszClassName = p->m_szAutoName;
			}
			T::WNDCLASSEX wcTemp;
			wcTemp = p->m_wc;
			p->m_atom = static_cast<ATOM>(T::GetClassInfoEx(p->m_wc.hInstance, p->m_wc.lpszClassName, &wcTemp));
			if (p->m_atom == 0)
			{
				p->m_atom = T::RegisterClassEx(pWinModule, &p->m_wc);
			}
		}
	}

	if (p->m_lpszOrigName != NULL)
	{
		ATLASSERT(pProc != NULL);
		ATLASSERT(p->pWndProc != NULL);
		*pProc = p->pWndProc;
	}
	return p->m_atom;
}

ATLINLINE ATLAPI_(ATOM) AtlWinModuleRegisterWndClassInfoA(_ATL_WIN_MODULE* pWinModule, _ATL_BASE_MODULE* pBaseModule, _ATL_WNDCLASSINFOA* p, WNDPROC* pProc)
{
	AtlModuleRegisterWndClassInfoParamA templateParameter;
	return AtlModuleRegisterWndClassInfoT<AtlModuleRegisterWndClassInfoParamA>(pBaseModule, pWinModule, p, pProc, templateParameter);
}

ATLINLINE ATLAPI_(ATOM) AtlWinModuleRegisterWndClassInfoW(_ATL_WIN_MODULE* pWinModule, _ATL_BASE_MODULE* pBaseModule, _ATL_WNDCLASSINFOW* p, WNDPROC* pProc)
{
	AtlModuleRegisterWndClassInfoParamW templateParameter;
	return AtlModuleRegisterWndClassInfoT<AtlModuleRegisterWndClassInfoParamW>(pBaseModule, pWinModule, p, pProc, templateParameter);
}

ATLINLINE ATLAPI_(HDC) AtlCreateTargetDC(HDC hdc, DVTARGETDEVICE* ptd)
{
	USES_CONVERSION;

	// cases  hdc, ptd, hdc is metafile, hic
//  NULL,    NULL,  n/a,    Display
//  NULL,   !NULL,  n/a,    ptd
//  !NULL,   NULL,  FALSE,  hdc
//  !NULL,   NULL,  TRUE,   display
//  !NULL,  !NULL,  FALSE,  ptd
//  !NULL,  !NULL,  TRUE,   ptd

	if (ptd != NULL)
	{
		LPDEVMODEOLE lpDevMode;
		LPOLESTR lpszDriverName;
		LPOLESTR lpszDeviceName;
		LPOLESTR lpszPortName;

		if (ptd->tdExtDevmodeOffset == 0)
			lpDevMode = NULL;
		else
			lpDevMode  = (LPDEVMODEOLE) ((LPSTR)ptd + ptd->tdExtDevmodeOffset);

		lpszDriverName = (LPOLESTR)((BYTE*)ptd + ptd->tdDriverNameOffset);
		lpszDeviceName = (LPOLESTR)((BYTE*)ptd + ptd->tdDeviceNameOffset);
		lpszPortName   = (LPOLESTR)((BYTE*)ptd + ptd->tdPortNameOffset);

		return ::CreateDC(OLE2CT(lpszDriverName), OLE2CT(lpszDeviceName),
			OLE2CT(lpszPortName), DEVMODEOLE2T(lpDevMode));
	}
	else if (hdc == NULL || GetDeviceCaps(hdc, TECHNOLOGY) == DT_METAFILE)
		return ::CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
	else
		return hdc;
}

/////////////////////////////////////////////////////////////////////////////
// Windowing : Conversion helpers

ATLINLINE ATLAPI_(void) AtlHiMetricToPixel(const SIZEL * lpSizeInHiMetric, LPSIZEL lpSizeInPix)
{
	int nPixelsPerInchX;    // Pixels per logical inch along width
	int nPixelsPerInchY;    // Pixels per logical inch along height

	HDC hDCScreen = GetDC(NULL);
	ATLASSERT(hDCScreen != NULL);
	nPixelsPerInchX = GetDeviceCaps(hDCScreen, LOGPIXELSX);
	nPixelsPerInchY = GetDeviceCaps(hDCScreen, LOGPIXELSY);
	ReleaseDC(NULL, hDCScreen);

	lpSizeInPix->cx = MAP_LOGHIM_TO_PIX(lpSizeInHiMetric->cx, nPixelsPerInchX);
	lpSizeInPix->cy = MAP_LOGHIM_TO_PIX(lpSizeInHiMetric->cy, nPixelsPerInchY);
}

ATLINLINE ATLAPI_(void) AtlPixelToHiMetric(const SIZEL * lpSizeInPix, LPSIZEL lpSizeInHiMetric)
{
	int nPixelsPerInchX;    // Pixels per logical inch along width
	int nPixelsPerInchY;    // Pixels per logical inch along height

	HDC hDCScreen = GetDC(NULL);
	ATLASSERT(hDCScreen != NULL);
	nPixelsPerInchX = GetDeviceCaps(hDCScreen, LOGPIXELSX);
	nPixelsPerInchY = GetDeviceCaps(hDCScreen, LOGPIXELSY);
	ReleaseDC(NULL, hDCScreen);

	lpSizeInHiMetric->cx = MAP_PIX_TO_LOGHIM(lpSizeInPix->cx, nPixelsPerInchX);
	lpSizeInHiMetric->cy = MAP_PIX_TO_LOGHIM(lpSizeInPix->cy, nPixelsPerInchY);
}

} //namespace ATL

#endif // !_ATL_DLL

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning (pop)
#endif
