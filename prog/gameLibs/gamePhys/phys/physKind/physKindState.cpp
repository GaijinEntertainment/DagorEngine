// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "physKindState_api.h"
#include <daNet/bitStream.h>
#include <math/dag_mathUtils.h>
#include <math/dag_bits.h>
#include <debug/dag_log.h>
#include <memory/dag_framemem.h>
#include <EASTL/algorithm.h>
#include <EASTL/unique_ptr.h>
#include <math.h>
#include <string.h>

void write_phys_kind_ext(danet::BitStream &bs, const uint8_t *ext, unsigned ext_max, unsigned limit)
{
  unsigned size = min(limit, ext_max);
  while (size && !ext[size - 1])
    --size;
  G_ASSERT(size <= UCHAR_MAX);
  bs.Write(uint8_t(size));
  bs.Write((const char *)ext, size);
}

int read_phys_kind_ext(const danet::BitStream &bs, uint8_t *ext, unsigned ext_max)
{
  uint8_t size = 0;
  if (!bs.Read(size) || bs.GetNumberOfUnreadBits() < uint32_t(size) * CHAR_BIT)
    return -1;
  unsigned stored = min<unsigned>(size, ext_max);
  bs.Read((char *)ext, stored);
  bs.IgnoreBytes(size - stored);
  memset(ext + stored, 0, ext_max - stored);
  return int(stored);
}

static uint8_t bit_width(uint64_t v)
{
  const uint32_t hi = uint32_t(v >> 32);
  return hi ? uint8_t(33 + __bsr(hi)) : (v ? uint8_t(1 + __bsr(uint32_t(v))) : 0);
}

void write_phys_kind_fixed(danet::BitStream &bs, const DPoint3 &v, int frac_bits)
{
  constexpr uint64_t maxMagnitude = (uint64_t(1) << PHYS_KIND_FIXED_MAX_WIDTH) - 1;
  const double scale = ldexp(1.0, frac_bits);
  uint64_t magnitude[3];
  bool negative[3];
  uint64_t all = 0;
  for (int i = 0; i < 3; ++i)
  {
    // double(maxMagnitude) is 2^62; NaN may pass clamp
    const int64_t q = llrint(clamp(v[i] * scale, -double(maxMagnitude), double(maxMagnitude)));
    negative[i] = q < 0;
    magnitude[i] = min(negative[i] ? 0 - uint64_t(q) : uint64_t(q), maxMagnitude);
    all |= magnitude[i];
  }
  const uint8_t width = bit_width(all);
  bs.WriteBits(&width, PHYS_KIND_FIXED_WIDTH_BITS);
  if (width)
    for (int i = 0; i < 3; ++i)
    {
      bs.WriteBits((const uint8_t *)&magnitude[i], width);
      bs.Write(negative[i]);
    }
}

bool read_phys_kind_fixed(const danet::BitStream &bs, DPoint3 &v, int frac_bits)
{
  uint8_t width = 0;
  if (!bs.ReadBits(&width, PHYS_KIND_FIXED_WIDTH_BITS) || width > PHYS_KIND_FIXED_MAX_WIDTH)
    return false;
  const double step = ldexp(1.0, -frac_bits);
  for (int i = 0; i < 3; ++i)
  {
    uint64_t magnitude = 0;
    bool negative = false;
    if (width && !(bs.ReadBits((uint8_t *)&magnitude, width) && bs.Read(negative)))
      return false;
    v[i] = (negative ? -double(magnitude) : double(magnitude)) * step;
  }
  return true;
}

// Offset binary: 0..2*steps fits in bits
static uint32_t quat_steps(int bits) { return (1u << (bits - 1)) - 1; }

void write_phys_kind_quat(danet::BitStream &bs, const Quat &quat, int bits)
{
  G_ASSERT(bits >= 2 && bits <= 24);
  const Quat q = normalize(quat);
  uint8_t largest = 0;
  for (uint8_t i = 1; i < 4; ++i)
    if (fabsf(q[i]) > fabsf(q[largest]))
      largest = i;
  const int32_t steps = int32_t(quat_steps(bits));
  const float scale = (q[largest] < 0.f ? -steps : steps) * float(M_SQRT2);
  bs.WriteBits(&largest, 2);
  for (int i = 0; i < 4; ++i)
    if (i != largest)
    {
      const uint32_t packed = uint32_t(clamp(int32_t(lrintf(q[i] * scale)), -steps, steps) + steps);
      bs.WriteBits((const uint8_t *)&packed, bits);
    }
}

