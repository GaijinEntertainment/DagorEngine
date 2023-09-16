// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLCOM_H__
#define __ATLCOM_H__

#pragma once

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning (push)
#pragma warning(disable: 4702) // unreachable code
#pragma warning(disable: 4355) // 'this' used in initializer list
#pragma warning(disable: 4511) // copy constructor could not be generated
#pragma warning(disable: 4512) // assignment operator could not be generated
#pragma warning(disable : 4668)	// is not defined as a preprocessor macro, replacing with '0' for '#if/#elif
#pragma warning(disable : 4820)	// padding added after member
#endif //!_ATL_NO_PRAGMA_WARNINGS

#ifdef _ATL_ALL_WARNINGS
#pragma warning( push )
#endif
#pragma warning(disable: 4127) // constant expression
#pragma warning(disable: 4786) // avoid 255-character limit warnings

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLBASE_H__
	#error atlcom.h requires atlbase.h to be included first
#endif

#pragma pack(push, _ATL_PACKING)

EXTERN_C const IID IID_ITargetFrame;

#include <limits.h>

namespace ATL
{

#define CComConnectionPointContainerImpl ATL::IConnectionPointContainerImpl
#define CComISupportErrorInfoImpl ATL::ISupportErrorInfoImpl
#define CComProvideClassInfo2Impl ATL::IProvideClassInfoImpl
#define CComDualImpl ATL::IDispatchImpl

#ifdef _ATL_DEBUG_QI
#ifndef _ATL_DEBUG
#define _ATL_DEBUG
#endif // _ATL_DEBUG
#endif // _ATL_DEBUG_QI

#ifdef _ATL_DEBUG_QI
#define _ATLDUMPIID(iid, name, hr) AtlDumpIID(iid, name, hr)
#else
#define _ATLDUMPIID(iid, name, hr) hr
#endif

#define _ATL_DEBUG_ADDREF_RELEASE_IMPL(className)\
	virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0;\
	virtual ULONG STDMETHODCALLTYPE Release(void) = 0;

/////////////////////////////////////////////////////////////////////////////
// AtlReportError

inline HRESULT WINAPI AtlReportError(const CLSID& clsid, UINT nID, const IID& iid = GUID_NULL,
	HRESULT hRes = 0, HINSTANCE hInst = _AtlBaseModule.GetResourceInstance())
{
	return AtlSetErrorInfo(clsid, (LPCOLESTR)MAKEINTRESOURCE(nID), 0, NULL, iid, hRes, hInst);
}

inline HRESULT WINAPI AtlReportError(const CLSID& clsid, UINT nID, DWORD dwHelpID,
	LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0, 
	HINSTANCE hInst = _AtlBaseModule.GetResourceInstance())
{
	return AtlSetErrorInfo(clsid, (LPCOLESTR)MAKEINTRESOURCE(nID), dwHelpID,
		lpszHelpFile, iid, hRes, hInst);
}

inline HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCSTR lpszDesc,
	DWORD dwHelpID, LPCSTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0)
{
	ATLASSERT(lpszDesc != NULL);
	if (lpszDesc == NULL)
		return E_POINTER;
	USES_CONVERSION_EX;
	LPCOLESTR pwszDesc = A2COLE_EX(lpszDesc, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
	if(pwszDesc == NULL)
		return E_OUTOFMEMORY;
	
	LPCWSTR pwzHelpFile = NULL;
	if(lpszHelpFile != NULL)
	{
		pwzHelpFile = A2CW_EX(lpszHelpFile, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
		if(pwzHelpFile == NULL)
			return E_OUTOFMEMORY;
	}
		
	return AtlSetErrorInfo(clsid, pwszDesc, dwHelpID, pwzHelpFile, iid, hRes, NULL);
}

inline HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCSTR lpszDesc,
	const IID& iid = GUID_NULL, HRESULT hRes = 0)
{
	return AtlReportError(clsid, lpszDesc, 0, NULL, iid, hRes);
}

inline HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCOLESTR lpszDesc,
	const IID& iid = GUID_NULL, HRESULT hRes = 0)
{
	return AtlSetErrorInfo(clsid, lpszDesc, 0, NULL, iid, hRes, NULL);
}

inline HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCOLESTR lpszDesc, DWORD dwHelpID,
	LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0)
{
	return AtlSetErrorInfo(clsid, lpszDesc, dwHelpID, lpszHelpFile, iid, hRes, NULL);
}

// Returns the apartment type that the current thread is in. false is returned
// if the thread isn't in an apartment.
inline bool AtlGetApartmentType(DWORD* pApartmentType)
{
	HRESULT hr = CoInitialize(NULL);
 	if (SUCCEEDED(hr))
		CoUninitialize();

	if (hr == S_FALSE)
	{
		*pApartmentType = COINIT_APARTMENTTHREADED;
		return true;
	}
#if ((_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM))
	else
	if (hr == RPC_E_CHANGED_MODE)
	{	
		*pApartmentType = COINIT_MULTITHREADED;
		return true;
	}
#endif
	return false;
}

/////////////////////////////////////////////////////////////////////////////
// CComTypeAttr

class CComTypeAttr
{
// Construction
public:
   CComTypeAttr( ITypeInfo* pTypeInfo ) throw() :
	  m_pTypeAttr( NULL ),
	  m_pTypeInfo( pTypeInfo )
   {
   }
   ~CComTypeAttr() throw()
   {
	  Release();
   }

// Operators
public:
   TYPEATTR* operator->() throw()
   {
	  ATLASSERT( m_pTypeAttr != NULL );

	  return m_pTypeAttr;
   }
   TYPEATTR** operator&() throw()
   {
	  ATLASSERT( m_pTypeAttr == NULL );

	  return &m_pTypeAttr;
   }

   operator const TYPEATTR*() const throw()
   {
	  return m_pTypeAttr;
   }

// Operations
public:
   void Release() throw()
   {
	  if( m_pTypeAttr != NULL )
	  {
		 ATLASSERT( m_pTypeInfo != NULL );
		 m_pTypeInfo->ReleaseTypeAttr( m_pTypeAttr );
		 m_pTypeAttr = NULL;
	  }
   }

public:
   TYPEATTR* m_pTypeAttr;
   CComPtr< ITypeInfo > m_pTypeInfo;
};


/////////////////////////////////////////////////////////////////////////////
// CComVarDesc

class CComVarDesc
{
// Construction
public:
   CComVarDesc( ITypeInfo* pTypeInfo ) throw() :
	  m_pVarDesc( NULL ),
	  m_pTypeInfo( pTypeInfo )
   {
   }
   ~CComVarDesc() throw()
   {
	  Release();
   }

// Operators
public:
   VARDESC* operator->() throw()
   {
	  ATLASSERT( m_pVarDesc != NULL );

	  return m_pVarDesc;
   }
   VARDESC** operator&() throw()
   {
	  ATLASSERT( m_pVarDesc == NULL );

	  return &m_pVarDesc;
   }

   operator const VARDESC*() const throw()
   {
	  return m_pVarDesc;
   }

// Operations
public:
   void Release() throw()
   {
	  if( m_pVarDesc != NULL )
	  {
		 ATLASSERT( m_pTypeInfo != NULL );
		 m_pTypeInfo->ReleaseVarDesc( m_pVarDesc );
		 m_pVarDesc = NULL;
	  }
   }

public:
   VARDESC* m_pVarDesc;
   CComPtr< ITypeInfo > m_pTypeInfo;
};


/////////////////////////////////////////////////////////////////////////////
// CComFuncDesc

class CComFuncDesc
{
// Construction
public:
   CComFuncDesc( ITypeInfo* pTypeInfo ) throw() :
	  m_pFuncDesc( NULL ),
	  m_pTypeInfo( pTypeInfo )
   {
   }
   ~CComFuncDesc() throw()
   {
	  Release();
   }

// Operators
public:
   FUNCDESC* operator->() throw()
   {
	  ATLASSERT( m_pFuncDesc != NULL );

	  return m_pFuncDesc;
   }
   FUNCDESC** operator&() throw()
   {
	  ATLASSERT( m_pFuncDesc == NULL );

	  return &m_pFuncDesc;
   }

   operator const FUNCDESC*() const throw()
   {
	  return m_pFuncDesc;
   }

// Operations
public:
   void Release() throw()
   {
	  if( m_pFuncDesc != NULL )
	  {
		 ATLASSERT( m_pTypeInfo != NULL );
		 m_pTypeInfo->ReleaseFuncDesc( m_pFuncDesc );
		 m_pFuncDesc = NULL;
	  }
   }

public:
   FUNCDESC* m_pFuncDesc;
   CComPtr< ITypeInfo > m_pTypeInfo;
};


/////////////////////////////////////////////////////////////////////////////
// CComExcepInfo

class CComExcepInfo :
   public EXCEPINFO
{
// Construction
public:
   CComExcepInfo()
   {
	  memset( this, 0, sizeof( *this ) );
   }
   ~CComExcepInfo()
   {
	  Clear();
   }

// Operations
public:
   void Clear()
   {
	  if (bstrSource != NULL)
		 ::SysFreeString(bstrSource);

	  if (bstrDescription != NULL)
	 ::SysFreeString(bstrDescription);

	  if (bstrHelpFile != NULL)
	 ::SysFreeString(bstrHelpFile);

	  memset(this, 0, sizeof(*this));
   }
};


//////////////////////////////////////////////////////////////////////////////
// IPersistImpl
template <class T>
class ATL_NO_VTABLE IPersistImpl : public IPersist
{
public:
	STDMETHOD(GetClassID)(CLSID *pClassID)
	{
		ATLTRACE(atlTraceCOM, 2, _T("IPersistImpl::GetClassID\n"));
		if (pClassID == NULL)
			return E_FAIL;
		*pClassID = T::GetObjectCLSID();
		return S_OK;
	}
};

//////////////////////////////////////////////////////////////////////////////
// CFakeFirePropNotifyEvent
class CFakeFirePropNotifyEvent
{
public:
	static HRESULT FireOnRequestEdit(IUnknown* /*pUnk*/, DISPID /*dispID*/)
	{
		return S_OK;
	}
	static HRESULT FireOnChanged(IUnknown* /*pUnk*/, DISPID /*dispID*/)
	{
		return S_OK;
	}
};
typedef CFakeFirePropNotifyEvent _ATL_PROP_NOTIFY_EVENT_CLASS;


//////////////////////////////////////////////////////////////////////////////
// ALT_PROP_VAL_MAP

struct ATL_PROPVALMAP_ENTRY
{
	DISPID dispid;
	VARIANT val;
	LPCOLESTR szDesc;
};

#define BEGIN_PROP_VAL_MAP(theClass) \
	static ATL::ATL_PROPVALMAP_ENTRY* GetPropValMap(int *cnt)\
	{\
		static ATL::ATL_PROPVALMAP_ENTRY pPropMap[] = \
		{

#define PROP_VAL_INT(dispid, ival, str) \
			{dispid, {VT_I4, 0, 0, 0, ival}, OLESTR(str)},


#define END_PROP_VAL_MAP() \
		}; \
		if (cnt)	\
			*cnt = sizeof(pPropMap)/sizeof(pPropMap[0]);	\
		return pPropMap; \
	}

#define DECLARE_EMPTY_PROP_VAL_MAP() \
public: \
	static ATL::ATL_PROPVALMAP_ENTRY* GetPropValMap(int *cnt)\
	{ \
		if (cnt)	\
			*cnt = 0;	\
		return NULL; \
	}

//////////////////////////////////////////////////////////////////////////////
// ATL Persistence

struct ATL_PROPMAP_ENTRY
{
	LPCOLESTR szDesc;
	DISPID dispid;
	const CLSID* pclsidPropPage;
	const IID* piidDispatch;
	DWORD dwOffsetData;
	DWORD dwSizeData;
	VARTYPE vt;
};

// This one is DEPRECATED and is used for ATL 2.X controls
// it includes an implicit m_sizeExtent
#define BEGIN_PROPERTY_MAP(theClass) \
	__if_not_exists(__ATL_PROP_NOTIFY_EVENT_CLASS) \
	{ \
		typedef ATL::_ATL_PROP_NOTIFY_EVENT_CLASS __ATL_PROP_NOTIFY_EVENT_CLASS; \
	} \
	typedef theClass _PropMapClass; \
	static ATL::ATL_PROPMAP_ENTRY* GetPropertyMap()\
	{\
		static ATL::ATL_PROPMAP_ENTRY pPropMap[] = \
		{ \
			{OLESTR("_cx"), 0, &CLSID_NULL, NULL, offsetof(_PropMapClass, m_sizeExtent.cx), sizeof(long), VT_UI4}, \
			{OLESTR("_cy"), 0, &CLSID_NULL, NULL, offsetof(_PropMapClass, m_sizeExtent.cy), sizeof(long), VT_UI4},

// This one can be used on any type of object, but does not
// include the implicit m_sizeExtent
#define BEGIN_PROP_MAP(theClass) \
	__if_not_exists(__ATL_PROP_NOTIFY_EVENT_CLASS) \
	{ \
		typedef ATL::_ATL_PROP_NOTIFY_EVENT_CLASS __ATL_PROP_NOTIFY_EVENT_CLASS; \
	} \
	typedef theClass _PropMapClass; \
	static ATL::ATL_PROPMAP_ENTRY* GetPropertyMap()\
	{\
		static ATL::ATL_PROPMAP_ENTRY pPropMap[] = \
		{

#define PROP_ENTRY(szDesc, dispid, clsid) \
		{OLESTR(szDesc), dispid, &clsid, &__uuidof(IDispatch), 0, 0, 0},

#define PROP_ENTRY_EX(szDesc, dispid, clsid, iidDispatch) \
		{OLESTR(szDesc), dispid, &clsid, &iidDispatch, 0, 0, 0},

#define PROP_PAGE(clsid) \
		{NULL, NULL, &clsid, &IID_NULL, 0, 0, 0},

#define PROP_DATA_ENTRY(szDesc, member, vt) \
		{OLESTR(szDesc), 0, &CLSID_NULL, NULL, offsetof(_PropMapClass, member), sizeof(((_PropMapClass*)0)->member), vt},

#define END_PROPERTY_MAP() \
			{NULL, 0, NULL, &IID_NULL, 0, 0, 0} \
		}; \
		return pPropMap; \
	}

#define END_PROP_MAP() \
			{NULL, 0, NULL, &IID_NULL, 0, 0, 0} \
		}; \
		return pPropMap; \
	}

//////////////////////////////////////////////////////////////////////////////
// IPersist* Helpers

ATLAPI AtlIPersistStreamInit_Load(LPSTREAM pStm, ATL_PROPMAP_ENTRY* pMap, void* pThis, IUnknown* pUnk);
ATLAPI AtlIPersistStreamInit_Save(LPSTREAM pStm, BOOL fClearDirty, ATL_PROPMAP_ENTRY* pMap, void* pThis, IUnknown* pUnk);
ATLAPI AtlIPersistPropertyBag_Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog, ATL_PROPMAP_ENTRY* pMap, void* pThis, IUnknown* pUnk);
ATLAPI AtlIPersistPropertyBag_Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties, ATL_PROPMAP_ENTRY* pMap, void* pThis, IUnknown* pUnk);

//////////////////////////////////////////////////////////////////////////////
// IPersistStreamInitImpl
template <class T>
class ATL_NO_VTABLE IPersistStreamInitImpl : public IPersistStreamInit
{
public:
	// IPersist
	STDMETHOD(GetClassID)(CLSID *pClassID)
	{
		ATLTRACE(atlTraceCOM, 2, _T("IPersistStreamInitImpl::GetClassID\n"));
		if (pClassID == NULL)
			return E_POINTER;
		*pClassID = T::GetObjectCLSID();
		return S_OK;
	}

	// IPersistStream
	STDMETHOD(IsDirty)()
	{
		ATLTRACE(atlTraceCOM, 2, _T("IPersistStreamInitImpl::IsDirty\n"));
		T* pT = static_cast<T*>(this);
		return (pT->m_bRequiresSave) ? S_OK : S_FALSE;
	}
	STDMETHOD(Load)(LPSTREAM pStm)
	{
		ATLTRACE(atlTraceCOM, 2, _T("IPersistStreamInitImpl::Load\n"));

		T* pT = static_cast<T*>(this);
		return pT->IPersistStreamInit_Load(pStm, T::GetPropertyMap());
	}
	STDMETHOD(Save)(LPSTREAM pStm, BOOL fClearDirty)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(atlTraceCOM, 2, _T("IPersistStreamInitImpl::Save\n"));
		return pT->IPersistStreamInit_Save(pStm, fClearDirty, T::GetPropertyMap());
	}
	STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pcbSize)
	{
		HRESULT hr = S_OK;
		T* pT = static_cast<T*>(this);
		ATLTRACE(atlTraceCOM, 2, _T("IPersistStreamInitImpl::GetSizeMax\n"));

		if (pcbSize == NULL)
			return E_POINTER;

		ATL_PROPMAP_ENTRY* pMap = T::GetPropertyMap();
		ATLASSERT(pMap != NULL);

		// Start the size with the size of the ATL version we write out.
		ULARGE_INTEGER nSize;
		nSize.HighPart = 0;
		nSize.LowPart = sizeof(DWORD);

		CComPtr<IDispatch> pDispatch;
		const IID* piidOld = NULL;
		for (int i = 0; pMap[i].pclsidPropPage != NULL; i++)
		{
			if (pMap[i].szDesc == NULL)
				continue;

			// check if raw data entry
			if (pMap[i].dwSizeData != 0)
			{
				nSize.QuadPart += pMap[i].dwSizeData;
				continue;
			}

			CComVariant var;
			if (pMap[i].piidDispatch != piidOld)
			{
				pDispatch.Release();
				if (FAILED(pT->GetUnknown()->QueryInterface(*pMap[i].piidDispatch, (void**)&pDispatch)))
				{
					ATLTRACE(atlTraceCOM, 0, _T("Failed to get a dispatch pointer for property #%i\n"), i);
					hr = E_FAIL;
					break;
				}
				piidOld = pMap[i].piidDispatch;
			}

			if (FAILED(pDispatch.GetProperty(pMap[i].dispid, &var)))
			{
				ATLTRACE(atlTraceCOM, 0, _T("Invoked failed on DISPID %x\n"), pMap[i].dispid);
				hr = E_FAIL;
				break;
			}
			nSize.QuadPart += var.GetSize();
		}
		*pcbSize = nSize;
		return hr;
	}

	// IPersistStreamInit
	STDMETHOD(InitNew)()
	{
		ATLTRACE(atlTraceCOM, 2, _T("IPersistStreamInitImpl::InitNew\n"));
		T* pT = static_cast<T*>(this);
		pT->m_bRequiresSave = TRUE;
		return S_OK;
	}

	HRESULT IPersistStreamInit_Load(LPSTREAM pStm, ATL_PROPMAP_ENTRY* pMap)
	{
		T* pT = static_cast<T*>(this);
		HRESULT hr = AtlIPersistStreamInit_Load(pStm, pMap, pT, pT->GetUnknown());
		if (SUCCEEDED(hr))
			pT->m_bRequiresSave = FALSE;
		return hr;

	}
	HRESULT IPersistStreamInit_Save(LPSTREAM pStm, BOOL fClearDirty, ATL_PROPMAP_ENTRY* pMap)
	{
		T* pT = static_cast<T*>(this);
		return AtlIPersistStreamInit_Save(pStm, fClearDirty, pMap, pT, pT->GetUnknown());
	}
};

//////////////////////////////////////////////////////////////////////////////
// IPersistStorageImpl
template <class T>
class ATL_NO_VTABLE IPersistStorageImpl : public IPersistStorage
{
public:
	// IPersist
	STDMETHOD(GetClassID)(CLSID *pClassID)
	{
		ATLTRACE(atlTraceCOM, 2, _T("IPersistStorageImpl::GetClassID\n"));
		if (pClassID == NULL)
			return E_POINTER;
		*pClassID = T::GetObjectCLSID();
		return S_OK;
	}

	// IPersistStorage
	STDMETHOD(IsDirty)(void)
	{
		ATLTRACE(atlTraceCOM, 2, _T("IPersistStorageImpl::IsDirty\n"));
		CComPtr<IPersistStreamInit> p;
		p.p = IPSI_GetIPersistStreamInit();
		return (p != NULL) ? p->IsDirty() : E_FAIL;
	}
	STDMETHOD(InitNew)(IStorage*)
	{
		ATLTRACE(atlTraceCOM, 2, _T("IPersistStorageImpl::InitNew\n"));
		CComPtr<IPersistStreamInit> p;
		p.p = IPSI_GetIPersistStreamInit();
		return (p != NULL) ? p->InitNew() : E_FAIL;
	}
	STDMETHOD(Load)(IStorage* pStorage)
	{
		ATLTRACE(atlTraceCOM, 2, _T("IPersistStorageImpl::Load\n"));
		if (pStorage == NULL)
			return E_INVALIDARG;
		CComPtr<IPersistStreamInit> p;
		p.p = IPSI_GetIPersistStreamInit();
		HRESULT hr = E_FAIL;
		if (p != NULL)
		{
			CComPtr<IStream> spStream;
			hr = pStorage->OpenStream(OLESTR("Contents"), NULL,
				STGM_DIRECT | STGM_SHARE_EXCLUSIVE, 0, &spStream);
			if (SUCCEEDED(hr))
				hr = p->Load(spStream);
		}
		return hr;
	}
	STDMETHOD(Save)(IStorage* pStorage, BOOL fSameAsLoad)
	{
		ATLTRACE(atlTraceCOM, 2, _T("IPersistStorageImpl::Save\n"));
		if (pStorage == NULL)
			return E_INVALIDARG;
		CComPtr<IPersistStreamInit> p;
		p.p = IPSI_GetIPersistStreamInit();
		HRESULT hr = E_FAIL;
		if (p != NULL)
		{
			CComPtr<IStream> spStream;
			static LPCOLESTR vszContents = OLESTR("Contents");
			hr = pStorage->CreateStream(vszContents,
				STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
				0, 0, &spStream);
			if (SUCCEEDED(hr))
				hr = p->Save(spStream, fSameAsLoad);
		}
		return hr;
	}
	STDMETHOD(SaveCompleted)(IStorage* /* pStorage */)
	{
		ATLTRACE(atlTraceCOM, 2, _T("IPersistStorageImpl::SaveCompleted\n"));
		return S_OK;
	}
	STDMETHOD(HandsOffStorage)(void)
	{
		ATLTRACE(atlTraceCOM, 2, _T("IPersistStorageImpl::HandsOffStorage\n"));
		return S_OK;
	}
private:
	IPersistStreamInit* IPSI_GetIPersistStreamInit();
};

template <class T>
IPersistStreamInit* IPersistStorageImpl<T>::IPSI_GetIPersistStreamInit()
{
	T* pT = static_cast<T*>(this);
	IPersistStreamInit* p;
	if (FAILED(pT->GetUnknown()->QueryInterface(__uuidof(IPersistStreamInit), (void**)&p)))
		pT->_InternalQueryInterface(__uuidof(IPersistStreamInit), (void**)&p);
	return p;
}


//////////////////////////////////////////////////////////////////////////////
// IPersistPropertyBagImpl
template <class T>
class ATL_NO_VTABLE IPersistPropertyBagImpl : public IPersistPropertyBag
{
public:
	// IPersist
	STDMETHOD(GetClassID)(CLSID *pClassID)
	{
		ATLTRACE(atlTraceCOM, 2, _T("IPersistPropertyBagImpl::GetClassID\n"));
		if (pClassID == NULL)
			return E_POINTER;
		*pClassID = T::GetObjectCLSID();
		return S_OK;
	}

	// IPersistPropertyBag
	//
	STDMETHOD(InitNew)()
	{
		ATLTRACE(atlTraceCOM, 2, _T("IPersistPropertyBagImpl::InitNew\n"));
		T* pT = static_cast<T*>(this);
		pT->m_bRequiresSave = TRUE;
		return S_OK;
	}
	STDMETHOD(Load)(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
	{
		ATLTRACE(atlTraceCOM, 2, _T("IPersistPropertyBagImpl::Load\n"));
		T* pT = static_cast<T*>(this);
		ATL_PROPMAP_ENTRY* pMap = T::GetPropertyMap();
		ATLASSERT(pMap != NULL);
		return pT->IPersistPropertyBag_Load(pPropBag, pErrorLog, pMap);
	}
	STDMETHOD(Save)(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
	{
		ATLTRACE(atlTraceCOM, 2, _T("IPersistPropertyBagImpl::Save\n"));
		T* pT = static_cast<T*>(this);
		ATL_PROPMAP_ENTRY* pMap = T::GetPropertyMap();
		ATLASSERT(pMap != NULL);
		return pT->IPersistPropertyBag_Save(pPropBag, fClearDirty, fSaveAllProperties, pMap);
	}
	HRESULT IPersistPropertyBag_Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog, ATL_PROPMAP_ENTRY* pMap)
	{
		T* pT = static_cast<T*>(this);
		HRESULT hr = AtlIPersistPropertyBag_Load(pPropBag, pErrorLog, pMap, pT, pT->GetUnknown());
		if (SUCCEEDED(hr))
			pT->m_bRequiresSave = FALSE;
		return hr;
	}
	HRESULT IPersistPropertyBag_Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties, ATL_PROPMAP_ENTRY* pMap)
	{
		T* pT = static_cast<T*>(this);
		return AtlIPersistPropertyBag_Save(pPropBag, fClearDirty, fSaveAllProperties, pMap, pT, pT->GetUnknown());
	}
};

