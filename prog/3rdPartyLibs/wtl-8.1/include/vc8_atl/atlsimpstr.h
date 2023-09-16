// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLSIMPSTR_H__
#define __ATLSIMPSTR_H__

#pragma once

#include <atldef.h>
#include <atlcore.h>
#include <atlexcept.h>
#include <atlmem.h>

extern "C"
{
	LONG __cdecl _InterlockedIncrement( LONG volatile * pn );
	LONG __cdecl _InterlockedDecrement( LONG volatile * pn );
};

#pragma intrinsic( _InterlockedIncrement )
#pragma intrinsic( _InterlockedDecrement )

#pragma pack(push,_ATL_PACKING)
namespace ATL
{

struct CStringData;

__interface IAtlStringMgr
{
public:
	// Allocate a new CStringData
	CStringData* Allocate( int nAllocLength, int nCharSize ) throw();
	// Free an existing CStringData
	void Free( CStringData* pData ) throw();
	// Change the size of an existing CStringData
	CStringData* Reallocate( CStringData* pData, int nAllocLength, int nCharSize ) throw();
	// Get the CStringData for a Nil string
	CStringData* GetNilString() throw();
	IAtlStringMgr* Clone() throw();
};

#ifdef _M_IX86
#ifndef _M_CEE
#define _AtlInterlockedIncrement _InterlockedIncrement
#define _AtlInterlockedDecrement _InterlockedDecrement
#else
#define _AtlInterlockedIncrement InterlockedIncrement
#define _AtlInterlockedDecrement InterlockedDecrement
/* managed code must use the non-intrinsics */
#ifdef InterlockedIncrement
#undef InterlockedIncrement
#endif
#ifdef InterlockedDecrement
#undef InterlockedDecrement
#endif
#endif  // !_M_CEE
#else
#define _AtlInterlockedIncrement InterlockedIncrement
#define _AtlInterlockedDecrement InterlockedDecrement
#endif  // _M_IX86_

struct CStringData
{
	IAtlStringMgr* pStringMgr;  // String manager for this CStringData
	int nDataLength;  // Length of currently used data in XCHARs (not including terminating null)
	int nAllocLength;  // Length of allocated data in XCHARs (not including terminating null)
	long nRefs;     // Reference count: negative == locked
	// XCHAR data[nAllocLength+1]  // A CStringData is always followed in memory by the actual array of character data

	void* data() throw()
	{
		return (this+1);
	}

	void AddRef() throw()
	{
		ATLASSERT(nRefs > 0);
		_AtlInterlockedIncrement(&nRefs);
	}
	bool IsLocked() const throw()
	{
		return nRefs < 0;
	}
	bool IsShared() const throw()
	{
		return( nRefs > 1 ); 
	}
	void Lock() throw()
	{
		ATLASSERT( nRefs <= 1 );
		nRefs--;  // Locked buffers can't be shared, so no interlocked operation necessary
		if( nRefs == 0 )
		{
			nRefs = -1;
		}
	}
	void Release() throw()
	{
		ATLASSERT( nRefs != 0 );

		if( _AtlInterlockedDecrement( &nRefs ) <= 0 )
		{
			pStringMgr->Free( this );
		}
	}
	void Unlock() throw()
	{
		ATLASSERT( IsLocked() );

		if(IsLocked())
		{
			nRefs++;  // Locked buffers can't be shared, so no interlocked operation necessary
			if( nRefs == 0 )
			{
				nRefs = 1;
			}
		}
	}
};

class CNilStringData :
	public CStringData
{
public:
	CNilStringData() throw()
	{
		pStringMgr = NULL;
		nRefs = 2;  // Never gets freed by IAtlStringMgr
		nDataLength = 0;
		nAllocLength = 0;
		achNil[0] = 0;
		achNil[1] = 0;
	}

	void SetManager( __in IAtlStringMgr* pMgr ) throw()
	{
		ATLASSERT( pStringMgr == NULL );
		pStringMgr = pMgr;
	}

public:
	wchar_t achNil[2];
};

template< typename BaseType, const int t_nSize >
class CStaticString
{
public:
	CStaticString( __in const BaseType* psz ) :
		m_psz( psz )
	{
	}

