#include "testVectorSerialization.pb.h"

#include <protolib/serialization/vectorSerialization.h>
#include <protolib/serialization/setSerialization.h>
#include <protolib/serialization/mapSerialization.h>
#include <protolib/serialization/vectorSerializationBlk.h>
#include <protolib/serialization/setSerializationBlk.h>
#include <protolib/serialization/mapSerializationBlk.h>

namespace proto_vector_test
{

  TestItem::TestItem()
  {
    clear();
  }


  void TestItem::clear()
  {
    name.clear();
  }


  TestItem::~TestItem()
  {
  }


  void TestItem::touchRecursive(proto::version value)
  {
    touch(value);
  }


  void TestItem::saveToDB(
      proto::io::OutputCodeStream & stream,
      const proto::BinarySaveOptions & saveOptions) const
  {
    stream.blockBegin(saveOptions.getNumber());
    stream.writeString(1, proto::str_cstr(name), proto::str_size(name));
    stream.blockEnd();
  }


  bool TestItem::loadFromDB(
      proto::io::InputCodeStream & stream,
      proto::io::StreamTag tag)
  {
    PROTO_VALIDATE(tag.type == proto::io::StreamTag::BLOCK_BEGIN);
    PROTO_VALIDATE(stream.readTag(tag));
    while (true)
    {
      if (tag.isBlockEnded())
        break;

      switch (tag.number)
      {
        case 1:
          PROTO_VALIDATE(proto::io::readString(stream, tag, name));
          PROTO_VALIDATE(stream.readTag(tag));
          break;
        default:
          PROTO_VALIDATE(stream.skip(tag)); // unknown field
          PROTO_VALIDATE(stream.readTag(tag));
      }

    }

    return true;
  }


  TestMessage::TestMessage()
  {
    clear();
  }


  void TestMessage::clear()
  {

    items.clear();
    items.touchStructure(proto::DEFAULT_VERSION);
  }


  TestMessage::~TestMessage()
  {
  }


  void TestMessage::touchRecursive(proto::version value)
  {
    touch(value);
    items.touchRecursive(value);
  }


  void TestMessage::saveToDB(
      proto::io::OutputCodeStream & stream,
      const proto::BinarySaveOptions & saveOptions) const
  {
    stream.blockBegin(saveOptions.getNumber());
    proto::io::writeVectorVersioned(stream, 1, items, &TestItem::saveToDB);
    stream.blockEnd();
  }


  bool TestMessage::loadFromDB(
      proto::io::InputCodeStream & stream,
      proto::io::StreamTag tag)
  {
    PROTO_VALIDATE(tag.type == proto::io::StreamTag::BLOCK_BEGIN);
    PROTO_VALIDATE(stream.readTag(tag));
    while (true)
    {
      if (tag.isBlockEnded())
        break;

      switch (tag.number)
      {
        case 1:
          PROTO_VALIDATE((proto::io::readVectorVersioned<TTestItemVector, TestItem>(
              stream, tag, items, &TestItem::loadFromDB)));
          PROTO_VALIDATE(stream.readTag(tag));
          break;
        default:
          PROTO_VALIDATE(stream.skip(tag)); // unknown field
          PROTO_VALIDATE(stream.readTag(tag));
      }

    }

    return true;
  }

}
