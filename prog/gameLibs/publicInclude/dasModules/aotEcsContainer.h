//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/component.h>
#include <daECS/core/sharedComponent.h>
#include <dasModules/aotEcs.h>


namespace bind_dascript
{

#define TYPE(type)                                                                                                              \
  inline void setInitHint##type(ecs::ComponentsInitializer &init, const char *key, uint32_t key_hash,                           \
    typename DasTypeParamAlias<type>::cparam_alias to)                                                                          \
  {                                                                                                                             \
    init[ecs::HashedConstString({key, key_hash})] = *(const type *)&to;                                                         \
  }                                                                                                                             \
  inline void setInit##type(ecs::ComponentsInitializer &init, const char *s, typename DasTypeParamAlias<type>::cparam_alias to) \
  {                                                                                                                             \
    if (s)                                                                                                                      \
      init[ECS_HASH_SLOW(s ? s : "")] = *(const type *)&to;                                                                     \
    else                                                                                                                        \
      logerr("unsupported empty string as key for ComponentsInitializer");                                                      \
  }                                                                                                                             \
  inline void fastSetInit##type(ecs::ComponentsInitializer &init, const ecs::component_t name, const ecs::FastGetInfo &lt,      \
    typename DasTypeParamAlias<type>::cparam_alias to, const char *nameStr)                                                     \
  {                                                                                                                             \
    init.insert(name, lt, *(const type *)&to, nameStr);                                                                         \
  }
ECS_BASE_TYPE_LIST
ECS_LIST_TYPE_LIST
#undef TYPE
inline void setInitStrHint(ecs::ComponentsInitializer &init, const char *key, uint32_t key_hash, const char *to)
{
  init[ecs::HashedConstString({key, key_hash})] = ecs::string(to ? to : "");
}
inline void setInitStr(ecs::ComponentsInitializer &init, const char *s, const char *to)
{
  setInitStrHint(init, s, ECS_HASH_SLOW(s ? s : "").hash, to);
}
inline void setVec4f(ecs::ComponentsInitializer &init, const char *s, vec4f value) { init[ECS_HASH_SLOW(s ? s : "")] = value; }
inline void fastSetInitStr(ecs::ComponentsInitializer &init, const ecs::component_t name, const ecs::FastGetInfo &lt, const char *to,
  const char *nameStr)
{
  init.insert(name, lt, to ? to : "", nameStr);
}
inline void setInitChildComponentHint(ecs::ComponentsInitializer &init, const char *key, uint32_t key_hash,
  const ecs::ChildComponent &to)
{
  init[ecs::HashedConstString({key, key_hash})] = to;
}
inline void setInitChildComponent(ecs::ComponentsInitializer &init, const char *s, const ecs::ChildComponent &to)
{
  setInitChildComponentHint(init, s, ECS_HASH_SLOW(s ? s : "").hash, to);
}
inline void fastSetInitChildComp(ecs::ComponentsInitializer &init, const ecs::component_t name, const ecs::FastGetInfo &lt,
  const ecs::ChildComponent &to, const char *nameStr)
{
  init.insert(name, lt, to, nameStr);
}
#define TYPE(type)                                                                                                        \
  inline void setCompMapHint##type(ecs::ComponentsMap &map, const char *key, uint32_t key_hash,                           \
    typename DasTypeParamAlias<type>::cparam_alias to)                                                                    \
  {                                                                                                                       \
    map[ecs::HashedConstString({key, key_hash})] = *(const type *)&to;                                                    \
  }                                                                                                                       \
  inline void setCompMap##type(ecs::ComponentsMap &map, const char *s, typename DasTypeParamAlias<type>::cparam_alias to) \
  {                                                                                                                       \
    setCompMapHint##type(map, s, ECS_HASH_SLOW(s ? s : "").hash, to);                                                     \
  }
ECS_BASE_TYPE_LIST
ECS_LIST_TYPE_LIST
#undef TYPE
inline void setCompMapStrHint(ecs::ComponentsMap &map, const char *key, uint32_t key_hash, const char *to)
{
  map[ecs::HashedConstString({key, key_hash})] = ecs::string(to ? to : "");
}
inline void setCompMapStr(ecs::ComponentsMap &map, const char *s, const char *to)
{
  setCompMapStrHint(map, s, ECS_HASH_SLOW(s ? s : "").hash, to);
}

