// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdint.h>
#include <float.h>
#include <math/dag_Point3.h>
#include <vecmath/dag_vecMath.h>
#include <generic/dag_span.h>
#include <util/dag_compilerDefs.h>
#include <gamePhys/collision/collisionType.h>

namespace dacoll
{

static constexpr uint16_t BUNDLE_NO_MATERIAL = 0x7fff;      // a face with no phys material: the consumer maps it to its default
static constexpr uint32_t BUNDLE_TERRAIN_OBJECT = 0;        // the ground's; an instance key is an odd 32-bit hash and can collide
static constexpr int BUNDLE_FLAGS = ETF_RI | ETF_HEIGHTMAP; // static geometry, and the ground is the heightfield alone

// dacoll owns these records; a consumer whose structs have the same shape hands its own arrays over and asserts the layout.
// DAGOR_MAY_ALIAS is what makes that handoff legal under type-based alias analysis.
struct DAGOR_MAY_ALIAS BundleSegment
{
  Point3 from, to;
};

#pragma pack(push, 1)
struct DAGOR_MAY_ALIAS BundleHit // ten bytes: a bundle streams tens of thousands, so the facing is a bit of the material word
{
  static constexpr uint16_t ENTER_BIT = 0x8000, MATERIAL_MASK = 0x7fff;
  float t = 0.f; // metres from the segment start; a negative t is an enter the segment starts inside, read as distance 0
  uint32_t objectId = 0;
  uint16_t materialFacing = ENTER_BIT | BUNDLE_NO_MATERIAL;

  uint16_t material() const { return materialFacing & MATERIAL_MASK; }
  void setMaterial(uint16_t m) { materialFacing = uint16_t((materialFacing & ENTER_BIT) | (m & MATERIAL_MASK)); }
  int facing() const { return (materialFacing & ENTER_BIT) ? 1 : -1; } // +1 into solid (dot(outward normal, dir) < 0), -1 out of it
  void setFacing(int f) { materialFacing = uint16_t((f > 0 ? ENTER_BIT : 0) | (materialFacing & MATERIAL_MASK)); }
};
#pragma pack(pop)

enum BundleEndpointState : uint8_t
{
  BUNDLE_ENDPOINTS_UNKNOWN = 0,
  BUNDLE_ENDPOINTS_KNOWN = 1,
  BUNDLE_ENDPOINT_FROM_INSIDE = 2,
  BUNDLE_ENDPOINT_TO_INSIDE = 4
};

struct DAGOR_MAY_ALIAS BundleRange // where one segment's hits sit in the shared hit buffer of a bundle
{
  uint32_t start = 0;
  uint16_t count = 0;
  uint8_t dropped = 0;                          // hits past the cap, saturated: every reader tests it for zero
  uint8_t endpoints = BUNDLE_ENDPOINTS_UNKNOWN; // BundleEndpointState bits for the flags walked; an inside bit counts only under KNOWN
  float cutoff = FLT_MAX;                       // metres at which the list stops being complete
};
static_assert(sizeof(BundleHit) == 10 && sizeof(BundleRange) == 12, "the record layout is part of the contract");

// Hits arrive in groups and in no order, so a cap reached inside one group keeps hits past the nearest one it lost.
struct BundleHitSink
{
  BundleHit *hits = nullptr;
  uint32_t capacity = 0;
  uint32_t count = 0;
  uint32_t dropped = 0;
  float cutoff = FLT_MAX;

