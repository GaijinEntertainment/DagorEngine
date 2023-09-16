#include <boost/test/unit_test.hpp>
#include <core/base/wraps.h>

#include <climits>

#include <protoConfig.h>

#include <protolib/serialization/inputCodeStream.h>
#include <protolib/serialization/outputCodeStream.h>

#include <unordered_set>


BOOST_AUTO_TEST_SUITE(proto_io)

BOOST_AUTO_TEST_CASE(test_buffer)
{
  char * nullPtr = NULL;

  proto::io::Buffer buffer;
  BOOST_CHECK_EQUAL(buffer.begin(), nullPtr);
  BOOST_CHECK_EQUAL(buffer.end(), nullPtr);
  BOOST_CHECK_EQUAL(buffer.size(), 0);

  buffer.resize(1000);

  BOOST_CHECK_NE(buffer.begin(), nullPtr);
  BOOST_CHECK_NE(buffer.end(), nullPtr);
  BOOST_CHECK_EQUAL(buffer.end(), buffer.begin() + 1000);
  BOOST_CHECK_EQUAL(buffer.size(), 1000);

  const char * begin = buffer.begin();

  proto::io::Buffer moveBuffer(buffer.move());

  BOOST_CHECK_EQUAL(buffer.begin(), nullPtr);
  BOOST_CHECK_EQUAL(buffer.end(), nullPtr);
  BOOST_CHECK_EQUAL(buffer.size(), 0);

  BOOST_CHECK_EQUAL(moveBuffer.begin(), begin);
  BOOST_CHECK_EQUAL(moveBuffer.end(), begin + 1000);
  BOOST_CHECK_EQUAL(moveBuffer.size(), 1000);
}

BOOST_AUTO_TEST_CASE(test_var_int)
{
  proto::io::OutputCodeStream ostream;

  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 0);
  long long int intValue;
  ostream.writeVarInt(6);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 1);
  ostream.writeVarInt(444);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 3);
  ostream.writeVarInt(-5);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 13);
  ostream.writeVarInt(32767);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 16);
  ostream.writeVarInt(LLONG_MAX);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 25);
  ostream.writeVarInt(LLONG_MIN);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 35);
  ostream.writeVarInt(INT_MAX);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 40);

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());

  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 0);
  BOOST_CHECK(istream.readVarInt(intValue));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 1);
  BOOST_CHECK_EQUAL(intValue, 6);

  BOOST_CHECK(istream.readVarInt(intValue));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 3);
  BOOST_CHECK_EQUAL(intValue, 444);

  BOOST_CHECK(istream.readVarInt(intValue));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 13);
  BOOST_CHECK_EQUAL(intValue, -5);

  BOOST_CHECK(istream.readVarInt(intValue));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 16);
  BOOST_CHECK_EQUAL(intValue, 32767);

  BOOST_CHECK(istream.readVarInt(intValue));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 25);
  BOOST_CHECK_EQUAL(intValue, LLONG_MAX);

  BOOST_CHECK(istream.readVarInt(intValue));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 35);
  BOOST_CHECK_EQUAL(intValue, LLONG_MIN);

  BOOST_CHECK(istream.readVarInt(intValue));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 40);
  BOOST_CHECK_EQUAL(intValue, INT_MAX);

  //TODO many tests
}


BOOST_AUTO_TEST_CASE(test_tag)
{
  proto::io::OutputCodeStream ostream;

  ostream.writeTag(proto::io::StreamTag::BLOCK_BEGIN, 1);
  ostream.writeTag(proto::io::StreamTag::VAR_INT_MINUS, 2000000);
  ostream.writeTag(proto::io::StreamTag::VAR_INT, proto::io::StreamTag::SHORT_NUMBER_LIMIT - 1);
  ostream.writeTag(proto::io::StreamTag::SPECIAL, proto::io::StreamTag::SHORT_NUMBER_LIMIT);
  ostream.writeTag(proto::io::StreamTag::EMPTY, proto::io::StreamTag::SHORT_NUMBER_LIMIT + 1);


  proto::io::StreamTag tag;
  int number = -1;

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::BLOCK_BEGIN);
  BOOST_CHECK_EQUAL(tag.number, 1);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::VAR_INT_MINUS);
  BOOST_CHECK_EQUAL(tag.number, 2000000);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::VAR_INT);
  BOOST_CHECK_EQUAL(tag.number, proto::io::StreamTag::SHORT_NUMBER_LIMIT - 1);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::SPECIAL);
  BOOST_CHECK_EQUAL(tag.number, proto::io::StreamTag::SHORT_NUMBER_LIMIT);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::EMPTY);
  BOOST_CHECK_EQUAL(tag.number, proto::io::StreamTag::SHORT_NUMBER_LIMIT + 1);
}


void testSkip(proto::io::OutputCodeStream & ostream)
{
  ostream.blockEnd();

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());

  proto::io::StreamTag tag;
  tag.type = proto::io::StreamTag::BLOCK_BEGIN;
  tag.number = 1;
  BOOST_CHECK(istream.skip(tag));
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), istream.getBinaryStream().tell());
}