bool read_phys_kind_quat(const danet::BitStream &bs, Quat &q, int bits)
{
  uint8_t largest = 0;
  if (!bs.ReadBits(&largest, 2))
    return false;
  const uint32_t steps = quat_steps(bits);
  const float unit = float(M_SQRT1_2) / steps;
  float rest = 1.f;
  for (int i = 0; i < 4; ++i)
    if (i != largest)
    {
      uint32_t packed = 0;
      if (!bs.ReadBits((uint8_t *)&packed, bits))
        return false;
      q[i] = (int32_t(min(packed, 2 * steps)) - int32_t(steps)) * unit;
      rest -= sqr(q[i]);
    }
  q[largest] = rest > 0.f ? sqrtf(rest) : 0.f;
  q = normalize(q);
  return true;
}

void write_phys_kind_snap_pose(danet::BitStream &bs, PhysKindLod lod, const gamephys::Loc &loc, const Point3 &vel)
{
  write_phys_kind_fixed(bs, loc.P, PHYS_KIND_SNAP_POS_FRAC_BITS[(int)lod]);
  write_phys_kind_quat(bs, loc.O.getQuat(), PHYS_KIND_SNAP_QUAT_BITS[(int)lod]);
  write_phys_kind_fixed(bs, vel, PHYS_KIND_SNAP_VEL_FRAC_BITS[(int)lod]);
}

bool read_phys_kind_snap_pose(const danet::BitStream &bs, PhysKindLod lod, DPoint3 &pos, Quat &quat, Point3 &vel)
{
  return read_phys_kind_fixed(bs, pos, PHYS_KIND_SNAP_POS_FRAC_BITS[(int)lod]) &&
         read_phys_kind_quat(bs, quat, PHYS_KIND_SNAP_QUAT_BITS[(int)lod]) &&
         read_phys_kind_fixed(bs, vel, PHYS_KIND_SNAP_VEL_FRAC_BITS[(int)lod]);
}

static constexpr uint32_t STEP_WIDTH_BITS = 5;
static constexpr uint32_t STEP_MAX_WIDTH = 31;

static const char *wire_field_error(const PhysKindWireField &f, unsigned ext_max)
{
  if (f.size == 0 || f.offset + f.size > ext_max)
    return "out of the ext";
  switch (f.codec)
  {
    case PhysKindWireCodec::Raw: return nullptr;
    case PhysKindWireCodec::Int:
    {
      const bool intSize = f.size == 1 || f.size == 2 || f.size == 4;
      return intSize && f.bits >= 1 && f.bits <= f.size * CHAR_BIT ? nullptr : "an int of bad size or bits";
    }
    case PhysKindWireCodec::Range: return f.size == 4 && f.bits >= 1 && f.bits <= 24 && f.max > f.min ? nullptr : "a bad range";
    case PhysKindWireCodec::Step: return f.size == 4 && f.step > 0.f ? nullptr : "a bad step";
  }
  return "an unknown codec";
}

static unsigned wire_field_max_bits(const PhysKindWireField &f)
{
  switch (f.codec)
  {
    case PhysKindWireCodec::Raw: return f.size * CHAR_BIT;
    case PhysKindWireCodec::Int:
    case PhysKindWireCodec::Range: return f.bits;
    case PhysKindWireCodec::Step: return 1 + STEP_WIDTH_BITS + STEP_MAX_WIDTH;
  }
  return 0;
}

static bool wire_field_at(const PhysKindWireField &f, PhysKindLod lod)
{
  return lod == PhysKindLod::Num || (f.snapLod != PhysKindLod::Num && lod <= f.snapLod);
}