inline const ecs::Template *getTemplateByName(const char *s) { return g_entity_mgr->getTemplateDB().getTemplateByName(s ? s : ""); }
inline const ecs::Template *buildTemplateByName(const char *s) { return g_entity_mgr->buildTemplateByName(s ? s : ""); }
inline bool templateHasComponentHint(const ecs::Template &tpl, const char *key, uint32_t key_hash)
{
  return tpl.hasComponent(ecs::HashedConstString({key, key_hash}), g_entity_mgr->getTemplateDB().data());
};
inline const char *getTemplatePath(const ecs::Template &tpl) { return tpl.getPath(); }
inline bool templateHasComponent(const ecs::Template &tpl, const char *s)
{
  return templateHasComponentHint(tpl, s, ECS_HASH_SLOW(s ? s : "").hash);
};
inline const ecs::ChildComponent *getTemplateComponentHint(const ecs::Template &tpl, const char *component_name,
  uint32_t component_key)
{
  return &tpl.getComponent(ecs::HashedConstString({component_name, component_key}), g_entity_mgr->getTemplateDB().data());
}
inline const ecs::ChildComponent *getTemplateComponent(const ecs::Template &tpl, const char *component_name)
{
  return getTemplateComponentHint(tpl, component_name, ECS_HASH_SLOW(component_name ? component_name : "").hash);
}


inline void setChildComponentStr(ecs::ChildComponent &o, const char *to) { o = ecs::string(to ? to : ""); }
inline bool is_object_empty(const ecs::Object &o) { return o.empty(); }
inline int32_t get_object_length(const ecs::Object &o) { return int32_t(o.size()); }
inline void setObjectStrHint(ecs::Object &o, const char *key, uint32_t key_hash, const char *to)
{
  o.addMember(ecs::HashedConstString({key, key_hash}), to ? to : "");
}
inline void setObjectStr(ecs::Object &o, const char *s, const char *to) { setObjectStrHint(o, s, ECS_HASH_SLOW(s ? s : "").hash, to); }
inline void setObjectChildComponentHint(ecs::Object &o, const char *key, uint32_t key_hash, const ecs::ChildComponent &to)
{
  o.addMember(ecs::HashedConstString({key, key_hash}), to);
}
inline void setObjectChildComponent(ecs::Object &o, const char *s, const ecs::ChildComponent &to)
{
  setObjectChildComponentHint(o, s, ECS_HASH_SLOW(s ? s : "").hash, to);
}
inline ecs::ChildComponent &insertHint(ecs::Object &o, const char *key, uint32_t key_hash)
{
  return o.insert(ecs::HashedConstString({key, key_hash}));
}
inline ecs::ChildComponent &insert(ecs::Object &o, const char *s) { return insertHint(o, s, ECS_HASH_SLOW(s ? s : "").hash); }
inline void clearObject(ecs::Object &o) { o.clear(); }
inline bool objectHasHint(const ecs::Object &o, const char *key, uint32_t key_hash)
{
  return o.hasMember(ecs::HashedConstString({key, key_hash}));
}
inline bool objectHas(const ecs::Object &o, const char *s) { return objectHasHint(o, s, ECS_HASH_SLOW(s ? s : "").hash); }
inline const char *objectGetStringHint(const ecs::Object &o, const char *key, uint32_t key_hash, const char *default_value)
{
  const eastl::string *res = o.getNullable<ecs::string>(ecs::HashedConstString({key, key_hash}));
  return res != nullptr ? res->c_str() : default_value;
}
inline const char *objectGetString(const ecs::Object &o, const char *key, const char *default_value)
{
  return objectGetStringHint(o, key, ECS_HASH_SLOW(key ? key : "").hash, default_value);
}
inline const char *objectPtrGetStringHint(const ecs::Object *o, const char *key, uint32_t key_hash, const char *default_value)
{
  return o ? objectGetStringHint(*o, key, key_hash, default_value) : default_value;
}
inline const char *objectPtrGetString(const ecs::Object *o, const char *key, const char *default_value)
{
  return o ? objectGetStringHint(*o, key, ECS_HASH_SLOW(key ? key : "").hash, default_value) : default_value;
}

