// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __STATREG_H__
#define __STATREG_H__

#pragma once

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLBASE_H__
	#error statreg.h requires atlbase.h to be included first
#endif

#define E_ATL_REGISTRAR_DESC              0x0201
#define E_ATL_NOT_IN_MAP                  0x0202
#define E_ATL_UNEXPECTED_EOS              0x0203
#define E_ATL_VALUE_SET_FAILED            0x0204
#define E_ATL_RECURSE_DELETE_FAILED       0x0205
#define E_ATL_EXPECTING_EQUAL             0x0206
#define E_ATL_CREATE_KEY_FAILED           0x0207
#define E_ATL_DELETE_KEY_FAILED           0x0208
#define E_ATL_OPEN_KEY_FAILED             0x0209
#define E_ATL_CLOSE_KEY_FAILED            0x020A
#define E_ATL_UNABLE_TO_COERCE            0x020B
#define E_ATL_BAD_HKEY                    0x020C
#define E_ATL_MISSING_OPENKEY_TOKEN       0x020D
#define E_ATL_CONVERT_FAILED              0x020E
#define E_ATL_TYPE_NOT_SUPPORTED          0x020F
#define E_ATL_COULD_NOT_CONCAT            0x0210
#define E_ATL_COMPOUND_KEY                0x0211
#define E_ATL_INVALID_MAPKEY              0x0212
#define E_ATL_UNSUPPORTED_VT              0x0213
#define E_ATL_VALUE_GET_FAILED            0x0214
#define E_ATL_VALUE_TOO_LARGE             0x0215
#define E_ATL_MISSING_VALUE_DELIMETER     0x0216
#define E_ATL_DATA_NOT_BYTE_ALIGNED       0x0217

namespace ATL
{
const TCHAR  chDirSep            = _T('\\');
const TCHAR  chRightBracket      = _T('}');
const TCHAR  chLeftBracket       = _T('{');
const TCHAR  chQuote             = _T('\'');
const TCHAR  chEquals            = _T('=');
const LPCTSTR  szStringVal       = _T("S");
const LPCTSTR  multiszStringVal  = _T("M");
const LPCTSTR  szDwordVal        = _T("D");
const LPCTSTR  szBinaryVal       = _T("B");
const LPCTSTR  szValToken        = _T("Val");
const LPCTSTR  szForceRemove     = _T("ForceRemove");
const LPCTSTR  szNoRemove        = _T("NoRemove");
const LPCTSTR  szDelete          = _T("Delete");


// Implementation helper
class CExpansionVectorEqualHelper
{
public:
	static bool IsEqualKey(const LPTSTR k1, const LPTSTR k2)
	{
		if (lstrcmpi(k1, k2) == 0)
			return true;
		return false;
	}

	// Not used
	static bool IsEqualValue(const LPCOLESTR /*v1*/, const LPCOLESTR /*v2*/)
	{
		return false;
	}
};

// Implementation helper
class CExpansionVector : public CSimpleMap<LPTSTR, LPOLESTR, CExpansionVectorEqualHelper >
{
public:
	~CExpansionVector()
	{
		 ClearReplacements();
	}

	BOOL Add(LPCTSTR lpszKey, LPCOLESTR lpszValue)
	{
		ATLASSERT(lpszKey != NULL && lpszValue != NULL);
		if (lpszKey == NULL || lpszValue == NULL)
			return FALSE;

		HRESULT hRes = S_OK;

		size_t cbKey = (lstrlen(lpszKey)+1)*sizeof(TCHAR);
		TCHAR* szKey = NULL;

		ATLTRY(szKey = new TCHAR[cbKey];)

		size_t cbValue = (ocslen(lpszValue)+1)*sizeof(OLECHAR);
		LPOLESTR szValue = NULL;
		ATLTRY(szValue = new OLECHAR[cbValue];)

		if (szKey == NULL || szValue == NULL)
			hRes = E_OUTOFMEMORY;
		else
		{
			memcpy(szKey, lpszKey, cbKey);
			memcpy(szValue, lpszValue, cbValue);
			if (!CSimpleMap<LPTSTR, LPOLESTR, CExpansionVectorEqualHelper>::Add(szKey, szValue))
				hRes = E_OUTOFMEMORY;
		}
		if (FAILED(hRes))
		{
			delete []szKey;
			delete []szValue;
		}
		return SUCCEEDED(hRes);
	}
	HRESULT ClearReplacements()
	{
		for (int i = 0; i < GetSize(); i++)
		{
			delete []GetKeyAt(i);
			delete []GetValueAt(i);
		}
		RemoveAll();
		return S_OK;
	}
};

class CRegObject;

class CRegParser
{
public:
	CRegParser(CRegObject* pRegObj);

	HRESULT  PreProcessBuffer(LPTSTR lpszReg, LPTSTR* ppszReg);
	HRESULT  RegisterBuffer(LPTSTR szReg, BOOL bRegister);

protected:

	void    SkipWhiteSpace();
	HRESULT NextToken(LPTSTR szToken);
	HRESULT AddValue(CRegKey& rkParent,LPCTSTR szValueName, LPTSTR szToken);
	BOOL    CanForceRemoveKey(LPCTSTR szKey);
	BOOL    HasSubKeys(HKEY hkey);
	BOOL    HasValues(HKEY hkey);
	HRESULT RegisterSubkeys(LPTSTR szToken, HKEY hkParent, BOOL bRegister, BOOL bInRecovery = FALSE);
	BOOL    IsSpace(TCHAR ch);
	LPTSTR  m_pchCur;

	CRegObject*     m_pRegObj;