const PhysKindWire *phys_kind_intern_wire(dag::ConstSpan<PhysKindWireField> fields, unsigned ext_max)
{
  for (const PhysKindWireField &f : fields)
    if (const char *error = wire_field_error(f, ext_max))
    {
      logerr("phys_kind wire: a field of %d bytes at byte %d is %s (ext %u bytes)", f.size, f.offset, error, ext_max);
      return nullptr;
    }
  static dag::Vector<eastl::unique_ptr<PhysKindWire>> interned;
  for (const eastl::unique_ptr<PhysKindWire> &wire : interned)
    if (wire->fields.size() == fields.size() && eastl::equal(fields.begin(), fields.end(), wire->fields.begin()))
      return wire.get();
  PhysKindWire &wire = *interned.emplace_back(eastl::make_unique<PhysKindWire>());
  wire.fields.assign(fields.begin(), fields.end());
  for (int lod = 0; lod < (int)PhysKindLod::Num; ++lod)
    for (const PhysKindWireField &f : fields)
      if (wire_field_at(f, PhysKindLod(lod)))
        wire.maxSnapBits[lod] += wire_field_max_bits(f);
  return &wire;
}

static int64_t load_int(const uint8_t *p, unsigned size, bool is_signed)
{
  uint32_t u = 0;
  memcpy(&u, p, size);
  const unsigned shift = 32 - size * CHAR_BIT;
  return is_signed ? int64_t(int32_t(u << shift) >> shift) : int64_t(u);
}

static void store_int(uint8_t *p, unsigned size, int64_t v)
{
  const uint32_t u = uint32_t(v);
  memcpy(p, &u, size);
}

static int64_t clamp_int(const PhysKindWireField &f, int64_t v)
{
  return f.isSigned ? clamp(v, -(int64_t(1) << (f.bits - 1)), (int64_t(1) << (f.bits - 1)) - 1)
                    : clamp(v, int64_t(0), (int64_t(1) << f.bits) - 1);
}

static float load_float(const uint8_t *p)
{
  float v;
  memcpy(&v, p, sizeof(v));
  return v;
}

static void store_float(uint8_t *p, float v) { memcpy(p, &v, sizeof(v)); }

struct RangeGrid
{
  double step;
  int64_t first;
  uint32_t last;
  float min, max;
};

// From 0, so a range around 0 has 0 exactly; the top code can pass max
static RangeGrid range_grid(const PhysKindWireField &f)
{
  const double step = (double(f.max) - f.min) / double((1u << f.bits) - 1);
  return {step, int64_t(ceil(f.min / step)), (1u << f.bits) - 1, f.min, f.max};
}

// Clamped first: a snapped value keeps its code
static uint32_t range_encode(const RangeGrid &g, float v)
{
  return uint32_t(clamp(int64_t(llrint(clamp(v, g.min, g.max) / g.step)), g.first, g.first + int64_t(g.last)) - g.first);
}

static float range_decode(const RangeGrid &g, uint32_t q) { return clamp(float(double(int64_t(q) + g.first) * g.step), g.min, g.max); }

static int64_t step_encode(const PhysKindWireField &f, float v)
{
  constexpr int64_t maxSteps = (int64_t(1) << STEP_MAX_WIDTH) - 1;
  // fast math: NaN may pass the clamp
  const int64_t q = llrint(clamp(double(v) / f.step, -double(maxSteps), double(maxSteps)));
  return clamp(q, -maxSteps, maxSteps);
}

static void write_wire_field(danet::BitStream &bs, const PhysKindWireField &f, const uint8_t *p)
{
  switch (f.codec)
  {
    case PhysKindWireCodec::Raw: bs.Write((const char *)p, f.size); break;
    case PhysKindWireCodec::Int:
    {
      const uint64_t u = uint64_t(clamp_int(f, load_int(p, f.size, f.isSigned)));
      bs.WriteBits((const uint8_t *)&u, f.bits);
      break;
    }
    case PhysKindWireCodec::Range:
    {
      const uint32_t q = range_encode(range_grid(f), load_float(p));
      bs.WriteBits((const uint8_t *)&q, f.bits);
      break;
    }
    case PhysKindWireCodec::Step:
    {
      const int64_t q = step_encode(f, load_float(p));
      const uint32_t magnitude = uint32_t(q < 0 ? -q : q);
      const uint8_t width = bit_width(magnitude);
      bs.WriteBits(&width, STEP_WIDTH_BITS);
      if (width)
      {
        bs.WriteBits((const uint8_t *)&magnitude, width);
        bs.Write(q < 0);
      }
      break;
    }
  }
}

