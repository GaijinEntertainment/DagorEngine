#ifndef _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_RAPIDJSON_H_
#define _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_RAPIDJSON_H_
#pragma once

#include "rapidjsonConfig.h"

#include <daScript/daScript.h>
#include <daScript/simulate/aot.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>
#include <rapidJsonUtils/rapidJsonUtils.h>


DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(rapidjson::Type, JsonType);
DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(rapidjson::ParseErrorCode, ParseErrorCode);

MAKE_TYPE_FACTORY(JsonValue, rapidjson::Value);
MAKE_TYPE_FACTORY(JsonDocument, rapidjson::Document);
MAKE_TYPE_FACTORY(JsonArray, rapidjson::Value::Array);
MAKE_TYPE_FACTORY(JsonConstArray, rapidjson::Value::ConstArray);

template <> struct das::isCloneable<rapidjson::Value::ConstArray> : false_type {};

namespace das
{
  template <>
  struct das_index<rapidjson::Value::Array>
  {
    static __forceinline rapidjson::Value & at(rapidjson::Value::Array &value, uint32_t index, Context *context)
    {
      if (index >= value.Size())
        context->throw_error_ex("Array index %d out of range %d", index, value.Size());
      return value[index];
    }
    static __forceinline rapidjson::Value & at(rapidjson::Value::Array &value, int32_t idx, Context *context)
    {
      return at(value, uint32_t(idx), context);
    }
  };

  template <>
  struct das_index<const rapidjson::Value::ConstArray>
  {
    static __forceinline const rapidjson::Value & at(const rapidjson::Value::ConstArray &value, uint32_t index, Context *context)
    {
      if (index >= value.Size())
        context->throw_error_ex("Array index %d out of range %d", index, value.Size());
      return value[index];
    }
    static __forceinline const rapidjson::Value & at(const rapidjson::Value::ConstArray &value, int32_t idx, Context *context)
    {
      return at(value, uint32_t(idx), context);
    }
  };

  template <typename ArrayType, typename ComponentType>
  struct das_json_array_iterator
  {
    __forceinline das_json_array_iterator(ArrayType &ar) : array(ar) {}
    template <typename QQ>
    __forceinline bool first(Context *, QQ *&value)
    {
      value = (QQ*)array.begin();
      end = array.end();
      return value != (QQ*)end;
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
  struct das_iterator<rapidjson::Value::Array>
    : das_json_array_iterator<rapidjson::Value::Array, rapidjson::Value>
  {
    __forceinline das_iterator(rapidjson::Value::Array &ar) : das_json_array_iterator(ar) {}
  };

  template <>
  struct das_iterator<const rapidjson::Value::ConstArray>
    : das_json_array_iterator<const rapidjson::Value::ConstArray, const rapidjson::Value>
  {
    __forceinline das_iterator(const rapidjson::Value::ConstArray &ar) : das_json_array_iterator(ar) {}
  };
}

namespace bind_dascript
{
  inline void PopBack(rapidjson::Value::Array &arr) { arr.PopBack(); }

  inline void SetObject(rapidjson::Value &json, const das::TBlock<void, rapidjson::Value> &block,
                        das::Context *context, das::LineInfoArg *at)
  {
    auto &obj = json.SetObject();
    vec4f arg = das::cast<rapidjson::Value *>::from(&obj);
    context->invoke(block, &arg, nullptr, at);
  }

  inline void SetArray(rapidjson::Value &json, const das::TBlock<void, rapidjson::Value> &block,
                       das::Context *context, das::LineInfoArg *at)
  {
    auto &obj = json.SetArray();
    vec4f arg = das::cast<rapidjson::Value *>::from(&obj);
    context->invoke(block, &arg, nullptr, at);
  }

  inline void Swap(rapidjson::Value &self, rapidjson::Value &other)
  {
    self.Swap(other);
  }

  template <typename T>
  inline void PushBack(rapidjson::Value &value, T member, rapidjson::Document &doc,
                       das::Context *context, das::LineInfoArg *at)
  {
    if (DAGOR_LIKELY(value.IsArray()))
      value.PushBack(rapidjson::Value(member, doc.GetAllocator()), doc.GetAllocator());
    else
      context->throw_error_at(at, "Array expected, instead got %d", value.GetType());
  }

  template <>
  inline void PushBack<const char *>(rapidjson::Value &value, const char * member, rapidjson::Document &doc,
                       das::Context *context, das::LineInfoArg *at)
  {
    if (DAGOR_LIKELY(value.IsArray()))
      value.PushBack(rapidjson::Value(member ? member : "", doc.GetAllocator()), doc.GetAllocator());
    else
      context->throw_error_at(at, "Array expected, instead got %d", value.GetType());
  }

