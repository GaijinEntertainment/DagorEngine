// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.

#ifndef __ATLTRACE_H__
#define __ATLTRACE_H__

#pragma once

#include <atldef.h>
#include <atlconv.h>

#ifdef _DEBUG
#include <stdio.h>
#include <stdarg.h>
#endif

#ifdef _DEBUG
#include <atldebugapi.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;
#endif  // _DEBUG
 

#pragma pack(push,_ATL_PACKING)
namespace ATL
{

// Declare a global instance of this class to automatically register a custom trace category at startup
class CTraceCategory
{
public:
	explicit CTraceCategory( LPCTSTR pszCategoryName, UINT nStartingLevel = 0 ) throw();

#ifdef _DEBUG
	UINT GetLevel() const throw();
	void SetLevel( UINT nLevel ) throw();
	ATLTRACESTATUS GetStatus() const throw();
	void SetStatus( ATLTRACESTATUS eStatus) throw();
#endif

	operator DWORD_PTR() const throw();

public:
#ifdef _DEBUG
	DWORD_PTR m_dwCategory;
#endif
};

#ifdef _DEBUG

class CTrace
{
public:
	typedef int (__cdecl *fnCrtDbgReport_t)(int,const char *,int,const char *,const char *,...);

private:
	CTrace(
#ifdef _ATL_NO_DEBUG_CRT
		fnCrtDbgReport_t pfnCrtDbgReport = NULL)
#else
		fnCrtDbgReport_t pfnCrtDbgReport = _CrtDbgReport)
#endif
		: m_hInst(reinterpret_cast<HINSTANCE>(&__ImageBase)),
			m_dwModule( 0 )
	{
		m_dwModule = AtlTraceRegister(m_hInst, pfnCrtDbgReport);
	}

	~CTrace()
	{
		AtlTraceUnregister(m_dwModule);
	}

public:
	bool ChangeCategory(DWORD_PTR dwCategory, UINT nLevel, ATLTRACESTATUS eStatus)
	{
		return 0 !=
			AtlTraceModifyCategory(0, dwCategory, nLevel, eStatus);
	}

	bool GetCategory(DWORD_PTR dwCategory, UINT *pnLevel, ATLTRACESTATUS *peStatus)
	{
		ATLASSERT(pnLevel && peStatus);
		return 0 != AtlTraceGetCategory(0, dwCategory, pnLevel, peStatus);
	}
	UINT GetLevel()
	{
		ATLTRACESTATUS eStatus;
		UINT nLevel;
		AtlTraceGetModule(0, m_dwModule, &nLevel, &eStatus);

		return nLevel;
	}
	void SetLevel(UINT nLevel)
	{
		AtlTraceModifyModule(0, m_dwModule, nLevel, ATLTRACESTATUS_ENABLED);
	}
	ATLTRACESTATUS GetStatus()
	{
		ATLTRACESTATUS eStatus;
		UINT nLevel;
		AtlTraceGetModule(0, m_dwModule, &nLevel, &eStatus);

		return eStatus;
	}
	void SetStatus(ATLTRACESTATUS eStatus)
	{
		ATLTRACESTATUS eOldStatus;
		UINT nLevel;
		AtlTraceGetModule(0, m_dwModule, &nLevel, &eOldStatus);
		AtlTraceModifyModule(0, m_dwModule, nLevel, eStatus);
	}
	void __cdecl TraceV(const char *pszFileName, int nLine,
		DWORD_PTR dwCategory, UINT nLevel, LPCSTR pszFmt, va_list args) const;
	void __cdecl TraceV(const char *pszFileName, int nLine,
		DWORD_PTR dwCategory, UINT nLevel, LPCWSTR pszFmt, va_list args) const;

	DWORD_PTR RegisterCategory(LPCSTR pszCategory)
		{return(AtlTraceRegisterCategoryA(m_dwModule, pszCategory));}
#ifdef _UNICODE
	DWORD_PTR RegisterCategory(LPCWSTR pszCategory)
		{return(AtlTraceRegisterCategoryU(m_dwModule, pszCategory));}
#endif