  void add(float t, int phys_mat_id, uint32_t object_id, int8_t facing)
  {
    if (count >= capacity)
    {
      dropped++;
      const float at = t > 0.f ? t : 0.f; // a negative t is a start inside solid, not a distance
      cutoff = at < cutoff ? at : cutoff;
      return;
    }
    BundleHit &h = hits[count++];
    h.t = t;
    h.objectId = object_id;
    const bool known = phys_mat_id >= 0 && phys_mat_id < int(BUNDLE_NO_MATERIAL); // past the mask it would spill into the facing bit
    h.materialFacing = uint16_t((facing > 0 ? BundleHit::ENTER_BIT : 0) | (known ? uint16_t(phys_mat_id) : BUNDLE_NO_MATERIAL));
  }
};

inline int8_t bundle_hit_facing(const Point3 &norm, const Point3 &dir) { return norm * dir < 0.f ? 1 : -1; }

enum class BundleEndpointSide : uint8_t
{
  Outside,
  Inside,
  Unresolved // the segment crossed no surface of that object
};

// A bound is not the solid: the object's own hits settle the side, the nearest for the from end and the farthest for the
// to end. One closed mesh at a time, so this is no parity count over overlapping solids.
inline BundleEndpointSide bundle_endpoint_side(dag::ConstSpan<BundleHit> hits, uint32_t object_id, bool from_end)
{
  const BundleHit *edge = nullptr;
  for (const BundleHit &h : hits)
  {
    if (h.objectId != object_id)
      continue;
    if (from_end && h.t < 0.f)
      return BundleEndpointSide::Inside;
    if (!edge || (from_end ? h.t < edge->t : h.t > edge->t))
      edge = &h;
  }
  if (!edge)
    return BundleEndpointSide::Unresolved;
  return (from_end ? edge->facing() < 0 : edge->facing() > 0) ? BundleEndpointSide::Inside : BundleEndpointSide::Outside;
}

// The probe ends outside the bound, so no hit of the object means outside.
inline BundleEndpointSide bundle_probe_side(dag::ConstSpan<BundleHit> probe_hits, uint32_t object_id)
{
  const BundleEndpointSide side = bundle_endpoint_side(probe_hits, object_id, /*from_end*/ true);
  return side == BundleEndpointSide::Unresolved ? BundleEndpointSide::Outside : side;
}

// A point just past the nearest face of a bound: a point inside the object's mesh has to cross it to get there.
inline vec3f bundle_bound_exit(bbox3f_cref box, vec3f p)
{
  const vec4f toMin = v_sub(p, box.bmin), toMax = v_sub(box.bmax, p);
  const vec4f dist = v_min(toMin, toMax);
  const vec4f step = v_add(v_max(dist, v_zero()), v_mul(v_max(v_sub(box.bmax, box.bmin), V_C_ONE), v_splats(0.01f)));
  const vec4f away = v_sel(step, v_neg(step), v_cmp_lt(toMin, toMax));
  return v_add(p, v_and(v_cmp_eq(dist, v_hmin3(dist)), away));
}

// Folds one bound that holds an endpoint into a segment's evidence; evidence without BUNDLE_ENDPOINTS_KNOWN is left as it is.
// A segment that crossed no face of the object never changed side, so an end outside the bound puts both ends outside;
// only a bound holding the whole segment asks contain_object, whose Unresolved drops the evidence for good: it must not read as air.
template <typename ContainmentFn>
inline void bundle_certify_endpoints(uint32_t &endpoints, dag::ConstSpan<BundleHit> hits, uint32_t object_id, bool contains_from,
  bool contains_to, const ContainmentFn &contain_object)
{
  if (!(endpoints & BUNDLE_ENDPOINTS_KNOWN))
    return;
  const BundleEndpointSide fromSide = bundle_endpoint_side(hits, object_id, /*from_end*/ true);
  if (fromSide == BundleEndpointSide::Unresolved)
  {
    if (!contains_from || !contains_to)
      return;
    const BundleEndpointSide side = contain_object(object_id);
    if (side == BundleEndpointSide::Inside)
      endpoints |= BUNDLE_ENDPOINT_FROM_INSIDE | BUNDLE_ENDPOINT_TO_INSIDE;
    else if (side == BundleEndpointSide::Unresolved)
      endpoints = BUNDLE_ENDPOINTS_UNKNOWN;
    return;
  }
  if (contains_from && fromSide == BundleEndpointSide::Inside)
    endpoints |= BUNDLE_ENDPOINT_FROM_INSIDE;
  if (contains_to && bundle_endpoint_side(hits, object_id, /*from_end*/ false) == BundleEndpointSide::Inside)
    endpoints |= BUNDLE_ENDPOINT_TO_INSIDE;
}

static constexpr uint32_t BUNDLE_HITS_PER_SEGMENT = 256;

// Every face crossing of every segment, unsorted, with a range per segment in order and without gaps: each range holds
// its segment's hits, at most hits_per_segment and fewer where out_hits runs short, the rest counted as dropped.
// Returns the hits written, and 0 with nothing written unless out_ranges has a slot per segment and hits_per_segment is
// above zero. Only ETF_RI and ETF_HEIGHTMAP are read, and ray_mat_id -1 keeps every instance. A segment with from == to
// keeps its slot with an empty range; it and a segment that dropped a hit keep unknown endpoints. certify_endpoints costs
// a box and two point tests per ray and visited instance, so a caller that does not read it does not pay. Safe from worker threads.
int trace_bundle_all_hits(dag::ConstSpan<BundleSegment> segments, int flags, int ray_mat_id, dag::Span<BundleHit> out_hits,
  dag::Span<BundleRange> out_ranges, uint32_t hits_per_segment = BUNDLE_HITS_PER_SEGMENT, bool certify_endpoints = false);

// Whether a face lies between the ends, over the geometry trace_bundle_all_hits walks and no other: rayhit reads the box,
// sphere and capsule nodes the walk drops. A segment inside one solid crosses nothing and answers false, except one at
// least a micrometre long under the ground, where one height read settles it. Safe from worker threads.
bool bundle_segment_any_hit(const Point3 &from, const Point3 &to, int flags, int ray_mat_id);

} // namespace dacoll
