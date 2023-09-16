#include <boost/test/unit_test.hpp>
#include <core/base/wraps.h>

#include <climits>

#include "generated/testMapSerialization.pb.h"
#include "generated/testMapSerialization.pb.cpp"

#include <boost/lexical_cast.hpp>

BOOST_AUTO_TEST_SUITE(proto_io_map)

const int ITEM_COUNT = 20;

static TestMessage prepareMessage()
{
  TestMessage message;
  for (int i = 0; i < ITEM_COUNT; ++i)
  {
    std::string strNum = boost::lexical_cast<std::string>(i);
    TestItem item;
    item.setName("name" + strNum);
    message.getItems().insert(proto::make_pair("item" + strNum, item));
  }

  return message;
}


BOOST_AUTO_TEST_CASE(test_empty)
{
  TestMessage message;
  message.touchRecursive(2);
  proto::io::OutputCodeStream ostream(1);
  message.saveToDB(ostream, proto::BinarySaveOptions(266));

  message = prepareMessage();

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());
  proto::io::StreamTag tag;
  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.number, 266);
  BOOST_CHECK(message.loadFromDB(istream, tag));
  BOOST_CHECK_EQUAL(message.getItems().size(), 0);
}


BOOST_AUTO_TEST_CASE(test_save)
{
  TestMessage message = prepareMessage();
  TStringToTestItemUnorderedMap & map = message.getItems();
  map.touch(3);
  map["item1"].touch(1);
  map["item2"].touch(2);
  map["item5"].touch(3);
  map["item7"].touch(100);

  proto::io::OutputCodeStream ostream(1);
  message.saveToDB(ostream, proto::BinarySaveOptions(266));

  message.clear();
  map["testf1"].setName("f1");
  map["item2"].setName("old2");

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());
  proto::io::StreamTag tag;
  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.number, 266);
  BOOST_CHECK(message.loadFromDB(istream, tag));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), ostream.getBinaryStream().tell());
  BOOST_CHECK_EQUAL(map.size(), 4);
  BOOST_CHECK_EQUAL(map["testf1"].getName(), "f1");

  BOOST_CHECK_EQUAL(map["item2"].getName(), "name2");
  BOOST_CHECK_EQUAL(map["item5"].getName(), "name5");
  BOOST_CHECK_EQUAL(map["item7"].getName(), "name7");
}


BOOST_AUTO_TEST_CASE(test_delete)
{
  TestMessage message = prepareMessage();
  TStringToTestItemUnorderedMap & map = message.getItems();
  map.touch(1);
  map.touchStructure(2);
  map["item2"].setName("testSaveAndDelete");
  map["item2"].touch(2);
  map.erase("item15");
  map.erase("item5");

  proto::io::OutputCodeStream ostream(1);
  message.saveToDB(ostream, proto::BinarySaveOptions(266));

  message = prepareMessage();

  map["test_delete"].setName("ddd");
  map["item4"].setName("must_be_unchanged");

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());
  proto::io::StreamTag tag;
  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.number, 266);
  BOOST_CHECK(message.loadFromDB(istream, tag));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), ostream.getBinaryStream().tell());
  BOOST_CHECK_EQUAL(map.size(), 18);
  BOOST_CHECK(map.find("item5") == map.end());
  BOOST_CHECK(map.find("item15") == map.end());
  BOOST_CHECK(map.find("test_delete") == map.end());
  BOOST_CHECK_EQUAL(map["item2"].getName(), "testSaveAndDelete");
  BOOST_CHECK_EQUAL(map["item4"].getName(), "must_be_unchanged");
}


BOOST_AUTO_TEST_SUITE_END()