inline const ecs::ChildComponent *objectGetChildHint(const ecs::Object &o, const char *key, uint32_t key_hash)
{
  auto res = o.find_as(ecs::HashedConstString({key, key_hash}));
  return res != o.end() ? &res->second : nullptr;
}
inline const ecs::ChildComponent *objectGetChild(const ecs::Object &o, const char *key)
{
  return objectGetChildHint(o, key, ECS_HASH_SLOW(key ? key : "").hash);
}
inline const ecs::ChildComponent *objectPtrGetChildHint(const ecs::Object *o, const char *key, uint32_t key_hash)
{
  return o ? objectGetChildHint(*o, key, key_hash) : nullptr;
}
inline const ecs::ChildComponent *objectPtrGetChild(const ecs::Object *o, const char *key)
{
  return o ? objectGetChildHint(*o, key, ECS_HASH_SLOW(key ? key : "").hash) : nullptr;
}

inline ecs::ChildComponent *objectGetRWChildHint(ecs::Object &o, const char *key, uint32_t key_hash)
{
  auto res = o.find_as(ecs::HashedConstString({key, key_hash}));
  return res != o.end() ? &res->second : nullptr;
}
inline ecs::ChildComponent *objectGetRWChild(ecs::Object &o, const char *key)
{
  return objectGetRWChildHint(o, key, ECS_HASH_SLOW(key ? key : "").hash);
}
inline ecs::ChildComponent *objectPtrGetRWChildHint(ecs::Object *o, const char *key, uint32_t key_hash)
{
  return o ? objectGetRWChildHint(*o, key, key_hash) : nullptr;
}
inline ecs::ChildComponent *objectPtrGetRWChild(ecs::Object *o, const char *key)
{
  return o ? objectGetRWChildHint(*o, key, ECS_HASH_SLOW(key ? key : "").hash) : nullptr;
}


inline void eraseObject(ecs::Object &a, const char *s) { a.erase(a.find_as(s ? s : "")); }
inline void eraseObjectHint(ecs::Object &a, const char *key, uint32_t key_hash)
{
  a.erase(a.find_as(ecs::HashedConstString({key, key_hash})));
}

inline void moveObject(ecs::Object &a, ecs::Object &b) { a = eastl::move(b); }
inline void swapObjects(ecs::Object &a, ecs::Object &b) { eastl::swap(a, b); }

inline bool is_array_empty(const ecs::Array &o) { return o.empty(); }
inline int32_t get_array_length(const ecs::Array &o) { return int32_t(o.size()); }
inline void pushArrayStr(ecs::Array &a, const char *to) { a.push_back(ecs::string(to ? to : "")); }
inline void pushAtArrayStr(ecs::Array &a, int idx, const char *to) { a.insert(a.begin() + idx, ecs::string(to ? to : "")); }
inline void pushChildComponent(ecs::Array &a, const ecs::ChildComponent &to) { a.push_back(to); }
inline void popArray(ecs::Array &a) { a.pop_back(); }
inline void clearArray(ecs::Array &a) { a.clear(); }
inline void reserveArray(ecs::Array &a, int count) { a.reserve(count); }
inline void moveArray(ecs::Array &a, ecs::Array &b) { a = eastl::move(b); }
inline void eraseArray(ecs::Array &a, int s, das::Context *ctx, das::LineInfoArg *line_info)
{
  if (a.size() <= uint32_t(s))
    ctx->throw_error_at(line_info, "Array::erase() index out of range, %d of %u", uint32_t(s), a.size());
  a.erase(a.begin() + s);
}
inline void eraseCountArray(ecs::Array &a, int s, int count, das::Context *ctx, das::LineInfoArg *line_info)
{
  if (count < 0)
    ctx->throw_error_at(line_info, "Array::erase() count is negative - %d", count);
  if (s + count > a.size())
    ctx->throw_error_at(line_info, "Array::erase() idx + count out of range, %d of %u", s + count, a.size());
  a.erase(a.begin() + s, a.begin() + s + count);
}

