// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.

#ifndef __ATLBASE_H__
#define __ATLBASE_H__

#pragma once

// Warnings outside of the push/pop sequence will be disabled for all user 
// projects.  The only warnings that should be disabled outside the push/pop
// are warnings that are a) benign and b) will show up in user projects 
// without being directly caused by the user

#pragma warning(disable: 4505) // unreferenced local function has been removed
#pragma warning(disable: 4710) // function couldn't be inlined
#pragma warning(disable: 4514) // unreferenced inlines are common

// These two warnings will occur in any class that contains or derives from a
// class with a private copy constructor or copy assignment operator.
#pragma warning(disable: 4511) // copy constructor could not be generated
#pragma warning(disable: 4512) // assignment operator could not be generated
#pragma warning(disable: 4355) // 'this' : used in base member initializer list

#ifdef _ATL_ALL_WARNINGS
#pragma warning( push )
#endif

#pragma warning(disable : 4668)	// is not defined as a preprocessor macro, replacing with '0' for '#if/#elif
#pragma warning(disable : 4820)	// padding added after member
#pragma warning(disable : 4917)	// a GUID can only be associated with a class, interface or namespace

#pragma warning(disable : 4217)	// member template functions cannot be used for copy-assignment or copy-construction

#pragma warning(disable: 4127) // constant expression
#pragma warning(disable: 4097) // typedef name used as synonym for class-name
#pragma warning(disable: 4786) // identifier was truncated in the debug information
#pragma warning(disable: 4291) // allow placement new
#pragma warning(disable: 4201) // nameless unions are part of C++
#pragma warning(disable: 4103) // pragma pack
#pragma warning(disable: 4268) // const static/global data initialized to zeros

#pragma warning (push)

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif
#ifndef ATL_NO_LEAN_AND_MEAN
#define ATL_NO_LEAN_AND_MEAN
#endif

#include <atldef.h>

#ifndef _WINSOCKAPI_
#include <winsock2.h>
#endif

#include <atlcore.h>
#include <ole2.h>
#include <atlcomcli.h>

#include <comcat.h>
#include <stddef.h>

#include <tchar.h>
#include <limits.h>

#include <olectl.h>
#include <atliface.h>

#if !defined(_ATL_MIN_CRT) & defined(_MT)
#include <errno.h>
#include <process.h>    // for _beginthreadex, _endthreadex
#endif

#ifdef _DEBUG
#include <stdio.h>
#include <stdarg.h>
#endif

#include <atlconv.h>
#include <shlwapi.h>
#include <atlsimpcoll.h>
#include <atltrace.h>
#include <atlexcept.h>

#define _ATL_TYPELIB_INDEX_LENGTH 10
#define _ATL_QUOTES_SPACE 2

#pragma pack(push, _ATL_PACKING)

#ifndef _ATL_NO_DEFAULT_LIBS

#if defined(_ATL_DLL)
	#pragma comment(lib, "atl.lib")
#endif

#ifdef _DEBUG
	#pragma comment(lib, "atlsd.lib")
#else
	#pragma comment(lib, "atls.lib")
#ifdef _ATL_MIN_CRT
	#pragma comment(lib, "atlmincrt.lib")
#endif
#endif

#endif  // !_ATL_NO_DEFAULT_LIBS

extern "C" const __declspec(selectany) GUID LIBID_ATLLib = 					{0x44EC0535,0x400F,0x11D0,{0x9D,0xCD,0x00,0xA0,0xC9,0x03,0x91,0xD3}};
extern "C" const __declspec(selectany) CLSID CLSID_Registrar = 				{0x44EC053A,0x400F,0x11D0,{0x9D,0xCD,0x00,0xA0,0xC9,0x03,0x91,0xD3}};
extern "C" const __declspec(selectany) IID IID_IRegistrar = 				{0x44EC053B,0x400F,0x11D0,{0x9D,0xCD,0x00,0xA0,0xC9,0x03,0x91,0xD3}};
extern "C" const __declspec(selectany) IID IID_IAxWinHostWindow = 			{0xb6ea2050,0x048a,0x11d1,{0x82,0xb9,0x00,0xc0,0x4f,0xb9,0x94,0x2e}};
extern "C" const __declspec(selectany) IID IID_IAxWinAmbientDispatch = 		{0xb6ea2051,0x048a,0x11d1,{0x82,0xb9,0x00,0xc0,0x4f,0xb9,0x94,0x2e}};
extern "C" const __declspec(selectany) IID IID_IInternalConnection = 		{0x72AD0770,0x6A9F,0x11d1,{0xBC,0xEC,0x00,0x60,0x08,0x8F,0x44,0x4E}};
extern "C" const __declspec(selectany) IID IID_IDocHostUIHandlerDispatch = 	{0x425B5AF0,0x65F1,0x11d1,{0x96,0x11,0x00,0x00,0xF8,0x1E,0x0D,0x0D}};
extern "C" const __declspec(selectany) IID IID_IAxWinHostWindowLic = 		{0x3935BDA8,0x4ED9,0x495c,{0x86,0x50,0xE0,0x1F,0xC1,0xE3,0x8A,0x4B}};
extern "C" const __declspec(selectany) IID IID_IAxWinAmbientDispatchEx = 	{0xB2D0778B,0xAC99,0x4c58,{0xA5,0xC8,0xE7,0x72,0x4E,0x53,0x16,0xB5}};

// REVIEW: Temp until it gets back into UUID.LIB
extern "C" const __declspec(selectany) CLSID CLSID_StdGlobalInterfaceTable = {0x00000323,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};


#ifndef _delayimp_h
extern "C" IMAGE_DOS_HEADER __ImageBase;
#endif

#ifdef _AFX
void AFXAPI AfxOleLockApp();
void AFXAPI AfxOleUnlockApp();
#endif	// _AFX

namespace ATL
{

struct _ATL_CATMAP_ENTRY
{
   int iType;
   const CATID* pcatid;
};

#define _ATL_CATMAP_ENTRY_END 0
#define _ATL_CATMAP_ENTRY_IMPLEMENTED 1
#define _ATL_CATMAP_ENTRY_REQUIRED 2

typedef HRESULT (WINAPI _ATL_CREATORFUNC)(void* pv, REFIID riid, LPVOID* ppv);
typedef HRESULT (WINAPI _ATL_CREATORARGFUNC)(void* pv, REFIID riid, LPVOID* ppv, DWORD_PTR dw);
typedef HRESULT (WINAPI _ATL_MODULEFUNC)(DWORD_PTR dw);
typedef LPCTSTR (WINAPI _ATL_DESCRIPTIONFUNC)();
typedef const struct _ATL_CATMAP_ENTRY* (_ATL_CATMAPFUNC)();
typedef void (__stdcall _ATL_TERMFUNC)(DWORD_PTR dw);

struct _ATL_TERMFUNC_ELEM
{
	_ATL_TERMFUNC* pFunc;
	DWORD_PTR dw;
	_ATL_TERMFUNC_ELEM* pNext;
};

/*
struct _ATL_OBJMAP_ENTRY20
{
	const CLSID* pclsid;
	HRESULT (WINAPI *pfnUpdateRegistry)(BOOL bRegister);
	_ATL_CREATORFUNC* pfnGetClassObject;
	_ATL_CREATORFUNC* pfnCreateInstance;
	IUnknown* pCF;
	DWORD dwRegister;
	_ATL_DESCRIPTIONFUNC* pfnGetObjectDescription;
};
*/

// Can't inherit from _ATL_OBJMAP_ENTRY20 
// because it messes up the OBJECT_MAP macros
struct _ATL_OBJMAP_ENTRY30 
{
	const CLSID* pclsid;
	HRESULT (WINAPI *pfnUpdateRegistry)(BOOL bRegister);
	_ATL_CREATORFUNC* pfnGetClassObject;
	_ATL_CREATORFUNC* pfnCreateInstance;
	IUnknown* pCF;
	DWORD dwRegister;
	_ATL_DESCRIPTIONFUNC* pfnGetObjectDescription;
	_ATL_CATMAPFUNC* pfnGetCategoryMap;
	HRESULT WINAPI RevokeClassObject()
	{
		if (dwRegister == 0)
			return S_OK;
		return CoRevokeClassObject(dwRegister);
	}
	HRESULT WINAPI RegisterClassObject(DWORD dwClsContext, DWORD dwFlags)
	{
		IUnknown* p = NULL;
		if (pfnGetClassObject == NULL)
			return S_OK;
		HRESULT hRes = pfnGetClassObject(pfnCreateInstance, __uuidof(IUnknown), (LPVOID*) &p);
		if (SUCCEEDED(hRes))
			hRes = CoRegisterClassObject(*pclsid, p, dwClsContext, dwFlags, &dwRegister);
		if (p != NULL)
			p->Release();
		return hRes;
	}
// Added in ATL 3.0
	void (WINAPI *pfnObjectMain)(bool bStarting);
};

typedef _ATL_OBJMAP_ENTRY30 _ATL_OBJMAP_ENTRY;

// Auto Object Map

#if defined(_M_IA64) || defined(_M_IX86)

#pragma section("ATL$__a", read, shared)
#pragma section("ATL$__z", read, shared)
#pragma section("ATL$__m", read, shared)
extern "C"
{
__declspec(selectany) __declspec(allocate("ATL$__a")) _ATL_OBJMAP_ENTRY* __pobjMapEntryFirst = NULL;
__declspec(selectany) __declspec(allocate("ATL$__z")) _ATL_OBJMAP_ENTRY* __pobjMapEntryLast = NULL;
}

#if !defined(_M_IA64)
#pragma comment(linker, "/merge:ATL=.rdata")
#endif

#else

extern "C"
{
__declspec(selectany) _ATL_OBJMAP_ENTRY* __pobjMapEntryFirst = NULL;
__declspec(selectany) _ATL_OBJMAP_ENTRY* __pobjMapEntryLast = NULL;
}

#endif  // defined(_M_IA64) || defined(_M_IX86)

struct _ATL_REGMAP_ENTRY
{
	LPCOLESTR     szKey;
	LPCOLESTR     szData;
};

struct _AtlCreateWndData
{
	void* m_pThis;
	DWORD m_dwThreadID;
	_AtlCreateWndData* m_pNext;
};


// perfmon registration/unregistration function definitions
typedef HRESULT (*_ATL_PERFREGFUNC)(HINSTANCE hDllInstance);
typedef HRESULT (*_ATL_PERFUNREGFUNC)();
__declspec(selectany) _ATL_PERFREGFUNC _pPerfRegFunc = NULL;
__declspec(selectany) _ATL_PERFUNREGFUNC _pPerfUnRegFunc = NULL;

/////////////////////////////////////////////////////////////////////////////
// Threading Model Support

template< class TLock >
class CComCritSecLock
{
public:
	CComCritSecLock( TLock& cs, bool bInitialLock = true );
	~CComCritSecLock() throw();

	HRESULT Lock() throw();
	void Unlock() throw();

// Implementation
private:
	TLock& m_cs;
	bool m_bLocked;

// Private to avoid accidental use
	CComCritSecLock( const CComCritSecLock& ) throw();
	CComCritSecLock& operator=( const CComCritSecLock& ) throw();
};

template< class TLock >
inline CComCritSecLock< TLock >::CComCritSecLock( TLock& cs, bool bInitialLock ) :
	m_cs( cs ),
	m_bLocked( false )
{
	if( bInitialLock )
	{
		HRESULT hr;

		hr = Lock();
		if( FAILED( hr ) )
		{
			AtlThrow( hr );
		}
	}
}

template< class TLock >
inline CComCritSecLock< TLock >::~CComCritSecLock() throw()
{
	if( m_bLocked )
	{
		Unlock();
	}
}

template< class TLock >
inline HRESULT CComCritSecLock< TLock >::Lock() throw()
{
	HRESULT hr;

	ATLASSERT( !m_bLocked );
	hr = m_cs.Lock();
	if( FAILED( hr ) )
	{
		return( hr );
	}
	m_bLocked = true;

	return( S_OK );
}

template< class TLock >
inline void CComCritSecLock< TLock >::Unlock() throw()
{
	ATLASSERT( m_bLocked );
	m_cs.Unlock();
	m_bLocked = false;
}

class CComMultiThreadModelNoCS
{
public:
	static ULONG WINAPI Increment(LPLONG p) throw() {return InterlockedIncrement(p);}
	static ULONG WINAPI Decrement(LPLONG p) throw() {return InterlockedDecrement(p);}
	typedef CComFakeCriticalSection AutoCriticalSection;
	typedef CComFakeCriticalSection CriticalSection;
	typedef CComMultiThreadModelNoCS ThreadModelNoCS;
};

class CComMultiThreadModel
{
public:
	static ULONG WINAPI Increment(LPLONG p) throw() {return InterlockedIncrement(p);}
	static ULONG WINAPI Decrement(LPLONG p) throw() {return InterlockedDecrement(p);}
	typedef CComAutoCriticalSection AutoCriticalSection;
	typedef CComCriticalSection CriticalSection;
	typedef CComMultiThreadModelNoCS ThreadModelNoCS;
};

class CComSingleThreadModel
{
public:
	static ULONG WINAPI Increment(LPLONG p) throw() {return ++(*p);}
	static ULONG WINAPI Decrement(LPLONG p) throw() {return --(*p);}
	typedef CComFakeCriticalSection AutoCriticalSection;
	typedef CComFakeCriticalSection CriticalSection;
	typedef CComSingleThreadModel ThreadModelNoCS;
};

#if defined(_ATL_SINGLE_THREADED)

#if defined(_ATL_APARTMENT_THREADED) || defined(_ATL_FREE_THREADED)
#pragma message ("More than one global threading model defined.")
#endif

	typedef CComSingleThreadModel CComObjectThreadModel;
	typedef CComSingleThreadModel CComGlobalsThreadModel;

#elif defined(_ATL_APARTMENT_THREADED)

#if defined(_ATL_SINGLE_THREADED) || defined(_ATL_FREE_THREADED)
#pragma message ("More than one global threading model defined.")
#endif

	typedef CComSingleThreadModel CComObjectThreadModel;
	typedef CComMultiThreadModel CComGlobalsThreadModel;

#elif defined(_ATL_FREE_THREADED)

#if defined(_ATL_SINGLE_THREADED) || defined(_ATL_APARTMENT_THREADED)
#pragma message ("More than one global threading model defined.")
#endif

	typedef CComMultiThreadModel CComObjectThreadModel;
	typedef CComMultiThreadModel CComGlobalsThreadModel;

#else
#pragma message ("No global threading model defined")
#endif

/////////////////////////////////////////////////////////////////////////////
// Module 

// Used by COM related code in ATL
struct _ATL_COM_MODULE70
{
	UINT cbSize;
	HINSTANCE m_hInstTypeLib;
	_ATL_OBJMAP_ENTRY** m_ppAutoObjMapFirst;
	_ATL_OBJMAP_ENTRY** m_ppAutoObjMapLast;
	CComCriticalSection m_csObjMap;
};
typedef _ATL_COM_MODULE70 _ATL_COM_MODULE;


// Used by Windowing code in ATL
struct _ATL_WIN_MODULE70
{
	UINT cbSize;
	CComCriticalSection m_csWindowCreate;
	_AtlCreateWndData* m_pCreateWndList;
	CSimpleArray<ATOM> m_rgWindowClassAtoms;
};
typedef _ATL_WIN_MODULE70 _ATL_WIN_MODULE;


struct _ATL_MODULE70
{
	UINT cbSize;
	LONG m_nLockCnt;
	_ATL_TERMFUNC_ELEM* m_pTermFuncs;
	CComCriticalSection m_csStaticDataInitAndTypeInfo;
};
typedef _ATL_MODULE70 _ATL_MODULE;

/////////////////////////////////////////////////////////////////////////////
//This define makes debugging asserts easier.
#define _ATL_SIMPLEMAPENTRY ((ATL::_ATL_CREATORARGFUNC*)1)

struct _ATL_INTMAP_ENTRY
{
	const IID* piid;       // the interface id (IID)
	DWORD_PTR dw;
	_ATL_CREATORARGFUNC* pFunc; //NULL:end, 1:offset, n:ptr
};

/////////////////////////////////////////////////////////////////////////////
// Thunks for __stdcall member functions

#if defined(_M_IX86)
#pragma pack(push,1)
struct _stdcallthunk
{
	DWORD   m_mov;          // mov dword ptr [esp+0x4], pThis (esp+0x4 is hWnd)
	DWORD   m_this;         //
	BYTE    m_jmp;          // jmp WndProc
	DWORD   m_relproc;      // relative jmp
	void Init(DWORD_PTR proc, void* pThis)
	{
		m_mov = 0x042444C7;  //C7 44 24 0C
		m_this = PtrToUlong(pThis);
		m_jmp = 0xe9;
		m_relproc = DWORD((INT_PTR)proc - ((INT_PTR)this+sizeof(_stdcallthunk)));
		// write block from data cache and
		//  flush from instruction cache
		FlushInstructionCache(GetCurrentProcess(), this, sizeof(_stdcallthunk));
	}
	//some thunks will dynamically allocate the memory for the code
	void* GetCodeAddress()
	{
		return this;
	}
};
#pragma pack(pop)
#elif defined (_M_ALPHA)
// For ALPHA we will stick the this pointer into a0, which is where
// the HWND is.  However, we don't actually need the HWND so this is OK.
#pragma pack(push,4)
struct _stdcallthunk //this should come out to 20 bytes
{
	DWORD ldah_at;      //  ldah    at, HIWORD(func)
	DWORD ldah_a0;      //  ldah    a0, HIWORD(this)
	DWORD lda_at;       //  lda     at, LOWORD(func)(at)
	DWORD lda_a0;       //  lda     a0, LOWORD(this)(a0)
	DWORD jmp;          //  jmp     zero,(at),0
	void Init(DWORD_PTR proc, void* pThis)
	{
		ldah_at = (0x279f0000 | HIWORD(proc)) + (LOWORD(proc)>>15);
		ldah_a0 = (0x261f0000 | HIWORD(pThis)) + (LOWORD(pThis)>>15);
		lda_at = 0x239c0000 | LOWORD(proc);
		lda_a0 = 0x22100000 | LOWORD(pThis);
		jmp = 0x6bfc0000;
		// write block from data cache and
		//  flush from instruction cache
		FlushInstructionCache(GetCurrentProcess(), this, sizeof(_stdcallthunk));
	}
	void* GetCodeAddress()
	{
		return this;
	}
};
#pragma pack(pop)
#elif defined(_SH3_)
#pragma pack(push,4)
struct _stdcallthunk // this should come out to 16 bytes
{
	WORD	m_mov_r0;		// mov.l	pFunc,r0
	WORD	m_mov_r1;		// mov.l	pThis,r1
	WORD	m_jmp;			// jmp		@r0
	WORD	m_nop;			// nop
	DWORD	m_pFunc;
	DWORD	m_pThis;
	void Init(DWORD_PTR proc, void* pThis)
	{
		m_mov_r0 = 0xd001;
		m_mov_r1 = 0xd402;
		m_jmp = 0x402b;
		m_nop = 0x0009;
		m_pFunc = (DWORD)proc;
		m_pThis = (DWORD)pThis;
		// write block from data cache and
		//  flush from instruction cache
		FlushInstructionCache(GetCurrentProcess(), this, sizeof(_stdcallthunk));
	}
	void* GetCodeAddress()
	{
		return this;
	}
};
#pragma pack(pop)
#elif defined(_MIPS_)
#pragma pack(push,4)
struct _stdcallthunk
{
	WORD	m_pFuncHi;
	WORD	m_lui_t0;		// lui		t0,PFUNC_HIGH
	WORD	m_pFuncLo;
	WORD	m_ori_t0;		// ori		t0,t0,PFUNC_LOW
	WORD	m_pThisHi;
	WORD	m_lui_a0;		// lui		a0,PTHIS_HIGH
	DWORD	m_jr_t0;		// jr		t0
	WORD	m_pThisLo;
	WORD	m_ori_a0;		// ori		a0,PTHIS_LOW
	void Init(DWORD_PTR proc, void* pThis)
	{
		m_pFuncHi = HIWORD(proc);
		m_lui_t0  = 0x3c08;
		m_pFuncLo = LOWORD(proc);
		m_ori_t0  = 0x3508;
		m_pThisHi = HIWORD(pThis);
		m_lui_a0  = 0x3c04;
		m_jr_t0   = 0x01000008;
		m_pThisLo = LOWORD(pThis);
		m_ori_a0  = 0x3484;
		// write block from data cache and
		//  flush from instruction cache
		FlushInstructionCache(GetCurrentProcess(), this, sizeof(_stdcallthunk));
	}
	void* GetCodeAddress()
	{
		return this;
	}
};
#pragma pack(pop)
#elif defined(_ARM_)
#pragma pack(push,4)
struct _stdcallthunk // this should come out to 16 bytes
{
	DWORD	m_mov_r0;		// mov	r0, pThis
	DWORD	m_mov_pc;		// mov	pc, pFunc
	DWORD	m_pThis;
	DWORD	m_pFunc;
	void Init(DWORD_PTR proc, void* pThis)
	{
		m_mov_r0 = 0xE59F0000;
		m_mov_pc = 0xE59FF000;
		m_pThis = (DWORD)pThis;
		m_pFunc = (DWORD)proc;
		// write block from data cache and
		//  flush from instruction cache
		FlushInstructionCache(GetCurrentProcess(), this, sizeof(_stdcallthunk));
	}
	void* GetCodeAddress()
	{
		return this;
	}
};
#pragma pack(pop)
#elif defined(_M_IA64)
#pragma pack(push,8)
#pragma section(".base", long, read, write)  // Declare section to place _StdCallThunkProc in.  Must be 'long'
extern "C" __declspec( allocate( ".base" ) ) void* _StdCallThunkProc;  // Actually, a global label exported from StdCallThunk.s
struct _FuncDesc
{
	void* pfn;
	void* gp;
};
struct _stdcallthunk
{
	_FuncDesc m_funcdesc;
	void* m_pFunc;
	void* m_pThis;
	void Init(DWORD_PTR proc, void* pThis)
	{
		m_funcdesc.pfn = &_StdCallThunkProc;  // Pointer to actual beginning of StdCallThunkProc
		m_funcdesc.gp = &m_pFunc;
		m_pFunc = reinterpret_cast< void* >( proc );
		m_pThis = pThis;
		::FlushInstructionCache( GetCurrentProcess(), this, sizeof( _stdcallthunk ) );
	}
	void* GetCodeAddress()
	{
		return( &m_funcdesc );
	}
};
#pragma pack(pop)
#else
#error Only ARM, ALPHA, SH3, MIPS, IA64, and X86 supported
#endif

#if defined(_M_IX86)
class CDynamicStdCallThunk
{
public:
	_stdcallthunk *pThunk;

	CDynamicStdCallThunk()
	{
		pThunk = NULL;
	}

	~CDynamicStdCallThunk()
	{
		if (pThunk)
			HeapFree(GetProcessHeap(), 0, pThunk);
	}

	void Init(DWORD_PTR proc, void *pThis)
	{
		if (pThunk == NULL)
			pThunk = static_cast<_stdcallthunk *>(HeapAlloc(GetProcessHeap(), 
				HEAP_GENERATE_EXCEPTIONS, sizeof(_stdcallthunk)));
		ATLASSERT(pThunk);
		pThunk->Init(proc, pThis);
	}

	void* GetCodeAddress()
	{
		ATLASSERT(pThunk);
		return pThunk->GetCodeAddress();
	}
};
typedef CDynamicStdCallThunk CStdCallThunk;
#else
typedef _stdcallthunk CStdCallThunk;
#endif  // _M_IX86

/////////////////////////////////////////////////////////////////////////////
// Global Functions

/////////////////////////////////////////////////////////////////////////////
// QI Support

ATLAPI AtlInternalQueryInterface(void* pThis,
	const _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject);

/////////////////////////////////////////////////////////////////////////////
// Inproc Marshaling helpers

ATLAPI AtlFreeMarshalStream(IStream* pStream);
ATLAPI AtlMarshalPtrInProc(IUnknown* pUnk, const IID& iid, IStream** ppStream);
ATLAPI AtlUnmarshalPtr(IStream* pStream, const IID& iid, IUnknown** ppUnk);

ATLAPI_(BOOL) AtlWaitWithMessageLoop(HANDLE hEvent);

/////////////////////////////////////////////////////////////////////////////
// Connection Point Helpers

ATLAPI AtlAdvise(IUnknown* pUnkCP, IUnknown* pUnk, const IID& iid, LPDWORD pdw);
ATLAPI AtlUnadvise(IUnknown* pUnkCP, const IID& iid, DWORD dw);

/////////////////////////////////////////////////////////////////////////////
// IDispatch Error handling

ATLAPI AtlSetErrorInfo(const CLSID& clsid, LPCOLESTR lpszDesc,
	DWORD dwHelpID, LPCOLESTR lpszHelpFile, const IID& iid, HRESULT hRes,
	HINSTANCE hInst);

/////////////////////////////////////////////////////////////////////////////
// Module

ATLAPI AtlComModuleRegisterClassObjects(_ATL_COM_MODULE* pComModule, DWORD dwClsContext, DWORD dwFlags);
ATLAPI AtlComModuleRevokeClassObjects(_ATL_COM_MODULE* pComModule);

ATLAPI AtlComModuleGetClassObject(_ATL_COM_MODULE* pComModule, REFCLSID rclsid, REFIID riid, LPVOID* ppv);

ATLAPI AtlComModuleRegisterServer(_ATL_COM_MODULE* pComModule, BOOL bRegTypeLib, const CLSID* pCLSID = NULL);
ATLAPI AtlComModuleUnregisterServer(_ATL_COM_MODULE* pComModule, BOOL bUnRegTypeLib, const CLSID* pCLSID = NULL);

ATLAPI AtlRegisterClassCategoriesHelper( REFCLSID clsid, const struct _ATL_CATMAP_ENTRY* pCatMap, BOOL bRegister );

ATLAPI AtlUpdateRegistryFromResourceD(HINSTANCE hInst, LPCOLESTR lpszRes,
	BOOL bRegister, struct _ATL_REGMAP_ENTRY* pMapEntries, IRegistrar* pReg = NULL);

ATLAPI AtlRegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex);
ATLAPI AtlUnRegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex);
ATLAPI AtlLoadTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex, BSTR* pbstrPath, ITypeLib** ppTypeLib);

ATLAPI_(DWORD) AtlGetVersion(void* pReserved);

ATLAPI AtlModuleAddTermFunc(_ATL_MODULE* pModule, _ATL_TERMFUNC* pFunc, DWORD_PTR dw);
ATLAPI_(void) AtlCallTermFunc(_ATL_MODULE* pModule);

ATLAPI AtlWinModuleInit(_ATL_WIN_MODULE* pWinModule);
ATLAPI AtlWinModuleTerm(_ATL_WIN_MODULE* pWinModule, HINSTANCE hInst);
ATLAPI_(void) AtlWinModuleAddCreateWndData(_ATL_WIN_MODULE* pWinModule, _AtlCreateWndData* pData, void* pObject);
ATLAPI_(void*) AtlWinModuleExtractCreateWndData(_ATL_WIN_MODULE* pWinModule);

/////////////////////////////////////////////////////////////////////////////
// Get Registrar object from ATL DLL.

#if !defined(_ATL_STATIC_REGISTRY)
#ifdef _ATL_DLL_IMPL
extern "C" HRESULT __stdcall AtlCreateRegistrar(IRegistrar** ppReg);
#else
extern "C" __declspec(dllimport) HRESULT __stdcall AtlCreateRegistrar(IRegistrar** ppReg);
#endif
#endif

/////////////////////////////////////////////////////////////////////////////
// GUID comparison
inline BOOL WINAPI InlineIsEqualUnknown(REFGUID rguid1)
{
   return (
	  ((PLONG) &rguid1)[0] == 0 &&
	  ((PLONG) &rguid1)[1] == 0 &&
#ifdef _ATL_BYTESWAP
	  ((PLONG) &rguid1)[2] == 0xC0000000 &&
	  ((PLONG) &rguid1)[3] == 0x00000046);
#else
	  ((PLONG) &rguid1)[2] == 0x000000C0 &&
	  ((PLONG) &rguid1)[3] == 0x46000000);
