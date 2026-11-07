//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vectorSet.h>
#include <EASTL/tuple.h>
#include <generic/dag_span.h>
#include <util/dag_stdint.h>

class GlobalVertexData;


namespace shader_meshlets
{
/// Use 'graphics/enableMeshlets' config key to activate feature on supported platforms
bool is_enabled();

/// Meshlets of all elements of one GlobalVertexData, in one midmem block.
struct GlobalMeshletData
{
  /// Allocates a new block; free it with destroy(), not delete.
  static GlobalMeshletData *create();
  static void destroy(GlobalMeshletData *d);

private:
  GlobalMeshletData() = default;
};

/// Indexed RElem range of a resource, in vdata-local coordinates
/// shaderMatVdataIndex is the index in the resource smvd list, vdataIndex is the index in that ShaderMatVdata
struct VdataElemRange
{
  uint16_t shaderMatVdataIndex, vdataIndex;
  uint32_t startIndex, primitiveCount;
  int32_t baseVertex;

  bool operator<(const VdataElemRange &b) const
  {
    return eastl::tie(shaderMatVdataIndex, vdataIndex, startIndex, primitiveCount, baseVertex) <
           eastl::tie(b.shaderMatVdataIndex, b.vdataIndex, b.startIndex, b.primitiveCount, b.baseVertex);
  }
};

/// Non-owning view of a VdataElemSet
class VdataElemSpan
{
public:
  explicit VdataElemSpan(dag::ConstSpan<VdataElemRange> e, int smvd_idx = -1, int vd_idx = -1) :
    elems(e), selectedSmvdIndex(smvd_idx), selectedVdataIndex(vd_idx)
  {}

  VdataElemSpan forShaderMatVdataIndex(int smvd_idx) const;
  VdataElemSpan forVdataIndex(int vd_idx) const;

  bool isVdataSelected() const { return selectedSmvdIndex >= 0 && selectedVdataIndex >= 0; }
  dag::ConstSpan<VdataElemRange> getElems() const { return elems; }
  bool empty() const { return elems.empty(); }

private:
  dag::ConstSpan<VdataElemRange> elems;
  int selectedSmvdIndex = -1;
  int selectedVdataIndex = -1;
};

using VdataElemSet = dag::VectorSet<VdataElemRange>;

/// Builds meshlets of vd_elems (all of one vdata, from forVdataIndex()) from the unpacked VB and 16-bit IB of vd.
/// The caller owns the result.
GlobalMeshletData *build(const GlobalVertexData &vd, VdataElemSpan vd_elems, const uint8_t *vb, const uint16_t *ib);

} // namespace shader_meshlets