	HRESULT GenerateError(UINT) {return DISP_E_EXCEPTION;}
	HRESULT HandleReplacements(LPTSTR& szToken);
	HRESULT SkipAssignment(LPTSTR szToken);

	BOOL    EndOfVar() { return chQuote == *m_pchCur && chQuote != *CharNext(m_pchCur); }
	static LPTSTR StrChr(LPTSTR lpsz, TCHAR ch);
	static HKEY HKeyFromString(LPTSTR szToken);
	static BYTE ChToByte(const TCHAR ch);
	static BOOL VTFromRegType(LPCTSTR szValueType, VARTYPE& vt);
	static const TCHAR* const rgszNeverDelete[];
	static const int cbNeverDelete;
	static const int MAX_VALUE = 4096;
	static const int MAX_TYPE = 4096;

	// Implementation Helper
	class CParseBuffer
	{
	public:
		int nPos;
		int nSize;
		LPTSTR p;
		CParseBuffer(int nInitial)
		{
			if (nInitial < 100)
				nInitial = 1000;
			nPos = 0;
			nSize = nInitial;
			p = (LPTSTR) CoTaskMemAlloc(nSize*sizeof(TCHAR));
			if (p != NULL)
				*p = NULL;
		}
		~CParseBuffer()
		{
			CoTaskMemFree(p);
		}
		BOOL Append(const TCHAR* pch, int nChars)
		{
			ATLASSERT(p != NULL);
			if ((nPos + nChars + 1) >= nSize)
			{
				while ((nPos + nChars + 1) >= nSize)
					nSize *=2;
				LPTSTR pTemp = (LPTSTR) CoTaskMemRealloc(p, nSize*sizeof(TCHAR));
				if (pTemp == NULL)
					return FALSE;
				p = pTemp;
			}
			memcpy(p + nPos, pch, int(nChars * sizeof(TCHAR)));
			nPos += nChars;
			*(p + nPos) = NULL;
			return TRUE;			
		}

