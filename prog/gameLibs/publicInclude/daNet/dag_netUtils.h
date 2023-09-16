//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ioSys/dag_baseIo.h>
#include <util/dag_stdint.h>
#include <math/dag_mathBase.h> // for clamp
#include <debug/dag_debug.h>
#include <daNet/bitStream.h>
#include <generic/dag_carray.h>
#include <memory/dag_framemem.h>

class String;
class Point3;
class DataBlock;
class Bitarray;
class SimpleString;

// NOTE:
// all this code has nothing with network. This is few (de)serialization and (un)packing methods.
// Some of methods in fact even used not in networking, but in quanitization
// TODO: split and move to some better place, like pack\unpack euler/dir should be in math or gameMath
// but before it probably should be also customizable, like pack_euler(&euler, int bits=12) or something

namespace netutils
{
void writeCompressedBuffer(danet::BitStream &msg, dag::ConstSpan<uint8_t> buf);
bool readCompressedBuffer(const danet::BitStream &msg, Tab<uint8_t> &buf);

void writeCompressedBlk(danet::BitStream *message, const DataBlock &blk);
bool readCompressedBlk(const danet::BitStream *message, DataBlock &blk);

void debugDump(const DataBlock *b);

float get_max(short); // not implemented
float get_max(char);  // not implemented
inline float get_max(uint16_t) { return 65535.f; }
inline float get_max(uint8_t) { return 255.f; }
inline float get_maxs(int16_t) { return 32767.f; }
inline float get_maxs(int8_t) { return 127.f; }

template <typename T>
inline T packun(float s, float _max, uint32_t n, const char *fu, const char *f, int l, const char *msg = "")
{
  G_ASSERT(n > 0 && n <= (sizeof(T) * 8) && n < 64);
  if (s > _max)
    logerr("%s(%d) : pack overlow in %s max = %f, val = %f, msg = %s", f, l, fu, _max, s, msg);
  else if (s < 0.f)
    logerr("%s(%d) : pack error (val is lesser then 0.f) in %s val = %f, msg = %s", f, l, fu, s, msg);
  return (T)clamp<uint64_t>(s / _max * (1ULL << n), 0, (1ULL << n) - 1);
}

template <typename T>
inline T packu(float s, float _max, const char *fu, const char *f, int l, const char *msg = "")
{
  (void)f;
  (void)fu;
  (void)l;
  if (s > _max)
    logerr("%s(%d) : pack overlow in %s max = %f, val = %f, msg = %s", f, l, fu, _max, s, msg);
  else if (s < 0.f)
    logerr("%s(%d) : pack error (val is lesser then 0.f) in %s val = %f, msg = %s", f, l, fu, s, msg);
  G_UNUSED(msg);
  return (T)((clamp(s, 0.f, _max) / _max) * get_max(T()));
}

template <typename T>
inline T packs(float s, float _max, const char *fu, const char *f, int l, const char *msg = "")
{
  if (rabs(s) > _max)
    logerr("%s(%d) : pack overlow in %s max = %f, val = %f, msg = %s", f, l, fu, _max, s, msg);
  G_UNUSED(msg);
  G_UNUSED(l);
  G_UNUSED(f);
  G_UNUSED(fu);
  return (T)((clamp(s, -_max, _max) / _max) * get_maxs(T()));
}

template <typename T>
inline float UNPACKN(T s, float _max, uint32_t n)
{
  G_ASSERT(n > 0 && n <= (sizeof(T) * 8) && n < 64);
  return float(((double)(s & ((1ULL << n) - 1))) / ((1ULL << n) - 1) * _max);
}

template <typename T>
inline float UNPACK(T s, float _max)
{
  return float(((double)s) / get_max(T()) * _max);
}

template <typename T>
inline float UNPACKS(T s, float _max)
{
  return float(((double)s) / get_maxs(T()) * _max);
}

template <typename T>
inline T packu_mm(float s, float _min, float _max, const char *fu, const char *f, int l, const char *msg = "")
{
  G_ASSERT(_max > _min);
  if (s > _max)
    logerr("%s(%d) : pack overlow in %s max = %f, val = %f, msg = %s", f, l, fu, _max, s, msg);
  else if (s < _min)
    logerr("%s(%d) : pack error (val is lesser then %f) in %s val = %f, msg = %s", f, l, _min, fu, s, msg);
  const float v = clamp(s, _min, _max);
  return (T)(((v - _min) / (_max - _min)) * get_max(T()));
}

template <typename T>
inline float UNPACK(T s, float _min, float _max)
{
  G_ASSERT(_max > _min);
  return float(((double)s) / get_max(T()) * (_max - _min) + _min);
}

// pack/unpack euler angles to 32 bits
// format : 3 bits for sign, 10 bits for heading, 9 for attitude, 10 for bank
unsigned int pack_euler(const Point3 &euler);
void unpack_euler(unsigned int packed, Point3 &res);

// pack/unpack euler angles to 3x16 bits
carray<int16_t, 3> pack_euler_16(const Point3 &euler);
void unpack_euler_16(carray<int16_t, 3> packed, Point3 &res);

// pack/unpack normalized direction to 3 bytes
void pack_dir(const Point3 &dir, uint8_t packed[3]);
void unpack_dir(const uint8_t packed[3], Point3 &res);

// pack/unpack car velocity in 32 bits
// format : 3 bits for axis signs (x, y, z), 11 bits for |v|, 9 for axis x, 9 for axis y
unsigned int pack_velocity(const Point3 &vel, float max_vel);
void unpack_velocity(unsigned int packed, Point3 &vel, float max_vel);

#define PACK_U16(s, _max)   netutils::packu<uint16_t>(s, _max, __FUNCTION__, __FILE__, __LINE__)
#define PACK_U8(s, _max)    netutils::packu<uint8_t>(s, _max, __FUNCTION__, __FILE__, __LINE__)
#define PACK_UN(s, _max, n) netutils::packun<uint8_t>(s, _max, n, __FUNCTION__, __FILE__, __LINE__)

#define PACK_S16(s, _max) netutils::packs<int16_t>(s, _max, __FUNCTION__, __FILE__, __LINE__)
#define PACK_S8(s, _max)  netutils::packs<int8_t>(s, _max, __FUNCTION__, __FILE__, __LINE__)

#define PACK_U16MM(s, _min, _max) netutils::packu_mm<uint16_t>(s, _min, _max, __FUNCTION__, __FILE__, __LINE__)
#define PACK_U8MM(s, _min, _max)  netutils::packu_mm<uint8_t>(s, _min, _max, __FUNCTION__, __FILE__, __LINE__)

#define PACK_U16_MSG(s, _max, msg)   netutils::packu<uint16_t>(s, _max, __FUNCTION__, __FILE__, __LINE__, msg)
#define PACK_U8_MSG(s, _max, msg)    netutils::packu<uint8_t>(s, _max, __FUNCTION__, __FILE__, __LINE__, msg)
#define PACK_UN_MSG(s, _max, n, msg) netutils::packun<uint8_t>(s, _max, n, __FUNCTION__, __FILE__, __LINE__, msg)

#define PACK_S16_MSG(s, _max, msg) netutils::packs<int16_t>(s, _max, __FUNCTION__, __FILE__, __LINE__, msg)
#define PACK_S8_MSG(s, _max, msg)  netutils::packs<int8_t>(s, _max, __FUNCTION__, __FILE__, __LINE__, msg)

#define PACK_U16MM_MSG(s, _min, _max, msg) netutils::packu_mm<uint16_t>(s, _min, _max, __FUNCTION__, __FILE__, __LINE__, msg)
#define PACK_U8MM_MSG(s, _min, _max, msg)  netutils::packu_mm<uint8_t>(s, _min, _max, __FUNCTION__, __FILE__, __LINE__, msg)

template <class T>
uint8_t truncate_to_uint8(T val)
{
  return static_cast<uint8_t>(val < 0xFF ? val : 0xFF);
}

template <class T>
uint16_t truncate_to_uint16(T val)
{
  return static_cast<uint16_t>(val < 0xFFFF ? val : 0xFFFF);
}

struct FloatSerializeProps
{
  bool sign = false;
  bool rough = false;
  float quantizeLimit = -1.f;
  FloatSerializeProps(bool s, bool r, float q) : sign(s), rough(r), quantizeLimit(q){};
};
void write_float(danet::BitStream &bs, float val, const FloatSerializeProps &props);
bool read_float(const danet::BitStream &bs, float &val, const FloatSerializeProps &props);

void write_dir(danet::BitStream &bs, const Point3 &dir);
bool read_dir(const danet::BitStream &bs, Point3 &dir);

void write_idx(danet::BitStream &bs, int idx);
bool read_idx(const danet::BitStream &bs, int &idx);

void write_bitarray(danet::BitStream &bs, const Bitarray &ba);
bool read_bitarray(const danet::BitStream &bs, Bitarray &ba);

void write_euler(danet::BitStream &bs, const Point3 &euler);
bool read_euler(const danet::BitStream &bs, Point3 &euler);

/// === serialize === (legacy)

enum SerializeMode
{
  READ,
  WRITE
};

template <class T, class = eastl::disable_if_t<eastl::is_const_v<T>>>
bool serialize(danet::BitStream &bs, T &val, SerializeMode mode) //-V659
{
  bool done = true;

  if (mode == WRITE)
    bs.Write(val);
  else
    done = bs.Read(val);

  return done;
}

// The overload is needed because BitStream::Read won't compile if provided with a const argument
template <class T>
bool serialize(danet::BitStream &bs, const T &val, SerializeMode mode) //-V659
{
  bool done = true;

  if (mode == WRITE)
    bs.Write(val);
  else
    G_VERIFYF(0, "Can't read into const");

  return done;
}

/// serialize array supporting stl-compatibility syntax
template <class Array, class SerializeFunc>
inline bool serialize_array(danet::BitStream &bs, Array &array, SerializeMode mode, SerializeFunc serialize_func)
{
  bool done = true;

  uint32_t size = mode == WRITE ? (uint32_t)array.size() : 0u;
  if (mode == WRITE)
    bs.WriteCompressed(size);
  else
    done = bs.ReadCompressed(size);
  if (done)
  {
    if (mode == READ)
      array.resize(size);

    for (int i = 0; i < size; ++i)
      done &= serialize_func(bs, array[i], mode);
  }

  return done;
}

/// serialize array supporting stl-compatibility syntax with default element serialization
template <class Array>
inline bool serialize_array(danet::BitStream &bs, Array &array, SerializeMode mode)
{
  return serialize_array(bs, array, mode,
    [](danet::BitStream &bs, auto &val, SerializeMode mode) -> bool { return serialize(bs, val, mode); });
}

bool serialize(danet::BitStream &bs, uint16_t &val, SerializeMode mode);
bool serialize_bitarray(danet::BitStream &bs, Bitarray &val, SerializeMode mode);
bool serialize_int_compressed(danet::BitStream &bs, int &val, SerializeMode mode, int min);
bool serialize_int_compressed(danet::BitStream &bs, int16_t &val, SerializeMode mode, int min);
template <typename T = int16_t>
bool serialize_float_compressed(danet::BitStream &bs, float &val, SerializeMode mode, float max);
bool serialize_positive_float_compressed(danet::BitStream &bs, float &val, SerializeMode mode, float max);
bool serialize_vec_compressed(danet::BitStream &bs, Point3 &pos, SerializeMode mode, float max);
inline bool serialize_pos_compressed(danet::BitStream &bs, Point3 &pos, SerializeMode mode, float max)
{
  return serialize_vec_compressed(bs, pos, mode, max);
}
bool serialize_dir_compressed(danet::BitStream &bs, Point3 &dir, SerializeMode mode);
bool serialize_simple_string(danet::BitStream &bs, SimpleString &str, SerializeMode mode);
inline bool serialize_idx(danet::BitStream &bs, int &val, SerializeMode mode) { return serialize_int_compressed(bs, val, mode, -1); }
inline bool serialize_idx(danet::BitStream &bs, int16_t &val, SerializeMode mode)
{
  return serialize_int_compressed(bs, val, mode, -1);
}
} // namespace netutils
