// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pointCloudGen.h"
#include "material.h"
#include "morton.h"
#include "packUtil.h"
#include <3d/dag_lockSbuffer.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_info.h>
#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/riShaderConstBuffers.h>
#include <algorithm>
#include <random>
#include <sampler.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/shUtils.h>
#include <shaders/dag_overrideStates.h>
#include <libTools/util/iLogWriter.h>
#include <EASTL/fixed_string.h>

static int plod_output_reg_no = -1;

#define GLOBAL_VARS_LIST           \
  VAR(plod_gen, true)              \
  VAR(rendinst_render_pass, false) \
  VAR(instancing_type, false)      \
  VAR(atest, false)                \
  VAR(small_sampled_buffers, false)

#define VAR(a, optional) static int a##_var_id = -1;
GLOBAL_VARS_LIST
#undef VAR

namespace plod
{

static constexpr int SHUFFLE_GROUP_SIZE = 64;

static constexpr int PERLIN_TEX1_CHANNEL = 3;
static constexpr int PERLIN_TEX2_CHANNEL = 5;

static constexpr int PERLIN_DIFFUSE1_CHANNEL = 1;
static constexpr int PERLIN_DIFFUSE2_CHANNEL = 2;
static constexpr int PERLIN_NORMAL1_CHANNEL = 4;
static constexpr int PERLIN_NORMAL2_CHANNEL = 5;

// Experimentally measured scale when points are faster to render (in case of 1 pixel point).
static constexpr float INIT_DENSITY_SCALE = 6.0f;

struct ExportShader
{
  const char *shaderName;
  const char *exportName;
  uint8_t materialChannels;
};

static constexpr eastl::array<ExportShader, 8> supported_shaders{{
  {"rendinst_simple", "rendinst_plod_simple", SIMPLE_MATERIAL_CHANNELS},
  {"rendinst_simple_painted", "rendinst_plod_simple", SIMPLE_MATERIAL_CHANNELS},
  {"rendinst_simple_colored", "rendinst_plod_simple", SIMPLE_MATERIAL_CHANNELS},
  {"rendinst_vcolor_layered", "rendinst_plod_simple", SIMPLE_MATERIAL_CHANNELS},
  {"rendinst_cliff", "rendinst_plod_simple", SIMPLE_MATERIAL_CHANNELS},
  {"rendinst_layered", "rendinst_plod_simple", SIMPLE_MATERIAL_CHANNELS},
  {"rendinst_mask_layered", "rendinst_plod_simple", SIMPLE_MATERIAL_CHANNELS},
  {"rendinst_perlin_layered", "rendinst_plod_perlin", PERLIN_MATERIAL_CHANNELS},
}};

static ExportShader get_export_shader(const char *shader_name, ILogWriter *log)
{
  for (const auto &shader : supported_shaders)
    if (plod::String(shader_name) == plod::String(shader.shaderName))
      return shader;

  log->addMessage(log->ERROR, "[PLOD] Point cloud is not supported for shader %s", shader_name);
  return {};
}

static bool is_elem_supported(const ShaderMesh::RElem &elem)
{
  for (const auto &shader : supported_shaders)
    if (plod::String(elem.e->getShaderClassName()) == plod::String(shader.shaderName))
      return true;

  return false;
}

static bool is_elem_plod(const ShaderMesh::RElem &elem)
{
  for (const auto &shader : supported_shaders)
    if (plod::String(elem.e->getShaderClassName()) == plod::String(shader.exportName))
      return true;

  return false;
}


static void shuffled_morton_sort(ProcessedPointCloudLods &point_cloud_lods)
{
  const auto min = point_cloud_lods.cloud.bbox.boxMin();
  const auto max = point_cloud_lods.cloud.bbox.boxMax();
  std::random_device rd;
  std::mt19937 randomGen(rd());
  for (int lod = point_cloud_lods.lodPointCounts.size() - 1; lod >= 0; --lod)
  {
    const auto mortonComp = [&](plod::ProcessedVertex a, plod::ProcessedVertex b) {
      return getMortonCode(a.position, min, max) < getMortonCode(b.position, min, max);
    };

    int lodStart = lod < point_cloud_lods.lodPointCounts.size() - 1 ? point_cloud_lods.lodPointCounts[lod + 1] : 0;
    int lodEnd = point_cloud_lods.lodPointCounts[lod];
    eastl::sort(point_cloud_lods.cloud.points.begin() + lodStart, point_cloud_lods.cloud.points.begin() + lodEnd, mortonComp);

    eastl::fixed_vector<eastl::fixed_vector<plod::ProcessedVertex, SHUFFLE_GROUP_SIZE>, 8> groups;

    for (int groupStart = lodStart; groupStart < lodEnd; groupStart += SHUFFLE_GROUP_SIZE)
    {
      int groupEnd = eastl::min(SHUFFLE_GROUP_SIZE, lodEnd - groupStart);
      eastl::fixed_vector<plod::ProcessedVertex, SHUFFLE_GROUP_SIZE> group(point_cloud_lods.cloud.points.begin() + groupStart,
        point_cloud_lods.cloud.points.begin() + groupStart + groupEnd);
      groups.emplace_back(eastl::move(group));
    }
    std::shuffle(groups.begin(), groups.end(), randomGen);

    int groupBegin = 0;
    for (auto &group : groups)
    {
      for (const auto v : group)
        point_cloud_lods.cloud.points[lodStart + groupBegin] = v;

      groupBegin += group.size();
    }
  }
}

static void mergeClouds(ProcessedPointCloudLods &to, const ProcessedPointCloudLods &from)
{
  Tab<ProcessedVertex> mergedPoints{};
  mergedPoints.reserve(from.cloud.points.size() + to.cloud.points.size());
  const auto lodCount = eastl::min(to.lodPointCounts.size(), from.lodPointCounts.size());
  to.lodPointCounts.resize(lodCount);

  for (int lod = lodCount - 1; lod >= 0; --lod)
  {
    int lodStart = lod < lodCount - 1 ? to.lodPointCounts[lod + 1] : 0;
    for (int i = lodStart; i < to.lodPointCounts[lod]; ++i)
      mergedPoints.push_back(to.cloud.points[i]);

    int newElemLodStart = lod < lodCount - 1 ? from.lodPointCounts[lod + 1] : 0;
    for (int i = newElemLodStart; i < from.lodPointCounts[lod]; ++i)
      mergedPoints.push_back(from.cloud.points[i]);
  }
  for (int lod = 0; lod < lodCount; ++lod)
    to.lodPointCounts[lod] += from.lodPointCounts[lod];

  to.cloud.points = eastl::move(mergedPoints);
}

PointCloudGenerator::PointCloudGenerator(const char *app_dir, DataBlock &app_blk, DagorAssetMgr *asset_manager,
  ILogWriter *log_writer) :
  appDir(app_dir), assetMgr(asset_manager), log(log_writer)
{
#define VAR(a, optional) a##_var_id = get_shader_variable_id(#a, optional);
  GLOBAL_VARS_LIST
#undef VAR
  ShaderGlobal::get_int_by_name("per_instance_data_no", rendinst::render::instancingTexRegNo);
  ShaderGlobal::get_int_by_name("plod_output_no", plod_output_reg_no);
  if (plod_output_reg_no == -1 || plod_gen_var_id == -1)
  {
    initialized = false;
    return;
  }
  appBlk.setFrom(&app_blk, app_blk.resolveFilename());
  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_TEST_DISABLE | shaders::OverrideState::CULL_NONE);
  overrideStateId = shaders::overrides::create(state);

