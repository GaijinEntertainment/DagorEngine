// Copyright (C) Gaijin Games KFT.  All rights reserved.

// Land-class / detail-texture (virtual texture content) loading.
// Split out of lmeshManager.cpp; part of the same gameLibs/landMesh library.

#include <landMesh/landRayTracer.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_texMgrTags.h>
#include <osApiWrappers/dag_files.h>
#include <perfMon/dag_cpuFreq.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_log.h>
#include <ioSys/dag_baseIo.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_roDataBlock.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_btagCompr.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaderMeshTexLoadCtrl.h>
#include <math/dag_Point2.h>
#include <math/dag_adjpow2.h>
#include <shaders/dag_shaders.h>
#include <debug/dag_debug.h>
#include <landMesh/lmeshManager.h>
#include <landMesh/lmeshRenderer.h>
#include <landMesh/biomeQuery.h>
#include <gameRes/dag_gameResources.h>
#include <osApiWrappers/dag_direct.h>
#include <render/blkToConstBuffer.h>

#include <heightmap/heightmapHandler.h>
#include <gameMath/traceUtils.h>
#include <EASTL/string.h>
#include <EASTL/hash_map.h>
#include <EASTL/type_traits.h>
#include <landMesh/landClass.h>
#include <landMesh/riLandClass.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <scene/dag_physMat.h>
#include <util/fnameMap.h>
#include <supp/dag_alloca.h>
#include <physMap/physMapLoad.h>
#include <osApiWrappers/dag_sharedMem.h>
#include "lmeshManagerInternal.h"

static TEXTUREID get_lc_texture(const char *name, String &temp);
static TEXTUREID get_and_load_lc_texture(const char *name, bool set_aniso, bool validate_name);
typedef carray<ska::flat_hash_map<eastl::string, int, ska::power_of_two_std_hash<eastl::string>>, NUM_TEXTURES_STACK> DetailsMap;

struct TexturesInArray
{
  const char *names[NUM_TEXTURES_STACK] = {0};
  int id = -1;
  TexturesInArray() = default;
  TexturesInArray(const char *a, const char *b, const char *c)
  {
    names[0] = a;
    names[1] = b;
    names[2] = c;
  }
};

static const char *detailTypeNames[NUM_TEXTURES_STACK] = {"albedo", "normal", "reflectance"};
static int get_texture_id_in_array(const char *name, TexturesInArray &texInArray, DetailsMap &detailsIds)
{
  G_UNUSED(name);
  if (texInArray.id >= 0)
    return texInArray.id;
  for (int dType = 0; dType < NUM_TEXTURES_STACK; ++dType)
  {
    const char *detName = texInArray.names[dType];
    if (!detName)
      continue;
    auto it = detailsIds[dType].find_as(detName);
    if (it != detailsIds[dType].end())
      texInArray.id = it->second;
    else
      texInArray.id = (int)detailsIds[dType].size();
    detailsIds[dType][detName] = texInArray.id;
  }
  return texInArray.id;
}

