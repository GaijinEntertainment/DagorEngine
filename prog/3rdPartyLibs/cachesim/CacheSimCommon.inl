#pragma once

///////////
// This file contains common code for all platform
// It must be included exactly ONCE for the entire library
//////////

#include "Precompiled.h"

#include "CacheSim.h"
#include "CacheSimInternals.h"
#include "CacheSimData.h"
#include "GenericHashTable.h"
#include "Md5.h"

extern "C"
{
#include "udis86/udis86.h"
}

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

namespace CacheSim
{
  inline int32_t AtomicCompareExchange(volatile int32_t* addr, int32_t new_val, int32_t old_val) { return _InterlockedCompareExchange((volatile LONG*)addr, new_val, old_val); }
  inline int32_t AtomicIncrement(volatile int32_t* addr) { return _InterlockedIncrement((volatile LONG*)addr); }
  inline void PrintError(const char* error) { MessageBoxA(NULL, error, "Error", MB_OK | MB_ICONERROR); }
  inline void SleepMilliseconds(int ms) { Sleep(ms); }

  enum
  {
    kMaxCalls = 128
  };

  struct StackKey
  {
    StackKey() {}

    StackKey(const uintptr_t frames[], size_t frame_count)
    {
      md5_state_t s;
      md5_init(&s);
      md5_append(&s, (const uint8_t*)frames, int(frame_count * sizeof frames[0]));
      md5_finish(&s, m_Hash);
    }

    bool IsValid() const
    {
      return 0 != (m_Qwords[0] | m_Qwords[1]);
    }

    void Invalidate()
    {
      m_Qwords[0] = m_Qwords[1] = 0;
    }

    union
    {
      uint8_t   m_Hash[16];
      uint32_t  m_Dwords[4];
      uint64_t  m_Qwords[2];
    };
  };


  struct ThreadState
  {
    int32_t     m_Generation;
    ud_t        m_Disassembler;
    uint32_t    m_StackIndex;                 ///< Index of current stack in callstack data. Recomputed whenever the call stack contents changes.
    int         m_LogicalCoreIndex;           ///< Index of logical core, -1
  };

#if defined(_MSC_VER)
  static thread_local ThreadState s_ThreadState;
#else
  // This is required to guarantee that we don't perform a dynamic memory allocation on first access resulting in a potential deadlock
  static thread_local ThreadState s_ThreadState __attribute__((tls_model("initial-exec")));
#endif

  static volatile int32_t g_Generation = 1;
  static uint32_t g_TraceEnabled = 0;

  static volatile int32_t g_Lock;
  static CacheSim::JaguarCacheSim g_Cache;

  static int s_CoreMappingCount = 0;
  static struct { uint64_t m_ThreadId; int m_LogicalCore; } s_CoreMappings[128];

  class AutoSpinLock
  {
  public:
    AutoSpinLock()
    {
      int count = 0;
      while (AtomicCompareExchange(&g_Lock, 1, 0) == 1)
      {
        if (count++ == 1000)
        {
          Sleep(0);
          count = 0;
        }
      }
    }

    ~AutoSpinLock()
    {
      g_Lock = 0;
    }
  };

  struct StackValue
  {
    StackValue() : m_Offset(0), m_Count(0) {}

    uint32_t  m_Offset;
    uint32_t  m_Count;
  };

  struct RipKey
  {
    RipKey() : m_Rip(0), m_StackOffset(0) {}
    RipKey(uintptr_t rip, uint32_t stack_offset) : m_Rip(rip), m_StackOffset(stack_offset) {}
    uintptr_t m_Rip;
    uint32_t  m_StackOffset;
  };

  bool operator==(const RipKey& l, const RipKey& r)
  {
    return l.m_Rip == r.m_Rip && l.m_StackOffset == r.m_StackOffset;
  }

  struct RipStats
  {
    RipStats() { memset(m_Stats, 0, sizeof m_Stats); }

    uint32_t    m_Stats[CacheSim::kAccessResultCount];
  };

  bool operator==(const StackKey& l, const StackKey& r)
  {
    return 0 == memcmp(l.m_Hash, r.m_Hash, sizeof l.m_Hash);
  }

  uint32_t HashTypeOverload(const CacheSim::StackKey& key)
  {
    return key.m_Dwords[0];
  }