  bool useStructuredBuffer = ShaderGlobal::get_interval_current_value(small_sampled_buffers_var_id);

  if (useStructuredBuffer)
    instanceBuffer = dag::buffers::create_persistent_sr_structured(sizeof(TMatrix4), 1, "plod_instancing");
  else
    instanceBuffer =
      dag::buffers::create_persistent_sr_tbuf(sizeof(TMatrix4) / sizeof(float4), TEXFMT_A32B32G32R32F, "plod_instancing");

  instanceBuffer->updateDataWithLock(0, sizeof(TMatrix4), &TMatrix4::IDENT.m, VBLOCK_WRITEONLY);
  initialized = true;
}

bool PointCloudGenerator::isAssetSupported(DagorAsset *asset)
{
  static const auto rendinstTypeId = assetMgr->getAssetTypeId("rendinst");
  return asset->getType() == rendinstTypeId;
}

void PointCloudGenerator::generate(DagorAsset *asset)
{
  G_ASSERT(plod_output_reg_no != -1);
  G_ASSERT(plod_gen_var_id != -1);
  if (!isAssetSupported(asset))
  {
    log->addMessage(ILogWriter::ERROR, "[PLOD] Asset %s is not supported", asset->getName());
    return;
  }
  const auto resIdx = rendinst::addRIGenExtraResIdx(asset->getName(), -1, -1, rendinst::AddRIFlag::UseShadow);
  const auto lodsRes = rendinst::getRIGenExtraRes(resIdx);
  if (lodsRes == nullptr)
  {
    log->addMessage(ILogWriter::ERROR, "[PLOD] Could not load rendInst: %s", asset->getName());
    return;
  }

  log->addMessage(log->NOTE, "[PLOD] Generating point cloud for %s", asset->getName());
  rendinst::updateRiExtraReqLod(resIdx, 0);
  auto pointClouds = generateMaterialPointCloudsFromRI(*lodsRes);
  if (pointClouds.empty())
  {
    log->addMessage(log->NOTE, "[PLOD] Point cloud is empty. Asset does not have supported material");
    return;
  }

  for (auto &cloud : pointClouds)
    shuffled_morton_sort(cloud);

  plod::String path({}, "%s/%s.plod.dag", asset->getFolderPath(), asset->getName());
  log->addMessage(log->NOTE, "[PLOD] Exporting cloud to %s", path);
  exportToDag(path.c_str(), pointClouds, asset->getName());
}