// this is helping class for all iterators, that iterates over data we need (kinda vector<>)
template <class Array>
struct DasArrayIterator : das::Iterator
{
  DasArrayIterator(Array *ar) : array(ar) {}
  virtual bool first(das::Context &, char *_value) override
  {
    if (!array->size())
      return false;
    iterator_type *value = (iterator_type *)_value;
    *value = &(*array)[0];
    end = array->end();
    return true;
  }
  virtual bool next(das::Context &, char *_value) override
  {
    iterator_type *value = (iterator_type *)_value;
    ++(*value);
    return *value != end;
  }
  virtual void close(das::Context &context, char *_value) override
  {
    if (_value)
    {
      iterator_type *value = (iterator_type *)_value;
      *value = nullptr;
    }
    context.heap->free((char *)this, sizeof(DasArrayIterator<Array>));
  }
  Array *array = nullptr; // can be unioned with end
  typedef decltype(array->begin()) iterator_type;
  iterator_type end;
};

template <typename TT>
das::Sequence das_Array_each_sequence(const TT &vec, das::Context *context)
{
  using Iterator = DasArrayIterator<TT>;
  char *iter = context->heap->allocate(sizeof(Iterator));
  context->heap->mark_comment(iter, "Tab<> iterator");
  new (iter) Iterator((TT *)&vec);
  return {(Iterator *)iter};
}

__forceinline das::TSequence<ecs::ChildComponent &> das_Array_each(ecs::Array &vec, das::Context *context)
{
  return das_Array_each_sequence(vec, context);
}

__forceinline das::TSequence<const ecs::ChildComponent &> das_Array_each_const(const ecs::Array &vec, das::Context *context)
{
  return das_Array_each_sequence(vec, context);
}

template <typename T>
inline bool emptyList(const T &list)
{
  return list.empty();
}
template <typename T>
inline int32_t lengthList(const T &list)
{
  return (int32_t)list.size();
}
template <typename T>
inline void resizeList(T &list, int size)
{
  list.resize(size);
}
template <typename T>
inline void reserveList(T &list, int size)
{
  list.reserve(size);
}
template <typename T>
inline void clearList(T &list)
{
  list.clear();
}
template <typename T>
inline void eraseList(T &list, int idx, das::Context *context)
{
  if (uint32_t(idx) >= list.size())
    context->throw_error_ex("erase list out of range, %u of %u", uint32_t(idx), list.size());
  list.erase(list.begin() + idx);
}
template <typename T>
inline void eraseListRange(T &list, int idx, int count, das::Context *context)
{
  if (idx < 0 || count < 0 || uint32_t(idx + count) > list.size())
    context->throw_error_ex("erasing list range is invalid: index=%i count=%i size=%u", idx, count, list.size());
  list.erase(list.begin() + idx, list.begin() + idx + count);
}
template <typename T>
inline void popList(T &list)
{
  list.pop_back();
}
template <typename T, typename V = typename T::value_type>
inline void pushList(T &a, typename DasTypeParamAlias<V>::cparam_alias to)
{
  a.push_back(*(const V *)&to);
}
inline void pushStringList(ecs::StringList &a, const char *to) { a.push_back(ecs::string(to ? to : "")); }
template <typename T, typename V = typename T::value_type>
inline void emplaceList(T &a, typename DasTypeParamAlias<V>::param_alias to)
{
  a.emplace_back(std::move(*(V *)&to));
}
inline void emplaceStringList(ecs::StringList &a, const char *to) { a.emplace_back(ecs::string(to ? to : "")); }
template <typename T>
inline const typename T::value_type *dataPtrList(const T &list)
{
  return list.data();
}
template <typename T>
inline void moveList(T &a, T &b)
{
  a = eastl::move(b);
}
template <typename T>
inline void swapLists(T &a, T &b)
{
  eastl::swap(a, b);
}


