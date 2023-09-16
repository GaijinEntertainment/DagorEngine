#pragma once

#include <EASTL/vector_set.h>
#include <EASTL/fixed_vector.h>


namespace darg
{

class Element;

class ElemGroup
{
public:
  ElemGroup() = default;
  ~ElemGroup();

  void addChild(Element *);
  void removeChild(Element *);

  void setStateFlags(int f);
  void clearStateFlags(int f);

private:
  template <typename T, size_t N>
  using FixedVecSet = eastl::vector_set<T, eastl::less<T>, EASTLAllocatorType, eastl::fixed_vector<T, N, /*overflow*/ true>>;

  FixedVecSet<Element *, 2> children;
};


} // namespace darg
