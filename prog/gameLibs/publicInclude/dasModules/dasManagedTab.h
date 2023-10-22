//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <daScript/simulate/simulate_visit_op.h>
#include <daScript/simulate/aot.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <dasModules/aotEcsContainer.h>

#define DAS_TAB_SIZE(CTYPE)                                \
  namespace das                                            \
  {                                                        \
  template <>                                              \
  struct das_default_vector_size<CTYPE>                    \
  {                                                        \
    static __forceinline uint32_t size(const CTYPE &value) \
    {                                                      \
      return uint32_t(value.size());                       \
    }                                                      \
  };                                                       \
  }

#define DAS_TAB_INDEX(CTYPE, ITEM_CTYPE)                                      \
  namespace das                                                               \
  {                                                                           \
  template <>                                                                 \
  struct das_index<CTYPE> : das_default_vector_index<CTYPE, ITEM_CTYPE>       \
  {};                                                                         \
  template <>                                                                 \
  struct das_index<const CTYPE> : das_default_vector_index<CTYPE, ITEM_CTYPE> \
  {};                                                                         \
  }

#define DAS_ARRAY_INDEX(CTYPE, ITEM_CTYPE)                                          \
  namespace das                                                                     \
  {                                                                                 \
  template <>                                                                       \
  struct das_index<CTYPE> : das_array_index<CTYPE, ITEM_CTYPE, CTYPE::size()>       \
  {};                                                                               \
  template <>                                                                       \
  struct das_index<const CTYPE> : das_array_index<CTYPE, ITEM_CTYPE, CTYPE::size()> \
  {};                                                                               \
  }

#define DAS_ARRAY_ITERATOR(CTYPE, ITEM_CTYPE)                                          \
  namespace das                                                                        \
  {                                                                                    \
  template <>                                                                          \
  struct das_iterator<CTYPE> : das_array_iterator<CTYPE, ITEM_CTYPE>                   \
  {                                                                                    \
    __forceinline das_iterator(CTYPE &ar) : das_array_iterator(ar)                     \
    {}                                                                                 \
  };                                                                                   \
  template <>                                                                          \
  struct das_iterator<const CTYPE> : das_array_iterator<const CTYPE, const ITEM_CTYPE> \
  {                                                                                    \
    __forceinline das_iterator(const CTYPE &ar) : das_array_iterator(ar)               \
    {}                                                                                 \
  };                                                                                   \
  }


#define DAS_BIND_STATIC_TAB(TYPE, CTYPE, ITEM_CTYPE, CPP_NAME) \
  DAS_VECTOR_FACTORY(TYPE, CTYPE, ITEM_CTYPE, CPP_NAME)        \
  DAS_TAB_SIZE(CTYPE)                                          \
  DAS_TAB_INDEX(CTYPE, ITEM_CTYPE)                             \
  DAS_ARRAY_ITERATOR(CTYPE, ITEM_CTYPE)

#define DAS_BIND_VECTOR(TYPE, CTYPE, ITEM_CTYPE, CPP_NAME) \
  DAS_VECTOR_FACTORY(TYPE, CTYPE, ITEM_CTYPE, CPP_NAME)    \
  DAS_TAB_INDEX(CTYPE, ITEM_CTYPE)                         \
  DAS_ARRAY_ITERATOR(CTYPE, ITEM_CTYPE)                    \
  DAS_TAB_SIZE(CTYPE)

#define DAS_BIND_VECTOR_SET(TYPE, CTYPE, ITEM_CTYPE, CPP_NAME) \
  DAS_VECTOR_SET_FACTORY(TYPE, CTYPE, ITEM_CTYPE, CPP_NAME)    \
  DAS_TAB_INDEX(CTYPE, ITEM_CTYPE)                             \
  DAS_ARRAY_ITERATOR(CTYPE, ITEM_CTYPE)                        \
  DAS_TAB_SIZE(CTYPE)

#define DAS_BIND_ARRAY(TYPE, CTYPE, ITEM_CTYPE) \
  MAKE_TYPE_FACTORY(TYPE, CTYPE)                \
  DAS_ARRAY_INDEX(CTYPE, ITEM_CTYPE)            \
  DAS_ARRAY_ITERATOR(CTYPE, ITEM_CTYPE)

