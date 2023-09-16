// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#pragma once
#ifndef __ATLALLOC_H__
#define __ATLALLOC_H__
#endif

#include <windows.h>
#include <ole2.h>

#pragma pack(push,_ATL_PACKING)
namespace ATL
{

/* 
This is	more than a	little unsatisfying. /Wp64 warns when we convert a size_t to an	int
because	it knows such a	conversion won't port. 
But, when we have overloaded templates,	there may well exist both conversions and we need 
to fool	the	warning	into not firing	on 32 bit builds
*/
#if !defined(_ATL_W64)
#if !defined(__midl) &&	(defined(_X86_)	|| defined(_M_IX86))
#define	_ATL_W64 __w64
#else
#define	_ATL_W64
#endif
#endif

/* Can't use ::std::numeric_limits<T> here because we don't want to introduce a new	
   deprendency of this code on SCL
*/

template<typename T>
class AtlLimits;

template<>
class AtlLimits<int _ATL_W64>
{
public:
	static const int _Min=INT_MIN;
	static const int _Max=INT_MAX;
};

template<>
class AtlLimits<unsigned int _ATL_W64>
{
public:
	static const unsigned int _Min=0;
	static const unsigned int _Max=UINT_MAX;
};

template<>
class AtlLimits<long _ATL_W64>
{
public:
	static const long _Min=LONG_MIN;
	static const long _Max=LONG_MAX;
};

template<>
class AtlLimits<unsigned long _ATL_W64>
{
public:
	static const unsigned long _Min=0;
	static const unsigned long _Max=ULONG_MAX;
};

template<>
class AtlLimits<long long>
{
public:
	static const long long _Min=LLONG_MIN;
	static const long long _Max=LLONG_MAX;
};

template<>
class AtlLimits<unsigned long long>
{
public:
	static const unsigned long long _Min=0;
	static const unsigned long long _Max=ULLONG_MAX;
};

/* generic version */
template<typename T>
inline HRESULT AtlAdd(T* ptResult, T tLeft, T tRight)
{
	if(::ATL::AtlLimits<T>::_Max-tLeft < tRight)
	{
		return E_INVALIDARG;
	}
	*ptResult= tLeft + tRight;
	return S_OK;
}

/* generic but compariatively slow version */
template<typename T>
inline HRESULT AtlMultiply(T* ptResult,	T tLeft, T tRight)
{
	/* avoid divide 0 */
	if(tLeft==0)
	{
		*ptResult=0;
		return S_OK;
	}
	if(::ATL::AtlLimits<T>::_Max/tLeft < tRight)
	{
		return E_INVALIDARG;
	}
	*ptResult= tLeft * tRight;
	return S_OK;
}

/* fast	version	for	32 bit integers	*/
template<>
inline HRESULT AtlMultiply(int _ATL_W64	*piResult, int _ATL_W64	iLeft, int _ATL_W64 iRight)
{
	__int64 i64Result=static_cast<__int64>(iLeft) * static_cast<__int64>(iRight);
	if(i64Result>INT_MAX || i64Result < INT_MIN)
	{
		return E_INVALIDARG;
	}
	*piResult=static_cast<int _ATL_W64>(i64Result);
	return S_OK;
}

template<>
inline HRESULT AtlMultiply(unsigned int	_ATL_W64 *piResult, unsigned int _ATL_W64 iLeft, unsigned int _ATL_W64 iRight)
{
	unsigned __int64 i64Result=static_cast<unsigned __int64>(iLeft) * static_cast<unsigned __int64>(iRight);
	if(i64Result>UINT_MAX)
	{
		return E_INVALIDARG;
	}
	*piResult=static_cast<unsigned int _ATL_W64>(i64Result);
	return S_OK;
}

template<>
inline HRESULT AtlMultiply(long	_ATL_W64 *piResult, long _ATL_W64 iLeft, long _ATL_W64 iRight)
{
	__int64 i64Result=static_cast<__int64>(iLeft) * static_cast<__int64>(iRight);
	if(i64Result>LONG_MAX || i64Result < LONG_MIN)
	{
		return E_INVALIDARG;
	}
	*piResult=static_cast<long _ATL_W64>(i64Result);
	return S_OK;
}

template<>
inline HRESULT AtlMultiply(unsigned long _ATL_W64 *piResult, unsigned long _ATL_W64 iLeft, unsigned long _ATL_W64 iRight)
{
	unsigned __int64 i64Result=static_cast<unsigned __int64>(iLeft) * static_cast<unsigned __int64>(iRight);
	if(i64Result>ULONG_MAX)
	{
		return E_INVALIDARG;
	}
	*piResult=static_cast<unsigned long _ATL_W64>(i64Result);
	return S_OK;
}

template <typename T>
inline T AtlMultiplyThrow(T tLeft, T tRight)
{
	T tResult;
	HRESULT hr=AtlMultiply(&tResult, tLeft, tRight);
	if(FAILED(hr))
	{
		AtlThrow(hr);
	}
	return tResult;
}

template <typename T>
inline T AtlAddThrow(T tLeft, T	tRight)
{
	T tResult;
	HRESULT hr=AtlAdd(&tResult, tLeft, tRight);
	if(FAILED(hr))
	{
		AtlThrow(hr);
	}
	return tResult;
}

inline LPVOID AtlCoTaskMemCAlloc(ULONG nCount, ULONG nSize)
{
	HRESULT hr;
	ULONG nBytes=0;
	if( FAILED(hr=::ATL::AtlMultiply(&nBytes, nCount, nSize)))
	{
		return NULL;
	}
	return ::CoTaskMemAlloc(nBytes);
}

inline LPVOID AtlCoTaskMemRecalloc(void	*pvMemory, ULONG nCount, ULONG nSize)
{
	HRESULT hr;
	ULONG nBytes=0;
	if( FAILED(hr=::ATL::AtlMultiply(&nBytes, nCount, nSize)))
	{
		return NULL;
	}
	return ::CoTaskMemRealloc(pvMemory, nBytes);
}

}	// namespace ATL
#pragma pack(pop)