	bool LoadSettings(LPCTSTR pszFileName = NULL) const
		{return 0 != AtlTraceLoadSettings(pszFileName);}
	void SaveSettings(LPCTSTR pszFileName = NULL) const
		{AtlTraceSaveSettings(pszFileName);}

public:
	static CTrace s_trace;

protected:
	HINSTANCE m_hInst;
	DWORD_PTR m_dwModule;
};

    inline void __cdecl CTrace::TraceV(const char *pszFileName, int nLine,
		DWORD_PTR dwCategory, UINT nLevel, LPCSTR pszFmt, va_list args) const
	{
		AtlTraceVA(m_dwModule, pszFileName, nLine, dwCategory, nLevel, pszFmt, args);
	}
	inline void __cdecl CTrace::TraceV(const char *pszFileName, int nLine,
		DWORD_PTR dwCategory, UINT nLevel, LPCWSTR pszFmt, va_list args) const
	{
		AtlTraceVU(m_dwModule, pszFileName, nLine, dwCategory, nLevel, pszFmt, args);
	}

extern CTraceCategory atlTraceGeneral;

class CTraceFileAndLineInfo
{
public:
	CTraceFileAndLineInfo(const char *pszFileName, int nLineNo)
		: m_pszFileName(pszFileName), m_nLineNo(nLineNo)
	{}

#pragma warning(push)
#pragma warning(disable : 4793)
	void __cdecl operator()(DWORD_PTR dwCategory, UINT nLevel, const char *pszFmt, ...) const
	{
		va_list ptr; va_start(ptr, pszFmt);
		ATL::CTrace::s_trace.TraceV(m_pszFileName, m_nLineNo, dwCategory, nLevel, pszFmt, ptr);
		va_end(ptr);
	}
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4793)
	void __cdecl operator()(DWORD_PTR dwCategory, UINT nLevel, const wchar_t *pszFmt, ...) const
	{
		va_list ptr; va_start(ptr, pszFmt);
		ATL::CTrace::s_trace.TraceV(m_pszFileName, m_nLineNo, dwCategory, nLevel, pszFmt, ptr);
		va_end(ptr);
	}
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4793)
	void __cdecl operator()(const char *pszFmt, ...) const
	{
		va_list ptr; va_start(ptr, pszFmt);
		ATL::CTrace::s_trace.TraceV(m_pszFileName, m_nLineNo, atlTraceGeneral, 0, pszFmt, ptr);
		va_end(ptr);
	}
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4793)
	void __cdecl operator()(const wchar_t *pszFmt, ...) const
	{
		va_list ptr; va_start(ptr, pszFmt);
		ATL::CTrace::s_trace.TraceV(m_pszFileName, m_nLineNo, atlTraceGeneral, 0, pszFmt, ptr);
		va_end(ptr);
	}
#pragma warning(pop)

private:
	/* unimplemented */
	CTraceFileAndLineInfo &__cdecl operator=(const CTraceFileAndLineInfo &right);

	const char *const m_pszFileName;
	const int m_nLineNo;
};

#endif  // _DEBUG

#ifdef _DEBUG

inline CTraceCategory::CTraceCategory( LPCTSTR pszCategoryName, UINT nStartingLevel ) throw() :
	m_dwCategory( 0 )
{
	m_dwCategory = ATL::CTrace::s_trace.RegisterCategory( pszCategoryName );
	ATL::CTrace::s_trace.ChangeCategory( m_dwCategory, nStartingLevel, ATLTRACESTATUS_INHERIT);
}

inline CTraceCategory::operator DWORD_PTR() const throw()
{
	return( m_dwCategory );
}

inline UINT CTraceCategory::GetLevel() const throw()
{
	UINT nLevel;
	ATLTRACESTATUS eStatus;
	ATL::CTrace::s_trace.GetCategory( m_dwCategory, &nLevel, &eStatus );

	return( nLevel );
}