#define TYPE(type)                                                                                                                    \
  inline void setChildComponent##type(ecs::ChildComponent &o, typename DasTypeParamAlias<type>::cparam_alias to)                      \
  {                                                                                                                                   \
    o = *(const type *)&to;                                                                                                           \
  }                                                                                                                                   \
  inline const typename DasTypeAlias<type>::alias *getChildComponent##type(const ecs::ChildComponent &o)                              \
  {                                                                                                                                   \
    return (const typename DasTypeAlias<type>::alias *)o.getNullable<type>();                                                         \
  }                                                                                                                                   \
  inline const typename DasTypeAlias<type>::alias *getChildComponentPtr##type(const ecs::ChildComponent *o)                           \
  {                                                                                                                                   \
    return o ? ((const typename DasTypeAlias<type>::alias *)o->getNullable<type>()) : nullptr;                                        \
  }                                                                                                                                   \
  inline typename DasTypeAlias<type>::alias *getChildComponentRW##type(ecs::ChildComponent &o)                                        \
  {                                                                                                                                   \
    return (typename DasTypeAlias<type>::alias *)o.getNullable<type>();                                                               \
  }                                                                                                                                   \
  inline typename DasTypeAlias<type>::alias *getChildComponentPtrRW##type(ecs::ChildComponent *o)                                     \
  {                                                                                                                                   \
    return o ? ((typename DasTypeAlias<type>::alias *)o->getNullable<type>()) : nullptr;                                              \
  }                                                                                                                                   \
  inline void setObjectHint##type(ecs::Object &o, const char *key, uint32_t key_hash,                                                 \
    typename DasTypeParamAlias<type>::cparam_alias to)                                                                                \
  {                                                                                                                                   \
    o.addMember(ecs::HashedConstString({key, key_hash}), *(const type *)&to);                                                         \
  }                                                                                                                                   \
  inline void setObject##type(ecs::Object &o, const char *s, typename DasTypeParamAlias<type>::cparam_alias to)                       \
  {                                                                                                                                   \
    o.addMember(s ? s : "", *(const type *)&to);                                                                                      \
  }                                                                                                                                   \
  inline void setArray##type(ecs::Array &a, int s, typename DasTypeParamAlias<type>::cparam_alias to, das::Context *context,          \
    das::LineInfoArg *line_info)                                                                                                      \
  {                                                                                                                                   \
    if (s >= a.size())                                                                                                                \
      context->throw_error_at(line_info, "Array index %d out of range %d", s, a.size());                                              \
    a[s] = *(const type *)&to;                                                                                                        \
  }                                                                                                                                   \
  inline const typename DasTypeAlias<type>::alias *getObject##type(const ecs::Object &o, const char *s)                               \
  {                                                                                                                                   \
    return (const typename DasTypeAlias<type>::alias *)o.getNullable<type>(ECS_HASH_SLOW(s ? s : ""));                                \
  }                                                                                                                                   \
  inline const typename DasTypeAlias<type>::alias *getObjectHint##type(const ecs::Object &o, const char *key, uint32_t key_hash)      \
  {                                                                                                                                   \
    return (const typename DasTypeAlias<type>::alias *)o.getNullable<type>(ecs::HashedConstString({key, key_hash}));                  \
  }                                                                                                                                   \
  inline const typename DasTypeAlias<type>::alias *getObjectPtr##type(const ecs::Object *o, const char *s)                            \
  {                                                                                                                                   \
    return o ? ((const typename DasTypeAlias<type>::alias *)o->getNullable<type>(ECS_HASH_SLOW(s ? s : ""))) : nullptr;               \
  }                                                                                                                                   \
  inline const typename DasTypeAlias<type>::alias *getObjectPtrHint##type(const ecs::Object *o, const char *key, uint32_t key_hash)   \
  {                                                                                                                                   \
    return o ? ((const typename DasTypeAlias<type>::alias *)o->getNullable<type>(ecs::HashedConstString({key, key_hash}))) : nullptr; \
  }                                                                                                                                   \
  inline typename DasTypeAlias<type>::alias *getArray##type(const ecs::Array &o, int s)                                               \
  {                                                                                                                                   \
    return (typename DasTypeAlias<type>::alias *)o[s].getNullable<type>();                                                            \
  }                                                                                                                                   \
  inline typename DasTypeAlias<type>::alias *getObjectRW##type(ecs::Object &o, const char *s)                                         \
  {                                                                                                                                   \
    return (typename DasTypeAlias<type>::alias *)o.getNullableRW<type>(ECS_HASH_SLOW(s ? s : ""));                                    \
  }                                                                                                                                   \
  inline typename DasTypeAlias<type>::alias *getObjectRWHint##type(ecs::Object &o, const char *key, uint32_t key_hash)                \
  {                                                                                                                                   \
    return (typename DasTypeAlias<type>::alias *)o.getNullableRW<type>(ecs::HashedConstString({key, key_hash}));                      \
  }                                                                                                                                   \
  inline typename DasTypeAlias<type>::alias *getObjectPtrRW##type(ecs::Object *o, const char *s)                                      \
  {                                                                                                                                   \
    return o ? ((typename DasTypeAlias<type>::alias *)o->getNullableRW<type>(ECS_HASH_SLOW(s ? s : ""))) : nullptr;                   \
  }                                                                                                                                   \
  inline typename DasTypeAlias<type>::alias *getObjectPtrRWHint##type(ecs::Object *o, const char *key, uint32_t key_hash)             \
  {                                                                                                                                   \
    return o ? ((typename DasTypeAlias<type>::alias *)o->getNullableRW<type>(ecs::HashedConstString({key, key_hash}))) : nullptr;     \
  }                                                                                                                                   \
  inline typename DasTypeAlias<type>::alias *getArrayRW##type(ecs::Array &o, int s)                                                   \
  {                                                                                                                                   \
    return (typename DasTypeAlias<type>::alias *)o[s].getNullable<type>();                                                            \
  }                                                                                                                                   \
  inline void pushArray##type(ecs::Array &a, typename DasTypeParamAlias<type>::cparam_alias to)                                       \
  {                                                                                                                                   \
    a.push_back(*(const type *)&to);                                                                                                  \
  }                                                                                                                                   \
  inline void pushAtArray##type(ecs::Array &a, int idx, typename DasTypeParamAlias<type>::cparam_alias to)                            \
  {                                                                                                                                   \
    a.insert(a.begin() + idx, *(const type *)&to);                                                                                    \
  }
