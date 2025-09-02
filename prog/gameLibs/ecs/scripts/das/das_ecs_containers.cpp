// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "das_ecs.h"
#include <dasModules/dasEvent.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <dasModules/dasModulesCommon.h>
#include <daScript/ast/ast_policy_types.h>
#include <daScript/simulate/sim_policy.h>
#include <dasModules/aotEcsContainer.h>
#include <daScript/simulate/simulate_visit_op.h>

MAKE_TYPE_FACTORY(ObjectIterator, bind_dascript::ObjectIterator)
MAKE_TYPE_FACTORY(ObjectConstIterator, bind_dascript::ObjectConstIterator)

namespace das
{
IMPLEMENT_OP2_EVAL_BOOL_POLICY(Equ, ecs::ChildComponent);
IMPLEMENT_OP2_EVAL_BOOL_POLICY(NotEqu, ecs::ChildComponent);
}; // namespace das

namespace bind_dascript
{
das::TypeInfo get_type_info(ecs::component_type_t component_type)
{
  das::TypeInfo res;
  memset(&res, 0, sizeof(res));

  switch (component_type)
  {
    case ecs::ComponentTypeInfo<bool>::type: res.type = das::Type::tBool; return res;

    case ecs::ComponentTypeInfo<float>::type: res.type = das::Type::tFloat; return res;

    case ecs::ComponentTypeInfo<int32_t>::type: res.type = das::Type::tInt; return res;

    case ecs::ComponentTypeInfo<uint32_t>::type: res.type = das::Type::tUInt; return res;

    case ecs::ComponentTypeInfo<int8_t>::type: res.type = das::Type::tInt8; return res;

    case ecs::ComponentTypeInfo<uint8_t>::type: res.type = das::Type::tUInt8; return res;

    case ecs::ComponentTypeInfo<int16_t>::type: res.type = das::Type::tInt16; return res;

    case ecs::ComponentTypeInfo<uint16_t>::type: res.type = das::Type::tUInt16; return res;

    case ecs::ComponentTypeInfo<int64_t>::type: res.type = das::Type::tInt64; return res;

    case ecs::ComponentTypeInfo<uint64_t>::type: res.type = das::Type::tUInt64; return res;

    case ecs::ComponentTypeInfo<Point2>::type: res.type = das::Type::tFloat2; return res;

    case ecs::ComponentTypeInfo<Point3>::type: res.type = das::Type::tFloat3; return res;

    case ecs::ComponentTypeInfo<Point4>::type: res.type = das::Type::tFloat4; return res;

    case ecs::ComponentTypeInfo<IPoint2>::type: res.type = das::Type::tInt2; return res;

    case ecs::ComponentTypeInfo<IPoint3>::type: res.type = das::Type::tInt3; return res;

    case ecs::ComponentTypeInfo<IPoint4>::type: res.type = das::Type::tInt4; return res;

    case ecs::ComponentTypeInfo<ecs::string>::type:
    {
      res.type = das::Type::tHandle;
      das::Module *builintModule = das::Module::require("$");
      res.annotation_or_name = (das::TypeAnnotation *)builintModule->findAnnotation("das_string").get();
      return res;
    }

    case ecs::ComponentTypeInfo<ecs::EntityId>::type:
    {
      res.type = das::Type::tHandle;
      das::Module *ecsModule = das::Module::require("ecs");
      res.annotation_or_name = (das::TypeAnnotation *)ecsModule->findAnnotation("EntityId").get();
      return res;
    }

    case ecs::ComponentTypeInfo<ecs::Object>::type:
    {
      res.type = das::Type::tHandle;
      das::Module *ecsModule = das::Module::require("ecs");
      res.annotation_or_name = (das::TypeAnnotation *)ecsModule->findAnnotation("Object").get();
      return res;
    }

    case ecs::ComponentTypeInfo<ecs::Array>::type:
    {
      res.type = das::Type::tHandle;
      das::Module *ecsModule = das::Module::require("ecs");
      res.annotation_or_name = (das::TypeAnnotation *)ecsModule->findAnnotation("Array").get();
      return res;
    }

    case ecs::ComponentTypeInfo<TMatrix>::type:
    {
      res.type = das::Type::tHandle;
      das::Module *ecsModule = das::Module::require("math");
      res.annotation_or_name = (das::TypeAnnotation *)ecsModule->findAnnotation("float3x4").get();
      return res;
    }

    case ecs::ComponentTypeInfo<E3DCOLOR>::type:
    {
      res.type = das::Type::tHandle;
      das::Module *ecsModule = das::Module::require("DagorMath");
      res.annotation_or_name = (das::TypeAnnotation *)ecsModule->findAnnotation("E3DCOLOR").get();
      return res;
    }
  }
  return res;
}

template <typename IterType>
struct ObjectIteratorAnnotationAny : das::ManagedStructureAnnotation<IterType, false>
{
  ObjectIteratorAnnotationAny(das::ModuleLibrary &ml, const char *ann, const char *cppn) :
    das::ManagedStructureAnnotation<IterType, false>(ann, ml)
  {
    this->cppName = cppn;
    // add property
    using ManagedType = IterType;
    this->template addProperty<DAS_BIND_MANAGED_PROP(get_key)>("key", "get_key");
    this->template addProperty<DAS_BIND_MANAGED_PROP(get_value)>("value", "get_value");
  }
  virtual bool rtti_isHandledTypeAnnotation() const override { return true; }
  virtual bool isRefType() const override { return true; }
  virtual bool isLocal() const override { return false; }
  virtual bool isPod() const override { return false; }
  virtual bool canCopy() const override { return false; }
  virtual bool canMove() const override { return false; }
};

struct ObjectIteratorAnnotation final : ObjectIteratorAnnotationAny<ObjectIterator>
{
  ObjectIteratorAnnotation(das::ModuleLibrary &ml) :
    ObjectIteratorAnnotationAny<ObjectIterator>(ml, "ObjectIterator", "bind_dascript::ObjectIterator")
  {}
};

struct ObjectConstIteratorAnnotation final : ObjectIteratorAnnotationAny<ObjectConstIterator>
{
  ObjectConstIteratorAnnotation(das::ModuleLibrary &ml) :
    ObjectIteratorAnnotationAny<ObjectConstIterator>(ml, "ObjectConstIterator", "bind_dascript::ObjectConstIterator")
  {}
};


// this is helping class for all iterators, that are separate Objects (i.e. class), kinda map<>
// you need to bind some ObjectIterator for it
template <class Object>
struct DasObjectIterator : das::Iterator
{
  DasObjectIterator(Object *obj, das::LineInfo *at) : das::Iterator(at), object(obj) {}
  virtual bool first(das::Context &, char *_value) override
  {
    if (!object->size())
      return false;
    G_STATIC_ASSERT(sizeof(iterator_type) == sizeof(void *));
    current = object->begin();
    end = object->end();
    G_ASSERT(current != end);
    iterator_type **value = (iterator_type **)_value;
    *value = &current;
    return true;
  }
  virtual bool next(das::Context &, char *_value) override
  {
    iterator_type **value = (iterator_type **)_value;
    ++(**value);
    return **value != end;
  }
  virtual void close(das::Context &context, char *_value) override
  {
    if (_value)
    {
      iterator_type **value = (iterator_type **)_value;
      *value = nullptr;
    }
    context.freeIterator((char *)this, debugInfo);
  }
  Object *object = nullptr; // can be unioned with end
  typedef decltype(object->begin()) iterator_type;
  iterator_type current, end;
};

struct ObjectAnnotation final : das::ManagedStructureAnnotation<ecs::Object, false>
{
  ObjectAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Object", ml)
  {
    cppName = " ::ecs::Object";
    addProperty<DAS_BIND_MANAGED_PROP(size)>("size");
    addProperty<DAS_BIND_MANAGED_PROP(empty)>("empty");
    iteratorType = das::makeType<ObjectIterator>(ml);
    iteratorType->ref = true;
    constIteratorType = das::makeType<ObjectConstIterator>(ml);
    constIteratorType->ref = true;
    constIteratorType->constant = true;
    atType = das::makeType<ecs::ChildComponent>(ml);
  }
  virtual bool rtti_isHandledTypeAnnotation() const override { return true; }
  virtual bool isRefType() const override { return true; }
  virtual bool isLocal() const override { return false; }
  virtual bool isPod() const override { return false; }
  virtual bool canCopy() const override { return false; }
  virtual bool canMove() const override { return false; }
  virtual bool canClone() const override { return true; }
  das::SimNode *simulateClone(das::Context &context, const das::LineInfo &at, das::SimNode *l, das::SimNode *r) const override
  {
    return context.code->makeNode<das::SimNode_CloneRefValueT<ecs::Object>>(at, l, r);
  }
  virtual bool isIterable() const override { return true; }