static bool read_wire_field(const danet::BitStream &bs, const PhysKindWireField &f, uint8_t *p)
{
  switch (f.codec)
  {
    case PhysKindWireCodec::Raw: return bs.Read((char *)p, f.size);
    case PhysKindWireCodec::Int:
    {
      uint64_t u = 0;
      if (!bs.ReadBits((uint8_t *)&u, f.bits))
        return false;
      const bool negative = f.isSigned && ((u >> (f.bits - 1)) & 1);
      store_int(p, f.size, negative ? int64_t(u) - (int64_t(1) << f.bits) : int64_t(u));
      return true;
    }
    case PhysKindWireCodec::Range:
    {
      uint32_t q = 0;
      if (!bs.ReadBits((uint8_t *)&q, f.bits))
        return false;
      store_float(p, range_decode(range_grid(f), q));
      return true;
    }
    case PhysKindWireCodec::Step:
    {
      uint8_t width = 0;
      uint32_t magnitude = 0;
      bool negative = false;
      if (!bs.ReadBits(&width, STEP_WIDTH_BITS) || (width && !(bs.ReadBits((uint8_t *)&magnitude, width) && bs.Read(negative))))
        return false;
      store_float(p, float((negative ? -double(magnitude) : double(magnitude)) * f.step));
      return true;
    }
  }
  return false;
}

void write_phys_kind_wire(danet::BitStream &bs, const PhysKindWire &wire, const uint8_t *ext, PhysKindLod lod)
{
  for (const PhysKindWireField &f : wire.fields)
    if (wire_field_at(f, lod))
      write_wire_field(bs, f, ext + f.offset);
}

bool read_phys_kind_wire(const danet::BitStream &bs, const PhysKindWire &wire, uint8_t *ext, PhysKindLod lod)
{
  for (const PhysKindWireField &f : wire.fields)
    if (wire_field_at(f, lod) && !read_wire_field(bs, f, ext + f.offset))
      return false;
  return true;
}

// Into stream layout: a partial last byte high
static bool read_phys_kind_bits(const danet::BitStream &bs, uint8_t *data, uint32_t bits)
{
  if (!bs.ReadBits(data, bits & ~7u))
    return false;
  if (const uint32_t rest = bits & 7u)
  {
    uint8_t last = 0;
    if (!bs.ReadBits(&last, rest))
      return false;
    data[bits / 8] = uint8_t(last << (8 - rest));
  }
  return true;
}

void write_phys_kind_snap_ext(danet::BitStream &bs, const PhysKindWire *wire, const uint8_t *ext, unsigned raw_limit, PhysKindLod lod)
{
  const uint32_t sizeAt = bs.GetWriteOffset();
  if (wire)
  {
    G_ASSERT(wire->maxSnapBits[(int)lod] <= PHYS_KIND_SNAP_EXT_MAX * CHAR_BIT);
    bs.Write(uint8_t(0));
    write_phys_kind_wire(bs, *wire, ext, lod);
  }
  else
    write_phys_kind_ext(bs, ext, PHYS_KIND_SNAP_EXT_MAX, raw_limit);
  bs.WriteAt(uint8_t(bs.GetWriteOffset() - sizeAt - CHAR_BIT), sizeAt);
}

bool read_phys_kind_snap_ext(const danet::BitStream &bs, uint8_t (&bits)[PHYS_KIND_SNAP_EXT_MAX], uint8_t &num_bits)
{
  memset(bits, 0, sizeof(bits));
  return bs.Read(num_bits) && num_bits <= PHYS_KIND_SNAP_EXT_MAX * CHAR_BIT && read_phys_kind_bits(bs, bits, num_bits);
}

bool decode_phys_kind_snap_ext(const PhysKindWire *wire, const uint8_t (&bits)[PHYS_KIND_SNAP_EXT_MAX], unsigned num_bits,
  PhysKindLod lod, uint8_t *ext, unsigned raw_limit)
{
  if (!wire)
  {
    const unsigned size = min(raw_limit, PHYS_KIND_SNAP_EXT_MAX);
    const unsigned stored = min(size, num_bits / CHAR_BIT);
    memcpy(ext, bits, stored);
    memset(ext + stored, 0, size - stored);
    return true;
  }
  const danet::BitStream bs(bits, PHYS_KIND_SNAP_EXT_MAX, /*copy*/ false);
  uint8_t scratch[PHYS_KIND_STATE_EXT_MAX];
  if (!read_phys_kind_wire(bs, *wire, scratch, lod) || bs.GetReadOffset() != num_bits)
    return false;
  bs.ResetReadPointer();
  return read_phys_kind_wire(bs, *wire, ext, lod);
}

