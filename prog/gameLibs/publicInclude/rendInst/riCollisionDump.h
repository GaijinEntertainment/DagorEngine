//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <limits.h>
#include <ioSys/dag_genIo.h>
#include <math/dag_Point3.h>
#include <vecmath/dag_vecMathDecl.h>

// Magic identifying a versioned collision dump. The MSB is set so it can never be
// mistaken for the legacy leading pool-count hint (always a small positive int):
// a legacy dump carries no magic and starts straight with that hint, so a reader
// distinguishes the two by the first word. Keep in sync with the writer in
// rendinst::dumpAllCollisions (prog/gameLibs/rendInst/dumpCollisions.cpp).
static constexpr uint32_t RI_COLLISION_DUMP_MAGIC = 0xD0C0B1A5u;
static constexpr int RI_COLLISION_DUMP_VERSION = 1;

// Reader for the collision dump written by rendinst::dumpAllCollisions
// (prog/gameLibs/rendInst/dumpCollisions.cpp). New dumps lead with
//   [MAGIC][version][poolCountHint]
// legacy dumps lead with [poolCountHint] only (no magic). Either way the body is
// per-record blocks:
//   [indexCount][indices][vertCount][verts(Point3)][instCount][insts(mat43f)]
// indexCount < 0 marks the wide form (uint32 indices, used when a mesh has
// >65535 verts); otherwise indices are 16-bit. This walk owns the layout, the
// sign convention and the bounds/index checks that keep a truncated or hostile
// dump from over-reading; the handler only decides what to keep and where it goes.
// Returns false on any truncated or malformed record (and on an unknown magic or
// version): the caller must reject the load rather than keep a partial scene.
//
// Handler concept (a record may be skipped whole, or kept with parts skipped):
//   bool    wantMesh(int recordIdx, bool wide);   // false => skip the whole record
//   void *  indexBuffer(int count, bool wide);    // dest for count native-width indices
//   Point3 *vertexBuffer(int count);              // dest for count verts
//   mat43f *instanceBuffer(int count);            // dest, or null to skip instance data
//   void    endMesh(int recordIdx, bool wide, int indexCount, int vertCount, int instCount);
// The buffer methods are called only for kept records; each returns storage the
// reader fills directly (no intermediate copy).
template <class Handler>
bool read_ri_collision_dump(IGenLoad &cb, Handler &h)
{
  const int64_t dataSize = cb.getTargetDataSize();
  if (dataSize <= 0)
    return true; // nothing to read
  // Both writers lead with at least one 4-byte word, so a shorter file is torn, not empty; and
  // IGenLoad tell/seek are 32-bit, so a >2 GiB dump cannot be framed safely (all narrowing casts
  // below are bounded by dataSize).
  if (dataSize < (int64_t)sizeof(int) || dataSize > (int64_t)INT_MAX)
    return false;

  const uint32_t firstWord = (uint32_t)cb.readInt();
  if (firstWord == RI_COLLISION_DUMP_MAGIC)
  {
    if (dataSize - cb.tell() < 2 * (int64_t)sizeof(int))
      return false; // truncated header
    if (cb.readInt() != RI_COLLISION_DUMP_VERSION)
      return false; // unknown version
    cb.readInt();   // pool-count hint, advisory only
  }
  else if (firstWord & 0x80000000u)
    return false; // MSB set but not our magic: corrupt or unknown format
  // else: legacy dump, firstWord was the advisory pool-count hint (discarded)

  for (int recordIdx = 0; cb.tell() < dataSize; ++recordIdx)
  {
    if (dataSize - cb.tell() < (int64_t)sizeof(int))
      return false; // 1-3 byte tail: torn mid-scalar, not a clean end of stream
    const int indexCount = cb.readInt();
    const bool wide = indexCount < 0;
    const int64_t idxNum = wide ? -(int64_t)indexCount : (int64_t)indexCount; // negate in 64-bit: INT_MIN-safe
    // Writer invariants that hold for skipped records too: never an empty record, always whole
    // faces. A violation is corruption regardless of wantMesh, so reject before deciding to keep.
    if (idxNum == 0 || (idxNum % 3) != 0)
      return false;
    const int64_t idxBytes = idxNum * (wide ? (int)sizeof(uint32_t) : (int)sizeof(uint16_t));
    if (idxBytes > dataSize - cb.tell())
      return false; // truncated index block
    const bool keep = h.wantMesh(recordIdx, wide);
    void *idxDst = keep ? h.indexBuffer((int)idxNum, wide) : nullptr;
    if (idxDst)
      cb.read(idxDst, (int)idxBytes);
    else
      cb.seekrel((int)idxBytes);

    if (dataSize - cb.tell() < (int64_t)sizeof(int))
      return false; // truncated before the vertex count
    const int vertCount = cb.readInt();
    if (vertCount <= 0 || (int64_t)vertCount * (int)sizeof(Point3) > dataSize - cb.tell())
      return false; // truncated vertex block (0 verts: see the empty-record gate above)

    // Indices precede vertCount in the file, so validate values only now: a kept record feeds
    // RenderSWRT::buildBLAS, whose leafOrderVertexFetch indexes the vertex array directly, so an
    // out-of-range index would over-read. Kept records only -- a skipped block was seeked past.
    if (idxDst)
    {
      if (wide)
      {
        const uint32_t *ix = (const uint32_t *)idxDst;
        for (int64_t k = 0; k < idxNum; ++k)
          if (ix[k] >= (uint32_t)vertCount)
            return false;
      }
      else
      {
        const uint16_t *ix = (const uint16_t *)idxDst;
        for (int64_t k = 0; k < idxNum; ++k)
          if (ix[k] >= vertCount)
            return false;
      }
    }

    if (Point3 *dst = keep ? h.vertexBuffer(vertCount) : nullptr)
      cb.read(dst, vertCount * (int)sizeof(Point3));
    else
      cb.seekrel(vertCount * (int)sizeof(Point3));

    if (dataSize - cb.tell() < (int64_t)sizeof(int))
      return false; // truncated before the instance count
    const int instCount = cb.readInt();
    if (instCount < 0 || (int64_t)instCount * (int)sizeof(mat43f) > dataSize - cb.tell())
      return false; // truncated instance block
    if (mat43f *dst = keep ? h.instanceBuffer(instCount) : nullptr)
      cb.read(dst, instCount * (int)sizeof(mat43f));
    else
      cb.seekrel(instCount * (int)sizeof(mat43f));

    if (keep)
      h.endMesh(recordIdx, wide, (int)idxNum, vertCount, instCount);
  }
  return true;
}
