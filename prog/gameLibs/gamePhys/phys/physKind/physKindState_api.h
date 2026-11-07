// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gamePhys/phys/commonPhysBase.h>
#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <EASTL/type_traits.h>

constexpr unsigned PHYS_KIND_STATE_EXT_MAX = 128;
constexpr unsigned PHYS_KIND_CTRL_EXT_MAX = 32;
constexpr unsigned PHYS_KIND_SNAP_EXT_MAX = 16;

// PhysKindState::operator== compares velocities on this grid, so a quantized AAS matches the owner's exact history
constexpr int PHYS_KIND_AAS_POS_FRAC_BITS = 12;
constexpr int PHYS_KIND_AAS_VEL_FRAC_BITS = 10;
constexpr int PHYS_KIND_AAS_OMEGA_FRAC_BITS = 12;
constexpr int PHYS_KIND_AAS_ACC_FRAC_BITS = 8;
constexpr int PHYS_KIND_AAS_GRAVITY_FRAC_BITS = 16;
constexpr int PHYS_KIND_AAS_QUAT_BITS = 18;

enum class PhysKindLod : uint8_t
{
  Closest,
  Normal,
  Faraway,
  Min,
  Num
};

void write_phys_kind_ext(danet::BitStream &bs, const uint8_t *ext, unsigned ext_max, unsigned limit);
// Zero-fills ext past the stored bytes; -1 on error
int read_phys_kind_ext(const danet::BitStream &bs, uint8_t *ext, unsigned ext_max);

// Fixed point with one bit count for the vector: step 2^-frac_bits, clamped at 62 bits
void write_phys_kind_fixed(danet::BitStream &bs, const DPoint3 &v, int frac_bits);
bool read_phys_kind_fixed(const danet::BitStream &bs, DPoint3 &v, int frac_bits);
inline void write_phys_kind_fixed(danet::BitStream &bs, const Point3 &v, int frac_bits)
{
  write_phys_kind_fixed(bs, dpoint3(v), frac_bits);
}
inline bool read_phys_kind_fixed(const danet::BitStream &bs, Point3 &v, int frac_bits)
{
  DPoint3 dv(0, 0, 0);
  const bool ok = read_phys_kind_fixed(bs, dv, frac_bits);
  v = Point3::xyz(dv);
  return ok;
}
constexpr unsigned PHYS_KIND_FIXED_WIDTH_BITS = 6;
constexpr unsigned PHYS_KIND_FIXED_MAX_WIDTH = 62;
constexpr unsigned PHYS_KIND_FIXED_MAX_BITS = PHYS_KIND_FIXED_WIDTH_BITS + 3 * (PHYS_KIND_FIXED_MAX_WIDTH + 1);
// Smallest three, bits per component with its sign (2..24); reads q or -q
void write_phys_kind_quat(danet::BitStream &bs, const Quat &q, int bits);
bool read_phys_kind_quat(const danet::BitStream &bs, Quat &q, int bits);

// Snapshot steps by PhysKindLod; at Closest a 2.5 m arm turns by about the position step
constexpr int PHYS_KIND_SNAP_POS_FRAC_BITS[(int)PhysKindLod::Num] = {9, 9, 7, 5};
constexpr int PHYS_KIND_SNAP_QUAT_BITS[(int)PhysKindLod::Num] = {12, 11, 9, 8};
constexpr int PHYS_KIND_SNAP_VEL_FRAC_BITS[(int)PhysKindLod::Num] = {7, 6, 5, 4};
constexpr unsigned PHYS_KIND_SNAP_POSE_MAX_BITS = 2 * PHYS_KIND_FIXED_MAX_BITS + 2 + 3 * PHYS_KIND_SNAP_QUAT_BITS[0];

void write_phys_kind_snap_pose(danet::BitStream &bs, PhysKindLod lod, const gamephys::Loc &loc, const Point3 &vel);
bool read_phys_kind_snap_pose(const danet::BitStream &bs, PhysKindLod lod, DPoint3 &pos, Quat &quat, Point3 &vel);