  uint32_t HashTypeOverload(const CacheSim::RipKey& key)
  {
    return uint32_t(key.m_Rip ^ (key.m_Rip >> 32) * 33 + 61 * key.m_StackOffset);
  }

  /// Maps 128-bit hash digests to call stacks.
  static GenericHashTable<StackKey, StackValue> g_Stacks;
  /// Maps RIP+Stack before that to stats
  static GenericHashTable<RipKey, RipStats> g_Stats;
  /// Raw storage array for stack trace values
  static struct
  {
    uintptr_t*  m_Frames;
    uint32_t    m_Count;
    uint32_t    m_ReserveCount;
  } g_StackData;

  RipStats* GetRipNode(uintptr_t pc, uint32_t stack_offset)
  {
    return g_Stats.Insert(RipKey(pc, stack_offset));
  }

  uint32_t InsertStack(const uintptr_t frames[], uint32_t frame_count)
  {
    StackKey key(frames, frame_count);

    if (StackValue* existing = g_Stacks.Find(key))
    {
      return existing->m_Offset;
    }

    // Create a new stack entry.
    uint32_t offset = g_StackData.m_Count;
    if (offset + frame_count + 1 > g_StackData.m_ReserveCount)
    {
      uint32_t new_reserve = g_StackData.m_ReserveCount ? 2 * g_StackData.m_ReserveCount : 65536;
      if (g_StackData.m_Frames)
      {
        g_StackData.m_Frames = (uintptr_t*)VirtualMemoryRealloc(g_StackData.m_Frames,
                                                                g_StackData.m_ReserveCount * sizeof(g_StackData.m_Frames[0]),
                                                                new_reserve * sizeof(g_StackData.m_Frames[0]));
      }
      else
      {
        g_StackData.m_Frames = (uintptr_t*)VirtualMemoryAlloc(new_reserve * sizeof g_StackData.m_Frames[0]);
      }

      g_StackData.m_ReserveCount = new_reserve;
    }

    memcpy(g_StackData.m_Frames + offset, frames, frame_count * sizeof frames[0]);
    g_StackData.m_Frames[offset + frame_count] = 0;
    g_StackData.m_Count += frame_count + 1;

    StackValue* val = g_Stacks.Insert(key);
    val->m_Offset = offset;
    val->m_Count = frame_count;

    return offset;
  }
}