Tab<ProcessedPointCloudLods> PointCloudGenerator::generateMaterialPointCloudsFromRI(const RenderableInstanceLodsResource &ri_res)
{
  const auto &usedLods = ri_res.getUsedLods();
  auto &firstLod = usedLods.front();
  const auto mesh = firstLod.scene->getMesh()->getMesh()->getMesh();
  const auto firstLodFaceCount = firstLod.scene->getMesh()->getMesh()->getMesh()->calcTotalFaces();

  int lastLodFaceCount = 0;
  for (int i = usedLods.size() - 1; i >= 0 && !lastLodFaceCount; i--)
  {
    const auto &lastMesh = usedLods[i].scene->getMesh()->getMesh()->getMesh();
    for (const auto &elem : lastMesh->getAllElems())
    {
      if (!is_elem_plod(elem))
        lastLodFaceCount += elem.numf;
      else
      {
        lastLodFaceCount = 0;
        break;
      }
    }
  }

  auto elemCount = mesh->getAllElems().size();
  Tab<ProcessedPointCloudLods> cloudElems{};
  auto elems = mesh->getElems(0, ShaderMesh::STG_COUNT - 1);
  for (auto elemIdx = 0; elemIdx < elemCount; ++elemIdx)
  {
    if (!is_elem_supported(elems[elemIdx]))
      continue;
    auto elemPointCloudLods = generatePointCloudFromMesh(*mesh, ri_res.bbox, elemIdx, firstLodFaceCount, lastLodFaceCount);
    const ProcessedPointCloudLods materialPointCloudLods = {
      elemPointCloudLods.lodPointCounts, collectMaterial(*mesh, elemPointCloudLods.points, elemIdx)};

    if (materialPointCloudLods.cloud.points.empty())
      continue;

    auto mergeableCloudLods =
      eastl::find_if(cloudElems.begin(), cloudElems.end(), [&materialPointCloudLods](const ProcessedPointCloudLods &cloudLods) {
        return cloudLods.cloud.materialData.isEqual(materialPointCloudLods.cloud.materialData) ||
               (cloudLods.cloud.materialData.className == "rendinst_plod_simple" &&
                 materialPointCloudLods.cloud.materialData.className == "rendinst_plod_simple");
      });

    if (mergeableCloudLods == cloudElems.end())
    {
      mergeableCloudLods = &cloudElems.emplace_back();
      mergeableCloudLods->cloud.bbox = ri_res.bbox;
      mergeableCloudLods->cloud.materialData = eastl::move(MaterialData(materialPointCloudLods.cloud.materialData));
      mergeableCloudLods->lodPointCounts.resize(materialPointCloudLods.lodPointCounts.size(), 0);
    }

    mergeClouds(*mergeableCloudLods, materialPointCloudLods);
  }
  return cloudElems;
}


