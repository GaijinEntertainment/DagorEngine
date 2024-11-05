// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

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

  // Non-copyable
  StcodeBuilder(const StcodeBuilder &other) = delete;
  StcodeBuilder &operator=(const StcodeBuilder &other) = delete;

  void pushBack(eastl::string &&str) { m_fragments.push_back(eastl::move(str)); }

  template <class... Args>
  void emplaceBack(Args &&...args)
  {
    m_fragments.push_back(eastl::string(eastl::forward<Args>(args)...));
  }

  template <class... Args>
  void emplaceBackFmt(const char *fmt, Args &&...args)
  {
    m_fragments.push_back(eastl::string(eastl::string::CtorSprintf{}, fmt, eastl::forward<Args>(args)...));
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

  void merge(StcodeBuilder &&other)
  {
    m_fragments.insert(m_fragments.end(), eastl::make_move_iterator(other.m_fragments.begin()),
      eastl::make_move_iterator(other.m_fragments.end()));
    other.m_fragments.clear();
  }

private:
  dag::Vector<eastl::string> m_fragments{};
};