#pragma pack(push,8)
namespace ATL
{
// forward declaration of Checked::memcpy_s

namespace Checked
{
    void __cdecl memcpy_s(void *s1, size_t s1max, const void *s2, size_t n);
}

/////////////////////////////////////////////////////////////////////////////
// Allocation helpers

class CCRTAllocator 
{
public:
	static void* Reallocate(void* p, size_t nBytes) throw()
	{
		return realloc(p, nBytes);
	}

	static void* Allocate(size_t nBytes) throw()
	{
		return malloc(nBytes);
	}

	static void Free(void* p) throw()
	{
		free(p);
	}
};

class CLocalAllocator
{
public:
	static void* Allocate(size_t nBytes) throw()
	{
		return ::LocalAlloc(LMEM_FIXED, nBytes);
	}
	static void* Reallocate(void* p, size_t nBytes) throw()
	{
		if (p==NULL){
			return ( Allocate(nBytes) );
		
		}
		if (nBytes==0){
			Free(p);
			return NULL;
		}
			return ::LocalReAlloc(p, nBytes, 0);
		 
	}
	static void Free(void* p) throw()
	{
		::LocalFree(p);
	}
};

class CGlobalAllocator
{
public:
	static void* Allocate(size_t nBytes) throw()
	{
		return ::GlobalAlloc(GMEM_FIXED, nBytes);
	}
	static void* Reallocate(void* p, size_t nBytes) throw()
	{
		if (p==NULL){
			return ( Allocate(nBytes) );
		
		}
		if (nBytes==0){
			Free(p);
			return NULL;
		}
		return ( ::GlobalReAlloc(p, nBytes, 0) );
	}
	static void Free(void* p) throw()
	{
		::GlobalFree(p);
	}
};

template <class T, class Allocator = CCRTAllocator>
class CHeapPtrBase
{
protected:
	CHeapPtrBase() throw() :
		m_pData(NULL)
	{
	}
	CHeapPtrBase(CHeapPtrBase<T, Allocator>& p) throw()
	{
		m_pData = p.Detach();  // Transfer ownership
	}
	explicit CHeapPtrBase(T* pData) throw() :
		m_pData(pData)
	{
	}

public:
	~CHeapPtrBase() throw()
	{
		Free();
	}

protected:
	CHeapPtrBase<T, Allocator>& operator=(CHeapPtrBase<T, Allocator>& p) throw()
	{
		if(m_pData != p.m_pData)
			Attach(p.Detach());  // Transfer ownership
		return *this;
	}

public:
	operator T*() const throw()
	{
		return m_pData;
	}

