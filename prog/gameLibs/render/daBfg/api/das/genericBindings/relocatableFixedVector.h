// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_relocatableFixedVector.h>
#include <dasModules/dasModulesCommon.h>
#include <dag/dag_relocatable.h>


namespace das
{

template <typename TT, size_t inplace_count>
struct das_default_vector_size<dag::RelocatableFixedVector<TT, inplace_count>>
{
  static __forceinline uint32_t size(const dag::RelocatableFixedVector<TT, inplace_count> &value) { return uint32_t(value.size()); }
};

template <typename TT, size_t inplace_count>
struct typeName<dag::RelocatableFixedVector<TT, inplace_count>>
{
  static string name() { return string("fixedVector`") + typeName<TT>::name() + "`" + to_string(inplace_count); }
};

template <typename TT, size_t inplace_count>
struct ManagedRelocatableFixedVectorAnnotation : ManagedVectorAnnotation<dag::RelocatableFixedVector<TT, inplace_count>>
{
  using ManagedVectorAnnotation<dag::RelocatableFixedVector<TT, inplace_count>>::ManagedVectorAnnotation;
  using ManagedVectorAnnotation<dag::RelocatableFixedVector<TT, inplace_count>>::vecType;

  virtual bool canMove() const override { return vecType->canMove(); }
  virtual bool canCopy() const override { return vecType->canCopy(); }
};

template <typename TT, size_t inplace_count>
struct typeFactory<dag::RelocatableFixedVector<TT, inplace_count>>
{
  using VT = dag::RelocatableFixedVector<TT, inplace_count>;

  static TypeDeclPtr make(const ModuleLibrary &library)
  {
    string declN = typeName<VT>::name();
    if (library.findAnnotation(declN, nullptr).size() == 0)
    {
      auto declT = makeType<TT>(library);
      auto ann = make_smart<ManagedRelocatableFixedVectorAnnotation<TT, inplace_count>>(declN, const_cast<ModuleLibrary &>(library));
      ann->cppName = "dag::RelocatableFixedVector<" + describeCppType(declT) + ", " + to_string(inplace_count) + ">";
      auto mod = library.front();
      mod->addAnnotation(ann);
      registerVectorFunctions<VT>::init(mod, library, declT->canCopy(), declT->canMove());
    }
    return makeHandleType(library, declN.c_str());
  }
};

} // namespace das
