// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/io/json.h>
#include <daECS/core/componentTypes.h>
#include <json/value.h>


#define JSON_TO_ECS_LIST                     \
  JSON_TO_ECS(null, Object())                \
  JSON_TO_ECS(int, value.asLargestInt())     \
  JSON_TO_ECS(uint, value.asLargestInt())    \
  JSON_TO_ECS(real, float(value.asDouble())) \
  JSON_TO_ECS(string, value.asCString())     \
  JSON_TO_ECS(boolean, value.asBool())       \
  JSON_TO_ECS(array, json_as<Array>(value))  \
  JSON_TO_ECS(object, json_as<Object>(value))


namespace ecs
{

template <class T>
T json_as(const Json::Value &json);


static void add_json_value(Object &obj, const Json::Value::const_iterator &it)
{
  const char *const key = it.memberName();
  G_ASSERT_RETURN(key != nullptr, );

  const Json::Value &value = *it;
  switch (value.type())
  {
#define JSON_TO_ECS(type, expr) \
  case Json::type##Value: obj.addMember(key, (expr)); return;

    JSON_TO_ECS_LIST

#undef JSON_TO_ECS
  }
}


static void add_json_value(Array &arr, const Json::Value::const_iterator &it)
{
  const Json::Value &value = *it;
  switch (value.type())
  {
#define JSON_TO_ECS(type, expr) \
  case Json::type##Value: arr.push_back(expr); return;

    JSON_TO_ECS_LIST

#undef JSON_TO_ECS
  }
}


template <class T>
T json_as(const Json::Value &json)
{
  T obj;
  const Json::Value::const_iterator end = json.end();
  for (Json::Value::const_iterator it = json.begin(); it != end; ++it)
    add_json_value(obj, it);

  return obj;
}


Object json_to_ecs_object(const Json::Value &json) { return (json.type() == Json::objectValue) ? json_as<Object>(json) : Object(); }


} // namespace ecs