//////////////////////////////////////////////////////////////////////////////
// CSecurityDescriptor
__declspec(deprecated) class CSecurityDescriptor
{
public:
	CSecurityDescriptor();
	~CSecurityDescriptor();

public:
	HRESULT Attach(PSECURITY_DESCRIPTOR pSelfRelativeSD);
	HRESULT AttachObject(HANDLE hObject);
	HRESULT Initialize();
	HRESULT InitializeFromProcessToken(BOOL bDefaulted = FALSE);
	HRESULT InitializeFromThreadToken(BOOL bDefaulted = FALSE, BOOL bRevertToProcessToken = TRUE);
	HRESULT SetOwner(PSID pOwnerSid, BOOL bDefaulted = FALSE);
	HRESULT SetGroup(PSID pGroupSid, BOOL bDefaulted = FALSE);
	HRESULT Allow(LPCTSTR pszPrincipal, DWORD dwAccessMask);
	HRESULT Deny(LPCTSTR pszPrincipal, DWORD dwAccessMask);
	HRESULT Revoke(LPCTSTR pszPrincipal);
	HRESULT Allow(PSID pSid, DWORD dwAccessMask);
	HRESULT Deny(PSID pSid, DWORD dwAccessMask);
	HRESULT Revoke(PSID pSid);

	// utility functions
	// Any PSID you get from these functions should be free()ed
	static HRESULT SetPrivilege(LPCTSTR Privilege, BOOL bEnable = TRUE, HANDLE hToken = NULL);
	static HRESULT GetTokenSids(HANDLE hToken, PSID* ppUserSid, PSID* ppGroupSid);
	static HRESULT GetProcessSids(PSID* ppUserSid, PSID* ppGroupSid = NULL);
	static HRESULT GetThreadSids(PSID* ppUserSid, PSID* ppGroupSid = NULL, BOOL bOpenAsSelf = FALSE);
	static HRESULT CopyACL(PACL pDest, PACL pSrc);
	static HRESULT GetCurrentUserSID(PSID *ppSid);
	static HRESULT GetPrincipalSID(LPCTSTR pszPrincipal, PSID *ppSid);
	static HRESULT AddAccessAllowedACEToACL(PACL *Acl, PSID pSid, DWORD dwAccessMask);
	static HRESULT AddAccessDeniedACEToACL(PACL *Acl, PSID pSid, DWORD dwAccessMask);
	static HRESULT RemovePrincipalFromACL(PACL Acl, PSID pSid);
	static HRESULT CloneSID(PSID *ppSIDDest, PSID pSIDSrc)
	{
		HRESULT hr = S_OK;
		if (ppSIDDest == NULL)
			return E_POINTER;

		if (*ppSIDDest != NULL)
			return E_INVALIDARG;
		*ppSIDDest = NULL;
			
		if (!IsValidSid(pSIDSrc))
			return E_INVALIDARG;

		DWORD dwSize = GetLengthSid(pSIDSrc);

		*ppSIDDest = (PSID) malloc(dwSize);
		if (*ppSIDDest == NULL)
			return E_OUTOFMEMORY;
		if (!CopySid(dwSize, *ppSIDDest, pSIDSrc))
		{
			hr = AtlHresultFromLastError();
			ATLASSERT(FALSE);
			free(*ppSIDDest);
			*ppSIDDest = NULL;
		}
		return hr;
	}
	operator PSECURITY_DESCRIPTOR()
	{
		return m_pSD;
	}

public:
	PSECURITY_DESCRIPTOR m_pSD;
	PSID m_pOwner;
	PSID m_pGroup;
	PACL m_pDACL;
	PACL m_pSACL;
};

inline CSecurityDescriptor::CSecurityDescriptor()
{
	m_pSD = NULL;
	m_pOwner = NULL;
	m_pGroup = NULL;
	m_pDACL = NULL;
	m_pSACL= NULL;
}

inline CSecurityDescriptor::~CSecurityDescriptor()
{
	delete m_pSD;
	free(m_pOwner);
	free(m_pGroup);
	free(m_pDACL);
	free(m_pSACL);
}

inline HRESULT CSecurityDescriptor::Initialize()
{
	delete m_pSD;
	m_pSD = NULL;

	free(m_pOwner);
	m_pOwner = NULL;

	free(m_pGroup);
	m_pGroup = NULL;

	free(m_pDACL);
	m_pDACL = NULL;

	free(m_pSACL);
	m_pSACL = NULL;

	ATLTRY(m_pSD = new SECURITY_DESCRIPTOR);
	if (m_pSD != NULL)
	{
		if (InitializeSecurityDescriptor(m_pSD, SECURITY_DESCRIPTOR_REVISION))
			return S_OK;

		HRESULT hr = AtlHresultFromLastError();
		delete m_pSD;
		m_pSD = NULL;
		ATLASSERT(FALSE);
		return hr;
	}

	return E_OUTOFMEMORY;
}

inline HRESULT CSecurityDescriptor::InitializeFromProcessToken(BOOL bDefaulted)
{
	HRESULT hr = Initialize();
	if (SUCCEEDED(hr))
	{
		PSID pUserSid = NULL;
		PSID pGroupSid = NULL;
		hr = GetProcessSids(&pUserSid, &pGroupSid);
		if (SUCCEEDED(hr))
		{
			hr = SetOwner(pUserSid, bDefaulted);
			if (SUCCEEDED(hr))
			{
				hr = SetGroup(pGroupSid, bDefaulted);
			}			
			free(pUserSid);
			free(pGroupSid);
			// If something failed reinitialize the object
			if (FAILED(hr))
				Initialize();
		}
	}
	return hr;
}

inline HRESULT CSecurityDescriptor::InitializeFromThreadToken(BOOL bDefaulted, BOOL bRevertToProcessToken)
{
	HRESULT hr = Initialize();
	if (SUCCEEDED(hr))
	{
		PSID pUserSid = NULL;
		PSID pGroupSid = NULL;
	
		hr = GetThreadSids(&pUserSid, &pGroupSid);
		if (HRESULT_CODE(hr) == ERROR_NO_TOKEN && bRevertToProcessToken)
			hr = GetProcessSids(&pUserSid, &pGroupSid);
		if (SUCCEEDED(hr))
		{
			hr = SetOwner(pUserSid, bDefaulted);
			if (SUCCEEDED(hr))
				hr = SetGroup(pGroupSid, bDefaulted);
			free(pUserSid);
			free(pGroupSid);
			// If something failed reinitialize the object
			if (FAILED(hr))
				Initialize();
		}
	}
	return hr;
}

inline HRESULT CSecurityDescriptor::SetOwner(PSID pOwnerSid, BOOL bDefaulted)
{
	ATLASSERT(m_pSD);
	HRESULT hr = S_OK;

	// Mark the SD as having no owner
	if (SetSecurityDescriptorOwner(m_pSD, NULL, bDefaulted))
	{
		free(m_pOwner);
		m_pOwner = NULL;

		// If they asked for no owner don't do the copy
		if (pOwnerSid == NULL)
			return S_OK;

		// Make a copy of the Sid for the return value
		hr = CloneSID(&m_pOwner, pOwnerSid);
		if (SUCCEEDED(hr))
		{
			ATLASSERT(IsValidSid(m_pOwner));

			if (!SetSecurityDescriptorOwner(m_pSD, m_pOwner, bDefaulted))
			{
				hr = AtlHresultFromLastError();
				ATLASSERT(FALSE);
				free(m_pOwner);
				m_pOwner = NULL;
			}
		}
	}
	else
	{
		hr = AtlHresultFromLastError();
		ATLASSERT(FALSE);
	}

	return hr;
}

inline HRESULT CSecurityDescriptor::SetGroup(PSID pGroupSid, BOOL bDefaulted)
{
	ATLASSERT(m_pSD);
	HRESULT hr = S_OK;

	// Mark the SD as having no Group
	if (SetSecurityDescriptorGroup(m_pSD, NULL, bDefaulted))
	{
		free(m_pGroup);
		m_pGroup = NULL;

		// If they asked for no Group don't do the copy
		if (pGroupSid == NULL)
			return S_OK;

		// Make a copy of the Sid for the return value
		hr = CloneSID(&m_pGroup, pGroupSid);
		if (SUCCEEDED(hr))
		{
			ATLASSERT(IsValidSid(m_pGroup));

			if (!SetSecurityDescriptorGroup(m_pSD, m_pGroup, bDefaulted))
			{
				hr = AtlHresultFromLastError();
				ATLASSERT(FALSE);
				free(m_pGroup);
				m_pGroup = NULL;
			}
		}
	}
	else
	{
		hr = AtlHresultFromLastError();
		ATLASSERT(FALSE);
	}

	return hr;
}

inline HRESULT CSecurityDescriptor::Allow(LPCTSTR pszPrincipal, DWORD dwAccessMask)
{
	PSID pSid;
	HRESULT hr = GetPrincipalSID(pszPrincipal, &pSid);
	if (SUCCEEDED(hr))
	{
		hr = Allow(pSid, dwAccessMask);
		free(pSid);
	}
	return hr;
}

inline HRESULT CSecurityDescriptor::Deny(LPCTSTR pszPrincipal, DWORD dwAccessMask)
{
	PSID pSid;
	HRESULT hr = GetPrincipalSID(pszPrincipal, &pSid);
	if (SUCCEEDED(hr))
	{
		hr = Deny(pSid, dwAccessMask);
		free(pSid);
	}
	return hr;
}

inline HRESULT CSecurityDescriptor::Revoke(LPCTSTR pszPrincipal)
{
	PSID pSid;
	HRESULT hr = GetPrincipalSID(pszPrincipal, &pSid);
	if (SUCCEEDED(hr))
	{
		hr = Revoke(pSid);
		free(pSid);
	}
	return hr;
}

inline HRESULT CSecurityDescriptor::Allow(PSID pSid, DWORD dwAccessMask)
{
	HRESULT hr = AddAccessAllowedACEToACL(&m_pDACL, pSid, dwAccessMask);
	if (SUCCEEDED(hr))
	{
		if (!SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE))
			hr = AtlHresultFromLastError();
	}
	return hr;
}

inline HRESULT CSecurityDescriptor::Deny(PSID pSid, DWORD dwAccessMask)
{
	HRESULT hr = AddAccessDeniedACEToACL(&m_pDACL, pSid, dwAccessMask);
	if (SUCCEEDED(hr))
	{
		if (!SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE))
			hr = AtlHresultFromLastError();
	}
	return hr;
}

inline HRESULT CSecurityDescriptor::Revoke(PSID pSid)
{
	HRESULT hr = RemovePrincipalFromACL(m_pDACL, pSid);
	if (SUCCEEDED(hr))
	{
		if (!SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE))
			hr = AtlHresultFromLastError();
	}
	return hr;
}

inline HRESULT CSecurityDescriptor::GetProcessSids(PSID* ppUserSid, PSID* ppGroupSid)
{
	HRESULT hr = S_OK;
	HANDLE hToken = NULL;
	if (ppUserSid)
		*ppUserSid = NULL;
	if (ppGroupSid)
		*ppGroupSid = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		hr = GetTokenSids(hToken, ppUserSid, ppGroupSid);
		CloseHandle(hToken);
	}
	else
	{
		// Couldn't open process token
		hr = AtlHresultFromLastError();
		ATLASSERT(FALSE);
	}

	return hr;
}

inline HRESULT CSecurityDescriptor::GetThreadSids(PSID* ppUserSid, PSID* ppGroupSid, BOOL bOpenAsSelf)
{
	HRESULT hr = S_OK;
	HANDLE hToken = NULL;
	if (ppUserSid)
		*ppUserSid = NULL;
	if (ppGroupSid)
		*ppGroupSid = NULL;
	if(OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, bOpenAsSelf, &hToken))
	{
		hr = GetTokenSids(hToken, ppUserSid, ppGroupSid);
		CloseHandle(hToken);
	} 
	else
		// Couldn't open thread token
		hr = AtlHresultFromLastError();

	return hr;
}

inline HRESULT CSecurityDescriptor::GetTokenSids(HANDLE hToken, PSID* ppUserSid, PSID* ppGroupSid)
{
	DWORD dwSize = 0;
	HRESULT hr = S_OK;
	if (ppUserSid != NULL)
		*ppUserSid = NULL;
	if (ppGroupSid != NULL)
		*ppGroupSid = NULL;

	if (ppUserSid != NULL)
	{
		PTOKEN_USER ptkUser = NULL;

		// Get length required for TokenUser by specifying buffer length of 0
		GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
		// Expected ERROR_INSUFFICIENT_BUFFER		
		DWORD dwError = GetLastError();
		if (dwError == ERROR_INSUFFICIENT_BUFFER)
		{
			ptkUser = (TOKEN_USER*) malloc(dwSize);
			if (ptkUser != NULL)
			{
				// Get Sid of process token.
				if (GetTokenInformation(hToken, TokenUser, ptkUser, dwSize, &dwSize))
				{
					// Make a copy of the Sid for the return value
					hr = CloneSID(ppUserSid, ptkUser->User.Sid);

#ifdef _DEBUG
					if (SUCCEEDED(hr))
					{
						ATLASSERT(IsValidSid(*ppUserSid));
					}
#endif
				}
				else
					// Couldn't get user info
					hr = AtlHresultFromLastError();

				free(ptkUser);
				ptkUser = NULL;
			}
			else
				hr = E_OUTOFMEMORY;
		}
		else
		{
			ATLASSERT(FALSE);
			hr = AtlHresultFromWin32(dwError);
		}
	}
	if (SUCCEEDED(hr) && ppGroupSid != NULL)
	{
		PTOKEN_PRIMARY_GROUP ptkGroup = NULL;

		// Get length required for TokenPrimaryGroup by specifying buffer length of 0
		GetTokenInformation(hToken, TokenPrimaryGroup, NULL, 0, &dwSize);
		DWORD dwError = GetLastError();
		// Expected ERROR_INSUFFICIENT_BUFFER
		if (dwError == ERROR_INSUFFICIENT_BUFFER)
		{
			ptkGroup = (TOKEN_PRIMARY_GROUP*) malloc(dwSize);
			if (ptkGroup != NULL)
			{
				// Get Sid of process token.
				if (GetTokenInformation(hToken, TokenPrimaryGroup, ptkGroup, dwSize, &dwSize))
				{
					// Make a copy of the Sid for the return value
					hr = CloneSID(ppGroupSid, ptkGroup->PrimaryGroup);

#ifdef _DEBUG					
					if (SUCCEEDED(hr))
					{
						ATLASSERT(IsValidSid(*ppGroupSid));
					}
#endif
				}
				else
					// Couldn't get user info
					hr = AtlHresultFromLastError();

				free(ptkGroup);
				ptkGroup = NULL;
			}
			else
				hr = E_OUTOFMEMORY;
		}
		else
			hr = AtlHresultFromWin32(dwError);
	}
	if (FAILED(hr))
	{
		if (ppUserSid != NULL)
		{
			free (*ppUserSid);
			*ppUserSid = NULL;
		}
		if (ppGroupSid != NULL)
		{
			free (*ppGroupSid);
			*ppGroupSid = NULL;
		}
	}

	return hr;
}


inline HRESULT CSecurityDescriptor::GetCurrentUserSID(PSID *ppSid)
{
	if (ppSid == NULL)
		return E_POINTER;
	*ppSid = NULL;

	HANDLE tkHandle;
	HRESULT hr = S_OK;

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tkHandle))
	{
		TOKEN_USER *tkUser = NULL;
		DWORD tkSize;

		// Get length required for TokenPrimaryGroup by specifying buffer length of 0
		GetTokenInformation(tkHandle, TokenUser, NULL, 0, &tkSize);
		DWORD dwError = GetLastError();
		// Expected ERROR_INSUFFICIENT_BUFFER
		if (dwError == ERROR_INSUFFICIENT_BUFFER)
		{
			tkUser = (TOKEN_USER *) malloc(tkSize);
			if (tkUser != NULL)
			{
				// Now make the real call
				if (GetTokenInformation(tkHandle, TokenUser, tkUser, tkSize, &tkSize))
				{
					hr = CloneSID(ppSid, tkUser->User.Sid);

#ifdef _DEBUG					
					if (SUCCEEDED(hr))
					{
						ATLASSERT(IsValidSid(*ppSid));
					}
#endif
				}
				else
					hr = AtlHresultFromLastError();

				free (tkUser);
				tkUser = NULL;
			}
			else
				hr = E_OUTOFMEMORY;
		}
		else
		{
			hr = AtlHresultFromWin32(dwError);
			ATLASSERT(FALSE);
		}
		CloseHandle(tkHandle);
	}
	else
		hr = AtlHresultFromLastError();

	return hr;
}


inline HRESULT CSecurityDescriptor::GetPrincipalSID(LPCTSTR pszPrincipal, PSID *ppSid)
{
	if (ppSid == NULL)
		return E_POINTER;
	if (pszPrincipal == NULL)
		return E_INVALIDARG;
	*ppSid = NULL;

	HRESULT hr;
	LPTSTR pszRefDomain = NULL;
	DWORD dwDomainSize = 0;
	DWORD dwSidSize = 0;
	SID_NAME_USE snu;
	DWORD dwError;

	// Call to get size info for alloc
	LookupAccountName(NULL, pszPrincipal, NULL, &dwSidSize, NULL, &dwDomainSize, &snu);

	dwError = GetLastError();
	if (dwError == ERROR_INSUFFICIENT_BUFFER)
	{
		ATLTRY(pszRefDomain = new TCHAR[dwDomainSize]);
		if (pszRefDomain != NULL)
		{
			*ppSid = (PSID) malloc(dwSidSize);
			if (*ppSid != NULL)
			{
				if (LookupAccountName(NULL, pszPrincipal, *ppSid, &dwSidSize, pszRefDomain, &dwDomainSize, &snu))
				{
					hr = S_OK;
				}
				else
				{
					hr = AtlHresultFromLastError();
					free(*ppSid);
					*ppSid = NULL;
				}
			}
			else
				hr = E_OUTOFMEMORY;
			delete[] pszRefDomain;
		}
		else
			hr = E_OUTOFMEMORY;
	}
	else
		hr = AtlHresultFromWin32(dwError);

	return hr;
}

inline HRESULT CSecurityDescriptor::Attach(PSECURITY_DESCRIPTOR pSelfRelativeSD)
{
	PACL    pDACL = NULL;
	PACL    pSACL = NULL;
	BOOL    bDACLPresent, bSACLPresent;
	BOOL    bDefaulted;
	HRESULT hr;
	PSID    pUserSid;
	PSID    pGroupSid;
	
	if (pSelfRelativeSD == NULL || !IsValidSecurityDescriptor(pSelfRelativeSD))
		return E_INVALIDARG;
	
	hr = Initialize();
	if(FAILED(hr))
		return hr;

	// get the existing DACL.
	if (GetSecurityDescriptorDacl(pSelfRelativeSD, &bDACLPresent, &pDACL, &bDefaulted))
	{
		if (bDACLPresent)
		{
			// pDACL should be valid if bDACLPresent is true
			ATLASSERT(pDACL != NULL);
			// allocate new DACL.
			m_pDACL = (PACL) malloc(pDACL->AclSize);
			if (m_pDACL != NULL)
			{
				// initialize the DACL
				if (InitializeAcl(m_pDACL, pDACL->AclSize, ACL_REVISION))
				{
					// copy the ACL
					hr = CopyACL(m_pDACL, pDACL);
					if (SUCCEEDED(hr) && !IsValidAcl(m_pDACL))
						hr = E_FAIL;
				}
				else
					hr = AtlHresultFromLastError();
			}
			else
				hr = E_OUTOFMEMORY;
		}
		// set the DACL
		if (SUCCEEDED(hr) && !SetSecurityDescriptorDacl(m_pSD, bDACLPresent, m_pDACL, bDefaulted))
			hr = AtlHresultFromLastError();
	}
	else
		hr = AtlHresultFromLastError();

	// get the existing SACL.
	if (SUCCEEDED(hr) && GetSecurityDescriptorSacl(pSelfRelativeSD, &bSACLPresent, &pSACL, &bDefaulted))
	{
		if (bSACLPresent)
		{
			// pSACL should be valid if bSACLPresent is true
			ATLASSERT(pSACL != NULL);
			// allocate new SACL.
			m_pSACL = (PACL) malloc(pSACL->AclSize);
			if (m_pSACL != NULL)
			{
				// initialize the SACL
				if (InitializeAcl(m_pSACL, pSACL->AclSize, ACL_REVISION))
				{
					// copy the ACL
					hr = CopyACL(m_pSACL, pSACL);
					if (SUCCEEDED(hr) && !IsValidAcl(m_pSACL))
						hr = E_FAIL;
				}
				else
					hr = AtlHresultFromLastError();
			}
			else
				hr = E_OUTOFMEMORY;
		}
		// set the SACL
		if (SUCCEEDED(hr) && !SetSecurityDescriptorSacl(m_pSD, bSACLPresent, m_pSACL, bDefaulted))
			hr = AtlHresultFromLastError();
	}
	else if (SUCCEEDED(hr))
		hr = AtlHresultFromLastError();
	
	if (SUCCEEDED(hr))
	{
		if (GetSecurityDescriptorOwner(pSelfRelativeSD, &pUserSid, &bDefaulted))
		{
			if (SUCCEEDED(hr = SetOwner(pUserSid, bDefaulted)))
			{
				if (GetSecurityDescriptorGroup(pSelfRelativeSD, &pGroupSid, &bDefaulted))
				{
					if (SUCCEEDED(hr = SetGroup(pGroupSid, bDefaulted)))
					{
						if (!IsValidSecurityDescriptor(m_pSD))
							hr = E_FAIL;
					}
				}
				else
					hr = AtlHresultFromLastError();
			}
		}
		else
			hr = AtlHresultFromLastError();
	}

	if (FAILED(hr))
	{
		free(m_pDACL);
		m_pDACL = NULL;

		free(m_pSACL);
		m_pSACL = NULL;

		free(m_pSD);
		m_pSD = NULL;
	}

	return hr;
}

inline HRESULT CSecurityDescriptor::AttachObject(HANDLE hObject)
{
	HRESULT hr;
	DWORD dwError;
	DWORD dwSize = 0;
	PSECURITY_DESCRIPTOR pSD = NULL;

	GetKernelObjectSecurity(hObject, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
		DACL_SECURITY_INFORMATION, NULL, 0, &dwSize);

	dwError = GetLastError();
	if (dwError != ERROR_INSUFFICIENT_BUFFER)
		return AtlHresultFromWin32(dwError);

	pSD = (PSECURITY_DESCRIPTOR) malloc(dwSize);
	if (pSD != NULL)
	{
		if (GetKernelObjectSecurity(hObject, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
			DACL_SECURITY_INFORMATION, pSD, dwSize, &dwSize))

			hr = Attach(pSD);
		else
			hr = AtlHresultFromLastError();
		free(pSD);
	}
	else
		hr = E_OUTOFMEMORY;

	return hr;
}