#endif
}



template <class T>
LPCTSTR AtlDebugGetClassName(T*)
{
#ifdef _DEBUG
	const _ATL_INTMAP_ENTRY* pEntries = T::_GetEntries();
	return (LPCTSTR)pEntries[-1].dw;
#else
	return NULL;
#endif
}

// Validation macro for OUT pointer
// Used in QI and CreateInstance
#define _ATL_VALIDATE_OUT_POINTER(x)	ATLASSERT(x != NULL);	\
	if (x == NULL)	\
		return E_POINTER;	\
	*x = NULL

/////////////////////////////////////////////////////////////////////////////
// Win32 libraries

#ifndef _ATL_NO_DEFAULT_LIBS
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "shlwapi.lib")
#endif  // !_ATL_NO_DEFAULT_LIBS

template< typename T >
class CAutoVectorPtr
{
public:
	CAutoVectorPtr() throw() :
		m_p( NULL )
	{
	}
	CAutoVectorPtr( CAutoVectorPtr< T >& p ) throw()
	{
		m_p = p.Detach();  // Transfer ownership
	}
	explicit CAutoVectorPtr( T* p ) throw() :
		m_p( p )
	{
	}
	~CAutoVectorPtr() throw()
	{
		Free();
	}

	operator T*() const throw()
	{
		return( m_p );
	}

	CAutoVectorPtr< T >& operator=( CAutoVectorPtr< T >& p ) throw()
	{
		Free();
		Attach( p.Detach() );  // Transfer ownership

		return( *this );
	}

	// Allocate the vector
	bool Allocate( size_t nElements ) throw()
	{
		ATLASSERT( m_p == NULL );
		ATLTRY( m_p = new T[nElements] );
		if( m_p == NULL )
		{
			return( false );
		}

		return( true );
	}
	// Attach to an existing pointer (takes ownership)
	void Attach( T* p ) throw()
	{
		ATLASSERT( m_p == NULL );
		m_p = p;
	}
	// Detach the pointer (releases ownership)
	T* Detach() throw()
	{
		T* p;

		p = m_p;
		m_p = NULL;

		return( p );
	}
	// Delete the vector pointed to, and set the pointer to NULL
	void Free() throw()
	{
		delete[] m_p;
		m_p = NULL;
	}

public:
	T* m_p;
};

template< typename T >
class CAutoPtr
{
public:
	CAutoPtr() throw() :
		m_p( NULL )
	{
	}
	template< typename TSrc >
	CAutoPtr( CAutoPtr< TSrc >& p ) throw()
	{
		m_p = p.Detach();  // Transfer ownership
	}
	template<>
	CAutoPtr( CAutoPtr< T >& p ) throw()
	{
		m_p = p.Detach();  // Transfer ownership
	}
	explicit CAutoPtr( T* p ) throw() :
		m_p( p )
	{
	}
	~CAutoPtr() throw()
	{
		Free();
	}

	// Templated version to allow pBase = pDerived
	template< typename TSrc >
	CAutoPtr< T >& operator=( CAutoPtr< TSrc >& p ) throw()
	{
		Free();
		Attach( p.Detach() );  // Transfer ownership

		return( *this );
	}
	template<>
	CAutoPtr< T >& operator=( CAutoPtr< T >& p ) throw()
	{
		Free();
		Attach( p.Detach() );  // Transfer ownership

		return( *this );
	}

	operator T*() const throw()
	{
		return( m_p );
	}
	T* operator->() const throw()
	{
		ATLASSERT( m_p != NULL );
		return( m_p );
	}

	// Attach to an existing pointer (takes ownership)
	void Attach( T* p ) throw()
	{
		ATLASSERT( m_p == NULL );
		m_p = p;
	}
	// Detach the pointer (releases ownership)
	T* Detach() throw()
	{
		T* p;

		p = m_p;
		m_p = NULL;

		return( p );
	}
	// Delete the object pointed to, and set the pointer to NULL
	void Free() throw()
	{
		delete m_p;
		m_p = NULL;
	}

public:
	T* m_p;
};

// static_cast_auto template functions.  Used like static_cast, only they work on CAutoPtr objects
template< class Dest, class Src >
Dest* static_cast_auto( const CAutoPtr< Src >& pSrc ) throw()
{
	return( static_cast< Dest* >( static_cast< Src* >( pSrc ) ) );
}


class CComAllocator 
{
public:
	static void* Reallocate(void* p, size_t nBytes) throw()
	{
#ifdef _WIN64
		if( nBytes > INT_MAX )
		{
			return( NULL );
		}
#endif
		return ::CoTaskMemRealloc(p, ULONG(nBytes));
	}
	static void* Allocate(size_t nBytes) throw()
	{
#ifdef _WIN64
		if( nBytes > INT_MAX )
		{
			return( NULL );
		}
#endif
		return ::CoTaskMemAlloc(ULONG(nBytes));
	}
	static void Free(void* p) throw()
	{
		::CoTaskMemFree(p);
	}
};

template <typename T>
class CComHeapPtr :
	public CHeapPtr<T, CComAllocator>
{
public:
	CComHeapPtr() throw()
	{
	}

	explicit CComHeapPtr(T* pData) throw() :
		CHeapPtr<T, CComAllocator>(pData)
	{
	}
};

template <class T, class Reallocator>
T* AtlSafeRealloc(T* pT, size_t cEls) throw()
{
	T* pTemp;

	pTemp = static_cast<T*>(Reallocator::Reallocate(pT, cEls*sizeof(T)));
	if (pTemp == NULL)
	{
		Reallocator::Free(pT);
		return NULL;
	}
	pT = pTemp;
	return pTemp;
}

class CHandle
{
public:
	CHandle() throw();
	CHandle( CHandle& h ) throw();
	explicit CHandle( HANDLE h ) throw();
	~CHandle() throw();

	CHandle& operator=( CHandle& h ) throw();

	operator HANDLE() const throw();

	// Attach to an existing handle (takes ownership).
	void Attach( HANDLE h ) throw();
	// Detach the handle from the object (releases ownership).
	HANDLE Detach() throw();

	// Close the handle.
	void Close() throw();

public:
	HANDLE m_h;
};

inline CHandle::CHandle() throw() :
	m_h( NULL )
{
}

inline CHandle::CHandle( CHandle& h ) throw() :
	m_h( NULL )
{
	Attach( h.Detach() );
}

inline CHandle::CHandle( HANDLE h ) throw() :
	m_h( h )
{
}

inline CHandle::~CHandle() throw()
{
	if( m_h != NULL )
	{
		Close();
	}
}

inline CHandle& CHandle::operator=( CHandle& h ) throw()
{
	if( this != &h )
	{
		if( m_h != NULL )
		{
			Close();
		}
		Attach( h.Detach() );
	}

	return( *this );
}

inline CHandle::operator HANDLE() const throw()
{
	return( m_h );
}

inline void CHandle::Attach( HANDLE h ) throw()
{
	ATLASSERT( m_h == NULL );
	m_h = h;  // Take ownership
}

inline HANDLE CHandle::Detach() throw()
{
	HANDLE h;

	h = m_h;  // Release ownership
	m_h = NULL;

	return( h );
}

inline void CHandle::Close() throw()
{
	if( m_h != NULL )
	{
		::CloseHandle( m_h );
		m_h = NULL;
	}
}

class CCritSecLock
{
public:
	CCritSecLock( CRITICAL_SECTION& cs, bool bInitialLock = true );
	~CCritSecLock() throw();

	void Lock();
	void Unlock() throw();

// Implementation
private:
	CRITICAL_SECTION& m_cs;
	bool m_bLocked;

// Private to avoid accidental use
	CCritSecLock( const CCritSecLock& ) throw();
	CCritSecLock& operator=( const CCritSecLock& ) throw();
};

inline CCritSecLock::CCritSecLock( CRITICAL_SECTION& cs, bool bInitialLock ) :
	m_cs( cs ),
	m_bLocked( false )
{
	if( bInitialLock )
	{
		Lock();
	}
}

inline CCritSecLock::~CCritSecLock() throw()
{
	if( m_bLocked )
	{
		Unlock();
	}
}

inline void CCritSecLock::Lock()
{
	ATLASSERT( !m_bLocked );
	__try
	{
		::EnterCriticalSection( &m_cs );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		AtlThrow( E_OUTOFMEMORY );
	}
	m_bLocked = true;
}

inline void CCritSecLock::Unlock() throw()
{
	ATLASSERT( m_bLocked );
	::LeaveCriticalSection( &m_cs );
	m_bLocked = false;
}

/////////////////////////////////////////////////////////////////////////////
// Interface debugging
#if defined(_ATL_DEBUG_INTERFACES) || defined(_ATL_DEBUG_QI)
HRESULT WINAPI AtlDumpIID(REFIID iid, LPCTSTR pszClassName, HRESULT hr) throw();
#endif	// _ATL_DEBUG_INTERFACES || _ATL_DEBUG_QI

#ifdef _ATL_DEBUG_INTERFACES

struct _QIThunk
{
	STDMETHOD(QueryInterface)(REFIID iid, void** pp)
	{
		ATLASSERT(m_dwRef >= 0);
		ATLASSERT(m_pUnk != NULL);
		return m_pUnk->QueryInterface(iid, pp);
	}
	STDMETHOD_(ULONG, AddRef)()
	{
		ATLASSERT(m_pUnk != NULL);
		if (m_bBreak)
			DebugBreak();
		m_pUnk->AddRef();
		return InternalAddRef();
	}
	ULONG InternalAddRef()
	{
		ATLASSERT(m_pUnk != NULL);
		if (m_bBreak)
			DebugBreak();
		ATLASSERT(m_dwRef >= 0);
		long l = InterlockedIncrement(&m_dwRef);

		TCHAR buf[512];
		wsprintf(buf, _T("QIThunk - %-10d\tAddRef  :\tObject = 0x%08x\tRefcount = %d\t"), m_nIndex, m_pUnk, m_dwRef);
		OutputDebugString(buf);
		AtlDumpIID(m_iid, m_lpszClassName, S_OK);

		if (l > m_dwMaxRef)
			m_dwMaxRef = l;
		return l;
	}
	STDMETHOD_(ULONG, Release)();