static intptr_t ReadReg(ud_type_t reg, const CONTEXT* ctx)
{
  switch (reg)
  {
  case  UD_R_AL:  return (int8_t)(ctx->Rax);
  case  UD_R_AH:  return (int8_t)(ctx->Rax >> 8);
  case  UD_R_AX:  return (int16_t)(ctx->Rax);
  case UD_R_EAX:  return (int32_t)(ctx->Rax);
  case UD_R_RAX:  return          (ctx->Rax);

  case  UD_R_BL:  return (int8_t)(ctx->Rbx);
  case  UD_R_BH:  return (int8_t)(ctx->Rbx >> 8);
  case  UD_R_BX:  return (int16_t)(ctx->Rbx);
  case UD_R_EBX:  return (int32_t)(ctx->Rbx);
  case UD_R_RBX:  return          (ctx->Rbx);

  case  UD_R_CL:  return (int8_t)(ctx->Rcx);
  case  UD_R_CH:  return (int8_t)(ctx->Rcx >> 8);
  case  UD_R_CX:  return (int16_t)(ctx->Rcx);
  case UD_R_ECX:  return (int32_t)(ctx->Rcx);
  case UD_R_RCX:  return          (ctx->Rcx);

  case  UD_R_DL:  return (int8_t)(ctx->Rdx);
  case  UD_R_DH:  return (int8_t)(ctx->Rdx >> 8);
  case  UD_R_DX:  return (int16_t)(ctx->Rdx);
  case UD_R_EDX:  return (int32_t)(ctx->Rdx);
  case UD_R_RDX:  return          (ctx->Rdx);

  case UD_R_SIL:  return (int8_t)(ctx->Rsi);
  case UD_R_SI:   return (int16_t)(ctx->Rsi);
  case UD_R_ESI:  return (int32_t)(ctx->Rsi);
  case UD_R_RSI:  return          (ctx->Rsi);

  case UD_R_DIL:  return (int8_t)(ctx->Rdi);
  case UD_R_DI:   return (int16_t)(ctx->Rdi);
  case UD_R_EDI:  return (int32_t)(ctx->Rdi);
  case UD_R_RDI:  return          (ctx->Rdi);

  case UD_R_BPL:  return (int8_t)(ctx->Rbp);
  case UD_R_BP:   return (int16_t)(ctx->Rbp);
  case UD_R_EBP:  return (int32_t)(ctx->Rbp);
  case UD_R_RBP:  return          (ctx->Rbp);

  case UD_R_SPL:  return (int8_t)(ctx->Rsp);
  case UD_R_SP:   return (int16_t)(ctx->Rsp);
  case UD_R_ESP:  return (int32_t)(ctx->Rsp);
  case UD_R_RSP:  return          (ctx->Rsp);

  case UD_R_R8B:  return (int8_t)(ctx->R8);
  case UD_R_R8W:  return (int16_t)(ctx->R8);
  case UD_R_R8D:  return (int32_t)(ctx->R8);
  case UD_R_R8:   return          (ctx->R8);

  case UD_R_R9B:  return (int8_t)(ctx->R9);
  case UD_R_R9W:  return (int16_t)(ctx->R9);
  case UD_R_R9D:  return (int32_t)(ctx->R9);
  case UD_R_R9:   return          (ctx->R9);

  case UD_R_R10B: return (int8_t)(ctx->R10);
  case UD_R_R10W: return (int16_t)(ctx->R10);
  case UD_R_R10D: return (int32_t)(ctx->R10);
  case UD_R_R10:  return          (ctx->R10);

  case UD_R_R11B: return (int8_t)(ctx->R11);
  case UD_R_R11W: return (int16_t)(ctx->R11);
  case UD_R_R11D: return (int32_t)(ctx->R11);
  case UD_R_R11:  return          (ctx->R11);

  case UD_R_R12B: return (int8_t)(ctx->R12);
  case UD_R_R12W: return (int16_t)(ctx->R12);
  case UD_R_R12D: return (int32_t)(ctx->R12);
  case UD_R_R12:  return          (ctx->R12);

  case UD_R_R13B: return (int8_t)(ctx->R13);
  case UD_R_R13W: return (int16_t)(ctx->R13);
  case UD_R_R13D: return (int32_t)(ctx->R13);
  case UD_R_R13:  return          (ctx->R13);

  case UD_R_R14B: return (int8_t)(ctx->R14);
  case UD_R_R14W: return (int16_t)(ctx->R14);
  case UD_R_R14D: return (int32_t)(ctx->R14);
  case UD_R_R14:  return          (ctx->R14);

  case UD_R_R15B: return (int8_t)(ctx->R15);
  case UD_R_R15W: return (int16_t)(ctx->R15);
  case UD_R_R15D: return (int32_t)(ctx->R15);
  case UD_R_R15:  return          (ctx->R15);


  case UD_R_RIP:  return          (ctx->Rip);
  }

  DebugBreak();
  return 0;
}

static uintptr_t AdjustFsSegment(uintptr_t address);
static uintptr_t AdjustGsSegment(uintptr_t address);

static uintptr_t ComputeEa(const ud_t* ud, int operand_index, const CONTEXT* ctx)
{
  uintptr_t addr = 0;
  const ud_operand_t& op = ud->operand[operand_index];

  switch (op.offset)
  {
  case 8:  addr += op.lval.sbyte; break;
  case 16: addr += op.lval.sword; break;
  case 32: addr += op.lval.sdword; break;
  case 64: addr += op.lval.sqword; break;
  }

  if (op.base != UD_NONE)
  {
    addr += ReadReg(op.base, ctx);
  }

  if (op.index != UD_NONE)
  {
    intptr_t regval = ReadReg(op.index, ctx);
    if (UD_NONE != op.scale)
      addr += regval * op.scale;
    else
      addr += regval;
  }

  switch (ud->pfx_seg)
  {
  case UD_R_FS:
    addr = AdjustFsSegment(addr);
    break;
  case UD_R_GS:
    addr = AdjustGsSegment(addr);
    break;
  }

  return addr;
}

static void InvalidateStack()
{
  using namespace CacheSim;
  s_ThreadState.m_StackIndex = ~0u;
}

