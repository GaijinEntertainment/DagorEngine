// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.

#ifndef __ATLCOMCLI_H__
#define __ATLCOMCLI_H__

#pragma once

#include <atlcore.h>
#include <ole2.h>
#include <olectl.h>

#pragma warning (push)
#pragma warning (disable: 4127)  // conditional expression constant
#pragma warning (disable: 4571)  //catch(...) blocks compiled with /EHs do NOT catch or re-throw Structured Exceptions


#pragma pack(push,_ATL_PACKING)
namespace ATL
{
/////////////////////////////////////////////////////////////////////////////
// Error to HRESULT helpers

ATL_NOINLINE inline HRESULT AtlHresultFromLastError() throw()
{
	DWORD dwErr = ::GetLastError();
	return HRESULT_FROM_WIN32(dwErr);
}

ATL_NOINLINE inline HRESULT AtlHresultFromWin32(__in DWORD nError) throw()
{
	return( HRESULT_FROM_WIN32( nError ) );
}

/////////////////////////////////////////////////////////////////////////////
// Smart Pointer helpers

ATLAPI_(IUnknown*) AtlComPtrAssign(IUnknown** pp, IUnknown* lp);
ATLAPI_(IUnknown*) AtlComQIPtrAssign(IUnknown** pp, IUnknown* lp, REFIID riid);

#ifndef _ATL_DLL

ATLINLINE ATLAPI_(IUnknown*) AtlComPtrAssign(__deref_out_opt IUnknown** pp, __in_opt IUnknown* lp)
{
	if (pp == NULL)
		return NULL;
		
	if (lp != NULL)
		lp->AddRef();
	if (*pp)
		(*pp)->Release();
	*pp = lp;
	return lp;
}

ATLINLINE ATLAPI_(IUnknown*) AtlComQIPtrAssign(__deref_out_opt IUnknown** pp, __in_opt IUnknown* lp, REFIID riid)
{
	if (pp == NULL)
		return NULL;

	IUnknown* pTemp = *pp;
	*pp = NULL;
	if (lp != NULL)
		lp->QueryInterface(riid, (void**)pp);
	if (pTemp)
		pTemp->Release();
	return *pp;
}

#endif // _ATL_DLL

/////////////////////////////////////////////////////////////////////////////
// COM Smart pointers

template <class T>
class _NoAddRefReleaseOnCComPtr : public T
{
	private:
		STDMETHOD_(ULONG, AddRef)()=0;
		STDMETHOD_(ULONG, Release)()=0;
};

inline __checkReturn HRESULT AtlSetChildSite(__inout IUnknown* punkChild, __in_opt IUnknown* punkParent)
{
	if (punkChild == NULL)
		return E_POINTER;

	HRESULT hr;
	IObjectWithSite* pChildSite = NULL;
	hr = punkChild->QueryInterface(__uuidof(IObjectWithSite), (void**)&pChildSite);
	if (SUCCEEDED(hr) && pChildSite != NULL)
	{
		hr = pChildSite->SetSite(punkParent);
		pChildSite->Release();
	}
	return hr;
}


//CComPtrBase provides the basis for all other smart pointers
//The other smartpointers add their own constructors and operators
template <class T>
class CComPtrBase
{
protected:
	CComPtrBase() throw()
	{
		p = NULL;
	}
	CComPtrBase(__in int nNull) throw()
	{
		ATLASSERT(nNull == 0);
		(void)nNull;
		p = NULL;
	}
	CComPtrBase(__in T* lp) throw()
	{
		p = lp;
		if (p != NULL)
			p->AddRef();
	}
public:
	typedef T _PtrClass;
	~CComPtrBase() throw()
	{
		if (p)
			p->Release();
	}
	operator T*() const throw()
	{
		return p;
	}
	T& operator*() const
	{
		ATLENSURE(p!=NULL);
		return *p;
	}
	//The assert on operator& usually indicates a bug.  If this is really
	//what is needed, however, take the address of the p member explicitly.
	T** operator&() throw()
	{
		ATLASSERT(p==NULL);
		return &p;
	}
	_NoAddRefReleaseOnCComPtr<T>* operator->() const throw()
	{
		ATLASSERT(p!=NULL);
		return (_NoAddRefReleaseOnCComPtr<T>*)p;
	}
	bool operator!() const throw()
	{
		return (p == NULL);
	}
	bool operator<(__in_opt T* pT) const throw()
	{
		return p < pT;
	}
	bool operator!=(__in_opt T* pT) const
	{
		return !operator==(pT);
	}
	bool operator==(__in_opt T* pT) const throw()
	{
		return p == pT;
	}

	// Release the interface and set to NULL
	void Release() throw()
	{
		T* pTemp = p;
		if (pTemp)
		{
			p = NULL;
			pTemp->Release();
		}
	}
	// Compare two objects for equivalence
	bool IsEqualObject(__in_opt IUnknown* pOther) throw()
	{
		if (p == NULL && pOther == NULL)
			return true;	// They are both NULL objects

		if (p == NULL || pOther == NULL)
			return false;	// One is NULL the other is not

		CComPtr<IUnknown> punk1;
		CComPtr<IUnknown> punk2;
		p->QueryInterface(__uuidof(IUnknown), (void**)&punk1);
		pOther->QueryInterface(__uuidof(IUnknown), (void**)&punk2);
		return punk1 == punk2;
	}
	// Attach to an existing interface (does not AddRef)
	void Attach(__in T* p2) throw()
	{
		if (p)
			p->Release();
		p = p2;
	}
	// Detach the interface (does not Release)
	T* Detach() throw()
	{
		T* pt = p;
		p = NULL;
		return pt;
	}
	__checkReturn HRESULT CopyTo(__deref_out_opt T** ppT) throw()
	{
		ATLASSERT(ppT != NULL);
		if (ppT == NULL)
			return E_POINTER;
		*ppT = p;
		if (p)
			p->AddRef();
		return S_OK;
	}
	__checkReturn HRESULT SetSite(__in_opt IUnknown* punkParent) throw()
	{
		return AtlSetChildSite(p, punkParent);
	}
	__checkReturn HRESULT Advise(__in IUnknown* pUnk, __in const IID& iid, __out LPDWORD pdw) throw()
	{
		return AtlAdvise(p, pUnk, iid, pdw);
	}
	__checkReturn HRESULT CoCreateInstance(__in REFCLSID rclsid, __in_opt LPUNKNOWN pUnkOuter = NULL, __in DWORD dwClsContext = CLSCTX_ALL) throw()
	{
		ATLASSERT(p == NULL);
		return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&p);
	}
	__checkReturn HRESULT CoCreateInstance(__in LPCOLESTR szProgID, __in_opt LPUNKNOWN pUnkOuter = NULL, __in DWORD dwClsContext = CLSCTX_ALL) throw()
	{
		CLSID clsid;
		HRESULT hr = CLSIDFromProgID(szProgID, &clsid);
		ATLASSERT(p == NULL);
		if (SUCCEEDED(hr))
			hr = ::CoCreateInstance(clsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&p);
		return hr;
	}
	template <class Q>
	__checkReturn HRESULT QueryInterface(__deref_out_opt Q** pp) const throw()
	{
		ATLASSERT(pp != NULL);
		return p->QueryInterface(__uuidof(Q), (void**)pp);
	}
	T* p;
};

template <class T>
class CComPtr : public CComPtrBase<T>
{
public:
	CComPtr() throw()
	{
	}
	CComPtr(int nNull) throw() :
		CComPtrBase<T>(nNull)
	{
	}
	CComPtr(T* lp) throw() :
		CComPtrBase<T>(lp)

	{
	}
	CComPtr(__in const CComPtr<T>& lp) throw() :
		CComPtrBase<T>(lp.p)
	{
	}
	T* operator=(__in_opt T* lp) throw()
	{
        if(*this!=lp)
        {
    		return static_cast<T*>(AtlComPtrAssign((IUnknown**)&p, lp));
        }
        return *this;
	}
	template <typename Q>
	T* operator=(__in const CComPtr<Q>& lp) throw()
	{
        if( !IsEqualObject(lp) )
        {
    		return static_cast<T*>(AtlComQIPtrAssign((IUnknown**)&p, lp, __uuidof(T)));
        }
        return *this;
	}
	T* operator=(__in const CComPtr<T>& lp) throw()
	{
        if(*this!=lp)
        {
    		return static_cast<T*>(AtlComPtrAssign((IUnknown**)&p, lp));
        }
        return *this;
	}
};

