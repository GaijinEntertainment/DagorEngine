// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <string.h>
#include <stdio.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_basePath.h>
#include <EASTL/bitvector.h>
#include <EASTL/sort.h>

#include "../daProfilerDumpUtils.h"
#include <windows.h>
#include <dbghelp.h>
#include <dbghelp.h>
#include <tlhelp32.h>
#include <Psapi.h>

#pragma comment(lib, "psapi.lib")

using namespace da_profiler;

static void close(HANDLE proc) { SymCleanup(proc); }
static BOOL debug_sym(HANDLE hProcess, ULONG ActionCode, PVOID d, PVOID)
{
  if (ActionCode == CBA_DEBUG_INFO)
    printf("%s", d);
  return FALSE;
}

static bool init(HANDLE proc, const char *pdb_search_path)
{
  SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST | SYMOPT_AUTO_PUBLICS | SYMOPT_DEBUG);
  if (!SymInitialize(proc, pdb_search_path, false))
    return false;
  // SymRegisterCallback(proc, (PSYMBOL_REGISTERED_CALLBACK)debug_sym, 0);
  printf("add symbols search path<%s>\n", pdb_search_path);
  SymSetSearchPath(proc, pdb_search_path);
  return true;
}

bool load_module(HANDLE proc, const char *module_name, uint64_t base_addr, uint64_t module_size)
{
  // printf("add module <%s>\n", module_name);
  return SymLoadModule(proc, NULL, (char *)module_name, NULL, base_addr, module_size) || GetLastError() == ERROR_SUCCESS;
}

static bool unload_module(const char *module_name, uint64_t base_addr) { return SymUnloadModule((char *)module_name, base_addr); }

extern bool stackhlp_get_symbol(void *addr, uint32_t &line, char *filename, size_t max_file_name, char *symbolname,
  size_t max_symbol_name);
static int show_usage();

int _cdecl main(int argc, char **argv)
{
  if (argc < 4)
    return show_usage();
  dd_add_base_path("");
  FullFileLoadCB lcb(argv[1]);
  if (!lcb.fileHandle)
  {
    printf("can't open file <%s>\n", argv[1]);
    return 1;
  }
  FullFileSaveCB scb(argv[2]);
  if (!scb.fileHandle)
  {
    printf("can't open file <%s>\n", argv[2]);
    return 1;
  }
  HANDLE proc = GetCurrentProcess();
  if (!init(proc, argv[3]))
  {
    printf("cant init\n");
    return 2;
  }

  for (;;)
  {
    SymbolsCache symbols;
    SymbolsSet symbolsSet;
    int symbolsSetBoard = -1;
    bool hasDescBoard = false;
    if (!file_to_file_dump(lcb, scb, [&](IGenLoad &cb, IGenSave &save_cb, const DataResponse &resp) {
          if (resp.type == DataResponse::CallstackPack)
          {
            save_cb.write(&resp, sizeof(resp));
            const int board = cb.readInt();
            if (board != symbolsSetBoard)
            {
              symbolsSetBoard = board;
              symbolsSet.clear();
            }
            save_cb.writeInt(board);
            for (;;)
            {
              uint64_t count = cb.readInt64();
              save_cb.writeInt64(count);
              if (count == ~0ULL)
                break;
              save_cb.writeInt64(cb.readInt64());  // thread
              save_cb.writeInt64(cb.readInt64());  // tick
              for (uint32_t j = 0; j < count; ++j) // store addresses. that should not be nesessary, we better establish communications
                                                   // with server or save minidump if not live
              {
                uint64_t addr = cb.readInt64();
                if (addr)
                  symbolsSet.insert(addr);
                save_cb.writeInt64(addr);
              }
            }
            return ResponseProcessorResult::Skip; // already saved
          }
          else if (resp.type == DataResponse::CallstackDescriptionBoard || (resp.type == DataResponse::NullFrame && !hasDescBoard))
          {
            DynamicMemGeneralSaveCB mem(tmpmem_ptr(), 65536, 65536);
            int board = symbolsSetBoard;
            if (resp.type == DataResponse::CallstackDescriptionBoard)
            {
              hasDescBoard = true;
              board = cb.readInt();
              read_modules(cb, symbols);
              read_symbols(cb, symbols);
              eastl::sort(symbols.modules.begin(), symbols.modules.end(), [](auto &a, auto &b) { return a.base < b.base; });
              eastl::bitvector<> modulesResolved;
              modulesResolved.resize(symbols.modules.size());
              auto resolveModule = [&](uint64_t addr) {
                auto i = eastl::find_if(symbols.modules.begin(), symbols.modules.end(),
                  [&](auto a) { return a.base <= addr && addr < a.base + a.size; });
                if (i != symbols.modules.end() && !modulesResolved[size_t(i - symbols.modules.begin())])
                {
                  modulesResolved.set(i - symbols.modules.begin(), true);
                  if (!load_module(proc, i->name, i->base, i->size))
                    printf("can't load module %s (%llx + %lld)\n", i->name, (long long)i->base, (long long)i->size);
                }
              };
              // load just used modules
              symbolsSet.iterate([&](uint64_t addr) { resolveModule(addr); });
              symbols.symbolMap.iterate([&](uint64_t addr, uint32_t) { resolveModule(addr); });
              /*//resolve all modules
              for (auto &m:symbols.modules)
              {
                if (!load_module(proc, m.name, m.base, m.size))
                  printf("can't load module %s (%llx + %lld)\n", m.name, (long long)m.base, (long long)m.size);
              }*/
            }
            uint32_t was_missing = 0;
            printf("resolving symbols..\n");
            uint32_t resolved = symbols.resolveUnresolved(symbolsSet, was_missing);
            printf("resolved %d symbols out of %d. %d are still missing\n", resolved, was_missing, was_missing - resolved);

            {
              mem.writeInt(board);
              write_modules(mem, symbols);
              write_symbols(mem, symbolsSet, symbols);
              send_data(save_cb, DataResponse::CallstackDescriptionBoard, mem);
            }

            for (auto &m : symbols.modules)
              unload_module(m.name, m.base);
            return ResponseProcessorResult::Skip;
          }
          else
            return ResponseProcessorResult::Copy;
        }))
      break;
  }

  return 0;
}
static int show_usage()
{
  printf("usage: source-path.dap dest-path.dap path-to-folder-with-pdb-and-exe\n");
  return 1;
}