	T* operator->() const throw()
	{
		ATLASSERT(m_pData != NULL);
		return m_pData;
	}

	T** operator&() throw()
	{
		ATLASSUME(m_pData == NULL);
		return &m_pData;
	}

	// Allocate a buffer with the given number of bytes
	bool AllocateBytes(size_t nBytes) throw()
	{
		ATLASSERT(m_pData == NULL);
		m_pData = static_cast<T*>(Allocator::Allocate(nBytes));
		if (m_pData == NULL)
			return false;

		return true;
	}

	// Attach to an existing pointer (takes ownership)
	void Attach(T* pData) throw()
	{
		Allocator::Free(m_pData);
		m_pData = pData;
	}

	// Detach the pointer (releases ownership)
	T* Detach() throw() 
	{
		T* pTemp = m_pData;
		m_pData = NULL;
		return pTemp;
	}

	// Free the memory pointed to, and set the pointer to NULL
	void Free() throw()
	{
		Allocator::Free(m_pData);
		m_pData = NULL;
	}

	// Reallocate the buffer to hold a given number of bytes
	bool ReallocateBytes(size_t nBytes) throw()
	{
		T* pNew;

		pNew = static_cast<T*>(Allocator::Reallocate(m_pData, nBytes));
		if (pNew == NULL)
			return false;
		m_pData = pNew;

		return true;
	}

public:
	T* m_pData;
};

template <typename T, class Allocator = CCRTAllocator>
class CHeapPtr :
	public CHeapPtrBase<T, Allocator>
{
public:
	CHeapPtr() throw()
	{
	}
	CHeapPtr(CHeapPtr<T, Allocator>& p) throw() :
		CHeapPtrBase<T, Allocator>(p)
	{
	}
	explicit CHeapPtr(T* p) throw() :
		CHeapPtrBase<T, Allocator>(p)
	{
	}

	CHeapPtr<T, Allocator>& operator=(CHeapPtr<T, Allocator>& p) throw()
	{
		CHeapPtrBase<T, Allocator>::operator=(p);

		return *this;
	}

	// Allocate a buffer with the given number of elements
	bool Allocate(size_t nElements = 1) throw()
	{
		size_t nBytes=0;
		if(FAILED(::ATL::AtlMultiply(&nBytes, nElements, sizeof(T))))
		{
			return false;
		}
		return AllocateBytes(nBytes);
	}

	// Reallocate the buffer to hold a given number of elements
	bool Reallocate(size_t nElements) throw()
	{
		size_t nBytes=0;
		if(FAILED(::ATL::AtlMultiply(&nBytes, nElements, sizeof(T))))
		{
			return false;
		}
		return ReallocateBytes(nBytes);
	}
};

template< typename T, int t_nFixedBytes = 128, class Allocator = CCRTAllocator >
class CTempBuffer
{
public:
	CTempBuffer() throw() :
		m_p( NULL )
	{
	}
	CTempBuffer( size_t nElements ) throw( ... ) :
		m_p( NULL )
	{
		Allocate( nElements );
	}