	STDMETHOD(f3)();
	STDMETHOD(f4)();
	STDMETHOD(f5)();
	STDMETHOD(f6)();
	STDMETHOD(f7)();
	STDMETHOD(f8)();
	STDMETHOD(f9)();
	STDMETHOD(f10)();
	STDMETHOD(f11)();
	STDMETHOD(f12)();
	STDMETHOD(f13)();
	STDMETHOD(f14)();
	STDMETHOD(f15)();
	STDMETHOD(f16)();
	STDMETHOD(f17)();
	STDMETHOD(f18)();
	STDMETHOD(f19)();
	STDMETHOD(f20)();
	STDMETHOD(f21)();
	STDMETHOD(f22)();
	STDMETHOD(f23)();
	STDMETHOD(f24)();
	STDMETHOD(f25)();
	STDMETHOD(f26)();
	STDMETHOD(f27)();
	STDMETHOD(f28)();
	STDMETHOD(f29)();
	STDMETHOD(f30)();
	STDMETHOD(f31)();
	STDMETHOD(f32)();
	STDMETHOD(f33)();
	STDMETHOD(f34)();
	STDMETHOD(f35)();
	STDMETHOD(f36)();
	STDMETHOD(f37)();
	STDMETHOD(f38)();
	STDMETHOD(f39)();
	STDMETHOD(f40)();
	STDMETHOD(f41)();
	STDMETHOD(f42)();
	STDMETHOD(f43)();
	STDMETHOD(f44)();
	STDMETHOD(f45)();
	STDMETHOD(f46)();
	STDMETHOD(f47)();
	STDMETHOD(f48)();
	STDMETHOD(f49)();
	STDMETHOD(f50)();
	STDMETHOD(f51)();
	STDMETHOD(f52)();
	STDMETHOD(f53)();
	STDMETHOD(f54)();
	STDMETHOD(f55)();
	STDMETHOD(f56)();
	STDMETHOD(f57)();
	STDMETHOD(f58)();
	STDMETHOD(f59)();
	STDMETHOD(f60)();
	STDMETHOD(f61)();
	STDMETHOD(f62)();
	STDMETHOD(f63)();
	STDMETHOD(f64)();
	STDMETHOD(f65)();
	STDMETHOD(f66)();
	STDMETHOD(f67)();
	STDMETHOD(f68)();
	STDMETHOD(f69)();
	STDMETHOD(f70)();
	STDMETHOD(f71)();
	STDMETHOD(f72)();
	STDMETHOD(f73)();
	STDMETHOD(f74)();
	STDMETHOD(f75)();
	STDMETHOD(f76)();
	STDMETHOD(f77)();
	STDMETHOD(f78)();
	STDMETHOD(f79)();
	STDMETHOD(f80)();
	STDMETHOD(f81)();
	STDMETHOD(f82)();
	STDMETHOD(f83)();
	STDMETHOD(f84)();
	STDMETHOD(f85)();
	STDMETHOD(f86)();
	STDMETHOD(f87)();
	STDMETHOD(f88)();
	STDMETHOD(f89)();
	STDMETHOD(f90)();
	STDMETHOD(f91)();
	STDMETHOD(f92)();
	STDMETHOD(f93)();
	STDMETHOD(f94)();
	STDMETHOD(f95)();
	STDMETHOD(f96)();
	STDMETHOD(f97)();
	STDMETHOD(f98)();
	STDMETHOD(f99)();
	STDMETHOD(f100)();
	STDMETHOD(f101)();
	STDMETHOD(f102)();
	STDMETHOD(f103)();
	STDMETHOD(f104)();
	STDMETHOD(f105)();
	STDMETHOD(f106)();
	STDMETHOD(f107)();
	STDMETHOD(f108)();
	STDMETHOD(f109)();
	STDMETHOD(f110)();
	STDMETHOD(f111)();
	STDMETHOD(f112)();
	STDMETHOD(f113)();
	STDMETHOD(f114)();
	STDMETHOD(f115)();
	STDMETHOD(f116)();
	STDMETHOD(f117)();
	STDMETHOD(f118)();
	STDMETHOD(f119)();
	STDMETHOD(f120)();
	STDMETHOD(f121)();
	STDMETHOD(f122)();
	STDMETHOD(f123)();
	STDMETHOD(f124)();
	STDMETHOD(f125)();
	STDMETHOD(f126)();
	STDMETHOD(f127)();
	STDMETHOD(f128)();
	STDMETHOD(f129)();
	STDMETHOD(f130)();
	STDMETHOD(f131)();
	STDMETHOD(f132)();
	STDMETHOD(f133)();
	STDMETHOD(f134)();
	STDMETHOD(f135)();
	STDMETHOD(f136)();
	STDMETHOD(f137)();
	STDMETHOD(f138)();
	STDMETHOD(f139)();
	STDMETHOD(f140)();
	STDMETHOD(f141)();
	STDMETHOD(f142)();
	STDMETHOD(f143)();
	STDMETHOD(f144)();
	STDMETHOD(f145)();
	STDMETHOD(f146)();
	STDMETHOD(f147)();
	STDMETHOD(f148)();
	STDMETHOD(f149)();
	STDMETHOD(f150)();
	STDMETHOD(f151)();
	STDMETHOD(f152)();
	STDMETHOD(f153)();
	STDMETHOD(f154)();
	STDMETHOD(f155)();
	STDMETHOD(f156)();
	STDMETHOD(f157)();
	STDMETHOD(f158)();
	STDMETHOD(f159)();
	STDMETHOD(f160)();
	STDMETHOD(f161)();
	STDMETHOD(f162)();
	STDMETHOD(f163)();
	STDMETHOD(f164)();
	STDMETHOD(f165)();
	STDMETHOD(f166)();
	STDMETHOD(f167)();
	STDMETHOD(f168)();
	STDMETHOD(f169)();
	STDMETHOD(f170)();
	STDMETHOD(f171)();
	STDMETHOD(f172)();
	STDMETHOD(f173)();
	STDMETHOD(f174)();
	STDMETHOD(f175)();
	STDMETHOD(f176)();
	STDMETHOD(f177)();
	STDMETHOD(f178)();
	STDMETHOD(f179)();
	STDMETHOD(f180)();
	STDMETHOD(f181)();
	STDMETHOD(f182)();
	STDMETHOD(f183)();
	STDMETHOD(f184)();
	STDMETHOD(f185)();
	STDMETHOD(f186)();
	STDMETHOD(f187)();
	STDMETHOD(f188)();
	STDMETHOD(f189)();
	STDMETHOD(f190)();
	STDMETHOD(f191)();
	STDMETHOD(f192)();
	STDMETHOD(f193)();
	STDMETHOD(f194)();
	STDMETHOD(f195)();
	STDMETHOD(f196)();
	STDMETHOD(f197)();
	STDMETHOD(f198)();
	STDMETHOD(f199)();
	STDMETHOD(f200)();
	STDMETHOD(f201)();
	STDMETHOD(f202)();
	STDMETHOD(f203)();
	STDMETHOD(f204)();
	STDMETHOD(f205)();
	STDMETHOD(f206)();
	STDMETHOD(f207)();
	STDMETHOD(f208)();
	STDMETHOD(f209)();
	STDMETHOD(f210)();
	STDMETHOD(f211)();
	STDMETHOD(f212)();
	STDMETHOD(f213)();
	STDMETHOD(f214)();
	STDMETHOD(f215)();
	STDMETHOD(f216)();
	STDMETHOD(f217)();
	STDMETHOD(f218)();
	STDMETHOD(f219)();
	STDMETHOD(f220)();
	STDMETHOD(f221)();
	STDMETHOD(f222)();
	STDMETHOD(f223)();
	STDMETHOD(f224)();
	STDMETHOD(f225)();
	STDMETHOD(f226)();
	STDMETHOD(f227)();
	STDMETHOD(f228)();
	STDMETHOD(f229)();
	STDMETHOD(f230)();
	STDMETHOD(f231)();
	STDMETHOD(f232)();
	STDMETHOD(f233)();
	STDMETHOD(f234)();
	STDMETHOD(f235)();
	STDMETHOD(f236)();
	STDMETHOD(f237)();
	STDMETHOD(f238)();
	STDMETHOD(f239)();
	STDMETHOD(f240)();
	STDMETHOD(f241)();
	STDMETHOD(f242)();
	STDMETHOD(f243)();
	STDMETHOD(f244)();
	STDMETHOD(f245)();
	STDMETHOD(f246)();
	STDMETHOD(f247)();
	STDMETHOD(f248)();
	STDMETHOD(f249)();
	STDMETHOD(f250)();
	STDMETHOD(f251)();
	STDMETHOD(f252)();
	STDMETHOD(f253)();
	STDMETHOD(f254)();
	STDMETHOD(f255)();
	STDMETHOD(f256)();
	STDMETHOD(f257)();
	STDMETHOD(f258)();
	STDMETHOD(f259)();
	STDMETHOD(f260)();
	STDMETHOD(f261)();
	STDMETHOD(f262)();
	STDMETHOD(f263)();
	STDMETHOD(f264)();
	STDMETHOD(f265)();
	STDMETHOD(f266)();
	STDMETHOD(f267)();
	STDMETHOD(f268)();
	STDMETHOD(f269)();
	STDMETHOD(f270)();
	STDMETHOD(f271)();
	STDMETHOD(f272)();
	STDMETHOD(f273)();
	STDMETHOD(f274)();
	STDMETHOD(f275)();
	STDMETHOD(f276)();
	STDMETHOD(f277)();
	STDMETHOD(f278)();
	STDMETHOD(f279)();
	STDMETHOD(f280)();
	STDMETHOD(f281)();
	STDMETHOD(f282)();
	STDMETHOD(f283)();
	STDMETHOD(f284)();
	STDMETHOD(f285)();
	STDMETHOD(f286)();
	STDMETHOD(f287)();
	STDMETHOD(f288)();
	STDMETHOD(f289)();
	STDMETHOD(f290)();
	STDMETHOD(f291)();
	STDMETHOD(f292)();
	STDMETHOD(f293)();
	STDMETHOD(f294)();
	STDMETHOD(f295)();
	STDMETHOD(f296)();
	STDMETHOD(f297)();
	STDMETHOD(f298)();
	STDMETHOD(f299)();
	STDMETHOD(f300)();
	STDMETHOD(f301)();
	STDMETHOD(f302)();
	STDMETHOD(f303)();
	STDMETHOD(f304)();
	STDMETHOD(f305)();
	STDMETHOD(f306)();
	STDMETHOD(f307)();
	STDMETHOD(f308)();
	STDMETHOD(f309)();
	STDMETHOD(f310)();
	STDMETHOD(f311)();
	STDMETHOD(f312)();
	STDMETHOD(f313)();
	STDMETHOD(f314)();
	STDMETHOD(f315)();
	STDMETHOD(f316)();
	STDMETHOD(f317)();
	STDMETHOD(f318)();
	STDMETHOD(f319)();
	STDMETHOD(f320)();
	STDMETHOD(f321)();
	STDMETHOD(f322)();
	STDMETHOD(f323)();
	STDMETHOD(f324)();
	STDMETHOD(f325)();
	STDMETHOD(f326)();
	STDMETHOD(f327)();
	STDMETHOD(f328)();
	STDMETHOD(f329)();
	STDMETHOD(f330)();
	STDMETHOD(f331)();
	STDMETHOD(f332)();
	STDMETHOD(f333)();
	STDMETHOD(f334)();
	STDMETHOD(f335)();
	STDMETHOD(f336)();
	STDMETHOD(f337)();
	STDMETHOD(f338)();
	STDMETHOD(f339)();
	STDMETHOD(f340)();
	STDMETHOD(f341)();
	STDMETHOD(f342)();
	STDMETHOD(f343)();
	STDMETHOD(f344)();
	STDMETHOD(f345)();
	STDMETHOD(f346)();
	STDMETHOD(f347)();
	STDMETHOD(f348)();
	STDMETHOD(f349)();
	STDMETHOD(f350)();
	STDMETHOD(f351)();
	STDMETHOD(f352)();
	STDMETHOD(f353)();
	STDMETHOD(f354)();
	STDMETHOD(f355)();
	STDMETHOD(f356)();
	STDMETHOD(f357)();
	STDMETHOD(f358)();
	STDMETHOD(f359)();
	STDMETHOD(f360)();
	STDMETHOD(f361)();
	STDMETHOD(f362)();
	STDMETHOD(f363)();
	STDMETHOD(f364)();
	STDMETHOD(f365)();
	STDMETHOD(f366)();
	STDMETHOD(f367)();
	STDMETHOD(f368)();
	STDMETHOD(f369)();
	STDMETHOD(f370)();
	STDMETHOD(f371)();
	STDMETHOD(f372)();
	STDMETHOD(f373)();
	STDMETHOD(f374)();
	STDMETHOD(f375)();
	STDMETHOD(f376)();
	STDMETHOD(f377)();
	STDMETHOD(f378)();
	STDMETHOD(f379)();
	STDMETHOD(f380)();
	STDMETHOD(f381)();
	STDMETHOD(f382)();
	STDMETHOD(f383)();
	STDMETHOD(f384)();
	STDMETHOD(f385)();
	STDMETHOD(f386)();
	STDMETHOD(f387)();
	STDMETHOD(f388)();
	STDMETHOD(f389)();
	STDMETHOD(f390)();
	STDMETHOD(f391)();
	STDMETHOD(f392)();
	STDMETHOD(f393)();
	STDMETHOD(f394)();
	STDMETHOD(f395)();
	STDMETHOD(f396)();
	STDMETHOD(f397)();
	STDMETHOD(f398)();
	STDMETHOD(f399)();
	STDMETHOD(f400)();
	STDMETHOD(f401)();
	STDMETHOD(f402)();
	STDMETHOD(f403)();
	STDMETHOD(f404)();
	STDMETHOD(f405)();
	STDMETHOD(f406)();
	STDMETHOD(f407)();
	STDMETHOD(f408)();
	STDMETHOD(f409)();
	STDMETHOD(f410)();
	STDMETHOD(f411)();
	STDMETHOD(f412)();
	STDMETHOD(f413)();
	STDMETHOD(f414)();
	STDMETHOD(f415)();
	STDMETHOD(f416)();
	STDMETHOD(f417)();
	STDMETHOD(f418)();
	STDMETHOD(f419)();
	STDMETHOD(f420)();
	STDMETHOD(f421)();
	STDMETHOD(f422)();
	STDMETHOD(f423)();
	STDMETHOD(f424)();
	STDMETHOD(f425)();
	STDMETHOD(f426)();
	STDMETHOD(f427)();
	STDMETHOD(f428)();
	STDMETHOD(f429)();
	STDMETHOD(f430)();
	STDMETHOD(f431)();
	STDMETHOD(f432)();
	STDMETHOD(f433)();
	STDMETHOD(f434)();
	STDMETHOD(f435)();
	STDMETHOD(f436)();
	STDMETHOD(f437)();
	STDMETHOD(f438)();
	STDMETHOD(f439)();
	STDMETHOD(f440)();
	STDMETHOD(f441)();
	STDMETHOD(f442)();
	STDMETHOD(f443)();
	STDMETHOD(f444)();
	STDMETHOD(f445)();
	STDMETHOD(f446)();
	STDMETHOD(f447)();
	STDMETHOD(f448)();
	STDMETHOD(f449)();
	STDMETHOD(f450)();
	STDMETHOD(f451)();
	STDMETHOD(f452)();
	STDMETHOD(f453)();
	STDMETHOD(f454)();
	STDMETHOD(f455)();
	STDMETHOD(f456)();
	STDMETHOD(f457)();
	STDMETHOD(f458)();
	STDMETHOD(f459)();
	STDMETHOD(f460)();
	STDMETHOD(f461)();
	STDMETHOD(f462)();
	STDMETHOD(f463)();
	STDMETHOD(f464)();
	STDMETHOD(f465)();
	STDMETHOD(f466)();
	STDMETHOD(f467)();
	STDMETHOD(f468)();
	STDMETHOD(f469)();
	STDMETHOD(f470)();
	STDMETHOD(f471)();
	STDMETHOD(f472)();
	STDMETHOD(f473)();
	STDMETHOD(f474)();
	STDMETHOD(f475)();
	STDMETHOD(f476)();
	STDMETHOD(f477)();
	STDMETHOD(f478)();
	STDMETHOD(f479)();
	STDMETHOD(f480)();
	STDMETHOD(f481)();
	STDMETHOD(f482)();
	STDMETHOD(f483)();
	STDMETHOD(f484)();
	STDMETHOD(f485)();
	STDMETHOD(f486)();
	STDMETHOD(f487)();
	STDMETHOD(f488)();
	STDMETHOD(f489)();
	STDMETHOD(f490)();
	STDMETHOD(f491)();
	STDMETHOD(f492)();
	STDMETHOD(f493)();
	STDMETHOD(f494)();
	STDMETHOD(f495)();
	STDMETHOD(f496)();
	STDMETHOD(f497)();
	STDMETHOD(f498)();
	STDMETHOD(f499)();
	STDMETHOD(f500)();
	STDMETHOD(f501)();
	STDMETHOD(f502)();
	STDMETHOD(f503)();
	STDMETHOD(f504)();
	STDMETHOD(f505)();
	STDMETHOD(f506)();
	STDMETHOD(f507)();
	STDMETHOD(f508)();
	STDMETHOD(f509)();
	STDMETHOD(f510)();
	STDMETHOD(f511)();
	STDMETHOD(f512)();
	STDMETHOD(f513)();
	STDMETHOD(f514)();
	STDMETHOD(f515)();
	STDMETHOD(f516)();
	STDMETHOD(f517)();
	STDMETHOD(f518)();
	STDMETHOD(f519)();
	STDMETHOD(f520)();
	STDMETHOD(f521)();
	STDMETHOD(f522)();
	STDMETHOD(f523)();
	STDMETHOD(f524)();
	STDMETHOD(f525)();
	STDMETHOD(f526)();
	STDMETHOD(f527)();
	STDMETHOD(f528)();
	STDMETHOD(f529)();
	STDMETHOD(f530)();
	STDMETHOD(f531)();
	STDMETHOD(f532)();
	STDMETHOD(f533)();
	STDMETHOD(f534)();
	STDMETHOD(f535)();
	STDMETHOD(f536)();
	STDMETHOD(f537)();
	STDMETHOD(f538)();
	STDMETHOD(f539)();
	STDMETHOD(f540)();
	STDMETHOD(f541)();
	STDMETHOD(f542)();
	STDMETHOD(f543)();
	STDMETHOD(f544)();
	STDMETHOD(f545)();
	STDMETHOD(f546)();
	STDMETHOD(f547)();
	STDMETHOD(f548)();
	STDMETHOD(f549)();
	STDMETHOD(f550)();
	STDMETHOD(f551)();
	STDMETHOD(f552)();
	STDMETHOD(f553)();
	STDMETHOD(f554)();
	STDMETHOD(f555)();
	STDMETHOD(f556)();
	STDMETHOD(f557)();
	STDMETHOD(f558)();
	STDMETHOD(f559)();
	STDMETHOD(f560)();
	STDMETHOD(f561)();
	STDMETHOD(f562)();
	STDMETHOD(f563)();
	STDMETHOD(f564)();
	STDMETHOD(f565)();
	STDMETHOD(f566)();
	STDMETHOD(f567)();
	STDMETHOD(f568)();
	STDMETHOD(f569)();
	STDMETHOD(f570)();
	STDMETHOD(f571)();
	STDMETHOD(f572)();
	STDMETHOD(f573)();
	STDMETHOD(f574)();
	STDMETHOD(f575)();
	STDMETHOD(f576)();
	STDMETHOD(f577)();
	STDMETHOD(f578)();
	STDMETHOD(f579)();
	STDMETHOD(f580)();
	STDMETHOD(f581)();
	STDMETHOD(f582)();
	STDMETHOD(f583)();
	STDMETHOD(f584)();
	STDMETHOD(f585)();
	STDMETHOD(f586)();
	STDMETHOD(f587)();
	STDMETHOD(f588)();
	STDMETHOD(f589)();
	STDMETHOD(f590)();
	STDMETHOD(f591)();
	STDMETHOD(f592)();
	STDMETHOD(f593)();
	STDMETHOD(f594)();
	STDMETHOD(f595)();
	STDMETHOD(f596)();
	STDMETHOD(f597)();
	STDMETHOD(f598)();
	STDMETHOD(f599)();
	STDMETHOD(f600)();
	STDMETHOD(f601)();
	STDMETHOD(f602)();
	STDMETHOD(f603)();
	STDMETHOD(f604)();
	STDMETHOD(f605)();
	STDMETHOD(f606)();
	STDMETHOD(f607)();
	STDMETHOD(f608)();
	STDMETHOD(f609)();
	STDMETHOD(f610)();
	STDMETHOD(f611)();
	STDMETHOD(f612)();
	STDMETHOD(f613)();
	STDMETHOD(f614)();
	STDMETHOD(f615)();
	STDMETHOD(f616)();
	STDMETHOD(f617)();
	STDMETHOD(f618)();
	STDMETHOD(f619)();
	STDMETHOD(f620)();
	STDMETHOD(f621)();
	STDMETHOD(f622)();
	STDMETHOD(f623)();
	STDMETHOD(f624)();
	STDMETHOD(f625)();
	STDMETHOD(f626)();
	STDMETHOD(f627)();
	STDMETHOD(f628)();
	STDMETHOD(f629)();
	STDMETHOD(f630)();
	STDMETHOD(f631)();
	STDMETHOD(f632)();
	STDMETHOD(f633)();
	STDMETHOD(f634)();
	STDMETHOD(f635)();
	STDMETHOD(f636)();
	STDMETHOD(f637)();
	STDMETHOD(f638)();
	STDMETHOD(f639)();
	STDMETHOD(f640)();
	STDMETHOD(f641)();
	STDMETHOD(f642)();
	STDMETHOD(f643)();
	STDMETHOD(f644)();
	STDMETHOD(f645)();
	STDMETHOD(f646)();
	STDMETHOD(f647)();
	STDMETHOD(f648)();
	STDMETHOD(f649)();
	STDMETHOD(f650)();
	STDMETHOD(f651)();
	STDMETHOD(f652)();
	STDMETHOD(f653)();
	STDMETHOD(f654)();
	STDMETHOD(f655)();
	STDMETHOD(f656)();
	STDMETHOD(f657)();
	STDMETHOD(f658)();
	STDMETHOD(f659)();
	STDMETHOD(f660)();
	STDMETHOD(f661)();
	STDMETHOD(f662)();
	STDMETHOD(f663)();
	STDMETHOD(f664)();
	STDMETHOD(f665)();
	STDMETHOD(f666)();
	STDMETHOD(f667)();
	STDMETHOD(f668)();
	STDMETHOD(f669)();
	STDMETHOD(f670)();
	STDMETHOD(f671)();
	STDMETHOD(f672)();
	STDMETHOD(f673)();
	STDMETHOD(f674)();
	STDMETHOD(f675)();
	STDMETHOD(f676)();
	STDMETHOD(f677)();
	STDMETHOD(f678)();
	STDMETHOD(f679)();
	STDMETHOD(f680)();
	STDMETHOD(f681)();
	STDMETHOD(f682)();
	STDMETHOD(f683)();
	STDMETHOD(f684)();
	STDMETHOD(f685)();
	STDMETHOD(f686)();
	STDMETHOD(f687)();
	STDMETHOD(f688)();
	STDMETHOD(f689)();
	STDMETHOD(f690)();
	STDMETHOD(f691)();
	STDMETHOD(f692)();
	STDMETHOD(f693)();
	STDMETHOD(f694)();
	STDMETHOD(f695)();
	STDMETHOD(f696)();
	STDMETHOD(f697)();
	STDMETHOD(f698)();
	STDMETHOD(f699)();
	STDMETHOD(f700)();
	STDMETHOD(f701)();
	STDMETHOD(f702)();
	STDMETHOD(f703)();
	STDMETHOD(f704)();
	STDMETHOD(f705)();
	STDMETHOD(f706)();
	STDMETHOD(f707)();
	STDMETHOD(f708)();
	STDMETHOD(f709)();
	STDMETHOD(f710)();
	STDMETHOD(f711)();
	STDMETHOD(f712)();
	STDMETHOD(f713)();
	STDMETHOD(f714)();
	STDMETHOD(f715)();
	STDMETHOD(f716)();
	STDMETHOD(f717)();
	STDMETHOD(f718)();
	STDMETHOD(f719)();
	STDMETHOD(f720)();
	STDMETHOD(f721)();
	STDMETHOD(f722)();
	STDMETHOD(f723)();
	STDMETHOD(f724)();
	STDMETHOD(f725)();
	STDMETHOD(f726)();
	STDMETHOD(f727)();
	STDMETHOD(f728)();
	STDMETHOD(f729)();
	STDMETHOD(f730)();
	STDMETHOD(f731)();
	STDMETHOD(f732)();
	STDMETHOD(f733)();
	STDMETHOD(f734)();
	STDMETHOD(f735)();
	STDMETHOD(f736)();
	STDMETHOD(f737)();
	STDMETHOD(f738)();
	STDMETHOD(f739)();
	STDMETHOD(f740)();
	STDMETHOD(f741)();
	STDMETHOD(f742)();
	STDMETHOD(f743)();
	STDMETHOD(f744)();
	STDMETHOD(f745)();
	STDMETHOD(f746)();
	STDMETHOD(f747)();
	STDMETHOD(f748)();
	STDMETHOD(f749)();
	STDMETHOD(f750)();
	STDMETHOD(f751)();
	STDMETHOD(f752)();
	STDMETHOD(f753)();
	STDMETHOD(f754)();
	STDMETHOD(f755)();
	STDMETHOD(f756)();
	STDMETHOD(f757)();
	STDMETHOD(f758)();
	STDMETHOD(f759)();
	STDMETHOD(f760)();
	STDMETHOD(f761)();
	STDMETHOD(f762)();
	STDMETHOD(f763)();
	STDMETHOD(f764)();
	STDMETHOD(f765)();
	STDMETHOD(f766)();
	STDMETHOD(f767)();
	STDMETHOD(f768)();
	STDMETHOD(f769)();
	STDMETHOD(f770)();
	STDMETHOD(f771)();
	STDMETHOD(f772)();
	STDMETHOD(f773)();
	STDMETHOD(f774)();
	STDMETHOD(f775)();
	STDMETHOD(f776)();
	STDMETHOD(f777)();
	STDMETHOD(f778)();
	STDMETHOD(f779)();
	STDMETHOD(f780)();
	STDMETHOD(f781)();
	STDMETHOD(f782)();
	STDMETHOD(f783)();
	STDMETHOD(f784)();
	STDMETHOD(f785)();
	STDMETHOD(f786)();
	STDMETHOD(f787)();
	STDMETHOD(f788)();
	STDMETHOD(f789)();
	STDMETHOD(f790)();
	STDMETHOD(f791)();
	STDMETHOD(f792)();
	STDMETHOD(f793)();
	STDMETHOD(f794)();
	STDMETHOD(f795)();
	STDMETHOD(f796)();
	STDMETHOD(f797)();
	STDMETHOD(f798)();
	STDMETHOD(f799)();
	STDMETHOD(f800)();
	STDMETHOD(f801)();
	STDMETHOD(f802)();
	STDMETHOD(f803)();
	STDMETHOD(f804)();
	STDMETHOD(f805)();
	STDMETHOD(f806)();
	STDMETHOD(f807)();
	STDMETHOD(f808)();
	STDMETHOD(f809)();
	STDMETHOD(f810)();
	STDMETHOD(f811)();
	STDMETHOD(f812)();
	STDMETHOD(f813)();
	STDMETHOD(f814)();
	STDMETHOD(f815)();
	STDMETHOD(f816)();
	STDMETHOD(f817)();
	STDMETHOD(f818)();
	STDMETHOD(f819)();
	STDMETHOD(f820)();
	STDMETHOD(f821)();
	STDMETHOD(f822)();
	STDMETHOD(f823)();
	STDMETHOD(f824)();
	STDMETHOD(f825)();
	STDMETHOD(f826)();
	STDMETHOD(f827)();
	STDMETHOD(f828)();
	STDMETHOD(f829)();
	STDMETHOD(f830)();
	STDMETHOD(f831)();
	STDMETHOD(f832)();
	STDMETHOD(f833)();
	STDMETHOD(f834)();
	STDMETHOD(f835)();
	STDMETHOD(f836)();
	STDMETHOD(f837)();
	STDMETHOD(f838)();
	STDMETHOD(f839)();
	STDMETHOD(f840)();
	STDMETHOD(f841)();
	STDMETHOD(f842)();
	STDMETHOD(f843)();
	STDMETHOD(f844)();
	STDMETHOD(f845)();
	STDMETHOD(f846)();
	STDMETHOD(f847)();
	STDMETHOD(f848)();
	STDMETHOD(f849)();
	STDMETHOD(f850)();
	STDMETHOD(f851)();
	STDMETHOD(f852)();
	STDMETHOD(f853)();
	STDMETHOD(f854)();
	STDMETHOD(f855)();
	STDMETHOD(f856)();
	STDMETHOD(f857)();
	STDMETHOD(f858)();
	STDMETHOD(f859)();
	STDMETHOD(f860)();
	STDMETHOD(f861)();
	STDMETHOD(f862)();
	STDMETHOD(f863)();
	STDMETHOD(f864)();
	STDMETHOD(f865)();
	STDMETHOD(f866)();
	STDMETHOD(f867)();
	STDMETHOD(f868)();
	STDMETHOD(f869)();
	STDMETHOD(f870)();
	STDMETHOD(f871)();
	STDMETHOD(f872)();
	STDMETHOD(f873)();
	STDMETHOD(f874)();
	STDMETHOD(f875)();
	STDMETHOD(f876)();
	STDMETHOD(f877)();
	STDMETHOD(f878)();
	STDMETHOD(f879)();
	STDMETHOD(f880)();
	STDMETHOD(f881)();
	STDMETHOD(f882)();
	STDMETHOD(f883)();
	STDMETHOD(f884)();
	STDMETHOD(f885)();
	STDMETHOD(f886)();
	STDMETHOD(f887)();
	STDMETHOD(f888)();
	STDMETHOD(f889)();
	STDMETHOD(f890)();
	STDMETHOD(f891)();
	STDMETHOD(f892)();
	STDMETHOD(f893)();
	STDMETHOD(f894)();
	STDMETHOD(f895)();
	STDMETHOD(f896)();
	STDMETHOD(f897)();
	STDMETHOD(f898)();
	STDMETHOD(f899)();
	STDMETHOD(f900)();
	STDMETHOD(f901)();
	STDMETHOD(f902)();
	STDMETHOD(f903)();
	STDMETHOD(f904)();
	STDMETHOD(f905)();
	STDMETHOD(f906)();
	STDMETHOD(f907)();
	STDMETHOD(f908)();
	STDMETHOD(f909)();
	STDMETHOD(f910)();
	STDMETHOD(f911)();
	STDMETHOD(f912)();
	STDMETHOD(f913)();
	STDMETHOD(f914)();
	STDMETHOD(f915)();
	STDMETHOD(f916)();
	STDMETHOD(f917)();
	STDMETHOD(f918)();
	STDMETHOD(f919)();
	STDMETHOD(f920)();
	STDMETHOD(f921)();
	STDMETHOD(f922)();
	STDMETHOD(f923)();
	STDMETHOD(f924)();
	STDMETHOD(f925)();
	STDMETHOD(f926)();
	STDMETHOD(f927)();
	STDMETHOD(f928)();
	STDMETHOD(f929)();
	STDMETHOD(f930)();
	STDMETHOD(f931)();
	STDMETHOD(f932)();
	STDMETHOD(f933)();
	STDMETHOD(f934)();
	STDMETHOD(f935)();
	STDMETHOD(f936)();
	STDMETHOD(f937)();
	STDMETHOD(f938)();
	STDMETHOD(f939)();
	STDMETHOD(f940)();
	STDMETHOD(f941)();
	STDMETHOD(f942)();
	STDMETHOD(f943)();
	STDMETHOD(f944)();
	STDMETHOD(f945)();
	STDMETHOD(f946)();
	STDMETHOD(f947)();
	STDMETHOD(f948)();
	STDMETHOD(f949)();
	STDMETHOD(f950)();
	STDMETHOD(f951)();
	STDMETHOD(f952)();
	STDMETHOD(f953)();
	STDMETHOD(f954)();
	STDMETHOD(f955)();
	STDMETHOD(f956)();
	STDMETHOD(f957)();
	STDMETHOD(f958)();
	STDMETHOD(f959)();
	STDMETHOD(f960)();
	STDMETHOD(f961)();
	STDMETHOD(f962)();
	STDMETHOD(f963)();
	STDMETHOD(f964)();
	STDMETHOD(f965)();
	STDMETHOD(f966)();
	STDMETHOD(f967)();
	STDMETHOD(f968)();
	STDMETHOD(f969)();
	STDMETHOD(f970)();
	STDMETHOD(f971)();
	STDMETHOD(f972)();
	STDMETHOD(f973)();
	STDMETHOD(f974)();
	STDMETHOD(f975)();
	STDMETHOD(f976)();
	STDMETHOD(f977)();
	STDMETHOD(f978)();
	STDMETHOD(f979)();
	STDMETHOD(f980)();
	STDMETHOD(f981)();
	STDMETHOD(f982)();
	STDMETHOD(f983)();
	STDMETHOD(f984)();
	STDMETHOD(f985)();
	STDMETHOD(f986)();
	STDMETHOD(f987)();
	STDMETHOD(f988)();
	STDMETHOD(f989)();
	STDMETHOD(f990)();
	STDMETHOD(f991)();
	STDMETHOD(f992)();
	STDMETHOD(f993)();
	STDMETHOD(f994)();
	STDMETHOD(f995)();
	STDMETHOD(f996)();
	STDMETHOD(f997)();
	STDMETHOD(f998)();
	STDMETHOD(f999)();
	STDMETHOD(f1000)();
	STDMETHOD(f1001)();
	STDMETHOD(f1002)();
	STDMETHOD(f1003)();
	STDMETHOD(f1004)();
	STDMETHOD(f1005)();
	STDMETHOD(f1006)();
	STDMETHOD(f1007)();
	STDMETHOD(f1008)();
	STDMETHOD(f1009)();
	STDMETHOD(f1010)();
	STDMETHOD(f1011)();
	STDMETHOD(f1012)();
	STDMETHOD(f1013)();
	STDMETHOD(f1014)();
	STDMETHOD(f1015)();
	STDMETHOD(f1016)();
	STDMETHOD(f1017)();
	STDMETHOD(f1018)();
	STDMETHOD(f1019)();
	STDMETHOD(f1020)();
	STDMETHOD(f1021)();
	STDMETHOD(f1022)();
	STDMETHOD(f1023)();
	_QIThunk(IUnknown* pOrig, LPCTSTR p, const IID& i, UINT n, bool b)
	{
		m_lpszClassName = p;
		m_iid = i;
		m_nIndex = n;
		m_dwRef = 0;
		m_dwMaxRef = 0;
		m_pUnk = pOrig;
		m_bBreak = b;
		m_bNonAddRefThunk = false;
	}
	IUnknown* m_pUnk;
	long m_dwRef;
	long m_dwMaxRef;
	LPCTSTR m_lpszClassName;
	IID m_iid;
	UINT m_nIndex;
	bool m_bBreak;
	bool m_bNonAddRefThunk;
	void Dump() throw();
};

#endif

/////////////////////////////////////////////////////////////////////////////
// Module Classes

#ifdef _ATL_STATIC_REGISTRY
#define UpdateRegistryFromResource UpdateRegistryFromResourceS
#else
#define UpdateRegistryFromResource UpdateRegistryFromResourceD
#endif // _ATL_STATIC_REGISTRY


class CAtlComModule : public _ATL_COM_MODULE
{
public:

	CAtlComModule() throw()
	{
		cbSize = sizeof(_ATL_COM_MODULE);

		m_hInstTypeLib = reinterpret_cast<HINSTANCE>(&__ImageBase);

		m_ppAutoObjMapFirst = &__pobjMapEntryFirst + 1;
		m_ppAutoObjMapLast = &__pobjMapEntryLast;

		if (FAILED(m_csObjMap.Init()))
		{
			ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to initialize critical section in CAtlComModule\n"));
			ATLASSERT(0);
			CAtlBaseModule::m_bInitFailed = true;
		}
	}

	~CAtlComModule()
	{
		Term();
	}

	// Called from ~CAtlComModule or from ~CAtlExeModule.
	void Term()
	{
		if (cbSize == 0)
			return;

		for (_ATL_OBJMAP_ENTRY** ppEntry = m_ppAutoObjMapFirst; ppEntry < m_ppAutoObjMapLast; ppEntry++)
		{
			if (*ppEntry != NULL)
			{
				_ATL_OBJMAP_ENTRY* pEntry = *ppEntry;
				if (pEntry->pCF != NULL)
					pEntry->pCF->Release();
				pEntry->pCF = NULL;
			}
		}
		m_csObjMap.Term();
		// Set to 0 to indicate that this function has been called
		// At this point no one should be concerned about cbsize
		// having the correct value
		cbSize = 0;
	}