inline HRESULT CSecurityDescriptor::CopyACL(PACL pDest, PACL pSrc)
{
	ACL_SIZE_INFORMATION aclSizeInfo;
	LPVOID pAce;
	ACE_HEADER *aceHeader;

	if (pDest == NULL)
		return E_POINTER;

	if (pSrc == NULL)
		return S_OK;

	if (!GetAclInformation(pSrc, (LPVOID) &aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
		return AtlHresultFromLastError();

	// Copy all of the ACEs to the new ACL
	for (UINT i = 0; i < aclSizeInfo.AceCount; i++)
	{
		if (!GetAce(pSrc, i, &pAce))
			return AtlHresultFromLastError();

		aceHeader = (ACE_HEADER *) pAce;

		if (!AddAce(pDest, ACL_REVISION, 0xffffffff, pAce, aceHeader->AceSize))
			return AtlHresultFromLastError();
	}

	return S_OK;
}

inline HRESULT CSecurityDescriptor::AddAccessDeniedACEToACL(PACL *ppAcl, PSID pSid, DWORD dwAccessMask)
{
	ACL_SIZE_INFORMATION aclSizeInfo;
	int aclSize;
	PACL oldACL, newACL = NULL;
	HRESULT hr = S_OK;

	if (ppAcl == NULL)
		return E_POINTER;

	oldACL = *ppAcl;
	
	if (pSid == NULL || !IsValidSid(pSid))
		return E_INVALIDARG;

	aclSizeInfo.AclBytesInUse = 0;
	if (*ppAcl != NULL && !GetAclInformation(oldACL, (LPVOID) &aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
		hr = AtlHresultFromLastError();

	if (SUCCEEDED(hr))
	{
		aclSize = aclSizeInfo.AclBytesInUse + sizeof(ACL) + sizeof(ACCESS_DENIED_ACE) + GetLengthSid(pSid) - sizeof(DWORD);
		newACL = (PACL) malloc(aclSize);
		if (newACL != NULL)
		{
			if (InitializeAcl(newACL, aclSize, ACL_REVISION))
			{
				// access denied ACEs should be before access allowed ACEs
				if (AddAccessDeniedAce(newACL, ACL_REVISION2, dwAccessMask, pSid))
				{
					// Copy existing ACEs to the new ACL
					hr = CopyACL(newACL, oldACL);
					if (SUCCEEDED(hr))
					{
						*ppAcl = newACL;
						free(oldACL);
					}
				}
				else
					hr = AtlHresultFromLastError();
			}
			else
				hr = AtlHresultFromLastError();

			if (FAILED(hr))
				free(newACL);
		}
		else
			hr = E_OUTOFMEMORY;
	}
	return hr;
}


inline HRESULT CSecurityDescriptor::AddAccessAllowedACEToACL(PACL *ppAcl, PSID pSid, DWORD dwAccessMask)
{
	ACL_SIZE_INFORMATION aclSizeInfo;
	int aclSize;
	HRESULT hr = S_OK;
	PACL oldACL, newACL = NULL;

	if (ppAcl == NULL)
		return E_POINTER;

	oldACL = *ppAcl;
	
	if (pSid == NULL || !IsValidSid(pSid))
		return E_INVALIDARG;

	aclSizeInfo.AclBytesInUse = 0;
	if (*ppAcl != NULL && !GetAclInformation(oldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
		hr = AtlHresultFromLastError();

	if (SUCCEEDED(hr))
	{
		aclSize = aclSizeInfo.AclBytesInUse + sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pSid) - sizeof(DWORD);
		newACL = (PACL) malloc(aclSize);
		if (newACL != NULL)
		{
			if (InitializeAcl(newACL, aclSize, ACL_REVISION))
			{
				// Copy existing ACEs
				hr = CopyACL(newACL, oldACL);
				if (SUCCEEDED(hr))
				{
					// Add access Allowed ACEs after all other existing ACEs (possibly access denied ACEs)
					if (AddAccessAllowedAce(newACL, ACL_REVISION2, dwAccessMask, pSid))
					{
						*ppAcl = newACL;
						free(oldACL);
					}
					else
						hr = AtlHresultFromLastError();
				}
			}
			else
				hr = AtlHresultFromLastError();

			if (FAILED(hr))
				free(newACL);
		}
		else
			hr = E_OUTOFMEMORY;
	}
	return hr;
}

inline HRESULT CSecurityDescriptor::RemovePrincipalFromACL(PACL pAcl, PSID principalSID)
{
	if (pAcl == NULL || principalSID == NULL || !IsValidSid(principalSID))
		return E_INVALIDARG;

	HRESULT hr = S_OK;
	ACL_SIZE_INFORMATION aclSizeInfo;
	if (!GetAclInformation(pAcl, (LPVOID) &aclSizeInfo, (DWORD) sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
	{
		hr = AtlHresultFromLastError();
		aclSizeInfo.AceCount = 0;
	}
		
	for (ULONG i = aclSizeInfo.AceCount; i > 0; i--)
	{
		ULONG uIndex = i - 1;
		LPVOID ace;	
		if (!GetAce(pAcl, uIndex, &ace))
		{
			hr = AtlHresultFromLastError();
			break;
		}

		ACE_HEADER *aceHeader = (ACE_HEADER *) ace;

		if (aceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE)
		{
			ACCESS_ALLOWED_ACE *accessAllowedAce = (ACCESS_ALLOWED_ACE *) ace;
			if (EqualSid(principalSID, (PSID) &accessAllowedAce->SidStart))
			{
				DeleteAce(pAcl, uIndex);
			}
		} 
		else if (aceHeader->AceType == ACCESS_DENIED_ACE_TYPE)
		{
			ACCESS_DENIED_ACE *accessDeniedAce = (ACCESS_DENIED_ACE *) ace;
			if (EqualSid(principalSID, (PSID) &accessDeniedAce->SidStart))
			{
				DeleteAce(pAcl, uIndex);
			}
		} 
		else if (aceHeader->AceType == SYSTEM_AUDIT_ACE_TYPE)
		{
			SYSTEM_AUDIT_ACE *systemAuditAce = (SYSTEM_AUDIT_ACE *) ace;
			if (EqualSid(principalSID, (PSID) &systemAuditAce->SidStart))
			{
				DeleteAce(pAcl, uIndex);
			}
		}
	}
	return hr;
}

inline HRESULT CSecurityDescriptor::SetPrivilege(LPCTSTR privilege, BOOL bEnable, HANDLE hToken)
{
	TOKEN_PRIVILEGES tpPrevious;
	TOKEN_PRIVILEGES tp;
	DWORD  cbPrevious = sizeof(TOKEN_PRIVILEGES);
	LUID   luid;
	HANDLE hTokenUsed;

	if (!LookupPrivilegeValue(NULL, privilege, &luid ))
		goto _Error;

	// if no token specified open process token
	if (hToken != 0)
		hTokenUsed = hToken;
	else
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hTokenUsed))
			goto _Error;

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = 0;

	if (!AdjustTokenPrivileges(hTokenUsed, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &tpPrevious, &cbPrevious))
		goto _Error_CloseHandle;

	tpPrevious.PrivilegeCount = 1;
	tpPrevious.Privileges[0].Luid = luid;

	if (bEnable)
		tpPrevious.Privileges[0].Attributes |= SE_PRIVILEGE_ENABLED;
	else
		tpPrevious.Privileges[0].Attributes &= ~SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hTokenUsed, FALSE, &tpPrevious, cbPrevious, NULL, NULL))
		goto _Error_CloseHandle;

	if(hToken == 0)
		CloseHandle(hTokenUsed);

	return S_OK;

	HRESULT hr;

_Error:
	hr = AtlHresultFromLastError();
	return hr;

_Error_CloseHandle:
	hr = AtlHresultFromLastError();
	if (hToken == 0)
		CloseHandle(hTokenUsed);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// COM Objects

#define DECLARE_PROTECT_FINAL_CONSTRUCT()\
	void InternalFinalConstructAddRef() {InternalAddRef();}\
	void InternalFinalConstructRelease() {InternalRelease();}

template <class T1>
class CComCreator
{
public:
	static HRESULT WINAPI CreateInstance(void* pv, REFIID riid, LPVOID* ppv)
	{
		ATLASSERT(ppv != NULL);
		if (ppv == NULL)
			return E_POINTER;
		*ppv = NULL;

		HRESULT hRes = E_OUTOFMEMORY;
		T1* p = NULL;
		ATLTRY(p = new T1(pv))
		if (p != NULL)
		{
			p->SetVoid(pv);
			p->InternalFinalConstructAddRef();
			hRes = p->FinalConstruct();
			if (SUCCEEDED(hRes))
				hRes = p->_AtlFinalConstruct();
			p->InternalFinalConstructRelease();
			if (hRes == S_OK)
				hRes = p->QueryInterface(riid, ppv);
			if (hRes != S_OK)
				delete p;
		}
		return hRes;
	}
};

template <class T1>
class CComInternalCreator
{
public:
	static HRESULT WINAPI CreateInstance(void* pv, REFIID riid, LPVOID* ppv)
	{
		ATLASSERT(ppv != NULL);
		if (ppv == NULL)
			return E_POINTER;
		*ppv = NULL;

		HRESULT hRes = E_OUTOFMEMORY;
		T1* p = NULL;
		ATLTRY(p = new T1(pv))
		if (p != NULL)
		{
			p->SetVoid(pv);
			p->InternalFinalConstructAddRef();
			hRes = p->FinalConstruct();
			if (SUCCEEDED(hRes))
				hRes = p->_AtlFinalConstruct();
			p->InternalFinalConstructRelease();
			if (hRes == S_OK)
				hRes = p->_InternalQueryInterface(riid, ppv);
			if (hRes != S_OK)
				delete p;
		}
		return hRes;
	}
};

template <HRESULT hr>
class CComFailCreator
{
public:
	static HRESULT WINAPI CreateInstance(void*, REFIID, LPVOID* ppv)
	{
		if (ppv == NULL)
			return E_POINTER;
		*ppv = NULL;

		return hr;
	}
};

template <class T1, class T2>
class CComCreator2
{
public:
	static HRESULT WINAPI CreateInstance(void* pv, REFIID riid, LPVOID* ppv)
	{
		ATLASSERT(ppv != NULL);

		return (pv == NULL) ? 
			T1::CreateInstance(NULL, riid, ppv) : 
			T2::CreateInstance(pv, riid, ppv);
	}
};

#define DECLARE_NOT_AGGREGATABLE(x) public:\
	typedef ATL::CComCreator2< ATL::CComCreator< ATL::CComObject< x > >, ATL::CComFailCreator<CLASS_E_NOAGGREGATION> > _CreatorClass;
#define DECLARE_AGGREGATABLE(x) public:\
	typedef ATL::CComCreator2< ATL::CComCreator< ATL::CComObject< x > >, ATL::CComCreator< ATL::CComAggObject< x > > > _CreatorClass;
#define DECLARE_ONLY_AGGREGATABLE(x) public:\
	typedef ATL::CComCreator2< ATL::CComFailCreator<E_FAIL>, ATL::CComCreator< ATL::CComAggObject< x > > > _CreatorClass;
#define DECLARE_POLY_AGGREGATABLE(x) public:\
	typedef ATL::CComCreator< ATL::CComPolyObject< x > > _CreatorClass;

struct _ATL_CREATORDATA
{
	_ATL_CREATORFUNC* pFunc;
};

template <class Creator>
class _CComCreatorData
{
public:
	static _ATL_CREATORDATA data;
};

template <class Creator>
_ATL_CREATORDATA _CComCreatorData<Creator>::data = {Creator::CreateInstance};

struct _ATL_CACHEDATA
{
	DWORD dwOffsetVar;
	_ATL_CREATORFUNC* pFunc;
};

template <class Creator, DWORD dwVar>
class _CComCacheData
{
public:
	static _ATL_CACHEDATA data;
};

template <class Creator, DWORD dwVar>
_ATL_CACHEDATA _CComCacheData<Creator, dwVar>::data = {dwVar, Creator::CreateInstance};

struct _ATL_CHAINDATA
{
	DWORD_PTR dwOffset;
	const _ATL_INTMAP_ENTRY* (WINAPI *pFunc)();
};

template <class base, class derived>
class _CComChainData
{
public:
	static _ATL_CHAINDATA data;
};

template <class base, class derived>
_ATL_CHAINDATA _CComChainData<base, derived>::data =
	{offsetofclass(base, derived), base::_GetEntries};

template <class T, const CLSID* pclsid>
class CComAggregateCreator
{
public:
	static HRESULT WINAPI CreateInstance(void* pv, REFIID/*riid*/, LPVOID* ppv) throw()
	{
		// Only Assert here. CoCreateInstance will return the correct HRESULT if ppv == NULL
		ATLASSERT(ppv != NULL && *ppv == NULL);

		ATLASSERT(pv != NULL);
		if (pv == NULL)
			return E_INVALIDARG;

		T* p = (T*) pv;
		// Add the following line to your object if you get a message about
		// GetControllingUnknown() being undefined
		// DECLARE_GET_CONTROLLING_UNKNOWN()
		return CoCreateInstance(*pclsid, p->GetControllingUnknown(), CLSCTX_INPROC, __uuidof(IUnknown), ppv);
	}
};

#ifdef _ATL_DEBUG
#define DEBUG_QI_ENTRY(x) \
		{NULL, \
		(DWORD_PTR)_T(#x), \
		(ATL::_ATL_CREATORARGFUNC*)0},
#else
#define DEBUG_QI_ENTRY(x)
#endif //_ATL_DEBUG

#ifdef _ATL_DEBUG_INTERFACES
#define _ATL_DECLARE_GET_UNKNOWN(x)\
	IUnknown* GetUnknown() throw() \
	{ \
		IUnknown* p; \
		_AtlDebugInterfacesModule.AddNonAddRefThunk(_GetRawUnknown(), _T(#x), &p); \
		return p; \
	}
#else
#define _ATL_DECLARE_GET_UNKNOWN(x) IUnknown* GetUnknown() throw() {return _GetRawUnknown();}
#endif

//If you get a message that FinalConstruct is ambiguous then you need to
// override it in your class and call each base class' version of this
#define BEGIN_COM_MAP(x) public: \
	typedef x _ComMapClass; \
	static HRESULT WINAPI _Cache(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw) throw()\
	{\
		_ComMapClass* p = (_ComMapClass*)pv;\
		p->Lock();\
		HRESULT hRes = E_FAIL; \
		__try \
		{ \
			hRes = CComObjectRootBase::_Cache(pv, iid, ppvObject, dw);\
		} \
		__finally \
		{ \
			p->Unlock();\
		} \
		return hRes;\
	}\
	IUnknown* _GetRawUnknown() throw() \
	{ ATLASSERT(_GetEntries()[0].pFunc == _ATL_SIMPLEMAPENTRY); return (IUnknown*)((INT_PTR)this+_GetEntries()->dw); } \
	_ATL_DECLARE_GET_UNKNOWN(x)\
	HRESULT _InternalQueryInterface(REFIID iid, void** ppvObject) throw() \
	{ return InternalQueryInterface(this, _GetEntries(), iid, ppvObject); } \
	const static ATL::_ATL_INTMAP_ENTRY* WINAPI _GetEntries() throw() { \
	static const ATL::_ATL_INTMAP_ENTRY _entries[] = { DEBUG_QI_ENTRY(x)

// For use by attributes for chaining to existing COM_MAP
#define BEGIN_ATTRCOM_MAP(x) public: \
	typedef x _AttrComMapClass; \
	const static ATL::_ATL_INTMAP_ENTRY* WINAPI _GetAttrEntries() throw() { \
	static const ATL::_ATL_INTMAP_ENTRY _entries[] = {

#define DECLARE_GET_CONTROLLING_UNKNOWN() public:\
	virtual IUnknown* GetControllingUnknown() throw() {return GetUnknown();}

#define COM_INTERFACE_ENTRY_BREAK(x)\
	{&_ATL_IIDOF(x), \
	NULL, \
	_Break},

#define COM_INTERFACE_ENTRY_NOINTERFACE(x)\
	{&_ATL_IIDOF(x), \
	NULL, \
	_NoInterface},

#define COM_INTERFACE_ENTRY(x)\
	{&_ATL_IIDOF(x), \
	offsetofclass(x, _ComMapClass), \
	_ATL_SIMPLEMAPENTRY},

#define COM_INTERFACE_ENTRY_IID(iid, x)\
	{&iid,\
	offsetofclass(x, _ComMapClass),\
	_ATL_SIMPLEMAPENTRY},

// The impl macros are now obsolete
#define COM_INTERFACE_ENTRY_IMPL(x)\
	COM_INTERFACE_ENTRY_IID(_ATL_IIDOF(x), x##Impl<_ComMapClass>)

#define COM_INTERFACE_ENTRY_IMPL_IID(iid, x)\
	COM_INTERFACE_ENTRY_IID(iid, x##Impl<_ComMapClass>)
//

#define COM_INTERFACE_ENTRY2(x, x2)\
	{&_ATL_IIDOF(x),\
	reinterpret_cast<DWORD_PTR>(static_cast<x*>(static_cast<x2*>(reinterpret_cast<_ComMapClass*>(8))))-8,\
	_ATL_SIMPLEMAPENTRY},

#define COM_INTERFACE_ENTRY2_IID(iid, x, x2)\
	{&iid,\
	reinterpret_cast<DWORD_PTR>(static_cast<x*>(static_cast<x2*>(reinterpret_cast<_ComMapClass*>(8))))-8,\
	_ATL_SIMPLEMAPENTRY},

#define COM_INTERFACE_ENTRY_FUNC(iid, dw, func)\
	{&iid, \
	dw, \
	func},

#define COM_INTERFACE_ENTRY_FUNC_BLIND(dw, func)\
	{NULL, \
	dw, \
	func},

#define COM_INTERFACE_ENTRY_TEAR_OFF(iid, x)\
	{&iid,\
	(DWORD_PTR)&ATL::_CComCreatorData<\
		ATL::CComInternalCreator< ATL::CComTearOffObject< x > >\
		>::data,\
	_Creator},

#define COM_INTERFACE_ENTRY_CACHED_TEAR_OFF(iid, x, punk)\
	{&iid,\
	(DWORD_PTR)&ATL::_CComCacheData<\
		ATL::CComCreator< ATL::CComCachedTearOffObject< x > >,\
		(DWORD_PTR)offsetof(_ComMapClass, punk)\
		>::data,\
	_Cache},

#define COM_INTERFACE_ENTRY_AGGREGATE(iid, punk)\
	{&iid,\
	(DWORD_PTR)offsetof(_ComMapClass, punk),\
	_Delegate},

#define COM_INTERFACE_ENTRY_AGGREGATE_BLIND(punk)\
	{NULL,\
	(DWORD_PTR)offsetof(_ComMapClass, punk),\
	_Delegate},

#define COM_INTERFACE_ENTRY_AUTOAGGREGATE(iid, punk, clsid)\
	{&iid,\
	(DWORD_PTR)&ATL::_CComCacheData<\
		ATL::CComAggregateCreator<_ComMapClass, &clsid>,\
		(DWORD_PTR)offsetof(_ComMapClass, punk)\
		>::data,\
	_Cache},

#define COM_INTERFACE_ENTRY_AUTOAGGREGATE_BLIND(punk, clsid)\
	{NULL,\
	(DWORD_PTR)&ATL::_CComCacheData<\
		ATL::CComAggregateCreator<_ComMapClass, &clsid>,\
		(DWORD_PTR)offsetof(_ComMapClass, punk)\
		>::data,\
	_Cache},

#define COM_INTERFACE_ENTRY_CHAIN(classname)\
	{NULL,\
	(DWORD_PTR)&ATL::_CComChainData<classname, _ComMapClass>::data,\
	_Chain},

#ifdef _ATL_DEBUG
#define END_COM_MAP() \
	__if_exists(_GetAttrEntries) {{NULL, (DWORD_PTR)_GetAttrEntries, _ChainAttr }, }\
	{NULL, 0, 0}}; return &_entries[1];} \
	virtual ULONG STDMETHODCALLTYPE AddRef( void) throw() = 0; \
	virtual ULONG STDMETHODCALLTYPE Release( void) throw() = 0; \
	STDMETHOD(QueryInterface)(REFIID, void**) throw() = 0;
#else
#define END_COM_MAP() \
	__if_exists(_GetAttrEntries) {{NULL, (DWORD_PTR)_GetAttrEntries, _ChainAttr }, }\
	{NULL, 0, 0}}; return _entries;} \
	virtual ULONG STDMETHODCALLTYPE AddRef( void) throw() = 0; \
	virtual ULONG STDMETHODCALLTYPE Release( void) throw() = 0; \
	STDMETHOD(QueryInterface)(REFIID, void**) throw() = 0;
#endif // _ATL_DEBUG

#define END_ATTRCOM_MAP() \
	{NULL, 0, 0}}; return _entries;} 


#define BEGIN_CATEGORY_MAP(x)\
	static const struct ATL::_ATL_CATMAP_ENTRY* GetCategoryMap() throw() {\
	static const struct ATL::_ATL_CATMAP_ENTRY pMap[] = {
#define IMPLEMENTED_CATEGORY( catid ) { _ATL_CATMAP_ENTRY_IMPLEMENTED, &catid },
#define REQUIRED_CATEGORY( catid ) { _ATL_CATMAP_ENTRY_REQUIRED, &catid },
#define END_CATEGORY_MAP()\
   { _ATL_CATMAP_ENTRY_END, NULL } };\
   return( pMap ); }

#define BEGIN_OBJECT_MAP(x) static ATL::_ATL_OBJMAP_ENTRY x[] = {
#define END_OBJECT_MAP()   {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}};
#define OBJECT_ENTRY(clsid, class) {&clsid, class::UpdateRegistry, class::_ClassFactoryCreatorClass::CreateInstance, class::_CreatorClass::CreateInstance, NULL, 0, class::GetObjectDescription, class::GetCategoryMap, class::ObjectMain },
#define OBJECT_ENTRY_NON_CREATEABLE(class) {&CLSID_NULL, class::UpdateRegistry, NULL, NULL, NULL, 0, NULL, class::GetCategoryMap, class::ObjectMain },
#define OBJECT_ENTRY_NON_CREATEABLE_EX(clsid, class) {&clsid, class::UpdateRegistry, NULL, NULL, NULL, 0, NULL, class::GetCategoryMap, class::ObjectMain },

#ifndef OBJECT_ENTRY_PRAGMA

#if defined(_M_IX86)
#define OBJECT_ENTRY_PRAGMA(class) __pragma(comment(linker, "/include:___pobjMap_" #class));
#elif defined(_M_IA64)
#define OBJECT_ENTRY_PRAGMA(class) __pragma(comment(linker, "/include:__pobjMap_" #class));
#else
#error Unknown Platform. define OBJECT_ENTRY_PRAGMA
#endif

#endif	//OBJECT_ENTRY_PRAGMA

#define OBJECT_ENTRY_AUTO(clsid, class) \
	__declspec(selectany) ATL::_ATL_OBJMAP_ENTRY __objMap_##class = {&clsid, class::UpdateRegistry, class::_ClassFactoryCreatorClass::CreateInstance, class::_CreatorClass::CreateInstance, NULL, 0, class::GetObjectDescription, class::GetCategoryMap, class::ObjectMain }; \
	extern "C" __declspec(allocate("ATL$__m")) __declspec(selectany) ATL::_ATL_OBJMAP_ENTRY* const __pobjMap_##class = &__objMap_##class; \
	OBJECT_ENTRY_PRAGMA(class)


#define OBJECT_ENTRY_NON_CREATEABLE_EX_AUTO(clsid, class) \
	__declspec(selectany) ATL::_ATL_OBJMAP_ENTRY __objMap_##class = {&clsid, class::UpdateRegistry, NULL, NULL, NULL, 0, NULL, class::GetCategoryMap, class::ObjectMain }; \
	extern "C" __declspec(allocate("ATL$__m")) __declspec(selectany) ATL::_ATL_OBJMAP_ENTRY* const __pobjMap_##class = &__objMap_##class; \
	OBJECT_ENTRY_PRAGMA(class)

// the functions in this class don't need to be virtual because
// they are called from CComObject
class CComObjectRootBase
{
public:
	CComObjectRootBase()
	{
		m_dwRef = 0L;
	}
	HRESULT FinalConstruct()
	{
		return S_OK;
	}
	// For library initialization only
	HRESULT _AtlFinalConstruct()
	{
		return S_OK;
	}
	void FinalRelease() {}
	void _AtlFinalRelease() {}		// temp

	void _HRPass(HRESULT hr)		// temp 
	{
		(hr);
	}

	void _HRFail(HRESULT hr)		// temp...
	{
		(hr);
	}

	//ObjectMain is called during Module::Init and Module::Term
	static void WINAPI ObjectMain(bool /* bStarting */) {}

	static const struct _ATL_CATMAP_ENTRY* GetCategoryMap() {return NULL;};
	static HRESULT WINAPI InternalQueryInterface(void* pThis,
		const _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject)
	{
		// Only Assert here. AtlInternalQueryInterface will return the correct HRESULT if ppvObject == NULL
#ifndef _ATL_OLEDB_CONFORMANCE_TESTS
		ATLASSERT(ppvObject != NULL);
#endif
		ATLASSERT(pThis != NULL);
		// First entry in the com map should be a simple map entry
		ATLASSERT(pEntries->pFunc == _ATL_SIMPLEMAPENTRY);
	#if defined(_ATL_DEBUG_INTERFACES) || defined(_ATL_DEBUG_QI)
		LPCTSTR pszClassName = (LPCTSTR) pEntries[-1].dw;
	#endif // _ATL_DEBUG_INTERFACES
		HRESULT hRes = AtlInternalQueryInterface(pThis, pEntries, iid, ppvObject);
	#ifdef _ATL_DEBUG_INTERFACES
		_AtlDebugInterfacesModule.AddThunk((IUnknown**)ppvObject, pszClassName, iid);
	#endif // _ATL_DEBUG_INTERFACES
		return _ATLDUMPIID(iid, pszClassName, hRes);
	}

//Outer funcs
	ULONG OuterAddRef()
	{
		return m_pOuterUnknown->AddRef();
	}
	ULONG OuterRelease()
	{
		return m_pOuterUnknown->Release();
	}
	HRESULT OuterQueryInterface(REFIID iid, void ** ppvObject)
	{
		return m_pOuterUnknown->QueryInterface(iid, ppvObject);
	}

	void SetVoid(void*) {}
	void InternalFinalConstructAddRef() {}
	void InternalFinalConstructRelease()
	{
		ATLASSERT(m_dwRef == 0);
	}
	// If this assert occurs, your object has probably been deleted
	// Try using DECLARE_PROTECT_FINAL_CONSTRUCT()


	static HRESULT WINAPI _Break(void* /* pv */, REFIID iid, void** /* ppvObject */, DWORD_PTR /* dw */)
	{
		(iid);
		_ATLDUMPIID(iid, _T("Break due to QI for interface "), S_OK);
		DebugBreak();
		return S_FALSE;
	}
	static HRESULT WINAPI _NoInterface(void* /* pv */, REFIID /* iid */, void** /* ppvObject */, DWORD_PTR /* dw */)
	{
		return E_NOINTERFACE;
	}
	static HRESULT WINAPI _Creator(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw)
	{
		_ATL_CREATORDATA* pcd = (_ATL_CREATORDATA*)dw;
		return pcd->pFunc(pv, iid, ppvObject);
	}
	static HRESULT WINAPI _Delegate(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw)
	{
		HRESULT hRes = E_NOINTERFACE;
		IUnknown* p = *(IUnknown**)((DWORD_PTR)pv + dw);
		if (p != NULL)
			hRes = p->QueryInterface(iid, ppvObject);
		return hRes;
	}
	static HRESULT WINAPI _Chain(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw)
	{
		_ATL_CHAINDATA* pcd = (_ATL_CHAINDATA*)dw;
		void* p = (void*)((DWORD_PTR)pv + pcd->dwOffset);
		return InternalQueryInterface(p, pcd->pFunc(), iid, ppvObject);
	}
	static HRESULT WINAPI _ChainAttr(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw)
	{
		const _ATL_INTMAP_ENTRY* (WINAPI *pFunc)() = (const _ATL_INTMAP_ENTRY* (WINAPI *)())dw;
		const _ATL_INTMAP_ENTRY *pEntries = pFunc();
		if (pEntries == NULL)
			return S_OK;
		return InternalQueryInterface(pv, pEntries, iid, ppvObject);
	}
	static HRESULT WINAPI _Cache(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw)
	{
		HRESULT hRes = E_NOINTERFACE;
		_ATL_CACHEDATA* pcd = (_ATL_CACHEDATA*)dw;
		IUnknown** pp = (IUnknown**)((DWORD_PTR)pv + pcd->dwOffsetVar);
		if (*pp == NULL)
			hRes = pcd->pFunc(pv, __uuidof(IUnknown), (void**)pp);
		if (*pp != NULL)
			hRes = (*pp)->QueryInterface(iid, ppvObject);
		return hRes;
	}

	union
	{
		long m_dwRef;
		IUnknown* m_pOuterUnknown;
	};
};

//foward declaration
template <class ThreadModel>
class CComObjectRootEx;

template <class ThreadModel>
class CComObjectLockT
{
public:
	CComObjectLockT(CComObjectRootEx<ThreadModel>* p)
	{
		if (p)
			p->Lock();
		m_p = p;
	}

	~CComObjectLockT()
	{
		if (m_p)
			m_p->Unlock();
	}
	CComObjectRootEx<ThreadModel>* m_p;
};

template <> class CComObjectLockT<CComSingleThreadModel>;

template <class ThreadModel>
class CComObjectRootEx : public CComObjectRootBase
{
public:
	typedef ThreadModel _ThreadModel;
	typedef typename _ThreadModel::AutoCriticalSection _CritSec;
	typedef CComObjectLockT<_ThreadModel> ObjectLock;

	ULONG InternalAddRef()
	{
		ATLASSERT(m_dwRef != -1L);
		return _ThreadModel::Increment(&m_dwRef);
	}
	ULONG InternalRelease()
	{
#ifdef _DEBUG
		LONG nRef = _ThreadModel::Decrement(&m_dwRef);
		if (nRef < -(LONG_MAX / 2))
		{
			ATLASSERT(0 && _T("Release called on a pointer that has already been released"));
		}
		return nRef;
#else
		return _ThreadModel::Decrement(&m_dwRef);
#endif
	}

	void Lock() {m_critsec.Lock();}
	void Unlock() {m_critsec.Unlock();}
private:
	_CritSec m_critsec;
};

template <>
class CComObjectRootEx<CComSingleThreadModel> : public CComObjectRootBase
{
public:
	typedef CComSingleThreadModel _ThreadModel;
	typedef _ThreadModel::AutoCriticalSection _CritSec;
	typedef CComObjectLockT<_ThreadModel> ObjectLock;

	ULONG InternalAddRef()
	{
		ATLASSERT(m_dwRef != -1L);
		return _ThreadModel::Increment(&m_dwRef);
	}
	ULONG InternalRelease()
	{
#ifdef _DEBUG
		long nRef = _ThreadModel::Decrement(&m_dwRef);
		if (nRef < -(LONG_MAX / 2))
		{
			ATLASSERT(0 && _T("Release called on a pointer that has already been released"));
		}
		return nRef;
#else
		return _ThreadModel::Decrement(&m_dwRef);
#endif	
	}

	void Lock() {}
	void Unlock() {}
};

template <>
class CComObjectLockT<CComSingleThreadModel>
{
public:
	CComObjectLockT(CComObjectRootEx<CComSingleThreadModel>*) {}
	~CComObjectLockT() {}
};

typedef CComObjectRootEx<CComObjectThreadModel> CComObjectRoot;

#if defined(_WINDLL) | defined(_USRDLL)
#define DECLARE_CLASSFACTORY_EX(cf) typedef ATL::CComCreator< ATL::CComObjectCached< cf > > _ClassFactoryCreatorClass;
#else
// don't let class factory refcount influence lock count
#define DECLARE_CLASSFACTORY_EX(cf) typedef ATL::CComCreator< ATL::CComObjectNoLock< cf > > _ClassFactoryCreatorClass;
#endif
#define DECLARE_CLASSFACTORY() DECLARE_CLASSFACTORY_EX(ATL::CComClassFactory)
#define DECLARE_CLASSFACTORY2(lic) DECLARE_CLASSFACTORY_EX(ATL::CComClassFactory2<lic>)
#define DECLARE_CLASSFACTORY_AUTO_THREAD() DECLARE_CLASSFACTORY_EX(ATL::CComClassFactoryAutoThread)
#define DECLARE_CLASSFACTORY_SINGLETON(obj) DECLARE_CLASSFACTORY_EX(ATL::CComClassFactorySingleton<obj>)

#define DECLARE_OBJECT_DESCRIPTION(x)\
	static LPCTSTR WINAPI GetObjectDescription() throw()\
	{\
		return _T(x);\
	}

#define DECLARE_NO_REGISTRY()\
	static HRESULT WINAPI UpdateRegistry(BOOL /*bRegister*/) throw()\
	{return S_OK;}

#define DECLARE_REGISTRY(class, pid, vpid, nid, flags)\
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister) throw()\
	{\
		return _Module.UpdateRegistryClass(GetObjectCLSID(), pid, vpid, nid,\
			flags, bRegister);\
	}

#define DECLARE_REGISTRY_RESOURCE(x)\
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister) throw()\
	{\
		__if_exists(_GetMiscStatus) \
		{ \
			ATL::_ATL_REGMAP_ENTRY regMapEntries[2]; \
			memset(&regMapEntries[1], 0, sizeof(ATL::_ATL_REGMAP_ENTRY)); \
			regMapEntries[0].szKey = L"OLEMISC"; \
			TCHAR szOleMisc[32]; \
			wsprintf(szOleMisc, _T("%d"), _GetMiscStatus()); \
			USES_CONVERSION_EX; \
			regMapEntries[0].szData = T2OLE_EX(szOleMisc, _ATL_SAFE_ALLOCA_DEF_THRESHOLD); \
			if (regMapEntries[0].szData == NULL) \
				return E_OUTOFMEMORY; \
			__if_exists(_Module) \
			{ \
				return _Module.UpdateRegistryFromResource(_T(#x), bRegister, regMapEntries); \
			} \
			__if_not_exists(_Module) \
			{ \
				return ATL::_pAtlModule->UpdateRegistryFromResource(_T(#x), bRegister, regMapEntries); \
			} \
		} \
		__if_not_exists(_GetMiscStatus) \
		{ \
			__if_exists(_Module) \
			{ \
				return _Module.UpdateRegistryFromResource(_T(#x), bRegister); \
			} \
			__if_not_exists(_Module) \
			{ \
				return ATL::_pAtlModule->UpdateRegistryFromResource(_T(#x), bRegister); \
			} \
		} \
	}

#define DECLARE_REGISTRY_RESOURCEID(x)\
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister) throw()\
	{\
		__if_exists(_GetMiscStatus) \
		{ \
			ATL::_ATL_REGMAP_ENTRY regMapEntries[2]; \
			memset(&regMapEntries[1], 0, sizeof(ATL::_ATL_REGMAP_ENTRY)); \
			regMapEntries[0].szKey = L"OLEMISC"; \
			TCHAR szOleMisc[32]; \
			wsprintf(szOleMisc, _T("%d"), _GetMiscStatus()); \
			USES_CONVERSION_EX; \
			regMapEntries[0].szData = T2OLE_EX(szOleMisc, _ATL_SAFE_ALLOCA_DEF_THRESHOLD); \
			if (regMapEntries[0].szData == NULL) \
				return E_OUTOFMEMORY; \
			__if_exists(_Module) \
			{ \
				return _Module.UpdateRegistryFromResource(x, bRegister, regMapEntries); \
			} \
			__if_not_exists(_Module) \
			{ \
				return ATL::_pAtlModule->UpdateRegistryFromResource(x, bRegister, regMapEntries); \
			} \
		} \
		__if_not_exists(_GetMiscStatus) \
		{ \
			__if_exists(_Module) \
			{ \
				return _Module.UpdateRegistryFromResource(x, bRegister); \
			} \
			__if_not_exists(_Module) \
			{ \
				return ATL::_pAtlModule->UpdateRegistryFromResource(x, bRegister); \
			} \
		} \
	}

//DECLARE_STATIC_* provided for backward compatibility
#ifdef _ATL_STATIC_REGISTRY
#define DECLARE_STATIC_REGISTRY_RESOURCE(x) DECLARE_REGISTRY_RESOURCE(x)
#define DECLARE_STATIC_REGISTRY_RESOURCEID(x) DECLARE_REGISTRY_RESOURCEID(x)
#endif //_ATL_STATIC_REGISTRY

#define DECLARE_OLEMISC_STATUS(x) \
	static DWORD _GetMiscStatus() throw() \
	{ \
		static DWORD m_dwOleMisc = x; \
		return m_dwOleMisc; \
	}

template<class Base> class CComObject; // fwd decl

template <class Owner, class ThreadModel = CComObjectThreadModel>
class CComTearOffObjectBase : public CComObjectRootEx<ThreadModel>
{
public:
	typedef Owner _OwnerClass;
	Owner* m_pOwner;
	CComTearOffObjectBase() {m_pOwner = NULL;}
};

//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class Base>
class CComObject : public Base
{
public:
	typedef Base _BaseClass;
	CComObject(void* = NULL) throw()
	{
		_pAtlModule->Lock();
	}
	// Set refcount to -(LONG_MAX/2) to protect destruction and 
	// also catch mismatched Release in debug builds
	virtual ~CComObject() throw()
	{
		m_dwRef = -(LONG_MAX/2);
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_AtlDebugInterfacesModule.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
		_pAtlModule->Unlock();
	}
	//If InternalAddRef or InternalRelease is undefined then your class
	//doesn't derive from CComObjectRoot
	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	//if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) throw()
	{return _InternalQueryInterface(iid, ppvObject);}
	template <class Q>
	HRESULT STDMETHODCALLTYPE QueryInterface(Q** pp) throw()
	{
		return QueryInterface(__uuidof(Q), (void**)pp);
	}

	static HRESULT WINAPI CreateInstance(CComObject<Base>** pp) throw();
};

template <class Base>
HRESULT WINAPI CComObject<Base>::CreateInstance(CComObject<Base>** pp) throw()
{
	ATLASSERT(pp != NULL);
	if (pp == NULL)
		return E_POINTER;
	*pp = NULL;

	HRESULT hRes = E_OUTOFMEMORY;
	CComObject<Base>* p = NULL;
	ATLTRY(p = new CComObject<Base>())
	if (p != NULL)
	{
		p->SetVoid(NULL);
		p->InternalFinalConstructAddRef();
		hRes = p->FinalConstruct();
		if (SUCCEEDED(hRes))
			hRes = p->_AtlFinalConstruct();
		p->InternalFinalConstructRelease();
		if (hRes != S_OK)
		{
			delete p;
			p = NULL;
		}
	}
	*pp = p;
	return hRes;
}

//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
// CComObjectCached is used primarily for class factories in DLL's
// but it is useful anytime you want to cache an object
template <class Base>
class CComObjectCached : public Base
{
public:
	typedef Base _BaseClass;
	CComObjectCached(void* = NULL){}
	// Set refcount to -(LONG_MAX/2) to protect destruction and 
	// also catch mismatched Release in debug builds
	virtual ~CComObjectCached()
	{
		m_dwRef = -(LONG_MAX/2);
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_AtlDebugInterfacesModule.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
	}
	//If InternalAddRef or InternalRelease is undefined then your class
	//doesn't derive from CComObjectRoot
	STDMETHOD_(ULONG, AddRef)() throw()
	{
		m_csCached.Lock();
		ULONG l = InternalAddRef();
		if (m_dwRef == 2)
			_pAtlModule->Lock();
		m_csCached.Unlock();
		return l;
	}
	STDMETHOD_(ULONG, Release)() throw()
	{
		m_csCached.Lock();
		InternalRelease();
		ULONG l = m_dwRef;
		m_csCached.Unlock();
		if (l == 0)
			delete this;
		else if (l == 1)
			_pAtlModule->Unlock();
		return l;
	}
	//if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) throw()
	{return _InternalQueryInterface(iid, ppvObject);}
	CComGlobalsThreadModel::AutoCriticalSection m_csCached;
	static HRESULT WINAPI CreateInstance(CComObjectCached<Base>** pp) throw();
};

template <class Base>
HRESULT WINAPI CComObjectCached<Base>::CreateInstance(CComObjectCached<Base>** pp) throw()
{
	ATLASSERT(pp != NULL);
	if (pp == NULL)
		return E_POINTER;
	*pp = NULL;

	HRESULT hRes = E_OUTOFMEMORY;
	CComObjectCached<Base>* p = NULL;
	ATLTRY(p = new CComObjectCached<Base>())
	if (p != NULL)
	{
		p->SetVoid(NULL);
		p->InternalFinalConstructAddRef();
		hRes = p->FinalConstruct();
		if (SUCCEEDED(hRes))
			hRes = p->_AtlFinalConstruct();
		p->InternalFinalConstructRelease();
		if (hRes != S_OK)
		{
			delete p;
			p = NULL;
		}
	}
	*pp = p;
	return hRes;
}


//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class Base>
class CComObjectNoLock : public Base
{
public:
	typedef Base _BaseClass;
	CComObjectNoLock(void* = NULL){}
	// Set refcount to -(LONG_MAX/2) to protect destruction and 
	// also catch mismatched Release in debug builds
	virtual ~CComObjectNoLock()
	{
		m_dwRef = -(LONG_MAX/2);
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_AtlDebugInterfacesModule.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
	}

	//If InternalAddRef or InternalRelease is undefined then your class
	//doesn't derive from CComObjectRoot
	STDMETHOD_(ULONG, AddRef)() throw() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)() throw()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	//if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) throw()
	{return _InternalQueryInterface(iid, ppvObject);}
};

// It is possible for Base not to derive from CComObjectRoot
// However, you will need to provide _InternalQueryInterface
template <class Base>
class CComObjectGlobal : public Base
{
public:
	typedef Base _BaseClass;
	CComObjectGlobal(void* = NULL)
	{
		m_hResFinalConstruct = S_OK;
		__if_exists(FinalConstruct)
		{
			__if_exists(InternalFinalConstructAddRef)
			{
				InternalFinalConstructAddRef();
			}
			m_hResFinalConstruct = FinalConstruct();
			__if_exists(InternalFinalConstructRelease)
			{
				InternalFinalConstructRelease();
			}
		}
	}
	virtual ~CComObjectGlobal()
	{
		__if_exists(FinalRelease)
		{
			FinalRelease();
		}
#ifdef _ATL_DEBUG_INTERFACES
		_AtlDebugInterfacesModule.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
	}

	STDMETHOD_(ULONG, AddRef)() throw()
	{
		return _pAtlModule->Lock();
	}
	STDMETHOD_(ULONG, Release)() throw()
	{
		return _pAtlModule->Unlock();
	}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) throw()
	{
		return _InternalQueryInterface(iid, ppvObject);
	}
	HRESULT m_hResFinalConstruct;
};

// It is possible for Base not to derive from CComObjectRoot
// However, you will need to provide FinalConstruct and InternalQueryInterface
template <class Base>
class CComObjectStack : public Base
{
public:
	typedef Base _BaseClass;
	CComObjectStack(void* = NULL){m_hResFinalConstruct = FinalConstruct();}
	virtual ~CComObjectStack()
	{
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_AtlDebugInterfacesModule.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
	}


	STDMETHOD_(ULONG, AddRef)() {ATLASSERT(FALSE);return 0;}
	STDMETHOD_(ULONG, Release)(){ATLASSERT(FALSE);return 0;}
	STDMETHOD(QueryInterface)(REFIID, void**)
	{ATLASSERT(FALSE);return E_NOINTERFACE;}
	HRESULT m_hResFinalConstruct;
};

// Base must be derived from CComObjectRoot
template <class Base>
class CComObjectStackEx : public Base
{
public:
	typedef Base _BaseClass;

	CComObjectStackEx(void* = NULL) 
	{ 
#ifdef _DEBUG
		m_dwRef = 0;
#endif

		m_hResFinalConstruct = FinalConstruct(); 
	}

	virtual ~CComObjectStackEx()
	{
		// This assert indicates mismatched ref counts.
		//
		// The ref count has no control over the
		// lifetime of this object, so you must ensure
		// by some other means that the object remains 
		// alive while clients have references to its interfaces.
		ATLASSERT(m_dwRef == 0);
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_AtlDebugInterfacesModule.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
	}

	STDMETHOD_(ULONG, AddRef)()
	{
#ifdef _DEBUG
		return InternalAddRef();
#else
		return 0;
#endif
	}

	STDMETHOD_(ULONG, Release)()
	{
#ifdef _DEBUG
		return InternalRelease();
#else
		return 0;
#endif
	}

	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{
		return _InternalQueryInterface(iid, ppvObject);
	}

	HRESULT m_hResFinalConstruct;
};

template <class Base> //Base must be derived from CComObjectRoot
class CComContainedObject : public Base
{
public:
	typedef Base _BaseClass;
	CComContainedObject(void* pv) {m_pOuterUnknown = (IUnknown*)pv;}
#ifdef _ATL_DEBUG_INTERFACES
	virtual ~CComContainedObject()
	{
		_AtlDebugInterfacesModule.DeleteNonAddRefThunk(_GetRawUnknown());
		_AtlDebugInterfacesModule.DeleteNonAddRefThunk(m_pOuterUnknown);
	}
#endif

	STDMETHOD_(ULONG, AddRef)() throw() {return OuterAddRef();}
	STDMETHOD_(ULONG, Release)() throw() {return OuterRelease();}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) throw()
	{
		return OuterQueryInterface(iid, ppvObject);
	}
	template <class Q>
	HRESULT STDMETHODCALLTYPE QueryInterface(Q** pp)
	{
		return QueryInterface(__uuidof(Q), (void**)pp);
	}
	//GetControllingUnknown may be virtual if the Base class has declared
	//DECLARE_GET_CONTROLLING_UNKNOWN()
	IUnknown* GetControllingUnknown() throw()
	{
#ifdef _ATL_DEBUG_INTERFACES
		IUnknown* p;
		_AtlDebugInterfacesModule.AddNonAddRefThunk(m_pOuterUnknown, _T("CComContainedObject"), &p);
		return p;
#else
		return m_pOuterUnknown;
#endif
	}
};

//contained is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class contained>
class CComAggObject :
	public IUnknown,
	public CComObjectRootEx< typename contained::_ThreadModel::ThreadModelNoCS >
{
public:
	typedef contained _BaseClass;
	CComAggObject(void* pv) : m_contained(pv)
	{
		_pAtlModule->Lock();
	}
	//If you get a message that this call is ambiguous then you need to
	// override it in your class and call each base class' version of this
	HRESULT FinalConstruct()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalConstruct();
		return m_contained.FinalConstruct();
	}
	void FinalRelease()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalRelease();
		m_contained.FinalRelease();
	}
	// Set refcount to -(LONG_MAX/2) to protect destruction and 
	// also catch mismatched Release in debug builds
	virtual ~CComAggObject()
	{
		m_dwRef = -(LONG_MAX/2);
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_AtlDebugInterfacesModule.DeleteNonAddRefThunk(this);
#endif
		_pAtlModule->Unlock();
	}

	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{
		ATLASSERT(ppvObject != NULL);
		if (ppvObject == NULL)
			return E_POINTER;
		*ppvObject = NULL;

		HRESULT hRes = S_OK;
		if (InlineIsEqualUnknown(iid))
		{
			*ppvObject = (void*)(IUnknown*)this;
			AddRef();
#ifdef _ATL_DEBUG_INTERFACES
			_AtlDebugInterfacesModule.AddThunk((IUnknown**)ppvObject, (LPCTSTR)contained::_GetEntries()[-1].dw, iid);
#endif // _ATL_DEBUG_INTERFACES
		}
		else
			hRes = m_contained._InternalQueryInterface(iid, ppvObject);
		return hRes;
	}
	template <class Q>
	HRESULT STDMETHODCALLTYPE QueryInterface(Q** pp)
	{
		return QueryInterface(__uuidof(Q), (void**)pp);
	}
	static HRESULT WINAPI CreateInstance(LPUNKNOWN pUnkOuter, CComAggObject<contained>** pp)
	{
		ATLASSERT(pp != NULL);
		if (pp == NULL)
			return E_POINTER;
		*pp = NULL;

		HRESULT hRes = E_OUTOFMEMORY;
		CComAggObject<contained>* p = NULL;
		ATLTRY(p = new CComAggObject<contained>(pUnkOuter))
		if (p != NULL)
		{
			p->SetVoid(NULL);
			p->InternalFinalConstructAddRef();
			hRes = p->FinalConstruct();
			if (SUCCEEDED(hRes))
				hRes = p->_AtlFinalConstruct();
			p->InternalFinalConstructRelease();
			if (hRes != S_OK)
			{
				delete p;
				p = NULL;
			}
		}
		*pp = p;
		return hRes;
	}

	CComContainedObject<contained> m_contained;
};

///////////////////////////////////////////////////////////////////////////////
// CComPolyObject can be either aggregated or not aggregated

template <class contained>
class CComPolyObject :
	public IUnknown,
	public CComObjectRootEx< typename contained::_ThreadModel::ThreadModelNoCS >
{
public:
	typedef contained _BaseClass;
	CComPolyObject(void* pv) : m_contained(pv ? pv : this)
	{
		_pAtlModule->Lock();
	}
	//If you get a message that this call is ambiguous then you need to
	// override it in your class and call each base class' version of this
	HRESULT FinalConstruct()
	{
		InternalAddRef();
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalConstruct();
		HRESULT hr = m_contained.FinalConstruct();
		InternalRelease();
		return hr;
	}
	void FinalRelease()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalRelease();
		m_contained.FinalRelease();
	}
	// Set refcount to -(LONG_MAX/2) to protect destruction and 
	// also catch mismatched Release in debug builds
	virtual ~CComPolyObject()
	{
		m_dwRef = -(LONG_MAX/2);
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_AtlDebugInterfacesModule.DeleteNonAddRefThunk(this);
#endif
		_pAtlModule->Unlock();
	}

	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{
#ifndef _ATL_OLEDB_CONFORMANCE_TESTS
		ATLASSERT(ppvObject != NULL);
#endif
		if (ppvObject == NULL)
			return E_POINTER;
		*ppvObject = NULL;

		HRESULT hRes = S_OK;
		if (InlineIsEqualUnknown(iid))
		{
			*ppvObject = (void*)(IUnknown*)this;
			AddRef();
#ifdef _ATL_DEBUG_INTERFACES
			_AtlDebugInterfacesModule.AddThunk((IUnknown**)ppvObject, (LPCTSTR)contained::_GetEntries()[-1].dw, iid);
#endif // _ATL_DEBUG_INTERFACES
		}
		else
			hRes = m_contained._InternalQueryInterface(iid, ppvObject);
		return hRes;
	}
	template <class Q>
	HRESULT STDMETHODCALLTYPE QueryInterface(Q** pp)
	{
		return QueryInterface(__uuidof(Q), (void**)pp);
	}
	static HRESULT WINAPI CreateInstance(LPUNKNOWN pUnkOuter, CComPolyObject<contained>** pp)
	{
		ATLASSERT(pp != NULL);
		if (pp == NULL)
			return E_POINTER;
		*pp = NULL;


		HRESULT hRes = E_OUTOFMEMORY;
		CComPolyObject<contained>* p = NULL;
		ATLTRY(p = new CComPolyObject<contained>(pUnkOuter))
		if (p != NULL)
		{
			p->SetVoid(NULL);
			p->InternalFinalConstructAddRef();
			hRes = p->FinalConstruct();
			if (SUCCEEDED(hRes))
				hRes = p->_AtlFinalConstruct();
			p->InternalFinalConstructRelease();
			if (hRes != S_OK)
			{
				delete p;
				p = NULL;
			}
		}
		*pp = p;
		return hRes;
	}

	CComContainedObject<contained> m_contained;
};

template <class Base>
class CComTearOffObject : public Base
{
public:
	CComTearOffObject(void* pv)
	{
		ATLASSERT(m_pOwner == NULL);
		m_pOwner = reinterpret_cast<Base::_OwnerClass*>(pv);
		m_pOwner->AddRef();
	}
	// Set refcount to -(LONG_MAX/2) to protect destruction and 
	// also catch mismatched Release in debug builds
	~CComTearOffObject()
	{
		m_dwRef = -(LONG_MAX/2);
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_AtlDebugInterfacesModule.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
		m_pOwner->Release();
	}

	STDMETHOD_(ULONG, AddRef)() throw() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)() throw()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) throw()
	{
		return m_pOwner->QueryInterface(iid, ppvObject);
	}
};

template <class contained>
class CComCachedTearOffObject :
	public IUnknown,
	public CComObjectRootEx<typename contained::_ThreadModel::ThreadModelNoCS>
{
public:
	typedef contained _BaseClass;
	CComCachedTearOffObject(void* pv) :
		m_contained(((contained::_OwnerClass*)pv)->GetControllingUnknown())
	{
		ATLASSERT(m_contained.m_pOwner == NULL);
		m_contained.m_pOwner = reinterpret_cast<contained::_OwnerClass*>(pv);
	}
	//If you get a message that this call is ambiguous then you need to
	// override it in your class and call each base class' version of this
	HRESULT FinalConstruct()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalConstruct();
		return m_contained.FinalConstruct();
	}
	void FinalRelease()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalRelease();
		m_contained.FinalRelease();
	}
	// Set refcount to -(LONG_MAX/2) to protect destruction and 
	// also catch mismatched Release in debug builds
	~CComCachedTearOffObject()
	{
		m_dwRef = -(LONG_MAX/2);
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_AtlDebugInterfacesModule.DeleteNonAddRefThunk(this);
#endif
	}


	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{
		ATLASSERT(ppvObject != NULL);
		if (ppvObject == NULL)
			return E_POINTER;
		*ppvObject = NULL;

		HRESULT hRes = S_OK;
		if (InlineIsEqualUnknown(iid))
		{
			*ppvObject = (void*)(IUnknown*)this;
			AddRef();
#ifdef _ATL_DEBUG_INTERFACES
			_AtlDebugInterfacesModule.AddThunk((IUnknown**)ppvObject, (LPCTSTR)contained::_GetEntries()[-1].dw, iid);
#endif // _ATL_DEBUG_INTERFACES
		}
		else
			hRes = m_contained._InternalQueryInterface(iid, ppvObject);
		return hRes;
	}
	CComContainedObject<contained> m_contained;
};

class CComClassFactory :
	public IClassFactory,
	public CComObjectRootEx<CComGlobalsThreadModel>
{
public:
	BEGIN_COM_MAP(CComClassFactory)
		COM_INTERFACE_ENTRY(IClassFactory)
	END_COM_MAP()

	virtual ~CComClassFactory()
	{
	}
	
	// IClassFactory
	STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj)
	{
		ATLASSERT(m_pfnCreateInstance != NULL);
		HRESULT hRes = E_POINTER;
		if (ppvObj != NULL)
		{
			*ppvObj = NULL;
			// can't ask for anything other than IUnknown when aggregating

			if ((pUnkOuter != NULL) && !InlineIsEqualUnknown(riid))
			{
				ATLTRACE(atlTraceCOM, 0, _T("CComClassFactory: asked for non IUnknown interface while creating an aggregated object"));
				hRes = CLASS_E_NOAGGREGATION;
			}
			else
				hRes = m_pfnCreateInstance(pUnkOuter, riid, ppvObj);
		}
		return hRes;
	}

	STDMETHOD(LockServer)(BOOL fLock)
	{
		if (fLock)
			_pAtlModule->Lock();
		else
			_pAtlModule->Unlock();
		return S_OK;
	}
	// helper
	void SetVoid(void* pv)
	{
		m_pfnCreateInstance = (_ATL_CREATORFUNC*)pv;
	}
	_ATL_CREATORFUNC* m_pfnCreateInstance;
};

template <class license>
class CComClassFactory2 : 
	public IClassFactory2,
	public CComObjectRootEx<CComGlobalsThreadModel>,
	public license
{
public:
	typedef license _LicenseClass;
	typedef CComClassFactory2<license> _ComMapClass;
BEGIN_COM_MAP(CComClassFactory2<license>)
	COM_INTERFACE_ENTRY(IClassFactory)
	COM_INTERFACE_ENTRY(IClassFactory2)
END_COM_MAP()
	// IClassFactory
	STDMETHOD(LockServer)(BOOL fLock)
	{
		if (fLock)
			_pAtlModule->Lock();
		else
			_pAtlModule->Unlock();
		return S_OK;
	}
	STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
		REFIID riid, void** ppvObj)
	{
		ATLASSERT(m_pfnCreateInstance != NULL);
		if (ppvObj == NULL)
			return E_POINTER;
		*ppvObj = NULL;
		if (!IsLicenseValid())
			return CLASS_E_NOTLICENSED;

		if ((pUnkOuter != NULL) && !InlineIsEqualUnknown(riid))
			return CLASS_E_NOAGGREGATION;
		else
			return m_pfnCreateInstance(pUnkOuter, riid, ppvObj);
	}
	// IClassFactory2
	STDMETHOD(CreateInstanceLic)(IUnknown* pUnkOuter, 
	  IUnknown* /* pUnkReserved */, REFIID riid, BSTR bstrKey, 
	  void** ppvObject)
	{
		ATLASSERT(m_pfnCreateInstance != NULL);
		if (ppvObject == NULL)
			return E_POINTER;
		*ppvObject = NULL;
		if ( ((bstrKey != NULL) && !VerifyLicenseKey(bstrKey)) ||
			 ((bstrKey == NULL) && !IsLicenseValid()) )
			return CLASS_E_NOTLICENSED;
		if ((pUnkOuter != NULL) && !InlineIsEqualUnknown(riid))
			return CLASS_E_NOAGGREGATION;
		else
			return m_pfnCreateInstance(pUnkOuter, riid, ppvObject);
	}
	STDMETHOD(RequestLicKey)(DWORD dwReserved, BSTR* pbstrKey)
	{
		if (pbstrKey == NULL)
			return E_POINTER;
		*pbstrKey = NULL;

		if (!IsLicenseValid())
			return CLASS_E_NOTLICENSED;
		return GetLicenseKey(dwReserved,pbstrKey) ? S_OK : E_FAIL;
	}
	STDMETHOD(GetLicInfo)(LICINFO* pLicInfo)
	{
		if (pLicInfo == NULL)
			return E_POINTER;
		pLicInfo->cbLicInfo = sizeof(LICINFO);
		pLicInfo->fLicVerified = IsLicenseValid();
		BSTR bstr = NULL;
		pLicInfo->fRuntimeKeyAvail = GetLicenseKey(0,&bstr);
		::SysFreeString(bstr);
		return S_OK;
	}
	void SetVoid(void* pv)
	{
		m_pfnCreateInstance = (_ATL_CREATORFUNC*)pv;
	}
	_ATL_CREATORFUNC* m_pfnCreateInstance;
};