//specialization for IDispatch
template <>
class CComPtr<IDispatch> : public CComPtrBase<IDispatch>
{
public:
	CComPtr() throw()
	{
	}
	CComPtr(IDispatch* lp) throw() :
		CComPtrBase<IDispatch>(lp)
	{
	}
	CComPtr(const CComPtr<IDispatch>& lp) throw() :
		CComPtrBase<IDispatch>(lp.p)
	{
	}
	IDispatch* operator=(IDispatch* lp) throw()
	{
        if(*this!=lp)
        {
            return static_cast<IDispatch*>(AtlComPtrAssign((IUnknown**)&p, lp));
        }
        return *this;
	}
	IDispatch* operator=(const CComPtr<IDispatch>& lp) throw()
	{
        if(*this!=lp)
        {
    		return static_cast<IDispatch*>(AtlComPtrAssign((IUnknown**)&p, lp.p));
        }
        return *this;
	}

// IDispatch specific stuff
	__checkReturn HRESULT GetPropertyByName(__in LPCOLESTR lpsz, __out VARIANT* pVar) throw()
	{
		ATLASSERT(p);
		ATLASSERT(pVar);
		DISPID dwDispID;
		HRESULT hr = GetIDOfName(lpsz, &dwDispID);
		if (SUCCEEDED(hr))
			hr = GetProperty(dwDispID, pVar);
		return hr;
	}
	__checkReturn HRESULT GetProperty(__in DISPID dwDispID, __out VARIANT* pVar) throw()
	{
		return GetProperty(p, dwDispID, pVar);
	}
	__checkReturn HRESULT PutPropertyByName(__in LPCOLESTR lpsz, __in VARIANT* pVar) throw()
	{
		ATLASSERT(p);
		ATLASSERT(pVar);
		DISPID dwDispID;
		HRESULT hr = GetIDOfName(lpsz, &dwDispID);
		if (SUCCEEDED(hr))
			hr = PutProperty(dwDispID, pVar);
		return hr;
	}
	__checkReturn HRESULT PutProperty(__in DISPID dwDispID, __in VARIANT* pVar) throw()
	{
		return PutProperty(p, dwDispID, pVar);
	}
	__checkReturn HRESULT GetIDOfName(__in LPCOLESTR lpsz, __out DISPID* pdispid) throw()
	{
		return p->GetIDsOfNames(IID_NULL, const_cast<LPOLESTR*>(&lpsz), 1, LOCALE_USER_DEFAULT, pdispid);
	}
	// Invoke a method by DISPID with no parameters
	__checkReturn HRESULT Invoke0(__in DISPID dispid, __out_opt VARIANT* pvarRet = NULL) throw()
	{
		DISPPARAMS dispparams = { NULL, NULL, 0, 0};
		return p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, NULL, NULL);
	}
	// Invoke a method by name with no parameters
	__checkReturn HRESULT Invoke0(__in LPCOLESTR lpszName, __out_opt VARIANT* pvarRet = NULL) throw()
	{
		HRESULT hr;
		DISPID dispid;
		hr = GetIDOfName(lpszName, &dispid);
		if (SUCCEEDED(hr))
			hr = Invoke0(dispid, pvarRet);
		return hr;
	}
	// Invoke a method by DISPID with a single parameter
	__checkReturn HRESULT Invoke1(__in DISPID dispid, VARIANT* pvarParam1, __out_opt VARIANT* pvarRet = NULL) throw()
	{
		DISPPARAMS dispparams = { pvarParam1, NULL, 1, 0};
		return p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, NULL, NULL);
	}
	// Invoke a method by name with a single parameter
	__checkReturn HRESULT Invoke1(__in LPCOLESTR lpszName, VARIANT* pvarParam1, __out_opt VARIANT* pvarRet = NULL) throw()
	{
		HRESULT hr;
		DISPID dispid;
		hr = GetIDOfName(lpszName, &dispid);
		if (SUCCEEDED(hr))
			hr = Invoke1(dispid, pvarParam1, pvarRet);
		return hr;
	}
	// Invoke a method by DISPID with two parameters
	__checkReturn HRESULT Invoke2(__in DISPID dispid, __in VARIANT* pvarParam1, __in VARIANT* pvarParam2, __out_opt VARIANT* pvarRet = NULL) throw();
	// Invoke a method by name with two parameters
	__checkReturn HRESULT Invoke2(__in LPCOLESTR lpszName, __in VARIANT* pvarParam1, __in VARIANT* pvarParam2, __out_opt VARIANT* pvarRet = NULL) throw()
	{
		HRESULT hr;
		DISPID dispid;
		hr = GetIDOfName(lpszName, &dispid);
		if (SUCCEEDED(hr))
			hr = Invoke2(dispid, pvarParam1, pvarParam2, pvarRet);
		return hr;
	}
	// Invoke a method by DISPID with N parameters
	__checkReturn HRESULT InvokeN(DISPID dispid, VARIANT* pvarParams, int nParams, VARIANT* pvarRet = NULL) throw()
	{
		DISPPARAMS dispparams = { pvarParams, NULL, nParams, 0};
		return p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, NULL, NULL);
	}
	// Invoke a method by name with Nparameters
	__checkReturn HRESULT InvokeN(LPCOLESTR lpszName, VARIANT* pvarParams, int nParams, VARIANT* pvarRet = NULL) throw()
	{
		HRESULT hr;
		DISPID dispid;
		hr = GetIDOfName(lpszName, &dispid);
		if (SUCCEEDED(hr))
			hr = InvokeN(dispid, pvarParams, nParams, pvarRet);
		return hr;
	}
	static __checkReturn HRESULT PutProperty(__inout IDispatch* p, __in DISPID dwDispID, __in VARIANT* pVar) throw()
	{
		ATLASSERT(p);
		ATLASSERT(pVar != NULL);
		if (pVar == NULL)
			return E_POINTER;
		
		if(p == NULL)
			return E_INVALIDARG;
		
		ATLTRACE(atlTraceCOM, 2, _T("CPropertyHelper::PutProperty\n"));
		DISPPARAMS dispparams = {NULL, NULL, 1, 1};
		dispparams.rgvarg = pVar;
		DISPID dispidPut = DISPID_PROPERTYPUT;
		dispparams.rgdispidNamedArgs = &dispidPut;

		if (pVar->vt == VT_UNKNOWN || pVar->vt == VT_DISPATCH || 
			(pVar->vt & VT_ARRAY) || (pVar->vt & VT_BYREF))
		{
			HRESULT hr = p->Invoke(dwDispID, IID_NULL,
				LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUTREF,
				&dispparams, NULL, NULL, NULL);
			if (SUCCEEDED(hr))
				return hr;
		}
		return p->Invoke(dwDispID, IID_NULL,
				LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
				&dispparams, NULL, NULL, NULL);
	}
	static __checkReturn HRESULT GetProperty(__in IDispatch* p, __in DISPID dwDispID, __out VARIANT* pVar) throw()
	{
		ATLASSERT(p);
		ATLASSERT(pVar != NULL);
		if (pVar == NULL)
			return E_POINTER;
		
		if(p == NULL)
			return E_INVALIDARG;
			
		ATLTRACE(atlTraceCOM, 2, _T("CPropertyHelper::GetProperty\n"));
		DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
		return p->Invoke(dwDispID, IID_NULL,
				LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
				&dispparamsNoArgs, pVar, NULL, NULL);
	}
};

template <class T, const IID* piid = &__uuidof(T)>
class CComQIPtr : public CComPtr<T>
{
public:
	CComQIPtr() throw()
	{
	}
	CComQIPtr(__in T* lp) throw() :
		CComPtr<T>(lp)
	{
	}
	CComQIPtr(__in const CComQIPtr<T,piid>& lp) throw() :
		CComPtr<T>(lp.p)
	{
	}
	CComQIPtr(__in_opt IUnknown* lp) throw()
	{
		if (lp != NULL)
			lp->QueryInterface(*piid, (void **)&p);
	}
	T* operator=(__in T* lp) throw()
	{
        if(*this!=lp)
        {
		    return static_cast<T*>(AtlComPtrAssign((IUnknown**)&p, lp));
        }
        return *this;
	}
	T* operator=(__in const CComQIPtr<T,piid>& lp) throw()
	{
        if(*this!=lp)
        {
    		return static_cast<T*>(AtlComPtrAssign((IUnknown**)&p, lp.p));
        }
        return *this;
	}
	T* operator=(__in IUnknown* lp) throw()
	{
        if(*this!=lp)
        {
    		return static_cast<T*>(AtlComQIPtrAssign((IUnknown**)&p, lp, *piid));
        }
        return *this;
	}
};

//Specialization to make it work
template<>
class CComQIPtr<IUnknown, &IID_IUnknown> : public CComPtr<IUnknown>
{
public:
	CComQIPtr() throw()
	{
	}
	CComQIPtr(__in IUnknown* lp) throw()
	{
		//Actually do a QI to get identity
		if (lp != NULL)
			lp->QueryInterface(__uuidof(IUnknown), (void **)&p);
	}
	CComQIPtr(__in const CComQIPtr<IUnknown,&IID_IUnknown>& lp) throw() :
		CComPtr<IUnknown>(lp.p)
	{
	}
	IUnknown* operator=(__in IUnknown* lp) throw()
	{
        if(*this!=lp)
        {
		    //Actually do a QI to get identity
		    return AtlComQIPtrAssign((IUnknown**)&p, lp, __uuidof(IUnknown));
        }
        return *this;
	}

	IUnknown* operator=(__in const CComQIPtr<IUnknown,&IID_IUnknown>& lp) throw()
	{
        if(*this!=lp)
        {
    		return AtlComPtrAssign((IUnknown**)&p, lp.p);
        }
        return *this;
	}
};

typedef CComQIPtr<IDispatch, &__uuidof(IDispatch)> CComDispatchDriver;

#define com_cast ATL::CComQIPtr

/////////////////////////////////////////////////////////////////////////////
// CComBSTR

class CComBSTR
{
public:
	BSTR m_str;

	CComBSTR() throw()
	{
		m_str = NULL;
	}

	CComBSTR(__in int nSize)
	{
		if (nSize == 0)
			m_str = NULL;
		else
		{
			m_str = ::SysAllocStringLen(NULL, nSize);
			if (!*this)
            {
				AtlThrow(E_OUTOFMEMORY);
            }
		}
	}

	CComBSTR(__in int nSize, __in_ecount_opt(nSize) LPCOLESTR sz)
	{
		if (nSize == 0)
        {
			m_str = NULL;
        }
		else
		{
			m_str = ::SysAllocStringLen(sz, nSize);
			if (!*this)
            {
				AtlThrow(E_OUTOFMEMORY);
            }
		}
	}

	CComBSTR(__in_opt LPCOLESTR pSrc)
	{
		if (pSrc == NULL)
        {
			m_str = NULL;
        }
		else
		{
			m_str = ::SysAllocString(pSrc);
			if (!*this)
            {
				AtlThrow(E_OUTOFMEMORY);
            }
		}
	}

	CComBSTR(__in const CComBSTR& src)
	{
		m_str = src.Copy();
		if (!!src && !*this)
        {
			AtlThrow(E_OUTOFMEMORY);
        }
	}

	CComBSTR(__in REFGUID guid)
	{
		OLECHAR szGUID[64];
		::StringFromGUID2(guid, szGUID, 64);
		m_str = ::SysAllocString(szGUID);
		if (!*this)
        {
			AtlThrow(E_OUTOFMEMORY);
        }
	}

	CComBSTR& operator=(__in const CComBSTR& src)
	{
		if (m_str != src.m_str)
		{
			::SysFreeString(m_str);
			m_str = src.Copy();
			if (!!src && !*this)
            {
				AtlThrow(E_OUTOFMEMORY);
            }
		}
		return *this;
	}