	operator const BaseType*() const
	{
		return m_psz;
	}

	static int __cdecl GetLength() 
	{
		return (t_nSize/sizeof( BaseType ))-1;
	}

private:
	const BaseType* m_psz;

private:
	CStaticString( const CStaticString& str ) throw();
	CStaticString& operator=( const CStaticString& str ) throw();
};

#define _ST( psz ) ATL::CStaticString< TCHAR, sizeof( _T( psz ) ) >( _T( psz ) )
#define _SA( psz ) ATL::CStaticString< char, sizeof( psz ) >( psz )
#define _SW( psz ) ATL::CStaticString< wchar_t, sizeof( L##psz ) >( L##psz )
#define _SO( psz ) _SW( psz )

template< typename BaseType = char >
class ChTraitsBase
{
public:
	typedef char XCHAR;
	typedef LPSTR PXSTR;
	typedef LPCSTR PCXSTR;
	typedef wchar_t YCHAR;
	typedef LPWSTR PYSTR;
	typedef LPCWSTR PCYSTR;
};

template<>
class ChTraitsBase< wchar_t >
{
public:
	typedef wchar_t XCHAR;
	typedef LPWSTR PXSTR;
	typedef LPCWSTR PCXSTR;
	typedef char YCHAR;
	typedef LPSTR PYSTR;
	typedef LPCSTR PCYSTR;
};

template< typename TCharType, bool t_bMFCDLL = false >
class CStrBufT;

template< typename BaseType , bool t_bMFCDLL = false>
class CSimpleStringT
{
public:
	typedef typename ChTraitsBase< BaseType >::XCHAR XCHAR;
	typedef typename ChTraitsBase< BaseType >::PXSTR PXSTR;
	typedef typename ChTraitsBase< BaseType >::PCXSTR PCXSTR;
	typedef typename ChTraitsBase< BaseType >::YCHAR YCHAR;
	typedef typename ChTraitsBase< BaseType >::PYSTR PYSTR;
	typedef typename ChTraitsBase< BaseType >::PCYSTR PCYSTR;

public:
	explicit CSimpleStringT( __in IAtlStringMgr* pStringMgr )
	{
		ATLENSURE( pStringMgr != NULL );
		CStringData* pData = pStringMgr->GetNilString();
		Attach( pData );
	}
	
	CSimpleStringT( __in const CSimpleStringT& strSrc )
	{
		CStringData* pSrcData = strSrc.GetData();
		CStringData* pNewData = CloneData( pSrcData );
		Attach( pNewData );
	}
	
	CSimpleStringT( __in const CSimpleStringT<BaseType, !t_bMFCDLL>& strSrc )
	{
		CStringData* pSrcData = strSrc.GetData();
		CStringData* pNewData = CloneData( pSrcData );
		Attach( pNewData );
	}
	
	CSimpleStringT( __in PCXSTR pszSrc, __in IAtlStringMgr* pStringMgr )
	{
		ATLENSURE( pStringMgr != NULL );

		int nLength = StringLength( pszSrc );
		CStringData* pData = pStringMgr->Allocate( nLength, sizeof( XCHAR ) );
		if( pData == NULL )
		{
			ThrowMemoryException();
		}
		Attach( pData );
		SetLength( nLength );
#if _SECURE_ATL
        CopyChars( m_pszData, nLength, pszSrc, nLength );
#else
        CopyChars( m_pszData, pszSrc, nLength );
#endif
	}
	CSimpleStringT( __in_ecount(nLength) const XCHAR* pchSrc, __in int nLength, __in IAtlStringMgr* pStringMgr )
	{
		ATLENSURE( pStringMgr != NULL );
		
		if(pchSrc == NULL && nLength != 0)
			AtlThrow(E_INVALIDARG);

		CStringData* pData = pStringMgr->Allocate( nLength, sizeof( XCHAR ) );
		if( pData == NULL )
		{
			ThrowMemoryException();
		}
		Attach( pData );
		SetLength( nLength );
#if _SECURE_ATL
        CopyChars( m_pszData, nLength, pchSrc, nLength );
#else
		CopyChars( m_pszData, pchSrc, nLength );
#endif
	}
	~CSimpleStringT() throw()
	{
		CStringData* pData = GetData();
		pData->Release();
	}
	
