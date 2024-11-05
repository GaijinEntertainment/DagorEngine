//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IPoint4.h>
#include <util/dag_simpleString.h>
#include <math/dag_bounds3.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>
#include <daECS/core/entityId.h>
#include <daECS/core/baseComponentTypes/objectType.h>
#include <daECS/core/baseComponentTypes/arrayType.h>
#include <daECS/core/baseComponentTypes/listType.h>
#include <EASTL/type_traits.h> //for true type
#include <EASTL/string.h>      //for ecs::string type
// all basic types
ECS_DECLARE_TYPE(bool);
ECS_DECLARE_TYPE_ALIAS(eastl::true_type, bool);
ECS_DECLARE_TYPE_ALIAS(eastl::false_type, bool);
ECS_DECLARE_TYPE(uint8_t);
ECS_DECLARE_TYPE(uint16_t);
ECS_DECLARE_TYPE(uint32_t);
ECS_DECLARE_TYPE(uint64_t);
ECS_DECLARE_TYPE(int8_t);
ECS_DECLARE_TYPE(int16_t);
ECS_DECLARE_TYPE(int);
ECS_DECLARE_TYPE(int64_t);
ECS_DECLARE_TYPE(float);
ECS_DECLARE_TYPE(E3DCOLOR);
ECS_DECLARE_TYPE(Point2);
ECS_DECLARE_TYPE(Point3);
ECS_DECLARE_TYPE(Point4);
ECS_DECLARE_TYPE(IPoint2);
ECS_DECLARE_TYPE(IPoint3);
ECS_DECLARE_TYPE(IPoint4);
ECS_DECLARE_TYPE(DPoint3);
ECS_DECLARE_TYPE(TMatrix);
ECS_DECLARE_TYPE(vec4f);
ECS_DECLARE_TYPE(bbox3f);
ECS_DECLARE_TYPE(mat44f);
ECS_DECLARE_TYPE(BBox3);

ECS_DECLARE_RELOCATABLE_TYPE(ecs::Array);
ECS_DECLARE_RELOCATABLE_TYPE(ecs::Object);

#define ECS_DECL_LIST_TYPES               \
  DECL_LIST_TYPE(UInt8List, uint8_t)      \
  DECL_LIST_TYPE(UInt16List, uint16_t)    \
  DECL_LIST_TYPE(UInt32List, uint32_t)    \
  DECL_LIST_TYPE(UInt64List, uint64_t)    \
  DECL_LIST_TYPE(StringList, ecs::string) \
  DECL_LIST_TYPE(EidList, ecs::EntityId)  \
  DECL_LIST_TYPE(FloatList, float)        \
  DECL_LIST_TYPE(Point2List, Point2)      \
  DECL_LIST_TYPE(Point3List, Point3)      \
  DECL_LIST_TYPE(Point4List, Point4)      \
  DECL_LIST_TYPE(IPoint2List, IPoint2)    \
  DECL_LIST_TYPE(IPoint3List, IPoint3)    \
  DECL_LIST_TYPE(IPoint4List, IPoint4)    \
  DECL_LIST_TYPE(BoolList, bool)          \
  DECL_LIST_TYPE(TMatrixList, TMatrix)    \
  DECL_LIST_TYPE(ColorList, E3DCOLOR)     \
  DECL_LIST_TYPE(Int8List, int8_t)        \
  DECL_LIST_TYPE(Int16List, int16_t)      \
  DECL_LIST_TYPE(IntList, int)            \
  DECL_LIST_TYPE(Int64List, int64_t)

namespace ecs
{
#define DECL_LIST_TYPE(type_alias, T) using type_alias = ecs::List<T>;
ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE
} // namespace ecs

#define DECL_LIST_TYPE(type_alias, T) ECS_DECLARE_RELOCATABLE_TYPE(ecs::type_alias);
ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE
