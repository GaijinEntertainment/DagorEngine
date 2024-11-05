// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqModules/sqModules.h>
#include <osApiWrappers/dag_unicode.h>
#include <memory/dag_framemem.h>
#include <sqrat.h>

#if _TARGET_PC_WIN
#include <Windows.h>
#endif

#if _TARGET_PC_WIN
static PVOID resolve_pe_export_sym(const uint8_t *map_base, size_t img_base, const char *api_name)
{
  auto dhdr = (const IMAGE_DOS_HEADER *)map_base;
  G_ASSERT(dhdr->e_magic == IMAGE_DOS_SIGNATURE);
  auto nthdr = (const IMAGE_NT_HEADERS *)(map_base + dhdr->e_lfanew);
  G_ASSERT(nthdr->Signature == IMAGE_NT_SIGNATURE);
#ifdef _WIN64
  G_ASSERT_RETURN(nthdr->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64, NULL);
#else
  G_ASSERT_RETURN(nthdr->FileHeader.Machine == IMAGE_FILE_MACHINE_I386, NULL);
#endif
  if (auto erva = nthdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress)
  {
    auto exports = (const IMAGE_EXPORT_DIRECTORY *)(map_base + erva);
    auto AddressOfFunctions = (const uint32_t *)(map_base + exports->AddressOfFunctions);
    auto AddressOfNames = (const uint32_t *)(map_base + exports->AddressOfNames);
    auto AddressOfNameOrdinals = (const uint16_t *)(map_base + exports->AddressOfNameOrdinals);
    for (DWORD i = 0; i < exports->NumberOfNames; i++)
      if (strcmp(api_name, ((char *)map_base + AddressOfNames[i])) == 0)
        return (void *)(img_base + AddressOfFunctions[AddressOfNameOrdinals[i]]);
  }
  return NULL;
}
static int is_executable_exports_symbol(const char *exe_name, const char *sym_name)
{
  Tab<wchar_t> tmpwc(framemem_ptr());
  HMODULE hmod =
    LoadLibraryExW(convert_utf8_to_u16_buf(tmpwc, exe_name, -1), NULL, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
  if (!hmod)
    return -1;
  void *sym = resolve_pe_export_sym((PBYTE)((uintptr_t)hmod & -4), 0, sym_name);
  FreeLibrary(hmod);
  return sym ? 1 : 0;
}
#else
static int is_executable_exports_symbol(const char *, const char *) { return -1; }
#endif

namespace bindquirrel
{

void bind_executable(SqModules *module_mgr)
{
  Sqrat::Table nsTbl(module_mgr->getVM());
  ///@module executable
  nsTbl.Func("is_exports_symbol", is_executable_exports_symbol);
  module_mgr->addNativeModule("executable", nsTbl);
}

} // namespace bindquirrel
