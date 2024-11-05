// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// required only for blk_writeBBF3.cpp

#define DEBUG_VALIDATION 0

#include <generic/dag_tab.h>
#include <generic/dag_DObject.h>
#include <memory/dag_mem.h>
#include <util/dag_string.h>


class IGenSave;
class IGenLoad;

namespace obsolete
{

/// This class maps strings to integers and back.
/// NULL is mapped to -1.
/// All other strings are mapped to non-negative numbers (>=0).
template <int PUSH_RESERVE = 16>
class HashNameMapT : public DObject
{
public:
  static const int MAX_STRING_SIZE = 0x7fff; // since size is written/read as signed short

public:
  HashNameMapT(unsigned hashBucketSize, IMemAlloc *mem = tmpmem, IMemAlloc *str_mem = strmem) : buckets(mem), strMem(str_mem), num(0)
  {
    buckets.resize(hashBucketSize);
    for (int i = 0; i < buckets.size(); i++)
      dag::set_allocator(buckets[i], mem);
  }

  virtual ~HashNameMapT()
  {
    clear();
    clear_and_shrink(buckets);
  }

  ///  Returns -1 if not found.
  int getNameId(const char *name) const
  {
    if (!name)
    {
#if DEBUG_VALIDATION
      G_ASSERT(validate.getNameId(name) == -1);
#endif
      return -1;
    }

    // hash lookup
    unsigned hash = string_hash(name) % buckets.size();
    const Tab<char *> &names = buckets[hash];

    // linear lookup through the bucket
    for (int i = 0; i < names.size(); ++i)
      if (strcmp(names[i], name) == 0)
        return i * buckets.size() + hash;

    return -1;
  }

  ///  erase item if found.
  int erase(const char *name)
  {
    G_ASSERT(name);

    // hash lookup
    unsigned hash = string_hash(name) % buckets.size();
    Tab<char *> &names = buckets[hash];

    // linear lookup through the bucket
    for (int i = 0; i < names.size(); ++i)
    {
      char *cname = names[i];
      if (strcmp(cname, name) == 0)
      {
        memfree(cname, strMem);
        num--;
        erase_items(names, i, 1);
        return i * buckets.size() + hash;
      }
    }

    return -1;
  }

  /// Returns -1 if NULL. Adds name to the list if not found.
  int addNameId(const char *name)
  {
    if (!name)
      return -1;

    // hash lookup
    unsigned hash = string_hash(name) % buckets.size();
    Tab<char *> &names = buckets[hash];

    // linear lookup through the bucket
    int i;
    for (i = 0; i < names.size(); ++i)
      if (strcmp(names[i], name) == 0)
        return i * buckets.size() + hash;

    // push to the bucket
    char *s = str_dup(name, strMem);
    i = names.size();
    names.push_back(s);
    num++;
    return i * buckets.size() + hash;
  }


  const char *class_name() const override { return "HashNameMapT"; }

  /// To be used instead of private copy constructor.
  void copyFrom(const HashNameMapT &nm)
  {
    clear();
    G_ASSERT(0 == num);
    buckets.resize(nm.buckets.size());
    for (int j = 0; j < buckets.size(); ++j)
    {
      Tab<char *> &names = buckets[j];
      const Tab<char *> &nmNames = nm.buckets[j];

      clear_and_shrink(names);
      dag::set_allocator(names, dag::get_allocator(buckets));
      names.resize(nmNames.size());
      num += nmNames.size();

      for (int i = 0; i < names.size(); ++i)
        names[i] = str_dup(nmNames[i], strMem);
    }

    G_ASSERT(nm.num == num);
  }


  /// Clear name list.
  void clear()
  {
    for (int j = 0; j < buckets.size(); ++j)
    {
      Tab<char *> &names = buckets[j];

      for (int i = 0; i < names.size(); ++i)
      {
        char *name = names[i];
        if (name)
          memfree(name, strMem);
      }

      num -= names.size();
      clear_and_shrink(names);
    }

    G_ASSERT(0 == num);
    num = 0;
  }

  void shrink()
  {
    buckets.shrink_to_fit();
    for (int i = 0; i < buckets.size(); i++)
      buckets[i].shrink_to_fit();
  }

  // djb2 hash.
  static inline unsigned string_hash(const char *name)
  {
    const unsigned char *str = (const unsigned char *)name;
    unsigned hash = 5381;

    while (*str)
      hash = hash * 33 + *str++;

    return hash;
  }

  /// Number of names in list.
  /// You can't get them all by iterating from 0 to nameCount()-1.
  bool isEmpty() const
  {
    G_ASSERT(num >= 0);
    return num <= 0;
  }

  int nameCount() const { return num; }

  /// Returns NULL when name_id is invalid.
  const char *getName(int i) const
  {
    if (i < 0)
      return NULL;

    // hash table indexing
    const Tab<char *> &names = buckets[i % buckets.size()];
    i /= buckets.size();
    if (i >= 0 && i < names.size())
      return names[i];
    else
      return NULL;
  }

  void rebaseNames(const char *old_mem_st, const char *old_mem_end, char *new_mem_st)
  {
    for (Tab<char *> *bs = buckets.data(), *be = bs + buckets.size(); bs < be; bs++)
      for (char **ns = bs->data(), **ne = ns + bs->size(); ns < ne; ns++)
        if (*ns >= old_mem_st && *ns < old_mem_end)
          *ns += new_mem_st - old_mem_st;
  }

protected:
  Tab<Tab<char *>> buckets;
  IMemAlloc *strMem;
  int num;


private:
  HashNameMapT(const HashNameMapT &nm) { copyFrom(nm); }
  HashNameMapT &operator=(const HashNameMapT &nm)
  {
    copyFrom(nm);
    return *this;
  }
};


typedef HashNameMapT<8> HashNameMapType;

class HashNameMapBucketsConv
{
public:
  bool isEnabled() const { return !tab.empty(); }

public:
  Tab<Tab<int>> tab;
};

class HashNameMap : public HashNameMapType
{
public:
  static const int MAX_STRING_COUNT = 0xffff;

  // for compatibility
  HashNameMap(unsigned hashBucketSize, IMemAlloc *m = tmpmem, IMemAlloc *m2 = strmem) : HashNameMapType(hashBucketSize, m, m2) {}

  /// Save this name map.
  void save(IGenSave &, bool save_le) const;

  /// Save this name map (compact version)
  void saveCompact(IGenSave &cwr, bool save_le) const;

  static bool hashValueCheckingOnLoad;
};

} // namespace obsolete