	// Registry support (helpers)
	HRESULT RegisterTypeLib()
	{
		return AtlRegisterTypeLib(m_hInstTypeLib, NULL);
	}
	HRESULT RegisterTypeLib(LPCTSTR lpszIndex)
	{
		USES_CONVERSION_EX;
		LPCOLESTR pwszTemp = NULL;
		if( lpszIndex != NULL )
		{
			pwszTemp = T2COLE_EX(lpszIndex, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE			
			if( pwszTemp == NULL )
				return E_OUTOFMEMORY;
#endif			
		}
		return AtlRegisterTypeLib(m_hInstTypeLib, pwszTemp);
	}
	HRESULT UnRegisterTypeLib()
	{
		return AtlUnRegisterTypeLib(m_hInstTypeLib, NULL);
	}
	HRESULT UnRegisterTypeLib(LPCTSTR lpszIndex)
	{
		USES_CONVERSION_EX;
		LPCOLESTR pwszTemp = NULL;
		if( lpszIndex != NULL )
		{
			pwszTemp = T2COLE_EX(lpszIndex, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE			
			if( pwszTemp == NULL )
				return E_OUTOFMEMORY;
#endif			
		}
		return AtlUnRegisterTypeLib(m_hInstTypeLib, pwszTemp);
	}

	// RegisterServer walks the ATL Autogenerated object map and registers each object in the map
	// If pCLSID is not NULL then only the object referred to by pCLSID is registered (The default case)
	// otherwise all the objects are registered
	HRESULT RegisterServer(BOOL bRegTypeLib = FALSE, const CLSID* pCLSID = NULL)
	{
		return AtlComModuleRegisterServer(this, bRegTypeLib, pCLSID);
	}

	// UnregisterServer walks the ATL Autogenerated object map and unregisters each object in the map
	// If pCLSID is not NULL then only the object referred to by pCLSID is unregistered (The default case)
	// otherwise all the objects are unregistered.
	HRESULT UnregisterServer(BOOL bRegTypeLib = FALSE, const CLSID* pCLSID = NULL)
	{
		return AtlComModuleUnregisterServer(this, bRegTypeLib, pCLSID);
	}

	// Implementation

	// Call ObjectMain for all the objects.
	void ExecuteObjectMain(bool bStarting)
	{
		for (_ATL_OBJMAP_ENTRY** ppEntry = m_ppAutoObjMapFirst; ppEntry < m_ppAutoObjMapLast; ppEntry++)
		{
			if (*ppEntry != NULL)
				(*ppEntry)->pfnObjectMain(bStarting);
		}
	}	
};

extern CAtlComModule _AtlComModule;

#ifdef _ATL_DEBUG_INTERFACES

class CAtlDebugInterfacesModule
{
public:
	CAtlDebugInterfacesModule() throw() :
		m_nIndexQI( 0 ),
		m_nIndexBreakAt( 0 )
	{
		if (FAILED(m_cs.Init()))
		{
			ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to initialize critical section in CAtlDebugInterfacesModule\n"));
			ATLASSERT(0);
			CAtlBaseModule::m_bInitFailed = true;
		}
	}
	~CAtlDebugInterfacesModule() throw()
	{	
		// Release all the class factories.
		_AtlComModule.Term();
		DumpLeakedThunks();
		m_cs.Term();
	}

	HRESULT AddThunk(IUnknown** pp, LPCTSTR lpsz, REFIID iid) throw()
	{
		if ((pp == NULL) || (*pp == NULL))
			return E_POINTER;
		IUnknown* p = *pp;
		_QIThunk* pThunk = NULL;
		CComCritSecLock<CComCriticalSection> lock(m_cs, false);
		HRESULT hr = lock.Lock();
		if (FAILED(hr))
		{
			ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to lock critical section in CAtlDebugInterfacesModule\n"));
			ATLASSERT(0);
			return hr;
		}

		// Check if exists already for identity
		if (InlineIsEqualUnknown(iid))
		{
			for (int i = 0; i < m_aThunks.GetSize(); i++)
			{
				if (m_aThunks[i]->m_pUnk == p && InlineIsEqualGUID(m_aThunks[i]->m_iid, iid))
				{
					m_aThunks[i]->InternalAddRef();
					pThunk = m_aThunks[i];
					break;
				}
			}
		}
		if (pThunk == NULL)
		{
			++m_nIndexQI;
			if (m_nIndexBreakAt == m_nIndexQI)
				DebugBreak();
			ATLTRY(pThunk = new _QIThunk(p, lpsz, iid, m_nIndexQI, (m_nIndexBreakAt == m_nIndexQI)));
			if (pThunk == NULL)
			{
				return E_OUTOFMEMORY;
			}
			pThunk->InternalAddRef();
			m_aThunks.Add(pThunk);
		}
		*pp = (IUnknown*)pThunk;
		return S_OK;
	}
	HRESULT AddNonAddRefThunk(IUnknown* p, LPCTSTR lpsz, IUnknown** ppThunkRet) throw()
	{
		if (ppThunkRet == NULL)
			return E_POINTER;
		*ppThunkRet = NULL;

		_QIThunk* pThunk = NULL;
		CComCritSecLock<CComCriticalSection> lock(m_cs, false);
		HRESULT hr = lock.Lock();
		if (FAILED(hr))
		{
			ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to lock critical section in CAtlDebugInterfacesModule\n"));
			ATLASSERT(0);
			return hr;
		}

		// Check if exists already for identity
		for (int i = 0; i < m_aThunks.GetSize(); i++)
		{
			if (m_aThunks[i]->m_pUnk == p)
			{
				m_aThunks[i]->m_bNonAddRefThunk = true;
				pThunk = m_aThunks[i];
				break;
			}
		}
		if (pThunk == NULL)
		{
			++m_nIndexQI;
			if (m_nIndexBreakAt == m_nIndexQI)
				DebugBreak();
			ATLTRY(pThunk = new _QIThunk(p, lpsz, __uuidof(IUnknown), m_nIndexQI, (m_nIndexBreakAt == m_nIndexQI)));
			if (pThunk == NULL)
			{
				return E_OUTOFMEMORY;
			}
			pThunk->m_bNonAddRefThunk = true;
			m_aThunks.Add(pThunk);
		}
		*ppThunkRet = (IUnknown*)pThunk;
		return S_OK;;
	}
	void DeleteNonAddRefThunk(IUnknown* pUnk) throw()
	{
		CComCritSecLock<CComCriticalSection> lock(m_cs, false);
		HRESULT hr = lock.Lock();
		if (FAILED(hr))
		{
			ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to lock critical section in CAtlDebugInterfacesModule\n"));
			ATLASSERT(0);
			return;
		}

		for (int i = 0; i < m_aThunks.GetSize(); i++)
		{
			if (m_aThunks[i]->m_bNonAddRefThunk && m_aThunks[i]->m_pUnk == pUnk)
			{
				delete m_aThunks[i];
				m_aThunks.RemoveAt(i);
				break;
			}
		}
	}
	void DeleteThunk(_QIThunk* p) throw()
	{
		CComCritSecLock<CComCriticalSection> lock(m_cs, false);
		HRESULT hr = lock.Lock();
		if (FAILED(hr))
		{
			ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to lock critical section in CAtlDebugInterfacesModule\n"));
			ATLASSERT(0);
			return;
		}

		int nIndex = m_aThunks.Find(p);
		if (nIndex != -1)
		{
			m_aThunks[nIndex]->m_pUnk = NULL;
			delete m_aThunks[nIndex];
			m_aThunks.RemoveAt(nIndex);
		}
	}
	bool DumpLeakedThunks()
	{
		bool b = false;
		for (int i = 0; i < m_aThunks.GetSize(); i++)
		{
			b = true;
			m_aThunks[i]->Dump();
			delete m_aThunks[i];
		}
		m_aThunks.RemoveAll();
		return b;
	}

public:
	UINT m_nIndexQI;
	UINT m_nIndexBreakAt;
	CSimpleArray<_QIThunk*> m_aThunks;
	CComCriticalSection m_cs;
};

extern CAtlDebugInterfacesModule _AtlDebugInterfacesModule;

#ifndef _ATL_STATIC_LIB_IMPL
// Should not be pulled into the static lib
__declspec (selectany) CAtlDebugInterfacesModule _AtlDebugInterfacesModule;
#endif

inline ULONG _QIThunk::Release()
{
	ATLASSERT(m_pUnk != NULL);
	if (m_bBreak)
		DebugBreak();
	ATLASSERT(m_dwRef > 0);

	// save copies of member variables we wish to use after the InterlockedDecrement
	UINT nIndex = m_nIndex;
	IUnknown* pUnk = m_pUnk;
	IID iid = m_iid;
	LPCTSTR lpszClassName = m_lpszClassName;
	bool bNonAddRefThunk = m_bNonAddRefThunk;

	ULONG l = InterlockedDecrement(&m_dwRef);

	TCHAR buf[512];
	wsprintf(buf, _T("QIThunk - %-10d\tRelease :\tObject = 0x%08x\tRefcount = %d\t"), nIndex, pUnk, l);
	OutputDebugString(buf);
	AtlDumpIID(iid, lpszClassName, S_OK);

	bool bDeleteThunk = (l == 0 && !bNonAddRefThunk);
	pUnk->Release();
	if (bDeleteThunk)
		_AtlDebugInterfacesModule.DeleteThunk(this);
	return l;
}

#endif 	// _ATL_DEBUG_INTERFACES


class CAtlWinModule : public _ATL_WIN_MODULE
{
public:
	CAtlWinModule()
	{
		cbSize = sizeof(_ATL_WIN_MODULE);
		HRESULT hr = AtlWinModuleInit(this);
		if (FAILED(hr))
			CAtlBaseModule::m_bInitFailed = true;
	}

	~CAtlWinModule()
	{
		Term();
	}

	void Term()
	{
		AtlWinModuleTerm(this, _AtlBaseModule.GetModuleInstance());
	}

	void AddCreateWndData(_AtlCreateWndData* pData, void* pObject)
	{
		AtlWinModuleAddCreateWndData(this, pData, pObject);
	}

	void* ExtractCreateWndData()
	{
		return AtlWinModuleExtractCreateWndData(this);
	}
};

__declspec(selectany) CAtlWinModule _AtlWinModule;


class CAtlModule;
__declspec(selectany) CAtlModule* _pAtlModule = NULL;


class ATL_NO_VTABLE CAtlModule : public _ATL_MODULE
{
public :
	static GUID m_libid;
	IGlobalInterfaceTable* m_pGIT;

	CAtlModule() throw()
	{
		// Should have only one instance of a class 
		// derived from CAtlModule in a project.
		ATLASSERT(_pAtlModule == NULL);
		cbSize = sizeof(_ATL_MODULE);
		m_pTermFuncs = NULL;

		m_nLockCnt = 0;
		_pAtlModule = this;
		if (FAILED(m_csStaticDataInitAndTypeInfo.Init()))
		{
			ATLTRACE(atlTraceGeneral, 0, _T("ERROR : Unable to initialize critical section in CAtlModule\n"));
			ATLASSERT(0);
			CAtlBaseModule::m_bInitFailed = true;
		}

		m_pGIT = NULL;
	}

	void Term() throw()
	{
		// cbSize == 0 indicates that Term has already been called
		if (cbSize == 0)
			return;

		// Call term functions
		if (m_pTermFuncs != NULL)
		{
			AtlCallTermFunc(this);
			m_pTermFuncs = NULL;
		}

		if (m_pGIT != NULL)
			m_pGIT->Release();

		m_csStaticDataInitAndTypeInfo.Term();

		cbSize = 0;
	}

	virtual ~CAtlModule() throw()
	{
		Term();
	}

	virtual LONG Lock() throw()
	{
		return CComGlobalsThreadModel::Increment(&m_nLockCnt);
	}

	virtual LONG Unlock() throw()
	{
		return CComGlobalsThreadModel::Decrement(&m_nLockCnt);
	}

	virtual LONG GetLockCount() throw()
	{
		return m_nLockCnt;
	}

	HRESULT AddTermFunc(_ATL_TERMFUNC* pFunc, DWORD_PTR dw) throw()
	{
		return AtlModuleAddTermFunc(this, pFunc, dw);
	}

	virtual HRESULT GetGITPtr(IGlobalInterfaceTable** ppGIT) throw()
	{
		ATLASSERT(ppGIT != NULL);

		if (ppGIT == NULL)
			return E_POINTER;

		HRESULT hr = S_OK;
		if (m_pGIT == NULL)
		{
			hr = ::CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER,
				__uuidof(IGlobalInterfaceTable), (void**)&m_pGIT);
		}

		if (SUCCEEDED(hr))
		{
			ATLASSERT(m_pGIT != NULL);
			*ppGIT = m_pGIT;
			m_pGIT->AddRef();
		}
		return hr;
	}

	virtual HRESULT AddCommonRGSReplacements(IRegistrarBase* /*pRegistrar*/) throw() = 0;

	// Resource-based Registration
#ifdef _ATL_STATIC_REGISTRY
	// Statically linking to Registry Ponent
	HRESULT WINAPI UpdateRegistryFromResourceS(LPCTSTR lpszRes, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw();
	HRESULT WINAPI UpdateRegistryFromResourceS(UINT nResID, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw();
#else
	HRESULT WINAPI UpdateRegistryFromResourceD(LPCTSTR lpszRes, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw()
	{
		if(lpszRes == NULL)
			return E_INVALIDARG;
		
		USES_CONVERSION_EX;
		LPCOLESTR pwszTemp = T2COLE_EX(lpszRes, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifdef _UNICODE
		if(pwszTemp == NULL) 
			return E_OUTOFMEMORY;
#endif		
		return UpdateRegistryFromResourceDHelper(pwszTemp, bRegister, pMapEntries);
	}
	HRESULT WINAPI UpdateRegistryFromResourceD(UINT nResID, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw()
	{
		return UpdateRegistryFromResourceDHelper((LPCOLESTR)MAKEINTRESOURCE(nResID), bRegister, pMapEntries);
	}
#endif

	// Implementation
#if !defined(_ATL_STATIC_REGISTRY)
	inline HRESULT WINAPI UpdateRegistryFromResourceDHelper(LPCOLESTR lpszRes, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw()
	{
		CComPtr<IRegistrar> spRegistrar;
		HRESULT hr = AtlCreateRegistrar(&spRegistrar);
		if (FAILED(hr))
			return hr;

		if (NULL != pMapEntries)
		{
			while (NULL != pMapEntries->szKey)
			{
				ATLASSERT(NULL != pMapEntries->szData);
				spRegistrar->AddReplacement(pMapEntries->szKey, pMapEntries->szData);
				pMapEntries++;
			}
		}

		hr = AddCommonRGSReplacements(spRegistrar);
		if (FAILED(hr))
			return hr;

		return AtlUpdateRegistryFromResourceD(_AtlBaseModule.GetModuleInstance(), lpszRes, bRegister,
			NULL, spRegistrar);
	}
#endif

	static void EscapeSingleQuote(LPOLESTR lpDest, LPCOLESTR lp) throw()
	{
		while (*lp)
		{
			*lpDest++ = *lp;
			if (*lp == '\'')
				*lpDest++ = *lp;
			lp++;
		}
		*lpDest = NULL;
	}


	// search for an occurence of string p2 in string p1
	static LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2) throw()
	{
		while (p1 != NULL && *p1 != NULL)
		{
			LPCTSTR p = p2;
			while (p != NULL && *p != NULL)
			{
				if (*p1 == *p)
					return CharNext(p1);
				p = CharNext(p);
			}
			p1 = CharNext(p1);
		}
		return NULL;
	}
#pragma warning(push)
#pragma warning(disable : 4302)	// 'type cast' : truncation from 'LPSTR' to 'TCHAR'

	static int WordCmpI(LPCTSTR psz1, LPCTSTR psz2) throw()
	{
		TCHAR c1 = (TCHAR)CharUpper((LPTSTR)*psz1);
		TCHAR c2 = (TCHAR)CharUpper((LPTSTR)*psz2);
		while (c1 != NULL && c1 == c2 && c1 != ' ' && c1 != '\t')
		{
			psz1 = CharNext(psz1);
			psz2 = CharNext(psz2);
			c1 = (TCHAR)CharUpper((LPTSTR)*psz1);
			c2 = (TCHAR)CharUpper((LPTSTR)*psz2);
		}
		if ((c1 == NULL || c1 == ' ' || c1 == '\t') && (c2 == NULL || c2 == ' ' || c2 == '\t'))
			return 0;

		return (c1 < c2) ? -1 : 1;
	}

#pragma warning (pop)	
};

__declspec(selectany) GUID CAtlModule::m_libid = {0x0, 0x0, 0x0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0} };

#define DECLARE_LIBID(libid) \
	static void InitLibId() throw() \
	{ \
		CAtlModule::m_libid = libid; \
	}

#define DECLARE_REGISTRY_APPID_RESOURCEID(resid, appid) \
	static LPCOLESTR GetAppId() throw() \
	{ \
		return OLESTR(appid); \
	} \
	static TCHAR* GetAppIdT() throw() \
	{ \
		return _T(appid); \
	} \
	static HRESULT WINAPI UpdateRegistryAppId(BOOL bRegister) throw() \
	{ \
		_ATL_REGMAP_ENTRY aMapEntries [] = \
		{ \
			{ OLESTR("APPID"), GetAppId() }, \
			{ NULL, NULL } \
		}; \
		return ATL::_pAtlModule->UpdateRegistryFromResource(resid, bRegister, aMapEntries); \
	}

inline HRESULT AtlGetGITPtr(IGlobalInterfaceTable** ppGIT) throw()
{
	if (ppGIT == NULL)
		return E_POINTER;

	if (_pAtlModule == NULL)
	{
		return CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER,
			__uuidof(IGlobalInterfaceTable), (void**)ppGIT);
	}
	else
	{
		return _pAtlModule->GetGITPtr(ppGIT);
	}
}

template <class T>
class ATL_NO_VTABLE CAtlModuleT : public CAtlModule
{
public :
	CAtlModuleT() throw()
	{
		T::InitLibId();
	}

	static void InitLibId() throw()
	{
	}

	HRESULT RegisterServer(BOOL bRegTypeLib = FALSE, const CLSID* pCLSID = NULL) throw()
	{
		(pCLSID);
		(bRegTypeLib);

		HRESULT hr = S_OK;

#ifndef _ATL_NO_COM_SUPPORT

		hr = _AtlComModule.RegisterServer(bRegTypeLib, pCLSID);

#endif	// _ATL_NO_COM_SUPPORT


#ifndef _ATL_NO_PERF_SUPPORT

		if (SUCCEEDED(hr) && _pPerfRegFunc != NULL)
			hr = (*_pPerfRegFunc)(_AtlBaseModule.m_hInst);

#endif

		return hr;
	}

	HRESULT UnregisterServer(BOOL bUnRegTypeLib, const CLSID* pCLSID = NULL) throw()
	{
		(bUnRegTypeLib);
		(pCLSID);

		HRESULT hr = S_OK;

#ifndef _ATL_NO_PERF_SUPPORT

		if (_pPerfUnRegFunc != NULL)
			hr = (*_pPerfUnRegFunc)();

#endif

#ifndef _ATL_NO_COM_SUPPORT

		if (SUCCEEDED(hr))
			hr = _AtlComModule.UnregisterServer(bUnRegTypeLib, pCLSID);

#endif	// _ATL_NO_COM_SUPPORT

		return hr;

	}

	static HRESULT WINAPI UpdateRegistryAppId(BOOL /*bRegister*/) throw()
	{
		return S_OK;
	}
	HRESULT RegisterAppId() throw()
	{
		return T::UpdateRegistryAppId(TRUE);
	}

	HRESULT UnregisterAppId() throw()
	{
		return T::UpdateRegistryAppId(FALSE);
	}

	virtual HRESULT AddCommonRGSReplacements(IRegistrarBase* pRegistrar) throw()
	{
		return pRegistrar->AddReplacement(L"APPID", T::GetAppId());
	}
	static LPCOLESTR GetAppId() throw()
	{
		return OLESTR("");
	}
};

template <class T>
class ATL_NO_VTABLE CAtlDllModuleT : public CAtlModuleT<T>
{
public :
	CAtlDllModuleT() throw()
	{
		_AtlComModule.ExecuteObjectMain(true);
	}

	~CAtlDllModuleT() throw()
	{
		_AtlComModule.ExecuteObjectMain(false);
	}

	BOOL WINAPI DllMain(DWORD dwReason, LPVOID /* lpReserved */) throw()
	{
		if (dwReason == DLL_PROCESS_ATTACH)
		{
			if (CAtlBaseModule::m_bInitFailed)
			{
				ATLASSERT(0);
				return FALSE;
			}

#ifdef _ATL_MIN_CRT
			DisableThreadLibraryCalls(_AtlBaseModule.GetModuleInstance());
#endif
		}
#ifdef _DEBUG
		else if (dwReason == DLL_PROCESS_DETACH)
		{
			// Prevent false memory leak reporting. ~CAtlWinModule may be too late.
			_AtlWinModule.Term();		
		}
#endif	// _DEBUG
		return TRUE;    // ok
	}

	HRESULT DllCanUnloadNow() throw()
	{
		T* pT = static_cast<T*>(this);
		return (pT->GetLockCount()==0) ? S_OK : S_FALSE;
	}

	HRESULT DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) throw()
	{
		T* pT = static_cast<T*>(this);
		return pT->GetClassObject(rclsid, riid, ppv);
	}

	HRESULT DllRegisterServer(BOOL bRegTypeLib = TRUE) throw()
	{
		// registers object, typelib and all interfaces in typelib
		T* pT = static_cast<T*>(this);
		HRESULT hr = pT->RegisterAppId();
		if (SUCCEEDED(hr))
			hr = pT->RegisterServer(bRegTypeLib);
		return hr;
	}

	HRESULT DllUnregisterServer(BOOL bUnRegTypeLib = TRUE) throw()
	{
		T* pT = static_cast<T*>(this);
		HRESULT hr = pT->UnregisterServer(bUnRegTypeLib);
		if (SUCCEEDED(hr))
			hr = pT->UnregisterAppId();
		return hr;
	}

	// Obtain a Class Factory
	HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) throw()
	{

#ifndef _ATL_OLEDB_CONFORMANCE_TESTS
		ATLASSERT(ppv != NULL);
#endif

		return AtlComModuleGetClassObject(&_AtlComModule, rclsid, riid, ppv);
	}
};

#ifndef _AFX

template <class T>
class ATL_NO_VTABLE CAtlExeModuleT : public CAtlModuleT<T>
{
public :
#ifndef _ATL_NO_COM_SUPPORT

	DWORD m_dwMainThreadID;
	HANDLE m_hEventShutdown;
	DWORD m_dwTimeOut;
	DWORD m_dwPause;
	bool m_bDelayShutdown;
	bool m_bActivity;

#endif // _ATL_NO_COM_SUPPORT

	CAtlExeModuleT() throw()

#ifndef _ATL_NO_COM_SUPPORT

		: m_dwMainThreadID(::GetCurrentThreadId()),
		m_dwTimeOut(5000),
		m_dwPause(1000),
		m_hEventShutdown(NULL),
		m_bDelayShutdown(true)

#endif // _ATL_NO_COM_SUPPORT

	{
		HRESULT hr = T::InitializeCom();
		if (FAILED(hr))
		{
		CAtlBaseModule::m_bInitFailed =	 true;
			return;
		}

#if !defined(_ATL_NO_COM_SUPPORT)

		_AtlComModule.ExecuteObjectMain(true);

#endif	//  !defined(_ATL_NO_COM_SUPPORT)

	}

	~CAtlExeModuleT() throw()
	{
#ifndef _ATL_NO_COM_SUPPORT

		_AtlComModule.ExecuteObjectMain(false);

#endif

		// Call term functions before COM is uninitialized
		Term();

#ifndef _ATL_NO_COM_SUPPORT

		// Clean up AtlComModule before COM is uninitialized
		_AtlComModule.Term();

#endif // _ATL_NO_COM_SUPPORT

		T::UninitializeCom();
	}

	static HRESULT InitializeCom() throw()
	{

#if ((_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM)) & defined(_ATL_FREE_THREADED)
		return CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
		return CoInitialize(NULL);
#endif
	}

	static void UninitializeCom() throw()
	{
		CoUninitialize();
	}

	LONG Unlock() throw()
	{
		LONG lRet = CComGlobalsThreadModel::Decrement(&m_nLockCnt);

#ifndef _ATL_NO_COM_SUPPORT

		if (lRet == 0)
		{
			if (m_bDelayShutdown)
			{
				m_bActivity = true;
				::SetEvent(m_hEventShutdown); // tell monitor that we transitioned to zero
			}
			else
			{
				::PostThreadMessage(m_dwMainThreadID, WM_QUIT, 0, 0);
			}
		}

#endif	// _ATL_NO_COM_SUPPORT

		return lRet;
	}

	void MonitorShutdown() throw()
	{
		while (1)
		{
			::WaitForSingleObject(m_hEventShutdown, INFINITE);
			DWORD dwWait = 0;
			do
			{
				m_bActivity = false;
				dwWait = ::WaitForSingleObject(m_hEventShutdown, m_dwTimeOut);
			} while (dwWait == WAIT_OBJECT_0);
			// timed out
			if (!m_bActivity && m_nLockCnt == 0) // if no activity let's really bail
			{
#if ((_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM)) & defined(_ATL_FREE_THREADED)
				::CoSuspendClassObjects();
				if (m_nLockCnt == 0)
#endif
					break;
			}
		}
		::CloseHandle(m_hEventShutdown);
		::PostThreadMessage(m_dwMainThreadID, WM_QUIT, 0, 0);
	}

	HANDLE StartMonitor() throw()
	{
		m_hEventShutdown = ::CreateEvent(NULL, false, false, NULL);
		if (m_hEventShutdown == NULL)
			return false;
		DWORD dwThreadID;
		HANDLE h = ::CreateThread(NULL, 0, MonitorProc, this, 0, &dwThreadID);
		return h;
	}

	static DWORD WINAPI MonitorProc(void* pv) throw()
	{
		CAtlExeModuleT<T>* p = static_cast<CAtlExeModuleT<T>*>(pv);
		p->MonitorShutdown();
		return 0;
	}

	int WinMain(int nShowCmd) throw()
	{
		if (CAtlBaseModule::m_bInitFailed)
		{
			ATLASSERT(0);
			return -1;
		}
		T* pT = static_cast<T*>(this);
		HRESULT hr = S_OK;

		LPTSTR lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
		if (pT->ParseCommandLine(lpCmdLine, &hr) == true)
			hr = pT->Run(nShowCmd);

#ifdef _DEBUG
		// Prevent false memory leak reporting. ~CAtlWinModule may be too late.
		_AtlWinModule.Term();		
#endif	// _DEBUG
		return hr;
	}

	// Scan command line and perform registration
	// Return value specifies if server should run

	// Parses the command line and registers/unregisters the rgs file if necessary
	bool ParseCommandLine(LPCTSTR lpCmdLine, HRESULT* pnRetCode) throw()
	{
		*pnRetCode = S_OK;

		TCHAR szTokens[] = _T("-/");

		T* pT = static_cast<T*>(this);
		LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
		while (lpszToken != NULL)
		{
			if (WordCmpI(lpszToken, _T("UnregServer"))==0)
			{
				*pnRetCode = pT->UnregisterServer(TRUE);
				if (SUCCEEDED(*pnRetCode))
					*pnRetCode = pT->UnregisterAppId();
				return false;
			}

			// Register as Local Server
			if (WordCmpI(lpszToken, _T("RegServer"))==0)
			{
				*pnRetCode = pT->RegisterAppId();
				if (SUCCEEDED(*pnRetCode))
					*pnRetCode = pT->RegisterServer(TRUE);
				return false;
			}

			lpszToken = FindOneOf(lpszToken, szTokens);
		}

		return true;
	}

	HRESULT PreMessageLoop(int /*nShowCmd*/) throw()
	{
		HRESULT hr = S_OK;
		T* pT = static_cast<T*>(this);
		pT;

#ifndef _ATL_NO_COM_SUPPORT

#if ((_WIN32_WINNT >= 0x0400 ) || defined(_WIN32_DCOM)) & defined(_ATL_FREE_THREADED)

		hr = pT->RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
			REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED);

		if (FAILED(hr))
			return hr;

		if (hr == S_OK)
		{
			if (m_bDelayShutdown)
			{
				CHandle h(pT->StartMonitor());
				if (h.m_h == NULL)
				{
					hr = E_FAIL;
				}
				else
				{
					hr = CoResumeClassObjects();
					ATLASSERT(SUCCEEDED(hr));
					if (FAILED(hr))
					{
						::SetEvent(m_hEventShutdown); // tell monitor to shutdown
						::WaitForSingleObject(h, m_dwTimeOut * 2);
					}
				}
			}
			else
			{
				hr = CoResumeClassObjects();
				ATLASSERT(SUCCEEDED(hr));
			}

			if (FAILED(hr))
				pT->RevokeClassObjects();
		}
		else
		{
			m_bDelayShutdown = false;
		}

#else

		hr = pT->RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
			REGCLS_MULTIPLEUSE);
		if (hr == S_OK)
		{
			if (m_bDelayShutdown && !pT->StartMonitor())
			{
				hr = E_FAIL;
			}
		}
		else
		{
			m_bDelayShutdown = false;
		}


#endif

#endif	// _ATL_NO_COM_SUPPORT

		ATLASSERT(SUCCEEDED(hr));
		return hr;
	}

	HRESULT PostMessageLoop() throw()
	{
		HRESULT hr = S_OK;

#ifndef _ATL_NO_COM_SUPPORT

		T* pT = static_cast<T*>(this);
		hr = pT->RevokeClassObjects();
		if (m_bDelayShutdown)
			Sleep(m_dwPause); //wait for any threads to finish

#endif	// _ATL_NO_COM_SUPPORT

		return hr;
	}

	void RunMessageLoop() throw()
	{
		MSG msg;
		while (GetMessage(&msg, 0, 0, 0) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	HRESULT Run(int nShowCmd = SW_HIDE) throw()
	{
		HRESULT hr = S_OK;

		T* pT = static_cast<T*>(this);
		hr = pT->PreMessageLoop(nShowCmd);

		// Call RunMessageLoop only if PreMessageLoop returns S_OK.
		if (hr == S_OK)
		{
			pT->RunMessageLoop();
		}

		// Call PostMessageLoop if PreMessageLoop returns success.
		if (SUCCEEDED(hr))
		{
			hr = pT->PostMessageLoop();
		}

		ATLASSERT(SUCCEEDED(hr));
		return hr;
	}

	// Register/Revoke All Class Factories with the OS (EXE only)
	HRESULT RegisterClassObjects(DWORD dwClsContext, DWORD dwFlags) throw()
	{
		return AtlComModuleRegisterClassObjects(&_AtlComModule, dwClsContext, dwFlags);
	}
	HRESULT RevokeClassObjects() throw()
	{
		return AtlComModuleRevokeClassObjects(&_AtlComModule);
	}
};

#ifndef _ATL_NO_SERVICE
template <class T, UINT nServiceNameID>
class ATL_NO_VTABLE CAtlServiceModuleT : public CAtlExeModuleT<T>
{
public :

	CAtlServiceModuleT() throw()
	{
		m_bService = TRUE;
		LoadString(_AtlBaseModule.GetModuleInstance(), nServiceNameID, m_szServiceName, sizeof(m_szServiceName) / sizeof(TCHAR));

		// set up the initial service status 
		m_hServiceStatus = NULL;
		m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		m_status.dwCurrentState = SERVICE_STOPPED;
		m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
		m_status.dwWin32ExitCode = 0;
		m_status.dwServiceSpecificExitCode = 0;
		m_status.dwCheckPoint = 0;
		m_status.dwWaitHint = 0;
	}

	int WinMain(int nShowCmd) throw()
	{
		if (CAtlBaseModule::m_bInitFailed)
		{
			ATLASSERT(0);
			return -1;
		}

		T* pT = static_cast<T*>(this);
		HRESULT hr = S_OK;

		LPTSTR lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
		if (pT->ParseCommandLine(lpCmdLine, &hr) == true)
			hr = pT->Start(nShowCmd);

#ifdef _DEBUG
		// Prevent false memory leak reporting. ~CAtlWinModule may be too late.
		_AtlWinModule.Term();		
#endif	// _DEBUG
		return hr;
	}

	HRESULT Start(int nShowCmd) throw()
	{
		T* pT = static_cast<T*>(this);
		// Are we Service or Local Server
		CRegKey keyAppID;
		LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_READ);
		if (lRes != ERROR_SUCCESS)
		{
			m_status.dwWin32ExitCode = lRes;
			return m_status.dwWin32ExitCode;
		}

		CRegKey key;
		lRes = key.Open(keyAppID, pT->GetAppIdT(), KEY_READ);
		if (lRes != ERROR_SUCCESS)
		{
			m_status.dwWin32ExitCode = lRes;
			return m_status.dwWin32ExitCode;
		}

		TCHAR szValue[MAX_PATH];
		DWORD dwLen = MAX_PATH;
		lRes = key.QueryStringValue(_T("LocalService"), szValue, &dwLen);

		m_bService = FALSE;
		if (lRes == ERROR_SUCCESS)
			m_bService = TRUE;

		if (m_bService)
		{
			SERVICE_TABLE_ENTRY st[] =
			{
				{ m_szServiceName, _ServiceMain },
				{ NULL, NULL }
			};
			if (::StartServiceCtrlDispatcher(st) == 0)
				m_status.dwWin32ExitCode = GetLastError();
			return m_status.dwWin32ExitCode;
		}
		// local server - call Run() directly, rather than
		// from ServiceMain()
		m_status.dwWin32ExitCode = pT->Run(nShowCmd);
		return m_status.dwWin32ExitCode;
	}

	inline HRESULT RegisterAppId(bool bService = false) throw()
	{
		if (!Uninstall())
			return E_FAIL;

		HRESULT hr = T::UpdateRegistryAppId(TRUE);
		if (FAILED(hr))
			return hr;

		CRegKey keyAppID;
		LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_WRITE);
		if (lRes != ERROR_SUCCESS)
			return AtlHresultFromWin32(lRes);

		CRegKey key;

		lRes = key.Create(keyAppID, T::GetAppIdT());
		if (lRes != ERROR_SUCCESS)
			return AtlHresultFromWin32(lRes);

		key.DeleteValue(_T("LocalService"));

		if (!bService)
			return S_OK;

		key.SetStringValue(_T("LocalService"), m_szServiceName);

		// Create service
		if (!Install())
			return E_FAIL;
		return S_OK;
	}

	HRESULT UnregisterAppId() throw()
	{
		if (!Uninstall())
			return E_FAIL;
		// First remove entries not in the RGS file.
		CRegKey keyAppID;
		LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_WRITE);
		if (lRes != ERROR_SUCCESS)
			return AtlHresultFromWin32(lRes);

		CRegKey key;
		lRes = key.Open(keyAppID, T::GetAppIdT(), KEY_WRITE);
		if (lRes != ERROR_SUCCESS)
			return AtlHresultFromWin32(lRes);
		key.DeleteValue(_T("LocalService"));