	operator CSimpleStringT<BaseType, !t_bMFCDLL>&()
	{
		return *(CSimpleStringT<BaseType, !t_bMFCDLL>*)this;
	}

	CSimpleStringT& operator=( __in const CSimpleStringT& strSrc )
	{
		CStringData* pSrcData = strSrc.GetData();
		CStringData* pOldData = GetData();
		if( pSrcData != pOldData)
		{
			if( pOldData->IsLocked() || pSrcData->pStringMgr != pOldData->pStringMgr )
			{
				SetString( strSrc.GetString(), strSrc.GetLength() );
			}
			else
			{
				CStringData* pNewData = CloneData( pSrcData );
				pOldData->Release();
				Attach( pNewData );
			}
		}

		return( *this );
	}
	
	CSimpleStringT& operator=( __in const CSimpleStringT<BaseType, !t_bMFCDLL>& strSrc )
	{
		CStringData* pSrcData = strSrc.GetData();
		CStringData* pOldData = GetData();
		if( pSrcData != pOldData)
		{
			if( pOldData->IsLocked() || pSrcData->pStringMgr != pOldData->pStringMgr )
			{
				SetString( strSrc.GetString(), strSrc.GetLength() );
			}
			else
			{
				CStringData* pNewData = CloneData( pSrcData );
				pOldData->Release();
				Attach( pNewData );
			}
		}

		return( *this );
	}
	
	CSimpleStringT& operator=( __in PCXSTR pszSrc )
	{
		SetString( pszSrc );

		return( *this );
	}

	CSimpleStringT& operator+=( __in const CSimpleStringT& strSrc )
	{
		Append( strSrc );

		return( *this );
	}
	template <bool bMFCDLL>
	CSimpleStringT& operator+=( __in const CSimpleStringT<BaseType, bMFCDLL>& strSrc )
	{
		Append( strSrc );

		return( *this );
	}
	
	CSimpleStringT& operator+=( __in PCXSTR pszSrc )
	{
		Append( pszSrc );

		return( *this );
	}
	template< int t_nSize >
	CSimpleStringT& operator+=( __in const CStaticString< XCHAR, t_nSize >& strSrc )
	{
		Append( static_cast<const XCHAR *>(strSrc), strSrc.GetLength() );

		return( *this );
	}
	CSimpleStringT& operator+=( __in char ch )
	{
		AppendChar( XCHAR( ch ) );

		return( *this );
	}
	CSimpleStringT& operator+=( __in unsigned char ch )
	{
		AppendChar( XCHAR( ch ) );

		return( *this );
	}
	CSimpleStringT& operator+=( __in wchar_t ch )
	{
		AppendChar( XCHAR( ch ) );

		return( *this );
	}

	XCHAR operator[]( __in int iChar ) const
	{
		ATLASSERT( (iChar >= 0) && (iChar <= GetLength()) );  // Indexing the '\0' is OK
		
		if( (iChar < 0) || (iChar > GetLength()) )
			AtlThrow(E_INVALIDARG);
			
		return( m_pszData[iChar] );
	}

	operator PCXSTR() const throw()
	{
		return( m_pszData );
	}