enum class PhysKindWireCodec : uint8_t
{
  Raw,   // size bytes as they are
  Int,   // int of 1, 2 or 4 bytes or a bool in bits, clamped to their range; isSigned picks a signed one
  Range, // float in 1 to 24 bits, in steps of (max - min) / (2^bits - 1) from 0, clamped to min..max
  Step,  // float in steps of step, within +-(2^31 - 1) steps
};

// Trivial for das arrays: make one with = {}
struct PhysKindWireField
{
  uint8_t offset;
  uint8_t size;
  PhysKindWireCodec codec;
  uint8_t bits;
  bool isSigned;
  // The farthest LOD whose snapshots carry the field; Num: no snapshot
  PhysKindLod snapLod;
  float min;
  float max;
  float step;

  bool operator==(const PhysKindWireField &) const = default;
};

struct PhysKindWire
{
  dag::Vector<PhysKindWireField> fields;
  uint32_t maxSnapBits[(int)PhysKindLod::Num] = {};
};

template <typename F>
inline PhysKindWireField phys_kind_wire_field(size_t offset, PhysKindWireCodec codec, PhysKindLod snap_lod)
{
  PhysKindWireField f = {};
  f.offset = uint8_t(offset);
  f.size = uint8_t(sizeof(F));
  f.codec = codec;
  f.isSigned = eastl::is_integral_v<F> && eastl::is_signed_v<F>;
  f.snapLod = snap_lod;
  return f;
}
inline PhysKindWireField phys_kind_wire_bits(PhysKindWireField f, int bits, float min = 0.f, float max = 0.f)
{
  f.bits = uint8_t(bits);
  f.min = min;
  f.max = max;
  return f;
}
inline PhysKindWireField phys_kind_wire_step(PhysKindWireField f, float step)
{
  f.step = step;
  return f;
}

// A field of a C++ kind struct T, as das annotations give it; an array member takes RAW only
#define PHYS_KIND_WIRE_FIELD_(T, fld, codec, lod) \
  phys_kind_wire_field<decltype(((T *)nullptr)->fld)>(offsetof(T, fld), PhysKindWireCodec::codec, lod)
#define PHYS_KIND_WIRE_RAW(T, fld, lod)       PHYS_KIND_WIRE_FIELD_(T, fld, Raw, lod)
#define PHYS_KIND_WIRE_INT(T, fld, bits, lod) phys_kind_wire_bits(PHYS_KIND_WIRE_FIELD_(T, fld, Int, lod), bits)
#define PHYS_KIND_WIRE_RANGE(T, fld, min, max, bits, lod) \
  phys_kind_wire_bits(PHYS_KIND_WIRE_FIELD_(T, fld, Range, lod), bits, min, max)
#define PHYS_KIND_WIRE_STEP(T, fld, step, lod) phys_kind_wire_step(PHYS_KIND_WIRE_FIELD_(T, fld, Step, lod), step)

// One copy for each layout, kept to the exit; nullptr and a logerr for a field out of ext_max or with a bad codec
const PhysKindWire *phys_kind_intern_wire(dag::ConstSpan<PhysKindWireField> fields, unsigned ext_max);
// lod Num takes all fields, another LOD the fields its snapshots carry; a read keeps the other bytes
void write_phys_kind_wire(danet::BitStream &bs, const PhysKindWire &wire, const uint8_t *ext, PhysKindLod lod);
bool read_phys_kind_wire(const danet::BitStream &bs, const PhysKindWire &wire, uint8_t *ext, PhysKindLod lod);