	CComBSTR& operator=(__in_opt LPCOLESTR pSrc)
	{
		if (pSrc != m_str)
		{
			::SysFreeString(m_str);
			if (pSrc != NULL)
			{
				m_str = ::SysAllocString(pSrc);
				if (!*this)
                {
					AtlThrow(E_OUTOFMEMORY);
                }
			}
			else
            {
				m_str = NULL;
            }
		}
		return *this;
	}

	~CComBSTR() throw();

	unsigned int Length() const throw()
	{
        return ::SysStringLen(m_str);
	}

	unsigned int ByteLength() const throw()
	{
        return ::SysStringByteLen(m_str);
	}

	operator BSTR() const throw()
	{
		return m_str;
	}


#ifndef ATL_CCOMBSTR_ADDRESS_OF_ASSERT
// Temp disable CComBSTR::operator& Assert
#define ATL_NO_CCOMBSTR_ADDRESS_OF_ASSERT
#endif


	BSTR* operator&() throw()
	{
#ifndef ATL_NO_CCOMBSTR_ADDRESS_OF_ASSERT
#pragma warning(push)
#pragma warning(disable:4068)
#pragma prefast(push)
#pragma prefast(disable:325, "We are deliberately checking if this has already been allocated")
		ATLASSERT(!*this);
#pragma prefast(pop)
#pragma warning(pop)
#endif
		return &m_str;
	}

	BSTR Copy() const throw()
	{
		if (!*this)
        {
			return NULL;
        }
		return ::SysAllocStringByteLen((char*)m_str, ::SysStringByteLen(m_str));
	}

	__checkReturn HRESULT CopyTo(__inout_opt BSTR* pbstr) throw()
	{
		ATLASSERT(pbstr != NULL);
		if (pbstr == NULL)
        {
			return E_POINTER;
        }
		*pbstr = Copy();
#pragma warning(push)
#pragma warning(disable:4068)
#pragma prefast(push)
#pragma prefast(disable:325, "We are checking allocation semantics here")
		if ((*pbstr == NULL) && (m_str != NULL))
        {
			return E_OUTOFMEMORY;
        }
#pragma prefast(pop)
#pragma warning(pop)
		return S_OK;
	}

	// copy BSTR to VARIANT
	__checkReturn HRESULT CopyTo(__out_opt VARIANT *pvarDest) throw()
	{
		ATLASSERT(pvarDest != NULL);
		HRESULT hRes = E_POINTER;
		if (pvarDest != NULL)
		{
			pvarDest->vt = VT_BSTR;
			pvarDest->bstrVal = Copy();
#pragma warning(push)
#pragma warning(disable:4068)
#pragma prefast(push)
#pragma prefast(disable:325, "We are checking allocation semantics here")
			if (pvarDest->bstrVal == NULL && m_str != NULL)
            {
				hRes = E_OUTOFMEMORY;
            }
#pragma prefast(pop)
#pragma warning(pop)
			else
            {
				hRes = S_OK;
            }
		}
		return hRes;
	}

	void Attach(__in BSTR src) throw()
	{
		if (m_str != src)
		{
			::SysFreeString(m_str);
			m_str = src;
		}
	}

	BSTR Detach() throw()
	{
		BSTR s = m_str;
		m_str = NULL;
		return s;
	}

	void Empty() throw()
	{
		::SysFreeString(m_str);
		m_str = NULL;
	}

	bool operator!() const throw()
	{
#pragma warning(push)
#pragma warning(disable:4068)
#pragma prefast(push)
#pragma prefast(disable:325, "The semantics of this function are about allocation, not content")
		return (m_str == NULL);
#pragma prefast(pop)
#pragma warning(pop)
	}

	__checkReturn HRESULT Append(__in const CComBSTR& bstrSrc) throw()
	{
		return AppendBSTR(bstrSrc.m_str);
	}

	__checkReturn HRESULT Append(__in_opt LPCOLESTR lpsz) throw()
	{
		return Append(lpsz, UINT(ocslen(lpsz)));
	}

	// a BSTR is just a LPCOLESTR so we need a special version to signify
	// that we are appending a BSTR
	__checkReturn HRESULT AppendBSTR(__in BSTR p) throw()
	{
        if (::SysStringLen(p) == 0)
        {
			return S_OK;
        }
		BSTR bstrNew = NULL;
		HRESULT hr;
		hr = VarBstrCat(m_str, p, &bstrNew);
		if (SUCCEEDED(hr))
		{
			::SysFreeString(m_str);
			m_str = bstrNew;
		}
		return hr;
	}

	__checkReturn HRESULT Append(__in_ecount_opt(nLen) LPCOLESTR lpsz, __in int nLen) throw()
	{
#pragma warning(push)
#pragma warning(disable:4068)
#pragma prefast(push)
#pragma prefast(disable:325, "The semantics of this function are about allocation, not content")
		if (lpsz == NULL || (m_str != NULL && nLen == 0))
#pragma prefast(pop)
#pragma warning(pop)
			return S_OK;
		int n1 = Length();
		if (n1+nLen < n1)
			return E_OUTOFMEMORY;
		BSTR b;
		b = ::SysAllocStringLen(NULL, n1+nLen);
#pragma warning(push)
#pragma warning(disable:4068)
#pragma prefast(push)
#pragma prefast(disable:325, "The semantics of this function are about allocation, not content")
		if (b == NULL)
#pragma prefast(pop)
#pragma warning(pop)
			return E_OUTOFMEMORY;
		if(::SysStringLen(m_str) > 0)
		{
			Checked::memcpy_s(b, (n1+nLen)*sizeof(OLECHAR), m_str, n1*sizeof(OLECHAR));
		}
		Checked::memcpy_s(b+n1, nLen*sizeof(OLECHAR), lpsz, nLen*sizeof(OLECHAR));
		b[n1+nLen] = NULL;
		SysFreeString(m_str);
		m_str = b;
		return S_OK;
	}

	__checkReturn HRESULT Append(__in char ch) throw()
	{
		OLECHAR chO = ch;

		return( Append( &chO, 1 ) );
	}

	__checkReturn HRESULT Append(__in wchar_t ch) throw()
	{
		return( Append( &ch, 1 ) );
	}

	__checkReturn HRESULT AppendBytes(__in_ecount_opt(nLen) const char* lpsz, __in int nLen) throw()
	{
		if (lpsz == NULL || nLen == 0)
			return S_OK;
		int n1 = ByteLength();
		if (n1+nLen < n1)
			return E_OUTOFMEMORY;
		BSTR b;
		b = ::SysAllocStringByteLen(NULL, n1+nLen);
		if (b == NULL)
        {
			return E_OUTOFMEMORY;
        }
		Checked::memcpy_s(b, n1+nLen, m_str, n1);
		Checked::memcpy_s(((char*)b)+n1, nLen, lpsz, nLen);
		*((OLECHAR*)(((char*)b)+n1+nLen)) = NULL;
		SysFreeString(m_str);
		m_str = b;
		return S_OK;
	}

	__checkReturn HRESULT AssignBSTR(const BSTR bstrSrc) throw()
	{
		HRESULT hr = S_OK;
		if (m_str != bstrSrc)
		{
			::SysFreeString(m_str);
#pragma warning(push)
#pragma warning(disable:4068)
#pragma prefast(push)
#pragma prefast(disable:325, "The semantics of this function are about allocation, not content")
			if (bstrSrc != NULL)
#pragma prefast(pop)
#pragma warning(pop)
			{
				m_str = ::SysAllocStringByteLen((char*)bstrSrc, ::SysStringByteLen(bstrSrc));
				if (!*this)
                {
					hr = E_OUTOFMEMORY;
                }
			}
			else
            {
				m_str = NULL;
            }
		}

		return hr;
	}

	__checkReturn HRESULT ToLower() throw()
	{
		if (::SysStringLen(m_str) > 0)
		{
#ifdef _UNICODE
			// Convert in place
			CharLowerBuff(m_str, Length());
#else
			// Cannot use conversion macros due to possible embedded NULLs
			UINT _acp = _AtlGetConversionACP();
			int _convert = WideCharToMultiByte(_acp, 0, m_str, Length(), NULL, 0, NULL, NULL);
			CTempBuffer<char> pszA;
			ATLTRY(pszA.Allocate(_convert));
			if (pszA == NULL)
				return E_OUTOFMEMORY;

			int nRet = WideCharToMultiByte(_acp, 0, m_str, Length(), pszA, _convert, NULL, NULL);
			if (nRet == 0)
			{
				ATLASSERT(0);
				return AtlHresultFromLastError();
			}

			CharLowerBuff(pszA, nRet);

			_convert = MultiByteToWideChar(_acp, 0, pszA, nRet, NULL, 0);

			CTempBuffer<WCHAR> pszW;
			ATLTRY(pszW.Allocate(_convert));
			if (pszW == NULL)
				return E_OUTOFMEMORY;

			nRet = MultiByteToWideChar(_acp, 0, pszA, nRet, pszW, _convert);
			if (nRet <= 0)
			{
				ATLASSERT(0);
				return AtlHresultFromLastError();
			}

			UINT nBytes=0;	
			HRESULT hr=S_OK;
			if( FAILED(hr=::ATL::AtlMultiply(&nBytes, static_cast<UINT>(nRet), static_cast<UINT>(sizeof(OLECHAR)))))
			{
				return hr;
			}
			BSTR b = ::SysAllocStringByteLen((LPCSTR) (LPWSTR) pszW, nBytes);
			if (b == NULL)
				return E_OUTOFMEMORY;
			SysFreeString(m_str);
			m_str = b;
#endif
		}
		return S_OK;
	}
	__checkReturn HRESULT ToUpper() throw()
	{
		if (::SysStringLen(m_str) > 0)
		{
#ifdef _UNICODE
			// Convert in place
			CharUpperBuff(m_str, Length());
#else
			// Cannot use conversion macros due to possible embedded NULLs
			UINT _acp = _AtlGetConversionACP();
			int _convert = WideCharToMultiByte(_acp, 0, m_str, Length(), NULL, 0, NULL, NULL);
			CTempBuffer<char> pszA;
			ATLTRY(pszA.Allocate(_convert));
			if (pszA == NULL)
				return E_OUTOFMEMORY;

			int nRet = WideCharToMultiByte(_acp, 0, m_str, Length(), pszA, _convert, NULL, NULL);
			if (nRet == 0)
			{
				ATLASSERT(0);
				return AtlHresultFromLastError();
			}

			CharUpperBuff(pszA, nRet);

			_convert = MultiByteToWideChar(_acp, 0, pszA, nRet, NULL, 0);

			CTempBuffer<WCHAR> pszW;
			ATLTRY(pszW.Allocate(_convert));
			if (pszW == NULL)
				return E_OUTOFMEMORY;

			nRet = MultiByteToWideChar(_acp, 0, pszA, nRet, pszW, _convert);
			if (nRet <= 0)
			{
				ATLASSERT(0);
				return AtlHresultFromLastError();
			}

			UINT nBytes=0;
			HRESULT hr=S_OK;
			if( FAILED(hr=::ATL::AtlMultiply(&nBytes, static_cast<UINT>(nRet), static_cast<UINT>(sizeof(OLECHAR)))))
			{		
				return hr;
			}
			BSTR b = ::SysAllocStringByteLen((LPCSTR) (LPWSTR) pszW, nBytes);
			if (b == NULL)
				return E_OUTOFMEMORY;
			SysFreeString(m_str);
			m_str = b;
#endif
		}
		return S_OK;
	}