BOOST_AUTO_TEST_CASE(test_var_int_with_tag)
{
  proto::io::OutputCodeStream ostream;

  const int hiLimitOfVar3Byte =(1 << 21);

  long long int intValue;

  ostream.writeVarInt(1, 0);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 1);
  ostream.writeVarInt(1, 1);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 3);
  ostream.writeVarInt(1, -1);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 5);
  ostream.writeVarInt(1, INT_MAX);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 10);
  ostream.writeVarInt(1, INT_MIN);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 15);
  ostream.writeVarInt(1, hiLimitOfVar3Byte);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 20);
  ostream.writeVarInt(1, -hiLimitOfVar3Byte);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 25);
  ostream.writeVarInt(1, hiLimitOfVar3Byte - 1);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 29);

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());

  proto::io::StreamTag tag;

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::EMPTY);
  BOOST_CHECK(istream.readVarInt(tag, intValue));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 1);
  BOOST_CHECK_EQUAL(intValue, 0);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::VAR_INT);
  BOOST_CHECK(istream.readVarInt(tag, intValue));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 3);
  BOOST_CHECK_EQUAL(intValue, 1);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::VAR_INT_MINUS);
  BOOST_CHECK(istream.readVarInt(tag, intValue));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 5);
  BOOST_CHECK_EQUAL(intValue, -1);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::RAW32);
  BOOST_CHECK(istream.readVarInt(tag, intValue));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 10);
  BOOST_CHECK_EQUAL(intValue, INT_MAX);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::RAW32);
  BOOST_CHECK(istream.readVarInt(tag, intValue));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 15);
  BOOST_CHECK_EQUAL(intValue, INT_MIN);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::RAW32);
  BOOST_CHECK(istream.readVarInt(tag, intValue));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 20);
  BOOST_CHECK_EQUAL(intValue, hiLimitOfVar3Byte);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::RAW32);
  BOOST_CHECK(istream.readVarInt(tag, intValue));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 25);
  BOOST_CHECK_EQUAL(intValue, -hiLimitOfVar3Byte);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::VAR_INT);
  BOOST_CHECK(istream.readVarInt(tag, intValue));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 29);
  BOOST_CHECK_EQUAL(intValue, hiLimitOfVar3Byte - 1);
}


BOOST_AUTO_TEST_CASE(test_bool)
{
  proto::io::OutputCodeStream ostream;

  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 0);
  ostream.writeBool(7, true);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 1);
  ostream.writeBool(5, false);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 2);

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());

  proto::io::StreamTag tag;
  bool value = false;

  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 0);
  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 1);
  BOOST_CHECK_EQUAL(tag.number, 7);
  BOOST_CHECK(istream.readBool(tag, value));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 1);
  BOOST_CHECK_EQUAL(value, true);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 2);
  BOOST_CHECK_EQUAL(tag.number, 5);
  BOOST_CHECK(istream.readBool(tag, value));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 2);
  BOOST_CHECK_EQUAL(value, false);

  testSkip(ostream);
}


BOOST_AUTO_TEST_CASE(test_float)
{
  proto::io::OutputCodeStream ostream;

  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 0);
  ostream.writeFloat(45, 1.7f);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 6);
  ostream.writeDouble(22, 5.7);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 16);

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());

  proto::io::StreamTag tag;

  float floatValue = 0;
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 0);
  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 2);
  BOOST_CHECK_EQUAL(tag.number, 45);
  BOOST_CHECK(istream.readFloat(tag, floatValue));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 6);
  BOOST_CHECK_EQUAL(floatValue, 1.7f);

  double doubleValue = 0;
  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 7);
  BOOST_CHECK_EQUAL(tag.number, 22);
  BOOST_CHECK(istream.readDouble(tag, doubleValue));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 16);
  BOOST_CHECK_EQUAL(doubleValue, 5.7);

  testSkip(ostream);
}


BOOST_AUTO_TEST_CASE(test_string)
{
  proto::io::OutputCodeStream ostream;

  std::string str1("simple string");
  std::string str2("hello world");

  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 0);
  ostream.writeString(15, "", 0);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 1);
  ostream.writeString(26347, str1.c_str(), str1.size());
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 19 );
  ostream.writeString(43958729, str2.c_str(), str2.size());
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 36 );
  ostream.writeString(2, str1.c_str(), str1.size());
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 38 );

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());

  proto::io::StreamTag tag;
  std::string value;;

  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 0);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::EMPTY);
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 1);
  BOOST_CHECK_EQUAL(tag.number, 15);
  BOOST_CHECK(readString(istream, tag, value));
  BOOST_CHECK_EQUAL("", value);

  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 1);


  BOOST_CHECK(istream.readTag(tag));

  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 5);
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::VAR_LENGTH);
  BOOST_CHECK_EQUAL(tag.number, 26347);
  BOOST_CHECK(readString(istream, tag, value));
  BOOST_CHECK_EQUAL(str1, value);

  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 19);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 24);
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::VAR_LENGTH);
  BOOST_CHECK_EQUAL(tag.number, 43958729);
  BOOST_CHECK(readString(istream, tag, value));
  BOOST_CHECK_EQUAL(str2, value);
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 36);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::VAR_INT);
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 37);
  BOOST_CHECK_EQUAL(tag.number, 2);
  BOOST_CHECK(readString(istream, tag, value));
  BOOST_CHECK_EQUAL(str1, value);
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 38);

  testSkip(ostream);
}


