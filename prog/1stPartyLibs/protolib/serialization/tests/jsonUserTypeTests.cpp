#include <boost/test/unit_test.hpp>
#include <core/base/wraps.h>

#include <protoConfig.h>
#include <protolib/serialization/userTypes/jsonValueImpl.h>

#include <json/reader.h>
#include <json/writer.h>


BOOST_AUTO_TEST_SUITE(proto_json_user_type)


const int MAGIC_TAG = 8942;


void check_input_stream(proto::io::InputCodeStream & stream,
                        const std::initializer_list<int> & numbers)
{
  proto::io::StreamTag tag;
  BOOST_CHECK(stream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.number, MAGIC_TAG);

  const_cast<proto::io::InputBinaryStream &>(stream.getBinaryStream()).seekBegin(0);
  for (int n : numbers)
  {
    BOOST_CHECK(stream.readTag(tag));
    BOOST_CHECK_EQUAL(tag.number, n);
    BOOST_CHECK(stream.skip(tag));
  }

  BOOST_CHECK(stream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.number, MAGIC_TAG);
}


BOOST_AUTO_TEST_CASE(test_null)
{
  proto::io::OutputCodeStream ostream;
  Json::Value v = Json::nullValue;
  BOOST_CHECK_EQUAL(v.type(), Json::nullValue);
  proto::serialize(ostream, 1, v);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().getBuffer().size(), 1);
  ostream.writeTag(proto::io::StreamTag::EMPTY, MAGIC_TAG);

  v = Json::objectValue;
  BOOST_CHECK_EQUAL(v.type(), Json::objectValue);

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());
  proto::io::StreamTag tag;
  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.number, 1);
  BOOST_CHECK(proto::serialize(istream, tag, v));
  BOOST_CHECK_EQUAL(v.type(), Json::nullValue);

  check_input_stream(istream, {1});
}


BOOST_AUTO_TEST_CASE(test_bare_value)
{
  proto::io::OutputCodeStream ostream;
  const int TESTINT = -123123;
  const std::string TESTSTRING = "test string";
  Json::Value v = TESTINT;
  BOOST_CHECK_EQUAL(v.type(), Json::intValue);
  proto::serialize(ostream, 1, v);
  v = TESTSTRING;
  BOOST_CHECK_EQUAL(v.type(), Json::stringValue);
  proto::serialize(ostream, 2, v);
  ostream.writeTag(proto::io::StreamTag::EMPTY, MAGIC_TAG);

  v = Json::objectValue;
  BOOST_CHECK_EQUAL(v.type(), Json::objectValue);

  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());
  proto::io::StreamTag tag;
  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.number, 1);
  BOOST_CHECK(proto::serialize(istream, tag, v));
  BOOST_CHECK_EQUAL(v.type(), Json::intValue);
  BOOST_CHECK_EQUAL(v.asInt(), TESTINT);
  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.number, 2);
  BOOST_CHECK(proto::serialize(istream, tag, v));
  BOOST_CHECK_EQUAL(v.type(), Json::stringValue);
  BOOST_CHECK_EQUAL(v.asString(), TESTSTRING);

  check_input_stream(istream, {1, 2});
}


BOOST_AUTO_TEST_CASE(test_empty_block)
{
  proto::io::OutputCodeStream ostream;
  Json::Value v = Json::objectValue;
  BOOST_CHECK_EQUAL(v.type(), Json::objectValue);
  proto::serialize(ostream, 1, v);
  BOOST_CHECK_EQUAL(ostream.getBinaryStream().getBuffer().size(), 1);
  ostream.writeTag(proto::io::StreamTag::EMPTY, MAGIC_TAG);

  v = Json::nullValue;
  BOOST_CHECK_EQUAL(v.type(), Json::nullValue);
  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());
  proto::io::StreamTag tag;
  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.number, 1);
  BOOST_CHECK(proto::serialize(istream, tag, v));

  BOOST_CHECK_EQUAL(v.type(), Json::objectValue);
  BOOST_CHECK_EQUAL(v.size(), 0);

  check_input_stream(istream, {1});
}


BOOST_AUTO_TEST_CASE(test_empty_array)
{
  proto::io::OutputCodeStream ostream;
  Json::Value v = Json::arrayValue;
  BOOST_CHECK_EQUAL(v.type(), Json::arrayValue);
  proto::serialize(ostream, 1, v);
  ostream.writeTag(proto::io::StreamTag::EMPTY, MAGIC_TAG);

  v = Json::nullValue;
  BOOST_CHECK_EQUAL(v.type(), Json::nullValue);
  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());
  proto::io::StreamTag tag;
  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.number, 1);
  BOOST_CHECK(proto::serialize(istream, tag, v));

  BOOST_CHECK_EQUAL(v.type(), Json::arrayValue);
  BOOST_CHECK_EQUAL(v.size(), 0);

  check_input_stream(istream, {1});
}


