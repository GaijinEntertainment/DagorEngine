//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <stdint.h>
#include <dag/dag_vector.h>

namespace build_bvh
{

typedef uint16_t bvh_float;

struct BLASDataInfo
{
  int start;     // byte offset of BLAS tree in BLAS buffer (BLAS_IS_BOX for pure boxes)
  uint32_t size; // BLAS tree data size in bytes; 0 = pure box (no BLAS traversal)
  bool isBox() const { return size == 0; }
};

// Per-model arrays indexed by modelId:
//   blasData[m]  - BLAS offset and size; blasData[m].size==0 means pure box
//   blasBoxes[m] - local-space AABB of the model
//   boxDist[m]   - distance threshold for treat-as-box shadow optimization;
//                   must be 0 for pure boxes (blasData[m].isBox()), >=1 for models with BLAS data.
//
// Box leaves (blasData[m].isBox()) are written as TLASLeaf::BOX_SIZE bytes (no blasStart/blasSize),
// non-box leaves are written as sizeof(TLASLeaf) bytes. Allocation must account for both.
// After build, numBoxLeaves contains the count of box leaves written.
struct WriteTLASTreeInfo
{
  const bbox3f *nodes = nullptr;
  uint8_t *bboxData = nullptr;
  const mat43f *tms = nullptr;
  const BLASDataInfo *blasData = nullptr;
  const bbox3f *blasBoxes = nullptr;
  const uint16_t *boxDist = nullptr;
  dag::Vector<int> *tlasLeavesOffsets = nullptr;
  uint32_t leafExtraSize = 0; // extra bytes per leaf beyond base leaf size; caller fills after build
  vec4f scale, ofs;
  uint32_t nodesOffset = 0, nodesSize = 0;
  uint32_t leavesOffset = 0, leavesSize = 0;
  uint32_t numBoxLeaves = 0; // output: number of box leaves written
  bool useHalves = true;     // true=FP16, false=UINT16
};

struct TLASNode
{
  uint32_t minMax[3];
  uint32_t skip;
};

// GPU layout (byte offsets): origin(0) scale(12) invTm(24) maxScale|distToTreatAsBox(48) [blasStart(52) blasSize(56)]
// Pure boxes (distToTreatAsBox==0): only BOX_SIZE=52 bytes written, blasStart/blasSize omitted.
// Non-boxes: full sizeof(TLASLeaf)=60 bytes. GPU defers reading blasStart/blasSize until needed.
struct TLASLeaf
{
  float origin[3];
  float scale[3];
  float invTm[6]; // col0[3], col1[3] of inverse rotation
  bvh_float maxScale;
  uint16_t distToTreatAsBox; // 0 = pure box (no BLAS data); >=1 = has BLAS
  uint32_t blasStart;        // byte offset of BLAS tree in BLAS buffer
  uint32_t blasSize;         // BLAS tree data size in bytes (loop bound for traversal)

  static constexpr uint32_t BOX_SIZE = 13 * 4; // leaf size for pure boxes (up to and including distToTreatAsBox)
};

void writeTLASTreeRoot(WriteTLASTreeInfo &info);

} // namespace build_bvh