	~CTempBuffer() throw()
	{
		if( m_p != reinterpret_cast< T* >( m_abFixedBuffer ) )
		{
			FreeHeap();
		}
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

	T* Allocate( size_t nElements ) throw( ... )
	{
		return( AllocateBytes( ::ATL::AtlMultiplyThrow(nElements,sizeof( T )) ) );
	}

	T* Reallocate( size_t nElements ) throw( ... )
	{
		ATLENSURE(nElements < size_t(-1)/sizeof(T) );		
		size_t nNewSize = nElements*sizeof( T ) ;
				
		if (m_p == NULL)
			return AllocateBytes(nNewSize);

		if (nNewSize > t_nFixedBytes)
		{
			if( m_p == reinterpret_cast< T* >( m_abFixedBuffer ) )
			{
				// We have to allocate from the heap and copy the contents into the new buffer
				AllocateHeap(nNewSize);
				Checked::memcpy_s(m_p, nNewSize, m_abFixedBuffer, t_nFixedBytes);
			}
			else
			{
				ReAllocateHeap( nNewSize );
			}
		}
		else
		{
			m_p = reinterpret_cast< T* >( m_abFixedBuffer );
		}

		return m_p;
	}

	T* AllocateBytes( size_t nBytes )
	{
		ATLASSERT( m_p == NULL );
		if( nBytes > t_nFixedBytes )
		{
			AllocateHeap( nBytes );
		}
		else
		{
			m_p = reinterpret_cast< T* >( m_abFixedBuffer );
		}

		return( m_p );
	}

private:
	ATL_NOINLINE void AllocateHeap( size_t nBytes )
	{
		T* p = static_cast< T* >( Allocator::Allocate( nBytes ) );
		if( p == NULL )
		{
			AtlThrow( E_OUTOFMEMORY );
		}
		m_p = p;
	}
 
	ATL_NOINLINE void ReAllocateHeap( size_t nNewSize)
	{
		T* p = static_cast< T* >( Allocator::Reallocate(m_p, nNewSize) );
		if ( p == NULL )
		{
			AtlThrow( E_OUTOFMEMORY );
		}
		m_p = p;
	}

	ATL_NOINLINE void FreeHeap() throw()
	{
		Allocator::Free( m_p );
	}

private:
	T* m_p;
	BYTE m_abFixedBuffer[t_nFixedBytes];
};


// Allocating memory on the stack without causing stack overflow.
// Only use these through the _ATL_SAFE_ALLOCA_* macros
namespace _ATL_SAFE_ALLOCA_IMPL
{

#ifndef _ATL_STACK_MARGIN
#if defined(_M_IX86)
#define _ATL_STACK_MARGIN	0x2000	// Minimum stack available after call to _ATL_SAFE_ALLOCA
#else //_M_AMD64 _M_IA64
#define _ATL_STACK_MARGIN	0x4000
#endif
#endif //_ATL_STACK_MARGIN

//Verifies if sufficient space is available on the stack.
//Note: This function should never be inlined, because the stack allocation
//may not be freed until the end of the calling function (instead of the end of _AtlVerifyStackAvailable).
//The use of __try/__except preverts inlining in this case.
inline bool _AtlVerifyStackAvailable(SIZE_T Size)
{
    bool bStackAvailable = true;

    __try
    {
		SIZE_T size=0;
		HRESULT hrAdd=::ATL::AtlAdd(&size, Size, static_cast<SIZE_T>(_ATL_STACK_MARGIN));
		if(FAILED(hrAdd))
		{
			ATLASSERT(FALSE);
			bStackAvailable = false;
		}
		else
		{
			PVOID p = _alloca(size);
			(p);
		}
    }
    __except ((EXCEPTION_STACK_OVERFLOW == GetExceptionCode()) ?
                   EXCEPTION_EXECUTE_HANDLER :
                   EXCEPTION_CONTINUE_SEARCH)
    {
        bStackAvailable = false;
        _resetstkoflw();
    }
    return bStackAvailable;
}


// Helper Classes to manage heap buffers for _ATL_SAFE_ALLOCA
template < class Allocator>
class CAtlSafeAllocBufferManager
{
private :
	struct CAtlSafeAllocBufferNode
	{
		CAtlSafeAllocBufferNode* m_pNext;
#if defined(_M_IX86)
		BYTE _pad[4];
#elif defined(_M_IA64)
		BYTE _pad[8];
#elif defined(_M_AMD64)
		BYTE _pad[8];
#else
	#error Only supported for X86, AMD64 and IA64
#endif
		void* GetData()
		{
			return (this + 1);
		}
	};

