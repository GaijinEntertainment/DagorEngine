
#ifndef VERTEX_CACHE_H

#define VERTEX_CACHE_H

#include "tristrip.h"

namespace TriStripifier
{

class VertexCache
{

public:
  VertexCache(int size)
  {
    numEntries = size;

    for (int i = 0; i < numEntries; i++)
      entries[i] = -1;
  }

  VertexCache() : VertexCache(16) {}
  ~VertexCache() {}

  bool InCache(int entry)
  {
    if (numEntries >= 1000000)
      return true;

    bool returnVal = false;
    for (int i = 0; i < numEntries; i++)
    {
      if (entries[i] == entry)
      {
        returnVal = true;
        break;
      }
    }

    return returnVal;
  }

  int AddEntry(int entry)
  {
    int removed;

    removed = entries[numEntries - 1];

    // push everything right one
    for (int i = numEntries - 2; i >= 0; i--)
    {
      entries[i + 1] = entries[i];
    }

    entries[0] = entry;

    return removed;
  }

  void Clear() { memset(entries, -1, sizeof(int) * numEntries); }

  void Copy(VertexCache *inVcache)
  {
    for (int i = 0; i < numEntries; i++)
    {
      inVcache->Set(i, entries[i]);
    }
  }

  int At(int index) { return entries[index]; }
  void Set(int index, int value) { entries[index] = value; }

private:
  int entries[PublicStripifier::CACHESIZE_MAXIMUM];
  int numEntries;
};

}; // namespace TriStripifier

#endif