static void GenerateMemoryAccesses(int core_index, const ud_t* ud, uint64_t rip, int ilen, const CONTEXT* ctx)
{
  using namespace CacheSim;
  int read_count = 0;
  int write_count = 0;

  struct MemOp { uintptr_t ea; size_t sz; };
  MemOp prefetch_op = { 0, 0 };
  MemOp reads[4];
  MemOp writes[4];

  auto data_r = [&](uintptr_t addr, size_t sz) -> void
  {
    if (sz == 0)
      DebugBreak();
    if (intptr_t(addr) < 0)
      DebugBreak();
    if (addr)
    {
      reads[read_count].ea = addr;
      reads[read_count].sz = sz;
      ++read_count;
    }
  };

  auto data_w = [&](uintptr_t addr, size_t sz) -> void
  {
    if (sz == 0)
      DebugBreak();
    if (intptr_t(addr) < 0)
      DebugBreak();
    if (addr)
    {
      writes[write_count].ea = addr;
      writes[write_count].sz = sz;
      ++write_count;
    }
  };

  //const int dir = ctx->EFlags & (1 << 10) ? -1 : 1;

  uint32_t existing_stack_index = s_ThreadState.m_StackIndex;

  // Handle instructions with implicit memory operands.
  switch (ud->mnemonic)
  {
    // String instructions.
  case UD_Ilodsb:
  case UD_Iscasb:
    data_r(ctx->Rsi, 1);
    break;
  case UD_Ilodsw:
  case UD_Iscasw:
    data_r(ctx->Rsi, 2);
    break;
  case UD_Ilodsd:
  case UD_Iscasd:
    data_r(ctx->Rsi, 4);
    break;
  case UD_Ilodsq:
  case UD_Iscasq:
    data_r(ctx->Rsi, 8);
    break;
  case UD_Istosb:
    data_w(ctx->Rdi, 1);
    break;
  case UD_Istosw:
    data_w(ctx->Rdi, 2);
    break;
  case UD_Istosd:
    data_w(ctx->Rdi, 4);
    break;
  case UD_Istosq:
    data_w(ctx->Rdi, 8);
    break;
  case UD_Imovsb:
    data_r(ctx->Rsi, 1);
    data_w(ctx->Rdi, 1);
    break;
  case UD_Imovsw:
    data_r(ctx->Rsi, 2);
    data_w(ctx->Rdi, 2);
    break;
  case UD_Imovsd:
    data_r(ctx->Rsi, 4);
    data_w(ctx->Rdi, 4);
    break;
  case UD_Imovsq:
    data_r(ctx->Rsi, 8);
    data_w(ctx->Rdi, 8);
    break;

    // Stack operations.
  case UD_Ipush:    data_w(ctx->Rsp, ud->operand[0].size / 8); break;
  case UD_Ipop:     data_w(ctx->Rsp, ud->operand[0].size / 8); break;
  case UD_Icall:    data_w(ctx->Rsp, 8); InvalidateStack(); break;
  case UD_Iret:     data_r(ctx->Rsp, 8); InvalidateStack(); break;
  }

  // Handle special memory ops operands
  switch (ud->mnemonic)
  {
  case UD_Ipause:
    // This helps to avoid deadlocks.
  {
    static volatile int32_t do_ms_step = 0;
    int32_t val = AtomicIncrement(&do_ms_step);
    SleepMilliseconds((val & 0x1fff) == 0 ? 1 : 0);
  }
  break;
  case UD_Ilea:
  case UD_Inop:
    // LEA doesn't actually access memory even though it has memory operands.
    // There also seem to be NOPs that do crazy things with memory operands.
    break;
  case UD_Iprefetch:
  case UD_Iprefetchnta:
  case UD_Iprefetcht0:
  case UD_Iprefetcht1:
  case UD_Iprefetcht2:
    prefetch_op.ea = ComputeEa(ud, 0, ctx);
    prefetch_op.sz = 64;
    break;

  case UD_Imovntq:
    // TODO: Handle this specially?
    data_w(ComputeEa(ud, 0, ctx), 8);
    break;

  case UD_Imovntdq:
  case UD_Imovntdqa:
    // TODO: Handle this specially?
    data_w(ComputeEa(ud, 0, ctx), 16);
    break;

  case UD_Ifxsave:
    data_w(ComputeEa(ud, 0, ctx), 512);
    break;

  case UD_Ifxrstor:
    data_r(ComputeEa(ud, 0, ctx), 512);
    break;

  default:
    for (int op = 0; op < ARRAY_SIZE(ud->operand) && ud->operand[op].type != UD_NONE; ++op)
    {
      if (UD_OP_MEM != ud->operand[op].type)
        continue;

      switch (ud->operand[op].access)
      {
      case UD_OP_ACCESS_READ:
        data_r(ComputeEa(ud, op, ctx), ud->operand[op].size / 8);
        break;
      case UD_OP_ACCESS_WRITE:
        data_w(ComputeEa(ud, op, ctx), ud->operand[op].size / 8);
        break;
      }
    }
  }

#if 0
  // This is a good thing to have while developing, but not in production.
  // It's not safe to call printf() here, because the thread we're tracing might be inside printf() too.
  // Hello reentrancy.
  static volatile int64_t insns = 0;
  int64_t count = AtomicInc64(&insns);
  if (0 == (count % 1'000'000))
  {
    printf("%lld million instructions traced\n", count / 1'000'000);
  }
#endif

  // Commit stats for this instruction in a critical section.
  AutoSpinLock lock;

  if (!g_TraceEnabled)
    return;

  // Find stats line for the instruction pointer
  RipStats* stats = GetRipNode(rip, existing_stack_index);

  stats->m_Stats[CacheSim::kInstructionsExecuted] += 1;

  // Generate I-cache traffic.
  {
    CacheSim::AccessResult r = g_Cache.Access(core_index, rip, ilen, CacheSim::kCodeRead);
    stats->m_Stats[r] += 1;

    // Generate prefetch traffic. Pretend prefetches are immediate reads and record how effective they were.
    if (prefetch_op.ea)
    {
      switch (g_Cache.Access(core_index, prefetch_op.ea, prefetch_op.sz, CacheSim::kRead))
      {
      case CacheSim::kD1Hit:
        stats->m_Stats[CacheSim::kPrefetchHitD1] += 1;
        break;
      case CacheSim::kL2Hit:
        stats->m_Stats[CacheSim::kPrefetchHitL2] += 1;
        break;
      }
    }
  }

  // Generate D-cache traffic.
  for (int i = 0; i < read_count; ++i)
  {
    CacheSim::AccessResult r = g_Cache.Access(core_index, reads[i].ea, reads[i].sz, CacheSim::kRead);
    stats->m_Stats[r] += 1;
  }

  for (int i = 0; i < write_count; ++i)
  {
    CacheSim::AccessResult r = g_Cache.Access(core_index, writes[i].ea, writes[i].sz, CacheSim::kWrite);
    stats->m_Stats[r] += 1;
  }
}