#define DAS_BIND_SPAN(TYPE, CTYPE, ITEM_CTYPE) \
  MAKE_TYPE_FACTORY(TYPE, CTYPE)               \
  DAS_TAB_INDEX(CTYPE, ITEM_CTYPE)             \
  DAS_ARRAY_ITERATOR(CTYPE, ITEM_CTYPE)

#define DAS_ANNOTATE_VECTOR(TYPE, CTYPE)                                                      \
  struct TYPE##Annotation : das::ManagedVectorAnnotation<CTYPE>                               \
  {                                                                                           \
    TYPE##Annotation(das::ModuleLibrary &ml) : das::ManagedVectorAnnotation<CTYPE>(#TYPE, ml) \
    {                                                                                         \
      cppName = " ::" #CTYPE;                                                                 \
    }                                                                                         \
  };

#define DAS_VECTOR_FACTORY(TYPE, CTYPE, ITEM_CTYPE, CPP_NAME)                                               \
  namespace das                                                                                             \
  {                                                                                                         \
  template <>                                                                                               \
  struct typeName<CTYPE>                                                                                    \
  {                                                                                                         \
    constexpr static const char *name()                                                                     \
    {                                                                                                       \
      return #TYPE;                                                                                         \
    }                                                                                                       \
  };                                                                                                        \
  template <>                                                                                               \
  struct typeFactory<CTYPE>                                                                                 \
  {                                                                                                         \
    static ___noinline TypeDeclPtr make(const ModuleLibrary &library)                                       \
    {                                                                                                       \
      das::string declN = typeName<CTYPE>::name();                                                          \
      if (library.findAnnotation(declN, nullptr).size() == 0)                                               \
      {                                                                                                     \
        auto declT = makeType<ITEM_CTYPE>(library);                                                         \
        auto ann = make_smart<ManagedVectorAnnotation<CTYPE>>(declN, const_cast<ModuleLibrary &>(library)); \
        ann->cppName = CPP_NAME;                                                                            \
        auto mod = library.front();                                                                         \
        mod->addAnnotation(ann);                                                                            \
        das::registerVectorFunctions<CTYPE>::init(mod, library, declT->canCopy(), declT->canMove());        \
      }                                                                                                     \
      return makeHandleType(library, declN.c_str());                                                        \
    }                                                                                                       \
  };                                                                                                        \
  };

#define DAS_VECTOR_SET_FACTORY(TYPE, CTYPE, ITEM_CTYPE, CPP_NAME)                                           \
  namespace das                                                                                             \
  {                                                                                                         \
  template <>                                                                                               \
  struct typeName<CTYPE>                                                                                    \
  {                                                                                                         \
    constexpr static const char *name()                                                                     \
    {                                                                                                       \
      return #TYPE;                                                                                         \
    }                                                                                                       \
  };                                                                                                        \
  template <>                                                                                               \
  struct typeFactory<CTYPE>                                                                                 \
  {                                                                                                         \
    static ___noinline TypeDeclPtr make(const ModuleLibrary &library)                                       \
    {                                                                                                       \
      das::string declN = typeName<CTYPE>::name();                                                          \
      if (library.findAnnotation(declN, nullptr).size() == 0)                                               \
      {                                                                                                     \
        auto declT = makeType<ITEM_CTYPE>(library);                                                         \
        auto ann = make_smart<ManagedVectorAnnotation<CTYPE>>(declN, const_cast<ModuleLibrary &>(library)); \
        ann->cppName = CPP_NAME;                                                                            \
        auto mod = library.front();                                                                         \
        mod->addAnnotation(ann);                                                                            \
        /*registerVectorFunctions<CTYPE>::init(mod, library, declT->canCopy(), declT->canMove()); */        \
        RegisterVecSetFunctions<CTYPE>::init(*mod, library, declT->canCopy(), declT->canMove());            \
      }                                                                                                     \
      return makeHandleType(library, declN.c_str());                                                        \
    }                                                                                                       \
  };                                                                                                        \
  };

namespace das
{
template <typename VectorType>
struct TabIterator : Iterator
{
  TabIterator(VectorType *ar) : array(ar), end(nullptr) {}

  virtual bool first(Context &, char *_value) override
  {
    if (!array->size())
      return false;
    iterator_type *value = (iterator_type *)_value;
    *value = &(*array)[0];
    end = array->end();
    return true;
  }

  virtual bool next(Context &, char *_value) override
  {
    iterator_type *value = (iterator_type *)_value;
    ++(*value);
    return *value != end;
  }

