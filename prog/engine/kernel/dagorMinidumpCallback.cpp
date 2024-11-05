// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_minidumpCallback.h>
#include <windows.h>
#include <shlwapi.h>
#include <dbghelp.h>
#include <stdlib.h>
#include <util/dag_globDef.h>
#include <util/dag_watchdog.h>
#include <osApiWrappers/dag_threads.h>

#pragma comment(lib, "shlwapi.lib")

const uintptr_t REGS_DUMP_MEMORY_FORWARD = 224;
const uintptr_t REGS_DUMP_MEMORY_BACK = 32;
const uintptr_t REGS_PTR_DUMP_SIZE = sizeof(uintptr_t) * 10;
const uintptr_t PTR_DUMP_SIZE = sizeof(uintptr_t) * 6;
const uintptr_t REG_SCAN_PTR_DATA_SIZE = sizeof(uintptr_t) * 16;
const uintptr_t STACK_SCAN_PTR_DATA_SIZE = sizeof(uintptr_t) * 6;
const uintptr_t MAX_VECTOR_ELEMENTS = 1024;
const uintptr_t VECTOR_PTR_DUMP_SIZE = 1024;
const uintptr_t VECTOR_MAX_ELEM_SIZE = 96; // enough for phys contacts data
const uintptr_t STRIPPED_STACK_SIZE = 256;

DagorHwException::MinidumpExceptionData::MinidumpExceptionData(EXCEPTION_POINTERS *_eptr, uintptr_t exc_thread_id)
{
  eptr = _eptr;
  excThread.threadId = exc_thread_id;
  excThread.needScanMem = !is_watchdog_thread(exc_thread_id);
  excThread.needDumpRet = true;
  if (_eptr)
    excThread.setRegisters(*_eptr->ContextRecord);
  mainThread.threadId = get_main_thread_id();
  mainThread.needScanMem = mainThread.threadId != exc_thread_id;
}

static BOOL get_vector_at_addr(uintptr_t addr, uintptr_t end, uint32_t &out_vector_size, uint32_t &out_data_size)
{
  struct DagVectorData // das::Array has same layout
  {
    uintptr_t data;
    uint32_t size;
    uint32_t capacity;
    bool validateCounters() const { return size > 0 && size <= capacity && capacity <= MAX_VECTOR_ELEMENTS; }
  };
  const DagVectorData *dv = (const DagVectorData *)addr;
  if (addr + sizeof(DagVectorData) <= end && dv->validateCounters())
  {
    out_vector_size = sizeof(DagVectorData);
    out_data_size = min(VECTOR_PTR_DUMP_SIZE, dv->size * VECTOR_MAX_ELEM_SIZE);
    return TRUE;
  }

  struct DagVectorWithAllocData
  {
    uintptr_t data;
    uintptr_t allocator;
    uint32_t size;
    uint32_t capacity;
    bool validateCounters() const { return size > 0 && size <= capacity && capacity <= MAX_VECTOR_ELEMENTS; }
  };
  const DagVectorWithAllocData *va = (const DagVectorWithAllocData *)addr;
  if (addr + sizeof(DagVectorWithAllocData) <= end && va->validateCounters())
  {
    out_vector_size = sizeof(DagVectorWithAllocData);
    out_data_size = min(VECTOR_PTR_DUMP_SIZE, va->size * VECTOR_MAX_ELEM_SIZE);
    return TRUE;
  }

  struct EastlVectorData
  {
    uintptr_t data;
    uintptr_t end;
    uintptr_t capacityEnd;
    bool validateCounters() const { return data < end && end <= capacityEnd && capacityEnd - data < 10000; }
  };
  const EastlVectorData *ev = (const EastlVectorData *)addr;
  if (addr + sizeof(EastlVectorData) <= end && ev->validateCounters())
  {
    out_vector_size = sizeof(EastlVectorData);
    out_data_size = min(VECTOR_PTR_DUMP_SIZE, ev->end - ev->data);
    return TRUE;
  }

  return FALSE;
}

