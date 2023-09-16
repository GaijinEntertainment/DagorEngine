
// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLDEF_H__
#define __ATLDEF_H__

#pragma once

#pragma warning(disable : 4619)	// there is no warning number

#include <atlrc.h>
#include <errno.h>
#include <sal.h>

#ifndef RC_INVOKED

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifdef UNDER_CE
	#error This version of ATL is not currently supported for CE. Look for the CE specific version.
#endif

// If you are mixing compilation units that are built as
// native code with those that are built /clr, you must define
// the symbol '_ATL_MIXED'. _ATL_MIXED must be defined for all 
// compilation units in an executable or it must be defined for none of them.
#if !defined(_ATL_MIXED)
namespace Inconsistent_definition_of_symbol__ATL_MIXED
{
	struct _Please_define_it_the_same_throughout_your_project { };
}
#else
namespace Inconsistent_definition_of_symbol__ATL_MIXED
{
#ifdef _M_IX86
#pragma comment(linker, "/include:??3@YAXPAX@Z")
#else
#pragma comment(linker, "/include:??3@YAXPEAX@Z")
#endif
	struct _Please_define_it_the_same_throughout_your_project { virtual void one(){} };
}
#endif
namespace Inconsistent_definition_of_symbol__ATL_MIXED
{
	__declspec(selectany) _Please_define_it_the_same_throughout_your_project clash = _Please_define_it_the_same_throughout_your_project ();
}

#if !defined(_ATL_MIXED)
namespace Define_the_symbol__ATL_MIXED
{
#if defined(_M_CEE)
	struct Thank_you { };
#else
#ifdef _M_IX86
#pragma comment(linker, "/include:??3@YAXPAX@Z")
#else
#pragma comment(linker, "/include:??3@YAXPEAX@Z")
#endif
	struct Thank_you { virtual void one(){} };
#endif
	__declspec(selectany) Thank_you clash = Thank_you();
}
#endif

#if defined(_ATL_MIXED)
#define _ATL_NATIVE_INITIALIZATION
#endif

#if !defined(_M_CEE)
#define _ATL_NATIVE_INITIALIZATION
#endif

#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE         // UNICODE is used by Windows headers
#endif
#endif

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE        // _UNICODE is used by C-runtime/MFC headers
#endif
#endif

#ifdef _DEBUG
#ifndef DEBUG
#define DEBUG
#endif
#endif

#ifdef _WIN64
#define _ATL_SUPPORT_VT_I8  // Always support VT_I8 on Win64.
#endif

#if !defined(UNALIGNED)
#if defined(_M_IA64) || defined(_M_AMD64)
#define UNALIGNED __unaligned
#else
#define UNALIGNED
#endif
#endif

#if !defined(_countof)
#if !defined(__cplusplus)
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#else
extern "C++"
{
template <typename _CountofType, size_t _SizeOfArray>
char (*__countof_helper(UNALIGNED _CountofType (&_Array)[_SizeOfArray]))[_SizeOfArray];
#define _countof(_Array) sizeof(*__countof_helper(_Array))
}
#endif
#endif

#ifndef AtlThrow
#ifndef _ATL_CUSTOM_THROW
#define AtlThrow ATL::AtlThrowImpl
#endif
#endif // AtlThrow

#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif // ATLASSERT