	void Append( __in PCXSTR pszSrc )
	{
		Append( pszSrc, StringLength( pszSrc ) );
	}
	void Append( __in_ecount(nLength) PCXSTR pszSrc, __in int nLength )
	{
		// See comment in SetString() about why we do this
		UINT_PTR nOffset = pszSrc-GetString();

		UINT nOldLength = GetLength();
		if (nOldLength < 0)
		{
			// protects from underflow
			nOldLength = 0;
		}
		int nNewLength = nOldLength+nLength;
		PXSTR pszBuffer = GetBuffer( nNewLength );
		if( nOffset <= nOldLength )
		{
			pszSrc = pszBuffer+nOffset;
			// No need to call CopyCharsOverlapped, since the destination is
			// beyond the end of the original buffer
		}
#if _SECURE_ATL
        CopyChars( pszBuffer+nOldLength, nLength, pszSrc, nLength );
#else
        CopyChars( pszBuffer+nOldLength, pszSrc, nLength );
#endif
		ReleaseBufferSetLength( nNewLength );
	}
	void AppendChar( __in XCHAR ch )
	{
		UINT nOldLength = GetLength();
		int nNewLength = nOldLength+1;
		PXSTR pszBuffer = GetBuffer( nNewLength );
		pszBuffer[nOldLength] = ch;
		ReleaseBufferSetLength( nNewLength );
	}
	void Append( __in const CSimpleStringT& strSrc )
	{
		Append( strSrc.GetString(), strSrc.GetLength() );
	}
	template <bool bMFCDLL>
	void Append( __in const CSimpleStringT<BaseType, bMFCDLL>& strSrc )
	{
		Append( strSrc.GetString(), strSrc.GetLength() );
	}	
	void Empty() throw()
	{
		CStringData* pOldData = GetData();
		IAtlStringMgr* pStringMgr = pOldData->pStringMgr;
		if( pOldData->nDataLength == 0 )
		{
			return;
		}

		if( pOldData->IsLocked() )
		{
			// Don't reallocate a locked buffer that's shrinking
			SetLength( 0 );
		}
		else
		{
			pOldData->Release();
			CStringData* pNewData = pStringMgr->GetNilString();
			Attach( pNewData );
		}
	}
	void FreeExtra()
	{
		CStringData* pOldData = GetData();
		int nLength = pOldData->nDataLength;
		IAtlStringMgr* pStringMgr = pOldData->pStringMgr;
		if( pOldData->nAllocLength == nLength )
		{
			return;
		}

		if( !pOldData->IsLocked() )  // Don't reallocate a locked buffer that's shrinking
		{
			CStringData* pNewData = pStringMgr->Allocate( nLength, sizeof( XCHAR ) );
			if( pNewData == NULL )
			{
				SetLength( nLength );
				return;
			}
			
#if _SECURE_ATL
            CopyChars( PXSTR( pNewData->data() ), nLength, 
				PCXSTR( pOldData->data() ), nLength );
#else
			CopyChars( PXSTR( pNewData->data() ), PCXSTR( pOldData->data() ), nLength );
#endif

			pOldData->Release();
			Attach( pNewData );
			SetLength( nLength );
		}
	}

	int GetAllocLength() const throw()
	{
		return( GetData()->nAllocLength );
	}
	XCHAR GetAt( __in int iChar ) const
	{
		ATLASSERT( (iChar >= 0) && (iChar <= GetLength()) );  // Indexing the '\0' is OK
		if( (iChar < 0) || (iChar > GetLength()) )
			AtlThrow(E_INVALIDARG);		
			
		return( m_pszData[iChar] );
	}
	PXSTR GetBuffer()
	{
		CStringData* pData = GetData();
		if( pData->IsShared() )
		{
			Fork( pData->nDataLength );
		}

		return( m_pszData );
	}
	__out_ecount(nMinBufferLength + 1) PXSTR GetBuffer( __in int nMinBufferLength )
	{
		return( PrepareWrite( nMinBufferLength ) );
	}
	__out_ecount(nLength + 1) PXSTR GetBufferSetLength( __in int nLength )
	{
		PXSTR pszBuffer = GetBuffer( nLength );
		SetLength( nLength );

		return( pszBuffer );
	}
	int GetLength() const throw()
	{
		return( GetData()->nDataLength );
	}
	IAtlStringMgr* GetManager() const throw()
	{
		IAtlStringMgr* pStringMgr = GetData()->pStringMgr;
		return pStringMgr ? pStringMgr->Clone() : NULL;
	}