static BOOL get_memory_range_for_dump(uintptr_t addr, uintptr_t bytes_fwd, uintptr_t bytes_back,
  const DagorHwException::MinidumpExceptionData *data, DagorHwException::MemoryRange &range, bool &is_rw_data)
{
  if (addr < 0x100000 || uint64_t(addr) >> 48 || int32_t(addr) == -1)
    return FALSE;
  if (data->excThread.stack.contain(addr) || data->mainThread.stack.contain(addr))
    return FALSE;
  if (data->code.contain(addr)) // exclude game code (assume it's not changed)
    return FALSE;
  if (data->ntdll.contain(addr) || data->user32.contain(addr) || data->kernel32.contain(addr) || data->kernelBase.contain(addr))
    return FALSE;
  if (data->videoDriver.contain(addr))
    return FALSE;
  MEMORY_BASIC_INFORMATION mem;
  if (VirtualQuery((void *)addr, &mem, sizeof(mem)) && mem.State & MEM_COMMIT)
  {
    if (mem.Protect == PAGE_READONLY && data->image.contain(addr)) // exclude known constants
      return FALSE;
    uintptr_t start = max(addr - bytes_back, (uintptr_t)mem.BaseAddress);
    uintptr_t end = min(addr + bytes_fwd, (uintptr_t)mem.BaseAddress + (uintptr_t)mem.RegionSize);
    if (addr >= start && addr < end)
    {
      range.from = start;
      range.to = end;
      is_rw_data = mem.Protect == PAGE_READWRITE;
      return TRUE;
    }
  }
  return FALSE;
}

static BOOL dump_memory(uintptr_t addr, uintptr_t bytes_fwd, uintptr_t bytes_back, DagorHwException::MinidumpExceptionData *data,
  PMINIDUMP_CALLBACK_OUTPUT callback_output, uint32_t scan_ptr_data_size, uint32_t scan_ptr_dump_len)
{
  DagorHwException::MemoryRange range;
  bool isRwData = false;
  if (get_memory_range_for_dump(addr, bytes_fwd, bytes_back, data, range, isRwData))
  {
    callback_output->MemoryBase = int64_t(intptr_t(range.from)); // sign extend for win32
    callback_output->MemorySize = ULONG(range.to - range.from);
    if (isRwData && !(addr & sizeof(uintptr_t) - 1))
    {
      uintptr_t end = min(addr + scan_ptr_data_size, range.to);
      for (uintptr_t m = addr; m < end; m += sizeof(uintptr_t))
      {
        uintptr_t dumpPtr = *(uintptr_t *)m;
        uint32_t dumpLen = scan_ptr_dump_len;
        uint32_t vectorSize = 0;
        if (get_vector_at_addr(m, end, vectorSize, dumpLen))
          vectorSize -= sizeof(uintptr_t);
        if (get_memory_range_for_dump(dumpPtr, dumpLen, 0, data, range, isRwData))
        {
          m += vectorSize; // skip vector if pointer valid
          if (data->memAddQueueSize >= countof(data->memAddQueue))
          {
            logerr("memAddQueue overflow (%i)", countof(data->memAddQueue));
            break;
          }
          data->memAddQueue[data->memAddQueueSize++] = range;
        }
      }
    }
    return TRUE;
  }
  return FALSE;
}

static BOOL dump_code(uintptr_t addr, uintptr_t bytes_fwd, uintptr_t bytes_back, DagorHwException::MinidumpExceptionData *data,
  PMINIDUMP_CALLBACK_OUTPUT callback_output)
{
  if (data->code.contain(addr))
  {
    uintptr_t from = max(addr - bytes_back, data->code.from);
    uintptr_t to = min(addr + bytes_fwd, data->code.to);
    callback_output->MemoryBase = int64_t(intptr_t(from));
    callback_output->MemorySize = ULONG(to - from);
    return TRUE;
  }
  else if (dump_memory(addr, bytes_fwd, bytes_back, data, callback_output, 0 /*scan ptrs*/, 0 /*dump ptrs len*/))
    return TRUE;
  return FALSE;
}