  virtual void close(Context &context, char *_value) override
  {
    if (_value)
    {
      iterator_type *value = (iterator_type *)_value;
      *value = nullptr;
    }
    context.heap->free((char *)this, sizeof(TabIterator));
  }

  VectorType *array;
  typedef decltype(array->begin()) iterator_type;
  iterator_type end;
};

template <typename TT>
struct das_default_vector_size<Tab<TT>>
{
  static __forceinline uint32_t size(const Tab<TT> &value) { return uint32_t(value.size()); }
};

template <typename T>
struct ManagedTabAnnotation : TypeAnnotation
{
  typedef T VectorType;
  using OT = typename T::value_type;

  struct SimNodeAtTab : SimNode_At
  {
    using TT = OT;
    DAS_PTR_NODE;

    SimNodeAtTab(const LineInfo &at, SimNode *rv, SimNode *idx, uint32_t ofs) : SimNode_At(at, rv, idx, 0, ofs, 0) {}

    virtual SimNode *visit(SimVisitor &vis) override
    {
      V_BEGIN();
      V_OP_TT(AtTab);
      V_SUB_THIS(value);
      V_SUB_THIS(index);
      V_ARG_THIS(stride);
      V_ARG_THIS(offset);
      V_ARG_THIS(range);
      V_END();
    }

    __forceinline char *compute(Context &context)
    {
      DAS_PROFILE_NODE
      auto pValue = (VectorType *)value->evalPtr(context);
      uint32_t idx = cast<uint32_t>::to(index->eval(context));
      if (idx >= pValue->size())
      {
        context.throw_error_at(debugInfo, "std::vector index out of range, %u of %u", idx, uint32_t(pValue->size()));
        return nullptr;
      }
      else
      {
        return ((char *)(&(*pValue)[idx])) + offset;
      }
    }
  };
  template <typename OOT>
  struct SimNodeAtTabR2V : SimNodeAtTab
  {
    using TT = OOT;
    SimNodeAtTabR2V(const LineInfo &at, SimNode *rv, SimNode *idx, uint32_t ofs) : SimNodeAtTab(at, rv, idx, ofs) {}
    virtual SimNode *visit(SimVisitor &vis) override
    {
      V_BEGIN();
      V_OP_TT(AtTabR2V);
      V_SUB_THIS(value);
      V_SUB_THIS(index);
      V_ARG_THIS(stride);
      V_ARG_THIS(offset);
      V_ARG_THIS(range);
      V_END();
    }
    DAS_EVAL_ABI virtual vec4f eval(Context &context) override
    {
      DAS_PROFILE_NODE
      OOT *pR = (OOT *)SimNodeAtTab::compute(context);
      DAS_ASSERT(pR);
      return cast<OOT>::from(*pR);
    }
#define EVAL_NODE(TYPE, CTYPE)                        \
  virtual CTYPE eval##TYPE(Context &context) override \
  {                                                   \
    DAS_PROFILE_NODE                                  \
    return *(CTYPE *)SimNodeAtTab::compute(context);  \
  }
    DAS_EVAL_NODE
#undef EVAL_NODE
  };

  ManagedTabAnnotation(const string &n, ModuleLibrary &lib) : TypeAnnotation(n)
  {
    vecType = makeType<OT>(lib);
    vecType->ref = true;
  }

  virtual bool rtti_isHandledTypeAnnotation() const override { return true; }
  virtual size_t getSizeOf() const override { return sizeof(VectorType); }
  virtual size_t getAlignOf() const override { return alignof(VectorType); }
  virtual bool isRefType() const override { return true; }
  virtual bool isIndexable(const TypeDeclPtr &indexType) const override { return indexType->isIndex(); }
  virtual bool isIterable() const override { return true; }
  virtual bool canMove() const override { return false; }
  virtual bool canCopy() const override { return false; }
  virtual bool isLocal() const override { return false; }
  virtual bool isYetAnotherVectorTemplate() const override { return true; }

  virtual TypeDeclPtr makeIndexType(const ExpressionPtr &, const ExpressionPtr &) const override
  {
    return make_smart<TypeDecl>(*vecType);
  }

  virtual TypeDeclPtr makeIteratorType(const ExpressionPtr &) const override { return make_smart<TypeDecl>(*vecType); }