  virtual das::TypeDeclPtr makeIteratorType(const das::ExpressionPtr &src) const override
  {
    if (src->type->isConst())
      return das::make_smart<das::TypeDecl>(*constIteratorType);
    else
      return das::make_smart<das::TypeDecl>(*iteratorType);
  }
  virtual das::SimNode *simulateGetIterator(das::Context &context, const das::LineInfo &at,
    const das::ExpressionPtr &src) const override
  {
    auto rv = src->simulate(context);
    if (src->type->isConst())
      return context.code->makeNode<das::SimNode_AnyIterator<const ecs::Object, DasObjectIterator<const ecs::Object>>>(at, rv);
    else
      return context.code->makeNode<das::SimNode_AnyIterator<ecs::Object, DasObjectIterator<ecs::Object>>>(at, rv);
  }
  // indexing
  virtual das::SimNode *simulateGetAt(das::Context &context, const das::LineInfo &at, const das::TypeDeclPtr &,
    const das::ExpressionPtr &rv, const das::ExpressionPtr &idx, uint32_t ofs) const override
  {
    if (rv->type->isConst())
      return context.code->makeNode<SimNodeAtObject<const ecs::Object>>(at, rv->simulate(context), idx->simulate(context), ofs);
    else
      return context.code->makeNode<SimNodeAtObject<ecs::Object>>(at, rv->simulate(context), idx->simulate(context), ofs);
  }
  virtual bool isIndexable(const das::TypeDeclPtr &indexType) const override { return indexType->isSimpleType(das::Type::tString); }
  virtual das::TypeDeclPtr makeIndexType(const das::ExpressionPtr &rv, const das::ExpressionPtr &) const override
  {
    auto ret = das::make_smart<das::TypeDecl>(das::Type::tPointer);
    ret->firstType = das::make_smart<das::TypeDecl>(*atType);
    ret->firstType->constant = rv->type->isConst();
    return ret;
  }
  template <class ObjectType>
  struct SimNodeAtObject : das::SimNode_At
  {
    DAS_PTR_NODE;
    SimNodeAtObject(const das::LineInfo &at, das::SimNode *rv, das::SimNode *idx, uint32_t ofs) :
      SimNode_At(at, rv, idx, 0, ofs, 0, "")
    {}
    __forceinline char *compute(das::Context &context)
    {
      ObjectType *pValue = (ObjectType *)value->evalPtr(context);
      char *idx = das::cast<char *>::to(index->eval(context));
      auto it = pValue->find_as(ECS_HASH_SLOW(idx ? idx : ""));
      if (it != pValue->end())
        return (char *)&it->second;
      return nullptr;
    }
    virtual das::SimNode *visit(das::SimVisitor &vis) override
    {
      using namespace das;
      using TT = ObjectType;
      V_BEGIN();
      V_OP_TT(AtEcsObject);
      V_SUB(value);
      V_SUB(index);
      V_END();
    }
  };

#if DAGOR_DBGLEVEL > 0
  virtual void walk(das::DataWalker &walker, void *obj) override
  {
    ecs::Object *object = (ecs::Object *)obj;
    das::Table tab;
    memset(&tab, 0, sizeof(tab));
    if (!ati)
    {
      auto dimType = das::make_smart<das::TypeDecl>(das::Type::tTable);
      dimType->firstType = das::make_smart<das::TypeDecl>(das::Type::tString);
      dimType->secondType = atType;
      ati = helpA.makeTypeInfo(nullptr, dimType);
    }

    walker.beforeTable(&tab, ati);
    if (walker.cancel())
      return;
    const uint32_t count = object->size();
    uint32_t i = 0;
    for (auto pair : *object)
    {
      const bool last = i == count - 1;
      i++;

      auto key = pair.first.c_str();
      walker.beforeTableKey(&tab, ati, (char *)&key, ati->firstType, count, last);
      if (walker.cancel())
        return;
      walker.walk((char *)&key, ati->firstType);
      if (walker.cancel())
        return;
      walker.afterTableKey(&tab, ati, (char *)&key, ati->firstType, count, last);
      if (walker.cancel())
        return;
      // value
      walker.beforeTableValue(&tab, ati, (char *)&pair.second, ati->secondType, count, last);
      if (walker.cancel())
        return;
      walker.walk((char *)&pair.second, ati->secondType);
      if (walker.cancel())
        return;
      walker.afterTableValue(&tab, ati, (char *)&pair.second, ati->secondType, count, last);
      if (walker.cancel())
        return;
    }
    if (walker.cancel())
      return;
    walker.afterTable(&tab, ati);
  }

