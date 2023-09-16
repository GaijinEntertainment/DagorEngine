#pragma once

#include <ioSys/dag_dataBlock.h>
#include <protolib/serialization/binaryStream.h>
#include <protolib/serialization/outputCodeStream.h>
#include <protolib/serialization/inputCodeStream.h>
#include <protolib/serialization/json/jsonSerializator.h>
#include <util/dag_baseDef.h>

namespace proto
{
  inline void clear(DataBlock & v)
  {
    v.reset();
  }

  void serialize(proto::io::OutputCodeStream & stream, proto::io::TNumber number, const DataBlock & blk);
  bool serialize(proto::io::InputCodeStream & stream, proto::io::StreamTag tag, DataBlock & blk);

  inline void load(const BlkSerializator & ser, const char * name, DataBlock & value)
  {
    value.setFrom(ser.blk.getBlockByNameEx(name));
  }

  inline void save(const BlkSerializator & ser, const char * name, const DataBlock & value)
  {
    ser.blk.addNewBlock(&value, name);
  }

  void load(const JsonReader & ser, const char * name, DataBlock & value);
  void save(const JsonWriter & ser, const char * name, const DataBlock & value);

  inline void touch_recursive(DataBlock &, version) {}
}
