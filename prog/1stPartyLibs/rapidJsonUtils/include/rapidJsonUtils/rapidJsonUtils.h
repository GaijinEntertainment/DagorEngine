//
// Dagor Engine 6.5 - 1st party libs
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace jsonutils
{
  // the result type is const rapidjson::Value* for const json argument and rapidjson::Value* for mutable json argument
  template<typename MemberType, typename Json>
  inline auto get_ptr(Json &json, const char *key, eastl::string /*out*/ *err_str = nullptr) -> decltype(&json.FindMember(key)->value)
  {
    auto fail = [&](const char *format, auto && ...args)
    {
      if (err_str)
        err_str->sprintf(format, eastl::forward<decltype(args)>(args)...);
      return nullptr;
    };

    if (!json.IsObject())
      return fail("json value is not object (type=%#x)", json.GetType());

    const auto memberIterator = json.FindMember(key ? key : "");
    if (memberIterator != json.MemberEnd() && memberIterator->value.template Is<MemberType>())
      return &memberIterator->value;

    return fail("json value has no '%s' member", key);
  }

  // if MemberType isn't trivially copyable, consider rewriting get_or to take "fallback" by forwarding reference
  template<typename MemberType, typename = eastl::enable_if_t<eastl::is_trivially_copyable_v<MemberType>>>
  inline MemberType get_or(const rapidjson::Value &json, const char* key, MemberType fallback)
  {
    const rapidjson::Value* ptr = get_ptr<MemberType>(json, key);
    return ptr ? ptr->template Get<MemberType>() : fallback;
  }

  template<typename T, typename = eastl::enable_if_t<eastl::is_trivially_copyable_v<T>>>
  inline T value_or(const rapidjson::Value &json, T fallback)
  {
    return json.template Is<T>() ? json.template Get<T>() : fallback;
  }

  template<typename MemberType, typename Json>
  inline eastl::optional<MemberType> get(Json &json, const char *key, eastl::string /*out*/ *err_str = nullptr)
  {
    auto* ptr = get_ptr<MemberType>(json, key, err_str);
    if (ptr)
      return ptr->template Get<MemberType>();
    return eastl::nullopt;
  }

  template<bool Pretty = false>
  inline rapidjson::StringBuffer stringify(const rapidjson::Value &json)
  {
    using PrettyWriter = rapidjson::PrettyWriter<rapidjson::StringBuffer>;
    using RegularWriter = rapidjson::Writer<rapidjson::StringBuffer>;
    using Writer = eastl::conditional_t<Pretty, PrettyWriter, RegularWriter>;

    rapidjson::StringBuffer buffer;
    Writer writer{buffer};
    json.Accept(writer);
    return buffer;
  }

  template<typename MemberType>
  inline void set(
    rapidjson::Document &doc,
    rapidjson::Value::StringRefType key,
    MemberType &&value,
    eastl::string /*out*/ *err_str = nullptr)
  {
    if (!doc.IsObject())
    {
      if (err_str)
        err_str->sprintf("json value is not object (type=%#x)", doc.GetType());
      return;
    }

    rapidjson::Document::MemberIterator it = doc.FindMember(key);

    if (it != doc.MemberEnd())
      it->value = eastl::move(value);
    else
      doc.AddMember(key, rapidjson::Value(eastl::move(value)), doc.GetAllocator());
  }

  void set_copy(
    rapidjson::Document &doc,
    rapidjson::Value::StringRefType key,
    const rapidjson::Value &value,
    eastl::string /*out*/ *err_str = nullptr);

  bool is_member_null(const rapidjson::Document &doc, const char *key);
}