	__out_ecount(m_nLength) PCXSTR GetString() const throw()
	{
		return( m_pszData );
	}
	bool IsEmpty() const throw()
	{
		return( GetLength() == 0 );
	}
	PXSTR LockBuffer()
	{
		CStringData* pData = GetData();
		if( pData->IsShared() )
		{
			Fork( pData->nDataLength );
			pData = GetData();  // Do it again, because the fork might have changed it
		}
		pData->Lock();

		return( m_pszData );
	}
	void UnlockBuffer() throw()
	{
		CStringData* pData = GetData();
		pData->Unlock();
	}
	void Preallocate( __in int nLength )
	{
		PrepareWrite( nLength );
	}
	void ReleaseBuffer( __in int nNewLength = -1 )
	{
		if( nNewLength == -1 )
		{
			nNewLength = StringLength( m_pszData );
		}
		SetLength( nNewLength );
	}
	void ReleaseBufferSetLength( __in int nNewLength )
	{
		ATLASSERT( nNewLength >= 0 );
		SetLength( nNewLength );
	}
	void Truncate( __in int nNewLength )
	{
		ATLASSERT( nNewLength <= GetLength() );
		GetBuffer( nNewLength );
		ReleaseBufferSetLength( nNewLength );
	}
	void SetAt( __in int iChar, __in XCHAR ch )
	{
		ATLASSERT( (iChar >= 0) && (iChar < GetLength()) );

		if( (iChar < 0) || (iChar >= GetLength()) )
			AtlThrow(E_INVALIDARG);		
			
		int nLength = GetLength();
		PXSTR pszBuffer = GetBuffer();
		pszBuffer[iChar] = ch;
		ReleaseBufferSetLength( nLength );
			
	}
	void SetManager( __in IAtlStringMgr* pStringMgr )
	{
		ATLASSERT( IsEmpty() );

		CStringData* pData = GetData();
		pData->Release();
		pData = pStringMgr->GetNilString();
		Attach( pData );
	}
	void SetString( __in PCXSTR pszSrc )
	{
		SetString( pszSrc, StringLength( pszSrc ) );
	}
	void SetString( __in_ecount(nLength) PCXSTR pszSrc, __in int nLength )
	{
		if( nLength == 0 )
		{
			Empty();
		}
		else
		{
			// It is possible that pszSrc points to a location inside of our 
			// buffer.  GetBuffer() might change m_pszData if (1) the buffer 
			// is shared or (2) the buffer is too small to hold the new 
			// string.  We detect this aliasing, and modify pszSrc to point
			// into the newly allocated buffer instead.
			
			if(pszSrc == NULL)
				AtlThrow(E_INVALIDARG);			
				
			UINT nOldLength = GetLength();
			UINT_PTR nOffset = pszSrc-GetString();
			// If 0 <= nOffset <= nOldLength, then pszSrc points into our 
			// buffer

			PXSTR pszBuffer = GetBuffer( nLength );
			if( nOffset <= nOldLength )
			{
#if _SECURE_ATL
                CopyCharsOverlapped( pszBuffer, nLength, 
					pszBuffer+nOffset, nLength );
#else
				CopyCharsOverlapped( pszBuffer, pszBuffer+nOffset, nLength );
#endif
			}
			else
			{
#if _SECURE_ATL
				CopyChars( pszBuffer, nLength, pszSrc, nLength );
#else
				CopyChars( pszBuffer, pszSrc, nLength );
#endif
			}
			ReleaseBufferSetLength( nLength );
		}
	}

public:
	friend CSimpleStringT operator+(
		__in const CSimpleStringT& str1,
		__in const CSimpleStringT& str2 )
	{
		CSimpleStringT s( str1.GetManager() );

		Concatenate( s, str1, str1.GetLength(), str2, str2.GetLength() );

		return( s );
	}