static void write_aas_location(danet::BitStream &bs, const gamephys::Loc &location)
{
  write_phys_kind_fixed(bs, location.P, PHYS_KIND_AAS_POS_FRAC_BITS);
  write_phys_kind_quat(bs, location.O.getQuat(), PHYS_KIND_AAS_QUAT_BITS);
}

static bool read_aas_location(const danet::BitStream &bs, gamephys::Loc &location)
{
  Quat q;
  if (!read_phys_kind_fixed(bs, location.P, PHYS_KIND_AAS_POS_FRAC_BITS) || !read_phys_kind_quat(bs, q, PHYS_KIND_AAS_QUAT_BITS))
    return false;
  location.O.setQuat(q);
  return true;
}

void PhysKindState::serialize(danet::BitStream &bs) const
{
  bs.Write(atTick);
  write_aas_location(bs, location);
  write_phys_kind_fixed(bs, velocity, PHYS_KIND_AAS_VEL_FRAC_BITS);
  write_phys_kind_fixed(bs, omega, PHYS_KIND_AAS_OMEGA_FRAC_BITS);
  write_phys_kind_fixed(bs, alpha, PHYS_KIND_AAS_ACC_FRAC_BITS);
  write_phys_kind_fixed(bs, acceleration, PHYS_KIND_AAS_ACC_FRAC_BITS);
  const bool isCustomGravity = gravDirection != Point3(0.f, -1.f, 0.f);
  bs.Write(uint8_t(flags | (isCustomGravity ? FLAG_WIRE_CUSTOM_GRAVITY : 0)));
  write_phys_kind_ext(bs, ext, PHYS_KIND_STATE_EXT_MAX, PHYS_KIND_STATE_EXT_MAX);
  if (isCustomGravity)
    write_phys_kind_fixed(bs, gravDirection, PHYS_KIND_AAS_GRAVITY_FRAC_BITS);
  write_controls_tick_delta(bs, atTick, lastAppliedControlsForTick);
  // Last: foreign states keep their tick
  bs.Write(actorTag);
}

bool PhysKindState::deserialize(const danet::BitStream &bs)
{
  gravDirection = Point3(0.f, -1.f, 0.f);
  if (!bs.Read(atTick) || !read_aas_location(bs, location) || !read_phys_kind_fixed(bs, velocity, PHYS_KIND_AAS_VEL_FRAC_BITS) ||
      !read_phys_kind_fixed(bs, omega, PHYS_KIND_AAS_OMEGA_FRAC_BITS))
    return false;
  if (!read_phys_kind_fixed(bs, alpha, PHYS_KIND_AAS_ACC_FRAC_BITS) ||
      !read_phys_kind_fixed(bs, acceleration, PHYS_KIND_AAS_ACC_FRAC_BITS) || !bs.Read(flags) ||
      read_phys_kind_ext(bs, ext, PHYS_KIND_STATE_EXT_MAX) < 0)
    return false;
  const bool isCustomGravity = flags & FLAG_WIRE_CUSTOM_GRAVITY;
  flags &= ~FLAG_WIRE_CUSTOM_GRAVITY;
  if (isCustomGravity)
  {
    if (!read_phys_kind_fixed(bs, gravDirection, PHYS_KIND_AAS_GRAVITY_FRAC_BITS))
      return false;
    gravDirection = normalize(gravDirection);
  }
  return read_controls_tick_delta(bs, atTick, lastAppliedControlsForTick) && bs.Read(actorTag);
}

void PhysKindState::applyPartialState(const CommonPhysPartialState &state)
{
  location = state.location;
  velocity = state.velocity;
}

// The speed tests are finer than the grid
static Point3 aas_velocity(const PhysKindState &s)
{
  constexpr float scale = float(1 << PHYS_KIND_AAS_VEL_FRAC_BITS);
  return Point3(rintf(s.velocity.x * scale), rintf(s.velocity.y * scale), rintf(s.velocity.z * scale)) / scale;
}

// The quat wire can flip the sign
static float rotation_diff_sq(const gamephys::Orient &a, const gamephys::Orient &b)
{
  const Point4 qa = a.getQuatAsP4(), qb = b.getQuatAsP4();
  return min(lengthSq(qa - qb), lengthSq(qa + qb));
}