ECS_BASE_TYPE_LIST
ECS_LIST_TYPE_LIST
#undef TYPE

struct ObjectIterator
{
  ecs::Object::iterator it;
  char *get_key() { return (char *)ecs::get_key_string(it).c_str(); }
  ecs::ChildComponent &get_value() { return (it->second); }
};

struct ObjectConstIterator
{
  mutable ecs::Object::const_iterator it;
  char *get_key() const { return (char *)ecs::get_key_string(it).c_str(); }
  const ecs::ChildComponent &get_value() const { return (it->second); }
};

} // namespace bind_dascript

namespace das
{

template <typename ArrayType, typename ComponentType>
struct das_array_iterator
{
  __forceinline das_array_iterator(ArrayType &ar) : array(ar) {}
  template <typename QQ>
  __forceinline bool first(Context *, QQ *&value)
  {
    value = (QQ *)array.begin();
    end = array.end();
    return value != (QQ *)end;
  }
  template <typename QQ>
  __forceinline bool next(Context *, QQ *&value)
  {
    ++value;
    return value != (QQ *)end;
  }
  template <typename QQ>
  __forceinline void close(Context *, QQ *&value)
  {
    value = nullptr;
  }
  ArrayType &array;
  ComponentType *end;
};

template <>
struct das_iterator<ecs::Array> : das_array_iterator<ecs::Array, ecs::ChildComponent>
{
  __forceinline das_iterator(ecs::Array &ar) : das_array_iterator(ar) {}
};

template <>
struct das_iterator<const ecs::Array> : das_array_iterator<const ecs::Array, const ecs::ChildComponent>
{
  __forceinline das_iterator(const ecs::Array &ar) : das_array_iterator(ar) {}
};

#define DECL_DAS_ITERATOR(list_type, item_type)                             \
  template <>                                                               \
  struct das_iterator<list_type> : das_array_iterator<list_type, item_type> \
  {                                                                         \
    __forceinline das_iterator(list_type &ar) : das_array_iterator(ar)      \
    {}                                                                      \
  };

#define DECL_DAS_INDEX(list_type, item_type)                                                  \
  template <>                                                                                 \
  struct das_index<list_type>                                                                 \
  {                                                                                           \
    static __forceinline item_type &at(list_type &value, uint32_t index, Context *context)    \
    {                                                                                         \
      if (index >= value.size())                                                              \
        context->throw_error_ex(#list_type " index %d out of range %d", index, value.size()); \
      return value[index];                                                                    \
    }                                                                                         \
    static __forceinline item_type &at(list_type &value, int32_t idx, Context *context)       \
    {                                                                                         \
      return at(value, uint32_t(idx), context);                                               \
    }                                                                                         \
  };

#define DECL_DAS_ITERATOR_AND_INDEX(list_type, item_type) \
  DECL_DAS_ITERATOR(list_type, item_type)                 \
  DECL_DAS_ITERATOR(const list_type, const item_type)     \
  DECL_DAS_INDEX(list_type, item_type)                    \
  DECL_DAS_INDEX(const list_type, const item_type)

#define DECL_LIST_TYPE(lt, t) DECL_DAS_ITERATOR_AND_INDEX(ecs::lt, t)
ECS_DECL_LIST_TYPES
#undef DECL_LIST_TYPE

#undef DECL_DAS_INDEX
#undef DECL_DAS_ITERATOR
#undef DECL_DAS_ITERATOR_AND_INDEX

template <typename ObjType, typename ObjItType, typename ItType>
struct das_object_iterator
{
  __forceinline das_object_iterator(ObjType &obj) : object(&obj) {}
  __forceinline bool first(Context *, ItType *&value)
  {
    if (!object->size())
      return false;
    current.it = object->begin();
    end = object->end();
    G_ASSERT(current.it != end);
    value = &current;
    return true;
  }
  __forceinline bool next(Context *, ItType *&value)
  {
    (*value).it++;
    return value->it != end;
  }
  __forceinline void close(Context *, ItType *&value) { value = nullptr; }
  ObjType *object = nullptr;
  typename das::remove_const<ItType>::type current;
  ObjItType end;
};

template <>
struct das_iterator<ecs::Object> : das_object_iterator<ecs::Object, ecs::Object::iterator, bind_dascript::ObjectIterator>
{
  __forceinline das_iterator(ecs::Object &obj) : das_object_iterator(obj) {}
};

template <>
struct das_iterator<const ecs::Object>
  : das_object_iterator<const ecs::Object, ecs::Object::const_iterator, const bind_dascript::ObjectConstIterator>
{
  __forceinline das_iterator(const ecs::Object &obj) : das_object_iterator(obj) {}
};

template <>
struct das_index<ecs::Object>
{
  static __forceinline ecs::ChildComponent *at(ecs::Object &value, const char *index, Context *)
  {
    ecs::ChildComponent *result = nullptr;
    auto it = value.find_as(ECS_HASH_SLOW(index ? index : ""));
    if (it != value.end())
      result = &it->second;
    return result;
  }
};

template <>
struct das_index<const ecs::Object>
{
  static __forceinline const ecs::ChildComponent *at(const ecs::Object &value, const char *index, Context *)
  {
    const ecs::ChildComponent *result = nullptr;
    auto it = value.find_as(ECS_HASH_SLOW(index ? index : ""));
    if (it != value.end())
      result = &it->second;
    return result;
  }
};

template <>
struct das_index<ecs::Array>
{
  static __forceinline ecs::ChildComponent &at(ecs::Array &value, uint32_t index, Context *context)
  {
    if (index >= value.size())
      context->throw_error_ex("Array index %d out of range %d", index, value.size());
    return value[index];
  }
  static __forceinline ecs::ChildComponent &at(ecs::Array &value, int32_t idx, Context *context)
  {
    return at(value, uint32_t(idx), context);
  }
};

template <>
struct das_index<const ecs::Array>
{
  static __forceinline const ecs::ChildComponent &at(const ecs::Array &value, uint32_t index, Context *context)
  {
    if (index >= value.size())
      context->throw_error_ex("Array index %d out of range %d", index, value.size());
    return value[index];
  }
  static __forceinline const ecs::ChildComponent &at(const ecs::Array &value, int32_t idx, Context *context)
  {
    return at(value, uint32_t(idx), context);
  }
};
} // namespace das

namespace das
{
template <>
struct das_new<::ecs::Object>
{
  typedef ::ecs::Object TT;
  static __forceinline TT *make(Context *) { return new ::ecs::Object; }

  template <typename QQ>
  static __forceinline TT *make_and_init(Context *__context__, QQ &&init)
  {
    TT *data = make(__context__);
    *data = init();
    return data;
  }
};
template <>
struct das_delete<::ecs::Object *>
{
  typedef ::ecs::Object TT;
  static __forceinline void clear(Context *, TT *&ptr)
  {
    delete ptr;
    ptr = nullptr;
  }
};
} // namespace das