	CAtlSafeAllocBufferNode* m_pHead;
public :
	
	CAtlSafeAllocBufferManager() : m_pHead(NULL) {};
	void* Allocate(SIZE_T nRequestedSize)
	{
		CAtlSafeAllocBufferNode *p = (CAtlSafeAllocBufferNode*)Allocator::Allocate(::ATL::AtlAddThrow(nRequestedSize, static_cast<SIZE_T>(sizeof(CAtlSafeAllocBufferNode))));
		if (p == NULL)
			return NULL;
		
		// Add buffer to the list
		p->m_pNext = m_pHead;
		m_pHead = p;
		
		return p->GetData();
	}
	~CAtlSafeAllocBufferManager()
	{
		// Walk the list and free the buffers
		while (m_pHead != NULL)
		{
			CAtlSafeAllocBufferNode* p = m_pHead;
			m_pHead = m_pHead->m_pNext;
			Allocator::Free(p);
		}
	}
};

}	// namespace _ATL_SAFE_ALLOCA_IMPL

}	// namespace ATL
 #pragma pack(pop)

// Use one of the following macros before using _ATL_SAFE_ALLOCA
// EX version allows specifying a different heap allocator
#define USES_ATL_SAFE_ALLOCA_EX(x)    ATL::_ATL_SAFE_ALLOCA_IMPL::CAtlSafeAllocBufferManager<x> _AtlSafeAllocaManager

#ifndef USES_ATL_SAFE_ALLOCA
#define USES_ATL_SAFE_ALLOCA		USES_ATL_SAFE_ALLOCA_EX(ATL::CCRTAllocator)
#endif

// nRequestedSize - requested size in bytes 
// nThreshold - size in bytes beyond which memory is allocated from the heap.

// Defining _ATL_SAFE_ALLOCA_ALWAYS_ALLOCATE_THRESHOLD_SIZE always allocates the size specified
// for threshold if the stack space is available irrespective of requested size.
// This available for testing purposes. It will help determine the max stack usage due to _alloca's
// Disable _alloca not within try-except prefast warning since we verify stack space is available before.
#ifdef _ATL_SAFE_ALLOCA_ALWAYS_ALLOCATE_THRESHOLD_SIZE
#define _ATL_SAFE_ALLOCA(nRequestedSize, nThreshold)	\
	__pragma(warning(push))\
	__pragma(warning(disable:4616))\
	__pragma(warning(disable:6255))\
	((nRequestedSize <= nThreshold && ATL::_ATL_SAFE_ALLOCA_IMPL::_AtlVerifyStackAvailable(nThreshold) ) ?	\
		_alloca(nThreshold) :	\
		((ATL::_ATL_SAFE_ALLOCA_IMPL::_AtlVerifyStackAvailable(nThreshold)) ? _alloca(nThreshold) : 0),	\
			_AtlSafeAllocaManager.Allocate(nRequestedSize))\
	__pragma(warning(pop))
#else
#define _ATL_SAFE_ALLOCA(nRequestedSize, nThreshold)	\
	__pragma(warning(push))\
	__pragma(warning(disable:4616))\
	__pragma(warning(disable:6255))\
	((nRequestedSize <= nThreshold && ATL::_ATL_SAFE_ALLOCA_IMPL::_AtlVerifyStackAvailable(nRequestedSize) ) ?	\
		_alloca(nRequestedSize) :	\
		_AtlSafeAllocaManager.Allocate(nRequestedSize))\
	__pragma(warning(pop))
#endif

// Use 1024 bytes as the default threshold in ATL
#ifndef _ATL_SAFE_ALLOCA_DEF_THRESHOLD
#define _ATL_SAFE_ALLOCA_DEF_THRESHOLD	1024
#endif