bool PhysKindState::operator==(const PhysKindState &a) const
{
  constexpr float posThresholdSq = 1e-4f;
  constexpr float velThresholdSq = 0.01f;
  constexpr float omegaThresholdSq = 1e-4f;
  constexpr float cos3Deg = 0.998629534754f;

  if (lengthSq(location.P - a.location.P) > posThresholdSq)
    return false;
  const Point3 vel = aas_velocity(*this), aVel = aas_velocity(a);
  const float velSq = vel.lengthSq();
  const float aVelSq = aVel.lengthSq();
  if (fabsf(velSq - aVelSq) > velThresholdSq)
    return false;
  if (velSq > 1e-12f && aVelSq > 1e-12f && (vel / sqrtf(velSq)) * (aVel / sqrtf(aVelSq)) < cos3Deg)
    return false;
  if (rotation_diff_sq(location.O, a.location.O) > 1e-6f)
    return false;
  return lengthSq(omega - a.omega) < omegaThresholdSq && flags == a.flags && memcmp(ext, a.ext, sizeof(ext)) == 0;
}

bool PhysKindState::hasLargeDifferenceWith(const PhysKindState &a) const
{
  return lengthSq(location.P - a.location.P) > 0.01f || rotation_diff_sq(location.O, a.location.O) > 1e-5f ||
         lengthSq(velocity - a.velocity) > 0.1f || lengthSq(omega - a.omega) > 0.01f;
}

static void write_controls_ext(danet::BitStream &bs, const PhysKindControls &controls)
{
  if (controls.wire)
    write_phys_kind_wire(bs, *controls.wire, controls.ext, PhysKindLod::Num);
  else
    write_phys_kind_ext(bs, controls.ext, PHYS_KIND_CTRL_EXT_MAX, PHYS_KIND_CTRL_EXT_MAX);
}

static bool read_controls_ext(const danet::BitStream &bs, PhysKindControls &controls)
{
  if (!controls.wire)
    return read_phys_kind_ext(bs, controls.ext, PHYS_KIND_CTRL_EXT_MAX) >= 0;
  memset(controls.ext, 0, sizeof(controls.ext));
  return read_phys_kind_wire(bs, *controls.wire, controls.ext, PhysKindLod::Num);
}

void PhysKindControls::serialize(danet::BitStream &bs) const
{
  bs.Write(producedAtTick);
  bs.Write(sequenceNumber);
  bs.Write(unitVersion);
  write_controls_ext(bs, *this);
}

bool PhysKindControls::deserialize(const danet::BitStream &bs)
{
  isForged = false;
  return bs.Read(producedAtTick) && producedAtTick >= 0 && bs.Read(sequenceNumber) && bs.Read(unitVersion) &&
         read_controls_ext(bs, *this);
}

void PhysKindControls::snapToWire()
{
  if (!wire)
    return;
  danet::BitStream bs(framemem_ptr());
  write_controls_ext(bs, *this);
  read_controls_ext(bs, *this);
}

void PhysKindControls::interpolate(const PhysKindControls &a, const PhysKindControls &b, float k, int sequence_number)
{
  producedAtTick = a.producedAtTick + int(floorf((b.producedAtTick - a.producedAtTick) * k + 0.5f));
  sequenceNumber = sequence_number;
  wire = a.wire;
  memcpy(ext, (k > 0.5f ? b : a).ext, sizeof(ext));
}

void PhysKindControls::serializeMinimalState(danet::BitStream &bs, const PhysKindControls &base_state) const
{
  bs.Write(uint8_t(clamp(base_state.producedAtTick - producedAtTick, 0, int(UCHAR_MAX))));
  write_controls_ext(bs, *this);
}

bool PhysKindControls::deserializeMinimalState(const danet::BitStream &bs, const PhysKindControls &base_state)
{
  uint8_t tickDelta = 0;
  wire = base_state.wire;
  if (!bs.Read(tickDelta) || !read_controls_ext(bs, *this))
    return false;
  producedAtTick = base_state.producedAtTick - tickDelta;
  return true;
}

void PhysKindControls::applyMinimalState(const PhysKindControls &minimal_state) { memcpy(ext, minimal_state.ext, sizeof(ext)); }

void PhysKindControls::reset() { memset(ext, 0, sizeof(ext)); }
