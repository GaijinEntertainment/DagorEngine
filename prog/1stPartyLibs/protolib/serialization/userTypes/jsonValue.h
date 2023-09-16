#pragma once


#include <json/value.h>
#include <protolib/serialization/binaryStream.h>
#include <protolib/serialization/outputCodeStream.h>
#include <protolib/serialization/inputCodeStream.h>
#include <protolib/serialization/json/jsonSerializator.h>

namespace proto
{
  inline void clear(Json::Value & value)
  {
    value = Json::Value();
  }

  void serialize(proto::io::OutputCodeStream & stream, proto::io::TNumber number, const Json::Value & value);
  bool serialize(proto::io::InputCodeStream & stream, proto::io::StreamTag tag, Json::Value & value);

  void load(const BlkSerializator & ser, const char * name, Json::Value & value);
  void save(const BlkSerializator & ser, const char * name, const Json::Value & value);

  inline void load(const JsonReader & ser, const char * name, Json::Value & value)
  {
    value = ser.getJson().get(name, Json::Value());
  }

  inline void save(const JsonWriter & ser, const char * name, const Json::Value & value)
  {
    ser.getJson()[name] = value;
  }

  inline void touch_recursive(Json::Value &, version) {}
}