  virtual SimNode *simulateGetAt(Context &context, const LineInfo &at, const TypeDeclPtr &, const ExpressionPtr &rv,
    const ExpressionPtr &idx, uint32_t ofs) const override
  {
    return context.code->makeNode<SimNodeAtTab>(at, rv->simulate(context), idx->simulate(context), ofs);
  }

  virtual SimNode *simulateGetAtR2V(Context &context, const LineInfo &at, const TypeDeclPtr &type, const ExpressionPtr &rv,
    const ExpressionPtr &idx, uint32_t ofs) const override
  {
    if (type->isHandle())
    {
      auto expr = context.code->makeNode<SimNodeAtTab>(at, rv->simulate(context), idx->simulate(context), ofs);
      return ExprRef2Value::GetR2V(context, at, type, expr);
    }
    else
    {
      return context.code->makeValueNode<SimNodeAtTabR2V>(type->baseType, at, rv->simulate(context), idx->simulate(context), ofs);
    }
  }

  virtual SimNode *simulateGetIterator(Context &context, const LineInfo &at, const ExpressionPtr &src) const override
  {
    auto rv = src->simulate(context);
    return context.code->makeNode<SimNode_AnyIterator<VectorType, TabIterator<VectorType>>>(at, rv);
  }

  virtual void walk(DataWalker &walker, void *vec) override
  {
    if (!ati)
    {
      auto dimType = make_smart<TypeDecl>(*vecType);
      dimType->ref = 0;
      dimType->dim.push_back(1);
      ati = helpA.makeTypeInfo(nullptr, dimType);
      ati->flags |= TypeInfo::flag_isHandled;
    }
    auto pVec = (VectorType *)vec;
    auto atit = *ati;
    atit.dim[atit.dimSize - 1] = uint32_t(pVec->size());
    atit.size = int(ati->size * pVec->size());
    walker.walk_dim((char *)pVec->data(), &atit);
  }
  TypeDeclPtr vecType;
  DebugInfoHelper helpA;
  TypeInfo *ati = nullptr;
};

template <typename VecT, typename TT, uint32_t size>
struct das_array_index
{
  static __forceinline TT &at(VecT &value, int32_t index, Context *__context__)
  {
    if (uint32_t(index) >= size)
      __context__->throw_error_ex("array index out of range, %u of %u", index, size);
    return value[index];
  }

  static __forceinline const TT &at(const VecT &value, int32_t index, Context *__context__)
  {
    if (uint32_t(index) >= size)
      __context__->throw_error_ex("array index out of range, %u of %u", index, size);
    return value[index];
  }

  static __forceinline TT &at(VecT &value, uint32_t idx, Context *__context__)
  {
    if (idx >= size)
      __context__->throw_error_ex("array index out of range, %u of %u", idx, size);
    return value[idx];
  }

