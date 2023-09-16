// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLCHECKED_H__
#define __ATLCHECKED_H__

#pragma once

#include <atldef.h>
#include <atlexcept.h>
#include <malloc.h>
#include <string.h>
#include <mbstring.h>
#include <wchar.h>
#include <tchar.h>
#include <stdlib.h>


#pragma pack(push,_ATL_PACKING)
namespace ATL
{

inline errno_t AtlCrtErrorCheck(errno_t nError)
{
	switch(nError)
	{
	case ENOMEM:
		AtlThrow(E_OUTOFMEMORY);
		break;
	case EINVAL:
	case ERANGE:
		AtlThrow(E_INVALIDARG);
		break;
	case 0:
	case STRUNCATE:
		break;
	default:
		AtlThrow(E_FAIL);
		break;
	}
	return nError;
}

/////////////////////////////////////////////////////////////////////////////
// Secure (Checked) CRT functions

namespace Checked
{

#if _SECURE_ATL

#ifdef _AFX
#define ATLMFC_CRT_ERRORCHECK(expr) AFX_CRT_ERRORCHECK(expr)
#else
#define ATLMFC_CRT_ERRORCHECK(expr) ATL_CRT_ERRORCHECK(expr)
#endif

inline void __cdecl memcpy_s(__out_bcount_part(_S1max,_N)  void *_S1, __in size_t _S1max, __in_bcount(_N) const void *_S2, __in size_t _N)
{
	ATLMFC_CRT_ERRORCHECK(::memcpy_s(_S1, _S1max, _S2, _N));
}

inline void __cdecl wmemcpy_s(__out_ecount_part(_N1,_N) wchar_t *_S1, __in size_t _N1, __in_ecount(_N) const wchar_t *_S2, __in size_t _N)
{
	ATLMFC_CRT_ERRORCHECK(::wmemcpy_s(_S1, _N1, _S2, _N));
}

inline void __cdecl memmove_s(__out_bcount_part(_S1max,_N) void *_S1, __in size_t _S1max, __in_bcount(_N) const void *_S2, size_t _N)
{
	ATLMFC_CRT_ERRORCHECK(::memmove_s(_S1, _S1max, _S2, _N));
}

inline void __cdecl strcpy_s(__out_ecount(_S1max) char *_S1, __in size_t _S1max, __in_z const char *_S2)
{
	ATLMFC_CRT_ERRORCHECK(::strcpy_s(_S1, _S1max, _S2));
}

inline void __cdecl wcscpy_s(__out_ecount(_S1max) wchar_t *_S1, __in size_t _S1max, __in_z const wchar_t *_S2)
{
	ATLMFC_CRT_ERRORCHECK(::wcscpy_s(_S1, _S1max, _S2));
}

inline void __cdecl tcscpy_s(__out_ecount(_SizeInChars) TCHAR * _Dst, __in size_t _SizeInChars, __in_z const TCHAR * _Src)
{
#ifndef _ATL_MIN_CRT
	ATLMFC_CRT_ERRORCHECK(::_tcscpy_s(_Dst, _SizeInChars, _Src));
#else
#ifdef UNICODE
	ATLMFC_CRT_ERRORCHECK(::wcscpy_s(_Dst, _SizeInChars, _Src));
#else
	ATLMFC_CRT_ERRORCHECK(::strcpy_s(_Dst, _SizeInChars, _Src));
#endif
#endif
}

inline errno_t __cdecl strncpy_s(__out_ecount(_SizeInChars) char *_Dest, __in size_t _SizeInChars, __in_z const char *_Source, __in size_t _Count)
{
	return ATLMFC_CRT_ERRORCHECK(::strncpy_s(_Dest, _SizeInChars, _Source,_Count));	
}

inline errno_t __cdecl wcsncpy_s(__out_ecount(_SizeInChars) wchar_t *_Dest, __in size_t _SizeInChars, __in_z const wchar_t *_Source, __in size_t _Count)
{
	return ATLMFC_CRT_ERRORCHECK(::wcsncpy_s(_Dest, _SizeInChars, _Source,_Count));	
}

inline errno_t __cdecl tcsncpy_s(__out_ecount(_SizeInChars) TCHAR *_Dest, __in size_t _SizeInChars, __in_z const TCHAR *_Source, __in size_t _Count)
{
#ifndef _ATL_MIN_CRT
	return ATLMFC_CRT_ERRORCHECK(::_tcsncpy_s(_Dest, _SizeInChars, _Source,_Count));	
#else
#ifdef UNICODE
	return ATLMFC_CRT_ERRORCHECK(::wcsncpy_s(_Dest, _SizeInChars, _Source,_Count));	
#else
	return ATLMFC_CRT_ERRORCHECK(::strncpy_s(_Dest, _SizeInChars, _Source,_Count));	
#endif
#endif
}

inline void __cdecl strcat_s(__inout_ecount_z(_SizeInChars) char * _Dst, __in size_t _SizeInChars, __in_z const char * _Src)
{
	ATLMFC_CRT_ERRORCHECK(::strcat_s(_Dst, _SizeInChars, _Src));
}

inline void __cdecl wcscat_s(__inout_ecount_z(_SizeInChars) wchar_t * _Dst, __in size_t _SizeInChars, __in_z const wchar_t * _Src)
{
	ATLMFC_CRT_ERRORCHECK(::wcscat_s(_Dst, _SizeInChars, _Src));
}

inline void __cdecl tcscat_s(__inout_ecount_z(_SizeInChars) TCHAR * _Dst, __in size_t _SizeInChars, __in_z const TCHAR * _Src)
{
#ifndef _ATL_MIN_CRT
	ATLMFC_CRT_ERRORCHECK(::_tcscat_s(_Dst, _SizeInChars, _Src));
#else
#ifdef UNICODE
	ATLMFC_CRT_ERRORCHECK(::wcscat_s(_Dst, _SizeInChars, _Src));
#else
	ATLMFC_CRT_ERRORCHECK(::strcat_s(_Dst, _SizeInChars, _Src));
#endif
#endif
}

inline void __cdecl strlwr_s(__inout_ecount_z(_SizeInChars) char * _Str, __in size_t _SizeInChars)
{
	ATLMFC_CRT_ERRORCHECK(::_strlwr_s(_Str, _SizeInChars));	
}

inline void __cdecl wcslwr_s(__inout_ecount_z(_SizeInChars) wchar_t * _Str, __in size_t _SizeInChars)
{
	ATLMFC_CRT_ERRORCHECK(::_wcslwr_s(_Str, _SizeInChars));	
}

inline void __cdecl mbslwr_s(__inout_bcount_z(_SizeInChars) unsigned char * _Str, __in size_t _SizeInChars)
{
	ATLMFC_CRT_ERRORCHECK(::_mbslwr_s(_Str, _SizeInChars));	
}

inline void __cdecl tcslwr_s(__inout_ecount_z(_SizeInChars) TCHAR * _Str, __in size_t _SizeInChars)
{
#ifndef _ATL_MIN_CRT
	ATLMFC_CRT_ERRORCHECK(::_tcslwr_s(_Str, _SizeInChars));	
#else
#ifdef UNICODE
	ATLMFC_CRT_ERRORCHECK(::_wcslwr_s(_Str, _SizeInChars));	
#else
	ATLMFC_CRT_ERRORCHECK(::_strlwr_s(_Str, _SizeInChars));	
#endif
#endif
}

inline void __cdecl strupr_s(__inout_ecount_z(_SizeInChars) char * _Str, __in size_t _SizeInChars)
{
	ATLMFC_CRT_ERRORCHECK(::_strupr_s(_Str, _SizeInChars));	
}

inline void __cdecl wcsupr_s(__inout_ecount_z(_SizeInChars) wchar_t * _Str, __in size_t _SizeInChars)
{
	ATLMFC_CRT_ERRORCHECK(::_wcsupr_s(_Str, _SizeInChars));	
}

inline void __cdecl mbsupr_s(__inout_bcount_z(_SizeInChars) unsigned char * _Str, __in size_t _SizeInChars)
{
	ATLMFC_CRT_ERRORCHECK(::_mbsupr_s(_Str, _SizeInChars));	
}

inline void __cdecl tcsupr_s(__inout_ecount_z(_SizeInChars) TCHAR * _Str, __in size_t _SizeInChars)
{
	ATLMFC_CRT_ERRORCHECK(::_tcsupr_s(_Str, _SizeInChars));	
}

inline void __cdecl itoa_s(__in int _Val, __out_ecount_z(_SizeInChars) char *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_itoa_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl itot_s(__in int _Val, __out_ecount_z(_SizeInChars) TCHAR *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_itot_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl ltoa_s(__in long _Val, __out_ecount_z(_SizeInChars) char *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_ltoa_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl ltot_s(__in long _Val, __out_ecount_z(_SizeInChars) TCHAR *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_ltot_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl ultoa_s(__in unsigned long _Val, __out_ecount_z(_SizeInChars) char *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_ultoa_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl ultow_s(__in unsigned long _Val, __out_ecount_z(_SizeInChars) wchar_t *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_ultow_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl ultot_s(__in unsigned long _Val, __out_ecount_z(_SizeInChars) TCHAR *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_ultot_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl i64toa_s(__in __int64 _Val, __out_ecount_z(_SizeInChars) char *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_i64toa_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl i64tow_s(__in __int64 _Val, __out_ecount_z(_SizeInChars) wchar_t *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_i64tow_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl ui64toa_s(__in unsigned __int64 _Val, __out_ecount_z(_SizeInChars) char *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_ui64toa_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl ui64tow_s(__in unsigned __int64 _Val, __out_ecount_z(_SizeInChars) wchar_t *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	ATLMFC_CRT_ERRORCHECK(::_ui64tow_s(_Val, _Buf, _SizeInChars, _Radix));
}

inline void __cdecl gcvt_s(__out_ecount_z(_SizeInChars) char *_Buffer, __in size_t _SizeInChars, __in double _Value, __in int _Ndec)
{
	ATLMFC_CRT_ERRORCHECK(::_gcvt_s(_Buffer, _SizeInChars, _Value, _Ndec));
}

inline void __cdecl tsplitpath_s(__in_z const TCHAR *_Path, __out_ecount_z_opt(_Drive_len) TCHAR *_Drive, __in size_t _Drive_len, 
	__out_ecount_z_opt(_Dir_len) TCHAR *_Dir, __in size_t _Dir_len, 
	__out_ecount_z_opt(_Fname_len) TCHAR *_Fname, __in size_t _Fname_len, 
	__out_ecount_z_opt(_Ext_len) TCHAR *_Ext, __in size_t _Ext_len)
{
	ATLMFC_CRT_ERRORCHECK(::_tsplitpath_s(_Path, _Drive, _Drive_len, _Dir, _Dir_len, _Fname, _Fname_len, _Ext, _Ext_len));
}

inline void __cdecl tmakepath_s(__out_ecount_z(_SizeInChars) TCHAR *_Path, __in size_t _SizeInChars, __in_z const TCHAR *_Drive, 
	__in_z const TCHAR *_Dir, __in_z const TCHAR *_Fname, __in_z const TCHAR *_Ext)
{
	ATLMFC_CRT_ERRORCHECK(::_tmakepath_s(_Path, _SizeInChars, _Drive, _Dir, _Fname, _Ext));
}

inline size_t __cdecl strnlen(__in_ecount(_Maxsize) const char *_Str, __in size_t _Maxsize)
{
	return ::strnlen(_Str, _Maxsize);
}

inline size_t __cdecl wcsnlen(__in_ecount(_Maxsize) const wchar_t *_Wcs, __in size_t _Maxsize)
{
	return ::wcsnlen(_Wcs, _Maxsize);
}

inline size_t __cdecl tcsnlen(__in_ecount(_Maxsize) const TCHAR *_Str, __in size_t _Maxsize)
{
	return ::_tcsnlen(_Str, _Maxsize);
}

inline int get_errno()
{
	int nErrNo;
	ATLMFC_CRT_ERRORCHECK(::_get_errno(&nErrNo));
	return nErrNo;
}

inline void set_errno(__in int _Value)
{
	ATLMFC_CRT_ERRORCHECK(::_set_errno(_Value));
}

#else // !_SECURE_ATL

#define ATLMFC_CRT_ERRORCHECK(expr) do { expr; } while (0)

inline void __cdecl memcpy_s(__out_bcount(_S1max) void *_S1, __in size_t _S1max, __in_bcount(_N) const void *_S2, size_t _N)
{
	(_S1max);
	memcpy(_S1, _S2, _N);
}

inline void __cdecl wmemcpy_s(__out_ecount(_N1) wchar_t *_S1, __in size_t _N1, __in_ecount(_N) const wchar_t *_S2, __in size_t _N)
{
	(_N1);
	::wmemcpy(_S1, _S2, _N);
}

inline void __cdecl memmove_s(__out_bcount(_S1max) void *_S1, __in size_t _S1max, __in_bcount(_N) const void *_S2, __in size_t _N)
{
	(_S1max);
	memmove(_S1, _S2, _N);
}

inline void __cdecl strcpy_s(__out_ecount_z(_S1max) char *_S1, __in size_t _S1max, __in_z const char *_S2)
{
	(_S1max);
	::strcpy(_S1, _S2);
}

inline void __cdecl wcscpy_s(__out_ecount_z(_S1max) wchar_t *_S1, __in size_t _S1max, __in_z const wchar_t *_S2)
{
	(_S1max);
	::wcscpy(_S1, _S2);
}

inline void __cdecl tcscpy_s(__out_ecount_z(_SizeInChars) TCHAR * _Dst, __in size_t _SizeInChars, __in_z const TCHAR * _Src)
{
	(_SizeInChars);
#ifndef _ATL_MIN_CRT
	::_tcscpy(_Dst, _Src);
#else
#ifdef UNICODE
	::wcscpy(_Dst, _Src);
#else
	::strcpy(_Dst, _Src);
#endif
#endif
}

/* ensure that strncpy_s null-terminate the dest string */
inline errno_t __cdecl strncpy_s(__out_ecount_z(_SizeInChars) char *_Dest, __in size_t _SizeInChars, __in_z const char *_Source,__in size_t _Count)
{
	if (_Count == _TRUNCATE)
	{
		_Count = _SizeInChars - 1;
	}
	while (_Count > 0 && *_Source != 0)
	{
		*_Dest++ = *_Source++;
		--_Count;
	}
	*_Dest = 0;

	return (*_Source!=0) ? STRUNCATE : 0;
}

inline errno_t __cdecl wcsncpy_s(__out_ecount_z(_SizeInChars) wchar_t *_Dest, __in size_t _SizeInChars, __in_z const wchar_t *_Source, __in size_t _Count)
{
	if (_Count == _TRUNCATE)
	{
		_Count = _SizeInChars - 1;
	}
	while (_Count > 0 && *_Source != 0)
	{
		*_Dest++ = *_Source++;
		--_Count;
	}
	*_Dest = 0;

	return (*_Source!=0) ? STRUNCATE : 0;
}

inline errno_t __cdecl tcsncpy_s(__out_ecount_z(_SizeInChars) TCHAR *_Dest, __in size_t _SizeInChars, __in_z const TCHAR *_Source,__in size_t _Count)
{
	if (_Count == _TRUNCATE)
	{
		if(_SizeInChars>0)
		{
			_Count = _SizeInChars - 1;
		}
		else
		{
			_Count =0;
		}
	}

#ifndef _ATL_MIN_CRT
#pragma warning(push)
#pragma warning(disable: 6535)
	::_tcsncpy(_Dest,_Source,_Count);
#pragma warning(pop)
	if(_SizeInChars>0)
	{
		_Dest[_SizeInChars-1] = 0;
	}
#else
	while (_Count > 0 && *_Source != 0)
	{
		*_Dest++ = *_Source++;
		--_Count;
	}
	*_Dest = 0;
#endif

	return (*_Source!=0) ? STRUNCATE : 0;
}

inline void __cdecl strcat_s(__inout_ecount_z(_SizeInChars) char * _Dst, __in size_t _SizeInChars, __in_z const char * _Src)
{
	(_SizeInChars);
	::strcat(_Dst, _Src);
}

inline void __cdecl wcscat_s(__inout_ecount_z(_SizeInChars) wchar_t * _Dst, __in size_t _SizeInChars, __in_z const wchar_t * _Src)
{
	(_SizeInChars);
	::wcscat(_Dst, _Src);
}

inline void __cdecl tcscat_s(__inout_ecount_z(_SizeInChars) TCHAR * _Dst, __in size_t _SizeInChars, __in_z const TCHAR * _Src)
{
	(_SizeInChars);
#ifndef _ATL_MIN_CRT
	::_tcscat(_Dst, _Src);
#else
#ifdef UNICODE
	::wcscat(_Dst, _Src);
#else
	::strcat(_Dst, _Src);
#endif
#endif
}

inline void __cdecl strlwr_s(__inout_ecount_z(_SizeInChars) char * _Str, size_t _SizeInChars)
{
	(_SizeInChars);
	::_strlwr(_Str);	
}

inline void __cdecl wcslwr_s(__inout_ecount_z(_SizeInChars) wchar_t * _Str, size_t _SizeInChars)
{
	(_SizeInChars);
	::_wcslwr(_Str);	
}

inline void __cdecl mbslwr_s(__inout_bcount_z(_SizeInChars) unsigned char * _Str, size_t _SizeInChars)
{
	(_SizeInChars);
	::_mbslwr(_Str);
}

inline void __cdecl tcslwr_s(__inout_ecount_z(_SizeInChars) TCHAR * _Str, size_t _SizeInChars)
{
	(_SizeInChars);
#ifndef _ATL_MIN_CRT
	::_tcslwr(_Str);
#else
#ifdef UNICODE
	::_wcslwr(_Str);
#else
	::_strlwr(_Str);
#endif
#endif
}

inline void __cdecl itoa_s(__in int _Val, __out_ecount_z(_SizeInChars) char *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	(_SizeInChars);
	::_itoa_s(_Val, _Buf, _SizeInChars, _Radix);
}

inline void __cdecl itot_s(__in int _Val, __out_ecount_z(_SizeInChars) TCHAR *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	(_SizeInChars);
	::_itot(_Val, _Buf, _Radix);
}

inline void __cdecl ltoa_s(__in long _Val, __out_ecount_z(_SizeInChars) char *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	(_SizeInChars);
	::_ltoa(_Val, _Buf, _Radix);
}

inline void __cdecl ltot_s(__in long _Val, __out_ecount_z(_SizeInChars) TCHAR *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	(_SizeInChars);
	::_ltot(_Val, _Buf, _Radix);
}

inline void __cdecl ultoa_s(__in unsigned long _Val, __out_ecount_z(_SizeInChars) char *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	(_SizeInChars);
	::_ultoa(_Val, _Buf, _Radix);
}

inline void __cdecl ultow_s(__in unsigned long _Val, __out_ecount_z(_SizeInChars) wchar_t *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	(_SizeInChars);
	::_ultow(_Val, _Buf, _Radix);
}

inline void __cdecl ultot_s(__in unsigned long _Val, __out_ecount_z(_SizeInChars) TCHAR *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	(_SizeInChars);
	::_ultot(_Val, _Buf, _Radix);
}

inline void __cdecl i64toa_s(__in __int64 _Val, __out_ecount_z(_SizeInChars) char *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	(_SizeInChars);
	::_i64toa(_Val, _Buf, _Radix);
}

inline void __cdecl i64tow_s(__in __int64 _Val, __out_ecount_z(_SizeInChars) wchar_t *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	(_SizeInChars);
	::_i64tow(_Val, _Buf, _Radix);
}

inline void __cdecl ui64toa_s(__in unsigned __int64 _Val, __out_ecount_z(_SizeInChars) char *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	(_SizeInChars);
	::_ui64toa(_Val, _Buf, _Radix);
}

inline void __cdecl ui64tow_s(__in unsigned __int64 _Val, __out_ecount_z(_SizeInChars) wchar_t *_Buf, __in size_t _SizeInChars, __in int _Radix)
{
	(_SizeInChars);
	::_ui64tow(_Val, _Buf, _Radix);
}

inline void __cdecl gcvt_s(__out_ecount_z(_SizeInChars) char *_Buffer, __in size_t _SizeInChars, __in double _Value, __in int _Ndec)
{
	(_SizeInChars);
	::_gcvt(_Value, _Ndec, _Buffer);
}

inline void __cdecl tsplitpath_s(__in_z const TCHAR *_Path, __out_ecount_z_opt(_Drive_len) TCHAR *_Drive, __in size_t _Drive_len, 
	__out_ecount_z_opt(_Dir_len) TCHAR *_Dir, __in size_t _Dir_len, 
	__out_ecount_z_opt(_Fname_ext) TCHAR *_Fname, __in size_t _Fname_len, 
	__out_ecount_z_opt(_Ext_len) TCHAR *_Ext, __in size_t _Ext_len)
{
	(_Drive_len, _Dir_len, _Fname_len, _Ext_len);
	::_tsplitpath(_Path, _Drive, _Dir, _Fname, _Ext);
}

inline void __cdecl tmakepath_s(__out_ecount_z(_SizeInChars) TCHAR *_Path, __in size_t _SizeInChars, __in_z const TCHAR *_Drive, 
	__in_z const TCHAR *_Dir, __in_z const TCHAR *_Fname, __in_z const TCHAR *_Ext)
{
	(_SizeInChars);
	::_tmakepath(_Path, _Drive, _Dir, _Fname, _Ext);
}

inline size_t __cdecl strnlen(__in_ecount(_Maxsize) const char *_Str, __in size_t _Maxsize)
{
	(_Maxsize);
	return ::strlen(_Str);
}

inline size_t __cdecl wcsnlen(__in_ecount(_Maxsize) const wchar_t *_Wcs, __in size_t _Maxsize)
{
	(_Maxsize);
	return ::wcslen(_Wcs);
}

inline size_t __cdecl tcsnlen(__in_ecount(_Maxsize) const TCHAR *_Str, __in size_t _Maxsize)
{
	(_Maxsize);
	return ::_tcslen(_Str);
}

inline int get_errno()
{
	return errno;
}

inline void set_errno(__in int _Value)
{
	errno = _Value;
}

#endif // _SECURE_ATL

} // namespace Checked

} // namespace ATL
#pragma pack(pop)

#endif // __ATLCHECKED_H__

/////////////////////////////////////////////////////////////////////////////
