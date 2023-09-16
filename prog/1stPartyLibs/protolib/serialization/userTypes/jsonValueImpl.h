#pragma once


#include <protoConfig.h>
#include "jsonValue.h"
#include <json/writer.h>
#include <json/reader.h>


namespace proto
{

  namespace json_value
  {

    enum JsonValueTags
    {
      JVT_BEGIN_BLOCK = 1,
      JVT_BEGIN_ARRAY,
      JVT_PARAM_NAME,
      JVT_PARAM_TYPE_STRING,
      JVT_PARAM_TYPE_INT,
      JVT_PARAM_TYPE_UINT,
      JVT_PARAM_TYPE_REAL,
      JVT_PARAM_TYPE_BOOL,
      JVT_PARAM_TYPE_NULL,
      JVT_PARAM_VALUE
    };

    void writeNamelessValue(proto::io::OutputCodeStream & stream, const Json::Value & value);

    void writeValue(proto::io::OutputCodeStream & stream, const std::string & name, const Json::Value & value)
    {
      stream.writeString(JVT_PARAM_NAME, name.c_str(), name.size());
      writeNamelessValue(stream, value);
    }

    void writeArray(proto::io::OutputCodeStream & stream, const Json::Value & value)
    {
      for (int i = 0; i < value.size(); i++)
      {
        writeNamelessValue(stream, value[i]);
      }
    }

    void writeBlock(proto::io::OutputCodeStream & stream, const Json::Value & value)
    {
      const Json::Value::Members & memberNames = value.getMemberNames();
      for (const std::string & name : memberNames)
      {
        const Json::Value & subValue = value[name];
        writeValue(stream, name, subValue);
      }
    }

    void writeNamelessValue(proto::io::OutputCodeStream & stream, const Json::Value & value)
    {
      switch (value.type())
      {
      case Json::nullValue:
        stream.writeTag(proto::io::StreamTag::EMPTY, JVT_PARAM_TYPE_NULL);
        break;
      case Json::intValue:
        stream.writeVarInt(JVT_PARAM_TYPE_INT, value.asInt64());
        break;
      case Json::uintValue:
        stream.writeVarInt(JVT_PARAM_TYPE_UINT, value.asUInt64());
        break;
      case Json::realValue:
        stream.writeDouble(JVT_PARAM_TYPE_REAL, value.asDouble());
        break;
      case Json::stringValue:
        {
          const char * str = value.asCString();
          stream.writeString(JVT_PARAM_TYPE_STRING, str, strlen(str));
        }
        break;
      case Json::booleanValue:
        stream.writeBool(JVT_PARAM_TYPE_BOOL, value.asBool());
        break;
      case Json::arrayValue:
        if (value.empty())
        {
          stream.writeTag(proto::io::StreamTag::EMPTY, JVT_BEGIN_ARRAY);
        }
        else
        {
          stream.blockBegin(JVT_BEGIN_ARRAY);
          json_value::writeArray(stream, value);
          stream.blockEnd();
        }
        break;
      case Json::objectValue:
        if (value.empty())
        {
          stream.writeTag(proto::io::StreamTag::EMPTY, JVT_BEGIN_BLOCK);
        }
        else
        {
          stream.blockBegin(JVT_BEGIN_BLOCK);
          json_value::writeBlock(stream, value);
          stream.blockEnd();
        }
        break;
      }
    }


    bool readValue(proto::io::InputCodeStream & stream, const proto::io::StreamTag & tag, Json::Value & value);

    bool readBlock(proto::io::InputCodeStream & stream, Json::Value & value)
    {
      proto::string tmpString;
      proto::io::StreamTag tag;
      PROTO_VALIDATE(stream.readTag(tag));

      while (!tag.isBlockEnded())
      {
        PROTO_VALIDATE(tag.number == JVT_PARAM_NAME);
        PROTO_VALIDATE(proto::io::readString(stream, tag, tmpString));
        Json::Value & subValue = value[tmpString];
        PROTO_VALIDATE(stream.readTag(tag));
        PROTO_VALIDATE(readValue(stream, tag, subValue));
        PROTO_VALIDATE(stream.readTag(tag));
      }

      return true;
    }


