//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_carray.h>
#include <generic/dag_tab.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IPoint4.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>

namespace dag
{
template <typename T>
class is_container
{
  typedef char yes[1];
  typedef char no[2];
  template <typename U>
  static yes &test(typename U::value_type *);
  template <typename U>
  static no &test(...);

public:
  static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes);
};
} // namespace dag

template <typename T>
struct BsonType
{
  static const uint8_t type = 0xFF;
  static void add(Tab<uint8_t> &stream, const T &value);
};

template <>
struct BsonType<int>
{
  static const uint8_t type = 0x10;
  static void add(Tab<uint8_t> &stream, const int &value) { append_items(stream, 4, (const uint8_t *)&value); }
};
template <>
struct BsonType<uint32_t>
{
  static const uint8_t type = 0x10;
  static void add(Tab<uint8_t> &stream, const uint32_t &value) { append_items(stream, 4, (const uint8_t *)&value); }
};
template <>
struct BsonType<uint16_t>
{
  static const uint8_t type = 0x10;
  static void add(Tab<uint8_t> &stream, const uint16_t &value)
  {
    uint32_t v = value;
    BsonType<uint32_t>::add(stream, v);
  }
};
template <>
struct BsonType<int64_t>
{
  static const uint8_t type = 0x11;
  static void add(Tab<uint8_t> &stream, const int64_t &value) { append_items(stream, 8, (const uint8_t *)&value); }
};
template <>
struct BsonType<uint8_t>
{
  static const uint8_t type = 0x10;
  static void add(Tab<uint8_t> &stream, const uint8_t &value)
  {
    uint32_t v = value;
    BsonType<uint32_t>::add(stream, v);
  }
};
template <>
struct BsonType<double>
{
  static const uint8_t type = 0x01;
  static void add(Tab<uint8_t> &stream, const double &value) { append_items(stream, 8, (const uint8_t *)&value); }
};
template <>
struct BsonType<float>
{
  static const uint8_t type = 0x01;
  static void add(Tab<uint8_t> &stream, const float &value)
  {
    double v = value;
    BsonType<double>::add(stream, v);
  }
};
template <>
struct BsonType<bool>
{
  static const uint8_t type = 0x08;
  static void add(Tab<uint8_t> &stream, const bool &value)
  {
    uint8_t v = value ? 1 : 0;
    append_items(stream, 1, (const uint8_t *)&v);
  }
};
template <>
struct BsonType<const char *>
{
  static const uint8_t type = 0x02;
  static void add(Tab<uint8_t> &stream, const char *const &value)
  {
    append_items(stream, ::strlen(value) + 1, (const uint8_t *)value);
  }
};

class BsonStream
{
  static const uint8_t embedded_document_type = 0x03;
  static const uint8_t array_type = 0x04;

  bool done;
  Tab<uint8_t> stream;
  Tab<uint32_t> documentStack;

  void begin();
  void begin(const char *name, uint8_t type);

  void addName(const char *name, uint8_t type);

  template <typename T>
  void add(const char *name, const T &value, uint8_t type)
  {
    G_ASSERT(!done);

    addName(name, type);
    BsonType<T>::add(stream, value);
  }

public:
  struct StringIndex
  {
    int num;
    int intIndex;
    carray<char, 16> index;

    StringIndex() : num(1), intIndex(0)
    {
      mem_set_0(index);
      index[0] = '0';
    }

    const char *str() const { return &index[0]; }

    int idx() const { return intIndex; }

    void increment()
    {
      ++intIndex;

      ++index[num - 1];
      if (index[num - 1] <= 0x39)
        return;

      for (int j = num - 1; j >= 0; --j)
      {
        if (index[j] <= 0x39)
          break;

        if (j == 0)
        {
          index[num] = '0';
          index[0] = '1';
          ++num;
        }
        else
        {
          index[j] = '0';
          ++index[j - 1];
        }
      }
    }
  };

  BsonStream();
  BsonStream(size_t stream_reserve_bytes, size_t stack_reserve_bytes);

  dag::ConstSpan<uint8_t> closeAndGetData();

  void begin(const char *name);
  void beginArray(const char *name);
  void end();

  bool isOpened() const { return !done; };

  template <typename T, eastl::disable_if_t<dag::is_container<T>::value, bool> = true>
  void add(const char *name, const T &value)
  {
    add(name, value, BsonType<T>::type);
  }


  template <typename V, typename T = typename V::value_type>
  void add(const char *name, const V &value)
  {
    G_ASSERT(!done);

    begin(name, array_type);
    for (StringIndex index; index.idx() < value.size(); index.increment())
      add(index.str(), value[index.idx()]);
    end();
  }
  template <typename V, typename T = typename V::value_type, typename Writer>
  void add(const char *name, const V &value, Writer &writer)
  {
    G_ASSERT(!done);

    begin(name, array_type);
    for (StringIndex index; index.idx() < value.size(); index.increment())
    {
      begin(index.str());
      writer.writeToBson(*this, value[index.idx()]);
      end();
    }
    end();
  }

  template <typename T, typename Writer, std::enable_if_t<dag::is_container<T>::value == false, bool> = true>
  void add(const char *name, const T &value, Writer &writer)
  {
    G_ASSERT(!done);
    begin(name);
    writer.writeToBson(*this, value);
    end();
  }

  void add(const char *name, const char *value);
  void add(const char *name, char *value) { add(name, (const char *)value); }
  void add(const char *name, const Point2 &value);
  void add(const char *name, const Point3 &value);
  void add(const char *name, const Point4 &value);
  void add(const char *name, const DPoint3 &value);
  void add(const char *name, const IPoint2 &value);
  void add(const char *name, const IPoint3 &value);
  void add(const char *name, const IPoint4 &value);
  void add(const char *name, const TMatrix &value);
  void add(const char *name, const E3DCOLOR &value);
  void add(const char *name, const DataBlock &value);

  void add(const DataBlock &value);
  void addAsTreeNode(const DataBlock &value, const char *name);
};