	bool LoadString(__in HINSTANCE hInst, __in UINT nID) throw()
	{
		::SysFreeString(m_str);
		m_str = NULL;
		return LoadStringResource(hInst, nID, m_str);
	}

	bool LoadString(__in UINT nID) throw()
	{
		::SysFreeString(m_str);
		m_str = NULL;
		return LoadStringResource(nID, m_str);
	}

	CComBSTR& operator+=(__in const CComBSTR& bstrSrc)
	{
		HRESULT hr;
		hr = AppendBSTR(bstrSrc.m_str);
		if (FAILED(hr))
			AtlThrow(hr);
		return *this;
	}

	CComBSTR& operator+=(__in_opt LPCOLESTR pszSrc)
	{
		HRESULT hr;
		hr = Append(pszSrc);
		if (FAILED(hr))
			AtlThrow(hr);
		return *this;
	}

	bool operator<(const CComBSTR& bstrSrc) const throw()
	{
		return VarBstrCmp(m_str, bstrSrc.m_str, LOCALE_USER_DEFAULT, 0) == static_cast<HRESULT>(VARCMP_LT);
	}
	bool operator<(LPCOLESTR pszSrc) const
	{
		CComBSTR bstr2(pszSrc);
		return operator<(bstr2);
	}
	bool operator<(LPOLESTR pszSrc) const
	{
		return operator<((LPCOLESTR)pszSrc);
	}

	bool operator>(const CComBSTR& bstrSrc) const throw()
	{
		return VarBstrCmp(m_str, bstrSrc.m_str, LOCALE_USER_DEFAULT, 0) == static_cast<HRESULT>(VARCMP_GT);
	}
	bool operator>(LPCOLESTR pszSrc) const
	{
		CComBSTR bstr2(pszSrc);
		return operator>(bstr2);
	}
	bool operator>(LPOLESTR pszSrc) const
	{
		return operator>((LPCOLESTR)pszSrc);
	}
	
	bool operator!=(const CComBSTR& bstrSrc) const throw()
	{
		return !operator==(bstrSrc);
	}
	bool operator!=(LPCOLESTR pszSrc) const
	{
		return !operator==(pszSrc);
	}
	bool operator!=(int nNull) const throw()
	{
		return !operator==(nNull);
	}
	bool operator!=(LPOLESTR pszSrc) const
	{
		return operator!=((LPCOLESTR)pszSrc);
	}

	bool operator==(const CComBSTR& bstrSrc) const throw()
	{
		return VarBstrCmp(m_str, bstrSrc.m_str, LOCALE_USER_DEFAULT, 0) == static_cast<HRESULT>(VARCMP_EQ);
	}
	bool operator==(LPCOLESTR pszSrc) const
	{
		CComBSTR bstr2(pszSrc);
		return operator==(bstr2);
	}
	bool operator==(LPOLESTR pszSrc) const
	{
		return operator==((LPCOLESTR)pszSrc);
	}
	
	bool operator==(int nNull) const throw()
	{
		ATLASSERT(nNull == NULL);
		(void)nNull;
		return (!*this);
	}

	CComBSTR(__in LPCSTR pSrc)
	{
		if (pSrc != NULL)
		{
			m_str = A2WBSTR(pSrc);
			if (!*this)
            {
				AtlThrow(E_OUTOFMEMORY);
            }
		}
		else
        {
			m_str = NULL;
        }
	}

	CComBSTR(__in int nSize, __in_ecount_opt(nSize) LPCSTR sz)
	{
		if (nSize != 0 && sz == NULL)
		{
			m_str = ::SysAllocStringLen(NULL, nSize);
			if (!*this)
            {
				AtlThrow(E_OUTOFMEMORY);
            }
			return;
		}

		m_str = A2WBSTR(sz, nSize);
		if (!*this && nSize != 0)
        {
			AtlThrow(E_OUTOFMEMORY);
        }
	}

	__checkReturn HRESULT Append(__in LPCSTR lpsz) throw()
	{
		if (lpsz == NULL)
			return S_OK;

		CComBSTR bstrTemp;
		ATLTRY(bstrTemp = lpsz);
		if (!bstrTemp)
        {
			return E_OUTOFMEMORY;
        }
		return Append(bstrTemp);
	}

	CComBSTR& operator=(__in LPCSTR pSrc)
	{
		::SysFreeString(m_str);
		m_str = A2WBSTR(pSrc);
		if (!*this && pSrc != NULL)
        {
			AtlThrow(E_OUTOFMEMORY);
        }
		return *this;
	}

	bool operator<(__in LPCSTR pszSrc) const
	{
		CComBSTR bstr2(pszSrc);
		return operator<(bstr2);
	}
	bool operator>(__in LPCSTR pszSrc) const
	{
		CComBSTR bstr2(pszSrc);
		return operator>(bstr2);
	}
	bool operator!=(__in LPCSTR pszSrc) const
	{
		return !operator==(pszSrc);
	}
	bool operator==(__in LPCSTR pszSrc) const
	{
		CComBSTR bstr2(pszSrc);
		return operator==(bstr2);
	}

	__checkReturn HRESULT WriteToStream(__inout IStream* pStream) throw()
	{
		ATLASSERT(pStream != NULL);
		if(pStream == NULL)
			return E_INVALIDARG;
			
		ULONG cb;
		ULONG cbStrLen = CComBSTR::GetStreamSize(m_str) - sizeof(ULONG);
		HRESULT hr = pStream->Write((void*) &cbStrLen, sizeof(cbStrLen), &cb);
		if (FAILED(hr))
			return hr;
		return cbStrLen ? pStream->Write((void*) m_str, cbStrLen, &cb) : S_OK;
	}

	__checkReturn HRESULT ReadFromStream(__inout IStream* pStream) throw()
	{
		ATLASSERT(pStream != NULL);
		if(pStream == NULL)
			return E_INVALIDARG;
			
		ATLASSERT(!*this); // should be empty
		Empty();
		
		ULONG cbStrLen = 0;
		HRESULT hr = pStream->Read((void*) &cbStrLen, sizeof(cbStrLen), NULL);
		if ((hr == S_OK) && (cbStrLen != 0))
		{
			//subtract size for terminating NULL which we wrote out
			//since SysAllocStringByteLen overallocates for the NULL
			m_str = SysAllocStringByteLen(NULL, cbStrLen-sizeof(OLECHAR));
			if (!*this)
            {
				hr = E_OUTOFMEMORY;
            }
			else
            {
				hr = pStream->Read((void*) m_str, cbStrLen, NULL);
            }
			// If SysAllocStringByteLen or IStream::Read failed, reset seek 
			// pointer to start of BSTR size.
			if (hr != S_OK)
			{
				LARGE_INTEGER nOffset;
				nOffset.QuadPart = -(static_cast<LONGLONG>(sizeof(cbStrLen)));
				pStream->Seek(nOffset, STREAM_SEEK_CUR, NULL);
			}
		}
		if (hr == S_FALSE)
			hr = E_FAIL;
		return hr;
	}

	static bool LoadStringResource(__in HINSTANCE hInstance, __in UINT uID, __deref_out_opt BSTR& bstrText) throw()
	{
		const ATLSTRINGRESOURCEIMAGE* pImage;

#pragma warning(push)
#pragma warning(disable:4068)
#pragma prefast(push)
#pragma prefast(disable:325, "The semantics of this function are about allocation, not content")
		ATLASSERT(bstrText == NULL);
#pragma prefast(pop)
#pragma warning(pop)

		pImage = AtlGetStringResourceImage(hInstance, uID);
		if (pImage != NULL)
		{
			bstrText = ::SysAllocStringLen(pImage->achString, pImage->nLength);
		}
#pragma warning(push)
#pragma warning(disable:4068)
#pragma prefast(push)
#pragma prefast(disable:325, "The semantics of this function are about allocation, not content")
		return (bstrText != NULL) ? true : false;
#pragma prefast(pop)
#pragma warning(pop)
	}

	static bool LoadStringResource(__in UINT uID, __deref_out_opt BSTR& bstrText) throw()
	{
		const ATLSTRINGRESOURCEIMAGE* pImage;

#pragma warning(push)
#pragma warning(disable:4068)
#pragma prefast(push)
#pragma prefast(disable:325, "The semantics of this function are about allocation, not content")
		ATLASSERT(bstrText == NULL);
#pragma prefast(pop)
#pragma warning(pop)

		pImage = AtlGetStringResourceImage(uID);
		if (pImage != NULL)
		{
			bstrText = ::SysAllocStringLen(pImage->achString, pImage->nLength);
		}

#pragma warning(push)
#pragma warning(disable:4068)
#pragma prefast(push)
#pragma prefast(disable:325, "The semantics of this function are about allocation, not content")
		return (bstrText != NULL) ? true : false;
#pragma prefast(pop)
#pragma warning(pop)
	}

	// each character in BSTR is copied to each element in SAFEARRAY
	__success(SUCCEEDED(return)) HRESULT BSTRToArray(__deref_out LPSAFEARRAY *ppArray) throw()
	{
		return VectorFromBstr(m_str, ppArray);
	}

