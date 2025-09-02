// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "commonUtils.h"
#include "shaderTab.h"
#include <debug/dag_assert.h>
#include <dag/dag_vector.h>
#include <EASTL/string.h>
#include <EASTL/algorithm.h>

class StcodeBuilder
{
public:
  StcodeBuilder() = default;
  StcodeBuilder(StcodeBuilder &&other) = default;
  StcodeBuilder &operator=(StcodeBuilder &&other) = default;

  NON_COPYABLE_TYPE(StcodeBuilder)

  void pushBack(eastl::string &&str) { m_fragments.push_back(eastl::move(str)); }

  template <class... Args>
  void emplaceBack(Args &&...args)
  {
    m_fragments.push_back(eastl::string(eastl::forward<Args>(args)...));
  }

  template <class... Args>
  void emplaceBackFmt(const char *fmt, Args &&...args)
  {
    m_fragments.push_back(string_f(fmt, eastl::forward<Args>(args)...));
  }

  void pushFront(eastl::string &&str) { m_fragments.insert(m_fragments.cbegin(), eastl::move(str)); }

  eastl::string release()
  {
    size_t size = 0;
    for (eastl::string &fragp : m_fragments)
      size += fragp.length();
    eastl::string res;
    res.resize(size);
    char *p = res.data();
    for (auto &fragp : m_fragments)
    {
      eastl::copy(fragp.cbegin(), fragp.cend(), p);
      p += fragp.length();
    }
    m_fragments.clear();
    return res;
  }

  void clear() { m_fragments.clear(); }

  bool empty() const { return m_fragments.empty(); }
  size_t fragCount() const { return m_fragments.size(); }

  void merge(StcodeBuilder &&other)
  {
    m_fragments.insert(m_fragments.end(), eastl::make_move_iterator(other.m_fragments.begin()),
      eastl::make_move_iterator(other.m_fragments.end()));
    other.m_fragments.clear();
  }

  template <class TClb>
  void iterateFragments(TClb &&cb)
  {
    for (const auto &frag : m_fragments)
      cb(frag);
  }

private:
  Tab<eastl::string> m_fragments{};
};