	friend CSimpleStringT operator+(
		__in const CSimpleStringT& str1,
		__in PCXSTR psz2 )
	{
		CSimpleStringT s( str1.GetManager() );

		Concatenate( s, str1, str1.GetLength(), psz2, StringLength( psz2 ) );

		return( s );
	}

	friend CSimpleStringT operator+(
		__in PCXSTR psz1,
		__in const CSimpleStringT& str2 )
	{
		CSimpleStringT s( str2.GetManager() );

		Concatenate( s, psz1, StringLength( psz1 ), str2, str2.GetLength() );

		return( s );
	}

	_ATL_INSECURE_DEPRECATE("CSimpleStringT::CopyChars must be passed a buffer size")
	static void __cdecl CopyChars
		( __out_ecount(nChars) XCHAR* pchDest, __in_ecount(nChars) const XCHAR* pchSrc, __in int nChars ) throw()
	{
		#pragma warning (push)
		#pragma warning(disable : 4996)
		memcpy( pchDest, pchSrc, nChars*sizeof( XCHAR ) );
		#pragma warning (pop)
	}
	static void __cdecl CopyChars
		( __out_ecount_part(nDestLen,nChars) XCHAR* pchDest, __in size_t nDestLen, __in_ecount(nChars) const XCHAR* pchSrc, __in int nChars ) throw()
	{
		memcpy_s( pchDest, nDestLen*sizeof( XCHAR ), 
			pchSrc, nChars*sizeof( XCHAR ) );
	}

