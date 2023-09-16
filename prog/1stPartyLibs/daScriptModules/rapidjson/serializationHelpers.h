#ifndef _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_SERIALIZATIONHELPERS_H_
#define _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_SERIALIZATIONHELPERS_H_
#pragma once

#include "jsonWriter.h"

namespace das
{

template <typename TJsonWriter>
class JsonWriteSerializationHelper
{
public:
  JsonWriteSerializationHelper(das::Context & context, TJsonWriter & writer_):
      writer(writer_)
  {
  }

  JsonWriteSerializationHelper(const JsonWriteSerializationHelper & other, const char * fieldName):
      writer(other.writer)
  {
    writer.key(fieldName);
  }

  JsonWriteSerializationHelper(const JsonWriteSerializationHelper & other, const das::ArrayElementTag &):
      writer(other.writer)
  {
  }

  das::pair<intptr_t, bool> readElementPtr()
  {
    // dummy
    return das::pair<intptr_t, bool>();
  }

  das::pair<intptr_t, bool> readPtr(const char *)
  {
    // dummy
    return das::pair<intptr_t, bool>();
  }

  uint32_t getArraySize()
  {
    // dummy
    return 0;
  }

  const char * readTableKey()
  {
    // dummy
    return "";
  }

  void writePtr(const char * fieldName, intptr_t ptr)
  {
    writer.key(fieldName);
    writer.appendPtr(ptr);
  }

  void writePtr(intptr_t val)
  {
    writePtr("$p", val);
  }

  void appendPtr(intptr_t val)
  {
    writer.appendPtr(val);
  }

  template <typename V>
  void handleField(const char * fieldName, V & value)
  {
    writer.key(fieldName);
    writer.append(value);
  }

  template <typename V>
  void handleSimpleType(V & value)
  {
    writer.append(value);
  }

  template <typename V>
  void handleArrayElement(V & value)
  {
    writer.append(value);
  }

  void handleStructureStart()
  {
    writer.startObj();
  }

  void handleStructureEnd()
  {
    writer.endObj();
  }

  void handleArrayStart()
  {
    writer.startArray();
  }

  void handleArrayEnd()
  {
    writer.endArray();
  }

private:
  TJsonWriter & writer;
};


} // namespace das


#endif // _GAIJIN_DASCRIPT_MODULES_RAPIDJSON_SERIALIZATIONHELPERS_H_