	// first character of each element in SAFEARRAY is copied to BSTR
	__checkReturn HRESULT ArrayToBSTR(__in const SAFEARRAY *pSrc) throw()
	{
		::SysFreeString(m_str);
		return BstrFromVector((LPSAFEARRAY)pSrc, &m_str);
	}
	static ULONG GetStreamSize(BSTR bstr)
	{
		ULONG ulSize=sizeof(ULONG);
#pragma warning(push)
#pragma warning(disable:4068)
#pragma prefast(push)
#pragma prefast(disable:325, "The semantics of this function are about allocation, not content")
		if (bstr != NULL)
#pragma prefast(pop)
#pragma warning(pop)
		{
			ulSize += SysStringByteLen(bstr) + sizeof(OLECHAR);
		}
		return ulSize;
	}
};

inline CComBSTR::~CComBSTR() throw()
	{
		::SysFreeString(m_str);
	}

inline void SysFreeStringHelper(__in CComBSTR& bstr)
{
	bstr.Empty();
}

inline void SysFreeStringHelper(BSTR bstr)
{
	::SysFreeString(bstr);	
}

inline __checkReturn HRESULT SysAllocStringHelper(__out_opt CComBSTR& bstrDest,BSTR bstrSrc)
{
	bstrDest=bstrSrc;
	return !bstrDest ? E_OUTOFMEMORY : S_OK;
}

inline __checkReturn HRESULT SysAllocStringHelper(__out_opt BSTR& bstrDest,BSTR bstrSrc)
{
	bstrDest=::SysAllocString(bstrSrc);
#pragma warning(push)
#pragma warning(disable:4068)
#pragma prefast(push)
#pragma prefast(disable:325, "The semantics of this function are about allocation, not content")
	return bstrDest==NULL ? E_OUTOFMEMORY : S_OK;
#pragma prefast(pop)
#pragma warning(pop)
}

/////////////////////////////////////////////////////////////
// Class to Adapt CComBSTR and CComPtr for use with STL containers
// the syntax to use it is
// std::vector< CAdapt <CComBSTR> > vect;

template <class T>
class CAdapt
{
public:
	CAdapt()
	{
	}
	CAdapt(__in const T& rSrc) :
		m_T( rSrc )
	{
	}

	CAdapt(__in const CAdapt& rSrCA) :
		m_T( rSrCA.m_T )
	{
	}

	CAdapt& operator=(__in const T& rSrc)
	{
		m_T = rSrc;
		return *this;
	}
	bool operator<(__in const T& rSrc) const
	{
		return m_T < rSrc;
	}
	bool operator==(__in const T& rSrc) const
	{
		return m_T == rSrc;
	}
	operator T&()
	{
		return m_T;
	}

	operator const T&() const
	{
		return m_T;
	}

	T m_T;
};

/////////////////////////////////////////////////////////////////////////////
// CComVariant


#define ATL_VARIANT_TRUE VARIANT_BOOL( -1 )
#define ATL_VARIANT_FALSE VARIANT_BOOL( 0 )

template< typename T > 
class CVarTypeInfo
{
//	static const VARTYPE VT;  // VARTYPE corresponding to type T
//	static T VARIANT::* const pmField;  // Pointer-to-member of corresponding field in VARIANT struct
};

template<>
class CVarTypeInfo< char >
{
public:
	static const VARTYPE VT = VT_I1;
	static char VARIANT::* const pmField;
};

__declspec( selectany ) char VARIANT::* const CVarTypeInfo< char >::pmField = &VARIANT::cVal;

template<>
class CVarTypeInfo< unsigned char >
{
public:
	static const VARTYPE VT = VT_UI1;
	static unsigned char VARIANT::* const pmField;
};

__declspec( selectany ) unsigned char VARIANT::* const CVarTypeInfo< unsigned char >::pmField = &VARIANT::bVal;

template<>
class CVarTypeInfo< char* >
{
public:
	static const VARTYPE VT = VT_I1|VT_BYREF;
	static char* VARIANT::* const pmField;
};

__declspec( selectany ) char* VARIANT::* const CVarTypeInfo< char* >::pmField = &VARIANT::pcVal;

template<>
class CVarTypeInfo< unsigned char* >
{
public:
	static const VARTYPE VT = VT_UI1|VT_BYREF;
	static unsigned char* VARIANT::* const pmField;
};

__declspec( selectany ) unsigned char* VARIANT::* const CVarTypeInfo< unsigned char* >::pmField = &VARIANT::pbVal;

template<>
class CVarTypeInfo< short >
{
public:
	static const VARTYPE VT = VT_I2;
	static short VARIANT::* const pmField;
};

__declspec( selectany ) short VARIANT::* const CVarTypeInfo< short >::pmField = &VARIANT::iVal;

template<>
class CVarTypeInfo< short* >
{
public:
	static const VARTYPE VT = VT_I2|VT_BYREF;
	static short* VARIANT::* const pmField;
};

__declspec( selectany ) short* VARIANT::* const CVarTypeInfo< short* >::pmField = &VARIANT::piVal;

template<>
class CVarTypeInfo< unsigned short >
{
public:
	static const VARTYPE VT = VT_UI2;
	static unsigned short VARIANT::* const pmField;
};

__declspec( selectany ) unsigned short VARIANT::* const CVarTypeInfo< unsigned short >::pmField = &VARIANT::uiVal;

#ifdef _NATIVE_WCHAR_T_DEFINED  // Only treat unsigned short* as VT_UI2|VT_BYREF if BSTR isn't the same as unsigned short*
template<>
class CVarTypeInfo< unsigned short* >
{
public:
	static const VARTYPE VT = VT_UI2|VT_BYREF;
	static unsigned short* VARIANT::* const pmField;
};

__declspec( selectany ) unsigned short* VARIANT::* const CVarTypeInfo< unsigned short* >::pmField = &VARIANT::puiVal;
#endif  // _NATIVE_WCHAR_T_DEFINED

template<>
class CVarTypeInfo< int >
{
public:
	static const VARTYPE VT = VT_I4;
	static int VARIANT::* const pmField;
};

__declspec( selectany ) int VARIANT::* const CVarTypeInfo< int >::pmField = &VARIANT::intVal;

template<>
class CVarTypeInfo< int* >
{
public:
	static const VARTYPE VT = VT_I4|VT_BYREF;
	static int* VARIANT::* const pmField;
};

__declspec( selectany ) int* VARIANT::* const CVarTypeInfo< int* >::pmField = &VARIANT::pintVal;

template<>
class CVarTypeInfo< unsigned int >
{
public:
	static const VARTYPE VT = VT_UI4;
	static unsigned int VARIANT::* const pmField;
};

__declspec( selectany ) unsigned int VARIANT::* const CVarTypeInfo< unsigned int >::pmField = &VARIANT::uintVal;

template<>
class CVarTypeInfo< unsigned int* >
{
public:
	static const VARTYPE VT = VT_UI4|VT_BYREF;
	static unsigned int* VARIANT::* const pmField;
};

__declspec( selectany ) unsigned int* VARIANT::* const CVarTypeInfo< unsigned int* >::pmField = &VARIANT::puintVal;

template<>
class CVarTypeInfo< long >
{
public:
	static const VARTYPE VT = VT_I4;
	static long VARIANT::* const pmField;
};

__declspec( selectany ) long VARIANT::* const CVarTypeInfo< long >::pmField = &VARIANT::lVal;

template<>
class CVarTypeInfo< long* >
{
public:
	static const VARTYPE VT = VT_I4|VT_BYREF;
	static long* VARIANT::* const pmField;
};

__declspec( selectany ) long* VARIANT::* const CVarTypeInfo< long* >::pmField = &VARIANT::plVal;

template<>
class CVarTypeInfo< unsigned long >
{
public:
	static const VARTYPE VT = VT_UI4;
	static unsigned long VARIANT::* const pmField;
};

__declspec( selectany ) unsigned long VARIANT::* const CVarTypeInfo< unsigned long >::pmField = &VARIANT::ulVal;

template<>
class CVarTypeInfo< unsigned long* >
{
public:
	static const VARTYPE VT = VT_UI4|VT_BYREF;
	static unsigned long* VARIANT::* const pmField;
};

__declspec( selectany ) unsigned long* VARIANT::* const CVarTypeInfo< unsigned long* >::pmField = &VARIANT::pulVal;

template<>
class CVarTypeInfo< __int64 >
{
public:
	static const VARTYPE VT = VT_I8;
	static __int64 VARIANT::* const pmField;
};

__declspec( selectany ) __int64 VARIANT::* const CVarTypeInfo< __int64 >::pmField = &VARIANT::llVal;

template<>
class CVarTypeInfo< __int64* >
{
public:
	static const VARTYPE VT = VT_I8|VT_BYREF;
	static __int64* VARIANT::* const pmField;
};

__declspec( selectany ) __int64* VARIANT::* const CVarTypeInfo< __int64* >::pmField = &VARIANT::pllVal;

template<>
class CVarTypeInfo< unsigned __int64 >
{
public:
	static const VARTYPE VT = VT_UI8;
	static unsigned __int64 VARIANT::* const pmField;
};

__declspec( selectany ) unsigned __int64 VARIANT::* const CVarTypeInfo< unsigned __int64 >::pmField = &VARIANT::ullVal;

template<>
class CVarTypeInfo< unsigned __int64* >
{
public:
	static const VARTYPE VT = VT_UI8|VT_BYREF;
	static unsigned __int64* VARIANT::* const pmField;
};

__declspec( selectany ) unsigned __int64* VARIANT::* const CVarTypeInfo< unsigned __int64* >::pmField = &VARIANT::pullVal;

template<>
class CVarTypeInfo< float >
{
public:
	static const VARTYPE VT = VT_R4;
	static float VARIANT::* const pmField;
};

__declspec( selectany ) float VARIANT::* const CVarTypeInfo< float >::pmField = &VARIANT::fltVal;

template<>
class CVarTypeInfo< float* >
{
public:
	static const VARTYPE VT = VT_R4|VT_BYREF;
	static float* VARIANT::* const pmField;
};

__declspec( selectany ) float* VARIANT::* const CVarTypeInfo< float* >::pmField = &VARIANT::pfltVal;

template<>
class CVarTypeInfo< double >
{
public:
	static const VARTYPE VT = VT_R8;
	static double VARIANT::* const pmField;
};