		BOOL AddChar(const TCHAR* pch)
		{
#ifndef _UNICODE		
			int nChars = int(CharNext(pch) - pch);
#else
			int nChars = 1;
#endif
			return Append(pch, nChars);

		}
		BOOL AddString(LPCOLESTR lpsz)
		{
			if (lpsz == NULL)
				return FALSE;
			USES_CONVERSION_EX;
			LPCTSTR lpszT = OLE2CT_EX(lpsz, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifdef _UNICODE
			if (lpszT == NULL)
				return FALSE;
#endif
			return Append(lpszT, (int)lstrlen(lpszT));
		}
		LPTSTR Detach()
		{
			LPTSTR lp = p;
			p = NULL;
			nSize = nPos = 0;
			return lp;
		}

	};
};

#if defined(_ATL_DLL) || defined(_ATL_DLL_IMPL)
class ATL_NO_VTABLE CRegObject
 : public IRegistrar
#else
class CRegObject  : public IRegistrarBase
#endif
{
public:

#if defined(_ATL_DLL) || defined(_ATL_DLL_IMPL)

#else
	STDMETHOD(QueryInterface)(const IID &,void ** )
	{
		ATLASSERT(_T("statically linked in CRegObject is not a com object. Do not callthis function"));
		return E_NOTIMPL;
	}

	STDMETHOD_(ULONG, AddRef)(void)
	{
		ATLASSERT(_T("statically linked in CRegObject is not a com object. Do not callthis function"));
		return 1;
	}
	STDMETHOD_(ULONG, Release)(void)
	{
		ATLASSERT(_T("statically linked in CRegObject is not a com object. Do not callthis function"));
		return 0;
	}
#endif
	virtual ~CRegObject(){ClearReplacements();}
	HRESULT FinalConstruct() {return S_OK;}
	void FinalRelease() {}


	// Map based methods
	HRESULT STDMETHODCALLTYPE AddReplacement(LPCOLESTR lpszKey, LPCOLESTR lpszItem);
	HRESULT STDMETHODCALLTYPE ClearReplacements();
	LPCOLESTR StrFromMap(LPTSTR lpszKey);

	// Register via a given mechanism
	HRESULT STDMETHODCALLTYPE ResourceRegister(LPCOLESTR pszFileName, UINT nID, LPCOLESTR pszType);
	HRESULT STDMETHODCALLTYPE ResourceRegisterSz(LPCOLESTR pszFileName, LPCOLESTR pszID, LPCOLESTR pszType);
	HRESULT STDMETHODCALLTYPE ResourceUnregister(LPCOLESTR pszFileName, UINT nID, LPCOLESTR pszType);
	HRESULT STDMETHODCALLTYPE ResourceUnregisterSz(LPCOLESTR pszFileName, LPCOLESTR pszID, LPCOLESTR pszType);
	HRESULT STDMETHODCALLTYPE FileRegister(LPCOLESTR bstrFileName)
	{
		return CommonFileRegister(bstrFileName, TRUE);
	}

	HRESULT STDMETHODCALLTYPE FileUnregister(LPCOLESTR bstrFileName)
	{
		return CommonFileRegister(bstrFileName, FALSE);
	}

	HRESULT STDMETHODCALLTYPE StringRegister(LPCOLESTR bstrData)
	{
		return RegisterWithString(bstrData, TRUE);
	}

	HRESULT STDMETHODCALLTYPE StringUnregister(LPCOLESTR bstrData)
	{
		return RegisterWithString(bstrData, FALSE);
	}

protected:

	HRESULT CommonFileRegister(LPCOLESTR pszFileName, BOOL bRegister);
	HRESULT RegisterFromResource(LPCOLESTR pszFileName, LPCTSTR pszID, LPCTSTR pszType, BOOL bRegister);
	HRESULT RegisterWithString(LPCOLESTR pszData, BOOL bRegister);

	static HRESULT GenerateError(UINT) {return DISP_E_EXCEPTION;}

	CExpansionVector								m_RepMap;
	CComObjectThreadModel::AutoCriticalSection      m_csMap;
};

inline HRESULT STDMETHODCALLTYPE CRegObject::AddReplacement(LPCOLESTR lpszKey, LPCOLESTR lpszItem)
{
	if (lpszKey == NULL || lpszItem == NULL)
		return E_INVALIDARG;
	m_csMap.Lock();
	USES_CONVERSION_EX;
	
	LPCTSTR lpszT = OLE2CT_EX(lpszKey, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);

#ifndef _UNICODE
	if(lpszT == NULL)
		return E_OUTOFMEMORY;
#endif
	
	BOOL bRet = m_RepMap.Add(lpszT, lpszItem);
	m_csMap.Unlock();
	return bRet ? S_OK : E_OUTOFMEMORY;
}

inline HRESULT CRegObject::RegisterFromResource(LPCOLESTR bstrFileName, LPCTSTR szID,
										 LPCTSTR szType, BOOL bRegister)
{
	USES_CONVERSION_EX;

	HRESULT     hr;
	CRegParser  parser(this);
	HINSTANCE   hInstResDll;
	HRSRC       hrscReg;
	HGLOBAL     hReg;
	DWORD       dwSize;
	LPSTR       szRegA;
	CTempBuffer<TCHAR, 1024> szReg;	

	LPCTSTR lpszBSTRFileName = OLE2CT_EX(bstrFileName, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
	if (lpszBSTRFileName == NULL)
	{
		return E_OUTOFMEMORY;
	}
#endif // _UNICODE

	hInstResDll = LoadLibraryEx(lpszBSTRFileName, NULL, LOAD_LIBRARY_AS_DATAFILE);

	if (NULL == hInstResDll)
	{
		ATLTRACE(atlTraceRegistrar, 0, _T("Failed to LoadLibrary on %s\n"), bstrFileName);
		hr = AtlHresultFromLastError();
		goto ReturnHR;
	}

	hrscReg = FindResource((HMODULE)hInstResDll, szID, szType);

	if (NULL == hrscReg)
	{
		ATLTRACE(atlTraceRegistrar, 0, (HIWORD(szID) == NULL) ? 
			_T("Failed to FindResource on ID:%d TYPE:%s\n") : 
			_T("Failed to FindResource on ID:%s TYPE:%s\n"), 
			szID, szType);
		hr = AtlHresultFromLastError();
		goto ReturnHR;
	}

	hReg = LoadResource((HMODULE)hInstResDll, hrscReg);

	if (NULL == hReg)
	{
		ATLTRACE(atlTraceRegistrar, 0, _T("Failed to LoadResource\n"));
		hr = AtlHresultFromLastError();
		goto ReturnHR;
	}

	dwSize = SizeofResource((HMODULE)hInstResDll, hrscReg);
	szRegA = (LPSTR)hReg;

	// Allocate extra space for NULL.
	ATLTRY(szReg.Allocate(dwSize + 1));
	if (szReg == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto ReturnHR;
	}

#ifdef _UNICODE
	if (::MultiByteToWideChar(_AtlGetConversionACP(), 0, szRegA, dwSize, szReg, dwSize) == 0)
	{
		hr = AtlHresultFromLastError();
		goto ReturnHR;
	}
#else
	memcpy(szReg, szRegA, dwSize);
#endif

	// Append a NULL at the end.
	szReg[dwSize] = NULL;

	hr = parser.RegisterBuffer(szReg, bRegister);

ReturnHR:

	if (NULL != hInstResDll)
		FreeLibrary((HMODULE)hInstResDll);
	return hr;
}

inline HRESULT STDMETHODCALLTYPE CRegObject::ResourceRegister(LPCOLESTR szFileName, UINT nID, LPCOLESTR szType)
{
	USES_CONVERSION_EX;

	LPCTSTR lpszT = OLE2CT_EX(szType, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
	if (lpszT == NULL)
	{
		return E_OUTOFMEMORY;
	}
#endif // _UNICODE

	return RegisterFromResource(szFileName, MAKEINTRESOURCE(nID), lpszT, TRUE);
}

inline HRESULT STDMETHODCALLTYPE CRegObject::ResourceRegisterSz(LPCOLESTR szFileName, LPCOLESTR szID, LPCOLESTR szType)
{
	USES_CONVERSION_EX;
	if (szID == NULL || szType == NULL)
		return E_INVALIDARG;
	
	LPCTSTR lpszID = OLE2CT_EX(szID, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
	LPCTSTR lpszType = OLE2CT_EX(szType, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
	if (lpszID == NULL || lpszType)
	{
		return E_OUTOFMEMORY;
	}
#endif // _UNICODE
	return RegisterFromResource(szFileName, lpszID, lpszType, TRUE);
}

inline HRESULT STDMETHODCALLTYPE CRegObject::ResourceUnregister(LPCOLESTR szFileName, UINT nID, LPCOLESTR szType)
{
	USES_CONVERSION_EX;

	LPCTSTR lpszT = OLE2CT_EX(szType, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
	if (lpszT == NULL)
	{
		return E_OUTOFMEMORY;
	}
#endif // _UNICODE
	return RegisterFromResource(szFileName, MAKEINTRESOURCE(nID), lpszT, FALSE);
}

inline HRESULT STDMETHODCALLTYPE CRegObject::ResourceUnregisterSz(LPCOLESTR szFileName, LPCOLESTR szID, LPCOLESTR szType)
{
	USES_CONVERSION_EX;
	if (szID == NULL || szType == NULL)
		return E_INVALIDARG;

	LPCTSTR lpszID = OLE2CT_EX(szID, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
	LPCTSTR lpszType = OLE2CT_EX(szType, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
	if (lpszID == NULL || lpszType == NULL)
	{
		return E_OUTOFMEMORY;
	}
#endif // _UNICODE

	return RegisterFromResource(szFileName, lpszID, lpszType, FALSE);
}

inline HRESULT CRegObject::RegisterWithString(LPCOLESTR bstrData, BOOL bRegister)
{
	USES_CONVERSION_EX;
	CRegParser  parser(this);

	LPCTSTR szReg = OLE2CT_EX(bstrData, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
	if (szReg == NULL)
	{
		return E_OUTOFMEMORY;
	}
#endif // _UNICODE

	HRESULT hr = parser.RegisterBuffer((LPTSTR)szReg, bRegister);

	return hr;
}

inline HRESULT CRegObject::ClearReplacements()
{
	m_csMap.Lock();
	HRESULT hr = m_RepMap.ClearReplacements();
	m_csMap.Unlock();
	return hr;
}


inline LPCOLESTR CRegObject::StrFromMap(LPTSTR lpszKey)
{
	m_csMap.Lock();
	LPCOLESTR lpsz = m_RepMap.Lookup(lpszKey);
	if (lpsz == NULL) // not found!!
		ATLTRACE(atlTraceRegistrar, 0, _T("Map Entry not found\n"));
	m_csMap.Unlock();
	return lpsz;
}

inline HRESULT CRegObject::CommonFileRegister(LPCOLESTR bstrFileName, BOOL bRegister)
{
	USES_CONVERSION_EX;

	CRegParser  parser(this);

	LPCTSTR lpszBSTRFileName = OLE2CT_EX(bstrFileName, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
	if (lpszBSTRFileName == NULL)
	{
		return E_OUTOFMEMORY;
	}
#endif // _UNICODE

	HANDLE hFile = CreateFile(lpszBSTRFileName, GENERIC_READ, 0, NULL,
							  OPEN_EXISTING,
							  FILE_ATTRIBUTE_READONLY,
							  NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		ATLTRACE2(atlTraceRegistrar, 0, _T("Failed to CreateFile on %s\n"), lpszBSTRFileName);
		return AtlHresultFromLastError();
	}

	HRESULT hRes = S_OK;
	DWORD cbRead;
	DWORD cbFile = GetFileSize(hFile, NULL); // No HiOrder DWORD required

	CTempBuffer<char, 1024> szReg;
	// Extra space for NULL.
	ATLTRY(szReg.Allocate(cbFile + 1));
	if (szReg == NULL)
	{
		hRes = E_OUTOFMEMORY;
		goto ReturnHR;
	}

	if (ReadFile(hFile, szReg, cbFile, &cbRead, NULL) == 0)
	{
		ATLTRACE2(atlTraceRegistrar, 0, "Read Failed on file%s\n", lpszBSTRFileName);
		hRes =  AtlHresultFromLastError();
	}
	if (SUCCEEDED(hRes))
	{
		szReg[cbRead] = NULL;

#ifdef _UNICODE
		CTempBuffer<WCHAR, 1024> szConverted;
		ATLTRY(szConverted.Allocate(cbFile + 1));
		if (szConverted == NULL)
		{
			hRes =  E_OUTOFMEMORY;
			goto ReturnHR;

		}
		if (::MultiByteToWideChar(_AtlGetConversionACP(), 0, szReg, cbFile + 1, szConverted, cbFile + 1) == 0)
		{
			hRes = AtlHresultFromLastError();
			goto ReturnHR;
		}
#else
		LPTSTR szConverted = szReg; 
#endif		
		hRes = parser.RegisterBuffer(szConverted, bRegister);
	}
ReturnHR:
	CloseHandle(hFile);
	return hRes;
}

__declspec(selectany) const TCHAR* const CRegParser::rgszNeverDelete[] =
{
	_T("AppID"),
	_T("CLSID"),
	_T("Component Categories"),
	_T("FileType"),
	_T("Interface"),
	_T("Hardware"),
	_T("Mime"),
	_T("SAM"),
	_T("SECURITY"),
	_T("SYSTEM"),
	_T("Software"),
	_T("TypeLib")
};

__declspec(selectany) const int CRegParser::cbNeverDelete = sizeof(rgszNeverDelete) / sizeof(LPCTSTR*);


inline BOOL CRegParser::VTFromRegType(LPCTSTR szValueType, VARTYPE& vt)
{
	struct typemap
	{
		LPCTSTR lpsz;
		VARTYPE vt;
	};
#pragma warning (push)
#pragma warning (disable : 4640)	// construction of local static object is not thread-safe

	static const typemap map[] = {
		{szStringVal, VT_BSTR},
		{multiszStringVal, VT_BSTR | VT_BYREF},
		{szDwordVal,  VT_UI4},
		{szBinaryVal, VT_UI1}
	};

#pragma warning (pop)

	for (int i=0;i<sizeof(map)/sizeof(typemap);i++)
	{
		if (!lstrcmpi(szValueType, map[i].lpsz))
		{
			vt = map[i].vt;
			return TRUE;
		}
	}

	return FALSE;

}

inline BYTE CRegParser::ChToByte(const TCHAR ch)
{
	switch (ch)
	{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
				return (BYTE) (ch - '0');
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
				return (BYTE) (10 + (ch - 'A'));
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
				return (BYTE) (10 + (ch - 'a'));
		default:
				ATLASSERT(FALSE);
				ATLTRACE(atlTraceRegistrar, 0, _T("Bogus value %c passed as binary Hex value\n"), ch);
				return 0;
	}
}

inline HKEY CRegParser::HKeyFromString(LPTSTR szToken)
{
	struct keymap
	{
		LPCTSTR lpsz;
		HKEY hkey;
	};
	static const keymap map[] = {
		{_T("HKCR"), HKEY_CLASSES_ROOT},
		{_T("HKCU"), HKEY_CURRENT_USER},
		{_T("HKLM"), HKEY_LOCAL_MACHINE},
		{_T("HKU"),  HKEY_USERS},
		{_T("HKPD"), HKEY_PERFORMANCE_DATA},
		{_T("HKDD"), HKEY_DYN_DATA},
		{_T("HKCC"), HKEY_CURRENT_CONFIG},
		{_T("HKEY_CLASSES_ROOT"), HKEY_CLASSES_ROOT},
		{_T("HKEY_CURRENT_USER"), HKEY_CURRENT_USER},
		{_T("HKEY_LOCAL_MACHINE"), HKEY_LOCAL_MACHINE},
		{_T("HKEY_USERS"), HKEY_USERS},
		{_T("HKEY_PERFORMANCE_DATA"), HKEY_PERFORMANCE_DATA},
		{_T("HKEY_DYN_DATA"), HKEY_DYN_DATA},
		{_T("HKEY_CURRENT_CONFIG"), HKEY_CURRENT_CONFIG}
	};

	for (int i=0;i<sizeof(map)/sizeof(keymap);i++)
	{
		if (!lstrcmpi(szToken, map[i].lpsz))
			return map[i].hkey;
	}
	return NULL;
}

inline LPTSTR CRegParser::StrChr(LPTSTR lpsz, TCHAR ch)
{
	LPTSTR p = NULL;

	if (lpsz == NULL)
		return NULL;
	
	while (*lpsz)
	{
		if (*lpsz == ch)
		{
			p = lpsz;
			break;
		}
		lpsz = CharNext(lpsz);
	}
	return p;
}

inline CRegParser::CRegParser(CRegObject* pRegObj)
{
	m_pRegObj           = pRegObj;
	m_pchCur            = NULL;
}

inline BOOL CRegParser::IsSpace(TCHAR ch)
{
	switch (ch)
	{
		case _T(' '):
		case _T('\t'):
		case _T('\r'):
		case _T('\n'):
				return TRUE;
	}

	return FALSE;
}

inline void CRegParser::SkipWhiteSpace()
{
	while(IsSpace(*m_pchCur))
		m_pchCur = CharNext(m_pchCur);
}

inline HRESULT CRegParser::NextToken(LPTSTR szToken)
{
	SkipWhiteSpace();

	// NextToken cannot be called at EOS
	if (NULL == *m_pchCur)
		return GenerateError(E_ATL_UNEXPECTED_EOS);

	LPCTSTR szOrig = szToken;
	// handle quoted value / key
	if (chQuote == *m_pchCur)
	{
		m_pchCur = CharNext(m_pchCur);

		while (NULL != *m_pchCur && !EndOfVar())
		{
			if (chQuote == *m_pchCur) // If it is a quote that means we must skip it
				m_pchCur = CharNext(m_pchCur);

			LPTSTR pchPrev = m_pchCur;
			m_pchCur = CharNext(m_pchCur);

			if (szToken + sizeof(WORD) >= MAX_VALUE + szOrig)
				return GenerateError(E_ATL_VALUE_TOO_LARGE);
			for (int i = 0; pchPrev+i < m_pchCur; i++, szToken++)
				*szToken = *(pchPrev+i);
		}

		if (NULL == *m_pchCur)
		{
			ATLTRACE(atlTraceRegistrar, 0, _T("NextToken : Unexpected End of File\n"));
			return GenerateError(E_ATL_UNEXPECTED_EOS);
		}

		*szToken = NULL;
		m_pchCur = CharNext(m_pchCur);
	}

	else
	{   
		// Handle non-quoted ie parse up till first "White Space"
		while (NULL != *m_pchCur && !IsSpace(*m_pchCur))
		{
			LPTSTR pchPrev = m_pchCur;
			m_pchCur = CharNext(m_pchCur);
			if (szToken + sizeof(WORD) >= MAX_VALUE + szOrig)
				return GenerateError(E_ATL_VALUE_TOO_LARGE);
			for (int i = 0; pchPrev+i < m_pchCur; i++, szToken++)
				*szToken = *(pchPrev+i);
		}

		*szToken = NULL;
	}
	return S_OK;
}

inline HRESULT CRegParser::AddValue(CRegKey& rkParent,LPCTSTR szValueName, LPTSTR szToken)
{
	HRESULT hr;

	TCHAR		szValue[MAX_VALUE];
	VARTYPE     vt = VT_EMPTY;
	LONG        lRes = ERROR_SUCCESS;
	UINT        nIDRes = 0;

	if (FAILED(hr = NextToken(szValue)))
		return hr;
	if (!VTFromRegType(szValue, vt))
	{
		ATLTRACE(atlTraceRegistrar, 0, _T("%s Type not supported\n"), szValue);
		return GenerateError(E_ATL_TYPE_NOT_SUPPORTED);
	}

	SkipWhiteSpace();
	if (FAILED(hr = NextToken(szValue)))
		return hr;

	switch (vt)
	{
	case VT_BSTR:
		{
			lRes = rkParent.SetStringValue(szValueName, szValue);
			ATLTRACE(atlTraceRegistrar, 2, _T("Setting Value %s at %s\n"), szValue, !szValueName ? _T("default") : szValueName);
			break;
		}
	case VT_BSTR | VT_BYREF:
		{
			ATLTRACE(atlTraceRegistrar, 2, _T("Setting Value %s at %s\n"), szValue, !szValueName ? _T("default") : szValueName);
			int nLen = lstrlen(szValue) + 1;
			CTempBuffer<TCHAR, 256> pszDestValue;
			ATLTRY(pszDestValue.Allocate(nLen));
			if (pszDestValue != NULL)
			{
				TCHAR* p = pszDestValue;
				TCHAR* q = szValue;
				nLen = 0;
				while (*q != NULL)
				{
					TCHAR* r = CharNext(q);
					if (*q == '\\' && *r == '0')
					{
						*p++ = NULL;
						q = CharNext(r);
					}
					else
					{
						*p = *q;
#ifndef _UNICODE
						if (IsDBCSLeadByte(*q))
						{
							p++;
							q++;
							*p = *q;
						}
#endif
						p++;
						q++;
					}
					nLen ++;
				}
				*p = NULL;
				lRes = rkParent.SetMultiStringValue(szValueName, pszDestValue);
			}
			else
			{
				lRes = ERROR_OUTOFMEMORY;
			}
		}
		break;
	case VT_UI4:
		{
			ULONG ulVal;
			USES_CONVERSION_EX;

			LPOLESTR lpszV = T2OLE_EX(szValue, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
	#ifndef _UNICODE
			if(lpszV == NULL) 
				return E_OUTOFMEMORY;
	#endif	
			VarUI4FromStr(lpszV, 0, 0, &ulVal);
			
			lRes = rkParent.SetDWORDValue(szValueName, ulVal);
			ATLTRACE(atlTraceRegistrar, 2, _T("Setting Value %d at %s\n"), ulVal, !szValueName ? _T("default") : szValueName);
			break;
		}
	case VT_UI1:
		{
			int cbValue = lstrlen(szValue);
			if (cbValue & 0x00000001)
			{
				ATLTRACE(atlTraceRegistrar, 0, _T("Binary Data does not fall on BYTE boundries\n"));
				return E_FAIL;
			}
			int cbValDiv2 = cbValue/2;
			CTempBuffer<BYTE, 256> rgBinary;
			ATLTRY(rgBinary.Allocate(cbValDiv2));			
			if (rgBinary == NULL)
				return E_FAIL;
			memset(rgBinary, 0, cbValDiv2);				
			for (int irg = 0; irg < cbValue; irg++)
				rgBinary[(irg/2)] |= (ChToByte(szValue[irg])) << (4*(1 - (irg & 0x00000001)));
			lRes = RegSetValueEx(rkParent, szValueName, 0, REG_BINARY, rgBinary, cbValDiv2);
			break;
		}
	}

	if (ERROR_SUCCESS != lRes)
	{
		nIDRes = E_ATL_VALUE_SET_FAILED;
		return AtlHresultFromWin32(lRes);
	}

	if (FAILED(hr = NextToken(szToken)))
		return hr;

	return S_OK;
}

inline BOOL CRegParser::CanForceRemoveKey(LPCTSTR szKey)
{
	for (int iNoDel = 0; iNoDel < cbNeverDelete; iNoDel++)
		if (!lstrcmpi(szKey, rgszNeverDelete[iNoDel]))
			 return FALSE;                       // We cannot delete it

	return TRUE;
}

inline BOOL CRegParser::HasSubKeys(HKEY hkey)
{
	DWORD       cbSubKeys = 0;

	if (RegQueryInfoKey(hkey, NULL, NULL, NULL,
							   &cbSubKeys, NULL, NULL,
							   NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
	{
		ATLTRACE(atlTraceRegistrar, 0, _T("Should not be here!!\n"));
		ATLASSERT(FALSE);
		return FALSE;
	}

	return cbSubKeys > 0;
}

inline BOOL CRegParser::HasValues(HKEY hkey)
{
	DWORD       cbValues = 0;

	LONG lResult = RegQueryInfoKey(hkey, NULL, NULL, NULL,
								  NULL, NULL, NULL,
								  &cbValues, NULL, NULL, NULL, NULL);
	if (ERROR_SUCCESS != lResult)
	{
		ATLTRACE(atlTraceRegistrar, 0, _T("RegQueryInfoKey Failed "));
		ATLASSERT(FALSE);
		return FALSE;
	}

	if (1 == cbValues)
	{
		DWORD cbMaxName= MAX_VALUE;
		TCHAR szValueName[MAX_VALUE];
		// Check to see if the Value is default or named
		lResult = RegEnumValue(hkey, 0, szValueName, &cbMaxName, NULL, NULL, NULL, NULL);
		if (ERROR_SUCCESS == lResult && (szValueName[0] != NULL))
			return TRUE; // Named Value means we have a value
		return FALSE;
	}

	return cbValues > 0; // More than 1 means we have a non-default value
}

inline HRESULT CRegParser::SkipAssignment(LPTSTR szToken)
{
	HRESULT hr;
	TCHAR szValue[MAX_VALUE];

	if (*szToken == chEquals)
	{
		if (FAILED(hr = NextToken(szToken)))
			return hr;
		// Skip assignment
		SkipWhiteSpace();
		if (FAILED(hr = NextToken(szValue)))
			return hr;
		if (FAILED(hr = NextToken(szToken)))
			return hr;
	}

	return S_OK;
}

inline HRESULT CRegParser::PreProcessBuffer(LPTSTR lpszReg, LPTSTR* ppszReg)
{
	ATLASSERT(lpszReg != NULL);
	ATLASSERT(ppszReg != NULL);

	if (lpszReg == NULL || ppszReg == NULL)
		return E_POINTER;
	
	*ppszReg = NULL;
	int nSize = lstrlen(lpszReg)*2;
	CParseBuffer pb(nSize);
	if (pb.p == NULL)
		return E_OUTOFMEMORY;
	m_pchCur = lpszReg;
	HRESULT hr = S_OK;

	while (*m_pchCur != NULL) // look for end
	{
		if (*m_pchCur == _T('%'))
		{
			m_pchCur = CharNext(m_pchCur);
			if (*m_pchCur == _T('%'))
			{
				if (!pb.AddChar(m_pchCur))
				{
					hr = E_OUTOFMEMORY;
					break;
				}
			}
			else
			{
				LPTSTR lpszNext = StrChr(m_pchCur, _T('%'));
				if (lpszNext == NULL)
				{
					ATLTRACE(atlTraceRegistrar, 0, _T("Error no closing % found\n"));
					hr = GenerateError(E_ATL_UNEXPECTED_EOS);
					break;
				}
				if ((lpszNext-m_pchCur) > 31)
				{
					hr = E_FAIL;
					break;
				}
				int nLength = int(lpszNext - m_pchCur);
				TCHAR buf[32];
				lstrcpyn(buf, m_pchCur, nLength+1);
				LPCOLESTR lpszVar = m_pRegObj->StrFromMap(buf);
				if (lpszVar == NULL)
				{
					hr = GenerateError(E_ATL_NOT_IN_MAP);
					break;
				}
				if (!pb.AddString(lpszVar))
				{
					hr = E_OUTOFMEMORY;
					break;
				}

				while (m_pchCur != lpszNext)
					m_pchCur = CharNext(m_pchCur);
			}
		}
		else
		{
			if (!pb.AddChar(m_pchCur))
			{
				hr = E_OUTOFMEMORY;
				break;
			}
		}

		m_pchCur = CharNext(m_pchCur);
	}
	if (SUCCEEDED(hr))
		*ppszReg = pb.Detach();
	return hr;
}

inline HRESULT CRegParser::RegisterBuffer(LPTSTR szBuffer, BOOL bRegister)
{
	TCHAR   szToken[MAX_VALUE];
	HRESULT hr = S_OK;

	LPTSTR szReg = NULL;
	hr = PreProcessBuffer(szBuffer, &szReg);
	if (FAILED(hr))
		return hr;

	ATLTRACE(atlTraceRegistrar, 0, szReg);
	ATLTRACE(atlTraceRegistrar, 0, _T("\n"));

	m_pchCur = szReg;

	// Preprocess szReg

	while (NULL != *m_pchCur)
	{
		if (FAILED(hr = NextToken(szToken)))
			break;
		HKEY hkBase;
		if ((hkBase = HKeyFromString(szToken)) == NULL)
		{
			ATLTRACE(atlTraceRegistrar, 0, _T("HKeyFromString failed on %s\n"), szToken);
			hr = GenerateError(E_ATL_BAD_HKEY);
			break;
		}

		if (FAILED(hr = NextToken(szToken)))
			break;

		if (chLeftBracket != *szToken)
		{
			ATLTRACE(atlTraceRegistrar, 0, _T("Syntax error, expecting a {, found a %s\n"), szToken);
			hr = GenerateError(E_ATL_MISSING_OPENKEY_TOKEN);
			break;
		}
		if (bRegister)
		{
			LPTSTR szRegAtRegister = m_pchCur;
			hr = RegisterSubkeys(szToken, hkBase, bRegister);
			if (FAILED(hr))
			{
				ATLTRACE(atlTraceRegistrar, 0, _T("Failed to register, cleaning up!\n"));
				m_pchCur = szRegAtRegister;
				RegisterSubkeys(szToken, hkBase, FALSE);
				break;
			}
		}
		else
		{
			if (FAILED(hr = RegisterSubkeys(szToken, hkBase, bRegister)))
				break;
		}

		SkipWhiteSpace();
	}
	CoTaskMemFree(szReg);
	return hr;
}

inline HRESULT CRegParser::RegisterSubkeys(LPTSTR szToken, HKEY hkParent, BOOL bRegister, BOOL bRecover)
{
	CRegKey keyCur;
	LONG    lRes;
	TCHAR  szKey[_MAX_PATH];
	BOOL    bDelete = TRUE;
	BOOL    bInRecovery = bRecover;
	HRESULT hr = S_OK;

	ATLTRACE(atlTraceRegistrar, 2, _T("Num Els = %d\n"), cbNeverDelete);
	if (FAILED(hr = NextToken(szToken)))
		return hr;


	while (*szToken != chRightBracket) // Continue till we see a }
	{
		bDelete = TRUE;
		BOOL bTokenDelete = !lstrcmpi(szToken, szDelete);

		if (!lstrcmpi(szToken, szForceRemove) || bTokenDelete)
		{
			if (FAILED(hr = NextToken(szToken)))
				break;

			if (bRegister)
			{
				CRegKey rkForceRemove;

				if (StrChr(szToken, chDirSep) != NULL)
					return GenerateError(E_ATL_COMPOUND_KEY);

				if (CanForceRemoveKey(szToken))
				{
					rkForceRemove.Attach(hkParent);
					// Error not returned. We will overwrite the values any way.
					rkForceRemove.RecurseDeleteKey(szToken);
					rkForceRemove.Detach();
				}
				if (bTokenDelete)
				{
					if (FAILED(hr = NextToken(szToken)))
						break;
					if (FAILED(hr = SkipAssignment(szToken)))
						break;
					goto EndCheck;
				}
			}

		}

		if (!lstrcmpi(szToken, szNoRemove))
		{
			bDelete = FALSE;    // set even for register
			if (FAILED(hr = NextToken(szToken)))
				break;
		}

		if (!lstrcmpi(szToken, szValToken)) // need to add a value to hkParent
		{
			TCHAR  szValueName[_MAX_PATH];

			if (FAILED(hr = NextToken(szValueName)))
				break;
			if (FAILED(hr = NextToken(szToken)))
				break;


			if (*szToken != chEquals)
				return GenerateError(E_ATL_EXPECTING_EQUAL);

			if (bRegister)
			{
				CRegKey rk;

				rk.Attach(hkParent);
				hr = AddValue(rk, szValueName, szToken);
				rk.Detach();

				if (FAILED(hr))
					return hr;

				goto EndCheck;
			}
			else
			{
				if (!bRecover && bDelete)
				{
					ATLTRACE(atlTraceRegistrar, 1, _T("Deleting %s\n"), szValueName);
					// We have to open the key for write to be able to delete.
					CRegKey rkParent;
					lRes = rkParent.Open(hkParent, NULL, KEY_WRITE);
					if (lRes == ERROR_SUCCESS)
					{
						lRes = rkParent.DeleteValue(szValueName);
						if (lRes != ERROR_SUCCESS && lRes != ERROR_FILE_NOT_FOUND)
						{
							// Key not present is not an error
							hr = AtlHresultFromWin32(lRes);
							break;
						}
					}
					else
					{
						hr = AtlHresultFromWin32(lRes);
						break;
					}
				}
				if (FAILED(hr = SkipAssignment(szToken)))
					break;
				continue;  // can never have a subkey
			}
		}

		if (StrChr(szToken, chDirSep) != NULL)
			return GenerateError(E_ATL_COMPOUND_KEY);

		if (bRegister)
		{
			lRes = keyCur.Open(hkParent, szToken, KEY_READ | KEY_WRITE);
			if (ERROR_SUCCESS != lRes)
			{
				// Failed all access try read only
				lRes = keyCur.Open(hkParent, szToken, KEY_READ);
				if (ERROR_SUCCESS != lRes)
				{
					// Finally try creating it
					ATLTRACE(atlTraceRegistrar, 2, _T("Creating key %s\n"), szToken);
					lRes = keyCur.Create(hkParent, szToken, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE);
					if (lRes != ERROR_SUCCESS)
						return AtlHresultFromWin32(lRes);
				}
			}

			if (FAILED(hr = NextToken(szToken)))
				break;


			if (*szToken == chEquals)
			{
				if (FAILED(hr = AddValue(keyCur, NULL, szToken))) // NULL == default
					break;
			}
		}
		else
		{
			if (!bRecover)
				lRes = keyCur.Open(hkParent, szToken, KEY_READ);
			else
				lRes = ERROR_FILE_NOT_FOUND;


			// Open failed set recovery mode
			if (lRes != ERROR_SUCCESS)
				bRecover = true;

			// TRACE out Key open status and if in recovery mode
#ifdef _DEBUG
			if (!bRecover)
				ATLTRACE(atlTraceRegistrar, 1, _T("Opened Key %s\n"), szToken);
			else
				ATLTRACE(atlTraceRegistrar, 0, _T("Ignoring Open key on %s : In Recovery mode\n"), szToken);
#endif //_DEBUG

			// Remember Subkey
			lstrcpyn(szKey, szToken, _MAX_PATH);

			if (FAILED(hr = NextToken(szToken)))
				break;
			if (FAILED(hr = SkipAssignment(szToken)))
				break;

			if (*szToken == chLeftBracket)
			{
				hr = RegisterSubkeys(szToken, keyCur.m_hKey, bRegister, bRecover);
				// In recover mode ignore error
				if (FAILED(hr) && !bRecover)
					break;
				// Skip the }
				if (FAILED(hr = NextToken(szToken)))
					break;
			}

#ifdef _DEBUG
			if (bRecover != bInRecovery)
				ATLTRACE(atlTraceRegistrar, 0, _T("Ending Recovery Mode\n"));
#endif
			bRecover = bInRecovery;

			if (lRes == ERROR_FILE_NOT_FOUND)
				// Key already not present so not an error.
				continue;

			if (lRes != ERROR_SUCCESS)
			{
				// We are recovery mode continue on errors else break
				if (bRecover)
					continue;
				else
				{
					hr = AtlHresultFromWin32(lRes);
					break;
				}
			}

			// If in recovery mode
			if (bRecover && HasSubKeys(keyCur))
			{
				// See if the KEY is in the NeverDelete list and if so, don't
				if (CanForceRemoveKey(szKey) && bDelete)
				{
					ATLTRACE(atlTraceRegistrar, 0, _T("Deleting non-empty subkey %s by force\n"), szKey);
					// Error not returned since we are in recovery mode. The error that caused recovery mode is returned
					keyCur.RecurseDeleteKey(szKey);
				}
				continue;
			}

			lRes = keyCur.Close();
			if (lRes != ERROR_SUCCESS)
			   return AtlHresultFromWin32(lRes);

			if (bDelete)
			{
				ATLTRACE(atlTraceRegistrar, 0, _T("Deleting Key %s\n"), szKey);
				CRegKey rkParent;
				rkParent.Attach(hkParent);
				lRes = rkParent.DeleteSubKey(szKey);
				rkParent.Detach();
				if (lRes != ERROR_SUCCESS)
				{
					hr = AtlHresultFromWin32(lRes);
					break;
				}
			}
		}

EndCheck:

		if (bRegister)
		{
			if (*szToken == chLeftBracket && lstrlen(szToken) == 1)
			{
				if (FAILED(hr = RegisterSubkeys(szToken, keyCur.m_hKey, bRegister, FALSE)))
					break;
				if (FAILED(hr = NextToken(szToken)))
					break;
			}
		}
	}

	return hr;
}

}; //namespace ATL

#endif //__STATREG_H__