static BOOL dump_thread_memory(DagorHwException::MinidumpExceptionData *data, DagorHwException::MinidumpThreadData &thread,
  PMINIDUMP_CALLBACK_OUTPUT callback_output)
{
  if (thread.stackPtr >= thread.stack.from)
  {
    while (thread.stackPtr < thread.stack.to)
    {
      uintptr_t addr = *(uintptr_t *)thread.stackPtr;
      if (thread.needDumpRet)
      {
        thread.needDumpRet = false;
        thread.stackPtr += sizeof(uintptr_t);
        if (dump_code(addr, 32 /*fwd*/, 128 /*back*/, data, callback_output))
          return TRUE;
        continue;
      }
      uint32_t len = PTR_DUMP_SIZE;
      uint32_t vectorSize = 0;
      if (get_vector_at_addr(thread.stackPtr, thread.stack.to, vectorSize, len))
        vectorSize -= sizeof(uintptr_t);
      thread.stackPtr += sizeof(uintptr_t);
      if (dump_memory(addr, len, 0 /*back*/, data, callback_output, STACK_SCAN_PTR_DATA_SIZE, PTR_DUMP_SIZE))
      {
        thread.stackPtr += vectorSize; // skip vector if .data valid
        return TRUE;
      }
    }
  }
  while (thread.scannedRegs < thread.regsCount)
  {
    uintptr_t regValue = thread.regs[thread.scannedRegs++];
    if (dump_memory(regValue, REGS_DUMP_MEMORY_FORWARD, REGS_DUMP_MEMORY_BACK, data, callback_output, REG_SCAN_PTR_DATA_SIZE,
          REGS_PTR_DUMP_SIZE))
      return TRUE;
  }
  return FALSE;
}

static BOOL minidump_memory_callback(DagorHwException::MinidumpExceptionData *data, PMINIDUMP_CALLBACK_OUTPUT callback_output)
{
  data->memCallbackCalled = true;
  if (data->memAddQueueSize)
  {
    DagorHwException::MemoryRange range = data->memAddQueue[--data->memAddQueueSize];
    callback_output->MemoryBase = int64_t(intptr_t(range.from));
    callback_output->MemorySize = ULONG(range.to - range.from);
    G_ASSERT(callback_output->MemorySize > 0 && callback_output->MemorySize <= 1024);
    return TRUE;
  }
  if (data->excThread.needScanMem && dump_thread_memory(data, data->excThread, callback_output))
    return TRUE;
  if (data->mainThread.needScanMem && dump_thread_memory(data, data->mainThread, callback_output))
    return TRUE;
  G_ASSERT(!data->memAddQueueSize);
  return FALSE;
}