void PointCloudGenerator::exportToDag(const char *path, dag::ConstSpan<ProcessedPointCloudLods> cloud_elems, const char *name)
{
  Tab<PointCloud> pointCloudElems{};
  for (const auto &cloudElem : cloud_elems)
  {
    PointCloud pointCloud;
    const auto pointCount = cloudElem.cloud.points.size();
    pointCloud.positions.reserve(pointCount);
    pointCloud.normals.reserve(pointCount);
    pointCloud.material.resize(cloudElem.cloud.points.front().material.size());
    for (size_t channel = 0; channel < pointCloud.material.size(); ++channel)
      pointCloud.material[channel].reserve(pointCount);
    for (const auto point : cloudElem.cloud.points)
    {
      pointCloud.positions.push_back(point.position);
      pointCloud.normals.push_back(-point.norm);
      for (size_t channel = 0; channel < pointCloud.material.size(); ++channel)
        if (point.material[channel].val != INVALID_MATERIAL_CHANNEl.val)
          pointCloud.material[channel].push_back(packToE3DCOLOR(point.material[channel].val, point.material[channel].mod));
    }
    pointCloud.materialData = eastl::move(MaterialData(cloudElem.cloud.materialData));
    pointCloudElems.emplace_back(eastl::move(pointCloud));
  }
  exporter.exportPointClouds(path, make_span(pointCloudElems), name);
}

static constexpr size_t CELL_DIM = 128;