/////////////////////////////////////////////////////////////////////////////////////////////
// Thread Pooling class factory
class CComClassFactoryAutoThread :
	public IClassFactory,
	public CComObjectRootEx<CComGlobalsThreadModel>
{
public:
	BEGIN_COM_MAP(CComClassFactoryAutoThread)
		COM_INTERFACE_ENTRY(IClassFactory)
	END_COM_MAP()

	virtual ~CComClassFactoryAutoThread()
	{
	}
	
	// helper
	void SetVoid(void* pv)
	{
		m_pfnCreateInstance = (_ATL_CREATORFUNC*)pv;
	}
	STDMETHODIMP CreateInstance(LPUNKNOWN pUnkOuter,
		REFIID riid, void** ppvObj)
	{
		ATLASSERT(m_pfnCreateInstance != NULL);
		HRESULT hRes = E_POINTER;
		if (ppvObj != NULL)
		{
			*ppvObj = NULL;
			// cannot aggregate across apartments
			ATLASSERT(pUnkOuter == NULL);
			if (pUnkOuter != NULL)
				hRes = CLASS_E_NOAGGREGATION;
			else
			{
				ATLASSERT(_pAtlAutoThreadModule && _T("Global instance of CAtlAutoThreadModule not declared"));
				if (_pAtlAutoThreadModule == NULL)
					return E_FAIL;

				hRes = _pAtlAutoThreadModule->CreateInstance(m_pfnCreateInstance, riid, ppvObj);
			}
		}
		return hRes;
	}
	STDMETHODIMP LockServer(BOOL fLock)
	{
		if (fLock)
			_pAtlModule->Lock();
		else
			_pAtlModule->Unlock();
		return S_OK;
	}
	_ATL_CREATORFUNC* m_pfnCreateInstance;
};


