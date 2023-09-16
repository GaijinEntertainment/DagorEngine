#include <daNet/dag_netUtils.h>
#include <daNet/daNetTypes.h>

#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_zstdIo.h>
#include <memory/dag_framemem.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/dag_mathAng.h>
#include <util/dag_bitArray.h>
#include <util/dag_simpleString.h>
#include <types/halfFloatSimple.h>

namespace netutils
{

void debugDump(const DataBlock *b)
{
  if (!b)
    return;
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
  DynamicMemGeneralSaveCB txtCwr(tmpmem, 0, 8 << 10);
  b->saveToTextStream(txtCwr);
  debug("%s", (const char *)txtCwr.data());
#endif
}

void writeCompressedBuffer(danet::BitStream &msg, dag::ConstSpan<uint8_t> buf)
{
  Tab<uint8_t> zip(framemem_ptr());
  zip.resize(max(4096u, buf.size() * 2)); // if compression worst than original
  size_t zwrite = zstd_compress(zip.data(), zip.size(), buf.data(), buf.size(), /*compressionLevel*/ 5);

  msg.reserveBits(BYTES_TO_BITS(sizeof(uint32_t) * 2 + (uint32_t)zwrite));
  msg.WriteCompressed((uint32_t)zwrite);
  msg.WriteCompressed((uint32_t)buf.size());
  msg.Write((const char *)zip.data(), (uint32_t)zwrite);
}

bool readCompressedBuffer(const danet::BitStream &msg, Tab<uint8_t> &buf)
{
  uint32_t comprSize = 0, size = 0;
  if (!msg.ReadCompressed(comprSize) || !msg.ReadCompressed(size))
    return false;
  G_ASSERT((comprSize < (1 << 20)) && (size < (1 << 20))); // sanity check
  if (BITS_TO_BYTES(msg.GetNumberOfUnreadBits()) < comprSize)
    return false;

  buf.resize(size);

  int readSize = BITS_TO_BYTES(msg.GetReadOffset());
  dag::ConstSpan<uint8_t> msgSlice(msg.GetData() + readSize, comprSize);

  size_t zread = zstd_decompress(buf.data(), buf.size(), msgSlice.data(), msgSlice.size());
  msg.SetReadOffset(msg.GetReadOffset() + BYTES_TO_BITS(msgSlice.size()));
  return zread == size;
}

void writeCompressedBlk(danet::BitStream *message, const DataBlock &blk)
{
  // TODO: consider replacement zlib with zstd with precoocked dictionary. see ioSys/dag_zlibIo.h for some details and benchmarks
  G_ASSERT(message);
  DynamicMemGeneralSaveCB cwr(framemem_ptr(), 0, 64 << 10);
  blk.saveToStream(cwr);
  writeCompressedBuffer(*message, dag::ConstSpan<uint8_t>((const uint8_t *)cwr.data(), cwr.size()));
}

bool readCompressedBlk(const danet::BitStream *message, DataBlock &blk)
{
  G_ASSERT(message);

  Tab<uint8_t> tmpBuf(framemem_ptr());
  if (!readCompressedBuffer(*message, tmpBuf))
    return false;

  InPlaceMemLoadCB crd(tmpBuf.data(), tmpBuf.size());
  bool ret = dblk::load_from_stream(blk, crd, dblk::ReadFlag::ROBUST | dblk::ReadFlag::BINARY_ONLY | dblk::ReadFlag::RESTORE_FLAGS);

  G_ASSERTF(blk.isValid(), "MP: Received blk is not valid");

  return ret;
}

#if !(_TARGET_APPLE || _TARGET_C3) || (_TARGET_PC_MACOSX && !_TARGET_SIMD_NEON)
#pragma float_control(push)
#pragma float_control(precise, on)
#endif

unsigned int pack_euler(const Point3 &euler)
{
  float heading = euler.y;
  float attitude = euler.z;
  float bank = euler.x;

  G_ASSERT(heading >= -PI && heading <= PI);
  G_ASSERT(attitude >= -HALFPI && attitude <= HALFPI);
  G_ASSERT(bank >= -PI && bank <= PI);

  unsigned int packed = 0;
  if (heading < 0)
    packed |= (1 << 31);
  if (attitude < 0)
    packed |= (1 << 30);
  if (bank < 0)
    packed |= (1 << 29);

  unsigned x;

  x = real2int(fabsf(heading) * ((1 << 10) - 1) / PI);
  G_ASSERT(x < (1 << 10));
  packed |= (x << 19);

  x = real2int(fabsf(attitude) * ((1 << 9) - 1) / HALFPI);
  G_ASSERT(x < (1 << 9));
  packed |= (x << 10);

  x = real2int(fabsf(bank) * ((1 << 10) - 1) / PI);
  G_ASSERT(x < (1 << 10));
  packed |= x;

  return packed;
}

void unpack_euler(unsigned int packed, Point3 &res)
{
  float heading = float((packed >> 19) & ((1 << 10) - 1)) * PI / ((1 << 10) - 1);
  float attitude = float((packed >> 10) & ((1 << 9) - 1)) * HALFPI / ((1 << 9) - 1);
  float bank = float(packed & ((1 << 10) - 1)) * PI / ((1 << 10) - 1);
  if (packed & (1 << 31))
    heading = -heading;
  if (packed & (1 << 30))
    attitude = -attitude;
  if (packed & (1 << 29))
    bank = -bank;

  res.x = bank;
  res.y = heading;
  res.z = attitude;
}

carray<int16_t, 3> pack_euler_16(const Point3 &euler)
{
  float heading = euler.y;
  float attitude = euler.z;
  float bank = euler.x;

  G_ASSERT(heading >= -PI && heading <= PI);
  G_ASSERT(attitude >= -HALFPI && attitude <= HALFPI);
  G_ASSERT(bank >= -PI && bank <= PI);

  carray<int16_t, 3> packed;
  packed[0] = PACK_S16(heading, PI);
  packed[1] = PACK_S16(attitude, HALFPI);
  packed[2] = PACK_S16(bank, PI);

  return packed;
}

void unpack_euler_16(carray<int16_t, 3> packed, Point3 &res)
{
  float heading = netutils::UNPACKS<int16_t>(packed[0], PI);
  float attitude = netutils::UNPACKS<int16_t>(packed[1], HALFPI);
  float bank = netutils::UNPACKS<int16_t>(packed[2], PI);

  res.x = bank;
  res.y = heading;
  res.z = attitude;
}

void pack_dir(const Point3 &dir, uint8_t packed[3])
{
  Point2 ang = dir_to_angles(dir);
  G_ASSERT(ang.x > (-PI - 1e-6f));
  G_ASSERT(ang.x < (PI + 1e-6f));
  G_ASSERT(ang.y > (-(PI / 2) - 1e-6f));
  G_ASSERT(ang.y < ((PI / 2) + 1e-6f));
  uint32_t p = 0;
  if (ang.x < 0)
    p |= 1 << 23;
  if (ang.y < 0)
    p |= 1 << 22;
  uint32_t az = real2int(fabsf(ang.x) / PI * float((1 << 11) - 1));
  uint32_t elev = real2int(fabsf(ang.y) / (PI / 2) * float((1 << 11) - 1));
  p |= az | (elev << 11);
  packed[0] = p & 0xFF;
  packed[1] = (p >> 8) & 0xFF;
  packed[2] = (p >> 16) & 0xFF;
}

void unpack_dir(const uint8_t packed[3], Point3 &res)
{
  uint32_t p = uint32_t(packed[0]) | (uint32_t(packed[1]) << 8) | (uint32_t(packed[2]) << 16);
  uint32_t az = p & ((1 << 11) - 1);
  uint32_t elev = (p >> 11) & ((1 << 11) - 1);
  Point2 ang;
  ang.x = float(az) / float((1 << 11) - 1) * PI;
  ang.y = float(elev) / float((1 << 11) - 1) * (PI / 2);
  if (p & (1 << 23))
    ang.x = -ang.x;
  if (p & (1 << 22))
    ang.y = -ang.y;
  res = angles_to_dir(ang);
}

unsigned int pack_velocity(const Point3 &vel, float maxSpeed)
{
  unsigned int packed = 0;

  float speed = length(vel);
  if (speed < 0.01f)
    return 0;
  Point3 axis = vel / speed;

  if (axis.x < 0)
    packed |= (1 << 31);
  if (axis.y < 0)
    packed |= (1 << 30);
  if (axis.z < 0)
    packed |= (1 << 29);

  unsigned int speedRange = (1 << 11) - 1;
  unsigned int packedSpeed = (unsigned int)((speed / maxSpeed) * float(speedRange));
  if (packedSpeed > speedRange)
    packedSpeed = speedRange;

  packed |= (packedSpeed << 18);

  unsigned int axisRange = (1 << 9) - 1;

  unsigned int x = (unsigned)floorf(fabsf(axis.x) * axisRange);
  unsigned int y = (unsigned)floorf(fabsf(axis.y) * axisRange);
  if (x > axisRange)
    x = axisRange;
  if (y > axisRange)
    y = axisRange;

  packed |= x << 9;
  packed |= y;

  return packed;
}

void unpack_velocity(unsigned int packed, Point3 &vel, float maxSpeed)
{
  if (!packed)
    vel.zero();
  else
  {
    unsigned int axisRange = (1 << 9) - 1;

    vel.x = float((packed >> 9) & axisRange) / float(axisRange);
    vel.y = float(packed & axisRange) / float(axisRange);
    float zSq = 1.0f - vel.x * vel.x - vel.y * vel.y;
    vel.z = (zSq > 0) ? sqrtf(zSq) : 0.0f;

    unsigned int speedRange = (1 << 11) - 1;
    unsigned int packedSpeed = (packed >> 18) & speedRange;
    float speed = float(packedSpeed) * maxSpeed / float(speedRange);
    vel *= speed;

    if (packed & (1 << 31))
      vel.x = -vel.x;
    if (packed & (1 << 30))
      vel.y = -vel.y;
    if (packed & (1 << 29))
      vel.z = -vel.z;
  }
}

#if !(_TARGET_APPLE || _TARGET_C3) || (_TARGET_PC_MACOSX && !_TARGET_SIMD_NEON)
#pragma float_control(pop)
#endif

bool serialize_bitarray(danet::BitStream &bs, Bitarray &ba, SerializeMode mode)
{
  bool done = true;

  uint32_t sizeBits = 0;
  if (mode == WRITE)
  {
    sizeBits = ba.size();
    bs.WriteCompressed(sizeBits);
    if (sizeBits > 0)
      bs.WriteBits((const uint8_t *)ba.getPtr(), sizeBits);
  }
  else
  {
    done &= bs.ReadCompressed(sizeBits);
    if (done && sizeBits > 0)
    {
      Tab<uint8_t> readBuffer(framemem_ptr());
      readBuffer.resize(BITS_TO_BYTES_WORD_ALIGNED(sizeBits));
      done &= bs.ReadBits(readBuffer.data(), sizeBits);
      if (done)
        ba.setPtr((const Bitarraybits *)readBuffer.data(), sizeBits);
    }
  }

  return done;
}

bool serialize_int_compressed(danet::BitStream &bs, int &val, SerializeMode mode, int min)
{
  bool done = true;

  if (mode == WRITE)
  {
    bs.WriteCompressed(uint32_t(clamp(val - min, 0, 0xFFFF)));
  }
  else
  {
    uint32_t ui = 0;
    done = bs.ReadCompressed(ui);
    if (done)
      val = int(ui) + min;
  }

  return done;
}

bool serialize_int_compressed(danet::BitStream &bs, int16_t &val, SerializeMode mode, int min)
{
  bool done = true;

  if (mode == WRITE)
  {
    bs.WriteCompressed(uint16_t(clamp(val - min, 0, 0xFFFF)));
  }
  else
  {
    uint16_t ui = 0;
    done = bs.ReadCompressed(ui);
    if (done)
      val = int(ui) + min;
  }

  return done;
}

template <>
bool serialize_float_compressed<int16_t>(danet::BitStream &bs, float &val, SerializeMode mode, float max)
{
  bool done = true;

  if (mode == WRITE)
  {
    bs.Write(PACK_S16(clamp(val, -max, max), max));
  }
  else
  {
    int16_t i16 = 0;
    done = bs.Read(i16);
    if (done)
      val = netutils::UNPACKS<int16_t>(i16, max);
  }

  return done;
}

template <>
bool serialize_float_compressed<int8_t>(danet::BitStream &bs, float &val, SerializeMode mode, float max)
{
  bool done = true;

  if (mode == WRITE)
  {
    bs.Write(PACK_S8(clamp(val, -max, max), max));
  }
  else
  {
    int8_t i8 = 0;
    done = bs.Read(i8);
    if (done)
      val = netutils::UNPACKS<int8_t>(i8, max);
  }

  return done;
}

bool serialize_positive_float_compressed(danet::BitStream &bs, float &val, SerializeMode mode, float max)
{
  bool done = true;

  if (mode == WRITE)
  {
    bs.WriteCompressed(PACK_U16(clamp(val, 0.f, max), max));
  }
  else
  {
    uint16_t ui16 = 0;
    done = bs.ReadCompressed(ui16);
    if (done)
      val = netutils::UNPACK<uint16_t>(ui16, max);
  }

  return done;
}

bool serialize_vec_compressed(danet::BitStream &bs, Point3 &pos, SerializeMode mode, float max)
{
  bool done = true;

  if (mode == WRITE)
  {
    bs.Write(PACK_S16(pos.x, max));
    bs.Write(PACK_S16(pos.y, max));
    bs.Write(PACK_S16(pos.z, max));
  }
  else
  {
    int16_t x = 0;
    int16_t y = 0;
    int16_t z = 0;
    done &= bs.Read(x);
    done &= bs.Read(y);
    done &= bs.Read(z);

    if (done)
      pos.set(netutils::UNPACKS<int16_t>(x, max), netutils::UNPACKS<int16_t>(y, max), netutils::UNPACKS<int16_t>(z, max));
  }

  return done;
}

bool serialize_dir_compressed(danet::BitStream &bs, Point3 &dir, SerializeMode mode)
{
  bool done = true;
  uint8_t dirPacked[] = {0, 0, 0};

  if (mode == WRITE)
  {
    netutils::pack_dir(dir, dirPacked);
    bs.WriteArray(dirPacked, countof(dirPacked));
  }
  else
  {
    done &= bs.ReadArray(dirPacked, countof(dirPacked));
    if (done)
      netutils::unpack_dir(dirPacked, dir);
  }

  return done;
}

bool serialize_simple_string(danet::BitStream &bs, SimpleString &str, SerializeMode mode)
{
  bool done = true;

  uint32_t size = 0;
  if (mode == WRITE)
  {
    size = str.length();
    bs.WriteCompressed(size);
    if (size > 0)
      bs.WriteArray(str.c_str(), size);
  }
  else
  {
    done &= bs.ReadCompressed(size);
    if (done && size > 0)
    {
      Tab<char> readBuffer(framemem_ptr());
      readBuffer.resize(size);
      done &= bs.ReadArray(readBuffer.data(), size);
      if (done)
        str.setStr(readBuffer.data(), size);
    }
  }

  return done;
}

bool serialize(danet::BitStream &bs, uint16_t &val, SerializeMode mode)
{
  if (mode == READ)
    return bs.ReadCompressed(val);

  bs.WriteCompressed(val);
  return true;
}

void write_float(danet::BitStream &bs, float val, const FloatSerializeProps &props)
{
  if (props.sign)
  {
    bs.Write(val < 0.f);
    val = fabs(val);
  }

  if (val < props.quantizeLimit)
  {
    bs.Write(true);
    if (props.rough)
      bs.Write(PACK_U8(val, props.quantizeLimit));
    else
      bs.WriteCompressed(PACK_U16(val, props.quantizeLimit));
  }
  else
  {
    bs.Write(false);
    if (props.rough)
    {
      HalfFloatSimplePositive f = val;
      bs.WriteCompressed(f.val);
    }
    else
    {
      bs.Write(val);
    }
  }
}

bool read_float(const danet::BitStream &bs, float &val, const FloatSerializeProps &props)
{
  bool done = true;

  bool neg = false;
  if (props.sign)
    done &= bs.Read(neg);

  bool quantize = false;
  done &= bs.Read(quantize);

  if (quantize)
  {
    if (props.rough)
    {
      uint8_t ui8 = 0;
      done &= bs.Read(ui8);
      if (done)
        val = netutils::UNPACK<uint8_t>(ui8, props.quantizeLimit);
    }
    else
    {
      uint16_t ui16 = 0;
      done &= bs.ReadCompressed(ui16);
      if (done)
        val = netutils::UNPACK<uint16_t>(ui16, props.quantizeLimit);
    }
  }
  else
  {
    if (props.rough)
    {
      HalfFloatSimplePositive f;
      done &= bs.ReadCompressed(f.val);
      val = f;
    }
    else
    {
      done &= bs.Read(val);
    }
  }

  if (neg)
    val = -val;

  return done;
}

void write_dir(danet::BitStream &bs, const Point3 &dir)
{
  uint8_t dirPacked[] = {0, 0, 0};
  netutils::pack_dir(dir, dirPacked);
  bs.WriteArray(dirPacked, countof(dirPacked));
}

bool read_dir(const danet::BitStream &bs, Point3 &dir)
{
  uint8_t dirPacked[] = {0, 0, 0};
  bool done = bs.ReadArray(dirPacked, countof(dirPacked));
  if (done)
    netutils::unpack_dir(dirPacked, dir);
  return done;
}

void write_idx(danet::BitStream &bs, int idx) { bs.WriteCompressed(uint16_t(clamp(idx + 1, 0, 0xFFFF))); }

bool read_idx(const danet::BitStream &bs, int &idx)
{
  uint16_t ui = 0;
  bool done = bs.ReadCompressed(ui);
  if (done)
    idx = int(ui) - 1;
  return done;
}

void write_bitarray(danet::BitStream &bs, const Bitarray &ba)
{
  uint32_t sizeBits = ba.size();
  bs.WriteCompressed(sizeBits);
  if (sizeBits > 0)
    bs.WriteBits((const uint8_t *)ba.getPtr(), sizeBits);
}

bool read_bitarray(const danet::BitStream &bs, Bitarray &ba)
{
  uint32_t sizeBits = 0;
  bool done = bs.ReadCompressed(sizeBits);

  if (done && sizeBits > 0)
  {
    Tab<uint8_t> readBuffer(framemem_ptr());
    readBuffer.resize(BITS_TO_BYTES_WORD_ALIGNED(sizeBits));
    done &= bs.ReadBits(readBuffer.data(), sizeBits);
    if (done)
      ba.setPtr((const Bitarraybits *)readBuffer.data(), sizeBits);
  }

  return done;
}

void write_euler(danet::BitStream &bs, const Point3 &angles)
{
  bs.Write(pack_euler({clamp(angles.x, -PI, PI), clamp(angles.y, -PI, PI), clamp(angles.z, -HALFPI, HALFPI)}));
}

bool read_euler(const danet::BitStream &bs, Point3 &angles)
{
  uint32_t packedAngles = 0;
  bool done = bs.Read(packedAngles);
  if (done)
    unpack_euler(packedAngles, angles);
  return done;
}

} // namespace netutils