    bool readArray(proto::io::InputCodeStream & stream, Json::Value & value)
    {
      proto::io::StreamTag tag;
      PROTO_VALIDATE(stream.readTag(tag));
      int idx = 0;

      while (!tag.isBlockEnded())
      {
        Json::Value & subValue = value[idx++];
        PROTO_VALIDATE(readValue(stream, tag, subValue));
        PROTO_VALIDATE(stream.readTag(tag));
      }

      return true;
    }


    bool readValue(proto::io::InputCodeStream & stream, const proto::io::StreamTag & tag, Json::Value & value)
    {
      PROTO_VALIDATE(!tag.isBlockEnded());
      switch (tag.number)
      {
      case JVT_PARAM_TYPE_NULL:
        value = Json::nullValue;
        break;
      case JVT_PARAM_TYPE_INT:
        {
          Json::Int64 v = 0;
          PROTO_VALIDATE(stream.readVarInt(tag, v));
          value = v;
        }
        break;
      case JVT_PARAM_TYPE_UINT:
        {
          Json::UInt64 v = 0;
          PROTO_VALIDATE(stream.readVarInt(tag, v));
          value = v;
        }
        break;
      case JVT_PARAM_TYPE_REAL:
        {
          double v = 0;
          PROTO_VALIDATE(stream.readDouble(tag, v));
          value = v;
        }
        break;
      case JVT_PARAM_TYPE_STRING:
        {
          proto::string v;
          PROTO_VALIDATE(proto::io::readString(stream, tag, v));
          value = v;
        }
        break;
      case JVT_PARAM_TYPE_BOOL:
        {
          bool v = false;
          PROTO_VALIDATE(stream.readBool(tag, v));
          value = v;
        }
        break;
      case JVT_BEGIN_BLOCK:
        {
          value = Json::objectValue;
          if (tag.type == proto::io::StreamTag::EMPTY)
            break;
          PROTO_VALIDATE(tag.type == proto::io::StreamTag::BLOCK_BEGIN);
          PROTO_VALIDATE(readBlock(stream, value));
        }
        break;
      case JVT_BEGIN_ARRAY:
        {
          value = Json::arrayValue;
          if (tag.type == proto::io::StreamTag::EMPTY)
            break;
          PROTO_VALIDATE(tag.type == proto::io::StreamTag::BLOCK_BEGIN);
          PROTO_VALIDATE(readArray(stream, value));
        }
        break;
      default:
        return false;
      }

      return true;
    }
  } // namespace json_value


  void serialize(proto::io::OutputCodeStream & stream, proto::io::TNumber number, const Json::Value & value)
  {
    if (value.type() == Json::nullValue)
    {
      stream.writeTag(proto::io::StreamTag::EMPTY, number);
      return;
    }

    if (value.type() == Json::objectValue && value.empty())
    {
      stream.writeTag(proto::io::StreamTag::EMPTY2, number);
      return;
    }

    stream.blockBegin(number);
    json_value::writeNamelessValue(stream, value);
    stream.blockEnd();
  }

  bool serialize(proto::io::InputCodeStream & stream, proto::io::StreamTag tag, Json::Value & value)
  {
    if (tag.type == proto::io::StreamTag::EMPTY)
    {
      value = Json::nullValue;
      return true;
    }

    value = Json::objectValue;
    if (tag.type == proto::io::StreamTag::EMPTY2)
    {
      return true;
    }

    PROTO_VALIDATE(tag.type == proto::io::StreamTag::BLOCK_BEGIN);
    PROTO_VALIDATE(stream.readTag(tag));
    PROTO_VALIDATE(json_value::readValue(stream, tag, value));
    PROTO_VALIDATE(stream.readTag(tag));
    PROTO_VALIDATE(tag.isBlockEnded());
    return true;
  }

  void load(const BlkSerializator & ser, const char * name, Json::Value & value)
  {
    value = Json::objectValue;
    const char * str = ser.blk.getStr(name, "");
    Json::Reader().parse(str, value);
  }

  void save(const BlkSerializator & ser, const char * name, const Json::Value & value)
  {
    ser.blk.addStr(name, Json::FastWriter().write(value).c_str());
  }

} // namespace proto