// Snapshot ext: bit count, then wire fields of lod or, without a wire, a prefix of raw_limit bytes; ext spans the wire's ext_max
void write_phys_kind_snap_ext(danet::BitStream &bs, const PhysKindWire *wire, const uint8_t *ext, unsigned raw_limit, PhysKindLod lod);
// The reader keeps the bits, as it does not know the kind
bool read_phys_kind_snap_ext(const danet::BitStream &bs, uint8_t (&bits)[PHYS_KIND_SNAP_EXT_MAX], uint8_t &num_bits);
// Without a wire it zero-fills up to raw_limit; false, ext unchanged, for bits that the wire does not fit
bool decode_phys_kind_snap_ext(const PhysKindWire *wire, const uint8_t (&bits)[PHYS_KIND_SNAP_EXT_MAX], unsigned num_bits,
  PhysKindLod lod, uint8_t *ext, unsigned raw_limit);

template <typename T, unsigned N>
inline T &phys_kind_ext_as(uint8_t (&ext)[N])
{
  static_assert(sizeof(T) <= N && alignof(T) <= 8 && eastl::is_trivially_copyable_v<T>);
  return *reinterpret_cast<T *>(ext);
}
template <typename T, unsigned N>
inline const T &phys_kind_ext_as(const uint8_t (&ext)[N])
{
  static_assert(sizeof(T) <= N && alignof(T) <= 8 && eastl::is_trivially_copyable_v<T>);
  return *reinterpret_cast<const T *>(ext);
}

struct PhysKindState
{
  enum : uint8_t
  {
    FLAG_SLEEP = 1 << 0,
    // wire only: set when the stream carries gravDirection; optional fields follow the ext in the order of their bits
    FLAG_WIRE_CUSTOM_GRAVITY = 1 << 1,
  };

  int32_t atTick = -1;
  int32_t lastAppliedControlsForTick = -2;
  bool canBeCheckedForSync = true;
  uint8_t flags = 0;
  uint8_t actorTag = 0;

  gamephys::Loc location;
  Point3 velocity = Point3(0.f, 0.f, 0.f);
  Point3 omega = Point3(0.f, 0.f, 0.f);
  Point3 alpha = Point3(0.f, 0.f, 0.f);
  Point3 acceleration = Point3(0.f, 0.f, 0.f);
  // a game sets it after each tick for the next, as from gravity zones; a resync replaces the state before the replayed ticks, so
  // the authority approved state carries it when it is not down, as vehicle states do
  Point3 gravDirection = Point3(0.f, -1.f, 0.f);

  alignas(8) uint8_t ext[PHYS_KIND_STATE_EXT_MAX] = {};

  bool isSleep() const { return flags & FLAG_SLEEP; }

  void serialize(danet::BitStream &bs) const;
  bool deserialize(const danet::BitStream &bs);
  bool deserialize(const danet::BitStream &bs, IPhysBase &) { return deserialize(bs); }

  void applyAlternativeHistoryState(const PhysKindState &) {}
  void applyPartialState(const CommonPhysPartialState &state);
  void applyResyncedState(const PhysKindState &) {}
  void applyDesyncedState(const PhysKindState &) {}

  bool operator==(const PhysKindState &a) const;
  bool hasLargeDifferenceWith(const PhysKindState &a) const;
};

struct PhysKindControls
{
  int32_t producedAtTick = -1;
  int sequenceNumber = 0;
  uint8_t unitVersion = 0;
  bool isForged = false;
  // Of the PhysKind, kept by copies; deserialize reads a wired kind by it
  const PhysKindWire *wire = nullptr;

  alignas(8) uint8_t ext[PHYS_KIND_CTRL_EXT_MAX] = {};

  int getSequenceNumber() const { return sequenceNumber; }
  void setSequenceNumber(int seq) { sequenceNumber = seq; }
  void setControlsForged(bool val) { isForged = val; }
  bool isControlsForged() const { return isForged; }

  DEFINE_COMMON_PHYS_CONTROLS_METHODS(PhysKindControls);
  bool deserialize(const danet::BitStream &bs);
  // The owner applies the values that the server gets
  void snapToWire();
  void init() { reset(); }
  void stepHistory() {}
};