SpatialGrid PointCloudGenerator::samplePointsOnMesh(const ShaderMesh &mesh, const BBox3 &bbox, int elem_idx, int mesh_face_count,
  int total_count, float &init_density)
{
  const auto bboxWidth = bbox.width();
  auto samples = SpatialGrid(bbox, eastl::max(eastl::max(bboxWidth.x, bboxWidth.y), bboxWidth.z) / CELL_DIM);
  auto elems = mesh.getElems(0, ShaderMesh::STG_COUNT - 1);
  G_ASSERT_RETURN(elem_idx < elems.size(), samples);
  auto &elem = elems[elem_idx];
  const auto elemFaceCount = elem.numf;
  const int initCount = static_cast<float>(elemFaceCount) / mesh_face_count * total_count;
  const auto vertexData = elem.vertexData;
  auto lockedVB = lock_sbuffer<const uint8_t>(vertexData->getVB(), 0, vertexData->getVbSize(), 0);
  auto VBMem = lockedVB.get();
  auto lockedIB = lock_sbuffer<const uint8_t>(vertexData->getIB(), 0, vertexData->getIbSize(), 0);
  auto IBMem = lockedIB.get();
  const auto indexSize = vertexData->getIbElemSz();
  IBMem += elem.si * indexSize;

  Tab<Polygon> polygons{};
  polygons.reserve(elem.numf);
  for (size_t i = 0; i < elem.numf; ++i)
  {
    Polygon poly{};
    auto face = IBMem + i * indexSize * 3;
    for (size_t j = 0; j < 3; j++)
    {
      uint32_t index = elem.baseVertex;
      if (indexSize == sizeof(uint16_t))
        index += *reinterpret_cast<const uint16_t *>(face + j * indexSize);
      else
        index += *reinterpret_cast<const uint32_t *>(face + j * indexSize);

      for (const auto &channel : elem.e->getChannels())
      {
        uint32_t offset = 0;
        uint32_t stride = 0;
        int type = 0;
        int mod = 0;
        mesh.getVbInfo(elem, channel.u, channel.ui, stride, offset, type, mod);
        switch (channel.u)
        {
          case SCUSAGE_POS:
            G_ASSERT(channel.t == SCTYPE_FLOAT3);
            memcpy(&poly.v[j].position, VBMem + index * stride + offset, sizeof(Point3));
            break;
          case SCUSAGE_NORM:
            G_ASSERT(channel.t == SCTYPE_E3DCOLOR);
            E3DCOLOR norm;
            memcpy(&norm, VBMem + index * stride + offset, sizeof(E3DCOLOR));
            poly.v[j].tbn.col[2] = unpackToPoint3(norm, mod);
            break;
          case SCUSAGE_TC:
            switch (channel.t)
            {
              case SCTYPE_FLOAT2: memcpy(&poly.v[j].tc[channel.ui], VBMem + index * stride + offset, sizeof(Point2)); break;
              case SCTYPE_HALF2:
              {
                Half2 tc{};
                memcpy(&tc, VBMem + index * stride + offset, sizeof(Half2));
                poly.v[j].tc[channel.ui] = tc;
                break;
              }
              case SCTYPE_SHORT2:
              {
                int16_t tc[2];
                memcpy(&tc, VBMem + index * stride + offset, sizeof(int16_t) * 2);
                poly.v[j].tc[channel.ui].x = tc[0];
                poly.v[j].tc[channel.ui].y = tc[1];
                break;
              }
              default: G_ASSERT(0); break;
            }
            break;
          case SCUSAGE_VCOL:
            G_ASSERT(channel.t == SCTYPE_E3DCOLOR);
            memcpy(&poly.v[j].color, VBMem + index * stride + offset, sizeof(E3DCOLOR));
            break;
          default: log->addMessage(log->ERROR, "Unsupported shader channel usage %d", channel.u); break;
        }
      }
    }
    polygons.push_back(eastl::move(poly));
  }
  float surfaceArea = 0;
  for (const auto &poly : polygons)
    surfaceArea += getPolygonSurfaceArea(poly);

  init_density = INIT_DENSITY_SCALE * eastl::max(initCount / surfaceArea, 1);
  for (const auto &poly : polygons)
    samplePointsOnPolygon(samples, poly, eastl::max(eastl::min(init_density * getPolygonSurfaceArea(poly), 128), 16));

  return samples;
}

