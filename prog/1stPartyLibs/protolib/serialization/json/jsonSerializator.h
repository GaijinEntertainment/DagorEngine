#pragma once
#ifndef _GAIJIN_PROTOIO_JSONSERIALIZATOR__
#define _GAIJIN_PROTOIO_JSONSERIALIZATOR__

#include <protoConfig.h>
#include <protolib/serialization/types.h>
#include <json/value.h>
#include <protolib/serialization/containersUtil.h>


namespace proto
{
  class JsonReader
  {
  public:
    explicit JsonReader(const Json::Value & value_, bool skipKeyField_ = false)
      : value(value_)
      , skipKeyField(skipKeyField_)
    {
    }

    JsonReader(const JsonReader & /*parent*/, const Json::Value & value_, bool skipKeyField_ = false)
      : value(value_)
      , skipKeyField(skipKeyField_)
    {
    }

    const Json::Value & getJson() const
    {
      return value;
    }

    bool isSkipKeyField() const
    {
      return skipKeyField;
    }

  private:
    const Json::Value & value;
    bool          skipKeyField;
  };


  class JsonWriter
  {
  public:
    explicit JsonWriter(Json::Value & value_, bool skipKeyField_ = false, bool writeInt64AsStr_ = true)
      : value(value_)
      , skipKeyField(skipKeyField_)
      , writeInt64AsStr(writeInt64AsStr_)
    {
    }

    JsonWriter(const JsonWriter & parent, Json::Value & value_, bool skipKeyField_ = false)
      : value(value_)
      , skipKeyField(skipKeyField_)
      , writeInt64AsStr(parent.shouldWriteInt64AsStr())
    {
    }

    Json::Value & getJson() const
    {
      return value;
    }

    bool isSkipKeyField() const
    {
      return skipKeyField;
    }

    bool shouldWriteInt64AsStr() const
    {
      return writeInt64AsStr;
    }

  private:
    Json::Value & value;
    bool          skipKeyField;
    bool          writeInt64AsStr;
  };


  namespace io
  {
    template <typename T>
    struct JsonValueWriter
    {
      static void write(Json::Value & dst, const T & val)
      {
        dst = val;
      }
    };

    template <>
    struct JsonValueWriter<string>
    {
      static void write(Json::Value & dst, const string & val)
      {
        dst = val.c_str();
      }
    };


    template <typename T>
    struct JsonValueWriterAsString
    {
      static void write(Json::Value & dst, const T & val)
      {
        util::Value2StrWrapper<T> wrapper;
        dst = wrapper.valueToStr(val);
      }
    };


    template <typename T>
    struct JsonValueReader;

#define DECLARE_READER(TYPE, IS_FN, AS_FN)                      \
    template <> struct JsonValueReader<TYPE>                    \
    {                                                           \
      static bool read(const Json::Value & src, TYPE & val)     \
      {                                                         \
        if (src.IS_FN())                                        \
        {                                                       \
          val = src.AS_FN();                                    \
          return true;                                          \
        }                                                       \
        return false;                                           \
      }                                                         \
    };

    DECLARE_READER(int32, isInt, asInt);
    DECLARE_READER(uint32, isIntegral, asUInt);
    DECLARE_READER(bool, isBool, asBool);
    DECLARE_READER(string, isString, asCString);

    namespace fallback
    {
      template <typename T>
      struct JsonValueReader;

      DECLARE_READER(int64, isIntegral, asInt64);
      DECLARE_READER(uint64, isIntegral, asUInt64);
      DECLARE_READER(float, isNumeric, asFloat);
      DECLARE_READER(double, isNumeric, asDouble);
    }

    template <typename T>
    struct JsonValueReadAsString
    {
      static bool read(const Json::Value & src, T & val)
      {
        if (src.isString())
        {
          util::Value2StrWrapper<T> wrapper;
          return wrapper.strToValue(src.asCString(), val);
        }
        else
        {
          return fallback::JsonValueReader<T>::read(src, val);
        }
      }
    };

    template <> struct JsonValueReader<int64> : JsonValueReadAsString<int64> {};
    template <> struct JsonValueReader<uint64> : JsonValueReadAsString<uint64> {};
    template <> struct JsonValueReader<float> : JsonValueReadAsString<float> {};
    template <> struct JsonValueReader<double> : JsonValueReadAsString<double> {};

#undef DECLARE_READER
  }

  template <typename T>
  void writeInt64ToJson(const proto::JsonWriter & ser, Json::Value & dst, const T & val)
  {
    if (ser.shouldWriteInt64AsStr())
      io::JsonValueWriterAsString<T>::write(dst, val);
    else
      io::JsonValueWriter<T>::write(dst, val);
  }

  template <typename T>
  void writeValueToJson(const proto::JsonWriter &, Json::Value & dst, const T & val)
  {
    io::JsonValueWriter<T>::write(dst, val);
  }

  inline void writeValueToJson(const proto::JsonWriter & ser, Json::Value & dst, const int64 & val)
  {
    writeInt64ToJson(ser, dst, val);
  }

  inline void writeValueToJson(const proto::JsonWriter & ser, Json::Value & dst, const uint64 & val)
  {
    writeInt64ToJson(ser, dst, val);
  }


  template <typename T>
  T readValueFromJson(const Json::Value & dst, const T & def)
  {
    T val(def);
    if (io::JsonValueReader<T>::read(dst, val))
    {
      return val;
    }

    return def;
  }
}

#endif // _GAIJIN_PROTOIO_JSONSERIALIZATOR__
