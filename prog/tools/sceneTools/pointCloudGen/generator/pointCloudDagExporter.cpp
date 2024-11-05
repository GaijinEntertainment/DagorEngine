// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pointCloudDagExporter.h"
#include <EASTL/fixed_string.h>
#include <libTools/dagFileRW/sceneImpIface.h>

namespace plod
{

static DagExpMater getDagMaterial(const MaterialData &material_data)
{
  DagExpMater material;
  material.name = material_data.matName;
  material.clsname = material_data.className;
  material.script = material_data.matScript;

  material.amb = e3dcolor(material_data.mat.amb);
  material.diff = e3dcolor(material_data.mat.diff);
  material.spec = e3dcolor(material_data.mat.spec);
  material.emis = e3dcolor(material_data.mat.emis);
  material.power = material_data.mat.power;

  memset(material.texid, 0xFF, sizeof(material.texid));
  return material;
}

static Tab<DagBigMapChannel> getMaterialChannels(PointCloud &cloud)
{
  Tab<DagBigMapChannel> mapChannel(tmpmem);
  for (int channelIdx = 0; channelIdx < cloud.material.size(); ++channelIdx)
  {
    if (!cloud.material[channelIdx].size())
      continue;

    auto &channel = mapChannel.push_back({});
    channel.numtv = cloud.material.front().size();
    channel.num_tv_coords = 1;
    channel.channel_id = channelIdx + 1;
    channel.tverts = reinterpret_cast<float *>(cloud.material[channelIdx].data());
    channel.tface = nullptr;
  }
  return mapChannel;
}

static const char *getNodeScript() { return "point_cloud:b=yes\n\t"; }

void PointCloudDagExporter::exportPointClouds(const char *file_name, dag::Span<PointCloud> clouds, const char *cloud_name)
{
  start_save_dag(file_name);
  for (const auto &cloud : clouds)
  {
    auto dagMaterial = getDagMaterial(cloud.materialData);
    save_mater(dagMaterial);
  }
  savePointCloudNodes(clouds, cloud_name);
  end_save_dag();
}

void PointCloudDagExporter::savePointCloudNodes(dag::Span<PointCloud> clouds, const char *cloud_name)
{
  start_save_nodes();
  for (int i = 0; i < clouds.size(); ++i)
  {
    eastl::fixed_string<char, 128> nodeName;
    nodeName.sprintf("%s_elem%d", cloud_name, i);
    start_save_node(nodeName.c_str(), TMatrix::IDENT, IMP_NF_RENDERABLE | IMP_NF_POINTCLOUD);

    Tab<uint16_t> nodeMaterials(tmpmem);
    nodeMaterials.push_back(i);
    save_node_mats(nodeMaterials.size(), nodeMaterials.data());

    eastl::fixed_string<char, 128> nodeScript = getNodeScript();
    save_node_script(nodeScript.c_str());
    ;

    const auto materialChannels = getMaterialChannels(clouds[i]);
    Tab<DagBigTFace> tFace(tmpmem);
    tFace.resize(1);
    save_dag_bigmesh(clouds[i].positions.size(), clouds[i].positions.data(), 0, nullptr, materialChannels.size(),
      materialChannels.data(), nullptr, tFace.data(), clouds[i].normals.data(), clouds[i].normals.size());

    end_save_node();
  }
  end_save_nodes();
}

} // namespace plod