	_ATL_INSECURE_DEPRECATE("CSimpleStringT::CopyCharsOverlapped must be passed a buffer size")
	static void __cdecl CopyCharsOverlapped
		( __out_ecount(nChars) XCHAR* pchDest, __in_ecount(nChars) const XCHAR* pchSrc, __in int nChars ) throw()
	{
		#pragma warning (push)
		#pragma warning(disable : 4996)
		memmove( pchDest, pchSrc, nChars*sizeof( XCHAR ) );
		#pragma warning (pop)
	}
	static void __cdecl CopyCharsOverlapped
		( __out_ecount_part(nDestLen, nDestLen) XCHAR* pchDest, __in size_t nDestLen, __in_ecount(nChars) const XCHAR* pchSrc, __in int nChars ) throw()
	{
		memmove_s( pchDest, nDestLen*sizeof( XCHAR ), 
			pchSrc, nChars*sizeof( XCHAR ) );
	}
#ifdef _ATL_MIN_CRT
	ATL_NOINLINE static int __cdecl StringLength( __in_z_opt PCXSTR psz ) throw()
	{
		int nLength = 0;
		if( psz != NULL )
		{
			const XCHAR* pch = psz;
			while( *pch != 0 )
			{
				nLength++;
				pch++;
			}
		}

		return( nLength );
	}
#else
	static int __cdecl StringLength( __in_z_opt const char* psz ) throw()
	{
		if( psz == NULL )
		{
			return( 0 );
		}
		return( int( strlen( psz ) ) );
	}
	static int __cdecl StringLength( __in_z_opt const wchar_t* psz ) throw()
	{
		if( psz == NULL )
		{
			return( 0 );
		}
		return( int( wcslen( psz ) ) );
	}
#endif

protected:
	static void __cdecl Concatenate( __out CSimpleStringT& strResult, __in_ecount(nLength1) PCXSTR psz1, __in int nLength1, __in_ecount(nLength2) PCXSTR psz2, __in int nLength2 )
	{
		int nNewLength = nLength1+nLength2;
		PXSTR pszBuffer = strResult.GetBuffer( nNewLength );
#if _SECURE_ATL
        CopyChars( pszBuffer, nLength1, psz1, nLength1 );
		CopyChars( pszBuffer+nLength1, nLength2, psz2, nLength2 );
#else
        CopyChars( pszBuffer, psz1, nLength1 );
		CopyChars( pszBuffer+nLength1, psz2, nLength2 );
#endif
		strResult.ReleaseBufferSetLength( nNewLength );
	}
	ATL_NOINLINE __declspec( noreturn ) static void __cdecl ThrowMemoryException()
	{
		AtlThrow( E_OUTOFMEMORY );
	}

// Implementation
private:
	void Attach( __in CStringData* pData ) throw()
	{
		m_pszData = static_cast< PXSTR >( pData->data() );
	}
	ATL_NOINLINE void Fork( __in int nLength )
	{
		CStringData* pOldData = GetData();
		int nOldLength = pOldData->nDataLength;
		CStringData* pNewData = pOldData->pStringMgr->Clone()->Allocate( nLength, sizeof( XCHAR ) );
		if( pNewData == NULL )
		{
			ThrowMemoryException();
		}
		int nCharsToCopy = ((nOldLength < nLength) ? nOldLength : nLength)+1;  // Copy '\0'
#if _SECURE_ATL
        CopyChars( PXSTR( pNewData->data() ), nCharsToCopy, 
			PCXSTR( pOldData->data() ), nCharsToCopy );
#else
        CopyChars( PXSTR( pNewData->data() ), PCXSTR( pOldData->data() ), nCharsToCopy );
#endif
		pNewData->nDataLength = nOldLength;
		pOldData->Release();
		Attach( pNewData );
	}
	CStringData* GetData() const throw()
	{
		return( reinterpret_cast< CStringData* >( m_pszData )-1 );
	}
	PXSTR PrepareWrite( __in int nLength )
	{
		CStringData* pOldData = GetData();
		int nShared = 1-pOldData->nRefs;  // nShared < 0 means true, >= 0 means false
		int nTooShort = pOldData->nAllocLength-nLength;  // nTooShort < 0 means true, >= 0 means false
		if( (nShared|nTooShort) < 0 )  // If either sign bit is set (i.e. either is less than zero), we need to copy data
		{
			PrepareWrite2( nLength );
		}

		return( m_pszData );
	}
	ATL_NOINLINE void PrepareWrite2( __in int nLength )
	{
		CStringData* pOldData = GetData();
		if( pOldData->nDataLength > nLength )
		{
			nLength = pOldData->nDataLength;
		}
		if( pOldData->IsShared() )
		{
			Fork( nLength );
		}
		else if( pOldData->nAllocLength < nLength )
		{
			// Grow exponentially, until we hit 1K.
			int nNewLength = pOldData->nAllocLength;
			if( nNewLength > 1024 )
			{
				nNewLength += 1024;
			}
			else
			{
				nNewLength *= 2;
			}
			if( nNewLength < nLength )
			{
				nNewLength = nLength;
			}
			Reallocate( nNewLength );
		}
	}
	ATL_NOINLINE void Reallocate( __in int nLength )
	{
		CStringData* pOldData = GetData();
		ATLASSERT( pOldData->nAllocLength < nLength );
		IAtlStringMgr* pStringMgr = pOldData->pStringMgr;
                if ( pOldData->nAllocLength >= nLength || nLength <= 0)
                {
			ThrowMemoryException();
                        return;
                }
		CStringData* pNewData = pStringMgr->Reallocate( pOldData, nLength, sizeof( XCHAR ) );
		if( pNewData == NULL )
		{
			ThrowMemoryException();
		}
		Attach( pNewData );
	}

	void SetLength( __in int nLength )
	{
		ATLASSERT( nLength >= 0 );
		ATLASSERT( nLength <= GetData()->nAllocLength );

		if( nLength < 0 || nLength > GetData()->nAllocLength)
			AtlThrow(E_INVALIDARG);
			
		GetData()->nDataLength = nLength;
		m_pszData[nLength] = 0;
	}

