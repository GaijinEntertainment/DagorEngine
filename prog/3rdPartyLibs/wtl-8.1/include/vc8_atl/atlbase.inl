// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.

#ifndef __ATLBASE_INL__
#define __ATLBASE_INL__

#pragma once

#ifndef __ATLBASE_H__
	#error atlbase.inl requires atlbase.h to be included first
#endif
#pragma warning(push)
#pragma warning(disable:4571) //catch(...) blocks compiled with /EHs do NOT catch or re-throw Structured Exceptions
namespace ATL
{

/////////////////////////////////////////////////////////////////////////////
// Connection Point Helpers

ATLINLINE ATLAPI AtlAdvise(IUnknown* pUnkCP, IUnknown* pUnk, const IID& iid, LPDWORD pdw)
{
	if(pUnkCP == NULL)
		return E_INVALIDARG;
		
	CComPtr<IConnectionPointContainer> pCPC;
	CComPtr<IConnectionPoint> pCP;
	HRESULT hRes = pUnkCP->QueryInterface(__uuidof(IConnectionPointContainer), (void**)&pCPC);
	if (SUCCEEDED(hRes))
		hRes = pCPC->FindConnectionPoint(iid, &pCP);
	if (SUCCEEDED(hRes))
		hRes = pCP->Advise(pUnk, pdw);
	return hRes;
}

ATLINLINE ATLAPI AtlUnadvise(IUnknown* pUnkCP, const IID& iid, DWORD dw)
{
	if(pUnkCP == NULL)
		return E_INVALIDARG;
		
	CComPtr<IConnectionPointContainer> pCPC;
	CComPtr<IConnectionPoint> pCP;
	HRESULT hRes = pUnkCP->QueryInterface(__uuidof(IConnectionPointContainer), (void**)&pCPC);
	if (SUCCEEDED(hRes))
		hRes = pCPC->FindConnectionPoint(iid, &pCP);
	if (SUCCEEDED(hRes))
		hRes = pCP->Unadvise(dw);
	return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// Inproc Marshaling helpers

//This API should be called from the same thread that called
//AtlMarshalPtrInProc
ATLINLINE ATLAPI AtlFreeMarshalStream(IStream* pStream)
{
	HRESULT hRes=S_OK;
	if (pStream != NULL)
	{
		LARGE_INTEGER l;
		l.QuadPart = 0;
		pStream->Seek(l, STREAM_SEEK_SET, NULL);
		hRes=CoReleaseMarshalData(pStream);
		pStream->Release();
	}
	return hRes;
}

ATLINLINE ATLAPI AtlMarshalPtrInProc(IUnknown* pUnk, const IID& iid, IStream** ppStream)
{
	ATLASSERT(ppStream != NULL);
	if (ppStream == NULL)
		return E_POINTER;
		
	HRESULT hRes = CreateStreamOnHGlobal(NULL, TRUE, ppStream);
	if (SUCCEEDED(hRes))
	{
		hRes = CoMarshalInterface(*ppStream, iid,
			pUnk, MSHCTX_INPROC, NULL, MSHLFLAGS_TABLESTRONG);
		if (FAILED(hRes))
		{
			(*ppStream)->Release();
			*ppStream = NULL;
		}
	}
	return hRes;
}

ATLINLINE ATLAPI AtlUnmarshalPtr(IStream* pStream, const IID& iid, IUnknown** ppUnk)
{
	ATLASSERT(ppUnk != NULL);
	if (ppUnk == NULL)
		return E_POINTER;

	*ppUnk = NULL;
	HRESULT hRes = E_INVALIDARG;
	if (pStream != NULL)
	{
		LARGE_INTEGER l;
		l.QuadPart = 0;
		pStream->Seek(l, STREAM_SEEK_SET, NULL);
		hRes = CoUnmarshalInterface(pStream, iid, (void**)ppUnk);
	}
	return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// Module

ATLINLINE ATLAPI AtlComModuleGetClassObject(_ATL_COM_MODULE* pComModule, REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	ATLASSERT(pComModule != NULL);
	if (pComModule == NULL)
		return E_INVALIDARG;
	if (pComModule->cbSize == 0)  // Module hasn't been initialized
		return E_UNEXPECTED;

	if (ppv == NULL)
		return E_POINTER;
	*ppv = NULL;

	HRESULT hr = S_OK;

	for (_ATL_OBJMAP_ENTRY** ppEntry = pComModule->m_ppAutoObjMapFirst; ppEntry < pComModule->m_ppAutoObjMapLast; ppEntry++)
	{
		if (*ppEntry != NULL)
		{
			_ATL_OBJMAP_ENTRY* pEntry = *ppEntry;
			if ((pEntry->pfnGetClassObject != NULL) && InlineIsEqualGUID(rclsid, *pEntry->pclsid))
			{
				if (pEntry->pCF == NULL)
				{
					CComCritSecLock<CComCriticalSection> lock(pComModule->m_csObjMap, false);
					hr = lock.Lock();
					if (FAILED(hr))
					{
						ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to lock critical section in AtlComModuleGetClassObject\n"));
						ATLASSERT(0);
						break;
					}
					if (pEntry->pCF == NULL)
						hr = pEntry->pfnGetClassObject(pEntry->pfnCreateInstance, __uuidof(IUnknown), (LPVOID*)&pEntry->pCF);
				}
				if (pEntry->pCF != NULL)
					hr = pEntry->pCF->QueryInterface(riid, ppv);
				break;
			}
		}
	}

	if (*ppv == NULL && hr == S_OK)
		hr = CLASS_E_CLASSNOTAVAILABLE;
	return hr;
}

ATLINLINE ATLAPI AtlComModuleRegisterClassObjects(_ATL_COM_MODULE* pComModule, DWORD dwClsContext, DWORD dwFlags)
{
	ATLASSERT(pComModule != NULL);
	if (pComModule == NULL)
		return E_INVALIDARG;

	HRESULT hr = S_FALSE;
	for (_ATL_OBJMAP_ENTRY** ppEntry = pComModule->m_ppAutoObjMapFirst; ppEntry < pComModule->m_ppAutoObjMapLast && SUCCEEDED(hr); ppEntry++)
	{
		if (*ppEntry != NULL)
			hr = (*ppEntry)->RegisterClassObject(dwClsContext, dwFlags);
	}
	return hr;
}

ATLINLINE ATLAPI AtlComModuleRevokeClassObjects(_ATL_COM_MODULE* pComModule)
{
	ATLASSERT(pComModule != NULL);
	if (pComModule == NULL)
		return E_INVALIDARG;

	HRESULT hr = S_OK;
	for (_ATL_OBJMAP_ENTRY** ppEntry = pComModule->m_ppAutoObjMapFirst; ppEntry < pComModule->m_ppAutoObjMapLast && hr == S_OK; ppEntry++)
	{
		if (*ppEntry != NULL)
			hr = (*ppEntry)->RevokeClassObject();
	}
	return hr;
}

ATLINLINE ATLAPI_(BOOL) AtlWaitWithMessageLoop(HANDLE hEvent)
{
	DWORD dwRet;
	MSG msg;

	while(1)
	{
		dwRet = MsgWaitForMultipleObjects(1, &hEvent, FALSE, INFINITE, QS_ALLINPUT);

		if (dwRet == WAIT_OBJECT_0)
			return TRUE;    // The event was signaled

		if (dwRet != WAIT_OBJECT_0 + 1)
			break;          // Something else happened

		// There is one or more window message available. Dispatch them
		while(PeekMessage(&msg,0,0,0,PM_NOREMOVE))
		{
			// check for unicode window so we call the appropriate functions
			BOOL bUnicode = ::IsWindowUnicode(msg.hwnd);
			BOOL bRet;

			if (bUnicode)
				bRet = ::GetMessageW(&msg, NULL, 0, 0);
			else
				bRet = ::GetMessageA(&msg, NULL, 0, 0);

			if (bRet > 0)
			{
				::TranslateMessage(&msg);

				if (bUnicode)
					::DispatchMessageW(&msg);
				else
					::DispatchMessageA(&msg);
			}

			if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
				return TRUE; // Event is now signaled.
		}
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// QI support

ATLINLINE ATLAPI AtlInternalQueryInterface(void* pThis,
	const _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject)
{
	ATLASSERT(pThis != NULL);
	ATLASSERT(pEntries!= NULL);
	
	if(pThis == NULL || pEntries == NULL)
		return E_INVALIDARG ;
		
	// First entry in the com map should be a simple map entry
	ATLASSERT(pEntries->pFunc == _ATL_SIMPLEMAPENTRY);
	if (ppvObject == NULL)
		return E_POINTER;
	*ppvObject = NULL;
	if (InlineIsEqualUnknown(iid)) // use first interface
	{
			IUnknown* pUnk = (IUnknown*)((INT_PTR)pThis+pEntries->dw);
			pUnk->AddRef();
			*ppvObject = pUnk;
			return S_OK;
	}
	while (pEntries->pFunc != NULL)
	{
		BOOL bBlind = (pEntries->piid == NULL);
		if (bBlind || InlineIsEqualGUID(*(pEntries->piid), iid))
		{
			if (pEntries->pFunc == _ATL_SIMPLEMAPENTRY) //offset
			{
				ATLASSERT(!bBlind);
				IUnknown* pUnk = (IUnknown*)((INT_PTR)pThis+pEntries->dw);
				pUnk->AddRef();
				*ppvObject = pUnk;
				return S_OK;
			}
			else //actual function call
			{
				HRESULT hRes = pEntries->pFunc(pThis,
					iid, ppvObject, pEntries->dw);
				if (hRes == S_OK || (!bBlind && FAILED(hRes)))
					return hRes;
			}
		}
		pEntries++;
	}
	return E_NOINTERFACE;
}

ATLINLINE ATLAPI_(DWORD) AtlGetVersion(void* /* pReserved */)
{
	return _ATL_VER;
}

/////////////////////////////////////////////////////////////////////////////
// Windowing

ATLINLINE ATLAPI_(void) AtlWinModuleAddCreateWndData(_ATL_WIN_MODULE* pWinModule, _AtlCreateWndData* pData, void* pObject)
{
	if (pWinModule == NULL)
		_AtlRaiseException((DWORD)EXCEPTION_ACCESS_VIOLATION);

	ATLASSERT(pData != NULL && pObject != NULL);
	if(pData == NULL || pObject == NULL)
		_AtlRaiseException((DWORD)EXCEPTION_ACCESS_VIOLATION);
		
	pData->m_pThis = pObject;
	pData->m_dwThreadID = ::GetCurrentThreadId();
	CComCritSecLock<CComCriticalSection> lock(pWinModule->m_csWindowCreate, false);
	if (FAILED(lock.Lock()))
	{
		ATLTRACE(atlTraceWindowing, 0, _T("ERROR : Unable to lock critical section in AtlWinModuleAddCreateWndData\n"));
		ATLASSERT(0);
		return;
	}
	pData->m_pNext = pWinModule->m_pCreateWndList;
	pWinModule->m_pCreateWndList = pData;
}

ATLINLINE ATLAPI_(void*) AtlWinModuleExtractCreateWndData(_ATL_WIN_MODULE* pWinModule)
{
	if (pWinModule == NULL)
		return NULL;

	void* pv = NULL;
	CComCritSecLock<CComCriticalSection> lock(pWinModule->m_csWindowCreate, false);
	if (FAILED(lock.Lock()))
	{
		ATLTRACE(atlTraceWindowing, 0, _T("ERROR : Unable to lock critical section in AtlWinModuleExtractCreateWndData\n"));
		ATLASSERT(0);
		return pv;
	}
	_AtlCreateWndData* pEntry = pWinModule->m_pCreateWndList;
	if(pEntry != NULL)
	{
		DWORD dwThreadID = ::GetCurrentThreadId();
		_AtlCreateWndData* pPrev = NULL;
		while(pEntry != NULL)
		{
			if(pEntry->m_dwThreadID == dwThreadID)
			{
				if(pPrev == NULL)
					pWinModule->m_pCreateWndList = pEntry->m_pNext;
				else
					pPrev->m_pNext = pEntry->m_pNext;
				pv = pEntry->m_pThis;
				break;
			}
			pPrev = pEntry;
			pEntry = pEntry->m_pNext;
		}
	}
	return pv;
}


ATLINLINE ATLAPI AtlWinModuleInit(_ATL_WIN_MODULE* pWinModule)
{
	if (pWinModule == NULL)
		return E_INVALIDARG;
		
	// check only in the DLL
	if (pWinModule->cbSize != sizeof(_ATL_WIN_MODULE))
		return E_INVALIDARG;

	pWinModule->m_pCreateWndList = NULL;

	HRESULT hr = pWinModule->m_csWindowCreate.Init();
	if (FAILED(hr))
	{
		ATLTRACE(atlTraceWindowing, 0, _T("ERROR : Unable to initialize critical section in AtlWinModuleInit\n"));
		ATLASSERT(0);
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Module

ATLINLINE ATLAPI AtlModuleAddTermFunc(_ATL_MODULE* pModule, _ATL_TERMFUNC* pFunc, DWORD_PTR dw)
{
	if (pModule == NULL)
		return E_INVALIDARG;
		
	HRESULT hr = S_OK;
	_ATL_TERMFUNC_ELEM* pNew = NULL;
	ATLTRY(pNew = new _ATL_TERMFUNC_ELEM);
	if (pNew == NULL)
		hr = E_OUTOFMEMORY;
	else
	{
		pNew->pFunc = pFunc;
		pNew->dw = dw;
		CComCritSecLock<CComCriticalSection> lock(pModule->m_csStaticDataInitAndTypeInfo, false);
		hr = lock.Lock();
		if (SUCCEEDED(hr))
		{
			pNew->pNext = pModule->m_pTermFuncs;
			pModule->m_pTermFuncs = pNew;
		}
		else
		{
			delete pNew;
			ATLTRACE(atlTraceGeneral, 0, _T("ERROR : Unable to lock critical section in AtlModuleAddTermFunc\n"));
			ATLASSERT(0);
		}
	}
	return hr;
}

ATLINLINE ATLAPI_(void) AtlCallTermFunc(_ATL_MODULE* pModule)
{
	if (pModule == NULL)
		_AtlRaiseException((DWORD)EXCEPTION_ACCESS_VIOLATION);
		
	_ATL_TERMFUNC_ELEM* pElem = pModule->m_pTermFuncs;
	_ATL_TERMFUNC_ELEM* pNext = NULL;
	while (pElem != NULL)
	{
		pElem->pFunc(pElem->dw);
		pNext = pElem->pNext;
		delete pElem;
		pElem = pNext;
	}
	pModule->m_pTermFuncs = NULL;
}

} // namespace ATL
#pragma warning(pop)
#endif // __ATLBASE_INL__