  static __forceinline const TT &at(const VecT &value, uint32_t idx, Context *__context__)
  {
    if (idx >= size)
      __context__->throw_error_ex("array index out of range, %u of %u", idx, size);
    return value[idx];
  }
};

template <typename TT>
__forceinline int32_t das_tab_length(const TT &vec)
{
  G_UNUSED(vec); // for carray, because of carray::size is static
  return int32_t(vec.size());
}

template <typename TT, typename QQ = typename TT::value_type>
__forceinline void das_vec_emplace_hint_at(TT &vec, QQ &value, int32_t at, Context *context)
{
  if (uint32_t(at) > vec.size())
    context->throw_error_ex("insert index out of range, %i of %u", at, uint32_t(vec.size()));
  vec.emplace_hint(vec.begin() + at, move(value));
}

template <typename TT, typename QQ = typename TT::value_type>
__forceinline void das_vector_push_back_unsorted(TT &vec, const QQ &value)
{
  vec.push_back_unsorted(value);
}

template <typename TT, typename QQ = typename TT::value_type>
__forceinline void das_vector_emplace_back_unsorted(TT &vec, QQ &value)
{
  vec.emplace_back_unsorted(move(value));
}

template <typename TT, bool byValue = das::has_cast<TT>::value>
struct RegisterVecSetFunctions;

template <typename TT>
struct RegisterVecSetFunctions<TT, false>
{
  static void init(Module &mod, const ModuleLibrary &lib, bool canCopy, bool canMove)
  {
    addExtern<DAS_BIND_FUN(das_tab_length<TT>)>(mod, lib, "length", SideEffects::none, "das_tab_length")->generated = true;
    if (canMove)
    {
      addExtern<DAS_BIND_FUN(das::das_vector_emplace_back_unsorted<TT>), SimNode_ExtFuncCall, permanentArgFn>(mod, lib, "emplace",
        SideEffects::modifyArgument, "das::das_vector_emplace_back_unsorted")
        ->generated = true;
      addExtern<DAS_BIND_FUN(das_vec_emplace_hint_at<TT>), SimNode_ExtFuncCall, permanentArgFn>(mod, lib, "emplace",
        SideEffects::modifyArgument, "das_vec_emplace_hint_at")
        ->generated = true;
    }
    if (canCopy)
    {
      addExtern<DAS_BIND_FUN(das::das_vector_push_back_unsorted<TT>)>(mod, lib, "push", SideEffects::modifyArgument,
        "das::das_vector_push_back_unsorted")
        ->generated = true;
      addExtern<DAS_BIND_FUN(das::das_vector_push<TT>)>(mod, lib, "push", SideEffects::modifyArgument, "das::das_vector_push")
        ->generated = true;
    }
    addExtern<DAS_BIND_FUN(das::das_vector_pop<TT>)>(mod, lib, "pop", SideEffects::modifyArgument, "das::das_vector_pop")->generated =
      true;
    addExtern<DAS_BIND_FUN(das::das_vector_erase<TT>)>(mod, lib, "erase", SideEffects::modifyArgument, "das::das_vector_erase")
      ->generated = true;
    addExtern<DAS_BIND_FUN(das::das_vector_erase_range<TT>)>(mod, lib, "erase", SideEffects::modifyArgument,
      "das::das_vector_erase_range")
      ->generated = true;
    addExtern<DAS_BIND_FUN(das::das_vector_clear<TT>)>(mod, lib, "clear", SideEffects::modifyArgument, "das::das_vector_clear")
      ->generated = true;
  }
};

template <typename TT>
struct RegisterVecSetFunctions<TT, true>
{
  static void init(Module &mod, const ModuleLibrary &lib, bool canCopy, bool canMove)
  {
    addExtern<DAS_BIND_FUN(das_tab_length<TT>)>(mod, lib, "length", SideEffects::none, "das_tab_length")->generated = true;
    if (canMove)
    {
      addExtern<DAS_BIND_FUN(das::das_vector_emplace_back<TT>), SimNode_ExtFuncCall, permanentArgFn>(mod, lib, "emplace",
        SideEffects::modifyArgument, "das::das_vector_emplace_back")
        ->generated = true;
      addExtern<DAS_BIND_FUN(das_vec_emplace_hint_at<TT>), SimNode_ExtFuncCall, permanentArgFn>(mod, lib, "emplace",
        SideEffects::modifyArgument, "das_vec_emplace_hint_at")
        ->generated = true;
    }
    if (canCopy)
    {
      addExtern<DAS_BIND_FUN(das::das_vector_push_back_value<TT>), SimNode_ExtFuncCall, permanentArgFn>(mod, lib, "push",
        SideEffects::modifyArgument, "das::das_vector_push_back_value")
        ->generated = true;
      addExtern<DAS_BIND_FUN(das::das_vector_push<TT>), SimNode_ExtFuncCall, permanentArgFn>(mod, lib, "push",
        SideEffects::modifyArgument, "das::das_vector_push")
        ->generated = true;
    }
    addExtern<DAS_BIND_FUN(das::das_vector_pop<TT>), SimNode_ExtFuncCall, permanentArgFn>(mod, lib, "pop", SideEffects::modifyArgument,
      "das::das_vector_pop")
      ->generated = true;
    addExtern<DAS_BIND_FUN(das::das_vector_erase<TT>), SimNode_ExtFuncCall, permanentArgFn>(mod, lib, "erase",
      SideEffects::modifyArgument, "das::das_vector_erase")
      ->generated = true;
    addExtern<DAS_BIND_FUN(das::das_vector_erase_range<TT>), SimNode_ExtFuncCall, permanentArgFn>(mod, lib, "erase",
      SideEffects::modifyArgument, "das::das_vector_erase_range")
      ->generated = true;
    addExtern<DAS_BIND_FUN(das::das_vector_clear<TT>), SimNode_ExtFuncCall, permanentArgFn>(mod, lib, "clear",
      SideEffects::modifyArgument, "das::das_vector_clear")
      ->generated = true;
  }
};
}; // namespace das