		return T::UpdateRegistryAppId(FALSE);
	}

	// Parses the command line and registers/unregisters the rgs file if necessary
	bool ParseCommandLine(LPCTSTR lpCmdLine, HRESULT* pnRetCode) throw()
	{
		if (!CAtlExeModuleT<T>::ParseCommandLine(lpCmdLine, pnRetCode))
			return false;

		TCHAR szTokens[] = _T("-/");
		*pnRetCode = S_OK;

		T* pT = static_cast<T*>(this);
		LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
		while (lpszToken != NULL)
		{
			if (WordCmpI(lpszToken, _T("Service"))==0)
			{
				*pnRetCode = pT->RegisterAppId(true);
				if (SUCCEEDED(*pnRetCode))
					*pnRetCode = pT->RegisterServer(TRUE);
				return false;
			}
			lpszToken = FindOneOf(lpszToken, szTokens);
		}
		return true;
	}

	void ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv) throw()
	{
		lpszArgv;
		dwArgc;
		HRESULT hr = E_FAIL;
		// Register the control request handler
		m_status.dwCurrentState = SERVICE_START_PENDING;
		m_hServiceStatus = RegisterServiceCtrlHandler(m_szServiceName, _Handler);
		if (m_hServiceStatus == NULL)
		{
			LogEvent(_T("Handler not installed"));
			return;
		}
		SetServiceStatus(SERVICE_START_PENDING);

		m_status.dwWin32ExitCode = S_OK;
		m_status.dwCheckPoint = 0;
		m_status.dwWaitHint = 0;

		T* pT = static_cast<T*>(this);
#ifndef _ATL_NO_COM_SUPPORT

		hr = T::InitializeCom();
		if (FAILED(hr))
			return;

		m_bDelayShutdown = false;
#endif
		// When the Run function returns, the service has stopped.
		m_status.dwWin32ExitCode = pT->Run(SW_HIDE);

#ifndef _ATL_NO_COM_SUPPORT
		if (m_bService)
			T::UninitializeCom();
#endif

		SetServiceStatus(SERVICE_STOPPED);
		LogEvent(_T("Service stopped"));
	}

	HRESULT Run(int nShowCmd = SW_HIDE) throw()
	{
		HRESULT hr = S_OK;
		T* pT = static_cast<T*>(this);

		hr = pT->PreMessageLoop(nShowCmd);

		if (hr == S_OK)
		{
			if (m_bService)
			{
				LogEvent(_T("Service started"));
				SetServiceStatus(SERVICE_RUNNING);
			}

			pT->RunMessageLoop();
		}

		if (SUCCEEDED(hr))
		{
			hr = pT->PostMessageLoop();
		}

		return hr;
	}

	HRESULT PreMessageLoop(int nShowCmd) throw()
	{
		HRESULT hr = S_OK;
		if (m_bService)
		{
			m_dwThreadID = GetCurrentThreadId();

			T* pT = static_cast<T*>(this);
			hr = pT->InitializeSecurity();

			if (FAILED(hr))
				return hr;
		}

		hr = CAtlExeModuleT<T>::PreMessageLoop(nShowCmd);
		if (FAILED(hr))
			return hr;

		return hr;
	}

	void OnStop() throw()
	{
		SetServiceStatus(SERVICE_STOP_PENDING);
		PostThreadMessage(m_dwThreadID, WM_QUIT, 0, 0);
	}

	void OnPause() throw()
	{
	}

	void OnContinue() throw()
	{
	}

	void OnInterrogate() throw()
	{
	}

	void OnShutdown() throw()
	{
	}

	void OnUnknownRequest(DWORD /*dwOpcode*/) throw()
	{
		LogEvent(_T("Bad service request"));	
	}

	void Handler(DWORD dwOpcode) throw()
	{
		T* pT = static_cast<T*>(this);

		switch (dwOpcode)
		{
		case SERVICE_CONTROL_STOP:
			pT->OnStop();
			break;
		case SERVICE_CONTROL_PAUSE:
			pT->OnPause();
			break;
		case SERVICE_CONTROL_CONTINUE:
			pT->OnContinue();
			break;
		case SERVICE_CONTROL_INTERROGATE:
			pT->OnInterrogate();
			break;
		case SERVICE_CONTROL_SHUTDOWN:
			pT->OnShutdown();
			break;
		default:
			pT->OnUnknownRequest(dwOpcode);
		}
	}

	BOOL IsInstalled() throw()
	{
		BOOL bResult = FALSE;

		SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

		if (hSCM != NULL)
		{
			SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, SERVICE_QUERY_CONFIG);
			if (hService != NULL)
			{
				bResult = TRUE;
				::CloseServiceHandle(hService);
			}
			::CloseServiceHandle(hSCM);
		}
		return bResult;
	}
	BOOL Install() throw()
	{
		if (IsInstalled())
			return TRUE;

		// Get the executable file path
		TCHAR szFilePath[MAX_PATH + _ATL_QUOTES_SPACE];
		DWORD dwFLen = ::GetModuleFileName(NULL, szFilePath + 1, MAX_PATH);
		if( dwFLen == 0 || dwFLen == MAX_PATH )
			return FALSE;

		// Quote the FilePath before calling CreateService
		szFilePath[0] = _T('\"');
		szFilePath[dwFLen + 1] = _T('\"');
		szFilePath[dwFLen + 2] = 0;
        
		SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (hSCM == NULL)
		{
			TCHAR szBuf[1024];
			if (AtlLoadString(ATL_SERVICE_MANAGER_OPEN_ERROR, szBuf, 1024) == 0)
				lstrcpy(szBuf,  _T("Could not open Service Manager"));
			MessageBox(NULL, szBuf, m_szServiceName, MB_OK);
			return FALSE;
		}

		SC_HANDLE hService = ::CreateService(
			hSCM, m_szServiceName, m_szServiceName,
			SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
			SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
			szFilePath, NULL, NULL, _T("RPCSS\0"), NULL, NULL);

		if (hService == NULL)
		{
			::CloseServiceHandle(hSCM);
			TCHAR szBuf[1024];
			if (AtlLoadString(ATL_SERVICE_START_ERROR, szBuf, 1024) == 0)
				lstrcpy(szBuf,  _T("Could not start service"));
			MessageBox(NULL, szBuf, m_szServiceName, MB_OK);
			return FALSE;
		}

		::CloseServiceHandle(hService);
		::CloseServiceHandle(hSCM);
		return TRUE;
	}

	BOOL Uninstall() throw()
	{
		if (!IsInstalled())
			return TRUE;

		SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

		if (hSCM == NULL)
		{
			TCHAR szBuf[1024];
			if (AtlLoadString(ATL_SERVICE_MANAGER_OPEN_ERROR, szBuf, 1024) == 0)
				lstrcpy(szBuf,  _T("Could not open Service Manager"));
			MessageBox(NULL, szBuf, m_szServiceName, MB_OK);
			return FALSE;
		}

		SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, SERVICE_STOP | DELETE);

		if (hService == NULL)
		{
			::CloseServiceHandle(hSCM);
			TCHAR szBuf[1024];
			if (AtlLoadString(ATL_SERVICE_OPEN_ERROR, szBuf, 1024) == 0)
				lstrcpy(szBuf,  _T("Could not open service"));
			MessageBox(NULL, szBuf, m_szServiceName, MB_OK);
			return FALSE;
		}
		SERVICE_STATUS status;
		BOOL bRet = ::ControlService(hService, SERVICE_CONTROL_STOP, &status);
		if (!bRet)
		{
			DWORD dwError = GetLastError();
			if (!((dwError == ERROR_SERVICE_NOT_ACTIVE) ||  (dwError == ERROR_SERVICE_CANNOT_ACCEPT_CTRL && status.dwCurrentState == SERVICE_STOP_PENDING)))
			{
				TCHAR szBuf[1024];
				if (AtlLoadString(ATL_SERVICE_STOP_ERROR, szBuf, 1024) == 0)
					lstrcpy(szBuf,  _T("Could not stop service"));
				MessageBox(NULL, szBuf, m_szServiceName, MB_OK);
			}
		}


		BOOL bDelete = ::DeleteService(hService);
		::CloseServiceHandle(hService);
		::CloseServiceHandle(hSCM);

		if (bDelete)
			return TRUE;

		TCHAR szBuf[1024];
		if (AtlLoadString(ATL_SERVICE_DELETE_ERROR, szBuf, 1024) == 0)
			lstrcpy(szBuf,  _T("Could not delete service"));
		MessageBox(NULL, szBuf, m_szServiceName, MB_OK);
		return FALSE;
	}

	LONG Unlock() throw()
	{
		LONG lRet;
		if (m_bService)
		{
			// We are running as a service, therefore transition to zero does not
			// unload the process
			lRet = CAtlModuleT<T>::Unlock();
		}
		else
		{
			// We are running as EXE, use MonitorShutdown logic provided by CExeModule
			lRet = CAtlExeModuleT<T>::Unlock();
		}
		return lRet;
	}

	void LogEventEx(int id, LPCTSTR pszMessage=NULL, WORD type = EVENTLOG_INFORMATION_TYPE) throw()
	{
		HANDLE hEventSource;
		if (m_szServiceName)
		{
			/* Get a handle to use with ReportEvent(). */
			hEventSource = RegisterEventSource(NULL, m_szServiceName);
			if (hEventSource != NULL)
			{
				/* Write to event log. */
				ReportEvent(hEventSource, 
							type,
							(WORD)0,
							id,
							NULL,
							(WORD)(pszMessage != NULL ? 1 : 0),
							0,
							pszMessage != NULL ? &pszMessage : NULL,
							NULL);
				DeregisterEventSource(hEventSource);
			}
		}
	}

	void __cdecl LogEvent(LPCTSTR pszFormat, ...) throw()
	{
		TCHAR chMsg[256];
		HANDLE hEventSource;
		LPTSTR lpszStrings[1];
		va_list pArg;

		va_start(pArg, pszFormat);
		_vstprintf(chMsg, pszFormat, pArg);
		va_end(pArg);

		lpszStrings[0] = chMsg;

		if (!m_bService)
		{
			// Not running as a service, so print out the error message 
			// to the console if possible
			_putts(chMsg);
		}

		/* Get a handle to use with ReportEvent(). */
		hEventSource = RegisterEventSource(NULL, m_szServiceName);
		if (hEventSource != NULL)
		{
			/* Write to event log. */
			ReportEvent(hEventSource, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, 1, 0, (LPCTSTR*) &lpszStrings[0], NULL);
			DeregisterEventSource(hEventSource);
		}
	}
	void SetServiceStatus(DWORD dwState) throw()
	{
		m_status.dwCurrentState = dwState;
		::SetServiceStatus(m_hServiceStatus, &m_status);
	}

//Implementation
protected:
	static void WINAPI _ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv) throw()
	{
		((T*)_pAtlModule)->ServiceMain(dwArgc, lpszArgv);
	}
	static void WINAPI _Handler(DWORD dwOpcode) throw()
	{
		((T*)_pAtlModule)->Handler(dwOpcode); 
	}

// data members
public:
	TCHAR m_szServiceName[256];
	SERVICE_STATUS_HANDLE m_hServiceStatus;
	SERVICE_STATUS m_status;
	BOOL m_bService;
	DWORD m_dwThreadID;
};

#endif //	_ATL_NO_SERVICE

#endif	// !_AFX

#ifdef _AFX
class CAtlMfcModule : public ATL::CAtlModuleT<CAtlMfcModule>
{
public :
	virtual LONG Lock() throw()
	{
#ifdef _USRDLL
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
		AfxOleLockApp();
		return AfxGetModuleState()->m_nObjectCount;
	}

	virtual LONG Unlock() throw()
	{
#ifdef _USRDLL
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
		AfxOleUnlockApp();
		return AfxGetModuleState()->m_nObjectCount;
	}

	virtual LONG GetLockCount() throw()
	{
#ifdef _USRDLL
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
		return AfxGetModuleState()->m_nObjectCount;
	}

	// Obtain a Class Factory (DLL only)
	HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
	{
		return ATL::AtlComModuleGetClassObject(&ATL::_AtlComModule, rclsid, riid, ppv);
	}

	// Register/Revoke All Class Factories with the OS (EXE only)
	HRESULT RegisterClassObjects(DWORD dwClsContext, DWORD dwFlags)
	{
		return ATL::AtlComModuleRegisterClassObjects(&ATL::_AtlComModule, dwClsContext, dwFlags);
	}

	HRESULT RevokeClassObjects()
	{
		return ATL::AtlComModuleRevokeClassObjects(&ATL::_AtlComModule);
	}
};

#endif //	_AFX

/////////////////////////////////////////////////////////////////////////////
// CComModule - Uses the functionality provided by other modules

#define THREADFLAGS_APARTMENT 0x1
#define THREADFLAGS_BOTH 0x2
#define AUTPRXFLAG 0x4

#ifndef _ATL_NO_COMMODULE

class CComModule;
__declspec(selectany) CComModule* _pModule = NULL;
class CComModule : public CAtlModuleT<CComModule>
{
public :

	CComModule()
	{
		// Should have only one instance of a class 
		// derived from CComModule in a project.
		ATLASSERT(_pModule == NULL);
		_pModule = this;
	}
	__declspec(property(get = get_m_hInst)) HINSTANCE m_hInst;
	HINSTANCE& get_m_hInst() const throw()
	{
		return _AtlBaseModule.m_hInst;
	}

	__declspec(property(get = get_m_hInstResource, put = put_m_hInstResource)) HINSTANCE m_hInstResource;
	HINSTANCE& get_m_hInstResource() const throw()
	{
		return _AtlBaseModule.m_hInstResource;
	}
	void put_m_hInstResource(HINSTANCE h) throw()
	{
		_AtlBaseModule.SetResourceInstance(h);
	}
	HINSTANCE SetResourceInstance(HINSTANCE h) throw()
	{
		return _AtlBaseModule.SetResourceInstance(h);
	}

	HINSTANCE GetModuleInstance() throw()
	{
		return _AtlBaseModule.m_hInst;
	}
	HINSTANCE GetResourceInstance() throw()
	{
		return _AtlBaseModule.m_hInstResource;
	}

	__declspec(property(get = get_m_hInstTypeLib, put = put_m_hInstTypeLib)) HINSTANCE m_hInstTypeLib;
	HINSTANCE& get_m_hInstTypeLib() const throw()
	{
		return _AtlComModule.m_hInstTypeLib;
	}
	void put_m_hInstTypeLib(HINSTANCE h) throw()
	{
		_AtlComModule.m_hInstTypeLib = h;
	}

	HINSTANCE GetTypeLibInstance() const throw()
	{
		return _AtlComModule.m_hInstTypeLib;
	}

	// For Backward compatibility
	_ATL_OBJMAP_ENTRY* m_pObjMap;

	__declspec(property(get  = get_m_csWindowCreate)) CRITICAL_SECTION m_csWindowCreate;
	CRITICAL_SECTION& get_m_csWindowCreate() throw();

	__declspec(property(get  = get_m_csObjMap)) CRITICAL_SECTION m_csObjMap;
	CRITICAL_SECTION& get_m_csObjMap() throw();

	__declspec(property(get  = get_m_csStaticDataInit)) CRITICAL_SECTION m_csTypeInfoHolder;
	__declspec(property(get  = get_m_csStaticDataInit)) CRITICAL_SECTION m_csStaticDataInit;
	CRITICAL_SECTION& get_m_csStaticDataInit() throw();
	void EnterStaticDataCriticalSection() throw()
	{
		EnterCriticalSection(&m_csStaticDataInit);
	}

	void LeaveStaticDataCriticalSection() throw()
	{
		LeaveCriticalSection(&m_csStaticDataInit);
	}

	__declspec(property(get  = get_dwAtlBuildVer)) DWORD dwAtlBuildVer;
	DWORD& get_dwAtlBuildVer() throw()
	{
		return _AtlBaseModule.dwAtlBuildVer;
	}

	__declspec(property(get  = get_m_pCreateWndList, put = put_m_pCreateWndList)) _AtlCreateWndData* m_pCreateWndList;
	_AtlCreateWndData*& get_m_pCreateWndList() throw();
	void put_m_pCreateWndList(_AtlCreateWndData* p) throw();

	__declspec(property(get  = get_pguidVer)) const GUID* pguidVer;
	const GUID*& get_pguidVer() throw()
	{
		return _AtlBaseModule.pguidVer;
	}

#ifdef _ATL_DEBUG_INTERFACES

	__declspec(property(get  = get_m_nIndexQI, put = put_m_nIndexQI)) UINT m_nIndexQI;
	UINT& get_m_nIndexQI() throw();
	void put_m_nIndexQI(UINT nIndex) throw();

	__declspec(property(get  = get_m_nIndexBreakAt, put = put_m_nIndexBreakAt)) UINT m_nIndexBreakAt;
	UINT& get_m_nIndexBreakAt() throw();
	void put_m_nIndexBreakAt(UINT nIndex) throw();

	__declspec(property(get  = get_m_paThunks)) CSimpleArray<_QIThunk*>* m_paThunks;
	CSimpleArray<_QIThunk*>* get_m_paThunks() throw();
	HRESULT AddThunk(IUnknown** pp, LPCTSTR lpsz, REFIID iid) throw();

	HRESULT AddNonAddRefThunk(IUnknown* p, LPCTSTR lpsz, IUnknown** ppThunkRet) throw();
	void DeleteNonAddRefThunk(IUnknown* pUnk) throw();
	void DeleteThunk(_QIThunk* p) throw();
	bool DumpLeakedThunks() throw();
#endif // _ATL_DEBUG_INTERFACES

	HRESULT Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, const GUID* plibid = NULL) throw();
	void Term() throw();

	HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) throw();
	// Register/Revoke All Class Factories with the OS (EXE only)
	HRESULT RegisterClassObjects(DWORD dwClsContext, DWORD dwFlags) throw();
	HRESULT RevokeClassObjects() throw();
	// Registry support (helpers)
	HRESULT RegisterTypeLib() throw();
	HRESULT RegisterTypeLib(LPCTSTR lpszIndex) throw();
	HRESULT UnRegisterTypeLib() throw();
	HRESULT UnRegisterTypeLib(LPCTSTR lpszIndex) throw();
	HRESULT RegisterServer(BOOL bRegTypeLib = FALSE, const CLSID* pCLSID = NULL) throw();
	HRESULT UnregisterServer(BOOL bUnRegTypeLib, const CLSID* pCLSID = NULL) throw();
	HRESULT UnregisterServer(const CLSID* pCLSID = NULL) throw();

	void AddCreateWndData(_AtlCreateWndData* pData, void* pObject) throw()
	{
		_AtlWinModule.AddCreateWndData(pData, pObject);
	}

	void* ExtractCreateWndData() throw()
	{
		return _AtlWinModule.ExtractCreateWndData();
	}

	// Only used in CComAutoThreadModule
	HRESULT CreateInstance(void* /*pfnCreateInstance*/, REFIID /*riid*/, void** /*ppvObj*/) throw()	
	{
		ATLASSERT(0);
		ATLTRACENOTIMPL(_T("CComModule::CreateInstance"));
	}

	HRESULT RegisterAppId(LPCTSTR pAppId);
	HRESULT UnregisterAppId(LPCTSTR pAppId);

	// Resource-based Registration
	virtual HRESULT WINAPI UpdateRegistryFromResourceD(LPCTSTR lpszRes, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw()
	{
#if defined(_ATL_STATIC_REGISTRY)
		(lpszRes);
		(bRegister);
		(pMapEntries);
		return E_FAIL;
#else
		return CAtlModuleT<CComModule>::UpdateRegistryFromResourceD(lpszRes, bRegister, pMapEntries);		
#endif
	}
	virtual HRESULT WINAPI UpdateRegistryFromResourceD(UINT nResID, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw()
	{
#if defined(_ATL_STATIC_REGISTRY)
		(nResID);
		(bRegister);
		(pMapEntries);
		return E_FAIL;
#else
		return CAtlModuleT<CComModule>::UpdateRegistryFromResourceD(nResID, bRegister, pMapEntries);
#endif
	}


	// Statically linking to Registry Ponent
	virtual HRESULT WINAPI UpdateRegistryFromResourceS(LPCTSTR lpszRes, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw()
	{
#ifdef _ATL_STATIC_REGISTRY
		return CAtlModuleT<CComModule>::UpdateRegistryFromResourceS(lpszRes, bRegister, pMapEntries);
#else
		(lpszRes);
		(bRegister);
		(pMapEntries);
		return E_FAIL;
#endif
	}
	virtual HRESULT WINAPI UpdateRegistryFromResourceS(UINT nResID, BOOL bRegister,
		struct _ATL_REGMAP_ENTRY* pMapEntries = NULL) throw()
	{
#ifdef _ATL_STATIC_REGISTRY
		return CAtlModuleT<CComModule>::UpdateRegistryFromResourceS(nResID, bRegister, pMapEntries);
#else
		(nResID);
		(bRegister);
		(pMapEntries);
		return E_FAIL;
#endif
	}


	// Use RGS file for registration

	ATL_DEPRECATED static HRESULT RegisterProgID(LPCTSTR lpszCLSID, LPCTSTR lpszProgID, LPCTSTR lpszUserDesc);
	// Standard Registration
	ATL_DEPRECATED HRESULT WINAPI UpdateRegistryClass(const CLSID& clsid, LPCTSTR lpszProgID,
		LPCTSTR lpszVerIndProgID, UINT nDescID, DWORD dwFlags, BOOL bRegister);
	ATL_DEPRECATED HRESULT WINAPI UpdateRegistryClass(const CLSID& clsid, LPCTSTR lpszProgID,
		LPCTSTR lpszVerIndProgID, LPCTSTR szDesc, DWORD dwFlags, BOOL bRegister);
	ATL_DEPRECATED HRESULT WINAPI RegisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
		LPCTSTR lpszVerIndProgID, LPCTSTR szDesc, DWORD dwFlags);
	ATL_DEPRECATED HRESULT WINAPI UnregisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
		LPCTSTR lpszVerIndProgID);

	BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /* lpReserved */, _ATL_OBJMAP_ENTRY* pObjMap, const GUID* pLibID)
	{
		if (dwReason == DLL_PROCESS_ATTACH)
		{
			if (CAtlBaseModule::m_bInitFailed)
			{
				ATLASSERT(0);
				return FALSE;
			}

			if (FAILED(Init(pObjMap, hInstance, pLibID)))
			{
				Term();
				return FALSE;
			}
#ifdef _ATL_MIN_CRT			
			DisableThreadLibraryCalls(hInstance);
#endif
		}
		else if (dwReason == DLL_PROCESS_DETACH)
			Term();
		return TRUE;    // ok
	}

	HRESULT DllCanUnloadNow()  throw()
	{
		return (GetLockCount()==0) ? S_OK : S_FALSE;
	}
	HRESULT DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)  throw()
	{
		return GetClassObject(rclsid, riid, ppv);
	}

	HRESULT DllRegisterServer(BOOL bRegTypeLib = TRUE)  throw()
	{
		// registers object, typelib and all interfaces in typelib
		return RegisterServer(bRegTypeLib);
	}

	HRESULT DllUnregisterServer(BOOL bUnRegTypeLib = TRUE)  throw()
	{
		return UnregisterServer(bUnRegTypeLib);
	}

};

#endif	// !_ATL_NO_COMMODULE

/////////////////////////////////////////////////////////////////////////////
// Thread creation helpers

#if !defined(_ATL_MIN_CRT) && defined(_MT)
// CRTThreadTraits
// This class is for use with CThreadPool or CWorkerThread
// It should be used if the worker class will use CRT
// functions.
class CRTThreadTraits
{
public:
	static HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpsa, DWORD dwStackSize, LPTHREAD_START_ROUTINE pfnThreadProc, void *pvParam, DWORD dwCreationFlags, DWORD *pdwThreadId) throw()
	{
		ATLASSERT(sizeof(DWORD) == sizeof(unsigned int)); // sanity check for pdwThreadId

		// _beginthreadex calls CreateThread which will set the last error value before it returns.
		return (HANDLE) _beginthreadex(lpsa, dwStackSize, (unsigned int (__stdcall *)(void *)) pfnThreadProc, pvParam, dwCreationFlags, (unsigned int *) pdwThreadId);
	}
};

#endif

// Win32ThreadTraits
// This class is for use with CThreadPool or CWorkerThread
// It should be used if the worker class will not use CRT
// functions.
class Win32ThreadTraits
{
public:
	static HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpsa, DWORD dwStackSize, LPTHREAD_START_ROUTINE pfnThreadProc, void *pvParam, DWORD dwCreationFlags, DWORD *pdwThreadId) throw()
	{
		return ::CreateThread(lpsa, dwStackSize, pfnThreadProc, pvParam, dwCreationFlags, pdwThreadId);
	}
};

#if !defined(_ATL_MIN_CRT) && defined(_MT)
	typedef CRTThreadTraits DefaultThreadTraits;
#else
	typedef Win32ThreadTraits DefaultThreadTraits;
#endif

template <typename T>
HANDLE CreateThreadT(LPSECURITY_ATTRIBUTES lpsa, DWORD dwStackSize, DWORD (WINAPI * pfn)(T *pparam), 
					 T *pparam, DWORD dwCreationFlags, LPDWORD pdw)
{
	return DefaultThreadTraits::CreateThread(lpsa, 
		dwStackSize, 
		(LPTHREAD_START_ROUTINE)pfn, 
		pparam, 
		dwCreationFlags, 
		pdw);
}