/////////////////////////////////////////////////////////////////////////////////////////////
// Singleton Class Factory
template <class T>
class CComClassFactorySingleton : public CComClassFactory
{
public:
	CComClassFactorySingleton() : m_hrCreate(S_OK)
	{
	}
	virtual ~CComClassFactorySingleton()
	{
	}
	// IClassFactory
	STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj)
	{
		HRESULT hRes = E_POINTER;
		if (ppvObj != NULL)
		{
			*ppvObj = NULL;
			// aggregation is not supported in Singletons
			ATLASSERT(pUnkOuter == NULL);
			if (pUnkOuter != NULL)
				hRes = CLASS_E_NOAGGREGATION;
			else
			{
				if (m_hrCreate == S_OK && m_spObj == NULL)
				{
					Lock();
					// Did another thread got here first
					if (m_hrCreate == S_OK && m_spObj == NULL)
					{
						__try
						{
							CComObjectCached<T> *p;
							m_hrCreate = CComObjectCached<T>::CreateInstance(&p);
							if (SUCCEEDED(m_hrCreate))
							{
								m_hrCreate = p->QueryInterface(IID_IUnknown, (void**)&m_spObj);
								if (FAILED(m_hrCreate))
								{
									delete p;
								}
							}
						}
						__finally
						{
							Unlock();
						}
					}
				}
				if (m_hrCreate == S_OK)
				{
					hRes = m_spObj->QueryInterface(riid, ppvObj);
				}
				else
				{
					hRes = m_hrCreate;
				}
			}
		}
		return hRes;
	}
	HRESULT m_hrCreate;
	CComPtr<IUnknown> m_spObj;
};

