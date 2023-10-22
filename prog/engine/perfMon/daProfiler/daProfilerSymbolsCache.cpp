#include "daProfilerInternal.h"
#include "daProfilePlatform.h"

namespace da_profiler
{

size_t SymbolsCache::memAllocated() const
{
  std::lock_guard<std::mutex> glock(lock);
  return symbolMap.bucket_count() * (sizeof(uint64_t) + sizeof(uint32_t)) + modules.capacity() * sizeof(ModuleInfo) +
         symbols.capacity() * sizeof(SymbolInfo) + strings.totalAllocated();
}

void SymbolsCache::initModules()
{
  if (!modules.empty())
    return;
  stackhlp_enum_modules([&](const char *n, size_t base, size_t size) {
    modules.emplace_back(SymbolsCache::ModuleInfo{base, size, strings.addName(n)});
    return true;
  });
}

uint32_t SymbolsCache::addSymbol(uint64_t addr)
{
  uint32_t index = symbolMap.findOr(addr, ~0u);
  if (DAGOR_LIKELY(index != ~0u))
    return index;
  char fun[256], file[256];
  uint32_t line = 0;
  if (stackhlp_get_symbol((void *)addr, line, file, sizeof(file), fun, sizeof(fun)))
  {
    index = symbols.size();
    symbols.emplace_back(SymbolInfo{strings.addNameId(fun), strings.addNameId(file), line});
  }

  symbolMap.emplace(addr, index);
  return index;
}

uint32_t SymbolsCache::resolveUnresolved(const SymbolsSet &set, uint32_t &was_missing)
{
  set.iterate([&](uint64_t addr) {
    auto it = symbolMap.emplace_if_missing(addr);
    if (it.second)
      *it.first = ~0u;
  });
  uint32_t cnt = symbols.size();
  was_missing = 0;
  symbolMap.iterate([&](uint64_t addr, uint32_t &ind) {
    if (ind != ~0u)
      return;
    ++was_missing;
    char fun[256], file[256];
    uint32_t line = 0;
    if (stackhlp_get_symbol((void *)addr, line, file, sizeof(file), fun, sizeof(fun)))
    {
      ind = symbols.size();
      symbols.emplace_back(SymbolInfo{strings.addNameId(fun), strings.addNameId(file), line});
    }
  });
  return symbols.size() - cnt;
}

}; // namespace da_profiler