static int FindLogicalCoreIndex(uint64_t thread_id)
{
  using namespace CacheSim;

  int count = s_CoreMappingCount;

  for (int i = 0; i < count; ++i)
  {
    if (s_CoreMappings[i].m_ThreadId == thread_id)
    {
      return s_CoreMappings[i].m_LogicalCore;
    }
  }

  return -1;
}

void CacheSim::SetThreadCoreMapping(uint64_t thread_id, int logical_core_id)
{
  using namespace CacheSim;

  AutoSpinLock lock;

  int count = s_CoreMappingCount;

  for (int i = 0; i < count; ++i)
  {
    if (s_CoreMappings[i].m_ThreadId == thread_id)
    {
      if (logical_core_id == -1)
      {
        // Remove the mapping
        s_CoreMappings[i] = s_CoreMappings[count - 1];
        --s_CoreMappingCount;
      }
      else
      {
        s_CoreMappings[i].m_LogicalCore = logical_core_id;
      }
      return;
    }
  }

  if (count == ARRAY_SIZE(s_CoreMappings))
  {
    DebugBreak(); // Increase array size
    return;
  }

  s_CoreMappings[count].m_ThreadId = thread_id;
  s_CoreMappings[count].m_LogicalCore = logical_core_id;

  ++s_CoreMappingCount;
}

void CacheSim::ResetThreadCoreMappings()
{
  s_CoreMappingCount = 0;
}

struct ModuleInfo
{
  char m_Filename[512];
  void* m_StartAddrInMemory;
  void* m_SegmentOffset;
  size_t m_Length;
};

struct ModuleList
{
  ModuleInfo m_Infos[1024];
  int m_Count = 0;
  int m_ModuleCallbacks = 0;
};

static ModuleList g_ModuleList;

static void DisableTrapFlag();
static void GetFilenameForSave(char* filename, size_t bufferSize);
static void GetModuleList(ModuleList* moduleList);

namespace
{
  template<typename T> void WriteHelper(FILE* f, const T& val)
  {
    fwrite(&val, 1, sizeof val, f);
  }
}