ProcessedPointCloud PointCloudGenerator::collectMaterial(const ShaderMesh &mesh, const RawPointCloud &cloud, int elem_idx)
{
  d3d::GpuAutoLock gpuLock{};
  auto elems = mesh.getElems(0, ShaderMesh::STG_COUNT - 1);
  G_ASSERT_RETURN(elem_idx < elems.size(), {});
  auto &elem = elems[elem_idx];

  const auto vertexStride = elem.e->getVertexStride();
  const auto exportShader = get_export_shader(elem.e->getShaderClassName(), log);
  const auto channelStride = sizeof(float4);
  const auto materialStride = exportShader.materialChannels * channelStride;

  if (!materialStride)
    return {};

  int atest = 0;
  elem.mat->getIntVariable(atest_var_id, atest);
  if (atest)
  {
    log->addMessage(log->WARNING, "Skipping shader mesh elem%d, because atest is not supported for point clouds.", elem_idx);
    return {};
  }

  materialBuffer = dag::buffers::create_ua_sr_structured(materialStride, cloud.size(), "plod_output");
  pointVB = resptr_detail::ResPtrFactory(d3d::create_vb(cloud.size() * vertexStride, SBCF_BIND_VERTEX, "point_vb"));

  rendinst::render::RiShaderConstBuffers cb;
  cb.setBBoxZero();
  cb.setInstancing(0, 4, 0, 0);

  d3d::settm(TM_VIEW, &TMatrix4::IDENT);
  if (shaders::overrides::get_current() != overrideStateId)
    shaders::overrides::set(overrideStateId);

  dag::Vector<ShaderChannel> channels;

  for (const auto &channel : elem.e->getChannels())
  {
    uint32_t offset = 0;
    uint32_t stride = 0;
    int type = 0;
    int mod = 0;
    mesh.getVbInfo(elem, channel.u, channel.ui, stride, offset, type, mod);
    channels.push_back({channel.u, channel.ui, stride, offset, type, mod});
  }

  Tab<uint8_t> VBData;
  VBData.resize(cloud.size() * vertexStride);
  auto VBMem = VBData.data();

  for (size_t i = 0; i < cloud.size(); ++i)
  {
    auto point = cloud[i];
    for (auto channel : channels)
      switch (channel.usage)
      {
        case SCUSAGE_POS:
          G_ASSERT(channel.type == SCTYPE_FLOAT3);
          memcpy(VBMem + i * channel.stride + channel.offset, static_cast<void *>(&point.position), sizeof(Point3));
          break;
        case SCUSAGE_NORM:
        {
          G_ASSERT(channel.type == SCTYPE_E3DCOLOR);
          E3DCOLOR norm = packToE3DCOLOR(point.tbn.getcol(2), channel.mod);
          memcpy(VBMem + i * channel.stride + channel.offset, static_cast<void *>(&norm), sizeof(E3DCOLOR));
          break;
        }
        case SCUSAGE_TC:
          switch (channel.type)
          {
            case SCTYPE_FLOAT2:
              memcpy(VBMem + i * channel.stride + channel.offset, static_cast<void *>(&point.tc[channel.usageIndex]), sizeof(Point2));
              break;
            case SCTYPE_HALF2:
            {
              Half2 tc = point.tc[channel.usageIndex];
              memcpy(VBMem + i * channel.stride + channel.offset, static_cast<void *>(&tc), sizeof(Half2));
              break;
            }
            case SCTYPE_SHORT2:
            {
              int16_t tc[2];
              tc[0] = clamp(real2int(point.tc[channel.usageIndex].x), -32768, 32767);
              tc[1] = clamp(real2int(point.tc[channel.usageIndex].y), -32768, 32767);
              memcpy(VBMem + i * channel.stride + channel.offset, static_cast<void *>(&tc), sizeof(int16_t) * 2);
              break;
            }
            default: G_ASSERT(0); break;
          }
          break;
        case SCUSAGE_VCOL:
          G_ASSERT(channel.type == SCTYPE_E3DCOLOR);
          memcpy(VBMem + i * channel.stride + channel.offset, static_cast<void *>(&point.color), sizeof(E3DCOLOR));
          break;
        default: log->addMessage(log->ERROR, "Unsupported shader channel usage %d", channel.usage); break;
      }
  }

  auto lockedPointVB = lock_sbuffer<uint8_t>(pointVB.getBuf(), 0, cloud.size() * vertexStride, 0);
  lockedPointVB.updateDataRange(0, VBData.begin(), VBData.size());
  lockedPointVB.close();

  auto plodShaderMaterial = elem.mat->clone();
  ShaderGlobal::set_int_fast(instancing_type_var_id, 0);
  ShaderGlobal::set_int_fast(rendinst_render_pass_var_id, eastl::to_underlying(rendinst::RenderPass::Normal));
  plodShaderMaterial->set_int_param(plod_gen_var_id, true);
  const auto plodElem = plodShaderMaterial->make_elem();
  uint32_t imm_const = 0;
  d3d::set_immediate_const(STAGE_VS, &imm_const, 1);
  d3d::setview(0, 0, 1024, 1024, -10, 10);

  d3d_err(d3d::setvsrc(0, pointVB.getBuf(), elem.e->getVertexStride()));
  d3d::set_rwbuffer(STAGE_PS, plod_output_reg_no, materialBuffer.getBuf());
  d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, instanceBuffer.getBuf());
  cb.flushPerDraw();

  plodElem->setReqTexLevel(0);
  plodElem->setStates();

  d3d::draw(PRIM_POINTLIST, 0, cloud.size());

  ProcessedPointCloud result{};
  auto lockedMaterialBuf = lock_sbuffer<uint8_t>(materialBuffer.getBuf(), 0, cloud.size(), 0);
  const auto materialMem = lockedMaterialBuf.get();
  result.points.resize(cloud.size());

  elem.mat->buildMaterialData(result.materialData);
  result.materialData.className = exportShader.exportName;

  eastl::fixed_vector<size_t, 4> skipChannels{};
  if (exportShader.materialChannels == PERLIN_MATERIAL_CHANNELS)
  {
    int perlinChannels = 3;
    if (elem.mat->get_texture(PERLIN_TEX1_CHANNEL) == BAD_TEXTUREID)
    {
      skipChannels.insert(skipChannels.end(), {PERLIN_DIFFUSE1_CHANNEL, PERLIN_NORMAL1_CHANNEL});
      perlinChannels--;
    }

    if (elem.mat->get_texture(PERLIN_TEX2_CHANNEL) == BAD_TEXTUREID)
    {
      skipChannels.insert(skipChannels.end(), {PERLIN_DIFFUSE2_CHANNEL, PERLIN_NORMAL2_CHANNEL});
      perlinChannels--;
    }
    eastl::fixed_string<char, 512> script = result.materialData.matScript.c_str();
    script.sprintf("%s\r\nmaterial_channel_count={%d}\r\n", result.materialData.matScript.c_str(), perlinChannels);
    result.materialData.matScript.setStr(script.c_str(), script.size());
  }

  for (size_t i = 0; i < result.points.size(); ++i)
  {
    result.points[i].position = cloud[i].position;
    result.points[i].norm = cloud[i].tbn.getcol(2);
    result.points[i].material.resize(materialStride / channelStride);
    for (size_t channel = 0; channel < result.points[i].material.size(); channel++)
      memcpy(&result.points[i].material[channel].val, materialMem + i * materialStride + channel * channelStride, channelStride);

    for (size_t channel : skipChannels)
      result.points[i].material[channel] = INVALID_MATERIAL_CHANNEl;
  }

  if (exportShader.materialChannels == SIMPLE_MATERIAL_CHANNELS)
    perturbNormals(cloud, make_span(result.points));
  else
    addTangents(cloud, make_span(result.points));
  return result;
}