/* 
Why does ATLASSUME exist?

ATL 8 has two existing validation models

ATLASSERT/ATLVERIFY - These are used to make sure a debug build reports a problem with the expression/invariant
ATLENSURE - Debug is the same as ATLVERIFY, retail throws a C++ exception

We added ATLENSURE because there were too many unreported error paths in ATL and we wanted to bail out of more
error conditions rather than just trying to continue in retail.

There might be a case for changing 'lots' of ATLASSERT to ATLENSURE, but we chose an incremental approach and only
changed over where we saw a problem with code reported from a customer or test case. This reduces code churn in our
code for this version.

In general, our approach is to try to make sure that when something goes wrong
- the client does not continue to run, because we report an error condition
- debug builds see an assertion about the problem

Sometimes we have code like

HRESULT ComMethod(void)
{
	ATLASSUME(m_pFoo);
	return m_pFoo->Method();
}

We could add
	if(!m_pFoo) return E_POINTER;

But this is very unlikely to help, since it removes the ability of the developer to debug this problem if it's seen in a retail
build of the application. 

We could try something more severe

	if(!m_pFoo) terminate(); // or your favourite shutdown function

This would ensure good reporting (because VC8 terminate generates a Windows Error Report and crash dump), but hardly seems a big win 
over the previous crash.

ATLENSURE might seem slightly better. It is debuggable and consistent with ATL in general. In fact, many parts of ATL do just this. 
But in this specific context, it doesn't look like a great choice. COM methods should not in general be emitting native C++ exceptions 
as an error reporting strategy. 

So we find ourselves in a quandry. For these kinds of methods, the traditional code (ATLASSERT followed by a crash), seems be the most 
debuggable thing to do in this situation. At least for VS8, we have decided to stick with this shape.

---

Now consider the impact of cl /analyze. We want cl /analyze to not warn about our potential dereferences when they refer to member variables
whose state was previously validated by another method. But we do want to see the impact of function contracts on the parameters of the
function. 

So we've done a broad replace of all the member-related ATLASSERT to ATLASSUME. 

*/

#ifndef ATLASSUME
#define ATLASSUME(expr) do { ATLASSERT(expr); __analysis_assume(!!(expr)); } while(0)
#endif // ATLASSERT

#ifndef ATLVERIFY
#ifdef _DEBUG
#define ATLVERIFY(expr) ATLASSERT(expr)
#else
#define ATLVERIFY(expr) (expr)
#endif // DEBUG
#endif // ATLVERIFY

#ifndef ATLENSURE_THROW
#define ATLENSURE_THROW(expr, hr)          \
do {                                       \
	int __atl_condVal=!!(expr);            \
	ATLASSERT(__atl_condVal);              \
	if(!(__atl_condVal)) AtlThrow(hr);     \
} while (0)
#endif // ATLENSURE

#ifndef ATLENSURE
#define ATLENSURE(expr) ATLENSURE_THROW(expr, E_FAIL)
#endif // ATLENSURE

#ifndef ATLENSURE_SUCCEEDED
#define ATLENSURE_SUCCEEDED(hr) ATLENSURE_THROW(SUCCEEDED(hr), hr)
#endif // ATLENSURE

/* Used inside COM methods that do not want to throw */
#ifndef ATLENSURE_RETURN_HR
#define ATLENSURE_RETURN_HR(expr, hr)          \
do {                                           \
	int __atl_condVal=!!(expr);                \
	ATLASSERT(__atl_condVal);                  \
	if(!(__atl_condVal)) return hr;            \
} while (0) 
#endif 

/* Used inside COM methods that do not want to throw */
#ifndef ATLENSURE_RETURN
#define ATLENSURE_RETURN(expr) ATLENSURE_RETURN_HR(expr, E_FAIL)
#endif 

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif // _SECURE_ATL

#if _SECURE_ATL

#ifndef ATL_CRT_ERRORCHECK
#define ATL_CRT_ERRORCHECK(expr) AtlCrtErrorCheck(expr)
#endif // ATL_CRT_ERRORCHECK

#ifndef ATL_CRT_ERRORCHECK_SPRINTF
#define ATL_CRT_ERRORCHECK_SPRINTF(expr) \
do { \
	errno_t _saveErrno = errno; \
	errno = 0; \
	(expr); \
	if(0 != errno) \
	{ \
		AtlCrtErrorCheck(errno); \
	} \
	else \
	{ \
		errno = _saveErrno; \
	} \
} while (0)
#endif // ATL_CRT_ERRORCHECK_SPRINTF

#else // !_SECURE_ATL

#define ATL_CRT_ERRORCHECK(expr) do { expr; } while (0)
#define ATL_CRT_ERRORCHECK_SPRINTF(expr) do { expr; } while (0)

#endif // _SECURE_ATL