	static CStringData* __cdecl CloneData( __in CStringData* pData )
	{
		CStringData* pNewData = NULL;

		IAtlStringMgr* pNewStringMgr = pData->pStringMgr->Clone();
		if( !pData->IsLocked() && (pNewStringMgr == pData->pStringMgr) )
		{
			pNewData = pData;
			pNewData->AddRef();
		}
		else
		{
			pNewData = pNewStringMgr->Allocate( pData->nDataLength, sizeof( XCHAR ) );
			if( pNewData == NULL )
			{
				ThrowMemoryException();
			}
			pNewData->nDataLength = pData->nDataLength;
#if _SECURE_ATL
            CopyChars( PXSTR( pNewData->data() ), pData->nDataLength+1,
				PCXSTR( pData->data() ), pData->nDataLength+1 );  // Copy '\0'
#else
			CopyChars( PXSTR( pNewData->data() ), PCXSTR( pData->data() ), pData->nDataLength+1 );  // Copy '\0'
#endif
		}

		return( pNewData );
	}

public :
	typedef CStrBufT<BaseType, t_bMFCDLL> CStrBuf;
private:
	PXSTR m_pszData;
	
	friend class CSimpleStringT<BaseType, !t_bMFCDLL>;
};

template< typename TCharType, bool t_bMFCDLL >
class CStrBufT
{
public:
	typedef CSimpleStringT< TCharType, t_bMFCDLL> StringType;
	typedef typename StringType::XCHAR XCHAR;
	typedef typename StringType::PXSTR PXSTR;
	typedef typename StringType::PCXSTR PCXSTR;

	static const DWORD AUTO_LENGTH = 0x01;  // Automatically determine the new length of the string at release.  The string must be null-terminated.
	static const DWORD SET_LENGTH = 0x02;  // Set the length of the string object at GetBuffer time

public:
	explicit CStrBufT( __in StringType& str ) throw( ... ) :
		m_str( str ),
		m_pszBuffer( NULL ),
#ifdef _DEBUG
		m_nBufferLength( str.GetLength() ),
#endif
		m_nLength( str.GetLength() )
	{
		m_pszBuffer = m_str.GetBuffer();
	}
	
	CStrBufT( __in StringType& str, __in int nMinLength, __in DWORD dwFlags = AUTO_LENGTH ) throw( ... ) :
		m_str( str ),
		m_pszBuffer( NULL ),
#ifdef _DEBUG
		m_nBufferLength( nMinLength ),
#endif
		m_nLength( (dwFlags&AUTO_LENGTH) ? -1 : nMinLength )
	{
		if( dwFlags&SET_LENGTH )
		{
			m_pszBuffer = m_str.GetBufferSetLength( nMinLength );
		}
		else
		{
			m_pszBuffer = m_str.GetBuffer( nMinLength );
		}
	}

	~CStrBufT()
	{
		m_str.ReleaseBuffer( m_nLength );
	}

	operator PXSTR() throw()
	{
		return( m_pszBuffer );
	}
	operator PCXSTR() const throw()
	{
		return( m_pszBuffer );
	}

	void SetLength( __in int nLength )
	{
		ATLASSERT( nLength >= 0 );		
		ATLASSERT( nLength <= m_nBufferLength );

		if( nLength < 0 )
			AtlThrow(E_INVALIDARG);
		
		m_nLength = nLength;
	}

// Implementation
private:
	StringType& m_str;
	PXSTR m_pszBuffer;
	int m_nLength;
#ifdef _DEBUG
	int m_nBufferLength;
#endif

// Private copy constructor and copy assignment operator to prevent accidental use
private:
	CStrBufT( const CStrBufT& ) throw();
	CStrBufT& operator=( const CStrBufT& ) throw();
};

typedef CSimpleStringT< TCHAR > CSimpleString;
typedef CSimpleStringT< char > CSimpleStringA;
typedef CSimpleStringT< wchar_t > CSimpleStringW;
typedef CStrBufT< TCHAR > CStrBuf;
typedef CStrBufT< char > CStrBufA;
typedef CStrBufT< wchar_t > CStrBufW;

};  // namespace ATL
#pragma pack(pop)

#endif  // __ATLSIMPSTR_H__
