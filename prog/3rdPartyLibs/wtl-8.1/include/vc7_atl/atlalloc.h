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

namespace ATL
{
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
		return ::GlobalReAlloc(p, nBytes, 0);
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
		ATLASSERT(m_pData == NULL);
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
		return AllocateBytes(nElements*sizeof(T));
	}

	// Reallocate the buffer to hold a given number of elements
	bool Reallocate(size_t nElements) throw()
	{
		return ReallocateBytes(nElements*sizeof(T));
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
		return( AllocateBytes( nElements*sizeof( T ) ) );
	}

	T* Reallocate( size_t nElements ) throw( ... )
	{
		size_t nNewSize = nElements*sizeof( T ) ;
		
		if (m_p == NULL)
			return AllocateBytes(nNewSize);

		if (nNewSize > t_nFixedBytes)
		{
			if( m_p == reinterpret_cast< T* >( m_abFixedBuffer ) )
			{
				// We have to allocate from the heap and copy the contents into the new buffer
				AllocateHeap(nNewSize);
				memcpy(m_p, m_abFixedBuffer, t_nFixedBytes);
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
#define _ATL_STACK_MARGIN	0x2000	// Minimum stack available after call to _ATL_SAFE_ALLOCA
#endif

// Verifies if sufficient space is available on the stack.
inline bool _AtlVerifyStackAvailable(SIZE_T Size)
{
    bool bStackAvailable = true;

    __try
    {
        PVOID p = _alloca(Size + _ATL_STACK_MARGIN);
        (p);
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
#elif
	#error Only supported for X86 and IA64
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
		CAtlSafeAllocBufferNode *p = (CAtlSafeAllocBufferNode*)Allocator::Allocate(nRequestedSize + sizeof(CAtlSafeAllocBufferNode));
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

// Use one of the following macros before using _ATL_SAFE_ALLOCA
// EX version allows specifying a different heap allocator
#define USES_ATL_SAFE_ALLOCA_EX(x)	ATL::_ATL_SAFE_ALLOCA_IMPL::CAtlSafeAllocBufferManager<x> _AtlSafeAllocaManager

#ifndef USES_ATL_SAFE_ALLOCA
#define USES_ATL_SAFE_ALLOCA		USES_ATL_SAFE_ALLOCA_EX(ATL::CCRTAllocator)
#endif

// nRequestedSize - requested size in bytes 
// nThreshold - size in bytes beyond which memory is allocated from the heap.

// Defining _ATL_SAFE_ALLOCA_ALWAYS_ALLOCATE_THRESHOLD_SIZE always allocates the size specified
// for threshold if the stack space is available irrespective of requested size.
// This available for testing purposes. It will help determine the max stack usage due to _alloca's

#ifdef _ATL_SAFE_ALLOCA_ALWAYS_ALLOCATE_THRESHOLD_SIZE
#define _ATL_SAFE_ALLOCA(nRequestedSize, nThreshold)	\
	((nRequestedSize <= nThreshold && ATL::_ATL_SAFE_ALLOCA_IMPL::_AtlVerifyStackAvailable(nThreshold) ) ?	\
		_alloca(nThreshold) :	\
		((ATL::_ATL_SAFE_ALLOCA_IMPL::_AtlVerifyStackAvailable(nThreshold)) ? _alloca(nThreshold) : 0),	\
			_AtlSafeAllocaManager.Allocate(nRequestedSize))
#else
#define _ATL_SAFE_ALLOCA(nRequestedSize, nThreshold)	\
	((nRequestedSize <= nThreshold && ATL::_ATL_SAFE_ALLOCA_IMPL::_AtlVerifyStackAvailable(nRequestedSize) ) ?	\
		_alloca(nRequestedSize) :	\
		_AtlSafeAllocaManager.Allocate(nRequestedSize))
#endif

// Use 1024 bytes as the default threshold in ATL
#ifndef _ATL_SAFE_ALLOCA_DEF_THRESHOLD
#define _ATL_SAFE_ALLOCA_DEF_THRESHOLD	1024
#endif