template <class T, const CLSID* pclsid = &CLSID_NULL>
class CComCoClass
{
public:
	DECLARE_CLASSFACTORY()
	DECLARE_AGGREGATABLE(T)
	typedef T _CoClass;
	static const CLSID& WINAPI GetObjectCLSID() {return *pclsid;}
	static LPCTSTR WINAPI GetObjectDescription() {return NULL;}
	static HRESULT WINAPI Error(LPCOLESTR lpszDesc,
		const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{
		return AtlReportError(GetObjectCLSID(), lpszDesc, iid, hRes);
	}
	static HRESULT WINAPI Error(LPCOLESTR lpszDesc, DWORD dwHelpID,
		LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{
		return AtlReportError(GetObjectCLSID(), lpszDesc, dwHelpID, lpszHelpFile,
			iid, hRes);
	}
	static HRESULT WINAPI Error(UINT nID, const IID& iid = GUID_NULL,
		HRESULT hRes = 0, HINSTANCE hInst = _AtlBaseModule.GetResourceInstance())
	{
		return AtlReportError(GetObjectCLSID(), nID, iid, hRes, hInst);
	}
	static HRESULT WINAPI Error(UINT nID, DWORD dwHelpID,
		LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL,
		HRESULT hRes = 0, HINSTANCE hInst = _AtlBaseModule.GetResourceInstance())
	{
		return AtlReportError(GetObjectCLSID(), nID, dwHelpID, lpszHelpFile,
			iid, hRes, hInst);
	}
	static HRESULT WINAPI Error(LPCSTR lpszDesc,
		const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{
		return AtlReportError(GetObjectCLSID(), lpszDesc, iid, hRes);
	}
	static HRESULT WINAPI Error(LPCSTR lpszDesc, DWORD dwHelpID,
		LPCSTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{
		return AtlReportError(GetObjectCLSID(), lpszDesc, dwHelpID,
			lpszHelpFile, iid, hRes);
	}
	template <class Q>
	static HRESULT CreateInstance(IUnknown* punkOuter, Q** pp)
	{
		return T::_CreatorClass::CreateInstance(punkOuter, __uuidof(Q), (void**) pp);
	}
	template <class Q>
	static HRESULT CreateInstance(Q** pp)
	{
		return T::_CreatorClass::CreateInstance(NULL, __uuidof(Q), (void**) pp);
	}
};

// ATL doesn't support multiple LCID's at the same time
// Whatever LCID is queried for first is the one that is used.
class CComTypeInfoHolder
{
// Should be 'protected' but can cause compiler to generate fat code.
public:
	const GUID* m_pguid;
	const GUID* m_plibid;
	WORD m_wMajor;
	WORD m_wMinor;

	ITypeInfo* m_pInfo;
	long m_dwRef;
	struct stringdispid
	{
		CComBSTR bstr;
		int nLen;
		DISPID id;
		stringdispid() : nLen(0), id(DISPID_UNKNOWN){}
	};
	stringdispid* m_pMap;
	int m_nCount;

public:

#ifdef _ATL_DLL_IMPL
	CComTypeInfoHolder(const GUID* pguid, const GUID* plibid, WORD wMajor, WORD wMinor) :
		m_pguid(pguid), m_plibid(plibid), m_wMajor(wMajor), m_wMinor(wMinor), 
		m_pInfo(NULL), m_dwRef(0), m_pMap(NULL), m_nCount(0)
	{
	}

	~CComTypeInfoHolder()
	{
		if (m_pInfo != NULL)
			m_pInfo->Release();
		m_pInfo = NULL;
		delete [] m_pMap;
		m_pMap = NULL;
	}
#endif

	HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo)
	{
		ATLASSERT(ppInfo != NULL);
		if (ppInfo == NULL)
			return E_POINTER;

		HRESULT hr = S_OK;
		if (m_pInfo == NULL)
			hr = GetTI(lcid);
		*ppInfo = m_pInfo;
		if (m_pInfo != NULL)
		{
			m_pInfo->AddRef();
			hr = S_OK;
		}
		return hr;
	}
	HRESULT GetTI(LCID lcid);
	HRESULT EnsureTI(LCID lcid)
	{
		HRESULT hr = S_OK;
		if (m_pInfo == NULL || m_pMap == NULL)
			hr = GetTI(lcid);
		return hr;
	}

	// This function is called by the module on exit
	// It is registered through _pAtlModule->AddTermFunc()
	static void __stdcall Cleanup(DWORD_PTR dw)
	{
		ATLASSERT(dw != 0);
		if (dw == 0)
			return;
			
		CComTypeInfoHolder* p = (CComTypeInfoHolder*) dw;
		if (p->m_pInfo != NULL)
			p->m_pInfo->Release();
		p->m_pInfo = NULL;
		delete [] p->m_pMap;
		p->m_pMap = NULL;
	}

	HRESULT GetTypeInfo(UINT /* itinfo */, LCID lcid, ITypeInfo** pptinfo)
	{
		return GetTI(lcid, pptinfo);
	}
	HRESULT GetIDsOfNames(REFIID /* riid */, LPOLESTR* rgszNames, UINT cNames,
		LCID lcid, DISPID* rgdispid)
	{
		HRESULT hRes = EnsureTI(lcid);
		if (m_pInfo != NULL)
		{
			hRes = E_FAIL;
			// Look in cache if
			//	cache is populated
			//	parameter names are not requested
			if (m_pMap != NULL && cNames == 1)
			{
				int n = int( ocslen(rgszNames[0]) );
				for (int j=m_nCount-1; j>=0; j--)
				{
					if ((n == m_pMap[j].nLen) &&
						(memcmp(m_pMap[j].bstr, rgszNames[0], m_pMap[j].nLen * sizeof(OLECHAR)) == 0))
					{
						rgdispid[0] = m_pMap[j].id;
						hRes = S_OK;
						break;
					}
				}
			}
			// if cache is empty or name not in cache or parameter names are requested,
			// delegate to ITypeInfo::GetIDsOfNames
			if (FAILED(hRes))
			{
				hRes = m_pInfo->GetIDsOfNames(rgszNames, cNames, rgdispid);
			}
		}
		return hRes;
	}

	HRESULT Invoke(IDispatch* p, DISPID dispidMember, REFIID /* riid */,
		LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
		EXCEPINFO* pexcepinfo, UINT* puArgErr)
	{
		HRESULT hRes = EnsureTI(lcid);
		if (m_pInfo != NULL)
			hRes = m_pInfo->Invoke(p, dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
		return hRes;
	}
	HRESULT LoadNameCache(ITypeInfo* pTypeInfo)
	{
		TYPEATTR* pta;
		HRESULT hr = pTypeInfo->GetTypeAttr(&pta);
		if (SUCCEEDED(hr))
		{
			stringdispid* pMap = NULL;
			m_nCount = pta->cFuncs;
			m_pMap = NULL;
			if (m_nCount != 0)
			{
				ATLTRY(pMap = new stringdispid[m_nCount]);
				if (pMap == NULL)
				{
					pTypeInfo->ReleaseTypeAttr(pta);
					return E_OUTOFMEMORY;
				}
			}
			for (int i=0; i<m_nCount; i++)
			{
				FUNCDESC* pfd;
				if (SUCCEEDED(pTypeInfo->GetFuncDesc(i, &pfd)))
				{
					CComBSTR bstrName;
					if (SUCCEEDED(pTypeInfo->GetDocumentation(pfd->memid, &bstrName, NULL, NULL, NULL)))
					{
						pMap[i].bstr.Attach(bstrName.Detach());
						pMap[i].nLen = SysStringLen(pMap[i].bstr);
						pMap[i].id = pfd->memid;
					}
					pTypeInfo->ReleaseFuncDesc(pfd);
				}
			}
			m_pMap = pMap;
			pTypeInfo->ReleaseTypeAttr(pta);
		}
		return S_OK;
	}
};


inline HRESULT CComTypeInfoHolder::GetTI(LCID lcid)
{
	//If this assert occurs then most likely didn't initialize properly
	ATLASSERT(m_plibid != NULL && m_pguid != NULL);
	ATLASSERT(!InlineIsEqualGUID(*m_plibid, GUID_NULL) && "Did you forget to pass the LIBID to CComModule::Init?");

	if (m_pInfo != NULL && m_pMap != NULL)
		return S_OK;

	CComCritSecLock<CComCriticalSection> lock(_pAtlModule->m_csStaticDataInitAndTypeInfo, false);
	HRESULT hRes = lock.Lock();
	if (FAILED(hRes))
	{
		ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to lock critical section in CComTypeInfoHolder::GetTI\n"));
		ATLASSERT(0);
		return hRes;
	}
	hRes = E_FAIL;
	if (m_pInfo == NULL)
	{
		ITypeLib* pTypeLib = NULL;
		if (InlineIsEqualGUID(CAtlModule::m_libid, *m_plibid) && m_wMajor == 0xFFFF && m_wMinor == 0xFFFF)
		{
			TCHAR szFilePath[MAX_PATH];
			DWORD dwFLen = ::GetModuleFileName(_AtlBaseModule.GetModuleInstance(), szFilePath, MAX_PATH);
			if( dwFLen != 0 && dwFLen != MAX_PATH )
			{
				USES_CONVERSION_EX;
				LPOLESTR pszFile = T2OLE_EX(szFilePath, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
				if (pszFile == NULL)
					return E_OUTOFMEMORY;
#endif
				hRes = LoadTypeLib(pszFile, &pTypeLib);
			}
		}
		else
		{
			hRes = LoadRegTypeLib(*m_plibid, m_wMajor, m_wMinor, lcid, &pTypeLib);
#ifdef _DEBUG			
			if (SUCCEEDED(hRes))
			{
				// Trace out an warning if the requested TypelibID is the same as the modules TypelibID
				// and versions do not match.
				// 
				// In most cases it is due to wrong version template parameters to IDispatchImpl, 
				// IProvideClassInfoImpl or IProvideClassInfo2Impl.
				// Set major and minor versions to 0xFFFF if the modules type lib has to be loaded
				// irrespective of its version.
				// 
				// Get the module's file path
				TCHAR szFilePath[MAX_PATH];
				DWORD dwFLen = ::GetModuleFileName(_AtlBaseModule.GetModuleInstance(), szFilePath, MAX_PATH);
				if( dwFLen != 0 && dwFLen != MAX_PATH )
				{
					USES_CONVERSION_EX;
					CComPtr<ITypeLib> spTypeLibModule;
					HRESULT hRes2 = S_OK;
					LPOLESTR pszFile = T2OLE_EX(szFilePath, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
					if (pszFile == NULL)
						hRes2 = E_OUTOFMEMORY;
					else
						hRes2 = LoadTypeLib(pszFile, &spTypeLibModule);
					if (SUCCEEDED(hRes2))
					{
						TLIBATTR* pLibAttr;
						hRes2 = spTypeLibModule->GetLibAttr(&pLibAttr);
						if (SUCCEEDED(hRes2))
						{
							if (InlineIsEqualGUID(pLibAttr->guid, *m_plibid) &&
								(pLibAttr->wMajorVerNum != m_wMajor ||
								pLibAttr->wMinorVerNum != m_wMinor))
							{
								ATLTRACE(atlTraceCOM, 0, _T("Warning : CComTypeInfoHolder::GetTI : Loaded typelib does not match the typelib in the module : %s\n"), szFilePath);
								ATLTRACE(atlTraceCOM, 0, _T("\tSee IDispatchImpl overview help topic for more information\n"), szFilePath);							
							}
							spTypeLibModule->ReleaseTLibAttr(pLibAttr);
						}
					}
				}
			}
			else
			{
				ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to load Typelibrary. (HRESULT = 0x%x)\n"), hRes);
				ATLTRACE(atlTraceCOM, 0, _T("\tVerify TypelibID and major version specified with\n"));
				ATLTRACE(atlTraceCOM, 0, _T("\tIDispatchImpl, CStockPropImpl, IProvideClassInfoImpl or IProvideCLassInfo2Impl\n"));
			}
#endif		
		}
		if (SUCCEEDED(hRes))
		{
			CComPtr<ITypeInfo> spTypeInfo;
			hRes = pTypeLib->GetTypeInfoOfGuid(*m_pguid, &spTypeInfo);
			if (SUCCEEDED(hRes))
			{
				CComPtr<ITypeInfo> spInfo(spTypeInfo);
				CComPtr<ITypeInfo2> spTypeInfo2;
				if (SUCCEEDED(spTypeInfo->QueryInterface(&spTypeInfo2)))
					spInfo = spTypeInfo2;

				m_pInfo = spInfo.Detach();
#ifndef _ATL_DLL_IMPL
				_pAtlModule->AddTermFunc(Cleanup, (DWORD_PTR)this);
#endif
			}
			pTypeLib->Release();
		}
	}
	else
	{
		// Another thread has loaded the typeinfo so we're OK.
		hRes = S_OK;
	}

	if (m_pInfo != NULL && m_pMap == NULL)
		LoadNameCache(m_pInfo);

	return hRes;
}

//////////////////////////////////////////////////////////////////////////////
// IObjectWithSite
//
template <class T>
class ATL_NO_VTABLE IObjectWithSiteImpl : public IObjectWithSite
{
public:
	virtual ~IObjectWithSiteImpl()
	{
	}
	
	STDMETHOD(SetSite)(IUnknown *pUnkSite)
	{
		ATLTRACE(atlTraceCOM, 2, _T("IObjectWithSiteImpl::SetSite\n"));
		T* pT = static_cast<T*>(this);
		pT->m_spUnkSite = pUnkSite;
		return S_OK;
	}
	STDMETHOD(GetSite)(REFIID riid, void **ppvSite)
	{
		ATLTRACE(atlTraceCOM, 2, _T("IObjectWithSiteImpl::GetSite\n"));
		T* pT = static_cast<T*>(this);
		ATLASSERT(ppvSite);
		HRESULT hRes = E_POINTER;
		if (ppvSite != NULL)
		{
			if (pT->m_spUnkSite)
				hRes = pT->m_spUnkSite->QueryInterface(riid, ppvSite);
			else
			{
				*ppvSite = NULL;
				hRes = E_FAIL;
			}
		}
		return hRes;
	}

	HRESULT SetChildSite(IUnknown* punkChild)
	{
		if (punkChild == NULL)
			return E_POINTER;

		HRESULT hr;
		CComPtr<IObjectWithSite> spChildSite;
		hr = punkChild->QueryInterface(__uuidof(IObjectWithSite), (void**)&spChildSite);
		if (SUCCEEDED(hr))
			hr = spChildSite->SetSite((IUnknown*)this);

		return hr;
	}

	static HRESULT SetChildSite(IUnknown* punkChild, IUnknown* punkParent)
	{
		return AtlSetChildSite(punkChild, punkParent);
	}

	CComPtr<IUnknown> m_spUnkSite;
};

//////////////////////////////////////////////////////////////////////////////
// IServiceProvider
//
template <class T>
class ATL_NO_VTABLE IServiceProviderImpl : public IServiceProvider
{
public:
	STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void** ppvObject)
	{
		ATLTRACE(atlTraceCOM, 2, _T("IServiceProviderImpl::QueryService\n"));

		T* pT = static_cast<T*>(this);
		return pT->_InternalQueryService(guidService, riid, ppvObject);
	}
};

#define BEGIN_SERVICE_MAP(x) public: \
	HRESULT _InternalQueryService(REFGUID guidService, REFIID riid, void** ppvObject) \
	{ \
		ATLASSERT(ppvObject != NULL); \
		if (ppvObject == NULL) \
			return E_POINTER; \
		*ppvObject = NULL;

#define SERVICE_ENTRY(x) \
		if (InlineIsEqualGUID(guidService, x)) \
			return QueryInterface(riid, ppvObject);

#define SERVICE_ENTRY_CHAIN(x) \
		ATL::CComQIPtr<IServiceProvider, &__uuidof(IServiceProvider)> spProvider(x); \
		if (spProvider != NULL) \
			return spProvider->QueryService(guidService, riid, ppvObject);

#define END_SERVICE_MAP() \
		return E_NOINTERFACE; \
	}


/////////////////////////////////////////////////////////////////////////////
// IDispEventImpl

ATLAPI AtlGetObjectSourceInterface(IUnknown* punkObj, GUID* plibid, IID* piid, unsigned short* pdwMajor, unsigned short* pdwMinor);

#ifndef _M_IX86
template <class T>
class CComStdCallThunk
{
public:
	typedef void (__stdcall T::*TMFP)();
	void* pVtable;
	void* pFunc;
	_stdcallthunk thunk;
	void Init(TMFP dw, void* pThis)
	{
		pVtable = &pFunc;
		pFunc = &thunk;		
		union {
			DWORD_PTR dwFunc;
			TMFP pfn;
		} pfn;
		pfn.pfn = dw;
		thunk.Init(pfn.dwFunc, pThis);
	}
};

#else

// used for _M_IX86
void __stdcall CComStdCallThunkHelper();

template <class T>
class CComStdCallThunk
{
public:
	typedef void (__stdcall T::*TMFP)();
	void *pVTable;								// pointer to artificial VTABLE
	void *pThis;								// pointer to the class
	TMFP pfn;									// Pointer to member function to call
	void (__stdcall *pfnHelper)();				// Artificial VTABLE entry. Points to CComStdCallThunkHelper
												// which modifies the stack and jumps to pfn

	void Init(TMFP pf, void *p)
	{
		pfnHelper = CComStdCallThunkHelper;
		pVTable = &pfnHelper;
		pThis = p;
		pfn = pf;
	}
};

#endif // _M_IX86

#ifndef _ATL_MAX_VARTYPES
#define _ATL_MAX_VARTYPES 8
#endif

struct _ATL_FUNC_INFO
{
	CALLCONV cc;
	VARTYPE vtReturn;
	SHORT nParams;
	VARTYPE pVarTypes[_ATL_MAX_VARTYPES];
};

class ATL_NO_VTABLE _IDispEvent
{
public:
	_IDispEvent() : m_libid(GUID_NULL), m_iid(IID_NULL), m_wMajorVerNum(0), m_wMinorVerNum(0), m_dwEventCookie(0xFEFEFEFE) { }
	//this method needs a different name than QueryInterface
	STDMETHOD(_LocDEQueryInterface)(REFIID riid, void ** ppvObject) = 0;
	virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0;
	virtual ULONG STDMETHODCALLTYPE Release(void) = 0;
	GUID m_libid; // used for dynamic case
	IID m_iid; // used for dynamic case
	unsigned short m_wMajorVerNum;    // Major version number. used for dynamic case
	unsigned short m_wMinorVerNum;    // Minor version number. used for dynamic case
	DWORD m_dwEventCookie;
	HRESULT DispEventAdvise(IUnknown* pUnk, const IID* piid)
	{
		ATLASSERT(m_dwEventCookie == 0xFEFEFEFE);
		return AtlAdvise(pUnk, (IUnknown*)this, *piid, &m_dwEventCookie);
	}
	HRESULT DispEventUnadvise(IUnknown* pUnk, const IID* piid)
	{
		HRESULT hr = AtlUnadvise(pUnk, *piid, m_dwEventCookie);
		m_dwEventCookie = 0xFEFEFEFE;
		return hr;
	}
	//---- add Advise & Unadvise for ease of calling from attribute code ----
	HRESULT Advise(IUnknown *punk) 
	{ 
		HRESULT hr = AtlGetObjectSourceInterface(punk, &m_libid, &m_iid, &m_wMajorVerNum, &m_wMinorVerNum);
		if (FAILED(hr))
			return hr;
		return DispEventAdvise(punk, &m_iid);
	}
	HRESULT Unadvise(IUnknown *punk) 
	{ 
		HRESULT hr = AtlGetObjectSourceInterface(punk, &m_libid, &m_iid, &m_wMajorVerNum, &m_wMinorVerNum);
		if (FAILED(hr))
			return hr;
		return DispEventUnadvise(punk, &m_iid);
	}
};

template <UINT nID, const IID* piid>
class ATL_NO_VTABLE _IDispEventLocator : public _IDispEvent
{
public:
};

template <UINT nID, class T, const IID* pdiid>
class ATL_NO_VTABLE IDispEventSimpleImpl : public _IDispEventLocator<nID, pdiid>
{
public:
	STDMETHOD(_LocDEQueryInterface)(REFIID riid, void ** ppvObject)
	{
		ATLASSERT(ppvObject != NULL);
		if (ppvObject == NULL)
			return E_POINTER;
		*ppvObject = NULL;

		if (InlineIsEqualGUID(riid, IID_NULL))
			return E_NOINTERFACE;

		if (InlineIsEqualGUID(riid, *pdiid) || 
			InlineIsEqualUnknown(riid) ||
			InlineIsEqualGUID(riid, __uuidof(IDispatch)) ||
			InlineIsEqualGUID(riid, m_iid))
		{
			*ppvObject = this;
			AddRef();
#ifdef _ATL_DEBUG_INTERFACES
			_AtlDebugInterfacesModule.AddThunk((IUnknown**)ppvObject, _T("IDispEventImpl"), riid);
#endif // _ATL_DEBUG_INTERFACES
			return S_OK;
		}
		else
			return E_NOINTERFACE;
	}

	// These are here only to support use in non-COM objects	
	virtual ULONG STDMETHODCALLTYPE AddRef()
	{
		return 1;
	}
	virtual ULONG STDMETHODCALLTYPE Release()
	{
		return 1;
	}

	STDMETHOD(GetTypeInfoCount)(UINT* /*pctinfo*/)
	{ATLTRACENOTIMPL(_T("IDispEventSimpleImpl::GetTypeInfoCount"));}

	STDMETHOD(GetTypeInfo)(UINT /*itinfo*/, LCID /*lcid*/, ITypeInfo** /*pptinfo*/)
	{ATLTRACENOTIMPL(_T("IDispEventSimpleImpl::GetTypeInfo"));}

	STDMETHOD(GetIDsOfNames)(REFIID /*riid*/, LPOLESTR* /*rgszNames*/, UINT /*cNames*/,
		LCID /*lcid*/, DISPID* /*rgdispid*/)
	{ATLTRACENOTIMPL(_T("IDispEventSimpleImpl::GetIDsOfNames"));}

	STDMETHOD(Invoke)(DISPID dispidMember, REFIID /*riid*/,
		LCID lcid, WORD /*wFlags*/, DISPPARAMS* pdispparams, VARIANT* pvarResult,
		EXCEPINFO* /*pexcepinfo*/, UINT* /*puArgErr*/)
	{
		const _ATL_EVENT_ENTRY<T>* pMap = T::_GetSinkMap();
		const _ATL_EVENT_ENTRY<T>* pFound = NULL;
		while (pMap->piid != NULL)
		{
			if ((pMap->nControlID == nID) && (pMap->dispid == dispidMember) && 
				(IsEqualIID(*(pMap->piid), *pdiid)))
			{
				pFound = pMap;
				break;
			}
			pMap++;
		}
		if (pFound == NULL)
			return S_OK;


		_ATL_FUNC_INFO info;
		_ATL_FUNC_INFO* pInfo;
		if (pFound->pInfo != NULL)
			pInfo = pFound->pInfo;
		else
		{
			pInfo = &info;
			HRESULT hr = GetFuncInfoFromId(*pdiid, dispidMember, lcid, info);
			if (FAILED(hr))
				return S_OK;
		}
		return InvokeFromFuncInfo(pFound->pfn, *pInfo, pdispparams, pvarResult);
	}

	//Helper for invoking the event
	HRESULT InvokeFromFuncInfo(void (__stdcall T::*pEvent)(), _ATL_FUNC_INFO& info, DISPPARAMS* pdispparams, VARIANT* pvarResult)
	{
		ATLASSERT(pdispparams->cArgs == (UINT)info.nParams);

		T* pT = static_cast<T*>(this);

		// If this assert occurs, then add 
		// #define _ATL_MAX_VARTYPES nnnn
		// before including atlcom.h
		ATLASSERT(info.nParams <= _ATL_MAX_VARTYPES);
		if (info.nParams > _ATL_MAX_VARTYPES)
		{
			return E_FAIL;
		}
		VARIANTARG* rgVarArgs[_ATL_MAX_VARTYPES];
		VARIANTARG** pVarArgs = info.nParams ? rgVarArgs : 0;

		UINT nIndex = 0;

#ifndef _ATL_IGNORE_NAMED_ARGS
		for (nIndex; nIndex < pdispparams->cNamedArgs; nIndex++)
			pVarArgs[pdispparams->rgdispidNamedArgs[nIndex]] = &pdispparams->rgvarg[nIndex];
#endif

		for (; nIndex < pdispparams->cArgs; nIndex++)
			pVarArgs[info.nParams-nIndex-1] = &pdispparams->rgvarg[nIndex];

		CComStdCallThunk<T> thunk;
		thunk.Init(pEvent, pT);

		CComVariant tmpResult;
		if (pvarResult == NULL)
			pvarResult = &tmpResult;

		HRESULT hr = DispCallFunc(
			&thunk,
			0,
			info.cc,
			info.vtReturn,
			info.nParams,
			info.pVarTypes,
			pVarArgs,
			pvarResult);
		ATLASSERT(SUCCEEDED(hr));
		return hr;
	}

	//Helper for finding the function index for a DISPID
	virtual HRESULT GetFuncInfoFromId(const IID& /*iid*/, DISPID /*dispidMember*/, LCID /*lcid*/, _ATL_FUNC_INFO& /*info*/)
	{
		ATLTRACE(_T("TODO: Classes using IDispEventSimpleImpl should override this method\n"));
		ATLASSERT(0);
		ATLTRACENOTIMPL(_T("IDispEventSimpleImpl::GetFuncInfoFromId"));
	}
	//Helpers for sinking events on random IUnknown*
	HRESULT DispEventAdvise(IUnknown* pUnk, const IID* piid)
	{
		ATLASSERT(m_dwEventCookie == 0xFEFEFEFE);
		if (m_dwEventCookie != 0xFEFEFEFE)
			return E_UNEXPECTED;
		return AtlAdvise(pUnk, (IUnknown*)this, *piid, &m_dwEventCookie);
	}
	HRESULT DispEventUnadvise(IUnknown* pUnk, const IID* piid)
	{
		HRESULT hr = AtlUnadvise(pUnk, *piid, m_dwEventCookie);
		m_dwEventCookie = 0xFEFEFEFE;
		return hr;
	}
	HRESULT DispEventAdvise(IUnknown* pUnk)
	{
		return _IDispEvent::DispEventAdvise(pUnk, pdiid);
	}
	HRESULT DispEventUnadvise(IUnknown* pUnk)
	{
		return _IDispEvent::DispEventUnadvise(pUnk, pdiid);
	}
};

//Helper for advising connections points from a sink map
template <class T>
inline HRESULT AtlAdviseSinkMap(T* pT, bool bAdvise)
{
	ATLASSERT(::IsWindow(pT->m_hWnd));
	const _ATL_EVENT_ENTRY<T>* pEntries = T::_GetSinkMap();
	if (pEntries == NULL)
		return S_OK;
	HRESULT hr = S_OK;
	while (pEntries->piid != NULL)
	{
		_IDispEvent* pDE = (_IDispEvent*)((DWORD_PTR)pT+pEntries->nOffset);
		bool bNotAdvised = pDE->m_dwEventCookie == 0xFEFEFEFE;
		if (bAdvise ^ bNotAdvised)
		{
			pEntries++;
			continue;
		}
		hr = E_FAIL;
		HWND h = pT->GetDlgItem(pEntries->nControlID);
		ATLASSERT(h != NULL);
		if (h != NULL)
		{
			CComPtr<IUnknown> spUnk;
			AtlAxGetControl(h, &spUnk);
			ATLASSERT(spUnk != NULL);
			if (spUnk != NULL)
			{
				if (bAdvise)
				{
					if (!InlineIsEqualGUID(IID_NULL, *pEntries->piid))
						hr = pDE->DispEventAdvise(spUnk, pEntries->piid);
					else
					{
						hr = AtlGetObjectSourceInterface(spUnk, &pDE->m_libid, &pDE->m_iid, &pDE->m_wMajorVerNum, &pDE->m_wMinorVerNum);
						if (FAILED(hr))
							return hr;
						hr = pDE->DispEventAdvise(spUnk, &pDE->m_iid);
					}
				}
				else
				{
					if (!InlineIsEqualGUID(IID_NULL, *pEntries->piid))
						hr = pDE->DispEventUnadvise(spUnk, pEntries->piid);
					else
						hr = pDE->DispEventUnadvise(spUnk, &pDE->m_iid);
				}
				ATLASSERT(hr == S_OK);
			}
		}
		if (FAILED(hr))
			break;
		pEntries++;
	}
	return hr;
}

#pragma warning(push)
#pragma warning(disable: 4061) // enumerate XXX not explicitly handled by a case label
inline VARTYPE AtlGetUserDefinedType(ITypeInfo *pTI, HREFTYPE hrt)
{
	if (pTI == NULL)
		E_INVALIDARG;

	CComPtr<ITypeInfo> spTypeInfo;
	VARTYPE vt = VT_USERDEFINED;
	HRESULT hr = E_FAIL;
	hr = pTI->GetRefTypeInfo(hrt, &spTypeInfo);
	if(FAILED(hr))
		return vt;
	TYPEATTR *pta = NULL;

	hr = spTypeInfo->GetTypeAttr(&pta);
	if(SUCCEEDED(hr) && pta && (pta->typekind == TKIND_ALIAS || pta->typekind == TKIND_ENUM))
	{
		if (pta->tdescAlias.vt == VT_USERDEFINED)
			vt = AtlGetUserDefinedType(spTypeInfo, pta->tdescAlias.hreftype);
		else
		{
			switch (pta->typekind)
			{
			case TKIND_ENUM :
				vt = VT_I4;
				break;
			case TKIND_INTERFACE :
				vt = VT_UNKNOWN;
				break;
			case TKIND_DISPATCH :
				vt = VT_DISPATCH;
				break;
			default:
				vt = pta->tdescAlias.vt;
			}
		}				
	}

	if(pta)
		spTypeInfo->ReleaseTypeAttr(pta);
	return vt;

}
#pragma warning(pop)

inline HRESULT AtlGetFuncInfoFromId(ITypeInfo* pTypeInfo, const IID& /*iid*/, DISPID dispidMember, LCID /*lcid*/, _ATL_FUNC_INFO& info)
{
	if (pTypeInfo == NULL)
		return E_INVALIDARG;

	HRESULT hr = S_OK;
	FUNCDESC* pFuncDesc = NULL;
	TYPEATTR* pAttr;
	hr = pTypeInfo->GetTypeAttr(&pAttr);
	if (FAILED(hr))
		return hr;
	int i;
	for (i=0;i<pAttr->cFuncs;i++)
	{
		hr = pTypeInfo->GetFuncDesc(i, &pFuncDesc);
		if (FAILED(hr))
			return hr;
		if (pFuncDesc->memid == dispidMember)
			break;
		pTypeInfo->ReleaseFuncDesc(pFuncDesc);
		pFuncDesc = NULL;
	}
	pTypeInfo->ReleaseTypeAttr(pAttr);
	if (pFuncDesc == NULL)
		return E_FAIL;

	// If this assert occurs, then add a #define _ATL_MAX_VARTYPES nnnn
	// before including atlcom.h
	ATLASSERT(pFuncDesc->cParams <= _ATL_MAX_VARTYPES);
	if (pFuncDesc->cParams > _ATL_MAX_VARTYPES)
		return E_FAIL;

	for (i = 0; i < pFuncDesc->cParams; i++)
	{
		info.pVarTypes[i] = pFuncDesc->lprgelemdescParam[i].tdesc.vt;
		if (info.pVarTypes[i] == VT_PTR)
			info.pVarTypes[i] = (VARTYPE)(pFuncDesc->lprgelemdescParam[i].tdesc.lptdesc->vt | VT_BYREF);
		if (info.pVarTypes[i] == VT_USERDEFINED)
			info.pVarTypes[i] = AtlGetUserDefinedType(pTypeInfo, pFuncDesc->lprgelemdescParam[i].tdesc.hreftype);
	}

	VARTYPE vtReturn = pFuncDesc->elemdescFunc.tdesc.vt;
	switch(vtReturn)
	{
	case VT_INT:
		vtReturn = VT_I4;
		break;
	case VT_UINT:
		vtReturn = VT_UI4;
		break;
	case VT_VOID:
		vtReturn = VT_EMPTY; // this is how DispCallFunc() represents void
		break;
	case VT_HRESULT:
		vtReturn = VT_ERROR;
		break;
	}
	info.vtReturn = vtReturn;
	info.cc = pFuncDesc->callconv;
	info.nParams = pFuncDesc->cParams;
	pTypeInfo->ReleaseFuncDesc(pFuncDesc);
	return S_OK;
}

template <UINT nID, class T, const IID* pdiid = &IID_NULL, const GUID* plibid = &GUID_NULL, 
	WORD wMajor = 0, WORD wMinor = 0, class tihclass = CComTypeInfoHolder>
class ATL_NO_VTABLE IDispEventImpl : public IDispEventSimpleImpl<nID, T, pdiid>
{
public:
	typedef tihclass _tihclass;

	IDispEventImpl()
	{
		m_libid = *plibid;
		m_iid = *pdiid;
		m_wMajorVerNum = wMajor;
		m_wMinorVerNum = wMinor;
	}

	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
	{ 
		if (pctinfo == NULL) 
			return E_POINTER; 
		*pctinfo = 1; 
		return S_OK;
	}

	STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
	{return _tih.GetTypeInfo(itinfo, lcid, pptinfo);}

	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
		LCID lcid, DISPID* rgdispid)
	{return _tih.GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);}

	//Helper for finding the function index for a DISPID
	HRESULT GetFuncInfoFromId(const IID& iid, DISPID dispidMember, LCID lcid, _ATL_FUNC_INFO& info)
	{
		CComPtr<ITypeInfo> spTypeInfo;
		if (InlineIsEqualGUID(*_tih.m_plibid, GUID_NULL))
		{
			_tih.m_plibid = &m_libid;
			_tih.m_pguid = &m_iid;
			_tih.m_wMajor = m_wMajorVerNum;
			_tih.m_wMinor = m_wMinorVerNum;
		}
		HRESULT hr = _tih.GetTI(lcid, &spTypeInfo);
		if (FAILED(hr))
			return hr;
		return AtlGetFuncInfoFromId(spTypeInfo, iid, dispidMember, lcid, info);
	}
	VARTYPE GetUserDefinedType(ITypeInfo *pTI, HREFTYPE hrt)
	{
		return AtlGetUserDefinedType(pTI, hrt);
	}
protected:
	static _tihclass _tih;
	static HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo)
	{return _tih.GetTI(lcid, ppInfo);}
};


template <UINT nID, class T, const IID* piid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
typename IDispEventImpl<nID, T, piid, plibid, wMajor, wMinor, tihclass>::_tihclass
IDispEventImpl<nID, T, piid, plibid, wMajor, wMinor, tihclass>::_tih =
	{piid, plibid, wMajor, wMinor, NULL, 0, NULL, 0};

template <class T>
struct _ATL_EVENT_ENTRY
{
	UINT nControlID;			//ID identifying object instance
	const IID* piid;			//dispinterface IID
	int nOffset;				//offset of dispinterface from this pointer
	DISPID dispid;				//DISPID of method/property
	void (__stdcall T::*pfn)();	//method to invoke
	_ATL_FUNC_INFO* pInfo;
};



//Sink map is used to set up event handling
#define BEGIN_SINK_MAP(_class)\
	typedef _class _GetSinkMapFinder;\
	static const ATL::_ATL_EVENT_ENTRY<_class>* _GetSinkMap()\
	{\
		typedef _class _atl_event_classtype;\
		static const ATL::_ATL_EVENT_ENTRY<_class> map[] = {


#define SINK_ENTRY_INFO(id, iid, dispid, fn, info) {id, &iid, (int)(INT_PTR)(static_cast<ATL::_IDispEventLocator<id, &iid>*>((_atl_event_classtype*)8))-8, dispid, (void (__stdcall _atl_event_classtype::*)())fn, info},
#define SINK_ENTRY_EX(id, iid, dispid, fn) SINK_ENTRY_INFO(id, iid, dispid, fn, NULL)
#define SINK_ENTRY(id, dispid, fn) SINK_ENTRY_EX(id, IID_NULL, dispid, fn)
#define END_SINK_MAP() {0, NULL, 0, 0, NULL, NULL} }; return map;}

/////////////////////////////////////////////////////////////////////////////
// IDispatchImpl

template <class T, const IID* piid = &__uuidof(T), const GUID* plibid = &CAtlModule::m_libid, WORD wMajor = 1,
WORD wMinor = 0, class tihclass = CComTypeInfoHolder>
class ATL_NO_VTABLE IDispatchImpl : public T
{
public:
	typedef tihclass _tihclass;
// IDispatch
	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
	{
		if (pctinfo == NULL) 
			return E_POINTER; 
		*pctinfo = 1;
		return S_OK;
	}
	STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
	{
		return _tih.GetTypeInfo(itinfo, lcid, pptinfo);
	}
	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
		LCID lcid, DISPID* rgdispid)
	{
		return _tih.GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
	}
	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
		LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
		EXCEPINFO* pexcepinfo, UINT* puArgErr)
	{
		return _tih.Invoke((IDispatch*)this, dispidMember, riid, lcid,
		wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
	}

#ifdef _ATL_DLL_IMPL
	// Do not cache type info if it is used in atl71.dll
	IDispatchImpl() : _tih(piid, plibid, wMajor, wMinor)
	{
	}
	virtual ~IDispatchImpl()
	{
	}

protected:
	_tihclass _tih;
	HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo)
	{
		return _tih.GetTI(lcid, ppInfo);
	}

#else

protected:
	static _tihclass _tih;
	static HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo)
	{
		return _tih.GetTI(lcid, ppInfo);
	}

#endif

};

#ifndef _ATL_DLL_IMPL

template <class T, const IID* piid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
typename IDispatchImpl<T, piid, plibid, wMajor, wMinor, tihclass>::_tihclass
IDispatchImpl<T, piid, plibid, wMajor, wMinor, tihclass>::_tih =
{piid, plibid, wMajor, wMinor, NULL, 0, NULL, 0};

#endif

/////////////////////////////////////////////////////////////////////////////
// IProvideClassInfoImpl
template <const CLSID* pcoclsid, const GUID* plibid = &CAtlModule::m_libid,
WORD wMajor = 1, WORD wMinor = 0, class tihclass = CComTypeInfoHolder>
class ATL_NO_VTABLE IProvideClassInfoImpl : public IProvideClassInfo
{
public:
	typedef tihclass _tihclass;

	STDMETHOD(GetClassInfo)(ITypeInfo** pptinfo)
	{
		return _tih.GetTypeInfo(0, LANG_NEUTRAL, pptinfo);
	}

protected:
	static _tihclass _tih;
};

template <const CLSID* pcoclsid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
typename IProvideClassInfoImpl<pcoclsid, plibid, wMajor, wMinor, tihclass>::_tihclass
IProvideClassInfoImpl<pcoclsid, plibid, wMajor, wMinor, tihclass>::_tih =
{pcoclsid,plibid, wMajor, wMinor, NULL, 0, NULL, 0};

/////////////////////////////////////////////////////////////////////////////
// IProvideClassInfo2Impl
template <const CLSID* pcoclsid, const IID* psrcid, const GUID* plibid = &CAtlModule::m_libid,
WORD wMajor = 1, WORD wMinor = 0, class tihclass = CComTypeInfoHolder>
class ATL_NO_VTABLE IProvideClassInfo2Impl : public IProvideClassInfo2
{
public:
	typedef tihclass _tihclass;

	STDMETHOD(GetClassInfo)(ITypeInfo** pptinfo)
	{
		return _tih.GetTypeInfo(0, LANG_NEUTRAL, pptinfo);
	}
	STDMETHOD(GetGUID)(DWORD dwGuidKind, GUID* pGUID)
	{
		if (pGUID == NULL)
			return E_POINTER;

		if (dwGuidKind == GUIDKIND_DEFAULT_SOURCE_DISP_IID && psrcid != NULL)
		{
			*pGUID = *psrcid;
			return S_OK;
		}
		*pGUID = GUID_NULL;
		return E_FAIL;
	}

protected:
	static _tihclass _tih;
};


template <const CLSID* pcoclsid, const IID* psrcid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
typename IProvideClassInfo2Impl<pcoclsid, psrcid, plibid, wMajor, wMinor, tihclass>::_tihclass
IProvideClassInfo2Impl<pcoclsid, psrcid, plibid, wMajor, wMinor, tihclass>::_tih =
{pcoclsid,plibid, wMajor, wMinor, NULL, 0, NULL, 0};


/////////////////////////////////////////////////////////////////////////////
// ISupportErrorInfoImpl

template <const IID* piid>
class ATL_NO_VTABLE ISupportErrorInfoImpl : public ISupportErrorInfo
{
public:
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
	{
		return (InlineIsEqualGUID(riid,*piid)) ? S_OK : S_FALSE;
	}
};


/////////////////////////////////////////////////////////////////////////////
// CComEnumImpl

// These _CopyXXX classes are used with enumerators in order to control
// how enumerated items are initialized, copied, and deleted

// Default is shallow copy with no special init or cleanup
template <class T>
class _Copy
{
public:
	static HRESULT copy(T* p1, T* p2) {memcpy(p1, p2, sizeof(T)); return S_OK;}
	static void init(T*) {}
	static void destroy(T*) {}
};

template<>
class _Copy<VARIANT>
{
public:
	static HRESULT copy(VARIANT* p1, VARIANT* p2) {p1->vt = VT_EMPTY; return VariantCopy(p1, p2);}
	static void init(VARIANT* p) {p->vt = VT_EMPTY;}
	static void destroy(VARIANT* p) {VariantClear(p);}
};

template<>
class _Copy<LPOLESTR>
{
public:
	static HRESULT copy(LPOLESTR* p1, LPOLESTR* p2)
	{
		HRESULT hr = S_OK;
		(*p1) = (LPOLESTR)CoTaskMemAlloc(sizeof(OLECHAR)*(ocslen(*p2)+1));
		if (*p1 == NULL)
			hr = E_OUTOFMEMORY;
		else
			ocscpy(*p1,*p2);
		return hr;
	}
	static void init(LPOLESTR* p) {*p = NULL;}
	static void destroy(LPOLESTR* p) { CoTaskMemFree(*p);}
};

template<>
class _Copy<OLEVERB>
{
public:
	static HRESULT copy(OLEVERB* p1, OLEVERB* p2)
	{
		HRESULT hr = S_OK;
		*p1 = *p2;
		if (p2->lpszVerbName == NULL)
			return S_OK;
		p1->lpszVerbName = (LPOLESTR)CoTaskMemAlloc(sizeof(OLECHAR)*(ocslen(p2->lpszVerbName)+1));
		if (p1->lpszVerbName == NULL)
			hr = E_OUTOFMEMORY;
		else
			ocscpy(p1->lpszVerbName,p2->lpszVerbName);
		return hr;
	}
	static void init(OLEVERB* p) { p->lpszVerbName = NULL;}
	static void destroy(OLEVERB* p) { if (p->lpszVerbName) CoTaskMemFree(p->lpszVerbName);}
};

template<>
class _Copy<CONNECTDATA>
{
public:
	static HRESULT copy(CONNECTDATA* p1, CONNECTDATA* p2)
	{
		*p1 = *p2;
		if (p1->pUnk)
			p1->pUnk->AddRef();
		return S_OK;
	}
	static void init(CONNECTDATA* ) {}
	static void destroy(CONNECTDATA* p) {if (p->pUnk) p->pUnk->Release();}
};

template <class T>
class _CopyInterface
{
public:
	static HRESULT copy(T** p1, T** p2)
	{
		*p1 = *p2;
		if (*p1)
			(*p1)->AddRef();
		return S_OK;
	}
	static void init(T** ) {}
	static void destroy(T** p) {if (*p) (*p)->Release();}
};

template<class T>
class ATL_NO_VTABLE CComIEnum : public IUnknown
{
public:
	STDMETHOD(Next)(ULONG celt, T* rgelt, ULONG* pceltFetched) = 0;
	STDMETHOD(Skip)(ULONG celt) = 0;
	STDMETHOD(Reset)(void) = 0;
	STDMETHOD(Clone)(CComIEnum<T>** ppEnum) = 0;
};


enum CComEnumFlags
{
	//see FlagBits in CComEnumImpl
	AtlFlagNoCopy = 0,
	AtlFlagTakeOwnership = 2,
	AtlFlagCopy = 3 // copy implies ownership
};

template <class Base, const IID* piid, class T, class Copy>
class ATL_NO_VTABLE CComEnumImpl : public Base
{
public:
	CComEnumImpl() {m_begin = m_end = m_iter = NULL; m_dwFlags = 0;}
	virtual ~CComEnumImpl();
	STDMETHOD(Next)(ULONG celt, T* rgelt, ULONG* pceltFetched);
	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)(void){m_iter = m_begin;return S_OK;}
	STDMETHOD(Clone)(Base** ppEnum);
	HRESULT Init(T* begin, T* end, IUnknown* pUnk,
		CComEnumFlags flags = AtlFlagNoCopy);
	CComPtr<IUnknown> m_spUnk;
	T* m_begin;
	T* m_end;
	T* m_iter;
	DWORD m_dwFlags;
protected:
	enum FlagBits
	{
		BitCopy=1,
		BitOwn=2
	};
};