BOOL CALLBACK DagorHwException::minidump_callback(PVOID callback_param, const PMINIDUMP_CALLBACK_INPUT callback_input,
  PMINIDUMP_CALLBACK_OUTPUT callback_output)
{
  MinidumpExceptionData *data = (MinidumpExceptionData *)callback_param;
  switch (callback_input->CallbackType) //-V::1037 Two or more case-branches perform the same actions.
  {
    case ModuleCallback:
      if (uint64_t(&minidump_callback) > callback_input->Module.BaseOfImage &&
          uint64_t(&minidump_callback) < callback_input->Module.BaseOfImage + callback_input->Module.SizeOfImage)
      {
        data->image.from = callback_input->Module.BaseOfImage;
        data->image.to = callback_input->Module.BaseOfImage + callback_input->Module.SizeOfImage;

        const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)data->image.from;
        const IMAGE_NT_HEADERS *pe = (const IMAGE_NT_HEADERS *)((char *)data->image.from + dos->e_lfanew);
        data->code.from = data->image.from + pe->OptionalHeader.BaseOfCode;
        data->code.to = data->code.from + pe->OptionalHeader.SizeOfCode;
      }
      else if (StrStrIW(callback_input->Module.FullPath, L"ntdll.dll"))
      {
        data->ntdll.from = callback_input->Module.BaseOfImage;
        data->ntdll.to = callback_input->Module.BaseOfImage + callback_input->Module.SizeOfImage;
      }
      else if (StrStrIW(callback_input->Module.FullPath, L"user32.dll"))
      {
        data->user32.from = callback_input->Module.BaseOfImage;
        data->user32.to = callback_input->Module.BaseOfImage + callback_input->Module.SizeOfImage;
      }
      else if (StrStrIW(callback_input->Module.FullPath, L"kernel32.dll"))
      {
        data->kernel32.from = callback_input->Module.BaseOfImage;
        data->kernel32.to = callback_input->Module.BaseOfImage + callback_input->Module.SizeOfImage;
      }
      else if (StrStrIW(callback_input->Module.FullPath, L"kernelbase.dll"))
      {
        data->kernelBase.from = callback_input->Module.BaseOfImage;
        data->kernelBase.to = callback_input->Module.BaseOfImage + callback_input->Module.SizeOfImage;
      }
      else if (StrStrIW(callback_input->Module.FullPath, L"nvwgf2um") || StrStrIW(callback_input->Module.FullPath, L"atidxx"))
      {
        data->videoDriver.from = callback_input->Module.BaseOfImage;
        data->videoDriver.to = callback_input->Module.BaseOfImage + callback_input->Module.SizeOfImage;
      }
      // Here is common filter effective in most cases, but I prefer to have full memory map
      // if (!(callback_output->ModuleWriteFlags & ModuleReferencedByMemory))
      // callback_output->ModuleWriteFlags &= ~ModuleWriteModule;
      return TRUE;

    case IncludeThreadCallback:
    {
      bool saveStack = true;
      bool allThreads = true;
      if (callback_input->IncludeThread.ThreadId == data->excThread.threadId ||
          callback_input->IncludeThread.ThreadId == data->mainThread.threadId ||
          DaThread::isDaThreadWinUnsafe(callback_input->IncludeThread.ThreadId, saveStack) || allThreads)
      {
        saveStack &= !is_watchdog_thread(callback_input->IncludeThread.ThreadId);
        callback_output->ThreadWriteFlags = ThreadWriteThread | ThreadWriteContext;
        if (saveStack)
          callback_output->ThreadWriteFlags |= ThreadWriteStack;
        if (callback_input->IncludeThread.ThreadId == data->excThread.threadId)
          callback_output->ThreadWriteFlags |= ThreadWriteInstructionWindow;
        return TRUE;
      }
      return FALSE;
    }

    case MemoryCallback: return minidump_memory_callback(data, callback_output);

    case RemoveMemoryCallback:
      if (data->memRemoveQueueSize)
      {
        DagorHwException::MemoryRange range = data->memRemoveQueue[--data->memRemoveQueueSize];
        callback_output->MemoryBase = int64_t(intptr_t(range.from));
        callback_output->MemorySize = ULONG(range.to - range.from);
        return TRUE;
      }
      return FALSE;

    case ThreadCallback:
    case ThreadExCallback:
      if (callback_input->Thread.ThreadId == data->excThread.threadId)
      {
        data->excThread.stack.from = callback_input->Thread.StackEnd;
        data->excThread.stack.to = callback_input->Thread.StackBase;
#if _TARGET_64BIT && _M_ARM64
        memcpy(&callback_input->Thread.Context.V[0], &data->eptr->ContextRecord->V[0], sizeof(M128A) * 16); //-V512
#elif _TARGET_64BIT
        memcpy(&callback_input->Thread.Context.Xmm0, &data->eptr->ContextRecord->Xmm0, sizeof(M128A) * 16); //-V512
#endif
      }
      else if (callback_input->Thread.ThreadId == data->mainThread.threadId)
      {
        data->mainThread.stack.from = callback_input->Thread.StackEnd;
        data->mainThread.stack.to = callback_input->Thread.StackBase;
        data->mainThread.setRegisters(callback_input->Thread.Context);
      }
      if (!(callback_output->ThreadWriteFlags & ThreadWriteStack))
      {
        // ThreadWriteContext saves stack, so we need to strip it manually
#if _TARGET_64BIT && _M_ARM64
        uintptr_t sp = callback_input->Thread.Context.Sp;
#elif _TARGET_64BIT
        uintptr_t sp = callback_input->Thread.Context.Rsp;
#else
        uintptr_t sp = int64_t(intptr_t(callback_input->Thread.Context.Esp));
#endif
        uintptr_t from = sp + STRIPPED_STACK_SIZE;
        uintptr_t to = uintptr_t(callback_input->Thread.StackBase);
        if (to > from)
        {
          if (data->memRemoveQueueSize < countof(data->memRemoveQueue))
            data->memRemoveQueue[data->memRemoveQueueSize++] = DagorHwException::MemoryRange{from, to};
          else
            callback_output->ThreadWriteFlags = ThreadWriteThread;
        }
      }
      return TRUE;

    case IncludeModuleCallback: return TRUE;

    default: return FALSE;
  }
}

int CALLBACK DagorHwException::write_minidump_fallback(HANDLE hProcess, DWORD dwProcessId, HANDLE hDumpFile, EXCEPTION_POINTERS *eptr,
  MINIDUMP_USER_STREAM_INFORMATION *user_streams)
{
  SetFilePointerEx(hDumpFile, {} /*dist*/, NULL, FILE_BEGIN);
  SetEndOfFile(hDumpFile);
  MINIDUMP_EXCEPTION_INFORMATION info{::GetCurrentThreadId(), eptr, FALSE};
  MiniDumpWriteDump(hProcess, dwProcessId, hDumpFile, MiniDumpWithIndirectlyReferencedMemory, &info, user_streams, NULL);
  return EXCEPTION_EXECUTE_HANDLER;
};