static void set_land_selfillum_parameters_to_shaders(const DataBlock &params_blk, const LandClassDetailTextures &land)
{
  const char *biomeGroupName = params_blk.getStr("detail_group", "");
  auto pos = eastl::find_if(land.lcDetailGroupNames.begin(), land.lcDetailGroupNames.end(),
    [biomeGroupName](const char *name) { return strcmp(name, biomeGroupName) == 0; });
  int foundDetailIndex = pos < land.lcDetailGroupNames.end() ? pos - land.lcDetailGroupNames.begin() : -1;

  Point4 color = params_blk.getPoint4("autodetect_color", Point4(10.0f, 10.0f, 10.0f, -100.0f));
  ShaderGlobal::set_float4(get_shader_variable_id("autodetect_land_selfillum_color", true),
    Color4(color.x, color.y, color.z, color.w));
  Point4 tolerance = params_blk.getPoint4("autodetect_tolerance", Point4(0.0f, 0.0f, 0.0f, -100.0f));
  ShaderGlobal::set_float4(get_shader_variable_id("autodetect_land_selfillum_tolerance", true),
    Color4(tolerance.x, tolerance.y, tolerance.z, tolerance.w));
  ShaderGlobal::set_float(get_shader_variable_id("land_selfillum_strength", true), params_blk.getReal("strength", 4.0f));
  ShaderGlobal::set_float(get_shader_variable_id("land_selfillum_worldscale", true), params_blk.getReal("world_scale", 0.9f));
  ShaderGlobal::set_int(get_shader_variable_id("land_selfillum_biome_group_id", true), foundDetailIndex);
  ShaderGlobal::set_int(get_shader_variable_id("land_selfillum_microdetail_id", true), params_blk.getInt("microdetail_index", -1));
}
static bool load_land_class(LandClassDetailTextures &land, const char *name, const DataBlock &blk, const DataBlock &grassMaskBlk,
  DetailsMap &arrayDetailsIds, DetailsMap &standardDetailsIds, LandClassData data_needed, bool validate_name,
  Tab<int> &detailGroupToPhysMats)
{
  bool ret = true;
  land.editorId = blk.getInt("editorId", 0);
  land.lcType = LC_SIMPLE;
  strncpy(&land.name[0], &name[0], sizeof(land.name));
  land.offset = -blk.getPoint2("offset", Point2(0, 0));
  Point2 size = blk.getPoint2("size", Point2(1, 1));
  land.tile = safediv(1.0f, size.x);

  land.lcDetailParamsVS.resize(1);
  land.lcDetailParamsVS[0] = Point4(1, 1, 1, 1);
  land.weighMapMulOffset = {-1, -1, -1, -1};
  land.weightMapNoiseScale = 0;

  if (!(data_needed & LC_TRIVIAL_DATA))
    return ret;

  land.randomFlowmapParams = Point4(0, 0, 0, 0);
  land.flowmapMask = Point4(1, 1, 1, 1);
  float detailSize = blk.getReal("detailSize", 16.13);
  float detailTile = safediv(1.0f, detailSize);
  float flowmapTiling = safediv(size.x, detailSize);
  land.lcDetailParamsVS[0] = Point4(detailTile, detailTile, detailTile, detailTile);

  String colorName;
  const char *colormapName = blk.getStr("texture", NULL);
  land.colormapId = get_lc_texture(colormapName, colorName);
  if (land.colormapId == BAD_TEXTUREID || !::acquire_managed_tex(land.colormapId))
  {
    logwarn("detail colormap texture '%s' not found in landclass <%s>", colormapName, name);
    ret = false;
  }

  const char *grassMaskName = 0;
  if (data_needed & LC_GRASS_DATA)
    grassMaskName = blk.getStr("grassmask", NULL);

  land.grassMaskTexId = BAD_TEXTUREID;
  if (data_needed & LC_GRASS_DATA)
  {
    land.resetGrassMask(grassMaskBlk, colormapName, grassMaskName);
  }

  // land.normalMapId = BAD_TEXTUREID;
  if ((data_needed & LC_DETAIL_DATA))
  {
    land.displacementMin = Point4(blk.getReal("displacementMin", -0.1f), 0, 0, 0);
    land.displacementMax = Point4(blk.getReal("displacementMax", 0.2f), 0, 0, 0);
    land.bumpScales = Point4(blk.getReal("bumpScale", 1.0f), 0, 0, 0);
    land.puddleScales = Point4(blk.getReal("puddleScale", 1.0f), 0, 0, 0);
    land.compatibilityDiffuseScales = Point4(blk.getReal("compatibilityDiffuseScale", 1.0f), 0, 0, 0);
    land.waterDecalBumpScale = Point4(blk.getReal("waterDecalBumpScale", 1.0f), 0, 0, 0);
    const char *defaultPhysmat = blk.getStr("physMat", NULL);
    land.physmatIds = IPoint4(PhysMat::getMaterialId(defaultPhysmat), 0, 0, 0);
    land.finalColorMul = blk.getPoint4("finalColorMul", Point4(1, 1, 1, 1));

    if (blk.getStr("shader", 0))
    {
      land.lcType = LC_CUSTOM;
      strncpy(land.shader_name, blk.getStr("shader", 0), sizeof(land.shader_name));

      if (blk.getBlockByName("landClassParams"))
      {
        create_constant_buffer(*blk.getBlockByName("landClassParams"), DataBlock(), land.lcDetails);
      }

      if (blk.getBlockByName("landClassTextures"))
      {
        const DataBlock &lcTexBlk = *blk.getBlockByName("landClassTextures");
        for (int pi = 0; pi < lcTexBlk.paramCount(); ++pi)
        {
          if (lcTexBlk.getParamType(pi) != DataBlock::TYPE_STRING)
            continue;
          land.lcTextures.push_back(get_and_load_lc_texture(lcTexBlk.getStr(pi), false, validate_name));
        }
      }

      if (strcmp(land.shader_name, "landmesh_indexed_landclass_weightmap") == 0)
      {
        const Point2 &size = blk.getPoint2("weight_map_size", Point2(-1, -1));
        const Point2 &offset = blk.getPoint2("weight_map_offset", Point2(0, 0));
        const float &noiseScale = blk.getReal("weight_map_noise_scale", 0);
        if (size.x <= 0 || size.y <= 0)
        {
          logerr("Invalid or missing value for weight_map_size: (%d, %d)", size.x, size.y);
        }
        else
        {
          land.weighMapMulOffset = {1 / size.x, 1 / size.y, offset.x / size.x, offset.y / size.y};
        }

        if (noiseScale < 0)
        {
          logerr("Invalid value for weight_map_noise_scale: %f, the value must be positive", noiseScale);
        }
        else
        {
          land.weightMapNoiseScale = noiseScale;
        }
      }

      int lcDetailsArrayStart = land.lcDetails.size();
      int floatsPerDetail = -1, detailsHeaderSize = lcDetailsArrayStart * 16;
      if (blk.getBlockByName("details") && blk.getBlockByName("details")->getBlockByName("scheme"))
      {
        const DataBlock &detailsBlk = *blk.getBlockByName("details");
        const DataBlock &detailsSchemeBlk = *detailsBlk.getBlockByName("scheme");
        Tab<NameOffset> detailsSchemeOffsets;
        floatsPerDetail = create_constant_buffer_offsets(detailsSchemeOffsets, detailsSchemeBlk, 1);
        debug("floatsPerDetail = %d", floatsPerDetail);
        G_ASSERT((floatsPerDetail & 3) == 0 && floatsPerDetail > 0);
        Tab<IPoint2> texturesNameOffsets;
        int detailGroupsParamNameId = -1;
        for (int di = 0; di < detailsSchemeOffsets.size(); ++di)
        {
          const int nameId = detailsSchemeBlk.getNameId(detailsSchemeOffsets[di].name);
          const int pi = detailsSchemeBlk.findParam(nameId);
          const char *paramName = detailsSchemeBlk.getParamName(pi);
          if (strcmp(paramName, "detail_group") == 0)
            detailGroupsParamNameId = nameId;
          else if (detailsSchemeBlk.getParamType(pi) == DataBlock::TYPE_STRING)
            texturesNameOffsets.push_back(IPoint2(nameId, detailsSchemeOffsets[di].ofs));
        }

        eastl::hash_map<eastl::string, TexturesInArray> lcUniqueDetailTextures;
        int detailsCnt = 0;
        const int detailNameId = detailsBlk.getNameId("detail");
        const int detailGroupsNameId = detailsBlk.getNameId("detail_group");
        const int schemeNameId = detailsBlk.getNameId("scheme");
        for (int bi = 0; bi < detailsBlk.blockCount(); ++bi)
        {
          const DataBlock &detTexBlk = *detailsBlk.getBlock(bi);
          if (detTexBlk.getBlockNameId() == detailNameId || detTexBlk.getBlockNameId() == detailGroupsNameId ||
              detTexBlk.getBlockNameId() == schemeNameId)
            continue;

          lcUniqueDetailTextures[detTexBlk.getBlockName()] = {detTexBlk.getStr(detailTypeNames[0], NULL),
            detTexBlk.getStr(detailTypeNames[1], NULL), detTexBlk.getStr(detailTypeNames[2], NULL)};
        }

        clear_and_shrink(land.lcDetailGroupNames);
        clear_and_shrink(detailGroupToPhysMats);

        for (int bi = detailsBlk.findBlock(detailGroupsNameId); bi != -1; bi = detailsBlk.findBlock(detailGroupsNameId, bi))
        {
          land.lcDetailGroupNames.push_back(detailsBlk.getBlock(bi)->getStr("name"));
          detailGroupToPhysMats.push_back(PhysMat::getMaterialId(detailsBlk.getBlock(bi)->getStr("physMat", nullptr)));
        }

        clear_and_shrink(land.lcDetailGroupIndices);

        for (int bi = detailsBlk.findBlock(detailNameId); bi != -1; bi = detailsBlk.findBlock(detailNameId, bi))
        {
          const DataBlock &detTexBlk = *detailsBlk.getBlock(bi);
          int lcDetailsAt = append_items(land.lcDetails, (floatsPerDetail + 3) / 4);
          create_constant_buffer_from_offsets(detailsSchemeOffsets, detailsSchemeBlk, detTexBlk, &land.lcDetails[lcDetailsAt].x);
          for (int i = 0; i < texturesNameOffsets.size(); ++i)
          {
            const char *texName =
              detTexBlk.getStrByNameId(texturesNameOffsets[i].x, detailsSchemeBlk.getStrByNameId(texturesNameOffsets[i].x, ""));
            auto lcUniqueIt = lcUniqueDetailTextures.find_as(texName);
            G_ASSERT_CONTINUE(texName);
            if (lcUniqueIt == lcUniqueDetailTextures.end())
            {
              G_ASSERT_LOG(0, "no such block <%s> with details in <%s>", texName, detTexBlk.getBlockName());
              continue;
            }
            const int ofs = texturesNameOffsets[i].y;
            G_ASSERT_CONTINUE(ofs >= 0);
            ((int *)&land.lcDetails[lcDetailsAt].x)[ofs] = get_texture_id_in_array(name, lcUniqueIt->second, arrayDetailsIds);
          }

          if (land.lcDetailGroupNames.size() > 0)
          {
            const char *detailGroupName =
              detTexBlk.getStrByNameId(detailGroupsParamNameId, detailsSchemeBlk.getStrByNameId(detailGroupsParamNameId, "grass"));
            int foundDetailIndex = -1;
            for (size_t i = 0; i < land.lcDetailGroupNames.size(); i++)
            {
              if (strcmp(detailGroupName, land.lcDetailGroupNames[i]) == 0)
              {
                foundDetailIndex = (int)i;
                break;
              }
            }
            if (foundDetailIndex < 0)
            {
              logerr("Invalid detail group name [%s] in detail block [%d]", detailGroupName, land.lcDetailGroupIndices.size());
              foundDetailIndex = 0;
            }
            land.lcDetailGroupIndices.push_back(foundDetailIndex);
          }
        }
      }

      if (blk.getBlockByName("landSelfillumParams"))
        set_land_selfillum_parameters_to_shaders(*blk.getBlockByName("landSelfillumParams"), land);

      if (land.lcDetails.size())
      {
        G_ASSERT(elem_size(land.lcDetails) == 16);
        G_ASSERT(floatsPerDetail > 0);
        G_ASSERT(detailsHeaderSize > 0);

        const uint32_t cbElementSize = elem_size(land.lcDetails);
        uint32_t maxDetailArraySize = 64;

        int lmesh_hc_detail_array_elementsVarId = ::get_shader_glob_var_id("lmesh_hc_detail_array_elements", true);
        int lmesh_hc_floats_per_detailVarId = ::get_shader_glob_var_id("lmesh_hc_floats_per_detail", true);
        if (VariableMap::isVariablePresent(lmesh_hc_detail_array_elementsVarId))
          maxDetailArraySize = ShaderGlobal::get_int(lmesh_hc_detail_array_elementsVarId);
        else
          // provide array size via shader to avoid OOB problems!
          logerr("landclass: missing lmesh_hc_detail_array_elements, using default %u detail array size", maxDetailArraySize);

        if (VariableMap::isVariablePresent(lmesh_hc_floats_per_detailVarId))
        {
          if (ShaderGlobal::get_int(lmesh_hc_floats_per_detailVarId) != floatsPerDetail &&
              strcmp(land.shader_name, "landmesh_indexed_landclass_normal") != 0)
            logerr("landclass: detail struct size from scheme = %i differs from one in shader = %i, correct scheme/shader",
              floatsPerDetail, ShaderGlobal::get_int(lmesh_hc_floats_per_detailVarId));
        }
        else
          // provide detail struct size via shader to avoid reading garbadge in shader!
          logerr("landclass: missing lmesh_hc_floats_per_detail, detail buffer can contain garbadge for shaders");

        int biome_lc_ofs_to_detail_info_cntVarId = ::get_shader_glob_var_id("biome_lc_ofs_to_detail_info_cnt", true);
        if (VariableMap::isVariablePresent(biome_lc_ofs_to_detail_info_cntVarId))
        {
          int DETAIL_INFO_CNT_OFS = ShaderGlobal::get_int(biome_lc_ofs_to_detail_info_cntVarId); // offset of details_cnt inside
                                                                                                 // DetailInfo in ints
          int lcDetailsWithoutHeaderCnt = ((int)land.lcDetails.size() - lcDetailsArrayStart) * 4 / cbElementSize;
          // we store the cnt in an unused member of the first detail (so we don't need extra padding for the header this way)
          reinterpret_cast<int *>(&land.lcDetails[0].x)[lcDetailsArrayStart * 4 + DETAIL_INFO_CNT_OFS] = lcDetailsWithoutHeaderCnt;
        }
        else
        {
          logwarn("landclass: missing biome_lc_ofs_to_detail_info_cnt shadervar, can't validate biome count in index texture");
        }

        const uint32_t maxDetailElements = (floatsPerDetail * 4 * maxDetailArraySize + detailsHeaderSize) / cbElementSize;
        uint32_t bufSize = max(maxDetailElements, land.lcDetails.size());
        // verify that cbuffer size in shader fully fits blk data
        if (bufSize > maxDetailElements)
          logerr("detail cbuffer in shader have %u elements max while asked for %u", maxDetailElements, bufSize);

        land.detailsCB = d3d::buffers::create_persistent_cb(bufSize, "lcDetails", RESTAG_LAND);
        G_ASSERT_LOG(land.detailsCB, "Can't create constant buffer for landclass %s", name);
        if (land.detailsCB)
        {
          if (!land.detailsCB->updateDataWithLock(0, data_size(land.lcDetails), land.lcDetails.data(),
                VBLOCK_WRITEONLY | VBLOCK_DISCARD))
          {
            G_ASSERT_LOG(0, "Can't write constant buffer for landclass %s", name);
            land.detailsCB->destroy();
            land.detailsCB = nullptr;
          }
        }
      }
    }
    else if (blk.getStr("detailmap", NULL))
    {
      land.lcType = blk.getStr("detailmap_r", NULL) ? LC_DETAILED_R : LC_DETAILED;
      land.flowmapTex = get_and_load_lc_texture(blk.getStr("flowmapTex", NULL), false, validate_name);
      land.lcTextures.resize(land.lcType == LC_DETAILED ? 1 : 2);
      land.lcTextures[0] = get_and_load_lc_texture(blk.getStr("detailmap", NULL), true, validate_name);
      if (land.lcType == LC_DETAILED_R)
        land.lcTextures[1] = get_and_load_lc_texture(blk.getStr("detailmap_r", NULL), true, validate_name);
    }
    else if (blk.getStr("splattingmap", NULL))
    {
      land.lcType = LC_MEGA_NO_NORMAL; //(!info.normalmapName || !info.normalmapName.length()) ? LC_MEGA_NO_NORMAL : LC_MEGA;
      land.flowmapTex = get_and_load_lc_texture(blk.getStr("flowmapTex", NULL), false, validate_name);
      land.lcTextures.resize(1);
      land.lcTextures[0] = get_and_load_lc_texture(blk.getStr("splattingmap", NULL), false, validate_name);
      float detailSizeRed = blk.getReal("detailSizeRed", detailSize);
      float detailSizeGreen = blk.getReal("detailSizeGreen", detailSize);
      float detailSizeBlue = blk.getReal("detailSizeBlue", detailSize);
      float detailSizeBlack = blk.getReal("detailSizeBlack", detailSize);
      land.lcDetailParamsVS[0][LandClassDetailTextures::DETAIL_RED] = safediv(1.0f, detailSizeRed);
      land.lcDetailParamsVS[0][LandClassDetailTextures::DETAIL_GREEN] = safediv(1.0f, detailSizeGreen);
      land.lcDetailParamsVS[0][LandClassDetailTextures::DETAIL_BLUE] = safediv(1.0f, detailSizeBlue);
      land.lcDetailParamsVS[0][LandClassDetailTextures::DETAIL_BLACK] = safediv(1.0f, detailSizeBlack);
      for (int i = 0; i < land.lcDetailTextures.size(); ++i)
      {
        land.lcDetailTextures[i].resize(LandClassDetailTextures::MAX_DETAILS);
        mem_set_ff(land.lcDetailTextures[i]);
      }

      land.displacementMin.x = blk.getReal("displacementMinRed", -0.1f);
      land.displacementMin.y = blk.getReal("displacementMinGreen", -0.1f);
      land.displacementMin.z = blk.getReal("displacementMinBlue", -0.1f);
      land.displacementMin.w = blk.getReal("displacementMinBlack", -0.1f);

      land.displacementMax.x = blk.getReal("displacementMaxRed", 0.2f);
      land.displacementMax.y = blk.getReal("displacementMaxGreen", 0.2f);
      land.displacementMax.z = blk.getReal("displacementMaxBlue", 0.2f);
      land.displacementMax.w = blk.getReal("displacementMaxBlack", 0.2f);

      land.bumpScales.x = blk.getReal("bumpScaleRed", 1.0f);
      land.bumpScales.y = blk.getReal("bumpScaleGreen", 1.0f);
      land.bumpScales.z = blk.getReal("bumpScaleBlue", 1.0f);
      land.bumpScales.w = blk.getReal("bumpScaleBlack", 1.0f);

      land.puddleScales.x = blk.getReal("puddleScaleRed", 0.5f);
      land.puddleScales.y = blk.getReal("puddleScaleGreen", 0.5f);
      land.puddleScales.z = blk.getReal("puddleScaleBlue", 0.5f);
      land.puddleScales.w = blk.getReal("puddleScaleBlack", 0.5f);

      land.compatibilityDiffuseScales.x = blk.getReal("compatibilityDiffuseScaleRed", 1.0f);
      land.compatibilityDiffuseScales.y = blk.getReal("compatibilityDiffuseScaleGreen", 1.0f);
      land.compatibilityDiffuseScales.z = blk.getReal("compatibilityDiffuseScaleBlue", 1.0f);
      land.compatibilityDiffuseScales.w = blk.getReal("compatibilityDiffuseScaleBlack", 1.0f);

      land.physmatIds.x = PhysMat::getMaterialId(blk.getStr("physMatRed", defaultPhysmat));
      land.physmatIds.y = PhysMat::getMaterialId(blk.getStr("physMatGreen", defaultPhysmat));
      land.physmatIds.z = PhysMat::getMaterialId(blk.getStr("physMatBlue", defaultPhysmat));
      land.physmatIds.w = PhysMat::getMaterialId(blk.getStr("physMatBlack", defaultPhysmat));

      flowmapTiling = safediv(size.x, max(max(detailSizeRed, detailSizeGreen), max(detailSizeBlue, detailSizeBlack)));

      for (int dType = 0; dType < NUM_TEXTURES_STACK; ++dType)
      {
        static const char *detailNames[NUM_TEXTURES_STACK][LandClassDetailTextures::MAX_DETAILS] = {
          {"detailRed", "detailGreen", "detailBlue", "detailBlack"}, {"detail1Red", "detail1Green", "detail1Blue", "detail1Black"},
          {"detail2Red", "detail2Green", "detail2Blue", "detail2Black"}};
        for (int i = 0; i < land.lcDetailTextures[dType].size(); ++i)
        {
          const char *detName = blk.getStr(detailNames[dType][i], NULL);
          if (!detName)
            continue;
          if (dType == 2)
            land.lcType = LC_MEGA_DETAILED;
          auto it = standardDetailsIds[dType].find_as(detName);
          if (it != standardDetailsIds[dType].end())
            land.lcDetailTextures[dType][i] = it->second;
          else
          {
            land.lcDetailTextures[dType][i] = (int16_t)standardDetailsIds[dType].size();
            standardDetailsIds[dType][detName] = land.lcDetailTextures[dType][i];
          }
        }
      }
    }
    land.randomFlowmapParams.x = max(blk.getReal("flowmap", floor(flowmapTiling)), 1.0f);
    land.randomFlowmapParams.y = blk.getReal("flowmapNoiseFrequency", 100.0f);
    land.flowmapMask = blk.getPoint4("flowmapMask", Point4(1.0f, 1.0f, 1.0f, 1.0f));
  }
  return ret;
}
static bool ends_with(const char *str, const char *end)
{
  return strlen(str) > strlen(end) && strcmp(str + strlen(str) - strlen(end), end) == 0;
}
static void process_lc_tex_name(const char *name, String &temp)
{
  temp = name;
  if (temp.length() && temp[temp.length() - 1] != '*' && temp[0] != '*')
  {
    const char *fname = dd_get_fname(name);
    temp = fname ? fname : name;
    const char *ext = strchr(temp.data(), '.');
    char buf[256];
    int len = int(ext ? min<uint32_t>(ext - temp.data(), 255) : strlen(temp.data()));
    strncpy(buf, temp.data(), len);
    buf[len] = 0;
    temp.printf(128, "%s*", buf);
  }
}
static TEXTUREID get_lc_texture(const char *name, String &temp)
{
  if (!name || name[0] == 0)
    return BAD_TEXTUREID;
  process_lc_tex_name(name, temp);
  return ::get_managed_texture_id_pp(temp);
}

