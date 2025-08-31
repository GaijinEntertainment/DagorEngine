// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "pointCloud.h"
#include <3d/dag_materialData.h>
#include <libTools/dagFileRW/dagFileExport.h>

namespace plod
{

class PointCloudDagExporter : public DagSaver
{
public:
  PointCloudDagExporter() = default;
  PointCloudDagExporter(const PointCloudDagExporter &) = delete;
  PointCloudDagExporter(PointCloudDagExporter &&) = default;
  PointCloudDagExporter &operator=(const PointCloudDagExporter &) = delete;
  PointCloudDagExporter &operator=(PointCloudDagExporter &&) = default;
  ~PointCloudDagExporter() override = default;

  void exportPointClouds(const char *file_name, dag::Span<PointCloud> clouds, const char *cloud_name);

private:
  void savePointCloudNodes(dag::Span<PointCloud> clouds, const char *cloud_name);
};

} // namespace plod