  template <typename T>
  inline void PushBackVal(rapidjson::Value &value, T member, rapidjson::Document &doc,
                            das::Context *context, das::LineInfoArg *at)
  {
    if (DAGOR_LIKELY(value.IsArray()))
      value.PushBack(rapidjson::Value(member), doc.GetAllocator());
    else
      context->throw_error_at(at, "Array expected, instead got %d", value.GetType());
  }

  template <typename T>
  inline void AddMember(rapidjson::Value &value, const char *key, T member, rapidjson::Document &doc,
                         das::Context *context, das::LineInfoArg *at)
  {
    if (DAGOR_LIKELY(value.IsObject()))
      value.AddMember({key ? key : "", doc.GetAllocator()}, {member, doc.GetAllocator()}, doc.GetAllocator());
    else
      context->throw_error_at(at, "Object expected, instead got %d", value.GetType());
  }

  template <typename T>
  inline void AddMemberVal(rapidjson::Value &value, const char *key, T member, rapidjson::Document &doc,
                           das::Context *context, das::LineInfoArg *at)
  {
    if (DAGOR_LIKELY(value.IsObject()))
      value.AddMember({key ? key : "", doc.GetAllocator()}, rapidjson::Value{member}, doc.GetAllocator());
    else
      context->throw_error_at(at, "Can add member '%s' to object only", key);
  }

  template<>
  inline void AddMember<const char *>(rapidjson::Value &value, const char *key, const char *member, rapidjson::Document &doc,
                         das::Context *context, das::LineInfoArg *at)
  {
    if (DAGOR_LIKELY(value.IsObject()))
      value.AddMember({key ? key : "", doc.GetAllocator()}, {member ? member : "", doc.GetAllocator()}, doc.GetAllocator());
    else
      context->throw_error_at(at, "Object expected, instead got %d", value.GetType());
  }

  template <typename Json,
            typename ArrType = eastl::conditional_t<eastl::is_const_v<Json>, rapidjson::Value::ConstArray, rapidjson::Value::Array>,
            typename TBlock = typename eastl::conditional_t<eastl::is_const_v<Json>,
                               das::TBlock<void, const ArrType>,
                               das::TBlock<void, ArrType>>>
  inline bool GetArray(Json &json, const TBlock &block,
                       das::Context *context, das::LineInfoArg *at)
  {
    if (json.IsArray())
    {
      ArrType arr = json.GetArray();
      vec4f arg = das::cast<ArrType *>::from(&arr);
      context->invoke(block, &arg, nullptr, at);
      return true;
    }

    return false;
  }

  template <typename Json,
            typename TBlock = typename eastl::conditional_t<eastl::is_const_v<Json>,
                               das::TBlock<void, const Json>,
                               das::TBlock<void, Json>>>
  inline bool FindMember(Json &json, const char *key, rapidjson::Type jst, const TBlock &block,
                         das::Context *context, das::LineInfoArg *at)
  {
    if (!json.IsObject())
      return false;

    const auto memberIterator = json.FindMember(key ? key : "");
    if (memberIterator != json.MemberEnd() && memberIterator->value.GetType() == jst)
    {
      vec4f arg = das::cast<Json *>::from(&memberIterator->value);
      context->invoke(block, &arg, nullptr, at);
      return true;
    }

    return false;
  }

  inline void json_stringify(const rapidjson::Value &json, const das::TBlock<void, das::TTemporary<const char*>> &block,
                             das::Context *context, das::LineInfoArg *at)
  {
    rapidjson::StringBuffer str = jsonutils::stringify<false>(json);
    vec4f args = das::cast<const char*>::from(str.GetString());
    context->invoke(block, &args, nullptr, at);
  }

  inline void json_stringify_pretty(const rapidjson::Value &json, const das::TBlock<void, das::TTemporary<const char*>> &block,
                                    das::Context *context, das::LineInfoArg *at)
  {
    rapidjson::StringBuffer str = jsonutils::stringify<true>(json);
    vec4f args = das::cast<const char*>::from(str.GetString());
    context->invoke(block, &args, nullptr, at);
  }

  inline void document_Parse(rapidjson::Document &doc, const char* str)
  {
    doc.Parse(str ? str : "", str ? strlen(str) : 0);
  }

}

#endif // _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_RAPIDJSON_H_