__declspec( selectany ) double VARIANT::* const CVarTypeInfo< double >::pmField = &VARIANT::dblVal;

template<>
class CVarTypeInfo< double* >
{
public:
	static const VARTYPE VT = VT_R8|VT_BYREF;
	static double* VARIANT::* const pmField;
};

__declspec( selectany ) double* VARIANT::* const CVarTypeInfo< double* >::pmField = &VARIANT::pdblVal;

template<>
class CVarTypeInfo< VARIANT >
{
public:
	static const VARTYPE VT = VT_VARIANT;
};

template<>
class CVarTypeInfo< BSTR >
{
public:
	static const VARTYPE VT = VT_BSTR;
	static BSTR VARIANT::* const pmField;
};

__declspec( selectany ) BSTR VARIANT::* const CVarTypeInfo< BSTR >::pmField = &VARIANT::bstrVal;

template<>
class CVarTypeInfo< BSTR* >
{
public:
	static const VARTYPE VT = VT_BSTR|VT_BYREF;
	static BSTR* VARIANT::* const pmField;
};

__declspec( selectany ) BSTR* VARIANT::* const CVarTypeInfo< BSTR* >::pmField = &VARIANT::pbstrVal;

template<>
class CVarTypeInfo< IUnknown* >
{
public:
	static const VARTYPE VT = VT_UNKNOWN;
	static IUnknown* VARIANT::* const pmField;
};

__declspec( selectany ) IUnknown* VARIANT::* const CVarTypeInfo< IUnknown* >::pmField = &VARIANT::punkVal;

template<>
class CVarTypeInfo< IUnknown** >
{
public:
	static const VARTYPE VT = VT_UNKNOWN|VT_BYREF;
	static IUnknown** VARIANT::* const pmField;
};

__declspec( selectany ) IUnknown** VARIANT::* const CVarTypeInfo< IUnknown** >::pmField = &VARIANT::ppunkVal;

template<>
class CVarTypeInfo< IDispatch* >
{
public:
	static const VARTYPE VT = VT_DISPATCH;
	static IDispatch* VARIANT::* const pmField;
};

__declspec( selectany ) IDispatch* VARIANT::* const CVarTypeInfo< IDispatch* >::pmField = &VARIANT::pdispVal;

template<>
class CVarTypeInfo< IDispatch** >
{
public:
	static const VARTYPE VT = VT_DISPATCH|VT_BYREF;
	static IDispatch** VARIANT::* const pmField;
};

__declspec( selectany ) IDispatch** VARIANT::* const CVarTypeInfo< IDispatch** >::pmField = &VARIANT::ppdispVal;

template<>
class CVarTypeInfo< CY >
{
public:
	static const VARTYPE VT = VT_CY;
	static CY VARIANT::* const pmField;
};

__declspec( selectany ) CY VARIANT::* const CVarTypeInfo< CY >::pmField = &VARIANT::cyVal;

template<>
class CVarTypeInfo< CY* >
{
public:
	static const VARTYPE VT = VT_CY|VT_BYREF;
	static CY* VARIANT::* const pmField;
};

__declspec( selectany ) CY* VARIANT::* const CVarTypeInfo< CY* >::pmField = &VARIANT::pcyVal;

class CComVariant : public tagVARIANT
{
// Constructors
public:
	CComVariant() throw()
	{
		::VariantInit(this);
	}
	~CComVariant() throw()
	{
		Clear();
	}

	CComVariant(__in const VARIANT& varSrc)
	{
		vt = VT_EMPTY;
		InternalCopy(&varSrc);
	}

	CComVariant(__in const CComVariant& varSrc)
	{
		vt = VT_EMPTY;
		InternalCopy(&varSrc);
	}
	CComVariant(__in LPCOLESTR lpszSrc)
	{
		vt = VT_EMPTY;
		*this = lpszSrc;
	}

	CComVariant(__in LPCSTR lpszSrc)
	{
		vt = VT_EMPTY;
		*this = lpszSrc;
	}

	CComVariant(__in bool bSrc)
	{
		vt = VT_BOOL;
		boolVal = bSrc ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE;
	}

	CComVariant(__in int nSrc, __in VARTYPE vtSrc = VT_I4) throw()
	{
		ATLASSERT(vtSrc == VT_I4 || vtSrc == VT_INT);
		vt = vtSrc;
		intVal = nSrc;
	}
	CComVariant(__in BYTE nSrc) throw()
	{
		vt = VT_UI1;
		bVal = nSrc;
	}
	CComVariant(__in short nSrc) throw()
	{
		vt = VT_I2;
		iVal = nSrc;
	}
	CComVariant(__in long nSrc, __in VARTYPE vtSrc = VT_I4) throw()
	{
		ATLASSERT(vtSrc == VT_I4 || vtSrc == VT_ERROR);
		vt = vtSrc;
		lVal = nSrc;
	}
	CComVariant(__in float fltSrc) throw()
	{
		vt = VT_R4;
		fltVal = fltSrc;
	}
	CComVariant(__in double dblSrc, __in VARTYPE vtSrc = VT_R8) throw()
	{
		ATLASSERT(vtSrc == VT_R8 || vtSrc == VT_DATE);
		vt = vtSrc;
		dblVal = dblSrc;
	}
#if (_WIN32_WINNT >= 0x0501) || defined(_ATL_SUPPORT_VT_I8)
	CComVariant(__in LONGLONG nSrc) throw()
	{
		vt = VT_I8;
		llVal = nSrc;
	}
	CComVariant(__in ULONGLONG nSrc) throw()
	{
		vt = VT_UI8;
		ullVal = nSrc;
	}
#endif
	CComVariant(CY __in cySrc) throw()
	{
		vt = VT_CY;
		cyVal.Hi = cySrc.Hi;
		cyVal.Lo = cySrc.Lo;
	}
	CComVariant(__in_opt IDispatch* pSrc) throw()
	{
		vt = VT_DISPATCH;
		pdispVal = pSrc;
		// Need to AddRef as VariantClear will Release
		if (pdispVal != NULL)
			pdispVal->AddRef();
	}
	CComVariant(__in_opt IUnknown* pSrc) throw()
	{
		vt = VT_UNKNOWN;
		punkVal = pSrc;
		// Need to AddRef as VariantClear will Release
		if (punkVal != NULL)
			punkVal->AddRef();
	}
	CComVariant(__in char cSrc) throw()
	{
		vt = VT_I1;
		cVal = cSrc;
	}
	CComVariant(__in unsigned short nSrc) throw()
	{
		vt = VT_UI2;
		uiVal = nSrc;
	}
	CComVariant(__in unsigned long nSrc) throw()
	{
		vt = VT_UI4;
		ulVal = nSrc;
	}
	CComVariant(__in unsigned int nSrc, __in VARTYPE vtSrc = VT_UI4) throw()
	{
		ATLASSERT(vtSrc == VT_UI4 || vtSrc == VT_UINT);
		vt = vtSrc;
		uintVal= nSrc;
	}
	CComVariant(__in const CComBSTR& bstrSrc)
	{
		vt = VT_EMPTY;
		*this = bstrSrc;
	}
	CComVariant(__in_opt const SAFEARRAY *pSrc)
	{
		LPSAFEARRAY pCopy;
		if (pSrc != NULL)
		{
			HRESULT hRes = ::SafeArrayCopy((LPSAFEARRAY)pSrc, &pCopy);
			if (SUCCEEDED(hRes) && pCopy != NULL)
			{
                ::ATL::AtlSafeArrayGetActualVartype((LPSAFEARRAY)pSrc, &vt);
				vt |= VT_ARRAY;
				parray = pCopy;
			}
			else
			{
				vt = VT_ERROR;
				scode = hRes;
#ifndef _ATL_NO_VARIANT_THROW
				ATLENSURE_THROW(FALSE, E_OUTOFMEMORY);
#endif
			}
		}
		else
		{
			vt = VT_EMPTY;
		}
	}
// Assignment Operators
public:
	CComVariant& operator=(__in const CComVariant& varSrc)
	{
        if(this!=&varSrc)
        {
		    InternalCopy(&varSrc);
        }
		return *this;
	}
	CComVariant& operator=(__in const VARIANT& varSrc)
	{
        if(static_cast<VARIANT *>(this)!=&varSrc)
        {
		    InternalCopy(&varSrc);
        }
		return *this;
	}

	CComVariant& operator=(__in const CComBSTR& bstrSrc)
	{
		Clear();
		vt = VT_BSTR;
		bstrVal = bstrSrc.Copy();
#pragma warning(push)
#pragma warning(disable:4068)
#pragma prefast(push)
#pragma prefast(disable:325, "We are checking allocation semantics here")
		if (bstrVal == NULL && bstrSrc.m_str != NULL)
		{
			vt = VT_ERROR;
			scode = E_OUTOFMEMORY;
#ifndef _ATL_NO_VARIANT_THROW
			ATLENSURE_THROW(FALSE, E_OUTOFMEMORY);
#endif
		}
#pragma prefast(pop)
#pragma warning(pop)
		return *this;
	}

	CComVariant& operator=(__in LPCOLESTR lpszSrc)
	{
		Clear();
		vt = VT_BSTR;
		bstrVal = ::SysAllocString(lpszSrc);

		if (bstrVal == NULL && lpszSrc != NULL)
		{
			vt = VT_ERROR;
			scode = E_OUTOFMEMORY;
#ifndef _ATL_NO_VARIANT_THROW
			ATLENSURE_THROW(FALSE, E_OUTOFMEMORY);
#endif

		}
		return *this;
	}

	CComVariant& operator=(__in LPCSTR lpszSrc)
	{
		USES_CONVERSION_EX;
		Clear();
		vt = VT_BSTR;
		bstrVal = ::SysAllocString(A2COLE_EX(lpszSrc, _ATL_SAFE_ALLOCA_DEF_THRESHOLD));

		if (bstrVal == NULL && lpszSrc != NULL)
		{
			vt = VT_ERROR;
			scode = E_OUTOFMEMORY;
#ifndef _ATL_NO_VARIANT_THROW
			ATLENSURE_THROW(FALSE, E_OUTOFMEMORY);
#endif
		}
		return *this;
	}