///////////////////////////////////////////////////////////////////////////////
// __declspec(novtable) is used on a class declaration to prevent the vtable
// pointer from being initialized in the constructor and destructor for the
// class.  This has many benefits because the linker can now eliminate the
// vtable and all the functions pointed to by the vtable.  Also, the actual
// constructor and destructor code are now smaller.
///////////////////////////////////////////////////////////////////////////////
// This should only be used on a class that is not directly createable but is
// rather only used as a base class.  Additionally, the constructor and
// destructor (if provided by the user) should not call anything that may cause
// a virtual function call to occur back on the object.
///////////////////////////////////////////////////////////////////////////////
// By default, the wizards will generate new ATL object classes with this
// attribute (through the ATL_NO_VTABLE macro).  This is normally safe as long
// the restriction mentioned above is followed.  It is always safe to remove
// this macro from your class, so if in doubt, remove it.
///////////////////////////////////////////////////////////////////////////////

#ifdef _ATL_DISABLE_NO_VTABLE
#define ATL_NO_VTABLE
#else
#define ATL_NO_VTABLE __declspec(novtable)
#endif

#ifdef _ATL_DISABLE_NOTHROW
#define ATL_NOTHROW
#else
#define ATL_NOTHROW __declspec(nothrow)
#endif

#ifdef _ATL_DISABLE_FORCEINLINE
#define ATL_FORCEINLINE
#else
#define ATL_FORCEINLINE __forceinline
#endif

#ifdef _ATL_DISABLE_NOINLINE
#define ATL_NOINLINE
#else
#define ATL_NOINLINE __declspec( noinline )
#endif

#ifdef _ATL_DISABLE_DEPRECATED
#define ATL_DEPRECATED(_Message)
#else
#define ATL_DEPRECATED(_Message) __declspec( deprecated(_Message) )
#endif

// If ATL80.DLL is being used then _ATL_STATIC_REGISTRY doesn't really make sense
#ifdef _ATL_DLL
#undef _ATL_STATIC_REGISTRY
#else
// If not linking to ATL80.DLL, use the static registrar and not building atl.dll
#ifndef _ATL_DLL_IMPL
#ifndef _ATL_STATIC_REGISTRY
#define _ATL_STATIC_REGISTRY
#endif
#endif
#endif

#ifdef _ATL_DEBUG_REFCOUNT
#ifndef _ATL_DEBUG_INTERFACES
#define _ATL_DEBUG_INTERFACES
#endif
#endif

#ifdef _DEBUG
#ifndef _ATL_DEBUG
#define _ATL_DEBUG
#endif // _ATL_DEBUG
#endif // _DEBUG

#ifdef _ATL_DEBUG_INTERFACES
#ifndef _ATL_DEBUG
#define _ATL_DEBUG
#endif // _ATL_DEBUG
#endif // _ATL_DEBUG_INTERFACES

#ifndef _ATL_HEAPFLAGS
#ifdef _MALLOC_ZEROINIT
#define _ATL_HEAPFLAGS HEAP_ZERO_MEMORY
#else
#define _ATL_HEAPFLAGS 0
#endif
#endif

#ifndef _ATL_PACKING
#define _ATL_PACKING 8
#endif

#if defined(_ATL_DLL)
	#define ATLAPI extern "C" HRESULT __declspec(dllimport) __stdcall
	#define ATLAPI_(x) extern "C" __declspec(dllimport) x __stdcall
	#define ATLINLINE
	#define ATLAPIINL extern "C" inline HRESULT __stdcall
	#define ATLAPIINL_(x) extern "C" inline x __stdcall
#elif defined(_ATL_DLL_IMPL)
	#define ATLAPI extern "C" inline HRESULT __stdcall
	#define ATLAPI_(x) extern "C" inline x __stdcall
	#define ATLAPIINL ATLAPI
	#define ATLAPIINL_(x) ATLAPI_(x)
	#define ATLINLINE
#else
	#define ATLAPI __declspec(nothrow) HRESULT __stdcall
	#define ATLAPI_(x) __declspec(nothrow) x __stdcall
	#define ATLAPIINL ATLAPI
	#define ATLAPIINL_(x) ATLAPI_(x)
	#define ATLINLINE inline