enum TestEnum
{
  TEZERO = 0,
  TEV1 = 55,
  TEV2 = 5916372,
  TEMINUS = -2384,
};


bool validate_enum_value(TestEnum value)
{
  switch (value)
  {
  case TEZERO:
  case TEV1:
  case TEV2:
  case TEMINUS:
    return true;
  }

  return false;
}


BOOST_AUTO_TEST_CASE(test_enum)
{
  proto::io::OutputCodeStream ostream;

  ostream.writeEnum(1, TEV1);
  ostream.writeEnum(2, TEZERO);
  ostream.writeEnum(3, TEV2);
  ostream.writeEnum(4, TEMINUS);

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());

  proto::io::StreamTag tag;
  int number = -1;
  TestEnum value;

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::VAR_INT);
  BOOST_CHECK_EQUAL(tag.number, 1);
  BOOST_CHECK(istream.readEnum(tag, value));
  BOOST_CHECK_EQUAL(value, TEV1);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::EMPTY);
  BOOST_CHECK_EQUAL(tag.number, 2);
  BOOST_CHECK(istream.readEnum(tag, value));
  BOOST_CHECK_EQUAL(value, TEZERO);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::RAW32);
  BOOST_CHECK_EQUAL(tag.number, 3);
  BOOST_CHECK(istream.readEnum(tag, value));
  BOOST_CHECK_EQUAL(value, TEV2);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::VAR_INT_MINUS);
  BOOST_CHECK_EQUAL(tag.number, 4);
  BOOST_CHECK(istream.readEnum(tag, value));
  BOOST_CHECK_EQUAL(value, TEMINUS);

  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), ostream.getBinaryStream().tell());

  testSkip(ostream);
}


BOOST_AUTO_TEST_CASE(test_block)
{
  proto::io::OutputCodeStream ostream;

  ostream.blockBegin(150);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 2);
  ostream.writeVarInt(5, 13);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 4);
  ostream.blockEnd();
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().tell(), 5);

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());

  proto::io::StreamTag tag;
  int number = -1;
  TestEnum value;
  proto::uint32 length = 0;

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 2);
  BOOST_CHECK_EQUAL(tag.number, 150);
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::BLOCK_BEGIN);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 3);
  BOOST_CHECK_EQUAL(tag.number, 5);
  BOOST_CHECK(istream.readVarInt(tag, length));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 4);
  BOOST_CHECK_EQUAL(length, 13);

  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(istream.getBinaryStream().tell(), 5);
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::SPECIAL);
  BOOST_CHECK_EQUAL(tag.number, proto::io::StreamTag::SPECIAL_BLOCK_END);

  testSkip(ostream);
}


BOOST_AUTO_TEST_CASE(test_strings_in_big_block)
{
  std::vector<std::string> testStrings = {"dsafdf", "dfu8723yrhu", "hello world", "beer"};
  proto::io::OutputCodeStream ostream;


  int counter = 1;
  ostream.blockBegin(150);
  srand(666);
  while (ostream.getBinaryStream().tell() < 4 * 1024 * 1024)
  {
    const std::string & selectString = testStrings.at(rand() % testStrings.size());
    ostream.writeString(counter++, selectString.c_str(), selectString.size());
  }

  testStrings.emplace_back("732he92hyr731hxd");
  testStrings.emplace_back("j2-3498fy7whgfiuasfd");
  testStrings.emplace_back("j2-dsdf");
  testStrings.emplace_back("j2-dsefq4rdf");
  testStrings.emplace_back("j2-sadfsadfqw4trq432t");

  for (int i = 0; i < 1000; ++i)
  {
    const std::string & selectString = testStrings.at(rand() % testStrings.size());
    ostream.writeString(counter++, selectString.c_str(), selectString.size());
  }

  ostream.blockEnd();

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());

  counter = 1;
  proto::io::StreamTag tag;
  int number = -1;

  std::unordered_set<std::string> testStringsSet(testStrings.begin(), testStrings.end());
  std::string result;
  BOOST_REQUIRE(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.number, 150);
  BOOST_CHECK_EQUAL(tag.type, proto::io::StreamTag::BLOCK_BEGIN);

  BOOST_CHECK(istream.readTag(tag));
  while (!tag.isBlockEnded())
  {
    BOOST_CHECK_EQUAL(counter, tag.number);
    counter++;
    if (!proto::io::readString(istream, tag, result))
    {
      BOOST_TEST_MESSAGE(istream.errorMessage());
      BOOST_REQUIRE(false);
    }

    if (!testStringsSet.count(result))
    {
      BOOST_TEST_MESSAGE(result);
      BOOST_REQUIRE(false);
    }

    BOOST_REQUIRE(istream.readTag(tag));
  }

  testSkip(ostream);
}

BOOST_AUTO_TEST_SUITE_END()