	CComVariant& operator=(__in bool bSrc)
	{
		if (vt != VT_BOOL)
		{
			Clear();
			vt = VT_BOOL;
		}
		boolVal = bSrc ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE;
		return *this;
	}

	CComVariant& operator=(__in int nSrc) throw()
	{
		if (vt != VT_I4)
		{
			Clear();
			vt = VT_I4;
		}
		intVal = nSrc;

		return *this;
	}

	CComVariant& operator=(__in BYTE nSrc) throw()
	{
		if (vt != VT_UI1)
		{
			Clear();
			vt = VT_UI1;
		}
		bVal = nSrc;
		return *this;
	}

	CComVariant& operator=(__in short nSrc) throw()
	{
		if (vt != VT_I2)
		{
			Clear();
			vt = VT_I2;
		}
		iVal = nSrc;
		return *this;
	}

	CComVariant& operator=(__in long nSrc) throw()
	{
		if (vt != VT_I4)
		{
			Clear();
			vt = VT_I4;
		}
		lVal = nSrc;
		return *this;
	}

	CComVariant& operator=(__in float fltSrc) throw()
	{
		if (vt != VT_R4)
		{
			Clear();
			vt = VT_R4;
		}
		fltVal = fltSrc;
		return *this;
	}

	CComVariant& operator=(__in double dblSrc) throw()
	{
		if (vt != VT_R8)
		{
			Clear();
			vt = VT_R8;
		}
		dblVal = dblSrc;
		return *this;
	}

	CComVariant& operator=(__in CY cySrc) throw()
	{
		if (vt != VT_CY)
		{
			Clear();
			vt = VT_CY;
		}
		cyVal.Hi = cySrc.Hi;
		cyVal.Lo = cySrc.Lo;
		return *this;
	}

	CComVariant& operator=(__in_opt IDispatch* pSrc) throw()
	{
		Clear();
		vt = VT_DISPATCH;
		pdispVal = pSrc;
		// Need to AddRef as VariantClear will Release
		if (pdispVal != NULL)
			pdispVal->AddRef();
		return *this;
	}

	CComVariant& operator=(__in_opt IUnknown* pSrc) throw()
	{
		Clear();
		vt = VT_UNKNOWN;
		punkVal = pSrc;

		// Need to AddRef as VariantClear will Release
		if (punkVal != NULL)
			punkVal->AddRef();
		return *this;
	}

	CComVariant& operator=(__in char cSrc) throw()
	{
		if (vt != VT_I1)
		{
			Clear();
			vt = VT_I1;
		}
		cVal = cSrc;
		return *this;
	}

	CComVariant& operator=(__in unsigned short nSrc) throw()
	{
		if (vt != VT_UI2)
		{
			Clear();
			vt = VT_UI2;
		}
		uiVal = nSrc;
		return *this;
	}

	CComVariant& operator=(__in unsigned long nSrc) throw()
	{
		if (vt != VT_UI4)
		{
			Clear();
			vt = VT_UI4;
		}
		ulVal = nSrc;
		return *this;
	}

	CComVariant& operator=(__in unsigned int nSrc) throw()
	{
		if (vt != VT_UI4)
		{
			Clear();
			vt = VT_UI4;
		}
		uintVal= nSrc;
		return *this;
	}

	CComVariant& operator=(__in_opt BYTE* pbSrc) throw()
	{
		if (vt != (VT_UI1|VT_BYREF))
		{
			Clear();
			vt = VT_UI1|VT_BYREF;
		}
		pbVal = pbSrc;
		return *this;
	}

	CComVariant& operator=(__in_opt short* pnSrc) throw()
	{
		if (vt != (VT_I2|VT_BYREF))
		{
			Clear();
			vt = VT_I2|VT_BYREF;
		}
		piVal = pnSrc;
		return *this;
	}

#ifdef _NATIVE_WCHAR_T_DEFINED
	CComVariant& operator=(__in_opt USHORT* pnSrc) throw()
	{
		if (vt != (VT_UI2|VT_BYREF))
		{
			Clear();
			vt = VT_UI2|VT_BYREF;
		}
		puiVal = pnSrc;
		return *this;
	}
#endif

	CComVariant& operator=(__in_opt int* pnSrc) throw()
	{
		if (vt != (VT_I4|VT_BYREF))
		{
			Clear();
			vt = VT_I4|VT_BYREF;
		}
		pintVal = pnSrc;
		return *this;
	}

	CComVariant& operator=(__in_opt UINT* pnSrc) throw()
	{
		if (vt != (VT_UI4|VT_BYREF))
		{
			Clear();
			vt = VT_UI4|VT_BYREF;
		}
		puintVal = pnSrc;
		return *this;
	}

	CComVariant& operator=(__in_opt long* pnSrc) throw()
	{
		if (vt != (VT_I4|VT_BYREF))
		{
			Clear();
			vt = VT_I4|VT_BYREF;
		}
		plVal = pnSrc;
		return *this;
	}

	CComVariant& operator=(__in_opt ULONG* pnSrc) throw()
	{
		if (vt != (VT_UI4|VT_BYREF))
		{
			Clear();
			vt = VT_UI4|VT_BYREF;
		}
		pulVal = pnSrc;
		return *this;
	}

#if (_WIN32_WINNT >= 0x0501) || defined(_ATL_SUPPORT_VT_I8)
	CComVariant& operator=(__in LONGLONG nSrc) throw()
	{
		if (vt != VT_I8)
		{
			Clear();
			vt = VT_I8;
		}
		llVal = nSrc;

		return *this;
	}

	CComVariant& operator=(__in_opt LONGLONG* pnSrc) throw()
	{
		if (vt != (VT_I8|VT_BYREF))
		{
			Clear();
			vt = VT_I8|VT_BYREF;
		}
		pllVal = pnSrc;
		return *this;
	}

	CComVariant& operator=(__in ULONGLONG nSrc) throw()
	{
		if (vt != VT_UI8)
		{
			Clear();
			vt = VT_UI8;
		}
		ullVal = nSrc;

		return *this;
	}

	CComVariant& operator=(__in_opt ULONGLONG* pnSrc) throw()
	{
		if (vt != (VT_UI8|VT_BYREF))
		{
			Clear();
			vt = VT_UI8|VT_BYREF;
		}
		pullVal = pnSrc;
		return *this;
	}
#endif

	CComVariant& operator=(__in_opt float* pfSrc) throw()
	{
		if (vt != (VT_R4|VT_BYREF))
		{
			Clear();
			vt = VT_R4|VT_BYREF;
		}
		pfltVal = pfSrc;
		return *this;
	}

	CComVariant& operator=(__in_opt double* pfSrc) throw()
	{
		if (vt != (VT_R8|VT_BYREF))
		{
			Clear();
			vt = VT_R8|VT_BYREF;
		}
		pdblVal = pfSrc;
		return *this;
	}

	CComVariant& operator=(__in_opt const SAFEARRAY *pSrc)
	{
		Clear();
		LPSAFEARRAY pCopy;
		if (pSrc != NULL)
		{
			HRESULT hRes = ::SafeArrayCopy((LPSAFEARRAY)pSrc, &pCopy);
			if (SUCCEEDED(hRes) && pCopy != NULL)
			{
				::ATL::AtlSafeArrayGetActualVartype((LPSAFEARRAY)pSrc, &vt);
				vt |= VT_ARRAY;
				parray = pCopy;
			}
			else
			{
				vt = VT_ERROR;
				scode = hRes;
#ifndef _ATL_NO_VARIANT_THROW
			ATLENSURE_THROW(FALSE, E_OUTOFMEMORY);
#endif
			}
		}
		return *this;
	}

// Comparison Operators
public:
	bool operator==(__in const VARIANT& varSrc) const throw()
	{
		// For backwards compatibility
		if (vt == VT_NULL && varSrc.vt == VT_NULL)
		{
			return true;
		}
		// Variants not equal if types don't match
		if (vt != varSrc.vt)
		{
			return false;
		}
		return VarCmp((VARIANT*)this, (VARIANT*)&varSrc, LOCALE_USER_DEFAULT, 0) == static_cast<HRESULT>(VARCMP_EQ);
	}

	bool operator!=(__in const VARIANT& varSrc) const throw()
	{
		return !operator==(varSrc);
	}

	bool operator<(__in const VARIANT& varSrc) const throw()
	{
		if (vt == VT_NULL && varSrc.vt == VT_NULL)
			return false;
		return VarCmp((VARIANT*)this, (VARIANT*)&varSrc, LOCALE_USER_DEFAULT, 0)== static_cast<HRESULT>(VARCMP_LT);
	}

	bool operator>(__in const VARIANT& varSrc) const throw()
	{
		if (vt == VT_NULL && varSrc.vt == VT_NULL)
			return false;
		return VarCmp((VARIANT*)this, (VARIANT*)&varSrc, LOCALE_USER_DEFAULT, 0)== static_cast<HRESULT>(VARCMP_GT);
	}

// Operations
public:
	HRESULT Clear() { return ::VariantClear(this); }
	HRESULT Copy(__in const VARIANT* pSrc) { return ::VariantCopy(this, const_cast<VARIANT*>(pSrc)); }
	// copy VARIANT to BSTR
	HRESULT CopyTo(__out BSTR *pstrDest)
	{
		ATLASSERT(pstrDest != NULL && vt == VT_BSTR);
		HRESULT hRes = E_POINTER;
		if (pstrDest != NULL && vt == VT_BSTR)
		{
			*pstrDest = ::SysAllocStringByteLen((char*)bstrVal, ::SysStringByteLen(bstrVal));
			if (*pstrDest == NULL)
				hRes = E_OUTOFMEMORY;
			else
				hRes = S_OK;
		}
		else if (vt != VT_BSTR)
			hRes = DISP_E_TYPEMISMATCH;
		return hRes;
	}
	HRESULT Attach(__in VARIANT* pSrc)
	{
		if(pSrc == NULL)
			return E_INVALIDARG;
			
		// Clear out the variant
		HRESULT hr = Clear();
		if (!FAILED(hr))
		{
			// Copy the contents and give control to CComVariant
			Checked::memcpy_s(this, sizeof(CComVariant), pSrc, sizeof(VARIANT));
			pSrc->vt = VT_EMPTY;
			hr = S_OK;
		}
		return hr;
	}

