// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "pointCloudDagExporter.h"
#include "spatialGrid.h"
#include "sampler.h"
#include <3d/dag_resPtr.h>
#include <3d/dag_materialData.h>
#include <assets/asset.h>
#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_shaderMesh.h>
#include <EASTL/fixed_string.h>
#include <shaders/dag_overrideStateId.h>

class DagorAssetMgr;
class ILogWriter;
class ShaderMesh;

namespace plod
{

using String = eastl::fixed_string<char, 128>;

using RawPointCloud = Tab<RawVertex>;

struct RawPointCloudLods
{
  Tab<size_t> lodPointCounts;
  RawPointCloud points;
};

struct ProcessedPointCloud
{
  BBox3 bbox;
  Tab<ProcessedVertex> points;
  MaterialData materialData;
};

struct ProcessedPointCloudLods
{
  Tab<size_t> lodPointCounts;
  ProcessedPointCloud cloud;
};

struct ShaderChannel
{
  uint16_t usage;
  uint16_t usageIndex;
  uint32_t stride;
  uint32_t offset;
  int type;
  int mod;
};

class PointCloudGenerator
{
public:
  PointCloudGenerator(const char *app_dir, DataBlock &app_blk, DagorAssetMgr *asset_manager, ILogWriter *log_writer);
  PointCloudGenerator(const PointCloudGenerator &) = delete;
  PointCloudGenerator(PointCloudGenerator &&) = default;
  PointCloudGenerator &operator=(const PointCloudGenerator &) = delete;
  PointCloudGenerator &operator=(PointCloudGenerator &&) = default;
  virtual ~PointCloudGenerator() = default;

  bool isInitialized() const { return initialized; }
  bool isAssetSupported(DagorAsset *asset);
  void generate(DagorAsset *asset);

protected:
  plod::String appDir;
  DataBlock appBlk;
  DagorAssetMgr *assetMgr;
  ILogWriter *log;

private:
  void exportToDag(const char *path, dag::ConstSpan<ProcessedPointCloudLods> cloudElems, const char *name);
  Tab<ProcessedPointCloudLods> generateMaterialPointCloudsFromRI(const RenderableInstanceLodsResource &ri_res);
  RawPointCloudLods generatePointCloudFromMesh(const ShaderMesh &mesh, const BBox3 &bbox, int elem_idx, int mesh_face_count,
    int total_count);
  ProcessedPointCloud collectMaterial(const ShaderMesh &mesh, const RawPointCloud &cloud, int elem_idx);
  SpatialGrid samplePointsOnMesh(const ShaderMesh &mesh, const BBox3 &bbox, int elem_idx, int mesh_face_count, int total_count,
    float &init_density);

  PointCloudDagExporter exporter;
  UniqueBuf pointVB;
  UniqueBuf materialBuffer;
  UniqueBuf instanceBuffer;
  shaders::OverrideStateId overrideStateId;
  bool initialized;
};

} // namespace plod