  das::TypeInfo *ati = nullptr;
#endif
  das::TypeDeclPtr iteratorType, constIteratorType, atType;
};

struct ArrayAnnotation final : das::ManagedStructureAnnotation<ecs::Array, false>
{
  ArrayAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Array", ml)
  {
    cppName = " ::ecs::Array";
    addProperty<DAS_BIND_MANAGED_PROP(size)>("size");
    addProperty<DAS_BIND_MANAGED_PROP(empty)>("empty");
    vecType = das::makeType<ecs::ChildComponent>(ml);
    vecType->ref = true;
  }
  virtual bool rtti_isHandledTypeAnnotation() const override { return true; }
  virtual bool isRefType() const override { return true; }
  virtual bool isLocal() const override { return false; }
  virtual bool isPod() const override { return false; }
  virtual bool canCopy() const override { return false; }
  virtual bool canMove() const override { return false; }
  virtual bool canClone() const override { return true; }
  das::SimNode *simulateClone(das::Context &context, const das::LineInfo &at, das::SimNode *l, das::SimNode *r) const override
  {
    return context.code->makeNode<das::SimNode_CloneRefValueT<ecs::Array>>(at, l, r);
  }
  // iterating
  virtual bool isIterable() const override { return true; }
  virtual das::TypeDeclPtr makeIteratorType(const das::ExpressionPtr &) const override
  {
    return das::make_smart<das::TypeDecl>(*vecType);
  }