template <class Base, const IID* piid, class T, class Copy>
CComEnumImpl<Base, piid, T, Copy>::~CComEnumImpl()
{
	if (m_dwFlags & BitOwn)
	{
		for (T* p = m_begin; p != m_end; p++)
			Copy::destroy(p);
		delete [] m_begin;
	}
}

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Next(ULONG celt, T* rgelt,
	ULONG* pceltFetched)
{
	if (pceltFetched != NULL)
		*pceltFetched = 0;
	if (celt == 0)
		return E_INVALIDARG;
	if (rgelt == NULL || (celt != 1 && pceltFetched == NULL))
		return E_POINTER;
	if (m_begin == NULL || m_end == NULL || m_iter == NULL)
		return E_FAIL;
	ULONG nRem = (ULONG)(m_end - m_iter);
	HRESULT hRes = S_OK;
	if (nRem < celt)
		hRes = S_FALSE;
	ULONG nMin = celt < nRem ? celt : nRem ;
	if (pceltFetched != NULL)
		*pceltFetched = nMin;
	T* pelt = rgelt;
	while(nMin--)
	{
		HRESULT hr = Copy::copy(pelt, m_iter);
		if (FAILED(hr))
		{
			while (rgelt < pelt)
				Copy::destroy(rgelt++);
			if (pceltFetched != NULL)
				*pceltFetched = 0;
			return hr;
		}
		pelt++;
		m_iter++;
	}
	return hRes;
}

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Skip(ULONG celt)
{
	if (celt == 0)
		return E_INVALIDARG;

	ULONG nRem = ULONG(m_end - m_iter);
	ULONG nSkip = (celt > nRem) ? nRem : celt;
	m_iter += nSkip;
	return (celt == nSkip) ? S_OK : S_FALSE;
}

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Clone(Base** ppEnum)
{
	typedef CComObject<CComEnum<Base, piid, T, Copy> > _class;
	HRESULT hRes = E_POINTER;
	if (ppEnum != NULL)
	{
		*ppEnum = NULL;
		_class* p;
		hRes = _class::CreateInstance(&p);
		if (SUCCEEDED(hRes))
		{
			// If this object has ownership of the data then we need to keep it around
			hRes = p->Init(m_begin, m_end, (m_dwFlags & BitOwn) ? this : m_spUnk);
			if (SUCCEEDED(hRes))
			{
				p->m_iter = m_iter;
				hRes = p->_InternalQueryInterface(*piid, (void**)ppEnum);
			}
			if (FAILED(hRes))
				delete p;
		}
	}
	return hRes;
}

template <class Base, const IID* piid, class T, class Copy>
HRESULT CComEnumImpl<Base, piid, T, Copy>::Init(T* begin, T* end, IUnknown* pUnk,
	CComEnumFlags flags)
{
	if (flags == AtlFlagCopy)
	{
		ATLASSERT(m_begin == NULL); //Init called twice?
		ATLTRY(m_begin = new T[end-begin])
		m_iter = m_begin;
		if (m_begin == NULL)
			return E_OUTOFMEMORY;
		for (T* i=begin; i != end; i++)
		{
			Copy::init(m_iter);
			HRESULT hr = Copy::copy(m_iter, i);
			if (FAILED(hr))
			{
				T* p = m_begin;
				while (p < m_iter)
					Copy::destroy(p++);
				delete [] m_begin;
				m_begin = m_end = m_iter = NULL;
				return hr;
			}
			m_iter++;
		}
		m_end = m_begin + (end-begin);
	}
	else
	{
		m_begin = begin;
		m_end = end;
	}
	m_spUnk = pUnk;
	m_iter = m_begin;
	m_dwFlags = flags;
	return S_OK;
}

template <class Base, const IID* piid, class T, class Copy, class ThreadModel = CComObjectThreadModel>
class ATL_NO_VTABLE CComEnum :
	public CComEnumImpl<Base, piid, T, Copy>,
	public CComObjectRootEx< ThreadModel >
{
public:
	typedef CComEnum<Base, piid, T, Copy > _CComEnum;
	typedef CComEnumImpl<Base, piid, T, Copy > _CComEnumBase;
	BEGIN_COM_MAP(_CComEnum)
		COM_INTERFACE_ENTRY_IID(*piid, _CComEnumBase)
	END_COM_MAP()
};

template <class Base, const IID* piid, class T, class Copy, class CollType>
class ATL_NO_VTABLE IEnumOnSTLImpl : public Base
{
public:
	HRESULT Init(IUnknown *pUnkForRelease, CollType& collection)
	{
		m_spUnk = pUnkForRelease;
		m_pcollection = &collection;
		m_iter = m_pcollection->begin();
		return S_OK;
	}
	STDMETHOD(Next)(ULONG celt, T* rgelt, ULONG* pceltFetched);
	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)(void)
	{
		if (m_pcollection == NULL)
			return E_FAIL;
		m_iter = m_pcollection->begin();
		return S_OK;
	}
	STDMETHOD(Clone)(Base** ppEnum);
//Data
	CComPtr<IUnknown> m_spUnk;
	CollType* m_pcollection;
	typename CollType::iterator m_iter;
};

template <class Base, const IID* piid, class T, class Copy, class CollType>
STDMETHODIMP IEnumOnSTLImpl<Base, piid, T, Copy, CollType>::Next(ULONG celt, T* rgelt,
	ULONG* pceltFetched)
{
	if (rgelt == NULL || (celt != 1 && pceltFetched == NULL))
		return E_POINTER;
	if (pceltFetched != NULL)
		*pceltFetched = 0;
	if (m_pcollection == NULL)
		return E_FAIL;

	ULONG nActual = 0;
	HRESULT hr = S_OK;
	T* pelt = rgelt;
	while (SUCCEEDED(hr) && m_iter != m_pcollection->end() && nActual < celt)
	{
		hr = Copy::copy(pelt, &*m_iter);
		if (FAILED(hr))
		{
			while (rgelt < pelt)
				Copy::destroy(rgelt++);
			nActual = 0;
		}
		else
		{
			pelt++;
			m_iter++;
			nActual++;
		}
	}
	if (SUCCEEDED(hr))
	{
		if (pceltFetched)
			*pceltFetched = nActual;
		if (nActual < celt)
			hr = S_FALSE;
	}
	return hr;
}

template <class Base, const IID* piid, class T, class Copy, class CollType>
STDMETHODIMP IEnumOnSTLImpl<Base, piid, T, Copy, CollType>::Skip(ULONG celt)
{
	HRESULT hr = S_OK;
	while (celt--)
	{
		if (m_iter != m_pcollection->end())
			m_iter++;
		else
		{
			hr = S_FALSE;
			break;
		}
	}
	return hr;
}

template <class Base, const IID* piid, class T, class Copy, class CollType>
STDMETHODIMP IEnumOnSTLImpl<Base, piid, T, Copy, CollType>::Clone(Base** ppEnum)
{
	typedef CComObject<CComEnumOnSTL<Base, piid, T, Copy, CollType> > _class;
	HRESULT hRes = E_POINTER;
	if (ppEnum != NULL)
	{
		*ppEnum = NULL;
		_class* p;
		hRes = _class::CreateInstance(&p);
		if (SUCCEEDED(hRes))
		{
			hRes = p->Init(m_spUnk, *m_pcollection);
			if (SUCCEEDED(hRes))
			{
				p->m_iter = m_iter;
				hRes = p->_InternalQueryInterface(*piid, (void**)ppEnum);
			}
			if (FAILED(hRes))
				delete p;
		}
	}
	return hRes;
}

template <class Base, const IID* piid, class T, class Copy, class CollType, class ThreadModel = CComObjectThreadModel>
class ATL_NO_VTABLE CComEnumOnSTL :
	public IEnumOnSTLImpl<Base, piid, T, Copy, CollType>,
	public CComObjectRootEx< ThreadModel >
{
public:
	typedef CComEnumOnSTL<Base, piid, T, Copy, CollType, ThreadModel > _CComEnum;
	typedef IEnumOnSTLImpl<Base, piid, T, Copy, CollType > _CComEnumBase;
	BEGIN_COM_MAP(_CComEnum)
		COM_INTERFACE_ENTRY_IID(*piid, _CComEnumBase)
	END_COM_MAP()
};

template <class T, class CollType, class ItemType, class CopyItem, class EnumType>
class ICollectionOnSTLImpl : public T
{
public:
	STDMETHOD(get_Count)(long* pcount)
	{
		if (pcount == NULL)
			return E_POINTER;
		ATLASSERT(m_coll.size()<=LONG_MAX);

		*pcount = (long)m_coll.size();

		return S_OK;
	}
	STDMETHOD(get_Item)(long Index, ItemType* pvar)
	{
		//Index is 1-based
		if (pvar == NULL)
			return E_POINTER;
		if (Index < 1)
			return E_INVALIDARG;
		HRESULT hr = E_FAIL;
		Index--;
		CollType::iterator iter = m_coll.begin();
		while (iter != m_coll.end() && Index > 0)
		{
			iter++;
			Index--;
		}
		if (iter != m_coll.end())
			hr = CopyItem::copy(pvar, &*iter);
		return hr;
	}
	STDMETHOD(get__NewEnum)(IUnknown** ppUnk)
	{
		if (ppUnk == NULL)
			return E_POINTER;
		*ppUnk = NULL;
		HRESULT hRes = S_OK;
		CComObject<EnumType>* p;
		hRes = CComObject<EnumType>::CreateInstance(&p);
		if (SUCCEEDED(hRes))
		{
			hRes = p->Init(this, m_coll);
			if (hRes == S_OK)
				hRes = p->QueryInterface(__uuidof(IUnknown), (void**)ppUnk);
		}
		if (hRes != S_OK)
			delete p;
		return hRes;
	}
	CollType m_coll;
};

//////////////////////////////////////////////////////////////////////////////
// ISpecifyPropertyPagesImpl
template <class T>
class ATL_NO_VTABLE ISpecifyPropertyPagesImpl : public ISpecifyPropertyPages
{
public:
	// ISpecifyPropertyPages
	//
	STDMETHOD(GetPages)(CAUUID* pPages)
	{
		ATLTRACE(atlTraceCOM, 2, _T("ISpecifyPropertyPagesImpl::GetPages\n"));
		ATL_PROPMAP_ENTRY* pMap = T::GetPropertyMap();
		return GetPagesHelper(pPages, pMap);
	}
protected:
	HRESULT GetPagesHelper(CAUUID* pPages, ATL_PROPMAP_ENTRY* pMap)
	{
		if (pPages == NULL)
			return E_POINTER;
		ATLASSERT(pMap != NULL);
		if (pMap == NULL)
			return E_POINTER;

		int nCnt = 0;
		int i;
		// Get count of unique pages to alloc the array
		for (i = 0; pMap[i].pclsidPropPage != NULL; i++)
		{
			// only allow non data entry types
			if (pMap[i].vt == 0)
			{
				// Does this property have a page?  CLSID_NULL means it does not
				if (!InlineIsEqualGUID(*pMap[i].pclsidPropPage, CLSID_NULL))
					nCnt++;
			}
		}
		pPages->pElems = (GUID*) CoTaskMemAlloc(sizeof(CLSID)*nCnt);
		if (pPages->pElems == NULL)
			return E_OUTOFMEMORY;
		// reset count of items we have added to the array
		nCnt = 0;
		for (i = 0; pMap[i].pclsidPropPage != NULL; i++)
		{
			// only allow non data entry types
			if (pMap[i].vt == 0)
			{
				// Does this property have a page?  CLSID_NULL means it does not
				if (!InlineIsEqualGUID(*pMap[i].pclsidPropPage, CLSID_NULL))
				{
					BOOL bFound = FALSE;
					// Search through array we are building up to see
					// if it is already in there
					for (int j=0; j<nCnt; j++)
					{
						if (InlineIsEqualGUID(*(pMap[i].pclsidPropPage), pPages->pElems[j]))
						{
							// It's already there, so no need to add it again
							bFound = TRUE;
							break;
						}
					}
					// If we didn't find it in there then add it
					if (!bFound)
						pPages->pElems[nCnt++] = *pMap[i].pclsidPropPage;
				}
			}
		}
		pPages->cElems = nCnt;
		return S_OK;
	}

};

#ifndef _ATL_NO_CONNECTION_POINTS
/////////////////////////////////////////////////////////////////////////////
// Connection Points

struct _ATL_CONNMAP_ENTRY
{
	DWORD_PTR dwOffset;
};


// We want the offset of the connection point relative to the connection
// point container base class
#define BEGIN_CONNECTION_POINT_MAP(x)\
	__if_not_exists(_atl_conn_classtype) \
	{ \
		typedef x _atl_conn_classtype;\
	} \
	static const ATL::_ATL_CONNMAP_ENTRY* GetConnMap(int* pnEntries) {\
	static const ATL::_ATL_CONNMAP_ENTRY _entries[] = {
#define BEGIN_ATTRCONNECTION_POINT_MAP(x)\
	__if_not_exists(_atl_conn_classtype) \
	{ \
		typedef x _atl_conn_classtype;\
	} \
	static const ATL::_ATL_CONNMAP_ENTRY* GetAttrConnMap(int* pnEntries) {\
	static const ATL::_ATL_CONNMAP_ENTRY _entries[] = {

// CONNECTION_POINT_ENTRY computes the offset of the connection point to the
// IConnectionPointContainer interface
#define CONNECTION_POINT_ENTRY(iid){offsetofclass(ATL::_ICPLocator<&iid>, _atl_conn_classtype)-\
	offsetofclass(ATL::IConnectionPointContainerImpl<_atl_conn_classtype>, _atl_conn_classtype)},
#define END_CONNECTION_POINT_MAP() \
	__if_exists(GetAttrConnMap) \
	{ \
		{(DWORD_PTR) -2}, \
		{(DWORD_PTR) GetAttrConnMap }, \
	} \
	{(DWORD_PTR)-1} }; \
	if (pnEntries) \
	{ \
		__if_exists(GetAttrConnMap) \
		{ \
			GetAttrConnMap(pnEntries); \
			*pnEntries += sizeof(_entries)/sizeof(ATL::_ATL_CONNMAP_ENTRY) - 3; \
		} \
		__if_not_exists(GetAttrConnMap) \
		{ \
		*pnEntries = sizeof(_entries)/sizeof(ATL::_ATL_CONNMAP_ENTRY) - 1; \
		} \
	} \
	return _entries;}
#define END_ATTRCONNECTION_POINT_MAP() \
	{(DWORD_PTR)-1} }; \
	if (pnEntries) \
	{ \
		*pnEntries = sizeof(_entries)/sizeof(ATL::_ATL_CONNMAP_ENTRY) - 1; \
	} \
	return _entries;}


#ifndef _DEFAULT_VECTORLENGTH
#define _DEFAULT_VECTORLENGTH 4
#endif

template <unsigned int nMaxSize>
class CComUnkArray
{
public:
	CComUnkArray()
	{
		memset(m_arr, 0, sizeof(IUnknown*)*nMaxSize);
	}
	DWORD Add(IUnknown* pUnk);
	BOOL Remove(DWORD dwCookie);
	// If there is more than one instance of the same IUnknown*,
	// this function returns the cookie for the first one.
	DWORD WINAPI GetCookie(IUnknown** ppFind)
	{
		ATLASSERT(ppFind && *ppFind);
		if (ppFind && *ppFind)
		{
			// find IUnknown* in array
			for (DWORD dwCookie = 0; dwCookie < nMaxSize; dwCookie++)
			{
				if (m_arr[dwCookie] == *ppFind)
					return dwCookie+1; // cookie minus one is an index into the array
			}
		}
		return 0;
	}
	IUnknown* WINAPI GetUnknown(DWORD dwCookie)
	{
		ATLASSERT(dwCookie != 0 && dwCookie <= nMaxSize);
		if (dwCookie != 0 && dwCookie <= nMaxSize)
			return m_arr[dwCookie-1]; // cookie minus one is an index into the array
		else
			return NULL;
	}
	IUnknown** begin()
	{
		return &m_arr[0];
	}
	IUnknown** end()
	{
		return &m_arr[nMaxSize];
	}
protected:
	IUnknown* m_arr[nMaxSize];
};

template <unsigned int nMaxSize>
inline DWORD CComUnkArray<nMaxSize>::Add(IUnknown* pUnk)
{
	DWORD dwCookie = 1;
	for (IUnknown** pp = begin(); pp < end(); pp++)
	{
		if (*pp == NULL)
		{
			*pp = pUnk;
			return dwCookie;
		}
		dwCookie++;
	}
	// If this fires then you need a larger array
	ATLASSERT(0);
	return 0;
}

template <unsigned int nMaxSize>
inline BOOL CComUnkArray<nMaxSize>::Remove(DWORD dwCookie)
{
	ATLASSERT(dwCookie != 0 && dwCookie <= nMaxSize);
	if (dwCookie != 0 && dwCookie <= nMaxSize && m_arr[dwCookie-1] != NULL)
	{
		m_arr[dwCookie-1] = NULL;
		return TRUE;
	}
	else
		return FALSE;
}

template<>
class CComUnkArray<1>
{
public:
	CComUnkArray()
	{
		m_arr[0] = NULL;
	}
	DWORD Add(IUnknown* pUnk)
	{
		if (m_arr[0] != NULL)
		{
			// If this fires then you need a larger array
			ATLASSERT(0);
			return 0;
		}
		m_arr[0] = pUnk;
		return 1;
	}
	BOOL Remove(DWORD dwCookie)
	{
		ATLASSERT(dwCookie == 1);
		if (dwCookie == 1 && m_arr[0] != NULL)
		{
			m_arr[0] = NULL;
			return TRUE;
		}
		else
			return FALSE;
	}
	DWORD WINAPI GetCookie(IUnknown** /* pp */)
	{
		return 1;
	}
	IUnknown* WINAPI GetUnknown(DWORD dwCookie)
	{
		ATLASSERT(dwCookie == 1);
		if (dwCookie == 1)
			return m_arr[0];
		else 
			return NULL;
	}
	IUnknown** begin()
	{
		return &m_arr[0];
	}
	IUnknown** end()
	{
		return (&m_arr[0])+1;
	}
protected:
	IUnknown* m_arr[1];
};

class CComDynamicUnkArray
{
public:
	CComDynamicUnkArray()
	{
		m_nSize = 0;
		m_ppUnk = NULL;
	}

	~CComDynamicUnkArray()
	{
		if (m_nSize > 1)
			free(m_ppUnk);
	}
	DWORD Add(IUnknown* pUnk);
	BOOL Remove(DWORD dwCookie);
	// If there is more than one instance of the same IUnknown*,
	// this function returns the cookie for the first one.
	DWORD WINAPI GetCookie(IUnknown** ppFind)
	{
		ATLASSERT(ppFind && *ppFind);
		if (ppFind && *ppFind)
		{
			IUnknown** ppUnk = NULL;
			DWORD dwCookie = 1;
			// find IUnknown* in array
			for (ppUnk = begin(); ppUnk < end(); ppUnk++)
			{
				if (*ppUnk == *ppFind)
					return dwCookie; // cookie minus one is an index into the array
				dwCookie++;
			}
		}
		return 0;
	}
	IUnknown* WINAPI GetUnknown(DWORD dwCookie)
	{
#ifndef _ATL_OLEDB_CONFORMANCE_TESTS
		ATLASSERT(dwCookie != 0 && dwCookie <= (DWORD)m_nSize);
#endif
		if (dwCookie != 0 && dwCookie <= (DWORD)m_nSize)
			return GetAt(dwCookie-1); // cookie minus one is an index into the array
		else
			return NULL;
	}
	IUnknown** begin()
	{
		return (m_nSize < 2) ? &m_pUnk : m_ppUnk;
	}
	IUnknown** end()
	{
		return (m_nSize < 2) ? (&m_pUnk)+m_nSize : &m_ppUnk[m_nSize];
	}

	IUnknown* GetAt(int nIndex)
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		if (nIndex >= 0 && nIndex < m_nSize)
			return (m_nSize < 2) ? m_pUnk : m_ppUnk[nIndex];
		else
			return NULL;

	}
	int GetSize() const
	{
		return m_nSize;
	}
	void clear()
	{
		if (m_nSize > 1)
			free(m_ppUnk);
		m_nSize = 0;
	}
protected:
	union
	{
		IUnknown** m_ppUnk;
		IUnknown* m_pUnk;
	};
	int m_nSize;
};

inline DWORD CComDynamicUnkArray::Add(IUnknown* pUnk)
{
	IUnknown** pp = NULL;
	// Use m_pUnk for first item.  No need to allocate array.
	if (m_nSize == 0)
	{
		m_pUnk = pUnk;
		m_nSize = 1;
		return 1;
	}
	else if (m_nSize == 1)
	{
		// Create array with _DEFAULT_VECTORLENGTH number of items.
		ATLTRY(pp = (IUnknown**)malloc(sizeof(IUnknown*)*_DEFAULT_VECTORLENGTH));
		if (pp == NULL)
			return 0;
		memset(pp, 0, sizeof(IUnknown*)*_DEFAULT_VECTORLENGTH);
		*pp = m_pUnk;
		m_ppUnk = pp;
		m_nSize = _DEFAULT_VECTORLENGTH;
	}
	// Walk array and use empty slots if any.
	DWORD dwCookie = 1;
	for (pp = begin(); pp < end(); pp++)
	{
		if (*pp == NULL)
		{
			*pp = pUnk;
			return dwCookie; // cookie minus one is index into array
		}
		dwCookie++;
	}
	// No empty slots so resize array.
	// # of new slots is double of current size.
	int nAlloc = m_nSize*2;
	pp = (IUnknown**)realloc(m_ppUnk, sizeof(IUnknown*)*nAlloc);
	if (pp == NULL)
		return 0;
	m_ppUnk = pp;
	memset(&m_ppUnk[m_nSize], 0, sizeof(IUnknown*)*m_nSize);
	m_ppUnk[m_nSize] = pUnk;
	dwCookie = m_nSize+1;
	m_nSize = nAlloc;
	return dwCookie; // cookie minus one is index into array
}

inline BOOL CComDynamicUnkArray::Remove(DWORD dwCookie)
{
#ifndef _ATL_OLEDB_CONFORMANCE_TESTS
	ATLASSERT(dwCookie != 0 && dwCookie <= (DWORD)m_nSize);
#endif
	if (dwCookie != 0 && dwCookie <= (DWORD)m_nSize)
	{
		if (m_nSize == 1)
		{
			if (m_pUnk == NULL)
				return FALSE;
			m_nSize = 0;
			m_pUnk = NULL;
		}
		else
		{
			// cookie minus one is index into array
			if (m_ppUnk[dwCookie-1] == NULL)
				return FALSE;
			m_ppUnk[dwCookie-1] = NULL;
		}
		return TRUE;
	}
	else
		return FALSE;
}

template <const IID* piid>
class ATL_NO_VTABLE _ICPLocator
{
public:
	//this method needs a different name than QueryInterface
	STDMETHOD(_LocCPQueryInterface)(REFIID riid, void ** ppvObject) = 0;
	virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0;
	virtual ULONG STDMETHODCALLTYPE Release(void) = 0;
};

template <class T, const IID* piid, class CDV = CComDynamicUnkArray >
class ATL_NO_VTABLE IConnectionPointImpl : public _ICPLocator<piid>
{
	typedef CComEnum<IEnumConnections, &__uuidof(IEnumConnections), CONNECTDATA,
		_Copy<CONNECTDATA> > CComEnumConnections;
	typedef CDV _CDV;
public:
	~IConnectionPointImpl();
	STDMETHOD(_LocCPQueryInterface)(REFIID riid, void ** ppvObject)
	{
#ifndef _ATL_OLEDB_CONFORMANCE_TESTS
		ATLASSERT(ppvObject != NULL);
#endif
		if (ppvObject == NULL)
			return E_POINTER;
		*ppvObject = NULL;

		if (InlineIsEqualGUID(riid, __uuidof(IConnectionPoint)) || InlineIsEqualUnknown(riid))
		{
			*ppvObject = this;
			AddRef();
#ifdef _ATL_DEBUG_INTERFACES
			_AtlDebugInterfacesModule.AddThunk((IUnknown**)ppvObject, _T("IConnectionPointImpl"), riid);
#endif // _ATL_DEBUG_INTERFACES
			return S_OK;
		}
		else
			return E_NOINTERFACE;
	}

	STDMETHOD(GetConnectionInterface)(IID* piid2)
	{
		if (piid2 == NULL)
			return E_POINTER;
		*piid2 = *piid;
		return S_OK;
	}
	STDMETHOD(GetConnectionPointContainer)(IConnectionPointContainer** ppCPC)
	{
		T* pT = static_cast<T*>(this);
		// No need to check ppCPC for NULL since QI will do that for us
		return pT->QueryInterface(__uuidof(IConnectionPointContainer), (void**)ppCPC);
	}
	STDMETHOD(Advise)(IUnknown* pUnkSink, DWORD* pdwCookie);
	STDMETHOD(Unadvise)(DWORD dwCookie);
	STDMETHOD(EnumConnections)(IEnumConnections** ppEnum);
	CDV m_vec;
};

template <class T, const IID* piid, class CDV>
IConnectionPointImpl<T, piid, CDV>::~IConnectionPointImpl()
{
	IUnknown** pp = m_vec.begin();
	while (pp < m_vec.end())
	{
		if (*pp != NULL)
			(*pp)->Release();
		pp++;
	}
}

