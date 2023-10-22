#include <util/dag_fastStrMap.h>

#ifdef _TARGET_STATIC_LIB
template class FastStrMapT<int, -1>;
template class FastStrMapT<int, 0>;
template class FastStrMapT<char *, 0>;
#endif

FastStrStrMap::FastStrStrMap(IMemAlloc *mem, bool ignore_case) : FastStrStrMap_(mem, ignore_case) {}
FastStrStrMap::FastStrStrMap(const FastStrStrMap &m) : FastStrStrMap_(m)
{
  for (int i = 0; i < fastMap.size(); i++)
    if (fastMap[i].id)
      fastMap[i].id = str_dup(fastMap[i].id, dag::get_allocator(fastMap));
}
FastStrStrMap::~FastStrStrMap() { reset(false); }

FastStrStrMap::IdType FastStrStrMap::addStrId(const char *str, FastStrStrMap::IdType id)
{
  IdType str_id = getStrId(str);
  if (!str_id)
  {
    str_id = id ? str_dup(id, dag::get_allocator(fastMap)) : id;
    Entry key = {str_dup(str, dag::get_allocator(fastMap)), str_id};
    if (ignoreCase)
      fastMap.insert(key, LambdaStricmp(), 32);
    else
      fastMap.insert(key, LambdaStrcmp(), 32);
  }

  return str_id;
}

bool FastStrStrMap::setStrId(const char *str, FastStrStrMap::IdType id)
{
  int idx = getStrIndex(str);

  if (idx == -1)
  {
    Entry key = {str_dup(str, dag::get_allocator(fastMap)), id ? str_dup(id, dag::get_allocator(fastMap)) : id};
    if (ignoreCase)
      fastMap.insert(key, LambdaStricmp(), 32);
    else
      fastMap.insert(key, LambdaStrcmp(), 32);
    return true;
  }
  else
  {
    if (fastMap[idx].id)
      memfree((void *)fastMap[idx].id, dag::get_allocator(fastMap));
    fastMap[idx].id = id ? str_dup(id, dag::get_allocator(fastMap)) : id;
    return false;
  }
}

bool FastStrStrMap::delStrId(const char *str)
{
  int idx = getStrIndex(str);
  if (idx == -1)
    return false;
  memfree((void *)fastMap[idx].name, dag::get_allocator(fastMap));
  if (fastMap[idx].id)
    memfree((void *)fastMap[idx].id, dag::get_allocator(fastMap));
  erase_items(fastMap, idx, 1);
  return true;
}

bool FastStrStrMap::delStrId(FastStrStrMap::IdType str_id)
{
  int cnt = fastMap.size();
  for (int i = fastMap.size() - 1; i >= 0; i--)
    if (fastMap[i].id == str_id)
    {
      memfree((void *)fastMap[i].name, dag::get_allocator(fastMap));
      if (fastMap[i].id)
        memfree((void *)fastMap[i].id, dag::get_allocator(fastMap));
      erase_items(fastMap, i, 1);
    }
  return fastMap.size() < cnt;
}

void FastStrStrMap::reset(bool erase_only)
{
  for (int i = fastMap.size() - 1; i >= 0; i--)
    if (fastMap[i].id)
      memfree((void *)fastMap[i].id, dag::get_allocator(fastMap));
  FastStrStrMap_::reset(erase_only);
}

#define EXPORT_PULL dll_pull_baseutil_fastStrMap
#include <supp/exportPull.h>
