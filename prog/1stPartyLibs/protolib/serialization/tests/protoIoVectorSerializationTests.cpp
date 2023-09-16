#include <boost/test/unit_test.hpp>
#include <core/base/wraps.h>

#include <protoConfig.h>

#include <protolib/serialization/vectorSerialization.h>
#include <protolib/serialization/vectorSerializationBlk.h>

#include <boost/lexical_cast.hpp>

#include "generated/testVectorSerialization.pb.cpp"

namespace proto_vector_test
{
  bool operator!=(
    const proto_vector_test::TestItem & o1,
    const proto_vector_test::TestItem & o2)
  {
    return o1.getName() != o2.getName();
  }

  std::ostream & operator<<(std::ostream & stream, const proto_vector_test::TestItem & obj)
  {
    return stream << obj.getName();
  }
}

using namespace proto_vector_test;


BOOST_AUTO_TEST_SUITE(proto_io_vector)

typedef proto::Vector<int>::TType TTestVector;
typedef proto::Vector<proto::string>::TType TStringVector;
BOOST_AUTO_TEST_CASE(int_empty)
{
  TTestVector srcVector;

  proto::io::OutputCodeStream ostream(1);

  proto::io::writeVector(ostream, 666, srcVector);

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());
  proto::io::StreamTag tag;
  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.number, 666);

  TTestVector dstVector;
  dstVector.push_back(444);
  BOOST_CHECK(proto::io::readVector(istream, tag, dstVector));
  BOOST_CHECK(dstVector.empty());
}


BOOST_AUTO_TEST_CASE(int_values)
{
  TTestVector srcVector;
  srcVector.push_back(5);
  srcVector.push_back(77);
  srcVector.push_back(984765);
  srcVector.push_back(-2);

  proto::io::OutputCodeStream ostream(1);

  proto::io::writeVector(ostream, 666, srcVector);

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());
  proto::io::StreamTag tag;
  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.number, 666);

  TTestVector dstVector;
  dstVector.push_back(444);
  BOOST_CHECK(proto::io::readVector(istream, tag, dstVector));
  BOOST_CHECK_EQUAL_COLLECTIONS(
      dstVector.begin(), dstVector.end(),
      srcVector.begin(), srcVector.end());
}

BOOST_AUTO_TEST_CASE(blk_int_empty)
{
  TTestVector srcVector;

  DataBlock blk;
  proto::BlkSerializator ser(blk, -2);
  proto::io::writeVector(ser, "test", srcVector);

  TTestVector dstVector;
  dstVector.push_back(444);
  proto::io::readVector(ser, "test", dstVector);
  BOOST_CHECK(dstVector.empty());
}

BOOST_AUTO_TEST_CASE(blk_int_values)
{
  TTestVector srcVector;
  srcVector.push_back(5);
  srcVector.push_back(77);
  srcVector.push_back(984765);
  srcVector.push_back(-2);

  DataBlock blk;
  proto::BlkSerializator ser(blk, -2);
  proto::io::writeVector(ser, "test", srcVector);

  TTestVector dstVector;
  dstVector.push_back(444);
  proto::io::readVector(ser, "test", dstVector);
  BOOST_CHECK_EQUAL_COLLECTIONS(
    dstVector.begin(), dstVector.end(),
    srcVector.begin(), srcVector.end());
}


BOOST_AUTO_TEST_CASE(blk_string_values)
{
  TStringVector srcVector;
  srcVector.push_back("5");
  srcVector.push_back("77");
  srcVector.push_back("");
  srcVector.push_back("984765");
  srcVector.push_back("-2");

  DataBlock blk;
  proto::BlkSerializator ser(blk, -2);
  proto::io::writeVector(ser, "test", srcVector);

  TStringVector dstVector;
  dstVector.push_back("444");
  proto::io::readVector(ser, "test", dstVector);
  BOOST_CHECK_EQUAL_COLLECTIONS(
    dstVector.begin(), dstVector.end(),
    srcVector.begin(), srcVector.end());
}

const int ITEM_COUNT = 20;

static TestMessage prepareMessage()
{
  TestMessage message;
  for (int i = 0; i < ITEM_COUNT; ++i)
  {
    std::string strNum = boost::lexical_cast<std::string>(i);
    TestItem item;
    item.setName("name" + strNum);
    message.getItems().push_back(item);
  }

  return message;
}


BOOST_AUTO_TEST_CASE(test_vector_versioned_all)
{
  TestMessage message = prepareMessage();
  message.touchRecursive(2);

  proto::io::OutputCodeStream ostream(1);
  message.saveToDB(ostream, proto::BinarySaveOptions(266));

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());
  proto::io::StreamTag tag;
  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.number, 266);

  TestMessage dstMessage;
  BOOST_CHECK(dstMessage.loadFromDB(istream, tag));

  BOOST_CHECK_EQUAL_COLLECTIONS(
      message.getItems().begin(), message.getItems().end(),
      dstMessage.getItems().begin(), dstMessage.getItems().end());
}


BOOST_AUTO_TEST_CASE(test_vector_versioned_delete)
{
  TestMessage message = prepareMessage();
  message.touchRecursive(2);
  TTestItemVector & items = message.getItems();
  message.getItems().erase(items.begin() + 5);
  for (TTestItemVector::iterator it = items.begin() + 5; it != items.end(); ++it)
  {
    (*it).touch(3);
  }

  message.getItems().touchStructure(3);

  proto::io::OutputCodeStream ostream(2);
  message.saveToDB(ostream, proto::BinarySaveOptions(266));
  BOOST_CHECK_LT(ostream.getBinaryStream().tell(), 150);

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());
  proto::io::StreamTag tag;
  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.number, 266);

  TestMessage dstMessage = prepareMessage();
  BOOST_CHECK(dstMessage.loadFromDB(istream, tag));

  BOOST_CHECK_EQUAL_COLLECTIONS(
    message.getItems().begin(), message.getItems().end(),
    dstMessage.getItems().begin(), dstMessage.getItems().end());
}

BOOST_AUTO_TEST_SUITE_END()