template <typename T>
HANDLE AtlCreateThread(DWORD (WINAPI* pfn)(T *pparam), T *pparam)
{
	return CreateThreadT(0, 0, pfn, pparam, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Thread Pooling classes

class _AtlAptCreateObjData
{
public:
	_ATL_CREATORFUNC* pfnCreateInstance;
	const IID* piid;
	HANDLE hEvent;
	LPSTREAM pStream;
	HRESULT hRes;
};

class CComApartment
{
public:
	CComApartment()
	{
		m_nLockCnt = 0;
		m_hThread = NULL;
	}
	static UINT ATL_CREATE_OBJECT;
	static DWORD WINAPI _Apartment(void* pv)
	{
		ATLASSERT(pv != NULL);
		return ((CComApartment*)pv)->Apartment();
	}
	DWORD Apartment()
	{
		CoInitialize(NULL);
		MSG msg;
		while(GetMessage(&msg, 0, 0, 0) > 0)
		{
			if (msg.message == ATL_CREATE_OBJECT)
			{
				_AtlAptCreateObjData* pdata = (_AtlAptCreateObjData*)msg.lParam;
				IUnknown* pUnk = NULL;
				pdata->hRes = pdata->pfnCreateInstance(NULL, __uuidof(IUnknown), (void**)&pUnk);
				if (SUCCEEDED(pdata->hRes))
					pdata->hRes = CoMarshalInterThreadInterfaceInStream(*pdata->piid, pUnk, &pdata->pStream);
				if (SUCCEEDED(pdata->hRes))
				{
					pUnk->Release();
					ATLTRACE(atlTraceCOM, 2, _T("Object created on thread = %d\n"), GetCurrentThreadId());
				}
#ifdef _DEBUG
				else
				{
					ATLTRACE(atlTraceCOM, 2, _T("Failed to create Object on thread = %d\n"), GetCurrentThreadId());
				}
#endif
				SetEvent(pdata->hEvent);
			}
			DispatchMessage(&msg);
		}
		CoUninitialize();

		return 0;
	}
	LONG Lock() {return CComGlobalsThreadModel::Increment(&m_nLockCnt);}
	LONG Unlock(){return CComGlobalsThreadModel::Decrement(&m_nLockCnt);
	}
	LONG GetLockCount() {return m_nLockCnt;}

	DWORD m_dwThreadID;
	HANDLE m_hThread;
	LONG m_nLockCnt;
};

__declspec(selectany) UINT CComApartment::ATL_CREATE_OBJECT = 0;

class CComSimpleThreadAllocator
{
public:
	CComSimpleThreadAllocator()
	{
		m_nThread = 0;
	}
	int GetThread(CComApartment* /*pApt*/, int nThreads)
	{
		if (++m_nThread == nThreads)
			m_nThread = 0;
		return m_nThread;
	}
	int m_nThread;
};

__interface IAtlAutoThreadModule
{
	virtual HRESULT CreateInstance(void* pfnCreateInstance, REFIID riid, void** ppvObj);
};

__declspec(selectany) IAtlAutoThreadModule* _pAtlAutoThreadModule;

template <class T, class ThreadAllocator = CComSimpleThreadAllocator, DWORD dwWait = INFINITE>
class ATL_NO_VTABLE CAtlAutoThreadModuleT : public IAtlAutoThreadModule
{
// This class is not for use in a DLL.
// If this class were used in a DLL,  there will be a deadlock when the DLL is unloaded.
// because of dwWait's default value of INFINITE
public:
	CAtlAutoThreadModuleT(int nThreads = T::GetDefaultThreads())
	{
		ATLASSERT(_pAtlAutoThreadModule == NULL);
		_pAtlAutoThreadModule = this;
		m_pApartments = NULL;
		m_nThreads= 0;

		ATLTRY(m_pApartments = new CComApartment[nThreads]);
		ATLASSERT(m_pApartments != NULL);
		if(m_pApartments == NULL)
		{
			CAtlBaseModule::m_bInitFailed = true;
		}

		memset(m_pApartments, 0, sizeof(CComApartment) * nThreads);

		m_nThreads = nThreads;
		for (int i = 0; i < nThreads; i++)
		{

#if !defined(_ATL_MIN_CRT) && defined(_MT)
			typedef unsigned ( __stdcall *pfnThread )( void * );
			m_pApartments[i].m_hThread = (HANDLE)_beginthreadex(NULL, 0, (pfnThread)CComApartment::_Apartment, &m_pApartments[i], 0, (UINT*)&m_pApartments[i].m_dwThreadID);
			if (m_pApartments[i].m_hThread == NULL)
			{
				HRESULT hr = E_FAIL;
				switch (errno)
				{
				case EAGAIN:
					hr = HRESULT_FROM_WIN32(ERROR_TOO_MANY_TCBS);
					break;
				case EINVAL:
					hr = E_INVALIDARG;
					break;
				}
				CAtlBaseModule::m_bInitFailed = true;
				break;
			}

#else
			m_pApartments[i].m_hThread = ::CreateThread(NULL, 0, CComApartment::_Apartment, (void*)&m_pApartments[i], 0, &m_pApartments[i].m_dwThreadID);
			// clean up allocated threads
			if (m_pApartments[i].m_hThread == NULL)
			{
				CAtlBaseModule::m_bInitFailed = true;
				break;
			}

#endif

		}
		if (!CAtlBaseModule::m_bInitFailed)
			CComApartment::ATL_CREATE_OBJECT = RegisterWindowMessage(_T("ATL_CREATE_OBJECT"));
	}

	virtual ~CAtlAutoThreadModuleT()
	{
		if (m_pApartments == NULL)
			return;

		DWORD dwCurrentThreadId = GetCurrentThreadId();
		int nCurrentThread = -1;
		for (int i=0; i < m_nThreads; i++)
		{
			if (m_pApartments[i].m_hThread == NULL)
				continue;
			if (m_pApartments[i].m_dwThreadID == dwCurrentThreadId)
			{
				nCurrentThread = i;
				continue;
			}
			while (::PostThreadMessage(m_pApartments[i].m_dwThreadID, WM_QUIT, 0, 0) == 0)
			{
				if (GetLastError() == ERROR_INVALID_THREAD_ID)
				{
					ATLASSERT(FALSE);
					break;
				}
				::Sleep(100);
			}
			::WaitForSingleObject(m_pApartments[i].m_hThread, dwWait);
			CloseHandle(m_pApartments[i].m_hThread);
		}
		if (nCurrentThread != -1)
			CloseHandle(m_pApartments[nCurrentThread].m_hThread);

		delete [] m_pApartments;
		m_pApartments = NULL;
	}

	HRESULT CreateInstance(void* pfnCreateInstance, REFIID riid, void** ppvObj)
	{
		ATLASSERT(ppvObj != NULL);
		if (ppvObj == NULL)
			return E_POINTER;
		*ppvObj = NULL;

		_ATL_CREATORFUNC* pFunc = (_ATL_CREATORFUNC*) pfnCreateInstance;
		_AtlAptCreateObjData data;
		data.pfnCreateInstance = pFunc;
		data.piid = &riid;
		data.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		data.hRes = S_OK;
		int nThread = m_Allocator.GetThread(m_pApartments, m_nThreads);
		int nIterations = 0;
		while(::PostThreadMessage(m_pApartments[nThread].m_dwThreadID, CComApartment::ATL_CREATE_OBJECT, 0, (LPARAM)&data) == 0 && ++nIterations < 100)
		{
			Sleep(100);
		}
		if (nIterations < 100)
		{
			AtlWaitWithMessageLoop(data.hEvent);
		}
		else
		{
			data.hRes = AtlHresultFromLastError();
		}
		CloseHandle(data.hEvent);
		if (SUCCEEDED(data.hRes))
			data.hRes = CoGetInterfaceAndReleaseStream(data.pStream, riid, ppvObj);
		return data.hRes;
	}
	DWORD dwThreadID;
	int m_nThreads;
	CComApartment* m_pApartments;
	ThreadAllocator m_Allocator;
	static int GetDefaultThreads()
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		return si.dwNumberOfProcessors * 4;
	}
};

class CAtlAutoThreadModule : public CAtlAutoThreadModuleT<CAtlAutoThreadModule>
{
public :
};

#ifndef _ATL_NO_COMMODULE

template <class ThreadAllocator = CComSimpleThreadAllocator, DWORD dwWait = INFINITE>
class CComAutoThreadModule : 
	public CComModule,
	public CAtlAutoThreadModuleT<CComAutoThreadModule, ThreadAllocator, dwWait>
{
public:
	CComAutoThreadModule(int nThreads = GetDefaultThreads()) :
		CAtlAutoThreadModuleT<CComAutoThreadModule, ThreadAllocator, dwWait>(nThreads)
	{
	}
	HRESULT Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, const GUID* plibid = NULL, int nThreads = GetDefaultThreads())
	{
		ATLASSERT(nThreads == GetDefaultThreads() && _T("Set number of threads through the constructor"));
		return CComModule::Init(p, h, plibid);
	}
};

#endif	// !_ATL_NO_COMMODULE

/////////////////////////////////////////////////////////////////////////////
// GIT Pointer 

template <class T>
class CComGITPtr
{
public:
	CComGITPtr() throw()
	{
		m_dwCookie = 0;
	}
	CComGITPtr(T* p)
	{
		m_dwCookie = 0;
		HRESULT hr = Attach(p);

		if (FAILED(hr))
			AtlThrow(hr);
	}
	CComGITPtr(const CComGITPtr& git)
	{
		m_dwCookie = 0;
		CComPtr<T> spT;

		HRESULT hr = git.CopyTo(&spT);
		if (SUCCEEDED(hr))
			hr = Attach(spT);

		if (FAILED(hr))
			AtlThrow(hr);
	}
	explicit CComGITPtr(DWORD dwCookie) throw()
	{
		ATLASSERT(m_dwCookie != NULL);		
		m_dwCookie = dwCookie;

#ifdef _DEBUG
		CComPtr<T> spT;
		HRESULT hr = CopyTo(&spT);
		ATLASSERT(SUCCEEDED(hr));
#endif
	}

	~CComGITPtr() throw()
	{
		Revoke();
	}
	CComGITPtr<T>& operator=(const CComGITPtr<T>& git)
	{
		CComPtr<T> spT;

		HRESULT hr = git.CopyTo(&spT);
		if (SUCCEEDED(hr))
			hr = Attach(spT);

		if (FAILED(hr))
			AtlThrow(hr);

		return *this;
	}
	CComGITPtr<T>& operator=(T* p)
	{
		HRESULT hr = Attach(p);
		if (FAILED(hr))
			AtlThrow(hr);
		return *this;
	}
	CComGITPtr<T>& operator=(DWORD dwCookie)
	{
		HRESULT hr = Attach(dwCookie);
		if (FAILED(hr))
			AtlThrow(hr);

		m_dwCookie = dwCookie;

#ifdef _DEBUG
		CComPtr<T> spT;
		hr = CopyTo(&spT);
		ATLASSERT(SUCCEEDED(hr));
#endif

		return *this;
	}

	// Get the cookie from the class
	operator DWORD() const
	{
		return m_dwCookie;
	}
	// Get the cookie from the class
	DWORD GetCookie() const
	{
		return m_dwCookie;
	}
	// Register the passed interface pointer in the GIT
	HRESULT Attach(T* p) throw()
	{
		CComPtr<IGlobalInterfaceTable> spGIT;
		HRESULT hr = E_FAIL;
		hr = AtlGetGITPtr(&spGIT);
		ATLASSERT(spGIT != NULL);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		if (m_dwCookie != 0)
			hr = spGIT->RevokeInterfaceFromGlobal(m_dwCookie);
		if (FAILED(hr))
			return hr;

		return spGIT->RegisterInterfaceInGlobal(p, __uuidof(T), &m_dwCookie);
	}

	HRESULT Attach(DWORD dwCookie) throw()
	{
		ATLASSERT(dwCookie != NULL);
		HRESULT hr = Revoke();
		if (FAILED(hr))
			return hr;
		m_dwCookie = dwCookie;
		return S_OK;
	}

	// Detach
	DWORD Detach() throw()
	{
		DWORD dwCookie = m_dwCookie;
		m_dwCookie = NULL;
		return dwCookie;
	}

	// Remove the interface from the GIT 
	HRESULT Revoke() throw()
	{
		HRESULT hr = S_OK;
		if (m_dwCookie != 0)
		{
			CComPtr<IGlobalInterfaceTable> spGIT;
			HRESULT hr = E_FAIL;
			hr = AtlGetGITPtr(&spGIT);

			ATLASSERT(spGIT != NULL);
			ATLASSERT(SUCCEEDED(hr));
			if (FAILED(hr))
				return hr;

			hr = spGIT->RevokeInterfaceFromGlobal(m_dwCookie);
			if (SUCCEEDED(hr))
				m_dwCookie = 0;
		}
		return hr;
	}
	// Get's the interface from the GIT and copies it to the passed pointer. The pointer
	// must be released by the caller when finished.
	HRESULT CopyTo(T** pp) const throw()
	{
		CComPtr<IGlobalInterfaceTable> spGIT;
		HRESULT hr = E_FAIL;
		hr = AtlGetGITPtr(&spGIT);

		ATLASSERT(spGIT != NULL);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;

		ATLASSERT(m_dwCookie!=NULL);
		return spGIT->GetInterfaceFromGlobal(m_dwCookie, __uuidof(T), (void**)pp);
	}
	DWORD m_dwCookie;
};

/////////////////////////////////////////////////////////////////////////////
// CRegKey

class CRegKey
{
public:
	CRegKey() throw();
	CRegKey( CRegKey& key ) throw();
	explicit CRegKey(HKEY hKey) throw();
	~CRegKey() throw();

	CRegKey& operator=( CRegKey& key ) throw();

// Attributes
public:
	operator HKEY() const throw();
	HKEY m_hKey;

// Operations
public:
	ATL_DEPRECATED LONG SetValue(DWORD dwValue, LPCTSTR lpszValueName);
	ATL_DEPRECATED LONG SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL, bool bMulti = false, int nValueLen = -1);
	LONG SetValue(LPCTSTR pszValueName, DWORD dwType, const void* pValue, ULONG nBytes) throw();
	LONG SetGUIDValue(LPCTSTR pszValueName, REFGUID guidValue) throw();
	LONG SetBinaryValue(LPCTSTR pszValueName, const void* pValue, ULONG nBytes) throw();
	LONG SetDWORDValue(LPCTSTR pszValueName, DWORD dwValue) throw();
	LONG SetQWORDValue(LPCTSTR pszValueName, ULONGLONG qwValue) throw();
	LONG SetStringValue(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD dwType = REG_SZ) throw();
	LONG SetMultiStringValue(LPCTSTR pszValueName, LPCTSTR pszValue) throw();

	ATL_DEPRECATED LONG QueryValue(DWORD& dwValue, LPCTSTR lpszValueName);
	ATL_DEPRECATED LONG QueryValue(LPTSTR szValue, LPCTSTR lpszValueName, DWORD* pdwCount);
	LONG QueryValue(LPCTSTR pszValueName, DWORD* pdwType, void* pData, ULONG* pnBytes) throw();
	LONG QueryGUIDValue(LPCTSTR pszValueName, GUID& guidValue) throw();
	LONG QueryBinaryValue(LPCTSTR pszValueName, void* pValue, ULONG* pnBytes) throw();
	LONG QueryDWORDValue(LPCTSTR pszValueName, DWORD& dwValue) throw();
	LONG QueryQWORDValue(LPCTSTR pszValueName, ULONGLONG& qwValue) throw();
	LONG QueryStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars) throw();
	LONG QueryMultiStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars) throw();

	// Get the key's security attributes.
	LONG GetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd, LPDWORD pnBytes) throw();
	// Set the key's security attributes.
	LONG SetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd) throw();

	LONG SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL) throw();
	static LONG WINAPI SetValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
		LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);

	// Create a new registry key (or open an existing one).
	LONG Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
		LPTSTR lpszClass = REG_NONE, DWORD dwOptions = REG_OPTION_NON_VOLATILE,
		REGSAM samDesired = KEY_READ | KEY_WRITE,
		LPSECURITY_ATTRIBUTES lpSecAttr = NULL,
		LPDWORD lpdwDisposition = NULL) throw();
	// Open an existing registry key.
	LONG Open(HKEY hKeyParent, LPCTSTR lpszKeyName,
		REGSAM samDesired = KEY_READ | KEY_WRITE) throw();
	// Close the registry key.
	LONG Close() throw();
	// Flush the key's data to disk.
	LONG Flush() throw();

	// Detach the CRegKey object from its HKEY.  Releases ownership.
	HKEY Detach() throw();
	// Attach the CRegKey object to an existing HKEY.  Takes ownership.
	void Attach(HKEY hKey) throw();

	// Enumerate the subkeys of the key.
	LONG EnumKey(DWORD iIndex, LPTSTR pszName, LPDWORD pnNameLength, FILETIME* pftLastWriteTime = NULL) throw();
	LONG NotifyChangeKeyValue(BOOL bWatchSubtree, DWORD dwNotifyFilter, HANDLE hEvent, BOOL bAsync = TRUE) throw();

	LONG DeleteSubKey(LPCTSTR lpszSubKey) throw();
	LONG RecurseDeleteKey(LPCTSTR lpszKey) throw();
	LONG DeleteValue(LPCTSTR lpszValue) throw();
};

inline CRegKey::CRegKey() throw() : 
	m_hKey( NULL )
{
}

inline CRegKey::CRegKey( CRegKey& key ) throw() :
	m_hKey( NULL )
{
	Attach( key.Detach() );
}

inline CRegKey::CRegKey(HKEY hKey) throw() :
	m_hKey(hKey)
{
}

inline CRegKey::~CRegKey() throw()
{Close();}

inline CRegKey& CRegKey::operator=( CRegKey& key ) throw()
{
	Close();
	Attach( key.Detach() );

	return( *this );
}

inline CRegKey::operator HKEY() const throw()
{return m_hKey;}

inline HKEY CRegKey::Detach() throw()
{
	HKEY hKey = m_hKey;
	m_hKey = NULL;
	return hKey;
}

inline void CRegKey::Attach(HKEY hKey) throw()
{
	ATLASSERT(m_hKey == NULL);
	m_hKey = hKey;
}

inline LONG CRegKey::DeleteSubKey(LPCTSTR lpszSubKey) throw()
{
	ATLASSERT(m_hKey != NULL);
	return RegDeleteKey(m_hKey, lpszSubKey);
}

inline LONG CRegKey::DeleteValue(LPCTSTR lpszValue) throw()
{
	ATLASSERT(m_hKey != NULL);
	return RegDeleteValue(m_hKey, (LPTSTR)lpszValue);
}

inline LONG CRegKey::Close() throw()
{
	LONG lRes = ERROR_SUCCESS;
	if (m_hKey != NULL)
	{
		lRes = RegCloseKey(m_hKey);
		m_hKey = NULL;
	}
	return lRes;
}

inline LONG CRegKey::Flush() throw()
{
	ATLASSERT(m_hKey != NULL);

	return ::RegFlushKey(m_hKey);
}

inline LONG CRegKey::EnumKey(DWORD iIndex, LPTSTR pszName, LPDWORD pnNameLength, FILETIME* pftLastWriteTime) throw()
{
	FILETIME ftLastWriteTime;

	ATLASSERT(m_hKey != NULL);
	if (pftLastWriteTime == NULL)
	{
		pftLastWriteTime = &ftLastWriteTime;
	}

	return ::RegEnumKeyEx(m_hKey, iIndex, pszName, pnNameLength, NULL, NULL, NULL, pftLastWriteTime);
}

inline LONG CRegKey::NotifyChangeKeyValue(BOOL bWatchSubtree, DWORD dwNotifyFilter, HANDLE hEvent, BOOL bAsync) throw()
{
	ATLASSERT(m_hKey != NULL);
	ATLASSERT((hEvent != NULL) || !bAsync);

	return ::RegNotifyChangeKeyValue(m_hKey, bWatchSubtree, dwNotifyFilter, hEvent, bAsync);
}

inline LONG CRegKey::Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
	LPTSTR lpszClass, DWORD dwOptions, REGSAM samDesired,
	LPSECURITY_ATTRIBUTES lpSecAttr, LPDWORD lpdwDisposition) throw()
{
	ATLASSERT(hKeyParent != NULL);
	DWORD dw;
	HKEY hKey = NULL;
	LONG lRes = RegCreateKeyEx(hKeyParent, lpszKeyName, 0,
		lpszClass, dwOptions, samDesired, lpSecAttr, &hKey, &dw);
	if (lpdwDisposition != NULL)
		*lpdwDisposition = dw;
	if (lRes == ERROR_SUCCESS)
	{
		lRes = Close();
		m_hKey = hKey;
	}
	return lRes;
}

inline LONG CRegKey::Open(HKEY hKeyParent, LPCTSTR lpszKeyName, REGSAM samDesired) throw()
{
	ATLASSERT(hKeyParent != NULL);
	HKEY hKey = NULL;
	LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, samDesired, &hKey);
	if (lRes == ERROR_SUCCESS)
	{
		lRes = Close();
		ATLASSERT(lRes == ERROR_SUCCESS);
		m_hKey = hKey;
	}
	return lRes;
}

#pragma warning(push)
#pragma warning(disable: 4996)
inline LONG CRegKey::QueryValue(DWORD& dwValue, LPCTSTR lpszValueName)
{
	DWORD dwType = NULL;
	DWORD dwCount = sizeof(DWORD);
	LONG lRes = RegQueryValueEx(m_hKey, lpszValueName, NULL, &dwType,
		(LPBYTE)&dwValue, &dwCount);
	ATLASSERT((lRes!=ERROR_SUCCESS) || (dwType == REG_DWORD));
	ATLASSERT((lRes!=ERROR_SUCCESS) || (dwCount == sizeof(DWORD)));
	if (dwType != REG_DWORD)
		return ERROR_INVALID_DATA;
	return lRes;
}

inline LONG CRegKey::QueryValue(LPTSTR pszValue, LPCTSTR lpszValueName, DWORD* pdwCount)
{
	ATLASSERT(pdwCount != NULL);
	DWORD dwType = NULL;
	LONG lRes = RegQueryValueEx(m_hKey, lpszValueName, NULL, &dwType,
		(LPBYTE)pszValue, pdwCount);
	ATLASSERT((lRes!=ERROR_SUCCESS) || (dwType == REG_SZ) ||
			 (dwType == REG_MULTI_SZ) || (dwType == REG_EXPAND_SZ));
	if (pszValue != NULL)
	{
		switch(dwType)
		{
			case REG_SZ:
			case REG_EXPAND_SZ:
				if ((*pdwCount) % sizeof(TCHAR) != 0 || pszValue[(*pdwCount) / sizeof(TCHAR) - 1] != 0)
		 			return ERROR_INVALID_DATA;
				break;
			case REG_MULTI_SZ:
				if ((*pdwCount) % sizeof(TCHAR) != 0 || (*pdwCount) / sizeof(TCHAR) < 2 || pszValue[(*pdwCount) / sizeof(TCHAR) -1] != 0 || pszValue[(*pdwCount) / sizeof(TCHAR) - 2] != 0 )
					return ERROR_INVALID_DATA;
				break;
			default:
				return ERROR_INVALID_DATA;
		}
	}
	return lRes;
}
#pragma warning(pop)	// 4996

inline LONG CRegKey::QueryValue(LPCTSTR pszValueName, DWORD* pdwType, void* pData, ULONG* pnBytes) throw()
{
	ATLASSERT(m_hKey != NULL);

	return( ::RegQueryValueEx(m_hKey, pszValueName, NULL, pdwType, static_cast< LPBYTE >( pData ), pnBytes) );
}

inline LONG CRegKey::QueryDWORDValue(LPCTSTR pszValueName, DWORD& dwValue) throw()
{
	LONG lRes;
	ULONG nBytes;
	DWORD dwType;

	ATLASSERT(m_hKey != NULL);

	nBytes = sizeof(DWORD);
	lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(&dwValue),
		&nBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_DWORD)
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}
inline LONG CRegKey::QueryQWORDValue(LPCTSTR pszValueName, ULONGLONG& qwValue) throw()
{
	LONG lRes;
	ULONG nBytes;
	DWORD dwType;

	ATLASSERT(m_hKey != NULL);

	nBytes = sizeof(ULONGLONG);
	lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(&qwValue),
		&nBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_QWORD)
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}

inline LONG CRegKey::QueryBinaryValue(LPCTSTR pszValueName, void* pValue, ULONG* pnBytes) throw()
{
	LONG lRes;
	DWORD dwType;

	ATLASSERT(pnBytes != NULL);
	ATLASSERT(m_hKey != NULL);

	lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(pValue),
		pnBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_BINARY)
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}

inline LONG CRegKey::QueryStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars) throw()
{
	LONG lRes;
	DWORD dwType;
	ULONG nBytes;

	ATLASSERT(m_hKey != NULL);
	ATLASSERT(pnChars != NULL);

	nBytes = (*pnChars)*sizeof(TCHAR);
	*pnChars = 0;
	lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(pszValue),
		&nBytes);
	
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if(dwType != REG_SZ && dwType != REG_EXPAND_SZ)
		return ERROR_INVALID_DATA;
	if (pszValue != NULL && (nBytes % sizeof(TCHAR) != 0 || pszValue[nBytes / sizeof(TCHAR) -1] != 0))
 		return ERROR_INVALID_DATA;

 	*pnChars = nBytes/sizeof(TCHAR);
	
	return ERROR_SUCCESS;
}

inline LONG CRegKey::QueryMultiStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars) throw()
{
	LONG lRes;
	DWORD dwType;
	ULONG nBytes;

	ATLASSERT(m_hKey != NULL);
	ATLASSERT(pnChars != NULL);

	if (pszValue != NULL && *pnChars < 2)
		return ERROR_INSUFFICIENT_BUFFER;
		
	nBytes = (*pnChars)*sizeof(TCHAR);
	*pnChars = 0;
	
	lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(pszValue),
		&nBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_MULTI_SZ)
		return ERROR_INVALID_DATA;
	if (pszValue != NULL && (nBytes % sizeof(TCHAR) != 0 || nBytes / sizeof(TCHAR) < 2 || pszValue[nBytes / sizeof(TCHAR) -1] != 0 || pszValue[nBytes / sizeof(TCHAR) - 2] != 0))
		return ERROR_INVALID_DATA;

	*pnChars = nBytes/sizeof(TCHAR);

	return ERROR_SUCCESS;
}

inline LONG CRegKey::QueryGUIDValue(LPCTSTR pszValueName, GUID& guidValue) throw()
{
	TCHAR szGUID[64];
	LONG lRes;
	ULONG nCount;
	HRESULT hr;

	ATLASSERT(m_hKey != NULL);

	guidValue = GUID_NULL;

	nCount = 64;
	lRes = QueryStringValue(pszValueName, szGUID, &nCount);

	if (lRes != ERROR_SUCCESS)
		return lRes;

	if(szGUID[0] != _T('{'))
		return ERROR_INVALID_DATA;

	USES_CONVERSION_EX;
	LPOLESTR lpstr = T2OLE_EX(szGUID, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
	if(lpstr == NULL) 
		return E_OUTOFMEMORY;
#endif	
		
	hr = ::CLSIDFromString(lpstr, &guidValue);
	if (FAILED(hr))
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}

inline LONG WINAPI CRegKey::SetValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
	ATLASSERT(lpszValue != NULL);
	CRegKey key;
	LONG lRes = key.Create(hKeyParent, lpszKeyName);
	if (lRes == ERROR_SUCCESS)
		lRes = key.SetStringValue(lpszValueName, lpszValue);
	return lRes;
}

inline LONG CRegKey::SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName) throw()
{
	ATLASSERT(lpszValue != NULL);
	CRegKey key;
	LONG lRes = key.Create(m_hKey, lpszKeyName);
	if (lRes == ERROR_SUCCESS)
		lRes = key.SetStringValue(lpszValueName, lpszValue);
	return lRes;
}

#pragma warning(push)
#pragma warning(disable: 4996)
inline LONG CRegKey::SetValue(DWORD dwValue, LPCTSTR pszValueName)
{
	ATLASSERT(m_hKey != NULL);
	return SetDWORDValue(pszValueName, dwValue);
}

inline LONG CRegKey::SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName, bool bMulti, int nValueLen)
{
	ATLASSERT(lpszValue != NULL);
	ATLASSERT(m_hKey != NULL);

	if (bMulti && nValueLen == -1)
		return ERROR_INVALID_PARAMETER;

	if (nValueLen == -1)
		nValueLen = lstrlen(lpszValue) + 1;

	DWORD dwType = bMulti ? REG_MULTI_SZ : REG_SZ;

	return ::RegSetValueEx(m_hKey, lpszValueName, NULL, dwType,
		reinterpret_cast<const BYTE*>(lpszValue), nValueLen*sizeof(TCHAR));
}
#pragma warning(pop)

inline LONG CRegKey::SetValue(LPCTSTR pszValueName, DWORD dwType, const void* pValue, ULONG nBytes) throw()
{
	ATLASSERT(m_hKey != NULL);
	return ::RegSetValueEx(m_hKey, pszValueName, NULL, dwType, static_cast<const BYTE*>(pValue), nBytes);
}

inline LONG CRegKey::SetBinaryValue(LPCTSTR pszValueName, const void* pData, ULONG nBytes) throw()
{
	ATLASSERT(m_hKey != NULL);
	return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_BINARY, reinterpret_cast<const BYTE*>(pData), nBytes);
}

inline LONG CRegKey::SetDWORDValue(LPCTSTR pszValueName, DWORD dwValue) throw()
{
	ATLASSERT(m_hKey != NULL);
	return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_DWORD, reinterpret_cast<const BYTE*>(&dwValue), sizeof(DWORD));
}

inline LONG CRegKey::SetQWORDValue(LPCTSTR pszValueName, ULONGLONG qwValue) throw()
{
	ATLASSERT(m_hKey != NULL);
	return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_QWORD, reinterpret_cast<const BYTE*>(&qwValue), sizeof(ULONGLONG));
}

inline LONG CRegKey::SetStringValue(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD dwType) throw()
{
	ATLASSERT(m_hKey != NULL);
	ATLASSERT(pszValue != NULL);
	ATLASSERT((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ));

	return ::RegSetValueEx(m_hKey, pszValueName, NULL, dwType, reinterpret_cast<const BYTE*>(pszValue), (lstrlen(pszValue)+1)*sizeof(TCHAR));
}

inline LONG CRegKey::SetMultiStringValue(LPCTSTR pszValueName, LPCTSTR pszValue) throw()
{
	LPCTSTR pszTemp;
	ULONG nBytes;
	ULONG nLength;

	ATLASSERT(m_hKey != NULL);
	ATLASSERT(pszValue != NULL);

	// Find the total length (in bytes) of all of the strings, including the
	// terminating '\0' of each string, and the second '\0' that terminates
	// the list.
	nBytes = 0;
	pszTemp = pszValue;
	do
	{
		nLength = lstrlen(pszTemp)+1;
		pszTemp += nLength;
		nBytes += nLength*sizeof(TCHAR);
	} while (nLength != 1);

	return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_MULTI_SZ, reinterpret_cast<const BYTE*>(pszValue),
		nBytes);
}

inline LONG CRegKey::SetGUIDValue(LPCTSTR pszValueName, REFGUID guidValue) throw()
{
	OLECHAR szGUID[64];

	ATLASSERT(m_hKey != NULL);

	::StringFromGUID2(guidValue, szGUID, 64);

	USES_CONVERSION_EX;
	LPCTSTR lpstr = OLE2CT_EX(szGUID, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
	if(lpstr == NULL) 
		return E_OUTOFMEMORY;
#endif	
	return SetStringValue(pszValueName, lpstr);
}

inline LONG CRegKey::GetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd, LPDWORD pnBytes) throw()
{
	ATLASSERT(m_hKey != NULL);
	ATLASSERT(pnBytes != NULL);

	return ::RegGetKeySecurity(m_hKey, si, psd, pnBytes);
}