inline void CTraceCategory::SetLevel( UINT nLevel ) throw()
{
	ATL::CTrace::s_trace.ChangeCategory( m_dwCategory, nLevel, ATLTRACESTATUS_ENABLED );
}

inline ATLTRACESTATUS CTraceCategory::GetStatus() const throw()
{
	UINT nLevel;
	ATLTRACESTATUS eStatus;
	ATL::CTrace::s_trace.GetCategory( m_dwCategory, &nLevel, &eStatus );

	return( eStatus );
}

inline void CTraceCategory::SetStatus( ATLTRACESTATUS eStatus ) throw()
{
	UINT nLevel;
	ATLTRACESTATUS eOldStatus;
	ATL::CTrace::s_trace.GetCategory( m_dwCategory, &nLevel, &eOldStatus );
	ATL::CTrace::s_trace.ChangeCategory( m_dwCategory, nLevel, eStatus );
}

#else  // !_DEBUG

inline CTraceCategory::CTraceCategory( LPCTSTR pszCategoryName, UINT nStartingLevel ) throw()
{
	(void)pszCategoryName;
	(void)nStartingLevel;
}

inline CTraceCategory::operator DWORD_PTR() const throw()
{
	return( 0 );
}

#endif  // _DEBUG

}  // namespace ATL
 