RawPointCloudLods PointCloudGenerator::generatePointCloudFromMesh(const ShaderMesh &mesh, const BBox3 &bbox, int elem_idx,
  int mesh_face_count, int total_count)
{
  Tab<GlobalVertexData *> vertexData;
  RawPointCloudLods cloudLods{};
  float initDensity = 0.f;
  auto samples = samplePointsOnMesh(mesh, bbox, elem_idx, mesh_face_count, total_count, initDensity);
  Tab<Tab<SampleId>> sampleLods;

  dag::RelocatableFixedVector<float, 20> densities;
  for (int i = 0; (static_cast<int>(initDensity) >> i) > 0; ++i)
    densities.push_back(static_cast<int>(initDensity) >> i);

  sampleLods.resize(densities.size());
  for (int lod = densities.size() - 1; lod >= 0; --lod)
  {
    float density = densities[lod];
    // Firstly remove samples from next lods, because they are part of the current lod
    for (size_t nextLod = lod + 1; nextLod < densities.size(); ++nextLod)
    {
      for (auto sampleId : sampleLods[nextLod])
        samples.removeSamplesInPoissonRadius(sampleId, getPoissonRadius(density));
    }
    // Then sample what is left
    auto sampleId = samples.popSample();
    size_t sampleDiffCount = 0;
    while (sampleId != SampleId::Invalid)
    {
      cloudLods.points.push_back(samples.getVertex(sampleId));
      sampleDiffCount++;
      sampleLods[lod].push_back(sampleId);
      samples.removeSamplesInPoissonRadius(sampleId, getPoissonRadius(density));
      sampleId = samples.popSample();
    }
    cloudLods.lodPointCounts.push_back(sampleDiffCount);
    samples.restoreSamplePool();
  }
  size_t prefixSum = 0;
  for (auto &count : cloudLods.lodPointCounts)
  {
    prefixSum += count;
    count = prefixSum;
  }
  eastl::reverse(cloudLods.lodPointCounts.begin(), cloudLods.lodPointCounts.end());
  return cloudLods;
}

} // namespace plod