inline LONG CRegKey::SetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd) throw()
{
	ATLASSERT(m_hKey != NULL);
	ATLASSERT(psd != NULL);

	return ::RegSetKeySecurity(m_hKey, si, psd);
}

inline LONG CRegKey::RecurseDeleteKey(LPCTSTR lpszKey) throw()
{
	CRegKey key;
	LONG lRes = key.Open(m_hKey, lpszKey, KEY_READ | KEY_WRITE);
	if (lRes != ERROR_SUCCESS)
	{
		if (lRes != ERROR_FILE_NOT_FOUND && lRes != ERROR_PATH_NOT_FOUND)
		{
			ATLTRACE(atlTraceCOM, 0, _T("CRegKey::RecurseDeleteKey : Failed to Open Key %s(Error = %d)\n"), lpszKey, lRes);
		}
		return lRes;
	}
	FILETIME time;
	DWORD dwSize = 256;
	TCHAR szBuffer[256];
	while (RegEnumKeyEx(key.m_hKey, 0, szBuffer, &dwSize, NULL, NULL, NULL,
		&time)==ERROR_SUCCESS)
	{
		lRes = key.RecurseDeleteKey(szBuffer);
		if (lRes != ERROR_SUCCESS)
			return lRes;
		dwSize = 256;
	}
	key.Close();
	return DeleteSubKey(lpszKey);
}

#ifndef _ATL_NO_COMMODULE

inline HRESULT CComModule::RegisterProgID(LPCTSTR lpszCLSID, LPCTSTR lpszProgID, LPCTSTR lpszUserDesc)
{
	CRegKey keyProgID;
	LONG lRes = keyProgID.Create(HKEY_CLASSES_ROOT, lpszProgID, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE);
	if (lRes == ERROR_SUCCESS)
	{
		lRes = keyProgID.SetStringValue(NULL, lpszUserDesc);
		if (lRes == ERROR_SUCCESS)
		{
			lRes = keyProgID.SetKeyValue(_T("CLSID"), lpszCLSID);
			if (lRes == ERROR_SUCCESS)
				return S_OK;
		}
	}
	return AtlHresultFromWin32(lRes);
}

inline HRESULT CComModule::RegisterAppId(LPCTSTR pAppId)
{
	CRegKey keyAppID;
	HRESULT hr = S_OK;
	LONG lRet;

	if ( (lRet = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_WRITE)) == ERROR_SUCCESS)
	{
		TCHAR szModule1[MAX_PATH];
		TCHAR szModule2[MAX_PATH];
		TCHAR* pszFileName;

		DWORD dwFLen = ::GetModuleFileName(GetModuleInstance(), szModule1, MAX_PATH);
		if ( dwFLen != 0 && dwFLen != MAX_PATH )
		{
			if (::GetFullPathName(szModule1, MAX_PATH, szModule2, &pszFileName) != 0)
			{
				CRegKey keyAppIDEXE;
				if ( (lRet = keyAppIDEXE.Create(keyAppID, pszFileName, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE)) == ERROR_SUCCESS)
				{
					lRet = keyAppIDEXE.SetStringValue(_T("AppID"), pAppId);
					if (lRet != ERROR_SUCCESS) 
					{
						ATLTRACE(atlTraceCOM, 0, _T("CComModule::RegisterAppId : Failed to set app id string value\n"));
						hr = AtlHresultFromWin32(lRet);
						return hr;
					}
				}
				else
				{
					ATLTRACE(atlTraceCOM, 0, _T("CComModule::RegisterAppId : Failed to create file name key\n"));
					hr = AtlHresultFromWin32(lRet);
					return hr;
				}
				if ( (lRet = keyAppIDEXE.Create(keyAppID, pAppId, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE)) == ERROR_SUCCESS)
				{	
					lRet = keyAppIDEXE.SetStringValue(NULL, pszFileName);
					if (lRet != ERROR_SUCCESS)
					{
						ATLTRACE(atlTraceCOM, 0, _T("CComModule::RegisterAppId : Failed to set file name string value\n"));
						hr = AtlHresultFromWin32(lRet);
						return hr;
					}
				}
				else
				{
					ATLTRACE(atlTraceCOM, 0, _T("CComModule::RegisterAppId : Failed to create app id key\n"));
					hr = AtlHresultFromWin32(lRet);
					return hr;
				}
			}
			else
			{
				ATLTRACE(atlTraceCOM, 0, _T("CComModule::RegisterAppId : Failed to get full path name for file %s\n"), szModule1);
				hr = AtlHresultFromLastError();
			}
		}
		else
		{
			ATLTRACE(atlTraceCOM, 0, _T("CComModule::RegisterAppId : Failed to get module name\n"));
			if( dwFLen == 0 )
				hr = AtlHresultFromLastError();
			else if( dwFLen == MAX_PATH )
				hr =  HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
		}
	}
	else
	{
		ATLTRACE(atlTraceCOM, 0, _T("CComModule::RegisterAppId : Failed to open registry key\n"));
		hr = AtlHresultFromWin32(lRet);
	}
	return hr;
}

inline HRESULT CComModule::UnregisterAppId(LPCTSTR pAppId)
{
	CRegKey keyAppID;
	HRESULT hr = S_OK;
	LONG lRet = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_READ | KEY_WRITE);

	if (lRet == ERROR_SUCCESS)
	{
		TCHAR szModule1[MAX_PATH];
		TCHAR szModule2[MAX_PATH];
		TCHAR* pszFileName;

		DWORD dwFLen = ::GetModuleFileName(GetModuleInstance(), szModule1, MAX_PATH);
		if ( dwFLen != 0 && dwFLen != MAX_PATH )
		{
			if (::GetFullPathName(szModule1, MAX_PATH, szModule2, &pszFileName) != 0)
			{
				if ((lRet = keyAppID.RecurseDeleteKey(pAppId)) != ERROR_SUCCESS)
				{
					if (lRet != ERROR_FILE_NOT_FOUND) 
						hr = AtlHresultFromWin32(lRet);
				}
				if ((lRet = keyAppID.RecurseDeleteKey(pszFileName)) != ERROR_SUCCESS)
				{
					if (lRet != ERROR_FILE_NOT_FOUND) 
						hr = AtlHresultFromWin32(lRet);
				}
			}
			else
			{
				ATLTRACE(atlTraceCOM, 0, _T("CComModule::UnregisterAppId : Failed to get full path name for file %s\n"), szModule1);
				hr = AtlHresultFromLastError();
			}
		}
		else
		{
			ATLTRACE(atlTraceCOM, 0, _T("CComModule::UnregisterAppId : Failed to get module name\n"));
			if( dwFLen == 0 )
				hr = AtlHresultFromLastError();
			else if( dwFLen == MAX_PATH )
				hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
		}
	}
	else
	{
		if (lRet != ERROR_FILE_NOT_FOUND && lRet != ERROR_PATH_NOT_FOUND)
		{
			ATLTRACE(atlTraceCOM, 0, _T("CComModule::UnregisterAppId : Failed to open registry key\n"));
			hr = AtlHresultFromWin32(lRet);
		}
	}
	return hr;
}
#endif	// !_ATL_NO_COMMODULE

#ifdef _ATL_STATIC_REGISTRY
}	// namespace ATL

#include <statreg.h>

namespace ATL
{

// Statically linking to Registry Ponent
inline HRESULT WINAPI CAtlModule::UpdateRegistryFromResourceS(LPCTSTR lpszRes, BOOL bRegister,
	struct _ATL_REGMAP_ENTRY* pMapEntries /*= NULL*/) throw()
{
	CRegObject ro;

	if (pMapEntries != NULL)
	{
		while (pMapEntries->szKey != NULL)
		{
			ATLASSERT(NULL != pMapEntries->szData);
			ro.AddReplacement(pMapEntries->szKey, pMapEntries->szData);
			pMapEntries++;
		}
	}

	HRESULT hr = AddCommonRGSReplacements(&ro);
	if (FAILED(hr))
		return hr;

	USES_CONVERSION_EX;
	TCHAR szModule[MAX_PATH];
	HINSTANCE hInst = _AtlBaseModule.GetModuleInstance();
	DWORD dwFLen = GetModuleFileName(hInst, szModule, MAX_PATH);
	if( dwFLen == 0 )
		return AtlHresultFromLastError();
	else if( dwFLen == MAX_PATH )
		return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