BOOST_AUTO_TEST_CASE(test_param_types)
{
  const double TESTDOUBLE = 123.4567890123;
  Json::Value v;
  v["int"] = -123123;
  v["int64"] = (Json::Int64)0x123456789ABCDEF0;
  v["uint"] = (unsigned)456456;
  v["double"] = TESTDOUBLE;
  v["bool"] = true;
  v["str"] = "test";
  v["null"] = Json::nullValue;
  v["array"] = Json::arrayValue;
  v["obj"] = Json::objectValue;

  proto::io::OutputCodeStream ostream;
  proto::serialize(ostream, 1, v);
  ostream.writeTag(proto::io::StreamTag::EMPTY, MAGIC_TAG);

  Json::Value v2;
  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());
  proto::io::StreamTag tag;
  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.number, 1);
  BOOST_CHECK(proto::serialize(istream, tag, v2));

  BOOST_CHECK_EQUAL(v2["int"].type(), Json::intValue);
  BOOST_CHECK_EQUAL(v2["int"].asInt(), -123123);

  BOOST_CHECK_EQUAL(v2["int64"].type(), Json::intValue);
  BOOST_CHECK_EQUAL(v2["int64"].asInt64(), 0x123456789ABCDEF0);

  BOOST_CHECK_EQUAL(v2["uint"].type(), Json::uintValue);
  BOOST_CHECK_EQUAL(v2["uint"].asUInt(), 456456);

  BOOST_CHECK_EQUAL(v2["double"].type(), Json::realValue);
  BOOST_CHECK_EQUAL(v2["double"].asDouble(), TESTDOUBLE); // should be binary identical

  BOOST_CHECK_EQUAL(v2["bool"].type(), Json::booleanValue);
  BOOST_CHECK_EQUAL(v2["bool"].asBool(), true);

  BOOST_CHECK_EQUAL(v2["null"].type(), Json::nullValue);
  BOOST_CHECK_EQUAL(v2["array"].type(), Json::arrayValue);
  BOOST_CHECK_EQUAL(v2["obj"].type(), Json::objectValue);

  check_input_stream(istream, {1});
}


BOOST_AUTO_TEST_CASE(test_complex)
{
  // taken from https://www.sitepoint.com/colors-json-example/
  const char * const srcJson = R"JSON(
{
  "colors": [
    {
      "color": "black",
      "category": "hue",
      "type": "primary",
      "code":
      {
        "rgba": [255,255,255,1],
        "hex": "#000"
      }
    },
    {
      "color": "white",
      "category": "value",
      "code":
      {
        "rgba": [0,0,0,1],
        "hex": "#FFF"
      }
    },
    {
      "color": "red",
      "category": "hue",
      "type": "primary",
      "code":
      {
        "rgba": [255,0,0,1],
        "hex": "#FF0"
      }
    },
    {
      "color": "blue",
      "category": "hue",
      "type": "primary",
      "code":
      {
        "rgba": [0,0,255,1],
        "hex": "#00F"
      }
    },
    {
      "color": "yellow",
      "category": "hue",
      "type": "primary",
      "code":
      {
        "rgba": [255,255,0,1],
        "hex": "#FF0"
      }
    },
    {
      "color": "green",
      "category": "hue",
      "type": "secondary",
      "code":
      {
        "rgba": [0,255,0,1],
        "hex": "#0F0"
      }
    }
  ],
  "nestedArraysOfDouble": [
    [0.1, 0.2, -0.3, [4.5, 6.7, 10000.0]],
    {
      "yetAnotherArray": [55.8],
      "andAnotherDouble": 123.45
    }
  ]
}
  )JSON";

  Json::Value v1;
  Json::Reader().parse(srcJson, v1);
  // make sure it reads properly
  BOOST_CHECK_EQUAL(v1.type(), Json::objectValue);
  BOOST_CHECK_EQUAL(v1["colors"].type(), Json::arrayValue);

  proto::io::OutputCodeStream ostream;
  proto::serialize(ostream, 1, v1);
  ostream.writeTag(proto::io::StreamTag::EMPTY, MAGIC_TAG);

  Json::Value v2;
  proto::io::InputCodeStream istream(ostream.getBinaryStream().getBuffer());
  proto::io::StreamTag tag;
  BOOST_CHECK(istream.readTag(tag));
  BOOST_CHECK_EQUAL(tag.number, 1);
  BOOST_CHECK(proto::serialize(istream, tag, v2));

  std::string str1 = Json::FastWriter().write(v1);
  std::string str2 = Json::FastWriter().write(v2);
  BOOST_CHECK_EQUAL(str1, str2);

  check_input_stream(istream, {1});
}


BOOST_AUTO_TEST_SUITE_END()
