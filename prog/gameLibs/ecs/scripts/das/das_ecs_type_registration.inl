// Copyright (C) Gaijin Games KFT.  All rights reserved.

#ifndef ECS_BIND_TYPE_LIST
#error "ECS_BIND_TYPE_LIST must be defined before including das_ecs_type_registration.inl"
#endif

#ifdef DAS_ECS_BIND_ARRAY
#define TYPE(type)                                                                                                                    \
  das::addExtern<DAS_BIND_FUN(setArray##type)>(*this, lib, "set", das::SideEffects::modifyArgument, "bind_dascript::setArray" #type); \
  das::addExtern<DAS_BIND_FUN(getArray##type)>(*this, lib, "get_" #type, das::SideEffects::none, "bind_dascript::getArray" #type);    \
  das::addExtern<DAS_BIND_FUN(getArrayRW##type)>(*this, lib, "getRW_" #type, das::SideEffects::modifyArgument,                        \
    "bind_dascript::getArrayRW" #type);                                                                                               \
  das::addExtern<DAS_BIND_FUN(pushArray##type)>(*this, lib, "push", das::SideEffects::modifyArgument,                                 \
    "bind_dascript::pushArray" #type);                                                                                                \
  das::addExtern<DAS_BIND_FUN(pushAtArray##type)>(*this, lib, "push", das::SideEffects::modifyArgument,                               \
    "bind_dascript::pushAtArray" #type);
ECS_BIND_TYPE_LIST
#undef TYPE
#undef DAS_ECS_BIND_ARRAY
#endif

#ifdef DAS_ECS_BIND_OBJECT_READ
#define TYPE(type)                                                                                                                \
  das::addExtern<DAS_BIND_FUN(getObjectHint##type)>(*this, lib, "get_" #type, das::SideEffects::none,                             \
    "bind_dascript::getObjectHint" #type);                                                                                        \
  auto getObjectExt##type = das::addExtern<DAS_BIND_FUN(getObject##type)>(*this, lib, "get_" #type, das::SideEffects::none,       \
    "bind_dascript::getObject" #type);                                                                                            \
  getObjectExt##type->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));            \
  das::addExtern<DAS_BIND_FUN(getObjectPtrHint##type)>(*this, lib, "get_" #type, das::SideEffects::none,                          \
    "bind_dascript::getObjectPtrHint" #type);                                                                                     \
  auto getObjectPtrExt##type = das::addExtern<DAS_BIND_FUN(getObjectPtr##type)>(*this, lib, "get_" #type, das::SideEffects::none, \
    "bind_dascript::getObjectPtr" #type);                                                                                         \
  getObjectPtrExt##type->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));
ECS_BIND_TYPE_LIST
#undef TYPE
#undef DAS_ECS_BIND_OBJECT_READ
#endif

#ifdef DAS_ECS_BIND_OBJECT_WRITE
#define TYPE(type)                                                                                                                   \
  das::addExtern<DAS_BIND_FUN(setObjectHint##type), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, "set",                \
    das::SideEffects::modifyArgument, "bind_dascript::setObjectHint" #type);                                                         \
  auto setObjectExt##type = das::addExtern<DAS_BIND_FUN(setObject##type), das::SimNode_ExtFuncCall, das::permanentArgFn>(*this, lib, \
    "set", das::SideEffects::modifyArgument, "bind_dascript::setObject" #type);                                                      \
  setObjectExt##type->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));
ECS_BIND_TYPE_LIST
#undef TYPE
#undef DAS_ECS_BIND_OBJECT_WRITE
#endif

#ifdef DAS_ECS_BIND_OBJECT_RW
#define TYPE(type)                                                                                                                \
  das::addExtern<DAS_BIND_FUN(getObjectRWHint##type)>(*this, lib, "getRW_" #type, das::SideEffects::modifyArgument,               \
    "bind_dascript::getObjectRWHint" #type);                                                                                      \
  auto getObjectRWExt##type = das::addExtern<DAS_BIND_FUN(getObjectRW##type)>(*this, lib, "getRW_" #type,                         \
    das::SideEffects::modifyArgument, "bind_dascript::getObjectRW" #type);                                                        \
  getObjectRWExt##type->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));          \
  das::addExtern<DAS_BIND_FUN(getObjectPtrRWHint##type)>(*this, lib, "getRW_" #type, das::SideEffects::modifyArgumentAndExternal, \
    "bind_dascript::getObjectPtrRWHint" #type);                                                                                   \
  auto getObjectPtrRWHint##type = das::addExtern<DAS_BIND_FUN(getObjectPtrRW##type)>(*this, lib, "getRW_" #type,                  \
    das::SideEffects::modifyArgumentAndExternal, "bind_dascript::getObjectPtrRW" #type);                                          \
  getObjectPtrRWHint##type->annotations.push_back(annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1>>()));
ECS_BIND_TYPE_LIST
#undef TYPE
#undef DAS_ECS_BIND_OBJECT_RW
#endif

#ifdef DAS_ECS_BIND_ENTITY_READ
#define TYPE(type)                                                                                                             \
  auto entityGetNullable##type = das::addExtern<DAS_BIND_FUN(entityGetNullableHint_##type)>(*this, lib, "get_" #type,          \
    das::SideEffects::accessExternal, "bind_dascript::entityGetNullableHint_" #type);                                          \
  entityGetNullable##type->annotations.push_back(annotation_declaration(das::make_smart<EcsGetOrFunctionAnnotation<1, 3>>())); \
  auto entityGetNullableExt##type = das::addExtern<DAS_BIND_FUN(entityGetNullable_##type)>(*this, lib, "get_" #type,           \
    das::SideEffects::accessExternal, "bind_dascript::entityGetNullable_" #type);                                              \
  entityGetNullableExt##type->annotations.push_back(                                                                           \
    annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1, /*only fast call*/ true>>()));
ECS_BIND_TYPE_LIST
#undef TYPE
#undef DAS_ECS_BIND_ENTITY_READ
#endif

#ifdef DAS_ECS_BIND_ENTITY_WRITE
#define TYPE(type)                                                                                                             \
  das::addExtern<DAS_BIND_FUN(entitySetHint##type)>(*this, lib, "set", das::SideEffects::modifyExternal,                       \
    "bind_dascript::entitySetHint" #type)                                                                                      \
    ->annotations.push_back(annotation_declaration(das::make_smart<EcsSetAnnotation<1, 3, /*optional*/ false>>(#type)));       \
  auto entitySetExt##type = das::addExtern<DAS_BIND_FUN(entitySet##type)>(*this, lib, "set", das::SideEffects::modifyExternal, \
    "bind_dascript::entitySet" #type);                                                                                         \
  entitySetExt##type->annotations.push_back(                                                                                   \
    annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1, /*only fast call*/ true>>()));
ECS_BIND_TYPE_LIST
#undef TYPE
#undef DAS_ECS_BIND_ENTITY_WRITE
#endif

#ifdef DAS_ECS_BIND_ENTITY_WRITE_OPTIONAL
#define TYPE(type)                                                                                                       \
  das::addExtern<DAS_BIND_FUN(entitySetOptionalHint##type)>(*this, lib, "setOptional", das::SideEffects::modifyExternal, \
    "bind_dascript::entitySetOptionalHint" #type)                                                                        \
    ->annotations.push_back(annotation_declaration(das::make_smart<EcsSetAnnotation<1, 3, /*optional*/ true>>(#type)));  \
  auto entitySetOptionalExt##type = das::addExtern<DAS_BIND_FUN(entitySetOptional##type)>(*this, lib, "setOptional",     \
    das::SideEffects::modifyExternal, "bind_dascript::entitySetOptional" #type);                                         \
  entitySetOptionalExt##type->annotations.push_back(                                                                     \
    annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1, /*only fast call*/ true>>()));
ECS_BIND_TYPE_LIST
#undef TYPE
#undef DAS_ECS_BIND_ENTITY_WRITE_OPTIONAL
#endif

#ifdef DAS_ECS_BIND_ENTITY_RW
#define TYPE(type)                                                                                                           \
  das::addExtern<DAS_BIND_FUN(entityGetNullableRWHint_##type)>(*this, lib, "getRW_" #type, das::SideEffects::modifyExternal, \
    "bind_dascript::entityGetNullableRWHint_" #type);                                                                        \
  auto entityGetNullableRWExt##type = das::addExtern<DAS_BIND_FUN(entityGetNullableRW_##type)>(*this, lib, "getRW_" #type,   \
    das::SideEffects::modifyExternal, "bind_dascript::entityGetNullableRW_" #type);                                          \
  entityGetNullableRWExt##type->annotations.push_back(                                                                       \
    annotation_declaration(das::make_smart<BakeHashFunctionAnnotation<1, /*only fast call*/ true>>()));
ECS_BIND_TYPE_LIST
#undef TYPE
#undef DAS_ECS_BIND_ENTITY_RW
#endif

#ifdef DAS_ECS_BIND_CHILD_COMPONENT
#define TYPE(type)                                                                                                                    \
  das::addExtern<DAS_BIND_FUN(setChildComponent##type)>(*this, lib, "set", das::SideEffects::modifyArgument,                          \
    "bind_dascript::setChildComponent" #type);                                                                                        \
  das::addExtern<DAS_BIND_FUN(getChildComponent##type)>(*this, lib, "get_" #type, das::SideEffects::none,                             \
    "bind_dascript::getChildComponent" #type);                                                                                        \
  das::addExtern<DAS_BIND_FUN(getChildComponentRW##type)>(*this, lib, "getRW_" #type, das::SideEffects::modifyArgument,               \
    "bind_dascript::getChildComponentRW" #type);                                                                                      \
  das::addExtern<DAS_BIND_FUN(getChildComponentPtr##type)>(*this, lib, "get_" #type, das::SideEffects::none,                          \
    "bind_dascript::getChildComponentPtr" #type);                                                                                     \
  das::addExtern<DAS_BIND_FUN(getChildComponentPtrRW##type)>(*this, lib, "getRW_" #type, das::SideEffects::modifyArgumentAndExternal, \
    "bind_dascript::getChildComponentPtrRW" #type);
ECS_BIND_TYPE_LIST
#undef TYPE
#undef DAS_ECS_BIND_CHILD_COMPONENT
#endif

#undef ECS_BIND_TYPE_LIST
