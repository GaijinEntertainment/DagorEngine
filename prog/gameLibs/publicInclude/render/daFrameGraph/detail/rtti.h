//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <debug/dag_log.h>
#include <generic/dag_fixedMoveOnlyFunction.h>
#include <generic/dag_functionRef.h>
#include <render/daFrameGraph/detail/resourceType.h>
#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>
#include <EASTL/utility.h>
#include <EASTL/span.h>

#ifndef DAFG_DEBUG_RTTI
#define DAFG_DEBUG_RTTI DAGOR_DBGLEVEL > 0
#endif

#if DAFG_DEBUG_RTTI
#include "rttiReflection.h"
#include <util/dag_string.h>
#include <EASTL/string_view.h>
#include <EASTL/type_traits.h>
#include <vecmath/dag_vecMathDecl.h>
#endif


namespace dafg::detail
{

struct RTTI
{
  using Offset = size_t;
  using Tag = dafg::ResourceSubtypeTag;
  using Field = eastl::pair<Offset, Tag>;
  using TypeFields = dag::Vector<Field>;

  using TaggedRTTI = eastl::pair<Tag, RTTI>;
  using TempStorage = dag::Vector<TaggedRTTI, framemem_allocator>;
  using StorageView = eastl::span<TaggedRTTI>;

  using TaggerRef = dag::FunctionRef<Tag()>;
  using MakerRef = dag::FunctionRef<RTTI()>;
  using RecursiveMakerRef = dag::FunctionRef<RTTI::TaggedRTTI(RTTI::TempStorage &)>;

  size_t size;
  size_t align;
  dag::FixedMoveOnlyFunction<4 * sizeof(void *), void(void *) const> ctor;
  dag::FixedMoveOnlyFunction<4 * sizeof(void *), void(void *) const> dtor;
  dag::FixedMoveOnlyFunction<4 * sizeof(void *), void(void *, const void *) const> copy;

#if DAFG_DEBUG_RTTI
  // blobs can be created in cpp code or in dascript
  // so we keep both variants of name to show them all
  // fields are simply pairs of offsets in parent type and type tags
  eastl::string_view name;
  String dasName;
  TypeFields fields;
#endif
};


// returns rtti for type T
template <class T>
RTTI make_rtti()
{
  return RTTI{sizeof(T), alignof(T),
    // IMPORTANT: () zero-initializes structs
    +[](void *p) { new (p) T(); }, +[](void *p) { static_cast<T *>(p)->~T(); },
    +[](void *ptr, const void *from) {
      if constexpr (eastl::is_copy_constructible<T>::value)
      {
        new (ptr) T(*reinterpret_cast<const T *>(from));
      }
      else
      {
        G_UNUSED(from);
        logerr("daFG: blob type is NOT copy constructible, constructing NEW blob instead");
        new (ptr) T();
      }
    },
#if DAFG_DEBUG_RTTI
    TypeName<T>::Get(), String{}, RTTI::TypeFields{}
#endif
  };
}

// returns tagged rtti for type T
// stores tagged rttis of its fields recursively in sub_storage
template <class T>
RTTI::TaggedRTTI make_rtti_recursive(RTTI::TempStorage &sub_storage)
{
  RTTI::TaggedRTTI typeRtti = {dafg::tag_for<T>(), make_rtti<T>()};

#if DAFG_DEBUG_RTTI
  if constexpr (eastl::is_aggregate_v<T> && !eastl::is_union_v<T> && !eastl::is_same_v<T, vec4f>)
  {
    constexpr size_t fieldsNum = count_fields<T>({});
    static_assert(fieldsNum <= 64,
      "Blobs with types, having more than 64 members are not supported yet!\nPlease, use less massive type.");
    static_assert(
      requires { tie_as_tuple<T>(T(), size_t_<fieldsNum>{}); },
      "Blob probably contains C-style array. Please, avoid using it in blobs.\nInstead of int a[4], use eastl::array<int, 4>.");
    using FieldsTupleType = decltype(tie_as_tuple<T>(T(), size_t_<fieldsNum>{}));

    size_t offset = 0;
    RTTI::TypeFields typeFields;
    typeFields.reserve(fieldsNum);

    ForLoop<fieldsNum>::iterate([&]<size_t I>() {
      using FieldType = eastl::remove_cv_t<eastl::remove_reference_t<eastl::tuple_element_t<I, FieldsTupleType>>>;
      constexpr size_t fieldSize = sizeof(FieldType);
      constexpr size_t fieldAlign = alignof(FieldType);

      if (offset % fieldAlign != 0)
        offset += (fieldAlign - offset % fieldAlign);
      typeFields.emplace_back(offset, dafg::tag_for<FieldType>());
      offset += fieldSize;

      sub_storage.emplace_back(eastl::move(make_rtti_recursive<FieldType>(sub_storage)));
    });

    typeRtti.second.fields = eastl::move(typeFields);
  }
#else
  G_UNUSED(sub_storage);
#endif

  return typeRtti;
}

} // namespace dafg::detail