void CacheSim::EndCapture(bool save)
{
  using namespace CacheSim;
  g_TraceEnabled = 0;

  DisableTrapFlag();

  if (!save)
    return;

  // It's tempting to remove the signal handler here
  //
  //    RemoveVectoredExceptionHandler(g_Handler);
  //    g_Handler = nullptr;
  //
  // ..but that's a mistake. There could be a syscall instruction paused in the kernel that
  // will come back and signal a single step trap at some arbitrary point in the future, so
  // we need our handler to stay in effect.


  AutoSpinLock lock;

  char filename[512];
  GetFilenameForSave(filename, ARRAY_SIZE(filename));
  
  if (FILE* f = fopen(filename, "wb"))
  {
    auto align = [&f]()
    {
      if (int needed = (8 - (ftell(f) & 7)) & 7)
      {
        static const uint8_t padding[8] = { 0 };
        fwrite(padding, 1, needed, f);
      }
    };

#define welem(value) WriteHelper(f, value)

#define wdata(data, size) fwrite(data, 1, size, f);

    struct PatchWord
    {
      long m_Offset;
      FILE* m_File;

      explicit PatchWord(FILE* f) : m_File(f), m_Offset(ftell(f))
      {
        static const uint8_t placeholder[] = { 0xcc, 0xdd, 0xee, 0xff };
        fwrite(placeholder, 1, sizeof placeholder, f);
      }

      void Update(uint32_t value)
      {
        long pos = ftell(m_File);
        fseek(m_File, m_Offset, SEEK_SET);
        if (ftell(m_File) != m_Offset)
        {
          DebugBreak();
        }
        fwrite(&value, 1, sizeof value, m_File);
        fseek(m_File, pos, SEEK_SET);
      }
    };

    welem(0xcace51afu);
    welem(0x00000002u);

    PatchWord module_offset{ f };
    PatchWord module_count{ f };

    PatchWord module_str_offset{ f };

    PatchWord frame_offset{ f };
    PatchWord frame_count{ f };

    PatchWord stats_offset{ f };
    PatchWord stats_count{ f };

    welem(0u); // symbol_offset
    welem(0u); // symbol_count
    welem(0u); // symbol_text_offset

    GetModuleList(&g_ModuleList);

    if (g_ModuleList.m_Count > 0)
    {
      align();

      module_offset.Update(ftell(f));
      module_count.Update(static_cast<uint32_t>(g_ModuleList.m_Count));
      uint32_t str_section_size = 0;

      for (size_t i = 0; i < g_ModuleList.m_Count; ++i)
      {
        ModuleInfo& info = g_ModuleList.m_Infos[i];
        size_t len = strlen(info.m_Filename) + 1;
        welem(reinterpret_cast<uintptr_t>(info.m_StartAddrInMemory));
        welem(reinterpret_cast<uintptr_t>(info.m_SegmentOffset));
        welem(static_cast<uint32_t>(info.m_Length));
        welem(static_cast<uint32_t>(str_section_size));
        str_section_size += (uint32_t)len;
      }

      module_str_offset.Update(ftell(f));
      for (size_t i = 0; i < g_ModuleList.m_Count; ++i)
      {
        wdata(g_ModuleList.m_Infos[i].m_Filename, strlen(g_ModuleList.m_Infos[i].m_Filename) + 1);
      }
    }
    align();

    // Write raw values for stack frames
    frame_offset.Update(ftell(f));
    frame_count.Update(g_StackData.m_Count);
    wdata(g_StackData.m_Frames, g_StackData.m_Count * sizeof g_StackData.m_Frames[0]);

    align();
    // Write stats
    stats_offset.Update(ftell(f));
    stats_count.Update((uint32_t)g_Stats.GetCount());

    for (const RipKey& key : g_Stats.Keys())
    {
      welem(key.m_Rip);
      welem(key.m_StackOffset);
      welem(*g_Stats.Find(key));
      welem(static_cast<uint32_t>(0));
    }

    fclose(f);
  }
  else
  {
    fprintf(stderr, "Failed to open %s for writing", filename);
  }

  g_Stats.FreeAll();
  g_Stacks.FreeAll();

  VirtualMemoryFree(g_StackData.m_Frames, g_StackData.m_ReserveCount);
  memset(&g_StackData, 0, sizeof g_StackData);
  memset(&g_ModuleList, 0, sizeof g_ModuleList);
}