  virtual das::SimNode *simulateGetIterator(das::Context &context, const das::LineInfo &at,
    const das::ExpressionPtr &src) const override
  {
    auto rv = src->simulate(context);
    if (src->type->isConst())
      return context.code->makeNode<das::SimNode_AnyIterator<const ecs::Array, DasArrayIterator<const ecs::Array>>>(at, rv);
    else
      return context.code->makeNode<das::SimNode_AnyIterator<ecs::Array, DasArrayIterator<ecs::Array>>>(at, rv);
  }

  bool isYetAnotherVectorTemplate() const override { return true; }

  // indexing
  virtual das::SimNode *simulateGetAt(das::Context &context, const das::LineInfo &at, const das::TypeDeclPtr &,
    const das::ExpressionPtr &rv, const das::ExpressionPtr &idx, uint32_t ofs) const override
  {
    auto rNode = rv->simulate(context), idxNode = idx->simulate(context);
    if (rv->type->isConst())
      return context.code->makeNode<SimNodeAtArray<const ecs::Array>>(at, rNode, idxNode, ofs);
    else
      return context.code->makeNode<SimNodeAtArray<ecs::Array>>(at, rNode, idxNode, ofs);
  }
  virtual bool isIndexable(const das::TypeDeclPtr &indexType) const override { return indexType->isIndex(); }
  virtual das::TypeDeclPtr makeIndexType(const das::ExpressionPtr &, const das::ExpressionPtr &) const override
  {
    return das::make_smart<das::TypeDecl>(*vecType);
  }
  template <class Array>
  struct SimNodeAtArray : das::SimNode_At
  {
    DAS_PTR_NODE;
    SimNodeAtArray(const das::LineInfo &at, das::SimNode *rv, das::SimNode *idx, uint32_t ofs) : SimNode_At(at, rv, idx, 0, ofs, 0, "")
    {}
    __forceinline char *compute(das::Context &context)
    {
      Array *pValue = (Array *)value->evalPtr(context);
      uint32_t idx = das::cast<uint32_t>::to(index->eval(context));
      if (idx >= pValue->size())
      {
        context.throw_error_at(debugInfo, "Array index %d out of range %d", idx, pValue->size());
        return nullptr;
      }
      else
        return ((char *)(&(*pValue)[idx])) + offset;
    }
    virtual das::SimNode *visit(das::SimVisitor &vis) override
    {
      using namespace das;
      using TT = Array;
      V_BEGIN();
      V_OP_TT(AtEcsArray);
      V_SUB(value);
      V_SUB(index);
      V_ARG(offset);
      V_END();
    }
  };
#if DAGOR_DBGLEVEL > 0
  virtual void walk(das::DataWalker &walker, void *vec) override
  {
    if (!ati)
    {
      auto dimType = das::make_smart<das::TypeDecl>(*vecType);
      dimType->ref = 0;
      dimType->dim.push_back(1234);
      ati = helpA.makeTypeInfo(nullptr, dimType);
    }
    ecs::Array *pVec = (ecs::Array *)vec;
    ati->dim[ati->dimSize - 1] = uint32_t(pVec->size());
    walker.walk_dim((char *)pVec->begin(), ati);
  }
  das::TypeInfo *ati = nullptr;
#endif
  das::TypeDeclPtr vecType;
};

struct SharedArrayAnnotation final : das::ManagedStructureAnnotation<ecs::SharedComponent<ecs::Array>, false>
{
  SharedArrayAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("SharedArray", ml)
  {
    cppName = "ecs::SharedComponent< ::ecs::Array>";

    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(get)>("get");
  }
};

struct SharedObjectAnnotation final : das::ManagedStructureAnnotation<ecs::SharedComponent<ecs::Object>, false>
{
  SharedObjectAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("SharedObject", ml)
  {
    cppName = "ecs::SharedComponent< ::ecs::Object>";

    addPropertyForManagedType<DAS_BIND_MANAGED_PROP(get)>("get");
  }
};

struct ChildComponentAnnotation final : das::ManagedStructureAnnotation<ecs::ChildComponent, false>
{
  ChildComponentAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ChildComponent", ml)
  {
    cppName = " ::ecs::ChildComponent";
    addProperty<DAS_BIND_MANAGED_PROP(isNull)>("isNull");
    addProperty<DAS_BIND_MANAGED_PROP(getUserType)>("userType", "getUserType");
    addProperty<DAS_BIND_MANAGED_PROP(getTypeId)>("typeId", "getTypeId");
    addProperty<DAS_BIND_MANAGED_PROP(getSize)>("size", "getSize");
  }
  virtual bool rtti_isHandledTypeAnnotation() const override { return true; }
  virtual bool isRefType() const override { return true; }
  virtual bool isLocal() const override { return false; }
  virtual bool isPod() const override { return false; }
  virtual bool canCopy() const override { return false; }
  virtual bool canMove() const override { return false; }
  virtual bool canClone() const override { return true; }
  das::SimNode *simulateClone(das::Context &context, const das::LineInfo &at, das::SimNode *l, das::SimNode *r) const override
  {
    return context.code->makeNode<das::SimNode_CloneRefValueT<ecs::ChildComponent>>(at, l, r);
  }

#if DAGOR_DBGLEVEL > 0
  virtual void walk(das::DataWalker &walker, void *child) override
  {
    ecs::ChildComponent *comp = (ecs::ChildComponent *)child;
    das::TypeInfo dasType = get_type_info(comp->getUserType());
    if (dasType.type != das::none)
      walker.walk((char *)comp->getEntityComponentRef().getRawData(), &dasType);
    else
      ManagedStructureAnnotation<ecs::ChildComponent, false>::walk(walker, child);
  }

#endif
};

void ECS::addContainerAnnotations(das::ModuleLibrary &lib)
{
  addAnnotation(das::make_smart<ChildComponentAnnotation>(lib));
  das::addFunctionBasic<ecs::ChildComponent>(*this, lib);
  addAnnotation(das::make_smart<ObjectIteratorAnnotation>(lib));
  addAnnotation(das::make_smart<ObjectConstIteratorAnnotation>(lib));
  addAnnotation(das::make_smart<ObjectAnnotation>(lib));
  addAnnotation(das::make_smart<ArrayAnnotation>(lib));
  addAnnotation(das::make_smart<SharedArrayAnnotation>(lib));
  addAnnotation(das::make_smart<SharedObjectAnnotation>(lib));

  das::addUsing<ecs::ChildComponent>(*this, lib, " ::ecs::ChildComponent");

  using method_getEntityComponentRef = DAS_CALL_MEMBER(ecs::ChildComponent::getEntityComponentRef);
  das::addExtern<DAS_CALL_METHOD(method_getEntityComponentRef), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
    "getEntityComponentRef", das::SideEffects::none, DAS_CALL_MEMBER_CPP(ecs::ChildComponent::getEntityComponentRef));
}
} // namespace bind_dascript