namespace ATL
{

#ifdef _DEBUG
#define DECLARE_TRACE_CATEGORY( name ) extern ATL::CTraceCategory name;
#else
#define DECLARE_TRACE_CATEGORY( name ) const DWORD_PTR name = 0;
#endif

DECLARE_TRACE_CATEGORY( atlTraceGeneral )
DECLARE_TRACE_CATEGORY( atlTraceCOM )  
DECLARE_TRACE_CATEGORY( atlTraceQI )	
DECLARE_TRACE_CATEGORY( atlTraceRegistrar )
DECLARE_TRACE_CATEGORY( atlTraceRefcount )
DECLARE_TRACE_CATEGORY( atlTraceWindowing )
DECLARE_TRACE_CATEGORY( atlTraceControls )
DECLARE_TRACE_CATEGORY( atlTraceHosting ) 
DECLARE_TRACE_CATEGORY( atlTraceDBClient )  
DECLARE_TRACE_CATEGORY( atlTraceDBProvider )
DECLARE_TRACE_CATEGORY( atlTraceSnapin )
DECLARE_TRACE_CATEGORY( atlTraceNotImpl )   
DECLARE_TRACE_CATEGORY( atlTraceAllocation )
DECLARE_TRACE_CATEGORY( atlTraceException )
DECLARE_TRACE_CATEGORY( atlTraceTime )
DECLARE_TRACE_CATEGORY( atlTraceCache )		
DECLARE_TRACE_CATEGORY( atlTraceStencil )
DECLARE_TRACE_CATEGORY( atlTraceString )
DECLARE_TRACE_CATEGORY( atlTraceMap )	
DECLARE_TRACE_CATEGORY( atlTraceUtil )		
DECLARE_TRACE_CATEGORY( atlTraceSecurity )
DECLARE_TRACE_CATEGORY( atlTraceSync )
DECLARE_TRACE_CATEGORY( atlTraceISAPI )		

// atlTraceUser categories are no longer needed.  Just declare your own trace category using CTraceCategory.
DECLARE_TRACE_CATEGORY( atlTraceUser )
DECLARE_TRACE_CATEGORY( atlTraceUser2 )
DECLARE_TRACE_CATEGORY( atlTraceUser3 )
DECLARE_TRACE_CATEGORY( atlTraceUser4 )

#pragma deprecated( atlTraceUser )
#pragma deprecated( atlTraceUser2 )
#pragma deprecated( atlTraceUser3 )
#pragma deprecated( atlTraceUser4 )

#ifdef _DEBUG

#ifndef _ATL_NO_DEBUG_CRT
class CNoUIAssertHook
{
public:
	CNoUIAssertHook()
	{
		ATLASSERT( s_pfnPrevHook == NULL );
		s_pfnPrevHook = _CrtSetReportHook(CrtHookProc);
	}
	~CNoUIAssertHook()
	{
		_CrtSetReportHook(s_pfnPrevHook);
		s_pfnPrevHook = NULL;
	}

private:
	static int __cdecl CrtHookProc(__in int eReportType, __in_z char* pszMessage, __out int* pnRetVal)
	{
		
		if (eReportType == _CRT_ASSERT)
		{
			::OutputDebugStringA( "ASSERTION FAILED\n" );
			::OutputDebugStringA( pszMessage );
			//If caller doesn't want retVal, so be it.
			if (pnRetVal != NULL)
			{
				*pnRetVal = 1;
			}
			return TRUE;
		}

		if (s_pfnPrevHook != NULL)
		{
			return s_pfnPrevHook(eReportType, pszMessage, pnRetVal);
		}
		else
		{
			return FALSE;
		}
	}

private:
	static _CRT_REPORT_HOOK s_pfnPrevHook;
};

__declspec( selectany ) _CRT_REPORT_HOOK CNoUIAssertHook::s_pfnPrevHook = NULL;

#define DECLARE_NOUIASSERT() ATL::CNoUIAssertHook _g_NoUIAssertHook;

#endif  // _ATL_NO_DEBUG_CRT

#ifndef ATLTRACE
#define ATLTRACE ATL::CTraceFileAndLineInfo(__FILE__, __LINE__)
#define ATLTRACE2 ATLTRACE
#endif

#pragma warning(push)
#pragma warning(disable : 4793)
inline void __cdecl AtlTrace(LPCSTR pszFormat, ...)
{
	va_list ptr;
	va_start(ptr, pszFormat);
	ATL::CTrace::s_trace.TraceV(NULL, -1, atlTraceGeneral, 0, pszFormat, ptr);
	va_end(ptr);
}
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4793)
inline void __cdecl AtlTrace(LPCWSTR pszFormat, ...)
{
	va_list ptr;
	va_start(ptr, pszFormat);
	ATL::CTrace::s_trace.TraceV(NULL, -1, atlTraceGeneral, 0, pszFormat, ptr);
	va_end(ptr);
}
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4793)
inline void __cdecl AtlTrace2(DWORD_PTR dwCategory, UINT nLevel, LPCSTR pszFormat, ...)
{
	va_list ptr;
	va_start(ptr, pszFormat);
	ATL::CTrace::s_trace.TraceV(NULL, -1, dwCategory, nLevel, pszFormat, ptr);
	va_end(ptr);
}
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4793)
inline void __cdecl AtlTrace2(DWORD_PTR dwCategory, UINT nLevel, LPCWSTR pszFormat, ...)
{
	va_list ptr;
	va_start(ptr, pszFormat);
	ATL::CTrace::s_trace.TraceV(NULL, -1, dwCategory, nLevel, pszFormat, ptr);
	va_end(ptr);
}
#pragma warning(pop)

#define ATLTRACENOTIMPL(funcname)  do { ATLTRACE(ATL::atlTraceNotImpl, 0, _T("ATL: %s not implemented.\n"), funcname); return E_NOTIMPL; } while(0)

#else // !DEBUG

#pragma warning(push)
#pragma warning(disable : 4793)
inline void __cdecl AtlTraceNull(...){}
inline void __cdecl AtlTrace(LPCSTR , ...){}
inline void __cdecl AtlTrace2(DWORD_PTR, UINT, LPCSTR , ...){}
inline void __cdecl AtlTrace(LPCWSTR , ...){}
inline void __cdecl AtlTrace2(DWORD_PTR, UINT, LPCWSTR , ...){}
#pragma warning(pop)

#ifndef ATLTRACE

#define ATLTRACE            __noop
#define ATLTRACE2           __noop
#endif //ATLTRACE
#define ATLTRACENOTIMPL(funcname)   return E_NOTIMPL
#define DECLARE_NOUIASSERT()

#endif //!_DEBUG

};  // namespace ATL
#pragma pack(pop)

#endif  // __ATLTRACE_H__
