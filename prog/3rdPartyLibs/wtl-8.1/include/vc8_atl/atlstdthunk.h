// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.

#ifndef __ATLSTDTHUNK_H__
#define __ATLSTDTHUNK_H__

#pragma once

#pragma push_macro("malloc")
#undef malloc
#pragma push_macro("realloc")
#undef realloc
#pragma push_macro("free")
#undef free
#pragma push_macro("new")
#undef new
#pragma push_macro("HeapAlloc")
#undef HeapAlloc
#pragma push_macro("HeapFree")
#undef HeapFree
#pragma push_macro("GetProcessHeap")
#undef GetProcessHeap



 

 
namespace ATL
{

/////////////////////////////////////////////////////////////////////////////
// Thunks for __stdcall member functions

#if defined(_M_IX86)
PVOID __stdcall __AllocStdCallThunk(VOID);
VOID  __stdcall __FreeStdCallThunk(PVOID);

#pragma pack(push,1)
struct _stdcallthunk
{
	DWORD   m_mov;          // mov dword ptr [esp+0x4], pThis (esp+0x4 is hWnd)
	DWORD   m_this;         //
	BYTE    m_jmp;          // jmp WndProc
	DWORD   m_relproc;      // relative jmp
	BOOL Init(DWORD_PTR proc, void* pThis)
	{
		m_mov = 0x042444C7;  //C7 44 24 0C
		m_this = PtrToUlong(pThis);
		m_jmp = 0xe9;
		m_relproc = DWORD((INT_PTR)proc - ((INT_PTR)this+sizeof(_stdcallthunk)));
		// write block from data cache and
		//  flush from instruction cache
		FlushInstructionCache(GetCurrentProcess(), this, sizeof(_stdcallthunk));
		return TRUE;
	}
	//some thunks will dynamically allocate the memory for the code
	void* GetCodeAddress()
	{
		return this;
	}
	void* operator new(size_t)
	{
        return __AllocStdCallThunk();
    }
    void operator delete(void* pThunk)
    {
        __FreeStdCallThunk(pThunk);
    }
};
#pragma pack(pop)

#elif defined(_M_AMD64)
PVOID __AllocStdCallThunk(VOID);
VOID  __FreeStdCallThunk(PVOID);
#pragma pack(push,2)
struct _stdcallthunk
{
    USHORT  RcxMov;         // mov rcx, pThis
    ULONG64 RcxImm;         // 
    USHORT  RaxMov;         // mov rax, target
    ULONG64 RaxImm;         //
    USHORT  RaxJmp;         // jmp target
    BOOL Init(DWORD_PTR proc, void *pThis)
    {
        RcxMov = 0xb948;          // mov rcx, pThis
        RcxImm = (ULONG64)pThis;  // 
        RaxMov = 0xb848;          // mov rax, target
        RaxImm = (ULONG64)proc;   //
        RaxJmp = 0xe0ff;          // jmp rax
        FlushInstructionCache(GetCurrentProcess(), this, sizeof(_stdcallthunk));
		return TRUE;
    }
	//some thunks will dynamically allocate the memory for the code
	void* GetCodeAddress()
	{
		return this;
	}
	void* operator new(size_t)
	{
        return __AllocStdCallThunk();
    }
    void operator delete(void* pThunk)
    {
        __FreeStdCallThunk(pThunk);
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
	BOOL Init(DWORD_PTR proc, void* pThis)
	{
		ldah_at = (0x279f0000 | HIWORD(proc)) + (LOWORD(proc)>>15);
		ldah_a0 = (0x261f0000 | HIWORD(pThis)) + (LOWORD(pThis)>>15);
		lda_at = 0x239c0000 | LOWORD(proc);
		lda_a0 = 0x22100000 | LOWORD(pThis);
		jmp = 0x6bfc0000;
		// write block from data cache and
		//  flush from instruction cache
		FlushInstructionCache(GetCurrentProcess(), this, sizeof(_stdcallthunk));
		return TRUE;
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
	BOOL Init(DWORD_PTR proc, void* pThis)
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
		return TRUE;
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
	BOOL Init(DWORD_PTR proc, void* pThis)
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
		return TRUE;
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
	BOOL Init(DWORD_PTR proc, void* pThis)
	{
		m_mov_r0 = 0xE59F0000;
		m_mov_pc = 0xE59FF000;
		m_pThis = (DWORD)pThis;
		m_pFunc = (DWORD)proc;
		// write block from data cache and
		//  flush from instruction cache
		FlushInstructionCache(GetCurrentProcess(), this, sizeof(_stdcallthunk));
		return TRUE;
	}
	void* GetCodeAddress()
	{
		return this;
	}
};
#pragma pack(pop)
#elif defined(_M_IA64)
#pragma pack(push,8)
extern "C" void _StdCallThunkProcProc(void);
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
    BOOL Init(DWORD_PTR proc, void* pThis)
    {
        m_funcdesc.pfn = ((_FuncDesc*)(&_StdCallThunkProcProc))->pfn;  // Pointer to actual beginning of StdCallThunkProc
        m_funcdesc.gp = &m_pFunc;
        m_pFunc = reinterpret_cast< void* >( proc );
        m_pThis = pThis;
        ::FlushInstructionCache( GetCurrentProcess(), this, sizeof( _stdcallthunk ) );
		return TRUE;
    }
    void* GetCodeAddress()
    {
        return( &m_funcdesc );
    }
};
#pragma pack(pop)
//IA64 thunks do not currently use the atlhunk.cpp allocator.
#else
#error Only ARM, ALPHA, SH3, MIPS, IA64, AMD64 and X86 supported
#endif


#if defined(_M_IX86) || defined (_M_AMD64)
 
#pragma pack(push,8)
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
		{
			delete pThunk;
		}
	}

	BOOL Init(DWORD_PTR proc, void *pThis)
	{
		if (pThunk == NULL) 
		{
			pThunk = new _stdcallthunk;
			if (pThunk == NULL)
			{
				return FALSE;
			}
		}
		return pThunk->Init(proc, pThis);
	}
	

	void* GetCodeAddress()
	{
		return pThunk->GetCodeAddress();
	}
};

#pragma pack(pop)
typedef CDynamicStdCallThunk CStdCallThunk;
#else
typedef _stdcallthunk CStdCallThunk;
#endif  // _M_IX86 || _M_AMD64

}   // namespace ATL
 

#pragma pop_macro("GetProcessHeap")
#pragma pop_macro("HeapAlloc")
#pragma pop_macro("HeapFree")
#pragma pop_macro("new")
#pragma pop_macro("free")
#pragma pop_macro("realloc")
#pragma pop_macro("malloc")

#endif // __ATLSTDTHUNK_H__