static TEXTUREID get_and_load_lc_texture(const char *name, bool set_aniso, bool validate_name)
{
  G_UNUSED(set_aniso);
  String temp;
  TEXTUREID id = get_lc_texture(name, temp);
  if (id != BAD_TEXTUREID)
  {
    BaseTexture *tex = ::acquire_managed_tex(id);
    if (!tex)
      logerr("can not load lc texture %s", temp.str());
  }
  else if (name && name[0] != 0)
  {
    logerr("no such lc texture %s (%s)", name, temp.str());
    if (validate_name)
      G_ASSERTF(0, "no such lc texture %s (%s)", name, temp.str());
  }
  return id;
}
void LandClassDetailTextures::resetGrassMask(const DataBlock &grassBlk, const char *color_name, const char *info_grass_mask_name)
{
  String grassMaskName;
  if (!info_grass_mask_name || info_grass_mask_name[0] == 0)
  {
    String detailName(dd_get_fname(color_name));

    if (const char *ext = dd_get_fname_ext(detailName))
      erase_items(detailName, ext - &detailName[0], (int)strlen(ext));
    if (detailName.length() > 0 && detailName[detailName.length() - 1] == '*')
      detailName[detailName.length() - 1] = 0;
    grassMaskName.printf(128, "%s*", grassBlk.getStr(detailName.str(), ""));
    debug("color_name=<%s> -> %s -> grassMaskName=<%s>", color_name, detailName, grassMaskName);
  }
  else
    process_lc_tex_name(info_grass_mask_name, grassMaskName);

  if (grassMaskName.length() > 1)
  {
    grassMaskTexId = ::get_managed_texture_id_pp(grassMaskName);
    G_ASSERTF(grassMaskTexId != BAD_TEXTUREID, "Grass mask '%s' not found", grassMaskName);
  }

  if (grassMaskTexId == BAD_TEXTUREID)
    grassMaskTexId = ::get_managed_texture_id_pp("grass_mask_black*");
  if (grassMaskTexId == BAD_TEXTUREID)
    logerr("grass mask absent color_name = %@ info_grass_mask_name = %@ grassMaskName = %@", color_name, info_grass_mask_name,
      grassMaskName);
  G_ASSERTF(grassMaskTexId != BAD_TEXTUREID, "grass_mask_black* mandatory asset not found");
  ::acquire_managed_tex(grassMaskTexId);
}
void LandMeshManager::loadLandClasses(IGenLoad &cb)
{
  for (auto &t : megaDetailsId)
    t.clear();

  DetailsMap arrayDetailsIds;
  DetailsMap standardDetailsIds;

  cb.beginBlock();
  const int landClassCount = cb.readInt();
  landClasses.resize(landClassCount);
  clear_and_resize(landClassesEditorId, landClassCount);
  biomeLandClassIdx = -1;

  String nm;
  for (int i = 0; i < landClassCount; ++i)
  {
    DataBlock blk;
    cb.beginBlock();
    cb.readString(nm);
    if (!toolsInternal && get_land_class_detail_data && get_land_class_detail_data(blk, nm))
      debug("landclass[%d] %s (res)", i, nm);
    else if (cb.getBlockRest())
    {
      blk.loadFromStream(cb);
      debug("landclass[%d] %s (level bindump)", i, nm);
    }
    else
      logerr("failed to load landclass[%d] %s", i, nm);
    cb.endBlock();

    if (getRenderDataNeeded() & LC_DETAIL_DATA)
    {
      if (blk.getStr("detailmap", NULL) && blk.getStr("splattingmap", NULL))
      {
        logerr("ambiguos landclass = <%s> it has both splattingmap and detailmap", nm.str());
#if DAGOR_DBGLEVEL > 0
        G_ASSERTF(0, "ambiguos landclass = <%s> it has both splattingmap and detailmap", nm.str());
#endif
      }
    }

    if (!load_land_class(landClasses[i], nm, blk, grassMaskBlk, arrayDetailsIds, standardDetailsIds, getRenderDataNeeded(),
          !toolsInternal, detailGroupsToPhysMats))
    {
      logerr("errors in landclass = <%s>", nm);
      landClasses[i].lcType = LC_SIMPLE;
    }
    float tiles_x = origin.x * landCellSize * landClasses[i].tile;
    float tiles_y = origin.y * landCellSize * landClasses[i].tile;
    if (fabsf(tiles_x - floorf(tiles_x)) + fabsf(tiles_y - floorf(tiles_y)) > 1e-5)
      logwarn("%s: change offset %@ -> %@, tileSz=%.1f", nm, landClasses[i].offset,
        landClasses[i].offset - Point2::xy(origin) * landCellSize, safediv(1.0f, landClasses[i].tile));
    landClasses[i].offset.x -= origin.x * landCellSize;
    landClasses[i].offset.y -= origin.y * landCellSize;
    static int inEditorGvId = ::get_shader_glob_var_id("in_editor", true);
    static int inEditor = ShaderGlobal::get_int_fast(inEditorGvId);
    if (!landClasses[i].lcTextures.empty() && (ends_with(landClasses[i].name, "_det") || ends_with(landClasses[i].name, "_biome") ||
                                                ((!strcmp(landClasses[i].shader_name, "landmesh_indexed_landclass") ||
                                                   !strcmp(landClasses[i].shader_name, "landmesh_indexed_landclass_normal") ||
                                                   !strcmp(landClasses[i].shader_name, "landmesh_indexed_landclass_weightmap")) &&
                                                  inEditor > 0)))
    {
      BaseTexture *lcTex = ::acquire_managed_tex(landClasses[i].lcTextures[0]);
      G_ASSERT_RETURN(lcTex != nullptr, );
      TextureInfo lcTexInfo;
      lcTex->getinfo(lcTexInfo);
      ::release_managed_tex(landClasses[i].lcTextures[0]);
      float landSize = safediv(1.0f, landClasses[i].tile);
      float worldLcTexelSize = landSize / lcTexInfo.w;

      ShaderGlobal::set_texture(::get_shader_variable_id("biomeIndicesTex", true), landClasses[i].lcTextures[0]);
      ShaderGlobal::set_float4(::get_shader_variable_id("biome_indices_tex_size", true), lcTexInfo.w, lcTexInfo.h, 1.0f / lcTexInfo.w,
        1.0f / lcTexInfo.h);
      ShaderGlobal::set_float4(::get_shader_variable_id("land_detail_mul_offset", true), landClasses[i].tile, -landClasses[i].tile,
        (landClasses[i].offset.x - offset.x + 0.5f * worldLcTexelSize) * landClasses[i].tile,
        (landClasses[i].offset.y - offset.z + 0.5f * worldLcTexelSize) * -landClasses[i].tile);

      biome_query::init_land_class(landClasses[i].lcTextures[0], landClasses[i].tile, landClasses[i].lcDetailGroupNames,
        landClasses[i].lcDetailGroupIndices);
      biome_query::set_details_cb(landClasses[i].detailsCB);
      biomeLandClassIdx = i;
    }

    landClassesEditorId[i] = landClasses[i].editorId;
  }
  cb.endBlock();

  for (int detailType = 0; detailType < NUM_TEXTURES_STACK; ++detailType)
  {
    if (standardDetailsIds[detailType].size())
    {
      Tab<const char *> textureNames;
      textureNames.resize(standardDetailsIds[detailType].size());
      for (const auto &it : standardDetailsIds[detailType])
        textureNames[it.second] = it.first.c_str();

      megaDetailsId[detailType].reserve(textureNames.size());
      for (auto name : textureNames)
      {
        TEXTUREID id = get_and_load_lc_texture(name, true, !toolsInternal);
        if (id == BAD_TEXTUREID)
        {
          if (!toolsInternal)
            G_ASSERTF(0, "can not load detail, <%s>", name);
        }
        megaDetailsId[detailType].push_back(id);
      }
    }
    if (arrayDetailsIds[detailType].size())
    {
      Tab<const char *> textureNames;
      textureNames.resize(arrayDetailsIds[detailType].size());
      for (const auto &it : arrayDetailsIds[detailType])
        textureNames[it.second] = it.first.c_str();

      megaDetailsArrayId[detailType] = ::add_managed_array_texture(String(128, "land_details%d*", detailType), textureNames);
      ArrayTexture *array = (ArrayTexture *)::acquire_managed_tex(megaDetailsArrayId[detailType]);
    }
  }
}
void LandMeshManager::loadDetailData(IGenLoad &cb)
{
  loadLandClasses(cb);

  detailMap.load(cb, baseDataOffset, toolsInternal);
}