#endif

#ifdef _ATL_NO_EXCEPTIONS
	#ifdef _AFX
	#error MFC projects cannot define _ATL_NO_EXCEPTIONS
	#endif
#else
	#ifndef _CPPUNWIND
	#define _ATL_NO_EXCEPTIONS
	#endif
#endif

#ifdef _CPPUNWIND

#ifndef ATLTRYALLOC

#ifdef _AFX
#define ATLTRYALLOC(x) try{x;} catch(CException* e){e->Delete();}
#else
/* prefast noise VSW 489981 */
#define ATLTRYALLOC(x) __pragma(warning(push)) __pragma(warning(disable: 4571)) try{x;} catch(...) {} __pragma(warning(pop))
#endif	//__AFX

#endif	//ATLTRYALLOC

// If you define _ATLTRY before including this file, then
// you should define _ATLCATCH and _ATLRETHROW as well.
#ifndef _ATLTRY
#define _ATLTRY try
#ifdef _AFX
#define _ATLCATCH( e ) catch( CException* e )
#else
#define _ATLCATCH( e ) catch( CAtlException e )
#endif

#define _ATLCATCHALL() __pragma(warning(push)) __pragma(warning(disable: 4571)) catch( ... ) __pragma(warning(pop))

#ifdef _AFX
#define _ATLDELETEEXCEPTION(e) e->Delete();
#else
#define _ATLDELETEEXCEPTION(e) e;
#endif

#define _ATLRETHROW throw
#endif	// _ATLTRY

/* 
COM functions should not throw. Which means we should protect their callers from C++ exceptions leaking out. These macros
can help with that, though they have not yet been applied to the whole of ATL, which uses a variety of patterns to achieve
this end
*/
 
