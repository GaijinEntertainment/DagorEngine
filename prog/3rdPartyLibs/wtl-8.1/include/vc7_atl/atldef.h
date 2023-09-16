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

#ifndef RC_INVOKED

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifdef UNDER_CE
	#error ATL not currently supported for CE
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

#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif

#ifndef ATLVERIFY
#ifdef _DEBUG
#define ATLVERIFY(expr) ATLASSERT(expr)
#else
#define ATLVERIFY(expr) (expr)
#endif // DEBUG
#endif // ATLVERIFY

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
#define ATL_DEPRECATED
#else
#define ATL_DEPRECATED __declspec( deprecated )
#endif

// If ATL71.DLL is being used then _ATL_STATIC_REGISTRY doesn't really make sense
#ifdef _ATL_DLL
#undef _ATL_STATIC_REGISTRY
#else
// If not linking to ATL71.DLL, use the static registrar and not building atl.dll
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
#elif defined(_ATL_DLL_IMPL)
	#define ATLAPI extern "C" inline HRESULT __stdcall
	#define ATLAPI_(x) extern "C" inline x __stdcall
	#define ATLINLINE
#else
	#define ATLAPI __declspec(nothrow) HRESULT __stdcall
	#define ATLAPI_(x) __declspec(nothrow) x __stdcall
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
#define ATLTRYALLOC(x) try{x;} catch(...){}
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

#define _ATLCATCHALL() catch( ... )

#ifdef _AFX
#define _ATLDELETEEXCEPTION(e) e->Delete();
#else
#define _ATLDELETEEXCEPTION(e) e;
#endif

#define _ATLRETHROW throw
#endif	// _ATLTRY

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
#define _ATL_VER 0x0710 // Active Template Library version 7.10

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
#define WIN32_LEAN_AND_MEAN
#define NOMCX
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

#define ATLAXWIN_CLASS	"AtlAxWin71"
#define ATLAXWINLIC_CLASS "AtlAxWinLic71"

#endif // __ATLDEF_H__

/////////////////////////////////////////////////////////////////////////////
