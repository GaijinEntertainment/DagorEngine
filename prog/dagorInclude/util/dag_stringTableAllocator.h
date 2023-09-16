//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_adjpow2.h>
#include <EASTL/unique_ptr.h>
#include <util/dag_compilerDefs.h>
#include <generic/dag_smallTab.h>

#pragma pack(push, 4)
struct StringTableAllocator
{
  enum
  {
    DEFAULT_PAGE_SIZE_SHIFT = 8,
    MAX_PAGE_SIZE_SHIFT = 13
  }; // by default we start with 256 bytes, and increase 'page' size up to 8192
  struct StringPage
  {
    eastl::unique_ptr<char[]> data;
    uint32_t left;
    uint32_t used;
    bool empty() const { return bool(data); }
    StringPage() = default;
    StringPage(eastl::unique_ptr<char[]> &&d, uint32_t l, uint32_t u) : data(eastl::move(d)), left(l), used(u) {}
    StringPage(StringPage &&a) : data(eastl::move(a.data)), left(a.left), used(a.used) { a.reset(); }
    StringPage &operator=(StringPage &&a)
    {
      data = eastl::move(a.data);
      left = eastl::move(a.left);
      used = eastl::move(a.used);
      a.reset();
      return *this;
    }
    void reset()
    {
      left = used = 0;
      data.reset();
    }
    StringPage(const StringPage &a) { *this = a; }
    StringPage &operator=(const StringPage &a)
    {
      if (this == &a)
        return *this;
      left = a.left;
      used = a.used;
      data.reset(new char[left + used]);
      memcpy(data.get(), a.data.get(), left + used);
      return *this;
    }
  };

  inline uint32_t get_page_ofs(uint32_t addr) const { return addr & ((1 << max_page_shift) - 1); }
  inline uint32_t get_page_id(uint32_t addr) const { return addr >> max_page_shift; }
  inline void get_page_addr(uint32_t addr, uint32_t &page_id, uint32_t &page_ofs) const
  {
    page_id = get_page_id(addr);
    page_ofs = get_page_ofs(addr);
  }
  StringPage *addPageUnsafe(uint32_t len, char *data)
  {
    if (head.data || pages.size())
      pages.emplace_back(eastl::move(head));
    head = StringPage{eastl::unique_ptr<char[]>(data), len, 0};
    return &head;
  }
  StringPage *addPageUnsafe(uint32_t len) { return addPageUnsafe(len, new char[len]); }
  uint8_t min8(uint8_t a, uint8_t b) { return a < b ? a : b; }
  uint8_t max8(uint8_t a, uint8_t b) { return a > b ? a : b; }
  DAGOR_NOINLINE StringPage *addPage(uint32_t len)
  {
    if (!head.data && pages.empty())
    {
      // page_shift = start_page_shift = (uint8_t)get_bigger_log2(len);
      page_shift = start_page_shift = max8(start_page_shift, get_bigger_log2(len));
      if (page_shift > max_page_shift)
        max_page_shift = page_shift;
      return addPageUnsafe(1 << page_shift);
    }
    for (;;)
    {
      page_shift = min8(page_shift + 1, max_page_shift);
      if (len > (1 << max_page_shift) && page_shift == max_page_shift)
        return addPageUnsafe(len);
      if (len <= (1 << page_shift))
        return addPageUnsafe(1 << page_shift);
      addPageUnsafe(0, NULL); // save some memory, 'empty' page allocated
    }
  }

  StringPage head = {nullptr, 0, 0};
  dag::Vector<StringPage> pages;
  uint8_t start_page_shift = DEFAULT_PAGE_SIZE_SHIFT;
  uint8_t page_shift = DEFAULT_PAGE_SIZE_SHIFT;
  uint8_t max_page_shift = MAX_PAGE_SIZE_SHIFT;
  uint8_t padding = 0;
  __forceinline void addDataRaw_(const char *name, size_t len)
  {
    if (DAGOR_UNLIKELY(head.left < len))
      addPage((uint32_t)len);
    memcpy(head.data.get() + head.used, name, len);
  }
  uint32_t getCurrentHeadOffset() const
  {
    const uint32_t pagesCount = (uint32_t)pages.size();
    return (pagesCount << max_page_shift) + head.used;
  }
  uint32_t addDataRaw(const char *name, size_t len)
  {
    addDataRaw_(name, len);
    const uint32_t ret = getCurrentHeadOffset();
    head.used += (uint32_t)len;
    head.left -= (uint32_t)len;
    return ret;
  }
  const char *getDataRawUnsafe(uint32_t ofs, uint32_t &max_len) const // returns pointer to data, if ofs was previously returned by
                                                                      // addDataRaw, max_len is maximum possible allocated size
  {
    uint32_t pageId, pageOfs;
    get_page_addr(ofs, pageId, pageOfs);
    if (pageId == pages.size())
    {
      max_len = head.used + head.left - pageOfs;
      return head.data.get() + pageOfs;
    }
    auto &page = pages[pageId];
    max_len = page.used + page.left - pageOfs;
    return page.data.get() + pageOfs;
  }
  const char *getDataRawUnsafe(uint32_t ofs) const // returns pointer to data, if ofs was previously returned by addDataRaw
  {
    uint32_t pageId, pageOfs;
    get_page_addr(ofs, pageId, pageOfs);
    return (pageId == pages.size() ? head.data.get() : pages[pageId].data.get()) + pageOfs;
  }
  size_t totalAllocated() const
  {
    size_t ret = head.used + head.left;
    for (auto &p : pages)
      ret += p.used + p.left;
    return ret;
  }
  size_t totalUsed() const
  {
    size_t ret = head.used;
    for (auto &p : pages)
      ret += p.used;
    return ret;
  }
  void shrink_to_fit() { pages.shrink_to_fit(); }
  void clear()
  {
    head = StringPage{nullptr, 0, 0};
    pages.clear();
  }
  StringTableAllocator(uint8_t start_page_shift_ = DEFAULT_PAGE_SIZE_SHIFT, uint8_t max_page_shift_ = MAX_PAGE_SIZE_SHIFT) :
    start_page_shift(start_page_shift_), page_shift(start_page_shift_), max_page_shift(max_page_shift_)
  {}
};
#pragma pack(pop)
