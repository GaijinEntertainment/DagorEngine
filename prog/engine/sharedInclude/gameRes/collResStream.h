// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameRes/dag_collisionResource.h>

// The persisted records of the collision resource stream (label COLLRES_STREAM_MAGIC |
// COLLRES_STREAM_VERSION): explicit little-endian structs with no implicit padding, content only.
// Body = the header, the bind records, the Data arrays in ArrayId order, then optional tail records.

inline constexpr uint32_t COLLRES_STREAM_MAGIC = CollisionGameResClassId;
inline constexpr uint32_t COLLRES_STREAM_MAGIC_MASK = 0xFFFF0000u;
inline constexpr uint32_t COLLRES_STREAM_VERSION_MASK = 0xFFFFu;
inline constexpr uint32_t COLLRES_STREAM_VERSION = 3;
inline constexpr uint32_t COLLRES_STREAM_ARRAYS = 11; // Data's ArrayIds before NODE_ORDER, pinned by the loader
inline constexpr uint64_t MAX_COLLRES_BLOCK_BYTES = 256u << 20;

struct CollResStreamHeader
{
  float boundingSphere[4];  // center | r^2
  float bindTraceSphere[4]; // the resource sphere widened over the composed frames | r^2
  // The resource box, once: CollisionResource keeps it as both a BBox3 and a bbox3f, and the two are
  // one value kept in lockstep, so the load derives the vector form from this and the wire cannot
  // carry two boxes that disagree.
  BBox3 boundingBox;
  float boundingSphereRad;
  uint32_t collisionFlags;                // HAS_*_FRT never set
  uint32_t counts[COLLRES_STREAM_ARRAYS]; // in-memory element counts, ArrayId order; TLAS is 0
};
static_assert(sizeof(CollResStreamHeader) == 108, "the stream header is 108 little-endian bytes");

// The default instance's PoseMeta at bind. The loader masks the bits it takes and re-derives the rest.
struct CollResBindRec
{
  float maxTmScale;
  uint8_t flags;  // transform-class bits
  uint8_t status; // TRACEABLE | GEOMETRY_BAKED | RETAINED_BAKE | COMPOSABLE
  uint16_t behaviorFlags;
};
static_assert(sizeof(CollResBindRec) == 8, "the bind record is 8 little-endian bytes");

// A CollisionNode without its re-derived fields (nodeIndex, nodeBlasOfs, verticesCount, the TLAS
// leaf, geomNodeId).
struct CollResNodeRec
{
  uint32_t nameOfs;      // into NAMES; 0 = the empty name
  uint32_t indicesCount; // emitted faces * 3; 0 = no geometry, no chunk record
  uint16_t behaviorFlags;
  int16_t physMatId;           // >= 0 a MAT_NAMES index; -1 none; <= -2 a pool slice start (-id - 2)
  uint16_t insideOfNode;       // INVALID_IDX or a node index
  uint16_t capsuleOrPlanesOfs; // CAPSULE: into CAPSULES; CONVEX: start in CONVEX_PLANES; else 0
  uint16_t planesCount;        // CONVEX only
  uint8_t flags;               // the class bits and TRACE_TWO_SIDED
  uint8_t type;                // CollisionResourceNodeType
  BBox3 modelBBox;
  float radiusAroundBoxCenter; // < 0: the exporter's zero-vert marker
};
static_assert(sizeof(CollResNodeRec) == 48, "the node record is 48 little-endian bytes");

// One per node with indicesCount != 0, in node order: the first three fields of NodeBlasChunkHeader,
// then the chunk as a bvhIO stream (its tree re-emitted into the block at load).
struct CollResChunkFrame
{
  float scale[3], invScale[3], bmin[3];
};
static_assert(sizeof(CollResChunkFrame) == 36 && offsetof(CollisionResource::NodeBlasChunkHeader, treeBytes) == 36,
  "the chunk frame is the header's leading 36 bytes");

// A tail record after the arrays: a loader that does not know the tag skips its bytes, so an
// addition here is not a version change. No writer emits one yet.
struct CollResTailRec
{
  uint32_t tag;   // never 0: a zero word past the arrays is a corrupt body, not a record
  uint32_t bytes; // of the payload that follows
};
static_assert(sizeof(CollResTailRec) == 8, "the tail record is 8 little-endian bytes");

// The wire layouts, field by field. A size assert alone cannot see a same-size field swap, and the
// suite's fixtures are emitted through these very structs, so they would move with the loader and
// stay green. The layout is frozen: a change to it is a new version.
static_assert(offsetof(CollResStreamHeader, boundingSphere) == 0 && offsetof(CollResStreamHeader, bindTraceSphere) == 16 &&
                offsetof(CollResStreamHeader, boundingBox) == 32 && offsetof(CollResStreamHeader, boundingSphereRad) == 56 &&
                offsetof(CollResStreamHeader, collisionFlags) == 60 && offsetof(CollResStreamHeader, counts) == 64,
  "the header field order is the wire order");
static_assert(offsetof(CollResNodeRec, nameOfs) == 0 && offsetof(CollResNodeRec, indicesCount) == 4 &&
                offsetof(CollResNodeRec, behaviorFlags) == 8 && offsetof(CollResNodeRec, physMatId) == 10 &&
                offsetof(CollResNodeRec, insideOfNode) == 12 && offsetof(CollResNodeRec, capsuleOrPlanesOfs) == 14 &&
                offsetof(CollResNodeRec, planesCount) == 16 && offsetof(CollResNodeRec, flags) == 18 &&
                offsetof(CollResNodeRec, type) == 19 && offsetof(CollResNodeRec, modelBBox) == 20 &&
                offsetof(CollResNodeRec, radiusAroundBoxCenter) == 44,
  "the node record field order is the wire order");