template <class T, const IID* piid, class CDV>
STDMETHODIMP IConnectionPointImpl<T, piid, CDV>::Advise(IUnknown* pUnkSink,
	DWORD* pdwCookie)
{
	T* pT = static_cast<T*>(this);
	IUnknown* p;
	HRESULT hRes = S_OK;
	if (pdwCookie != NULL)
		*pdwCookie = 0;
	if (pUnkSink == NULL || pdwCookie == NULL)
		return E_POINTER;
	IID iid;
	GetConnectionInterface(&iid);
	hRes = pUnkSink->QueryInterface(iid, (void**)&p);
	if (SUCCEEDED(hRes))
	{
		pT->Lock();
		*pdwCookie = m_vec.Add(p);
		hRes = (*pdwCookie != NULL) ? S_OK : CONNECT_E_ADVISELIMIT;
		pT->Unlock();
		if (hRes != S_OK)
			p->Release();
	}
	else if (hRes == E_NOINTERFACE)
		hRes = CONNECT_E_CANNOTCONNECT;
	if (FAILED(hRes))
		*pdwCookie = 0;
	return hRes;
}

template <class T, const IID* piid, class CDV>
STDMETHODIMP IConnectionPointImpl<T, piid, CDV>::Unadvise(DWORD dwCookie)
{
	T* pT = static_cast<T*>(this);
	pT->Lock();
	IUnknown* p = m_vec.GetUnknown(dwCookie);
	HRESULT hRes = m_vec.Remove(dwCookie) ? S_OK : CONNECT_E_NOCONNECTION;
	pT->Unlock();
	if (hRes == S_OK && p != NULL)
		p->Release();
	return hRes;
}

template <class T, const IID* piid, class CDV>
STDMETHODIMP IConnectionPointImpl<T, piid, CDV>::EnumConnections(
	IEnumConnections** ppEnum)
{
	if (ppEnum == NULL)
		return E_POINTER;
	*ppEnum = NULL;
	CComObject<CComEnumConnections>* pEnum = NULL;
	ATLTRY(pEnum = new CComObject<CComEnumConnections>)
	if (pEnum == NULL)
		return E_OUTOFMEMORY;
	T* pT = static_cast<T*>(this);
	pT->Lock();
	CONNECTDATA* pcd = NULL;
	ATLTRY(pcd = new CONNECTDATA[m_vec.end()-m_vec.begin()])
	if (pcd == NULL)
	{
		delete pEnum;
		pT->Unlock();
		return E_OUTOFMEMORY;
	}
	CONNECTDATA* pend = pcd;
	// Copy the valid CONNECTDATA's
	for (IUnknown** pp = m_vec.begin();pp<m_vec.end();pp++)
	{
		if (*pp != NULL)
		{
			(*pp)->AddRef();
			pend->pUnk = *pp;
			pend->dwCookie = m_vec.GetCookie(pp);
			pend++;
		}
	}
	// don't copy the data, but transfer ownership to it
	pEnum->Init(pcd, pend, NULL, AtlFlagTakeOwnership);
	pT->Unlock();
	HRESULT hRes = pEnum->_InternalQueryInterface(__uuidof(IEnumConnections), (void**)ppEnum);
	if (FAILED(hRes))
		delete pEnum;
	return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// IConnectionPointContainerImpl

template <class T>
class ATL_NO_VTABLE IConnectionPointContainerImpl : public IConnectionPointContainer
{
	typedef CComEnum<IEnumConnectionPoints,
		&__uuidof(IEnumConnectionPoints), IConnectionPoint*,
		_CopyInterface<IConnectionPoint> >
		CComEnumConnectionPoints;
public:
	STDMETHOD(EnumConnectionPoints)(IEnumConnectionPoints** ppEnum)
	{
		if (ppEnum == NULL)
			return E_POINTER;
		*ppEnum = NULL;
		CComEnumConnectionPoints* pEnum = NULL;
		ATLTRY(pEnum = new CComObject<CComEnumConnectionPoints>)
		if (pEnum == NULL)
			return E_OUTOFMEMORY;

		int nCPCount;
		const _ATL_CONNMAP_ENTRY* pEntry = T::GetConnMap(&nCPCount);

		// allocate an initialize a vector of connection point object pointers
		USES_ATL_SAFE_ALLOCA;
		IConnectionPoint** ppCP = (IConnectionPoint**)_ATL_SAFE_ALLOCA(sizeof(IConnectionPoint*)*nCPCount, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
		if (ppCP == NULL)
		{
			delete pEnum;
			return E_OUTOFMEMORY;
		}

		int i = 0;
		while (pEntry->dwOffset != (DWORD_PTR)-1)
		{
			if (pEntry->dwOffset == (DWORD_PTR)-2)
			{
				pEntry++;
				const _ATL_CONNMAP_ENTRY* (*pFunc)(int*) =  (const _ATL_CONNMAP_ENTRY* (*)(int*))(pEntry->dwOffset);
				pEntry = pFunc(NULL);
				continue;
			}
			ppCP[i++] = (IConnectionPoint*)((INT_PTR)this+pEntry->dwOffset);
			pEntry++;
		}

		// copy the pointers: they will AddRef this object
		HRESULT hRes = pEnum->Init((IConnectionPoint**)&ppCP[0],
			(IConnectionPoint**)&ppCP[nCPCount],
			reinterpret_cast<IConnectionPointContainer*>(this), AtlFlagCopy);
		if (FAILED(hRes))
		{
			delete pEnum;
			return hRes;
		}
		hRes = pEnum->QueryInterface(__uuidof(IEnumConnectionPoints), (void**)ppEnum);
		if (FAILED(hRes))
			delete pEnum;
		return hRes;
	}
	STDMETHOD(FindConnectionPoint)(REFIID riid, IConnectionPoint** ppCP)
	{
		if (ppCP == NULL)
			return E_POINTER;
		*ppCP = NULL;
		HRESULT hRes = CONNECT_E_NOCONNECTION;
		const _ATL_CONNMAP_ENTRY* pEntry = T::GetConnMap(NULL);
		IID iid;
		while (pEntry->dwOffset != (DWORD_PTR)-1)
		{
			if (pEntry->dwOffset == (DWORD_PTR)-2)
			{
				pEntry++;
				const _ATL_CONNMAP_ENTRY* (*pFunc)(int*) =  (const _ATL_CONNMAP_ENTRY* (*)(int*))(pEntry->dwOffset);
				pEntry = pFunc(NULL);
				continue;
			}
			IConnectionPoint* pCP =
				(IConnectionPoint*)((INT_PTR)this+pEntry->dwOffset);
			if (SUCCEEDED(pCP->GetConnectionInterface(&iid)) &&
				InlineIsEqualGUID(riid, iid))
			{
				*ppCP = pCP;
				pCP->AddRef();
				hRes = S_OK;
				break;
			}
			pEntry++;
		}
		return hRes;
	}
};

#endif //!_ATL_NO_CONNECTION_POINTS

/////////////////////////////////////////////////////////////////////////////
// IExternalConnectionImpl

// An object that implements IExternalConnection should explicitly call 
// CoDisconnectObject on itself when its external reference count drops to 0. 
// This call will cause the stub manager to call Release on the object so the 
// object can destroy itself.

template <class T> 
class IExternalConnectionImpl : public IExternalConnection
{
public:
	IExternalConnectionImpl(void) : m_nStrongLocks(0) {}
	STDMETHODIMP_(DWORD) AddConnection(DWORD extconn, DWORD /*dwReserved*/)
	{
		DWORD dw = 0;
		if (extconn & EXTCONN_STRONG)
		{
			dw = T::_ThreadModel::Increment(&m_nStrongLocks);
			static_cast<T*>(this)->OnAddConnection(dw == 1);
		}
		return dw;
	}
	STDMETHODIMP_(DWORD) ReleaseConnection(DWORD extconn, DWORD /*dwReserved*/, BOOL bLastUnlockReleases)
	{
		DWORD dw = 0;
		if (extconn & EXTCONN_STRONG)
		{
			dw = T::_ThreadModel::Decrement(&m_nStrongLocks);
			static_cast<T*>(this)->OnReleaseConnection(dw == 0, !!bLastUnlockReleases);
		}
		return dw;
	}

	// Services provided by this class
	bool DoIHaveAStub() { return m_nStrongLocks != 0; }
	LONG GetStrongConnectionCount() { return m_nStrongLocks; }
	// Extensibility points provided by this class
	void OnAddConnection(bool bThisIsFirstLock) {}
	void OnReleaseConnection(bool bThisIsLastUnlock, bool bLastUnlockReleases)
	{
		if (bThisIsLastUnlock && bLastUnlockReleases)
			CoDisconnectObject(static_cast<T*>(this)->GetUnknown(), 0);
	}
	// Implementation
	LONG m_nStrongLocks;
};

#pragma pack(pop)

// All exports go here
#ifndef _ATL_DLL

/////////////////////////////////////////////////////////////////////////////
// IDispatch Error handling

ATLINLINE ATLAPI AtlSetErrorInfo(const CLSID& clsid, LPCOLESTR lpszDesc, DWORD dwHelpID,
	LPCOLESTR lpszHelpFile, const IID& iid, HRESULT hRes, HINSTANCE hInst)
{
	USES_CONVERSION_EX;
	TCHAR szDesc[1024];
	szDesc[0] = NULL;
	// For a valid HRESULT the id should be in the range [0x0200, 0xffff]
	if (IS_INTRESOURCE(lpszDesc)) //id
	{
		UINT nID = LOWORD((DWORD_PTR)lpszDesc);
		ATLASSERT((nID >= 0x0200 && nID <= 0xffff) || hRes != 0);
		if (LoadString(hInst, nID, szDesc, 1024) == 0)
		{
			ATLASSERT(FALSE);
			lstrcpy(szDesc, _T("Unknown Error"));
		}
		lpszDesc = T2OLE_EX(szDesc, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
		if(lpszDesc == NULL) 
			return E_OUTOFMEMORY;
#endif			
		if (hRes == 0)
			hRes = MAKE_HRESULT(3, FACILITY_ITF, nID);
	}

	CComPtr<ICreateErrorInfo> pICEI;
	if (SUCCEEDED(CreateErrorInfo(&pICEI)))
	{
		CComPtr<IErrorInfo> pErrorInfo;
		pICEI->SetGUID(iid);
		LPOLESTR lpsz;
		ProgIDFromCLSID(clsid, &lpsz);
		if (lpsz != NULL)
			pICEI->SetSource(lpsz);
		if (dwHelpID != 0 && lpszHelpFile != NULL)
		{
			pICEI->SetHelpContext(dwHelpID);
			pICEI->SetHelpFile(const_cast<LPOLESTR>(lpszHelpFile));
		}
		CoTaskMemFree(lpsz);
		pICEI->SetDescription((LPOLESTR)lpszDesc);
		if (SUCCEEDED(pICEI->QueryInterface(__uuidof(IErrorInfo), (void**)&pErrorInfo)))
			SetErrorInfo(0, pErrorInfo);
	}
	return (hRes == 0) ? DISP_E_EXCEPTION : hRes;
}

/////////////////////////////////////////////////////////////////////////////
// IPersist* helpers.

ATLINLINE ATLAPI AtlIPersistStreamInit_Load(LPSTREAM pStm, ATL_PROPMAP_ENTRY* pMap, void* pThis, IUnknown* pUnk)
{
	ATLASSERT(pMap != NULL);
	if (pStm == NULL || pMap == NULL || pThis == NULL || pUnk == NULL)
		return E_INVALIDARG;

	HRESULT hr = S_OK;
	DWORD dwVer;
	hr = pStm->Read(&dwVer, sizeof(DWORD), NULL);
	if (FAILED(hr))
		return hr;
	if (dwVer > _ATL_VER)
		return E_FAIL;

	CComPtr<IDispatch> pDispatch;
	const IID* piidOld = NULL;
	for (int i = 0; pMap[i].pclsidPropPage != NULL; i++)
	{
		if (pMap[i].szDesc == NULL)
			continue;

		// check if raw data entry
		if (pMap[i].dwSizeData != 0)
		{
			void* pData = (void*) (pMap[i].dwOffsetData + (DWORD_PTR)pThis);
			// call CComBSTR::ReadFromStream for BSTRs
			if (pMap[i].vt == VT_BSTR)
			{
				CComBSTR bstrRead;
				hr = bstrRead.ReadFromStream(pStm);
				(*(BSTR*)pData) = bstrRead.Detach();
			}
			else
				hr = pStm->Read(pData, pMap[i].dwSizeData, NULL);
			if (FAILED(hr))
				return hr;
			continue;
		}

		CComVariant var;

		hr = var.ReadFromStream(pStm);
		if (FAILED(hr))
			break;

		if (pMap[i].piidDispatch != piidOld)
		{
			pDispatch.Release();
			if (FAILED(pUnk->QueryInterface(*pMap[i].piidDispatch, (void**)&pDispatch)))
			{
				ATLTRACE(atlTraceCOM, 0, _T("Failed to get a dispatch pointer for property #%i\n"), i);
				hr = E_FAIL;
				break;
			}
			piidOld = pMap[i].piidDispatch;
		}

		if (FAILED(pDispatch.PutProperty(pMap[i].dispid, &var)))
		{
			ATLTRACE(atlTraceCOM, 0, _T("Invoked failed on DISPID %x\n"), pMap[i].dispid);
			hr = E_FAIL;
			break;
		}
	}
	return hr;
}

ATLINLINE ATLAPI AtlIPersistStreamInit_Save(LPSTREAM pStm,
	BOOL /* fClearDirty */, ATL_PROPMAP_ENTRY* pMap,
	void* pThis, IUnknown* pUnk)
{
	ATLASSERT(pMap != NULL);
	if (pStm == NULL || pMap == NULL || pThis == NULL || pUnk == NULL)
		return E_INVALIDARG;

	DWORD dw = _ATL_VER;
	HRESULT hr = pStm->Write(&dw, sizeof(DWORD), NULL);
	if (FAILED(hr))
		return hr;

	CComPtr<IDispatch> pDispatch;
	const IID* piidOld = NULL;
	for (int i = 0; pMap[i].pclsidPropPage != NULL; i++)
	{
		if (pMap[i].szDesc == NULL)
			continue;

		// check if raw data entry
		if (pMap[i].dwSizeData != 0)
		{
			void* pData = (void*) (pMap[i].dwOffsetData + (DWORD_PTR)pThis);
			// call CComBSTR::WriteToStream for BSTRs
			if (pMap[i].vt == VT_BSTR)
			{
				CComBSTR bstrWrite;
				bstrWrite.Attach(*(BSTR*)pData);
				hr = bstrWrite.WriteToStream(pStm);
				bstrWrite.Detach();
			}
			else
				hr = pStm->Write(pData, pMap[i].dwSizeData, NULL);
			if (FAILED(hr))
				return hr;
			continue;
		}

		CComVariant var;
		if (pMap[i].piidDispatch != piidOld)
		{
			pDispatch.Release();
			if (FAILED(pUnk->QueryInterface(*pMap[i].piidDispatch, (void**)&pDispatch)))
			{
				ATLTRACE(atlTraceCOM, 0, _T("Failed to get a dispatch pointer for property #%i\n"), i);
				hr = E_FAIL;
				break;
			}
			piidOld = pMap[i].piidDispatch;
		}

		if (FAILED(pDispatch.GetProperty(pMap[i].dispid, &var)))
		{
			ATLTRACE(atlTraceCOM, 0, _T("Invoked failed on DISPID %x\n"), pMap[i].dispid);
			hr = E_FAIL;
			break;
		}

		hr = var.WriteToStream(pStm);
		if (FAILED(hr))
			break;
	}
	return hr;
}
ATLINLINE ATLAPI AtlIPersistPropertyBag_Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog, ATL_PROPMAP_ENTRY* pMap, void* pThis, IUnknown* pUnk)
{
	if (pPropBag == NULL || pMap == NULL || pThis == NULL || pUnk == NULL)
		return E_INVALIDARG;

	CComPtr<IDispatch> pDispatch;
	const IID* piidOld = NULL;
	for (int i = 0; pMap[i].pclsidPropPage != NULL; i++)
	{
		if (pMap[i].szDesc == NULL)
			continue;

		CComVariant var;

		// If raw entry skip it - we don't handle it for property bags just yet
		if (pMap[i].dwSizeData != 0)
		{
			var.vt = pMap[i].vt; 
			void* pData = (void*) (pMap[i].dwOffsetData + (DWORD_PTR)pThis);
			HRESULT hr = pPropBag->Read(pMap[i].szDesc, &var, pErrorLog);
			if (SUCCEEDED(hr))
			{
				// check the type - we only deal with limited set
				switch (pMap[i].vt)
				{
				case VT_UI1:
				case VT_I1:
					*((BYTE*)pData) = var.bVal;
					break;
				case VT_BOOL:
					*((VARIANT_BOOL*)pData) = var.boolVal;
					break;
				case VT_I2:
				case VT_UI2:
					*((short*)pData) = var.iVal;
					break;
				case VT_I4:
				case VT_UI4:
				case VT_INT:
				case VT_UINT:
					*((long*)pData) = var.lVal;
					break;
				case VT_BSTR:
					*((BSTR*)pData) = ::SysAllocString(var.bstrVal);
					if (*((BSTR*)pData) == NULL && var.bstrVal != NULL)
						return E_OUTOFMEMORY;
					break;
				}
			}
			continue;
		}

		if (pMap[i].piidDispatch != piidOld)
		{
			pDispatch.Release();
			if (FAILED(pUnk->QueryInterface(*pMap[i].piidDispatch, (void**)&pDispatch)))
			{
				ATLTRACE(atlTraceCOM, 0, _T("Failed to get a dispatch pointer for property #%i\n"), i);
				return E_FAIL;
			}
			piidOld = pMap[i].piidDispatch;
		}

		if (FAILED(pDispatch.GetProperty(pMap[i].dispid, &var)))
		{
			ATLTRACE(atlTraceCOM, 0, _T("Invoked failed on DISPID %x\n"), pMap[i].dispid);
			return E_FAIL;
		}

		HRESULT hr = pPropBag->Read(pMap[i].szDesc, &var, pErrorLog);
		if (FAILED(hr))
		{
#ifdef _DEBUG
			USES_CONVERSION_EX;
			LPCTSTR lp = OLE2CT_EX(pMap[i].szDesc, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);

			if (hr == E_INVALIDARG)
			{
				if (lp == NULL)
					ATLTRACE(atlTraceCOM, 0, _T("Property not in Bag\n"));
				else
					ATLTRACE(atlTraceCOM, 0, _T("Property %s not in Bag\n"), lp);
			}
			else
			{
				// Many containers return different ERROR values for Member not found
				if (lp == NULL)
					ATLTRACE(atlTraceCOM, 0, _T("Error attempting to read Property from PropertyBag \n"));
				else
					ATLTRACE(atlTraceCOM, 0, _T("Error attempting to read Property %s from PropertyBag \n"), lp);
			}
#endif			
			continue;
		}

		if (FAILED(pDispatch.PutProperty(pMap[i].dispid, &var)))
		{
			ATLTRACE(atlTraceCOM, 0, _T("Invoked failed on DISPID %x\n"), pMap[i].dispid);
			return E_FAIL;
		}
	}
	return S_OK;
}
ATLINLINE ATLAPI AtlIPersistPropertyBag_Save(LPPROPERTYBAG pPropBag,
	BOOL /* fClearDirty */, BOOL /* fSaveAllProperties */,
	ATL_PROPMAP_ENTRY* pMap, void* pThis, IUnknown* pUnk)
{
	if (pPropBag == NULL)
	{
		ATLTRACE(atlTraceCOM, 0, _T("PropBag pointer passed in was invalid\n"));
		return E_INVALIDARG;
	}
	if (pMap == NULL || pThis == NULL || pUnk == NULL)
		return E_INVALIDARG;

	CComPtr<IDispatch> pDispatch;
	const IID* piidOld = NULL;
	for (int i = 0; pMap[i].pclsidPropPage != NULL; i++)
	{
		if (pMap[i].szDesc == NULL)
			continue;

		CComVariant var;

		// If raw entry skip it - we don't handle it for property bags just yet
		if (pMap[i].dwSizeData != 0)
		{
			void* pData = (void*) (pMap[i].dwOffsetData + (DWORD_PTR)pThis);
			// check the type - we only deal with limited set
			bool bTypeOK = false;
			switch (pMap[i].vt)
			{
			case VT_UI1:
			case VT_I1:
				var.bVal = *((BYTE*)pData);
				bTypeOK = true;
				break;
			case VT_BOOL:
				var.boolVal = *((VARIANT_BOOL*)pData);
				bTypeOK = true;
				break;
			case VT_UI2:
				var.iVal = *((short*)pData);
				bTypeOK = true;
				break;
			case VT_UI4:
			case VT_INT:
			case VT_UINT:
				var.lVal = *((long*)pData);
				bTypeOK = true;
				break;
			case VT_BSTR:
				var.bstrVal = ::SysAllocString(*((BSTR*)pData));
				if (var.bstrVal == NULL && *((BSTR*)pData) != NULL)
					return E_OUTOFMEMORY;
				bTypeOK = true;
				break;
			}
			if (bTypeOK)
			{
				var.vt = pMap[i].vt;
				HRESULT hr = pPropBag->Write(pMap[i].szDesc, &var);
				if (FAILED(hr))
					return hr;
			}
			continue;
		}

		if (pMap[i].piidDispatch != piidOld)
		{
			pDispatch.Release();
			if (FAILED(pUnk->QueryInterface(*pMap[i].piidDispatch, (void**)&pDispatch)))
			{
				ATLTRACE(atlTraceCOM, 0, _T("Failed to get a dispatch pointer for property #%i\n"), i);
				return E_FAIL;
			}
			piidOld = pMap[i].piidDispatch;
		}

		if (FAILED(pDispatch.GetProperty(pMap[i].dispid, &var)))
		{
			ATLTRACE(atlTraceCOM, 0, _T("Invoked failed on DISPID %x\n"), pMap[i].dispid);
			return E_FAIL;
		}

		if (var.vt == VT_UNKNOWN || var.vt == VT_DISPATCH)
		{
			if (var.punkVal == NULL)
			{
				ATLTRACE(atlTraceCOM, 2, _T("Warning skipping empty IUnknown in Save\n"));
				continue;
			}
		}

		HRESULT hr = pPropBag->Write(pMap[i].szDesc, &var);
		if (FAILED(hr))
			return hr;
	}
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Connection Point Sink Helper

ATLINLINE ATLAPI AtlGetObjectSourceInterface(IUnknown* punkObj, GUID* plibid, IID* piid, unsigned short* pdwMajor, unsigned short* pdwMinor)
{
	if (plibid == NULL || piid == NULL || pdwMajor == NULL || pdwMinor == NULL)
		return E_POINTER;
		
	*plibid = GUID_NULL;
	*piid = IID_NULL;
	*pdwMajor = 0;
	*pdwMinor = 0;
	

	HRESULT hr = E_FAIL;
	if (punkObj != NULL)
	{
		CComPtr<IDispatch> spDispatch;
		hr = punkObj->QueryInterface(__uuidof(IDispatch), (void**)&spDispatch);
		if (SUCCEEDED(hr))
		{
			CComPtr<ITypeInfo> spTypeInfo;
			hr = spDispatch->GetTypeInfo(0, 0, &spTypeInfo);
			if (SUCCEEDED(hr))
			{
				CComPtr<ITypeLib> spTypeLib;
				hr = spTypeInfo->GetContainingTypeLib(&spTypeLib, 0);
				if (SUCCEEDED(hr))
				{
					TLIBATTR* plibAttr;
					hr = spTypeLib->GetLibAttr(&plibAttr);
					if (SUCCEEDED(hr))
					{
						memcpy(plibid, &plibAttr->guid, sizeof(GUID));
						*pdwMajor = plibAttr->wMajorVerNum;
						*pdwMinor = plibAttr->wMinorVerNum;
						spTypeLib->ReleaseTLibAttr(plibAttr);
						// First see if the object is willing to tell us about the
						// default source interface via IProvideClassInfo2
						CComPtr<IProvideClassInfo2> spProvideClassInfo;
						hr = punkObj->QueryInterface(__uuidof(IProvideClassInfo2), (void**)&spProvideClassInfo);
						if (SUCCEEDED(hr) && spProvideClassInfo != NULL)
							hr = spProvideClassInfo->GetGUID(GUIDKIND_DEFAULT_SOURCE_DISP_IID, piid);
						else
						{
							// No, we have to go hunt for it
							CComPtr<ITypeInfo> spInfoCoClass;
							// If we have a clsid, use that
							// Otherwise, try to locate the clsid from IPersist
							CComPtr<IPersist> spPersist;
							CLSID clsid;
							hr = punkObj->QueryInterface(__uuidof(IPersist), (void**)&spPersist);
							if (SUCCEEDED(hr))
							{
								hr = spPersist->GetClassID(&clsid);
								if (SUCCEEDED(hr))
								{
									hr = spTypeLib->GetTypeInfoOfGuid(clsid, &spInfoCoClass);
									if (SUCCEEDED(hr))
									{
										TYPEATTR* pAttr=NULL;
										spInfoCoClass->GetTypeAttr(&pAttr);
										if (pAttr != NULL)
										{
											HREFTYPE hRef;
											for (int i = 0; i < pAttr->cImplTypes; i++)
											{
												int nType;
												hr = spInfoCoClass->GetImplTypeFlags(i, &nType);
												if (SUCCEEDED(hr))
												{
													if (nType == (IMPLTYPEFLAG_FDEFAULT | IMPLTYPEFLAG_FSOURCE))
													{
														// we found it
														hr = spInfoCoClass->GetRefTypeOfImplType(i, &hRef);
														if (SUCCEEDED(hr))
														{
															CComPtr<ITypeInfo> spInfo;
															hr = spInfoCoClass->GetRefTypeInfo(hRef, &spInfo);
															if (SUCCEEDED(hr))
															{
																TYPEATTR* pAttrIF;
																spInfo->GetTypeAttr(&pAttrIF);
																if (pAttrIF != NULL)
																{
																	memcpy(piid, &pAttrIF->guid, sizeof(GUID));
																	spInfo->ReleaseTypeAttr(pAttrIF);
																}
															}
														}
														break;
													}
												}
											}
											spInfoCoClass->ReleaseTypeAttr(pAttr);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return hr;
}

#endif	// !_ATL_DLL

}	// namespace ATL

#ifdef _ATL_ALL_WARNINGS
#pragma warning( pop )
#endif

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning (pop)
#endif

#endif // __ATLCOM_H__

/////////////////////////////////////////////////////////////////////////////
