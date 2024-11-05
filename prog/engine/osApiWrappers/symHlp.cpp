// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_symHlp.h>

#if _TARGET_PC_WIN
#include <osApiWrappers/dag_direct.h>
#include <util/dag_globDef.h>
#include <windows.h>
#include <dbghelp.h>
#include <dbghelp.h>
#include <tlhelp32.h>
#include <Psapi.h>
// #include <debug/dag_debug.h>

#pragma comment(lib, "psapi.lib")

static char pdb_search_path[4096] = ".;";
void symhlp_close()
{
  HANDLE proc = GetCurrentProcess();
  SymCleanup(proc);
}
bool symhlp_init(bool in)
{
  HANDLE proc = GetCurrentProcess();
  if (SymInitialize(proc, pdb_search_path, in))
  {
    SymSetOptions(SYMOPT_DEFERRED_LOADS);
    return true;
  }
  return false;
}

void symhlp_init_default()
{
  symhlp_init();

  char exe_path[DAGOR_MAX_PATH];
  GetModuleFileNameA(GetModuleHandle(NULL), exe_path, countof(exe_path));
  symhlp_load(exe_path);

  static const char *const knownModules[] = {"ntdll.dll", "kernelbase.dll", "kernel32.dll", "user32.dll"};
  for (int i = 0; i < countof(knownModules); ++i)
  {
    MODULEINFO mi;
    HMODULE hmod = GetModuleHandle(knownModules[i]);
    if (hmod && GetModuleInformation(GetCurrentProcess(), hmod, &mi, sizeof(mi)))
      symhlp_load(knownModules[i], (uintptr_t)mi.lpBaseOfDll, mi.SizeOfImage);
  }
}

bool symhlp_find_module_addr(const char *pe_img_name, uintptr_t &base_addr, uintptr_t &module_size)
{
  HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
  MODULEENTRY32 me32;

  //  Take a snapshot of all modules in the specified process.
  hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
  if (hModuleSnap == INVALID_HANDLE_VALUE)
    return false;

  //  Set the size of the structure before using it.
  me32.dwSize = sizeof(MODULEENTRY32);

  //  Retrieve information about the first module,
  //  and exit if unsuccessful
  if (!Module32First(hModuleSnap, &me32))
  {
    CloseHandle(hModuleSnap); // Must clean up the snapshot object!
    return false;
  }

  //  Now walk the module list of the process,
  //  and search for specified module
  do
  {
    /*
    debug( "\n     MODULE NAME:     %s",           me32.szModule );
    debug( "     executable     = %s",             me32.szExePath );
    debug( "     process ID     = 0x%08X",         me32.th32ProcessID );
    debug( "     ref count (g)  =     0x%04X",     me32.GlblcntUsage );
    debug( "     ref count (p)  =     0x%04X",     me32.ProccntUsage );
    debug( "     base address   = 0x%08X", (DWORD) me32.modBaseAddr );
    debug( "     base size      = %d",             me32.modBaseSize );
    */

    if (dd_fname_equal(me32.szExePath, pe_img_name) || dd_fname_equal(me32.szModule, pe_img_name))
    {
      CloseHandle(hModuleSnap);
      base_addr = (uintptr_t)me32.modBaseAddr;
      module_size = me32.modBaseSize;
      return true;
    }

  } while (Module32Next(hModuleSnap, &me32));

  //  Do not forget to clean up the snapshot object.
  CloseHandle(hModuleSnap);
  return false;
}

bool symhlp_load(const char *pe_img_name, uintptr_t base_addr, uintptr_t module_size)
{
  if (base_addr == 0)
    symhlp_find_module_addr(pe_img_name, base_addr, module_size);
  // DEBUG_CTX("symhlp_load: <%s> base=%08X size=%08X", pe_img_name, base_addr, module_size);
  char pdb_dir[DAGOR_MAX_PATH];
  dd_get_fname_location(pdb_dir, pe_img_name);
  strcat(pdb_dir, ";");
  if (!strstr(pdb_search_path, pdb_dir) && strlen(pdb_search_path) + strlen(pdb_dir) < sizeof(pdb_search_path))
  {
    strcat(pdb_search_path, pdb_dir);
    SymSetSearchPath(GetCurrentProcess(), pdb_search_path);
  }

  return SymLoadModule(GetCurrentProcess(), NULL, (char *)pe_img_name, NULL, base_addr, module_size);
}

bool symhlp_unload(const char *pe_img_name, uintptr_t base_addr)
{
  uintptr_t module_size;

  if (base_addr == 0)
    symhlp_find_module_addr(pe_img_name, base_addr, module_size);
  // DEBUG_CTX("symhlp_unload: <%s> %08X %d", pe_img_name, base_addr);
  if (base_addr == 0)
    return false;
  return SymUnloadModule((char *)pe_img_name, base_addr);
}

#else
bool symhlp_load(const char *, uintptr_t, uintptr_t) { return true; }
bool symhlp_unload(const char *, uintptr_t) { return true; }
void symhlp_close() {}
bool symhlp_init(bool /*in*/) { return false; }
void symhlp_init_default() {}

#endif

#define EXPORT_PULL dll_pull_osapiwrappers_symHlp
#include <supp/exportPull.h>