	HRESULT Detach(__out VARIANT* pDest)
	{
		ATLASSERT(pDest != NULL);
		if(pDest == NULL)
			return E_POINTER;
			
		// Clear out the variant
		HRESULT hr = ::VariantClear(pDest);
		if (!FAILED(hr))
		{
			// Copy the contents and remove control from CComVariant
			Checked::memcpy_s(pDest, sizeof(VARIANT), this, sizeof(VARIANT));
			vt = VT_EMPTY;
			hr = S_OK;
		}
		return hr;
	}

	HRESULT ChangeType(__in VARTYPE vtNew, __in_opt const VARIANT* pSrc = NULL)
	{
		VARIANT* pVar = const_cast<VARIANT*>(pSrc);
		// Convert in place if pSrc is NULL
		if (pVar == NULL)
			pVar = this;
		// Do nothing if doing in place convert and vts not different
		return ::VariantChangeType(this, pVar, 0, vtNew);
	}

	template< typename T >
	void SetByRef( __in T* pT ) throw()
	{
		Clear();
		vt = CVarTypeInfo< T >::VT|VT_BYREF;
		byref = pT;
	}

	HRESULT WriteToStream(__inout IStream* pStream);
	HRESULT ReadFromStream(__inout IStream* pStream);

	// Return the size in bytes of the current contents
	ULONG GetSize() const;

// Implementation
public:
	HRESULT InternalClear()
	{
		HRESULT hr = Clear();
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
		{
			vt = VT_ERROR;
			scode = hr;
#ifndef _ATL_NO_VARIANT_THROW
			AtlThrow(hr);
#endif
		}
		return hr;
	}

	void InternalCopy(__in const VARIANT* pSrc)
	{
		HRESULT hr = Copy(pSrc);
		if (FAILED(hr))
		{
			vt = VT_ERROR;
			scode = hr;
#ifndef _ATL_NO_VARIANT_THROW
			AtlThrow(hr);
#endif
		}
	}
};

#pragma warning(push)
#pragma warning(disable: 4702)
inline HRESULT CComVariant::WriteToStream(__inout IStream* pStream)
{
	if(pStream == NULL)
		return E_INVALIDARG;
		
	HRESULT hr = pStream->Write(&vt, sizeof(VARTYPE), NULL);
	if (FAILED(hr))
		return hr;

	int cbWrite = 0;
	switch (vt)
	{
	case VT_UNKNOWN:
	case VT_DISPATCH:
		{
			CComPtr<IPersistStream> spStream;
			if (punkVal != NULL)
			{
				hr = punkVal->QueryInterface(__uuidof(IPersistStream), (void**)&spStream);
				if (FAILED(hr))
				{
					hr = punkVal->QueryInterface(__uuidof(IPersistStreamInit), (void**)&spStream);
					if (FAILED(hr))
						return hr;
				}
			}
			if (spStream != NULL)
				return OleSaveToStream(spStream, pStream);
			return WriteClassStm(pStream, CLSID_NULL);
		}
	case VT_UI1:
	case VT_I1:
		cbWrite = sizeof(BYTE);
		break;
	case VT_I2:
	case VT_UI2:
	case VT_BOOL:
		cbWrite = sizeof(short);
		break;
	case VT_I4:
	case VT_UI4:
	case VT_R4:
	case VT_INT:
	case VT_UINT:
	case VT_ERROR:
		cbWrite = sizeof(long);
		break;
	case VT_I8:
	case VT_UI8:
		cbWrite = sizeof(LONGLONG);
		break;
	case VT_R8:
	case VT_CY:
	case VT_DATE:
		cbWrite = sizeof(double);
		break;
	default:
		break;
	}
	if (cbWrite != 0)
		return pStream->Write((void*) &bVal, cbWrite, NULL);

	CComBSTR bstrWrite;
	CComVariant varBSTR;
	if (vt != VT_BSTR)
	{
		hr = VariantChangeType(&varBSTR, this, VARIANT_NOVALUEPROP, VT_BSTR);
		if (FAILED(hr))
			return hr;
		bstrWrite.Attach(varBSTR.bstrVal);
	}
	else
		bstrWrite.Attach(bstrVal);

	hr = bstrWrite.WriteToStream(pStream);
	bstrWrite.Detach();
	return hr;
}
#pragma warning(pop)	// C4702

inline HRESULT CComVariant::ReadFromStream(__inout IStream* pStream)
{
	ATLASSERT(pStream != NULL);
	if(pStream == NULL)
		return E_INVALIDARG;
		
	HRESULT hr;
	hr = VariantClear(this);
	if (FAILED(hr))
		return hr;
	VARTYPE vtRead = VT_EMPTY;
	ULONG cbRead = 0;
	hr = pStream->Read(&vtRead, sizeof(VARTYPE), &cbRead);
	if (hr == S_FALSE || (cbRead != sizeof(VARTYPE) && hr == S_OK))
		hr = E_FAIL;
	if (FAILED(hr))
		return hr;

	vt = vtRead;
	cbRead = 0;
	switch (vtRead)
	{
	case VT_UNKNOWN:
	case VT_DISPATCH:
		{
			punkVal = NULL;
			hr = OleLoadFromStream(pStream,
				(vtRead == VT_UNKNOWN) ? __uuidof(IUnknown) : __uuidof(IDispatch),
				(void**)&punkVal);
			// If IPictureDisp or IFontDisp property is not set, 
			// OleLoadFromStream() will return REGDB_E_CLASSNOTREG.
			if (hr == REGDB_E_CLASSNOTREG)
				hr = S_OK;
			return hr;
		}
	case VT_UI1:
	case VT_I1:
		cbRead = sizeof(BYTE);
		break;
	case VT_I2:
	case VT_UI2:
	case VT_BOOL:
		cbRead = sizeof(short);
		break;
	case VT_I4:
	case VT_UI4:
	case VT_R4:
	case VT_INT:
	case VT_UINT:
	case VT_ERROR:
		cbRead = sizeof(long);
		break;
	case VT_I8:
	case VT_UI8:
		cbRead = sizeof(LONGLONG);
		break;
	case VT_R8:
	case VT_CY:
	case VT_DATE:
		cbRead = sizeof(double);
		break;
	default:
		break;
	}
	if (cbRead != 0)
	{
		hr = pStream->Read((void*) &bVal, cbRead, NULL);
		if (hr == S_FALSE)
			hr = E_FAIL;
		return hr;
	}
	CComBSTR bstrRead;

	hr = bstrRead.ReadFromStream(pStream);
	if (FAILED(hr))
	{
		// If CComBSTR::ReadFromStream failed, reset seek pointer to start of
		// variant type.
		LARGE_INTEGER nOffset;
		nOffset.QuadPart = -(static_cast<LONGLONG>(sizeof(VARTYPE)));
		pStream->Seek(nOffset, STREAM_SEEK_CUR, NULL);
		return hr;
	}
	vt = VT_BSTR;
	bstrVal = bstrRead.Detach();
	if (vtRead != VT_BSTR)
		hr = ChangeType(vtRead);
	return hr;
}

inline ULONG CComVariant::GetSize() const
{
	ULONG nSize = sizeof(VARTYPE);
	HRESULT hr;

	switch (vt)
	{
	case VT_UNKNOWN:
	case VT_DISPATCH:
		{
			CComPtr<IPersistStream> spStream;
			if (punkVal != NULL)
			{
				hr = punkVal->QueryInterface(__uuidof(IPersistStream), (void**)&spStream);
				if (FAILED(hr))
				{
					hr = punkVal->QueryInterface(__uuidof(IPersistStreamInit), (void**)&spStream);
					if (FAILED(hr))
						break;
				}
			}
			if (spStream != NULL)
			{
				ULARGE_INTEGER nPersistSize;
				nPersistSize.QuadPart = 0;
				spStream->GetSizeMax(&nPersistSize);
				nSize += nPersistSize.LowPart + sizeof(CLSID);
			}
			else
				nSize += sizeof(CLSID);
		}
		break;
	case VT_UI1:
	case VT_I1:
		nSize += sizeof(BYTE);
		break;
	case VT_I2:
	case VT_UI2:
	case VT_BOOL:
		nSize += sizeof(short);
		break;
	case VT_I4:
	case VT_UI4:
	case VT_R4:
	case VT_INT:
	case VT_UINT:
	case VT_ERROR:
		nSize += sizeof(long);
		break;
	case VT_I8:
	case VT_UI8:
		nSize += sizeof(LONGLONG);
		break;
	case VT_R8:
	case VT_CY:
	case VT_DATE:
		nSize += sizeof(double);
		break;
	default:
		break;
	}
	if (nSize == sizeof(VARTYPE))
	{
		VARTYPE vtTmp = vt;
		BSTR        bstr = NULL;
		CComVariant varBSTR;
		if (vtTmp != VT_BSTR)
		{
			hr = VariantChangeType(&varBSTR, const_cast<VARIANT*>((const VARIANT*)this), VARIANT_NOVALUEPROP, VT_BSTR);
			if (SUCCEEDED(hr))
			{
				bstr = varBSTR.bstrVal;
				vtTmp = VT_BSTR;
			}
		} else
		{
			bstr = bstrVal;
		}

		if (vtTmp == VT_BSTR)
		{
			// Add the size of the length + string (in bytes) + NULL terminator.
			nSize += CComBSTR::GetStreamSize(bstr);			
		}
	}
	return nSize;
}

inline HRESULT CComPtr<IDispatch>::Invoke2(__in DISPID dispid, __in VARIANT* pvarParam1, __in VARIANT* pvarParam2, __out_opt VARIANT* pvarRet) throw()
{
	if(pvarParam1 == NULL || pvarParam2 == NULL)
		return E_INVALIDARG;
			
	CComVariant varArgs[2] = { *pvarParam2, *pvarParam1 };
	DISPPARAMS dispparams = { &varArgs[0], NULL, 2, 0};
	return p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, NULL, NULL);
}

}	// namespace ATL
#pragma pack(pop)

#pragma warning (pop)	

#ifndef _ATL_NO_AUTOMATIC_NAMESPACE
using namespace ATL;
#endif //!_ATL_NO_AUTOMATIC_NAMESPACE

#endif	// __ATLCOMCLI_H__