	LPOLESTR pszModule;
	pszModule = T2OLE_EX(szModule, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
	if(pszModule == NULL) 
		return E_OUTOFMEMORY;
#endif

	OLECHAR pszModuleUnquoted[_MAX_PATH * 2];
	EscapeSingleQuote(pszModuleUnquoted, pszModule);
	
	HRESULT hRes;
	if ((hInst == NULL) || (hInst == GetModuleHandle(NULL))) // register as EXE
	{
		// If Registering as an EXE, then we quote the resultant path.
		// We don't do it for a DLL, because LoadLibrary fails if the path is
		// quoted
		OLECHAR pszModuleQuote[(_MAX_PATH + _ATL_QUOTES_SPACE)*2]; 		
		pszModuleQuote[0] = OLESTR('\"');
		ocscpy(pszModuleQuote + 1, pszModuleUnquoted);
		int nLen = ocslen(pszModuleQuote);
		pszModuleQuote[nLen] = OLESTR('\"');
		pszModuleQuote[nLen + 1] = 0;

		hRes = ro.AddReplacement(OLESTR("Module"), pszModuleQuote);
	}
	else
	{
		hRes = ro.AddReplacement(OLESTR("Module"), pszModuleUnquoted);
	}	
	
	if(FAILED(hRes))
		return hRes;
		
	hRes = ro.AddReplacement(OLESTR("Module_Raw"), pszModuleUnquoted);
	if(FAILED(hRes))
		return hRes;

	LPCOLESTR szType = OLESTR("REGISTRY");
	LPCOLESTR pszRes = T2COLE_EX(lpszRes, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE	
	if(pszRes == NULL) 
		return E_OUTOFMEMORY;
#endif	
	hr = (bRegister) ? ro.ResourceRegisterSz(pszModule, pszRes, szType) :
		ro.ResourceUnregisterSz(pszModule, pszRes, szType);
	return hr;
}
inline HRESULT WINAPI CAtlModule::UpdateRegistryFromResourceS(UINT nResID, BOOL bRegister,
	struct _ATL_REGMAP_ENTRY* pMapEntries /*= NULL*/) throw()
{
	CRegObject ro;

	if (pMapEntries != NULL)
	{
		while (pMapEntries->szKey != NULL)
		{
			ATLASSERT(NULL != pMapEntries->szData);
			ro.AddReplacement(pMapEntries->szKey, pMapEntries->szData);
			pMapEntries++;
		}
	}

	HRESULT hr = AddCommonRGSReplacements(&ro);
	if (FAILED(hr))
		return hr;

	USES_CONVERSION_EX;
	TCHAR szModule[MAX_PATH];
	HINSTANCE hInst = _AtlBaseModule.GetModuleInstance();
	DWORD dwFLen = GetModuleFileName(hInst, szModule, MAX_PATH);
	if( dwFLen == 0 )
		return AtlHresultFromLastError();
	else if( dwFLen == MAX_PATH )
		return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

	LPOLESTR pszModule;
	pszModule = T2OLE_EX(szModule, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
	if(pszModule == NULL) 
		return E_OUTOFMEMORY;
#endif

	OLECHAR pszModuleUnquoted[_MAX_PATH * 2];
	EscapeSingleQuote(pszModuleUnquoted, pszModule);
	
	HRESULT hRes;
	if ((hInst == NULL) || (hInst == GetModuleHandle(NULL))) // register as EXE
	{
		// If Registering as an EXE, then we quote the resultant path.
		// We don't do it for a DLL, because LoadLibrary fails if the path is
		// quoted
		OLECHAR pszModuleQuote[(_MAX_PATH + _ATL_QUOTES_SPACE)*2]; 		
		pszModuleQuote[0] = OLESTR('\"');
		ocscpy(pszModuleQuote + 1, pszModuleUnquoted);
		int nLen = ocslen(pszModuleQuote);
		pszModuleQuote[nLen] = OLESTR('\"');
		pszModuleQuote[nLen + 1] = 0;

		hRes = ro.AddReplacement(OLESTR("Module"), pszModuleQuote);
	}
	else
	{
		hRes = ro.AddReplacement(OLESTR("Module"), pszModuleUnquoted);
	}	

	if(FAILED(hRes))
		return hRes;
		
	hRes = ro.AddReplacement(OLESTR("Module_Raw"), pszModuleUnquoted);
	if(FAILED(hRes))
		return hRes;

	LPCOLESTR szType = OLESTR("REGISTRY");
	hr = (bRegister) ? ro.ResourceRegister(pszModule, nResID, szType) :
		ro.ResourceUnregister(pszModule, nResID, szType);
	return hr;
}
#endif //_ATL_STATIC_REGISTRY

#ifndef _ATL_NO_COMMODULE

#pragma warning( push )
#pragma warning( disable: 4996 )  // Disable "deprecated symbol" warning

inline HRESULT WINAPI CComModule::UpdateRegistryClass(const CLSID& clsid, LPCTSTR lpszProgID,
	LPCTSTR lpszVerIndProgID, UINT nDescID, DWORD dwFlags, BOOL bRegister)
{
	if (bRegister)
	{
		TCHAR szDesc[256];
		LoadString(m_hInst, nDescID, szDesc, 256);
		return RegisterClassHelper(clsid, lpszProgID, lpszVerIndProgID, szDesc, dwFlags);
	}
	return UnregisterClassHelper(clsid, lpszProgID, lpszVerIndProgID);
}

inline HRESULT WINAPI CComModule::UpdateRegistryClass(const CLSID& clsid, LPCTSTR lpszProgID,
	LPCTSTR lpszVerIndProgID, LPCTSTR szDesc, DWORD dwFlags, BOOL bRegister)
{
	if (bRegister)
		return RegisterClassHelper(clsid, lpszProgID, lpszVerIndProgID, szDesc, dwFlags);
	return UnregisterClassHelper(clsid, lpszProgID, lpszVerIndProgID);
}

inline HRESULT WINAPI CComModule::RegisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
	LPCTSTR lpszVerIndProgID, LPCTSTR szDesc, DWORD dwFlags)
{
	static const TCHAR szProgID[] = _T("ProgID");
	static const TCHAR szVIProgID[] = _T("VersionIndependentProgID");
	static const TCHAR szLS32[] = _T("LocalServer32");
	static const TCHAR szIPS32[] = _T("InprocServer32");
	static const TCHAR szThreadingModel[] = _T("ThreadingModel");
	static const TCHAR szAUTPRX32[] = _T("AUTPRX32.DLL");
	static const TCHAR szApartment[] = _T("Apartment");
	static const TCHAR szBoth[] = _T("both");
	USES_CONVERSION_EX;
	TCHAR szModule[_MAX_PATH + _ATL_QUOTES_SPACE];

	// If the ModuleFileName's length is equal or greater than the 3rd parameter
	// (length of the buffer passed),GetModuleFileName fills the buffer (truncates
	// if neccessary), but doesn't null terminate it. It returns the same value as 
	// the 3rd parameter passed. So if the return value is the same as the 3rd param
	// then you have a non null terminated buffer (which may or may not be truncated)
	// We pass (szModule + 1) because in case it's an EXE we need to quote the PATH
	// The quote is done later in this method before the SetKeyValue is called
	DWORD dwLen = GetModuleFileName(m_hInst, szModule + 1, MAX_PATH);
	if (dwLen == 0)
	{
		return AtlHresultFromLastError();
	}
	else if( dwLen == MAX_PATH )
	{
		return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
	}

	LPOLESTR lpOleStr;
	HRESULT hRes = StringFromCLSID(clsid, &lpOleStr);
	if (FAILED(hRes))
		return hRes;

	LPTSTR lpsz = OLE2T_EX(lpOleStr, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
	if(lpsz == NULL)
	{
		CoTaskMemFree(lpOleStr);
		return E_OUTOFMEMORY;
	}
#endif

	hRes = RegisterProgID(lpsz, lpszProgID, szDesc);
	if (hRes == S_OK)
		hRes = RegisterProgID(lpsz, lpszVerIndProgID, szDesc);
	LONG lRes = ERROR_SUCCESS;
	if (hRes == S_OK)
	{
		CRegKey key;
		lRes = key.Open(HKEY_CLASSES_ROOT, _T("CLSID"), KEY_READ | KEY_WRITE);
		if (lRes == ERROR_SUCCESS)
		{
			lRes = key.Create(key, lpsz, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE);
			if (lRes == ERROR_SUCCESS)
			{
				lRes = key.SetStringValue(NULL, szDesc);
				if (lRes == ERROR_SUCCESS)
				{
					lRes = key.SetKeyValue(szProgID, lpszProgID);
					if (lRes == ERROR_SUCCESS)
					{
						lRes = key.SetKeyValue(szVIProgID, lpszVerIndProgID);
						if (lRes == ERROR_SUCCESS)
						{
							if ((m_hInst == NULL) || (m_hInst == GetModuleHandle(NULL))) // register as EXE
							{
								// If Registering as an EXE, then we quote the resultant path.
								// We don't do it for a DLL, because LoadLibrary fails if the path is
								// quoted
								szModule[0] = _T('\"');
								szModule[dwLen + 1] = _T('\"');
								szModule[dwLen + 2] = 0;

								lRes = key.SetKeyValue(szLS32, szModule);
							}
							else
							{
								lRes = key.SetKeyValue(szIPS32, (dwFlags & AUTPRXFLAG) ? szAUTPRX32 : szModule + 1);
								if (lRes == ERROR_SUCCESS)
								{
									LPCTSTR lpszModel = (dwFlags & THREADFLAGS_BOTH) ? szBoth :
										(dwFlags & THREADFLAGS_APARTMENT) ? szApartment : NULL;
									if (lpszModel != NULL)
										lRes = key.SetKeyValue(szIPS32, lpszModel, szThreadingModel);
								}
							}
						}
					}
				}
			}
		}
	}
	CoTaskMemFree(lpOleStr);
	if (lRes != ERROR_SUCCESS)
		hRes = AtlHresultFromWin32(lRes);
	return hRes;
}

inline HRESULT WINAPI CComModule::UnregisterClassHelper(const CLSID& clsid, LPCTSTR lpszProgID,
	LPCTSTR lpszVerIndProgID)
{
	USES_CONVERSION_EX;
	CRegKey key;
	LONG lRet;

	key.Attach(HKEY_CLASSES_ROOT);
	if (lpszProgID != NULL && lstrcmpi(lpszProgID, _T("")))
	{
		lRet = key.RecurseDeleteKey(lpszProgID);
		if (lRet != ERROR_SUCCESS && lRet != ERROR_FILE_NOT_FOUND && lRet != ERROR_PATH_NOT_FOUND)
		{
			ATLTRACE(atlTraceCOM, 0, _T("Failed to Unregister ProgID : %s\n"), lpszProgID);
			key.Detach();
			return AtlHresultFromWin32(lRet);
		}
	}
	if (lpszVerIndProgID != NULL && lstrcmpi(lpszVerIndProgID, _T("")))
	{
		lRet = key.RecurseDeleteKey(lpszVerIndProgID);
		if (lRet != ERROR_SUCCESS && lRet != ERROR_FILE_NOT_FOUND && lRet != ERROR_PATH_NOT_FOUND)
		{
			ATLTRACE(atlTraceCOM, 0, _T("Failed to Unregister Version Independent ProgID : %s\n"), lpszVerIndProgID);
			key.Detach();
			return AtlHresultFromWin32(lRet);
		}
	}
	LPOLESTR lpOleStr;
	HRESULT hr = StringFromCLSID(clsid, &lpOleStr);
	if (SUCCEEDED(hr))
	{
		LPTSTR lpsz = OLE2T_EX(lpOleStr, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
		if(lpsz == NULL)
		{
			CoTaskMemFree(lpOleStr);
			return E_OUTOFMEMORY;
		}
#endif	

		lRet = key.Open(key, _T("CLSID"), KEY_READ | KEY_WRITE);
		if (lRet == ERROR_SUCCESS)
			lRet = key.RecurseDeleteKey(lpsz);
		if (lRet != ERROR_SUCCESS && lRet != ERROR_FILE_NOT_FOUND && lRet != ERROR_PATH_NOT_FOUND)
		{
			ATLTRACE(atlTraceCOM, 0, _T("Failed to delete CLSID : %s\n"), lpsz);
			hr = AtlHresultFromWin32(lRet);
		}
		CoTaskMemFree(lpOleStr);
	}
	else
	{
		ATLTRACE(atlTraceCOM, 0, _T("Failed to delete CLSID : {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
			clsid.Data1, 
			clsid.Data2, 
			clsid.Data3, 
			clsid.Data4[0],
			clsid.Data4[1],
			clsid.Data4[2],
			clsid.Data4[3],
			clsid.Data4[4],
			clsid.Data4[5],
			clsid.Data4[6],
			clsid.Data4[7]
			);
	}
	key.Detach();
	return hr;
}

#pragma warning( pop ) // Disable "deprecated symbol" warning

#endif	// !_ATL_NO_COMMODULE

#ifdef _ATL_DEBUG_INTERFACES

inline void _QIThunk::Dump() throw()
{
	TCHAR buf[512];
	if (m_dwRef != 0)
	{
		wsprintf(buf, _T("ATL: QIThunk - %-10d\tLEAK    :\tObject = 0x%08x\tRefcount = %d\tMaxRefCount = %d\t"), m_nIndex, m_pUnk, m_dwRef, m_dwMaxRef);
		OutputDebugString(buf);
		AtlDumpIID(m_iid, m_lpszClassName, S_OK);
	}
	else
	{
		wsprintf(buf, _T("ATL: QIThunk - %-10d\tNonAddRef LEAK :\tObject = 0x%08x\t"), m_nIndex, m_pUnk);
		OutputDebugString(buf);
		AtlDumpIID(m_iid, m_lpszClassName, S_OK);
	}
}

#endif	// _ATL_DEBUG_INTERFACES

#if defined(_ATL_DEBUG_INTERFACES) || defined(_ATL_DEBUG_QI)
__forceinline HRESULT WINAPI AtlDumpIID(REFIID iid, LPCTSTR pszClassName, HRESULT hr) throw()
{
	USES_CONVERSION_EX;
	CRegKey key;
	TCHAR szName[100];
	DWORD dwType;
	DWORD dw = sizeof(szName);

	LPOLESTR pszGUID = NULL;
	if (FAILED(StringFromCLSID(iid, &pszGUID)))
		return hr;

	OutputDebugString(pszClassName);
	OutputDebugString(_T(" - "));

	LPTSTR lpszGUID = OLE2T_EX(pszGUID, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
	if(lpszGUID == NULL)
	{
		CoTaskMemFree(pszGUID);
		return hr;
	}
#endif
	// Attempt to find it in the interfaces section
	if (key.Open(HKEY_CLASSES_ROOT, _T("Interface"), KEY_READ) == ERROR_SUCCESS)
	{
		if (key.Open(key, lpszGUID, KEY_READ) == ERROR_SUCCESS)
		{
			*szName = 0;
			if (RegQueryValueEx(key.m_hKey, (LPTSTR)NULL, NULL, &dwType, (LPBYTE)szName, &dw) == ERROR_SUCCESS)
			{
				OutputDebugString(szName);
			}
		}
	}
	// Attempt to find it in the clsid section
	else if (key.Open(HKEY_CLASSES_ROOT, _T("CLSID"), KEY_READ) == ERROR_SUCCESS)
	{
		if (key.Open(key, lpszGUID, KEY_READ) == ERROR_SUCCESS)
		{
			*szName = 0;
			if (RegQueryValueEx(key.m_hKey, (LPTSTR)NULL, NULL, &dwType, (LPBYTE)szName, &dw) == ERROR_SUCCESS)
			{
				OutputDebugString(_T("(CLSID\?\?\?) "));
				OutputDebugString(szName);
			}
		}
	}
	else
		OutputDebugString(lpszGUID);

	if (hr != S_OK)
		OutputDebugString(_T(" - failed"));
	OutputDebugString(_T("\n"));
	CoTaskMemFree(pszGUID);

	return hr;
}
#endif	// _ATL_DEBUG_INTERFACES || _ATL_DEBUG_QI

#pragma pack(pop)

// WM_FORWARDMSG - used to forward a message to another window for processing
// WPARAM - DWORD dwUserData - defined by user
// LPARAM - LPMSG pMsg - a pointer to the MSG structure
// return value - 0 if the message was not processed, nonzero if it was
#define WM_FORWARDMSG       0x037F

}; //namespace ATL

#ifndef _ATL_NO_AUTOMATIC_NAMESPACE
using namespace ATL;
#endif //!_ATL_NO_AUTOMATIC_NAMESPACE

#ifdef _ATL_ATTRIBUTES
#include <atlplus.h>
#endif

namespace ATL
{

// All exports go here
// Pull in if building ATL DLL or not linking to ATL DLL
#ifndef _ATL_DLL

/////////////////////////////////////////////////////////////////////////////
// statics

static inline UINT WINAPI AtlGetDirLen(LPCOLESTR lpszPathName) throw()
{
	ATLASSERT(lpszPathName != NULL);
	if(lpszPathName == NULL)
		return 0;

	// always capture the complete file name including extension (if present)
	LPCOLESTR lpszTemp = lpszPathName;
	for (LPCOLESTR lpsz = lpszPathName; *lpsz != NULL; )
	{
		LPCOLESTR lp = CharNextO(lpsz);
		// remember last directory/drive separator
		if (*lpsz == OLESTR('\\') || *lpsz == OLESTR('/') || *lpsz == OLESTR(':'))
			lpszTemp = lp;
		lpsz = lp;
	}

	return UINT( lpszTemp-lpszPathName );
}

/////////////////////////////////////////////////////////////////////////////
// TypeLib registration

#define _ATL_MAX_PATH_PLUS_INDEX (_MAX_PATH + _ATL_TYPELIB_INDEX_LENGTH)

ATLINLINE ATLAPI AtlLoadTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex, BSTR* pbstrPath, ITypeLib** ppTypeLib)
{
	ATLASSERT(pbstrPath != NULL && ppTypeLib != NULL);
	if (pbstrPath == NULL || ppTypeLib == NULL)
		return E_POINTER;

	*pbstrPath = NULL;
	*ppTypeLib = NULL;

	USES_CONVERSION_EX;
	ATLASSERT(hInstTypeLib != NULL);
	TCHAR szModule[_ATL_MAX_PATH_PLUS_INDEX];

	DWORD dwFLen = GetModuleFileName(hInstTypeLib, szModule, MAX_PATH);
	if( dwFLen == 0 )
		return AtlHresultFromLastError();
	else if( dwFLen == MAX_PATH )
		return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

	// get the extension pointer in case of fail
	LPTSTR lpszExt = NULL;

	lpszExt = PathFindExtension(szModule);

	if (lpszIndex != NULL)
	{
		LPCTSTR lpcszIndex = OLE2CT_EX(lpszIndex, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
		if(lpcszIndex == NULL) 
			return E_OUTOFMEMORY;
#endif		
		int nIndexLen = lstrlen(lpcszIndex);
		
		if( dwFLen + nIndexLen >= _ATL_MAX_PATH_PLUS_INDEX )
			return E_FAIL;
		lstrcpy(szModule + dwFLen,lpcszIndex);
	}
	LPOLESTR lpszModule = T2OLE_EX(szModule, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
	if(lpszModule == NULL) 
		return E_OUTOFMEMORY;
#endif	
	HRESULT hr = LoadTypeLib(lpszModule, ppTypeLib);
	if (!SUCCEEDED(hr))
	{
		// typelib not in module, try <module>.tlb instead
		TCHAR szExt[] = _T(".tlb");
		if ((lpszExt - szModule + sizeof(szExt)/sizeof(TCHAR)) > _MAX_PATH)
			return E_FAIL;
			
		lstrcpy(lpszExt, szExt);
		lpszModule = T2OLE_EX(szModule, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
#ifndef _UNICODE
		if(lpszModule == NULL) 
			return E_OUTOFMEMORY;
#endif		
		hr = LoadTypeLib(lpszModule, ppTypeLib);
	}
	if (SUCCEEDED(hr))
	{
		*pbstrPath = ::SysAllocString(lpszModule);
		if (*pbstrPath == NULL)
			hr = E_OUTOFMEMORY;
	}
	return hr;
}

ATLINLINE ATLAPI AtlUnRegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex)
{
	CComBSTR bstrPath;
	CComPtr<ITypeLib> pTypeLib;
	HRESULT hr = AtlLoadTypeLib(hInstTypeLib, lpszIndex, &bstrPath, &pTypeLib);
	if (SUCCEEDED(hr))
	{
		TLIBATTR* ptla;
		hr = pTypeLib->GetLibAttr(&ptla);
		if (SUCCEEDED(hr))
		{
			hr = UnRegisterTypeLib(ptla->guid, ptla->wMajorVerNum, ptla->wMinorVerNum, ptla->lcid, ptla->syskind);
			pTypeLib->ReleaseTLibAttr(ptla);
		}
	}
	return hr;
}

ATLINLINE ATLAPI AtlRegisterTypeLib(HINSTANCE hInstTypeLib, LPCOLESTR lpszIndex)
{
	CComBSTR bstrPath;
	CComPtr<ITypeLib> pTypeLib;
	HRESULT hr = AtlLoadTypeLib(hInstTypeLib, lpszIndex, &bstrPath, &pTypeLib);
	if (SUCCEEDED(hr))
	{
		OLECHAR szDir[MAX_PATH];
		wcsncpy(szDir,bstrPath,MAX_PATH - 1);
		szDir[MAX_PATH - 1] = 0;
		// If index is specified remove it from the path
		if (lpszIndex != NULL)
		{
			size_t nLenPath = ocslen(szDir);
			size_t nLenIndex = ocslen(lpszIndex);
			if (memcmp(szDir + nLenPath - nLenIndex, lpszIndex, nLenIndex) == 0)
				szDir[nLenPath - nLenIndex] = 0;
		}
		szDir[AtlGetDirLen(szDir)] = 0;
		hr = ::RegisterTypeLib(pTypeLib, bstrPath, szDir);
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Registration

// AtlComModuleRegisterServer walks the ATL Autogenerated Object Map and registers each object in the map
// If pCLSID is not NULL then only the object referred to by pCLSID is registered (The default case)
// otherwise all the objects are registered
ATLINLINE ATLAPI AtlComModuleRegisterServer(_ATL_COM_MODULE* pComModule, BOOL bRegTypeLib, const CLSID* pCLSID)
{
	ATLASSERT(pComModule != NULL);
	if (pComModule == NULL)
		return E_INVALIDARG;
	ATLASSERT(pComModule->m_hInstTypeLib != NULL);

	HRESULT hr = S_OK;

	for (_ATL_OBJMAP_ENTRY** ppEntry = pComModule->m_ppAutoObjMapFirst; ppEntry < pComModule->m_ppAutoObjMapLast; ppEntry++)
	{
		if (*ppEntry != NULL)
		{
			_ATL_OBJMAP_ENTRY* pEntry = *ppEntry;
			if (pCLSID != NULL)
			{
				if (!IsEqualGUID(*pCLSID, *pEntry->pclsid))
					continue;
			}
			hr = pEntry->pfnUpdateRegistry(TRUE);
			if (FAILED(hr))
				break;
			hr = AtlRegisterClassCategoriesHelper( *pEntry->pclsid,
				pEntry->pfnGetCategoryMap(), TRUE );
			if (FAILED(hr))
				break;
		}
	}

	if (SUCCEEDED(hr) && bRegTypeLib)
		hr = AtlRegisterTypeLib(pComModule->m_hInstTypeLib, 0);

	return hr;
}

// AtlComUnregisterServer walks the ATL Object Map and unregisters each object in the map
// If pCLSID is not NULL then only the object referred to by pCLSID is unregistered (The default case)
// otherwise all the objects are unregistered.
ATLINLINE ATLAPI AtlComModuleUnregisterServer(_ATL_COM_MODULE* pComModule, BOOL bUnRegTypeLib, const CLSID* pCLSID)
{
	ATLASSERT(pComModule != NULL);
	if (pComModule == NULL)
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	for (_ATL_OBJMAP_ENTRY** ppEntry = pComModule->m_ppAutoObjMapFirst; ppEntry < pComModule->m_ppAutoObjMapLast; ppEntry++)
	{
		if (*ppEntry != NULL)
		{
			_ATL_OBJMAP_ENTRY* pEntry = *ppEntry;
			if (pCLSID != NULL)
			{
				if (!IsEqualGUID(*pCLSID, *pEntry->pclsid))
					continue;
			}
			hr = AtlRegisterClassCategoriesHelper( *pEntry->pclsid, pEntry->pfnGetCategoryMap(), FALSE );
			if (FAILED(hr))
				break;
			hr = pEntry->pfnUpdateRegistry(FALSE); //unregister
			if (FAILED(hr))
				break;
		}
	}
	if (SUCCEEDED(hr) && bUnRegTypeLib)
		hr = AtlUnRegisterTypeLib(pComModule->m_hInstTypeLib, 0);

	return hr;
}

ATLINLINE ATLAPI AtlRegisterClassCategoriesHelper( REFCLSID clsid,
   const struct _ATL_CATMAP_ENTRY* pCatMap, BOOL bRegister )
{
   CComPtr< ICatRegister > pCatRegister;
   HRESULT hResult;
   const struct _ATL_CATMAP_ENTRY* pEntry;
   CATID catid;

   if( pCatMap == NULL )
   {
	  return( S_OK );
   }

   if (InlineIsEqualGUID(clsid, GUID_NULL))
   {
	  ATLASSERT(0 && _T("Use OBJECT_ENTRY_NON_CREATEABLE_EX macro if you want to register class categories for non creatable objects."));
	  return S_OK;
   }

   hResult = CoCreateInstance( CLSID_StdComponentCategoriesMgr, NULL,
	  CLSCTX_INPROC_SERVER, __uuidof(ICatRegister), (void**)&pCatRegister );
   if( FAILED( hResult ) )
   {
	  // Since not all systems have the category manager installed, we'll allow
	  // the registration to succeed even though we didn't register our
	  // categories.  If you really want to register categories on a system
	  // without the category manager, you can either manually add the
	  // appropriate entries to your registry script (.rgs), or you can
	  // redistribute comcat.dll.
	  return( S_OK );
   }

   hResult = S_OK;
   pEntry = pCatMap;
   while( pEntry->iType != _ATL_CATMAP_ENTRY_END )
   {
	  catid = *pEntry->pcatid;
	  if( bRegister )
	  {
		 if( pEntry->iType == _ATL_CATMAP_ENTRY_IMPLEMENTED )
		 {
			hResult = pCatRegister->RegisterClassImplCategories( clsid, 1,
			   &catid );
		 }
		 else
		 {
			ATLASSERT( pEntry->iType == _ATL_CATMAP_ENTRY_REQUIRED );
			hResult = pCatRegister->RegisterClassReqCategories( clsid, 1,
			   &catid );
		 }
		 if( FAILED( hResult ) )
		 {
			return( hResult );
		 }
	  }
	  else
	  {
		 if( pEntry->iType == _ATL_CATMAP_ENTRY_IMPLEMENTED )
		 {
			pCatRegister->UnRegisterClassImplCategories( clsid, 1, &catid );
		 }
		 else
		 {
			ATLASSERT( pEntry->iType == _ATL_CATMAP_ENTRY_REQUIRED );
			pCatRegister->UnRegisterClassReqCategories( clsid, 1, &catid );
		 }
	  }
	  pEntry++;
   }

   // When unregistering remove "Implemented Categories" and "Required Categories" subkeys if they are empty.
   if (!bRegister)
   {
		OLECHAR szGUID[64];
		::StringFromGUID2(clsid, szGUID, 64);
		USES_CONVERSION_EX;
		TCHAR* pszGUID = OLE2T_EX(szGUID, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
		if (pszGUID != NULL)
		{
			TCHAR szKey[128];
			lstrcpy(szKey, _T("CLSID\\"));
			lstrcat(szKey, pszGUID);
			lstrcat(szKey, _T("\\Required Categories"));

			CRegKey root(HKEY_CLASSES_ROOT);
			CRegKey key;
			DWORD cbSubKeys = 0;

			LRESULT lRes = key.Open(root, szKey, KEY_READ);
			if (lRes == ERROR_SUCCESS)
			{
				lRes = RegQueryInfoKey(key, NULL, NULL, NULL, &cbSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
				key.Close();
				if (lRes == ERROR_SUCCESS && cbSubKeys == 0)
				{
					root.DeleteSubKey(szKey);
				}
			}

			lstrcpy(szKey, _T("CLSID\\"));
			lstrcat(szKey, pszGUID);
			lstrcat(szKey, _T("\\Implemented Categories"));
			lRes = key.Open(root, szKey, KEY_READ);
			if (lRes == ERROR_SUCCESS)
			{
				lRes = RegQueryInfoKey(key, NULL, NULL, NULL, &cbSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
				key.Close();
				if (lRes == ERROR_SUCCESS && cbSubKeys == 0)
				{
					root.DeleteSubKey(szKey);
				}
			}
		}
   }
   return( S_OK );
}

ATLINLINE ATLAPI AtlWinModuleTerm(_ATL_WIN_MODULE* pWinModule, HINSTANCE hInst)
{
	if (pWinModule == NULL)
		return E_INVALIDARG;
	if (pWinModule->cbSize == 0)
		return S_OK;
	if (pWinModule->cbSize != sizeof(_ATL_WIN_MODULE))
		return E_INVALIDARG;

	for (int i = 0; i < pWinModule->m_rgWindowClassAtoms.GetSize(); i++)
		UnregisterClass((LPCTSTR)pWinModule->m_rgWindowClassAtoms[i], hInst);
	pWinModule->m_rgWindowClassAtoms.RemoveAll();
	pWinModule->m_csWindowCreate.Term();
	pWinModule->cbSize = 0;
	return S_OK;
}

#endif // _ATL_DLL

/////////////////////////////////////////////////////////////////////////////
// General DLL Version Helpers

#pragma warning(push)
#pragma warning(disable : 4191)	// 'type cast' : unsafe conversion from 'FARPROC' to 'DLLGETVERSIONPROC'

inline HRESULT AtlGetDllVersion(HINSTANCE hInstDLL, DLLVERSIONINFO* pDllVersionInfo)
{
	ATLASSERT(!::IsBadWritePtr(pDllVersionInfo, sizeof(DLLVERSIONINFO)));

	// We must get this function explicitly because some DLLs don't implement it.
	DLLGETVERSIONPROC pfnDllGetVersion = (DLLGETVERSIONPROC)::GetProcAddress(hInstDLL, "DllGetVersion");
	if(pfnDllGetVersion == NULL)
		return E_NOTIMPL;

	return (*pfnDllGetVersion)(pDllVersionInfo);
}

#pragma warning(pop)

inline HRESULT AtlGetDllVersion(LPCTSTR lpstrDllName, DLLVERSIONINFO* pDllVersionInfo)
{
	HINSTANCE hInstDLL = ::LoadLibrary(lpstrDllName);
	if(hInstDLL == NULL)
		return AtlHresultFromLastError();
	HRESULT hRet = AtlGetDllVersion(hInstDLL, pDllVersionInfo);
	::FreeLibrary(hInstDLL);
	return hRet;
}

// Common Control Versions:
//   Win95/WinNT 4.0    maj=4 min=00
//   IE 3.x             maj=4 min=70
//   IE 4.0             maj=4 min=71
//   IE 5.0             maj=5 min=80
//   Win2000            maj=5 min=81
inline HRESULT AtlGetCommCtrlVersion(LPDWORD pdwMajor, LPDWORD pdwMinor)
{
	ATLASSERT(!::IsBadWritePtr(pdwMajor, sizeof(DWORD)) && !::IsBadWritePtr(pdwMinor, sizeof(DWORD)));

	DLLVERSIONINFO dvi;
	memset(&dvi, 0, sizeof(dvi));
	dvi.cbSize = sizeof(dvi);

	HRESULT hRet = AtlGetDllVersion(_T("comctl32.dll"), &dvi);

	if(SUCCEEDED(hRet))
	{
		*pdwMajor = dvi.dwMajorVersion;
		*pdwMinor = dvi.dwMinorVersion;
	}
	else if(hRet == E_NOTIMPL)
	{
		// If DllGetVersion is not there, then the DLL is a version
		// previous to the one shipped with IE 3.x
		*pdwMajor = 4;
		*pdwMinor = 0;
		hRet = S_OK;
	}

	return hRet;
}

// Shell Versions:
//   Win95/WinNT 4.0                                maj=4 min=00
//   IE 3.x, IE 4.0 without Web Integrated Desktop  maj=4 min=00
//   IE 4.0 with Web Integrated Desktop             maj=4 min=71
//   IE 4.01 with Web Integrated Desktop            maj=4 min=72
//   Win2000                                        maj=5 min=00
inline HRESULT AtlGetShellVersion(LPDWORD pdwMajor, LPDWORD pdwMinor)
{
	ATLASSERT(!::IsBadWritePtr(pdwMajor, sizeof(DWORD)) && !::IsBadWritePtr(pdwMinor, sizeof(DWORD)));

	DLLVERSIONINFO dvi;
	memset(&dvi, 0, sizeof(dvi));
	dvi.cbSize = sizeof(dvi);
	HRESULT hRet = AtlGetDllVersion(_T("shell32.dll"), &dvi);

	if(SUCCEEDED(hRet))
	{
		*pdwMajor = dvi.dwMajorVersion;
		*pdwMinor = dvi.dwMinorVersion;
	}
	else if(hRet == E_NOTIMPL)
	{
		// If DllGetVersion is not there, then the DLL is a version
		// previous to the one shipped with IE 4.x
		*pdwMajor = 4;
		*pdwMinor = 0;
		hRet = S_OK;
	}

	return hRet;
}

inline ATL_DEPRECATED HRESULT AtlModuleRegisterClassObjects(_ATL_MODULE* /*pM*/, DWORD dwClsContext, DWORD dwFlags)
{
	return AtlComModuleRegisterClassObjects(&_AtlComModule, dwClsContext, dwFlags);
}

inline ATL_DEPRECATED HRESULT AtlModuleRevokeClassObjects(_ATL_MODULE* /*pM*/)
{
	return AtlComModuleRevokeClassObjects(&_AtlComModule);
}

inline ATL_DEPRECATED HRESULT AtlModuleGetClassObject(_ATL_MODULE* /*pM*/, REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifndef _ATL_OLEDB_CONFORMANCE_TESTS
	ATLASSERT(ppv != NULL);
#endif

	return AtlComModuleGetClassObject(&_AtlComModule, rclsid, riid, ppv);
}

inline ATL_DEPRECATED HRESULT AtlModuleRegisterServer(_ATL_MODULE* /*pM*/, BOOL bRegTypeLib, const CLSID* pCLSID = NULL)
{
	return AtlComModuleRegisterServer(&_AtlComModule, bRegTypeLib, pCLSID);
}

inline ATL_DEPRECATED HRESULT AtlModuleUnregisterServer(_ATL_MODULE* /*pM*/, const CLSID* pCLSID = NULL)
{
	return AtlComModuleUnregisterServer(&_AtlComModule, FALSE, pCLSID);
}

inline ATL_DEPRECATED HRESULT AtlModuleUnregisterServerEx(_ATL_MODULE* /*pM*/, BOOL bUnRegTypeLib, const CLSID* pCLSID = NULL)
{
	return AtlComModuleUnregisterServer(&_AtlComModule, bUnRegTypeLib, pCLSID);
}

inline ATL_DEPRECATED HRESULT AtlModuleUpdateRegistryFromResourceD(_ATL_MODULE* /*pM*/, LPCOLESTR lpszRes,
	BOOL bRegister, struct _ATL_REGMAP_ENTRY* pMapEntries, IRegistrar* pReg = NULL)
{
	return AtlUpdateRegistryFromResourceD(_AtlBaseModule.GetModuleInstance(), lpszRes, bRegister, pMapEntries, pReg);
}

inline ATL_DEPRECATED HRESULT AtlModuleRegisterTypeLib(_ATL_MODULE* /*pM*/, LPCOLESTR lpszIndex)
{
	return AtlRegisterTypeLib(_AtlComModule.m_hInstTypeLib, lpszIndex);
}

inline ATL_DEPRECATED HRESULT AtlModuleUnRegisterTypeLib(_ATL_MODULE* /*pM*/, LPCOLESTR lpszIndex)
{
	return AtlUnRegisterTypeLib(_AtlComModule.m_hInstTypeLib, lpszIndex);
}

inline ATL_DEPRECATED HRESULT AtlModuleLoadTypeLib(_ATL_MODULE* /*pM*/, LPCOLESTR lpszIndex, BSTR* pbstrPath, ITypeLib** ppTypeLib)
{
	return AtlLoadTypeLib(_AtlComModule.m_hInstTypeLib, lpszIndex, pbstrPath, ppTypeLib);
}

inline ATL_DEPRECATED HRESULT AtlModuleInit(_ATL_MODULE* /*pM*/, _ATL_OBJMAP_ENTRY* /*p*/, HINSTANCE /*h*/)
{
	return S_OK;
}

inline ATL_DEPRECATED HRESULT AtlModuleTerm(_ATL_MODULE* /*pM*/)
{
	return S_OK;
}

inline ATL_DEPRECATED void AtlModuleAddCreateWndData(_ATL_MODULE* /*pM*/, _AtlCreateWndData* pData, void* pObject)
{
	AtlWinModuleAddCreateWndData(&_AtlWinModule, pData, pObject);
}

inline ATL_DEPRECATED void* AtlModuleExtractCreateWndData(_ATL_MODULE* /*pM*/)
{
	return AtlWinModuleExtractCreateWndData(&_AtlWinModule);
}

#ifndef _ATL_NO_COMMODULE

inline CRITICAL_SECTION& CComModule::get_m_csWindowCreate() throw()
{
	return _AtlWinModule.m_csWindowCreate.m_sec;
}

inline CRITICAL_SECTION& CComModule::get_m_csObjMap() throw()
{
	return _AtlComModule.m_csObjMap.m_sec;
}

inline CRITICAL_SECTION& CComModule::get_m_csStaticDataInit() throw()
{
	return m_csStaticDataInitAndTypeInfo.m_sec;
}

inline _AtlCreateWndData*& CComModule::get_m_pCreateWndList()  throw()
{
	return _AtlWinModule.m_pCreateWndList;
}
inline void CComModule::put_m_pCreateWndList(_AtlCreateWndData* p) throw()
{
	_AtlWinModule.m_pCreateWndList = p;
}
#ifdef _ATL_DEBUG_INTERFACES
inline UINT& CComModule::get_m_nIndexQI() throw()
{
	return _AtlDebugInterfacesModule.m_nIndexQI;
}
inline void CComModule::put_m_nIndexQI(UINT nIndex) throw()
{
	_AtlDebugInterfacesModule.m_nIndexQI = nIndex;
}
inline UINT& CComModule::get_m_nIndexBreakAt() throw()
{
	return _AtlDebugInterfacesModule.m_nIndexBreakAt;
}
inline void CComModule::put_m_nIndexBreakAt(UINT nIndex) throw()
{
	_AtlDebugInterfacesModule.m_nIndexBreakAt = nIndex;
}
inline CSimpleArray<_QIThunk*>* CComModule::get_m_paThunks() throw()
{
	return &_AtlDebugInterfacesModule.m_aThunks;
}
inline HRESULT CComModule::AddThunk(IUnknown** pp, LPCTSTR lpsz, REFIID iid) throw()
{
	return _AtlDebugInterfacesModule.AddThunk(pp, lpsz, iid);
}
inline HRESULT CComModule::AddNonAddRefThunk(IUnknown* p, LPCTSTR lpsz, IUnknown** ppThunkRet) throw()
{
	return _AtlDebugInterfacesModule.AddNonAddRefThunk(p, lpsz, ppThunkRet);
}

inline void CComModule::DeleteNonAddRefThunk(IUnknown* pUnk) throw()
{
	_AtlDebugInterfacesModule.DeleteNonAddRefThunk(pUnk);
}

inline void CComModule::DeleteThunk(_QIThunk* p) throw()
{
	_AtlDebugInterfacesModule.DeleteThunk(p);
}

inline bool CComModule::DumpLeakedThunks() throw()
{
	return _AtlDebugInterfacesModule.DumpLeakedThunks();
}
#endif // _ATL_DEBUG_INTERFACES

inline HRESULT CComModule::Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE /*h*/, const GUID* plibid) throw()
{
	if (plibid != NULL)
		m_libid = *plibid;

	_ATL_OBJMAP_ENTRY* pEntry;
	if (p != (_ATL_OBJMAP_ENTRY*)-1)
	{
		m_pObjMap = p;
		if (m_pObjMap != NULL)
		{
			pEntry = m_pObjMap;
			while (pEntry->pclsid != NULL)
			{
				pEntry->pfnObjectMain(true); //initialize class resources
				pEntry++;
			}
		}
	}
	for (_ATL_OBJMAP_ENTRY** ppEntry = _AtlComModule.m_ppAutoObjMapFirst; ppEntry < _AtlComModule.m_ppAutoObjMapLast; ppEntry++)
	{
		if (*ppEntry != NULL)
			(*ppEntry)->pfnObjectMain(true); //initialize class resources
	}
	return S_OK;
}
inline void CComModule::Term() throw()
{
	_ATL_OBJMAP_ENTRY* pEntry;
	if (m_pObjMap != NULL)
	{
		pEntry = m_pObjMap;
		while (pEntry->pclsid != NULL)
		{
			if (pEntry->pCF != NULL)
				pEntry->pCF->Release();
			pEntry->pCF = NULL;
			pEntry->pfnObjectMain(false); //cleanup class resources
			pEntry++;
		}
	}
	for (_ATL_OBJMAP_ENTRY** ppEntry = _AtlComModule.m_ppAutoObjMapFirst; ppEntry < _AtlComModule.m_ppAutoObjMapLast; ppEntry++)
	{
		if (*ppEntry != NULL)
			(*ppEntry)->pfnObjectMain(false); //cleanup class resources
	}
#ifdef _DEBUG
	// Prevent false memory leak reporting. ~CAtlWinModule may be too late.
	_AtlWinModule.Term();		
#endif	// _DEBUG

	CAtlModuleT<CComModule>::Term();
}

inline HRESULT CComModule::GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) throw()
{
	if (ppv == NULL)
		return E_POINTER;
	HRESULT hr = S_OK;
	_ATL_OBJMAP_ENTRY* pEntry;
	if (m_pObjMap != NULL)
	{
		pEntry = m_pObjMap;
		while (pEntry->pclsid != NULL)
		{
			if ((pEntry->pfnGetClassObject != NULL) && InlineIsEqualGUID(rclsid, *pEntry->pclsid))
			{
				if (pEntry->pCF == NULL)
				{
					CComCritSecLock<CComCriticalSection> lock(_AtlComModule.m_csObjMap, false);
					hr = lock.Lock();
					if (FAILED(hr))
					{
						ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to lock critical section in CComModule::GetClassObject\n"));
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
			pEntry++;
		}
	}
	if (*ppv == NULL && hr == S_OK)
		hr = AtlComModuleGetClassObject(&_AtlComModule, rclsid, riid, ppv);
	return hr;
}

// Register/Revoke All Class Factories with the OS (EXE only)
inline HRESULT CComModule::RegisterClassObjects(DWORD dwClsContext, DWORD dwFlags) throw()
{
	HRESULT hr = S_OK;
	_ATL_OBJMAP_ENTRY* pEntry;
	if (m_pObjMap != NULL)
	{
		pEntry = m_pObjMap;
		while (pEntry->pclsid != NULL && hr == S_OK)
		{
			hr = pEntry->RegisterClassObject(dwClsContext, dwFlags);
			pEntry++;
		}
	}
	if (hr == S_OK)
		hr = AtlComModuleRegisterClassObjects(&_AtlComModule, dwClsContext, dwFlags);
	return hr;
}
inline HRESULT CComModule::RevokeClassObjects() throw()
{
	HRESULT hr = S_OK;
	_ATL_OBJMAP_ENTRY* pEntry;
	if (m_pObjMap != NULL)
	{
		pEntry = m_pObjMap;
		while (pEntry->pclsid != NULL && hr == S_OK)
		{
			hr = pEntry->RevokeClassObject();
			pEntry++;
		}
	}
	if (hr == S_OK)
		hr = AtlComModuleRevokeClassObjects(&_AtlComModule);
	return hr;
}

// Registry support (helpers)
inline HRESULT CComModule::RegisterTypeLib() throw()
{
	return _AtlComModule.RegisterTypeLib();
}
inline HRESULT CComModule::RegisterTypeLib(LPCTSTR lpszIndex) throw()
{
	return _AtlComModule.RegisterTypeLib(lpszIndex);
}
inline HRESULT CComModule::UnRegisterTypeLib() throw()
{
	return _AtlComModule.UnRegisterTypeLib();
}
inline HRESULT CComModule::UnRegisterTypeLib(LPCTSTR lpszIndex) throw()
{
	return _AtlComModule.UnRegisterTypeLib(lpszIndex);
}

inline HRESULT CComModule::RegisterServer(BOOL bRegTypeLib /*= FALSE*/, const CLSID* pCLSID /*= NULL*/) throw()
{
	HRESULT hr = S_OK;
	_ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;
	if (pEntry != NULL)
	{
		for (;pEntry->pclsid != NULL; pEntry++)
		{
			if (pCLSID != NULL)
			{
				if (!IsEqualGUID(*pCLSID, *pEntry->pclsid))
					continue;
			}
			hr = pEntry->pfnUpdateRegistry(TRUE);
			if (FAILED(hr))
				break;
			hr = AtlRegisterClassCategoriesHelper( *pEntry->pclsid,
				pEntry->pfnGetCategoryMap(), TRUE );
			if (FAILED(hr))
				break;
		}
	}
	if (SUCCEEDED(hr))
		hr = CAtlModuleT<CComModule>::RegisterServer(bRegTypeLib, pCLSID);
	return hr;
}

inline HRESULT CComModule::UnregisterServer(BOOL bUnRegTypeLib, const CLSID* pCLSID /*= NULL*/) throw()
{
	HRESULT hr = S_OK;	
	_ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;
	if (pEntry != NULL)
	{
		for (;pEntry->pclsid != NULL; pEntry++)
		{
			if (pCLSID != NULL)
			{
				if (!IsEqualGUID(*pCLSID, *pEntry->pclsid))
					continue;
			}
			hr = AtlRegisterClassCategoriesHelper( *pEntry->pclsid,
				pEntry->pfnGetCategoryMap(), FALSE );
			if (FAILED(hr))
				break;
			hr = pEntry->pfnUpdateRegistry(FALSE); //unregister
			if (FAILED(hr))
				break;
		}
	}
	if (SUCCEEDED(hr))
		hr = CAtlModuleT<CComModule>::UnregisterServer(bUnRegTypeLib, pCLSID);
	return hr;
}

inline HRESULT CComModule::UnregisterServer(const CLSID* pCLSID /*= NULL*/) throw()
{
	return UnregisterServer(FALSE, pCLSID);
}

#endif	// !_ATL_NO_COMMODULE

}	// namespace ATL

#pragma warning( pop )

#if !defined(_ATL_DLL) && !defined(_DEBUG)

#include <atlbase.inl>

#endif	// !_ATL_DLL && !_DEBUG

#ifdef _ATL_ALL_WARNINGS
#pragma warning( pop )
#endif

/////////////////////////////////////////////////////////////////////////////

#endif // __ATLBASE_H__