#ifndef _ATL_COM_BEGIN 
#define _ATL_COM_BEGIN \
	HRESULT __hrAtlComMethod=S_OK; \
	try \
	{
#endif

#ifdef _AFX
/* Nice to do something more complex here in future to translate an MFC exception to a better HR */
#define _AFX_COM_END_PART \
	catch(CException *e) \
	{ \
		if(e) \
		{ \
			e->Delete(); \
		} \
		__hrAtlComMethod=E_FAIL; \
	}
#else
#define _AFX_COM_END_PART \
	catch(CAtlException e) \
	{ \
		__hrAtlComMethod=e.m_hr; \
	}
#endif

#ifndef _ATL_COM_END 
#define _ATL_COM_END \
	_AFX_COM_END_PART \
	catch(...) \
	{ \
		__hrAtlComMethod=E_FAIL; \
	} \
	return hr; 
#endif



#else //_CPPUNWIND

#ifndef ATLTRYALLOC
#define ATLTRYALLOC(x) x;
#endif	//ATLTRYALLOC

// if _ATLTRY is defined before including this file then 
// _ATLCATCH and _ATLRETHROW should be defined as well.
#ifndef _ATLTRY
#define _ATLTRY
#define _ATLCATCH( e ) __pragma(warning(push)) __pragma(warning(disable: 4127)) if( false ) __pragma(warning(pop))
#define _ATLCATCHALL() __pragma(warning(push)) __pragma(warning(disable: 4127)) if( false ) __pragma(warning(pop))
#define _ATLDELETEEXCEPTION(e)
#define _ATLRETHROW
#endif	// _ATLTRY

#endif	//_CPPUNWIND

#ifndef ATLTRY
#define ATLTRY(x) ATLTRYALLOC(x)
#endif	//ATLTRY

#define offsetofclass(base, derived) ((DWORD_PTR)(static_cast<base*>((derived*)_ATL_PACKING))-_ATL_PACKING)

/////////////////////////////////////////////////////////////////////////////
// Master version numbers

#define _ATL     1      // Active Template Library
#define _ATL_VER 0x0800 // Active Template Library version 8.00

/////////////////////////////////////////////////////////////////////////////
// Threading

#ifndef _ATL_SINGLE_THREADED
#ifndef _ATL_APARTMENT_THREADED
#ifndef _ATL_FREE_THREADED
#define _ATL_FREE_THREADED
#endif
#endif
#endif

// UUIDOF
#ifndef _ATL_NO_UUIDOF
#define _ATL_IIDOF(x) __uuidof(x)
#else
#define _ATL_IIDOF(x) IID_##x
#endif

// Lean and mean
#ifndef ATL_NO_LEAN_AND_MEAN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMCX
#define NOMCX
#endif
#endif	// ATL_NO_LEAN_AND_MEAN

#ifdef NOSERVICE
#ifndef _ATL_NO_SERVICE
#define _ATL_NO_SERVICE
#endif	// _ATL_NO_SERVICE
#else
#ifdef _ATL_NO_SERVICE
#ifndef NOSERVICE
#define NOSERVICE
#endif	// NOSERVICE
#endif	// _ATL_NO_SERVICE
#endif	// NOSERVICE

#include <malloc.h>
#ifdef _DEBUG
#include <stdlib.h>
#endif
#ifndef _ATL_NO_DEBUG_CRT
// Warning: if you define the above symbol, you will have
// to provide your own definition of the ATLASSERT(x) macro
// in order to compile ATL
	#include <crtdbg.h>
#endif

#endif // RC_INVOKED

#define ATLAXWIN_CLASS	"AtlAxWin80"
#define ATLAXWINLIC_CLASS "AtlAxWinLic80"

// _ATL_INSECURE_DEPRECATE define
#ifndef _ATL_INSECURE_DEPRECATE
#ifdef _ATL_SECURE_NO_DEPRECATE
#define _ATL_INSECURE_DEPRECATE(_Message)
#else
#define _ATL_INSECURE_DEPRECATE(_Message) __declspec(deprecated(_Message))
#endif // _ATL_SECURE_NO_DEPRECATE
#endif // _ATL_INSECURE_DEPRECATE

/*
This is called when something really bad happens -- so bad
that we consider it dangerous to even throw an exception
*/
#ifndef _ATL_FATAL_SHUTDOWN
#define _ATL_FATAL_SHUTDOWN do { ::TerminateProcess(::GetCurrentProcess(), 0); } while(0);
#endif

//ATL/MFC code should use standard pointer to member standard syntax &MyClass::MyMethod, instead
//of the legacy non-standard syntax - MyMethod.
#ifdef _ATL_ENABLE_PTM_WARNING
#define PTM_WARNING_DISABLE
#define PTM_WARNING_RESTORE
#else
#define PTM_WARNING_DISABLE \
	__pragma(warning( push )) \
	__pragma(warning( disable : 4867 ))
#define PTM_WARNING_RESTORE \
	__pragma(warning( pop ))
#endif //_ATL_ENABLE_PTM_WARNING

/* we have to define our own versions of MAKEINTRESOURCE and IS_INTRESOURCE to 
 * fix warning 6268. At least until those macros are not cleanend in PSDK.
   Same comes true for those definitions of constants which use the above macros
*/
#define ATL_MAKEINTRESOURCEA(i) ((LPSTR)((ULONG_PTR)((WORD)(i))))
#define ATL_MAKEINTRESOURCEW(i) ((LPWSTR)((ULONG_PTR)((WORD)(i))))
#ifdef UNICODE
#define ATL_MAKEINTRESOURCE  ATL_MAKEINTRESOURCEW
#else
#define ATL_MAKEINTRESOURCE  ATL_MAKEINTRESOURCEA
#endif // !UNICODE
#define ATL_IS_INTRESOURCE(_r) ((((ULONG_PTR)(_r)) >> 16) == 0)

/*
 * Predefined Resource Types
 */
#define ATL_RT_CURSOR           ATL_MAKEINTRESOURCE(1)
#define ATL_RT_BITMAP           ATL_MAKEINTRESOURCE(2)
#define ATL_RT_ICON             ATL_MAKEINTRESOURCE(3)
#define ATL_RT_MENU             ATL_MAKEINTRESOURCE(4)
#define ATL_RT_DIALOG           ATL_MAKEINTRESOURCE(5)
#define ATL_RT_STRING           ATL_MAKEINTRESOURCE(6)
#define ATL_RT_FONTDIR          ATL_MAKEINTRESOURCE(7)
#define ATL_RT_FONT             ATL_MAKEINTRESOURCE(8)
#define ATL_RT_ACCELERATOR      ATL_MAKEINTRESOURCE(9)
#define ATL_RT_RCDATA           ATL_MAKEINTRESOURCE(10)
#define ATL_RT_MESSAGETABLE     ATL_MAKEINTRESOURCE(11)

#define ATL_DIFFERENCE     11
#define ATL_RT_GROUP_CURSOR ATL_MAKEINTRESOURCE((ULONG_PTR)ATL_RT_CURSOR + ATL_DIFFERENCE)
#define ATL_RT_GROUP_ICON   ATL_MAKEINTRESOURCE((ULONG_PTR)ATL_RT_ICON + ATL_DIFFERENCE)
#define ATL_RT_VERSION      ATL_MAKEINTRESOURCE(16)
#define ATL_RT_DLGINCLUDE   ATL_MAKEINTRESOURCE(17)
#if(WINVER >= 0x0400)
#define ATL_RT_PLUGPLAY     ATL_MAKEINTRESOURCE(19)
#define ATL_RT_VXD          ATL_MAKEINTRESOURCE(20)
#define ATL_RT_ANICURSOR    ATL_MAKEINTRESOURCE(21)
#define ATL_RT_ANIICON      ATL_MAKEINTRESOURCE(22)
#endif /* WINVER >= 0x0400 */
#define ATL_RT_HTML         ATL_MAKEINTRESOURCE(23)
#ifdef RC_INVOKED
#define ATL_RT_MANIFEST                        24
#define ATL_CREATEPROCESS_MANIFEST_RESOURCE_ID  1
#define ATL_ISOLATIONAWARE_MANIFEST_RESOURCE_ID 2
#define ATL_ISOLATIONAWARE_NOSTATICIMPORT_MANIFEST_RESOURCE_ID 3
#define ATL_MINIMUM_RESERVED_MANIFEST_RESOURCE_ID 1   /* inclusive */
#define ATL_MAXIMUM_RESERVED_MANIFEST_RESOURCE_ID 16  /* inclusive */
#else  /* RC_INVOKED */
#define ATL_RT_MANIFEST                        ATL_MAKEINTRESOURCE(24)
#define ATL_CREATEPROCESS_MANIFEST_RESOURCE_ID ATL_MAKEINTRESOURCE( 1)
#define ATL_ISOLATIONAWARE_MANIFEST_RESOURCE_ID ATL_MAKEINTRESOURCE(2)
#define ATL_ISOLATIONAWARE_NOSTATICIMPORT_MANIFEST_RESOURCE_ID ATL_MAKEINTRESOURCE(3)
#define ATL_MINIMUM_RESERVED_MANIFEST_RESOURCE_ID ATL_MAKEINTRESOURCE( 1 /*inclusive*/)
#define ATL_MAXIMUM_RESERVED_MANIFEST_RESOURCE_ID ATL_MAKEINTRESOURCE(16 /*inclusive*/)
#endif /* RC_INVOKED */

/* sal.h stuff that is not in the current LKG */
#ifndef __out_ecount_part_z
#define __out_ecount_part_z(size,length)                        __out_ecount_part(size,length) __post __nullterminated
#endif

#ifndef __out_ecount_part_z_opt
#define __out_ecount_part_z_opt(size,length)                    __out_ecount_part_opt(size,length) __post __nullterminated
#endif

#ifndef __deref_opt_out_z
#define __deref_opt_out_z                                       __deref_opt_out __post __deref __nullterminated
#endif

#ifndef __out_bcount_part_z
#define __out_bcount_part_z(size,length)                        __out_bcount_part(size,length) __post __nullterminated
#endif

#endif // __ATLDEF_H__

/////////////////////////////////////////////////////////////////////////////
