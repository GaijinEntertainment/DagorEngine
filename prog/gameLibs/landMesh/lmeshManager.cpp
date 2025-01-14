// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <landMesh/landRayTracer.h>
#include <rendInst/rendInstGen.h>
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
// #include <rendInst/rendInstGen.h>
#include <osApiWrappers/dag_direct.h>
#include <render/blkToConstBuffer.h>

#include <heightmap/heightmapHandler.h>
#include "lmeshCulling.inc.cpp"
#include <gameMath/traceUtils.h>
#include <EASTL/string.h>
#include <EASTL/hash_map.h>
#include <landMesh/landClass.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <scene/dag_physMat.h>
#include <util/fnameMap.h>
#include <supp/dag_alloca.h>
#include <physMap/physMapLoad.h>

#define MIN_TILE_SIZE 0.5

static TEXTUREID get_lc_texture(const char *name, String &temp);
static TEXTUREID get_and_load_lc_texture(const char *name, bool set_aniso, bool validate_name);
static inline IGenLoad &ptr_to_ref(IGenLoad *crd) { return *crd; }

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
    const int current = texInArray.id;
    G_UNUSED(current);
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
  ShaderGlobal::set_color4(get_shader_variable_id("autodetect_land_selfillum_color", true),
    Color4(color.x, color.y, color.z, color.w));
  ShaderGlobal::set_real(get_shader_variable_id("land_selfillum_strength", true), params_blk.getReal("strength", 4.0f));
  ShaderGlobal::set_real(get_shader_variable_id("land_selfillum_worldscale", true), params_blk.getReal("world_scale", 0.9f));
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
  if (land.colormapId != BAD_TEXTUREID)
  {
    BaseTexture *colormap = ::acquire_managed_tex(land.colormapId);
    if (!colormap)
    {
      logwarn("detail colormap texture '%s' not found in landclass <%s>", colormapName, name);
      ret = false;
    }
  }
  else
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
    land.compatibilityDiffuseScales = Point4(blk.getReal("compatibilityDiffuseScale", 1.0f), 0, 0, 0);
    land.waterDecalBumpScale = Point4(blk.getReal("waterDecalBumpScale", 1.0f), 0, 0, 0);
    const char *defaultPhysmat = blk.getStr("physMat", NULL);
    land.physmatIds = IPoint4(PhysMat::getMaterialId(defaultPhysmat), 0, 0, 0);

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
          if (ShaderGlobal::get_int(lmesh_hc_floats_per_detailVarId) != floatsPerDetail)
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

        land.detailsCB = d3d::buffers::create_persistent_cb(bufSize, "lcDetails");
        G_ASSERT_LOG(land.detailsCB, "Can't create constant buffer for landclass %s", name);
        if (land.detailsCB)
        {
          if (!land.detailsCB->updateDataWithLock(0, data_size(land.lcDetails), land.lcDetails.data(),
                VBLOCK_WRITEONLY | VBLOCK_DISCARD))
          {
            G_ASSERT_LOG(0, "Can't write constant buffer for landclass %s", name);
            land.detailsCB->destroy();
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
    land.flowmapMask = blk.getPoint4("flowmapMask", Point4(1.0f, 1.0f, 1.0f, 1.0f));
  }
  return ret;
}

static bool ends_with(const char *str, const char *end)
{
  return strlen(str) > strlen(end) && strcmp(str + strlen(str) - strlen(end), end) == 0;
}

static VDECL lmeshVdecl = BAD_VDECL;

static constexpr int MAX_CELLS_WIDTH = 64; // because we use 6 bits for storing index
LandMeshManager::DetailMap::DetailMap() : cells(midmem) {}

LoadElement::LoadElement() : tex1(NULL), tex2(NULL), tex1Id(BAD_TEXTUREID), tex2Id(BAD_TEXTUREID) {}

LoadElement::~LoadElement()
{
  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(tex1Id, tex1);
  ShaderGlobal::reset_from_vars_and_release_managed_tex_verified(tex2Id, tex2);
}

void LandMeshManager::DetailMap::load(IGenLoad &cb, int base_ofs, bool tools_internal)
{
  cb.readInt(sizeX);
  cb.readInt(sizeY);
  cb.readInt(texSize);
  cb.readInt(texElemSize);
  cells.resize(sizeX * sizeY);
  if (tools_internal)
  {
    for (int i = 0; i < cells.size(); ++i)
    {
      mem_set_ff(cells[i].detTexIds);
      cells[i].tex1 = d3d::create_tex(NULL, texSize, texSize, TEXCF_BEST | TEXCF_ABEST, 1);
      d3d_err(cells[i].tex1);
      cells[i].tex1->texaddr(TEXADDR_CLAMP);
      cells[i].tex1Id = ::register_managed_tex(String(32, "land_tex#%d", i), cells[i].tex1);

      cells[i].tex2 = d3d::create_tex(NULL, texSize, texSize, TEXCF_BEST | TEXCF_ABEST, 1);
      d3d_err(cells[i].tex2);
      cells[i].tex2->texaddr(TEXADDR_CLAMP);
      cells[i].tex2Id = ::register_managed_tex(String(32, "land_tex2#%d", i), cells[i].tex2);
    }
    return;
  }


  SmallTab<int, MidmemAlloc> offsets;
  clear_and_resize(offsets, sizeX * sizeY);
  cb.readTabData(offsets);
  cells.resize(offsets.size());
  //
  for (int i = 0; i < offsets.size(); ++i)
  {
    // G_ASSERT(offsets[index]+manager.getBaseOffset());
    cb.seekto(base_ofs + offsets[i]);
    load(i, cb);
  }
}

void LandMeshManager::DetailMap::clear() { clear_and_shrink(cells); }

void LandMeshManager::DetailMap::load(int i, IGenLoad &cb)
{
  LoadElement &e = cells[i];
  DAGOR_TRY
  {
    cb.read(&e.detTexIds[0], DET_TEX_NUM); // fixme: we should directly upload that

    int len, tex2Offset;
    cb.readInt(len);
    cb.readInt(tex2Offset);
    if (len)
    {
      e.tex1 = d3d::create_ddsx_tex(cb, TEXCF_RGB | TEXCF_SYSTEXCOPY, 0, 0, "land_tex");

      d3d_err(e.tex1);
      e.tex1->texaddr(TEXADDR_CLAMP);
      e.tex1Id = ::register_managed_tex(String(32, "land_tex#%d", i), e.tex1);
      if (tex2Offset != 0 && len - tex2Offset > 0)
      {
        e.tex2 = d3d::create_ddsx_tex(cb, TEXCF_RGB | TEXCF_SYSTEXCOPY, 0, 0, "land_tex2");
        d3d_err(e.tex2);
        e.tex2->texaddr(TEXADDR_CLAMP);

        e.tex2Id = ::register_managed_tex(String(32, "land_tex2#%d", i), e.tex2);
      }
      else
      {
        e.tex2Id = BAD_TEXTUREID;
        e.tex2 = NULL;
      }
    }
    else
    {
      e.tex1Id = BAD_TEXTUREID;
      e.tex1 = NULL;
    }
  }
  DAGOR_CATCH(const IGenLoad::LoadException &exc) { G_ASSERT(false); }
}

void LandMeshManager::DetailMap::getLandDetailTexture(int index, TEXTUREID &tex1, TEXTUREID &tex2, uint8_t detail_tex_ids[DET_TEX_NUM])
{
  LoadElement &e = cells[index];
  memcpy(detail_tex_ids, e.detTexIds.data(), DET_TEX_NUM);
  tex1 = e.tex1Id;
  tex2 = e.tex2Id;
}


LandMeshManager::LandMeshManager(bool tools_internal) :
  gridCellSize(1),
  fileVersion(1),
  baseDataOffset(0),
  tileTexId(BAD_TEXTUREID),
  tileXSize(0),
  tileYSize(0),
  visRange(-1),
  vertTexId(BAD_TEXTUREID),
  vertNmTexId(BAD_TEXTUREID),
  vertDetTexId(BAD_TEXTUREID),
  renderDataNeeded(LC_ALL_DATA),
  hmapHandler(NULL),
  toolsInternal(tools_internal)
{
  mem_set_0(megaDetailsArrayId); // fill with BAD_TEXTUREID=0
  landTracer = NULL;
  cullingState.cullMode = LandMeshCullingState::ASYNC_CULLING;
  lmeshVdata = combinedVdata = NULL;
  useVertTexforHMAP = true;
  mayRenderHmap = true;
}


LandMeshManager::~LandMeshManager() { close(); }

void LandMeshManager::CellData::clear()
{
  for (auto &m : meshes)
    del_it(m);
  clear_and_shrink(isCombinedBig);
}

#define RELEASE_MANAGED_TEX(t)                                     \
  {                                                                \
    ((t) == BAD_TEXTUREID ? ((void)0) : release_managed_tex((t))); \
    t = BAD_TEXTUREID;                                             \
  }
void LandClassDetailTextures::close()
{
  RELEASE_MANAGED_TEX(colormapId);
  RELEASE_MANAGED_TEX(flowmapTex);
  RELEASE_MANAGED_TEX(grassMaskTexId);
  for (int i = 0; i < lcTextures.size(); ++i)
    RELEASE_MANAGED_TEX(lcTextures[i]);
  // RELEASE_MANAGED_TEX(normalMapId);
  if (detailsCB)
    detailsCB->destroy();
  reset();
}

void LandMeshManager::evictSplattingData()
{
  for (int i = 0; i < landClasses.size(); ++i)
    landClasses[i].close();
  for (int dtype = 0; dtype < megaDetailsId.size(); ++dtype)
    for (int i = 0; i < megaDetailsId[dtype].size(); ++i)
      RELEASE_MANAGED_TEX(megaDetailsId[dtype][i]);
  ShaderGlobal::reset_from_vars(megaDetailsArrayId[0]);
  for (int i = 0; i < megaDetailsArrayId.size(); ++i)
    RELEASE_MANAGED_TEX(megaDetailsArrayId[i]);
  clear_and_shrink(landClasses);
  detailMap.clear();
  for (int i = 0; i < cells.size(); ++i)
    del_it(cells[i].decal);
}

void LandMeshManager::close()
{
  evictSplattingData();
  RELEASE_MANAGED_TEX(vertTexId);

  RELEASE_MANAGED_TEX(vertNmTexId);

  RELEASE_MANAGED_TEX(vertDetTexId);

  for (int i = 0; i < cells.size(); ++i)
    cells[i].clear();
  clear_and_shrink(cells);

  RELEASE_MANAGED_TEX(tileTexId);

  del_it(landTracer);
  baseDataOffset = 0;
  del_it(hmapHandler);
  useVertTexforHMAP = true;
  holesMgr.reset();
}
#undef RELEASE_MANAGED_TEX

bool LandMeshManager::loadMeshData(IGenLoad &cb)
{
  int tmp[4];
  cb.read(tmp, sizeof(int) * 4);
  int vdataCount = tmp[2];
  smvd = ShaderMatVdata::create(tmp[0], tmp[1], tmp[2], tmp[3]);
  smvd->loadTexStr(cb, false); //?
  // smvd->loadTexIdx ( cb, texMap );//?
  smvd->loadMatVdata("landmesh", cb, VDATA_NO_IBVB);

  clear_and_resize(cells, mapSizeX * mapSizeY);
  cb.beginBlock();
  for (int i = 0; i < cells.size(); ++i)
  {
    int len;
    cb.beginBlock();

    cb.beginBlock();
    for (int lod = 0; lod < LOD_COUNT; ++lod)
    {
      if (!cb.getBlockRest()) // additional lods, that are not saved yet
        break;
      cb.beginBlock();
      cells[i].land[lod] = ShaderMesh::load(cb, cb.getBlockRest(), *smvd);
      cb.endBlock();
    }
    cb.endBlock();

    cb.beginBlock();
    cells[i].decal = ShaderMesh::load(cb, cb.getBlockRest(), *smvd);
    cb.endBlock();

    cb.beginBlock();
    cells[i].combined = ShaderMesh::load(cb, cb.getBlockRest(), *smvd);
    static int bigVarId = get_shader_variable_id("big", true);
    if (cells[i].combined)
    {
      dag::ConstSpan<ShaderMesh::RElem> elems = cells[i].combined->getAllElems();
      clear_and_resize(cells[i].isCombinedBig, elems.size());
      for (int elemNo = 0; elemNo < elems.size(); elemNo++)
      {
        int big = 1;
        cells[i].isCombinedBig[elemNo] = bigVarId <= 0 || (!elems[elemNo].mat->getIntVariable(bigVarId, big) || big == 1);
      }
    }
    cb.endBlock();

    if (cb.getBlockRest())
    {
      cb.beginBlock();
      cells[i].patches = ShaderMesh::load(cb, cb.getBlockRest(), *smvd);
      cb.endBlock();
    }

    cb.endBlock();
  }

  cb.beginBlock();
  clear_and_resize(cellBoundings, mapSizeX * mapSizeY);
  clear_and_resize(cellBoundingsRadius, mapSizeX * mapSizeY);
  cb.read(cellBoundings.data(), data_size(cellBoundings));
  cb.read(cellBoundingsRadius.data(), data_size(cellBoundingsRadius));
  cb.endBlock();

  landBbox.setempty();
  for (int i = 0; i < cellBoundings.size(); i++)
    landBbox += cellBoundings[i];

  const float yofs = landBbox.isempty() ? 0 : 0.5f * (landBbox[1].y + landBbox[0].y);
  const float yscale = landBbox.isempty() ? 0 : 0.5f * (landBbox[1].y - landBbox[0].y);

  if (hmapHandler)
    landBbox += hmapHandler->getWorldBox();

  if (landBbox.width().y < VERY_SMALL_NUMBER)
    landBbox[1].y += VERY_SMALL_NUMBER;

  cb.endBlock();

  // decode vertices:
  int64_t reft;
  reft = ref_time_ticks();

  static constexpr int MAX_VDATA = 64;
  carray<uint8_t, MAX_VDATA> vdataUsage;
  enum
  {
    VDATA_LOD0 = 0x01,
    VDATA_COMBINED = 0x10,
    VDATA_DECAL = 0x20,
    VDATA_PATCHES = 0x40
  };
  GlobalVertexData *startVD = smvd->getGlobVData(0);

  memset(vdataUsage.data(), 0, vdataCount);

  int maxLod = -1;

  for (int lod = 0; lod < LOD_COUNT; ++lod)
  {
    uint8_t flag = VDATA_LOD0 << lod;
    for (int i = 0; i < cells.size(); ++i)
    {
      if (cells[i].land[lod] && cells[i].land[lod]->getAllElems().size())
      {
        maxLod = max(maxLod, lod);
        for (int ei = 0; ei < cells[i].land[lod]->getAllElems().size(); ++ei)
          vdataUsage[cells[i].land[lod]->getAllElems()[ei].vertexData - startVD] |= flag;
      }
    }
  }

  for (int i = 0; i < cells.size(); ++i)
  {
    if (cells[i].decal)
      for (int ei = 0; ei < cells[i].decal->getAllElems().size(); ++ei)
        vdataUsage[cells[i].decal->getAllElems()[ei].vertexData - startVD] |= VDATA_DECAL;
    if (cells[i].combined)
    {
      for (int ei = 0; ei < cells[i].combined->getAllElems().size(); ++ei)
        vdataUsage[cells[i].combined->getAllElems()[ei].vertexData - startVD] |= VDATA_COMBINED;
    }
    if (cells[i].patches)
    {
      for (int ei = 0; ei < cells[i].patches->getAllElems().size(); ++ei)
        vdataUsage[cells[i].patches->getAllElems()[ei].vertexData - startVD] |= VDATA_PATCHES;
    }
  }
#if DAGOR_DBGLEVEL > 0
  for (int i = 0; i < vdataCount; ++i)
    G_ASSERT(is_pow_of2(vdataUsage[i]));
#endif

  carray<int, MAX_VDATA> lmeshBaseVertex;
  carray<int, MAX_VDATA> lmeshVertexCount;
  carray<int, MAX_VDATA> lmeshBaseIndex;

  // debug("lmesh vdata count = %d", tmp[2]);
  carray<int, LOD_COUNT + 1> totalLmeshIndices;
  carray<int, LOD_COUNT + 1> totalLmeshVert;
  mem_set_0(totalLmeshVert);
  mem_set_0(totalLmeshIndices);
  int maxLodUsed = -1;
  int lmeshVdataCount = 0;
  int firstLmeshVdata = vdataCount;

  int lmeshCombinedCount = 0;
  int firstCombinedVdata = vdataCount;
  int maxCombinedStride = 0;

  G_ASSERT(vdataCount <= MAX_VDATA);
  for (int i = 0; i < vdataCount; ++i)
  {
    // debug("vdata %d %p", i, smvd->getGlobVData(i));
    if (!(vdataUsage[i] & (VDATA_COMBINED | VDATA_DECAL | VDATA_PATCHES)))
    {
      firstLmeshVdata = min(firstLmeshVdata, i);
      int vertCount = *(int *)smvd->getGlobVData(i)->getVBMem();
      lmeshVertexCount[i] = vertCount;
      int lodNo = get_log2w(vdataUsage[i]);
      maxLodUsed = max(lodNo, maxLodUsed);
      lmeshBaseVertex[i] = totalLmeshVert[lodNo];
      totalLmeshVert[lodNo] += vertCount;
      totalLmeshIndices[lodNo] += *(int *)smvd->getGlobVData(i)->getIBMem();
      lmeshVdataCount++;
    }
    if (vdataUsage[i] & VDATA_COMBINED)
    {
      maxCombinedStride = max(maxCombinedStride, smvd->getGlobVData(i)->getStride());
      firstCombinedVdata = min(firstCombinedVdata, i);
      int vertCount = *(int *)smvd->getGlobVData(i)->getVBMem();
      lmeshVertexCount[i] = vertCount;
      lmeshBaseVertex[i] = totalLmeshVert[maxLod + 1];
      totalLmeshVert[maxLod + 1] += vertCount;
      totalLmeshIndices[maxLod + 1] += *(int *)smvd->getGlobVData(i)->getIBMem();
      lmeshCombinedCount++;
    }
  }
  G_ASSERT(maxLod == maxLodUsed);

  G_ASSERT(lmeshVdataCount >= maxLodUsed + 1);

  struct FixedGvdStorage : public dag::Span<GlobalVertexData>
  {
    SmallTab<char, TmpmemAlloc> stor;
    FixedGvdStorage(unsigned n)
    {
      stor.resize(n * elem_size(*this));
      mem_set_0(stor);
      set((GlobalVertexData *)stor.data(), data_size(stor) / elem_size(*this));
    }
    void clear()
    {
      reset();
      clear_and_shrink(stor);
    }
  };

  FixedGvdStorage lmeshVertexData(maxLodUsed + 2);

  Tab<uint8_t> lmeshVData;
  Tab<uint8_t> lmeshIData;

  decalElems.resize(mapSizeY * mapSizeX);
  for (int z = 0, i = 0; z < mapSizeY; ++z)
  {
    for (int x = 0; x < mapSizeX; ++x, i++)
    {
      ShaderMesh *sm = cells[i].decal;
      if (!sm)
      {
        clear_and_shrink(decalElems[i].elemBoxes);
        continue;
      }
      clear_and_resize(decalElems[i].elemBoxes, sm->getAllElems().size());
      for (int ei = 0; ei < sm->getAllElems().size(); ++ei)
      {
        const ShaderMesh::RElem &re = sm->getAllElems()[ei];
        int srcStride = re.vertexData->getStride();
        G_ASSERT(srcStride >= 8);
        int vDataI = re.vertexData - startVD;
        int *vbData = (int *)re.vertexData->getVBMem();
        const uint8_t *vertSrcPtr = (const uint8_t *)(vbData + 1);
        vertSrcPtr += re.sv * srcStride;

        IBBox2 box;
        dag::ConstSpan<ShaderChannelId> channels = re.e->getChannels();
        if (channels.size())
        {
          switch (channels[0].t)
          {
            case SCTYPE_SHORT4N:
              for (int vi = 0; vi < re.numv; vi++, vertSrcPtr += srcStride)
              {
                int16_t *vertSrc = (int16_t *)vertSrcPtr;
                box += IPoint2(vertSrc[0], vertSrc[2]);
              }
              break;

            case SCTYPE_FLOAT3:
            case SCTYPE_FLOAT4:
              for (int vi = 0; vi < re.numv; vi++, vertSrcPtr += srcStride)
              {
                float *vertSrc = (float *)vertSrcPtr;
                box += IPoint2(vertSrc[0] * 32767, vertSrc[2] * 32767);
              }
              break;
          }

          IPoint2 cellOfs(32768 + x * 65535, 32768 + z * 65535);
          box[0] += cellOfs;
          box[1] += cellOfs;
        }

        decalElems[i].elemBoxes[ei] = box;
      }
    }
  }


  for (int lod = 0; lod <= maxLodUsed + 1; ++lod)
  {
    if (!totalLmeshVert[lod])
      continue;

    int newVdataId = lod <= maxLodUsed ? firstLmeshVdata + lod : firstCombinedVdata;

    int destStride = lod <= maxLodUsed ? 8 : maxCombinedStride; // 4 shorts each vertex

    lmeshVData.resize(totalLmeshVert[lod] * destStride);
    lmeshIData.resize(totalLmeshIndices[lod]);

    unsigned flags = VDATA_D3D_RESET_READY;
    int lodMask = lod <= maxLodUsed ? (VDATA_LOD0 << lod) : VDATA_COMBINED;
    for (int indicesOfs = 0, i = 0; i < vdataCount; ++i)
    {
      // debug("vdata %d %p", i, smvd->getGlobVData(i));
      if (vdataUsage[i] == lodMask)
      {
        int idxSize = *(int *)smvd->getGlobVData(i)->getIBMem();
        memcpy(lmeshIData.data() + indicesOfs, (int *)smvd->getGlobVData(i)->getIBMem() + 1, idxSize);
        lmeshBaseIndex[i] = indicesOfs;
        indicesOfs += idxSize;
      }
    }
    G_ASSERT(mapSizeY <= MAX_CELLS_WIDTH && mapSizeX <= MAX_CELLS_WIDTH);

    for (int z = 0, i = 0; z < mapSizeY; ++z)
      for (int x = 0; x < mapSizeX; ++x, i++)
      {
        ShaderMesh *sm = lod <= maxLodUsed ? cells[i].land[lod] : cells[i].combined;
        if (sm)
        {
          for (int ei = 0; ei < sm->getAllElems().size(); ++ei)
          {
            const ShaderMesh::RElem &re = sm->getAllElems()[ei];
            int srcStride = re.vertexData->getStride();
            G_ASSERT(srcStride == 8 || (lod > maxLodUsed && srcStride > 8));
            int vDataI = re.vertexData - startVD;
            int *vbData = (int *)re.vertexData->getVBMem();
            if (srcStride == 8) // lmesh, 4 shorts each vertex
            {
              G_ASSERT(lod <= maxLodUsed);
              G_ASSERT(vDataI >= firstLmeshVdata && vDataI < firstLmeshVdata + lmeshVdataCount);
              const int16_t *vertSrc = (const int16_t *)(vbData + 1);
              vertSrc += re.sv * 4; // 4 shorts each vertex
              int16_t *vertDst = (int16_t *)(lmeshVData.data() + destStride * (lmeshBaseVertex[vDataI] + re.sv));
              for (int vi = 0; vi < re.numv; vi++, vertSrc += 4, vertDst += 4)
              {
                vertDst[0] = vertSrc[0];
                vertDst[1] = vertSrc[1];
                vertDst[2] = vertSrc[2];
                vertDst[3] = z * MAX_CELLS_WIDTH + x;
              }
            }
            else // lmesh combined
            {
              G_ASSERT(lod > maxLodUsed);
              G_ASSERT(vDataI >= firstCombinedVdata && vDataI < firstCombinedVdata + lmeshCombinedCount);
              const uint8_t *vertSrcPtr = (const uint8_t *)(vbData + 1);
              vertSrcPtr += re.sv * srcStride;
              uint8_t *vertDstPtr = (lmeshVData.data() + destStride * (lmeshBaseVertex[vDataI] + re.sv));
              int tail = srcStride - 8;
              for (int vi = 0; vi < re.numv; vi++, vertSrcPtr += srcStride, vertDstPtr += destStride)
              {
                int16_t *vertDst = (int16_t *)vertDstPtr;
                int16_t *vertSrc = (int16_t *)vertSrcPtr;
                vertDst[0] = vertSrc[0];
                vertDst[1] = vertSrc[1];
                vertDst[2] = vertSrc[2];
                vertDst[3] = (z * MAX_CELLS_WIDTH + x) | ((vertSrc[3]) & 0x7000); // highest 3 bits are for transparency
                memcpy(vertDst + 4, vertSrc + 4, tail);
              }
            }
            const_cast<ShaderMesh::RElem &>(re).baseVertex = lmeshBaseVertex[vDataI];
            unsigned flag = (*vbData > 65536) ? VDATA_I32 : VDATA_I16;
            flags |= flag;
            const_cast<ShaderMesh::RElem &>(re).si += lmeshBaseIndex[vDataI] / (flag == VDATA_I32 ? 4 : 2);
            const_cast<ShaderMesh::RElem &>(re).vertexData = startVD + newVdataId;
          }
        }
      }
    G_ASSERT((flags & (VDATA_I32 | VDATA_I16)) != (VDATA_I32 | VDATA_I16));

    lmeshVertexData[lod].initGvdMem(totalLmeshVert[lod], destStride, lmeshIData.size(), flags, lmeshVData.data(), lmeshIData.data());
  }
  debug("re-encoded VB in %dus", get_time_usec(reft));

  FixedGvdStorage vdata(vdataCount);
  memcpy(vdata.data() + firstLmeshVdata, lmeshVertexData.data(), (maxLodUsed + 1) * sizeof(GlobalVertexData));
  if (totalLmeshVert[maxLodUsed + 1])
    memcpy(vdata.data() + firstCombinedVdata, lmeshVertexData.data() + maxLodUsed + 1, sizeof(GlobalVertexData));

  this->lmeshVdata = smvd->getGlobVData(0) + firstLmeshVdata;
  this->combinedVdata = totalLmeshVert[maxLodUsed + 1] ? smvd->getGlobVData(0) + firstCombinedVdata : NULL;

  debug("combined baseind = %d, basevert = %d", lmeshBaseIndex[firstCombinedVdata], lmeshBaseVertex[firstCombinedVdata]);
  int ibsz = 0, vbsz = 0;
  for (int i = 0; i < vdataCount; ++i)
  {
    debug("vdata %d %p", i, smvd->getGlobVData(i));
    int *ibData = (int *)smvd->getGlobVData(i)->getIBMem();
    int *vbData = (int *)smvd->getGlobVData(i)->getVBMem();
    // print first indices for future debug in renderdoc/pix
    if (smvd->getGlobVData(i)->getIbSize())
    {
      const int DEBUG_PAIR_COUNT_MAX = 4;
      const int idxStride = smvd->getGlobVData(i)->getIbElemSz();
      const int DEBUG_PAIR_COUNT = min(DEBUG_PAIR_COUNT_MAX, *ibData / idxStride / 2);
      debug("  ib size = %d, stride = %d, first %d indices:", *ibData, idxStride, DEBUG_PAIR_COUNT * 2);
      const int16_t *idxPtr16 = (int16_t *)(ibData + 1);
      const int *idxPtr32 = ibData + 1;
      for (int j = 0; j < 2 * DEBUG_PAIR_COUNT; j += 2)
      {
        const int firstIdx = idxStride == 2 ? idxPtr16[j] : idxPtr32[j];
        const int secondIdx = idxStride == 2 ? idxPtr16[j + 1] : idxPtr32[j + 1];
        debug("    0x%x, 0x%x", firstIdx, secondIdx);
      }
    }
    if (vdataUsage[i] & (VDATA_DECAL | VDATA_PATCHES))
    {
      G_ASSERT(i > firstLmeshVdata + maxLodUsed || i < firstLmeshVdata);
      G_ASSERT(i != firstCombinedVdata);
      vdata[i].initGvdMem(*vbData, smvd->getGlobVData(i)->getStride(), *ibData, VDATA_D3D_RESET_READY, vbData + 1, ibData + 1);
    }
  }
  for (int i = 0; i < vdata.size(); ++i)
    smvd->getGlobVData(i)->free();

  memcpy(smvd->getGlobVData(0), vdata.data(), data_size(vdata));
  vdata.clear();
  lmeshVertexData.clear();
  for (int i = 0; i < vdataCount; ++i)
  {
    if (!smvd->getGlobVData(i)->getVB())
      continue;
    ibsz += smvd->getGlobVData(i)->getIB()->ressize();
    vbsz += smvd->getGlobVData(i)->getVB()->ressize();
    debug("lmesh vbib %d: vb size = %d, stride=%d, ib size=%d %s", i, smvd->getGlobVData(i)->getVB()->ressize(),
      smvd->getGlobVData(i)->getStride(), smvd->getGlobVData(i)->getIB()->ressize(),
      smvd->getGlobVData(i)->getIB()->getFlags() & SBCF_INDEX32 ? "32bit" : "16bit");
  }
  debug("total vb size = %d, ib size=%d", vbsz, ibsz);

  ShaderGlobal::set_real(get_shader_variable_id("landCellSize", true), landCellSize);
  ShaderGlobal::set_color4(get_shader_variable_id("landCellShortDecodeXZ", true),
    Color4(0.5 / 32767. * landCellSize, landCellSize, landCellSize * (origin.x + 0.5f) + offset.x,
      landCellSize * (origin.y + 0.5f) + offset.z));
  ShaderGlobal::set_color4(get_shader_variable_id("landCellShortDecodeY", true), Color4(yscale / 32767., yofs, 0, 0));

  return true;
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

static inline TEXTUREID get_managed_texture_id_pp(const char *name)
{
  String tmp_storage;
  return ::get_managed_texture_id(IShaderMatVdataTexLoadCtrl::preprocess_tex_name(name, tmp_storage));
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
      erase_items(detailName, ext - &detailName[0], i_strlen(ext));
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

void LandMeshManager::afterDeviceReset(LandMeshRenderer *lrend, bool full_reset)
{
  if (full_reset)
  {
    int t0 = get_time_msec();

    // reload mesh data
    FullFileLoadCB crd(srcFileName);
    crd.seekto(srcFileMeshMapOfs);

    if (lrend)
      lrend->resetOptSceneAndStates();
    clear_and_shrink(cells);
    lmeshVdata = nullptr;
    combinedVdata = nullptr;
    smvd = nullptr;

    fatal_context_push("landMesh::mesh");
    loadMeshData(crd);
    fatal_context_pop();
    debug("reloaded land mesh data for %d msec (from %s:0x%x)", get_time_msec() - t0, srcFileName, srcFileMeshMapOfs);
  }

  for (auto &land : landClasses)
  {
    if (land.detailsCB)
    {
      if (!land.detailsCB->updateDataWithLock(0, data_size(land.lcDetails), land.lcDetails.data(), VBLOCK_WRITEONLY | VBLOCK_DISCARD))
      {
        if (!d3d::is_in_device_reset_now())
          logerr("Can't write constant buffer for landclass");
      }
    }
  }

  if (hmapHandler)
    hmapHandler->afterDeviceReset();
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

  String nm;
  for (int i = 0; i < landClassCount; ++i)
  {
    DataBlock blk;
    cb.beginBlock();
    cb.readString(nm);
    if (GameResource *land_cls_res =
          !toolsInternal ? ::get_one_game_resource_ex(GAMERES_HANDLE_FROM_STRING(nm), rendinst::HUID_LandClassGameRes) : NULL)
    {
      blk = rendinst::get_detail_data(land_cls_res);
      release_game_resource(land_cls_res);
      debug("landclass[%d] %s (res)", i, nm);
    }
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
    if (ends_with(landClasses[i].name, "_det") || (!strcmp(landClasses[i].shader_name, "landmesh_indexed_landclass") && inEditor > 0))
    {
      BaseTexture *lcTex = ::acquire_managed_tex(landClasses[i].lcTextures[0]);
      G_ASSERT_RETURN(lcTex != nullptr, );
      TextureInfo lcTexInfo;
      lcTex->getinfo(lcTexInfo);
      ::release_managed_tex(landClasses[i].lcTextures[0]);
      float landSize = safediv(1.0f, landClasses[i].tile);
      float worldLcTexelSize = landSize / lcTexInfo.w;

      ShaderGlobal::set_texture(::get_shader_variable_id("biomeIndicesTex", true), landClasses[i].lcTextures[0]);
      {
        d3d::SamplerInfo smpInfo;
        smpInfo.filter_mode = d3d::FilterMode::Point;
        smpInfo.mip_map_mode = d3d::MipMapMode::Disabled;
        ShaderGlobal::set_sampler(::get_shader_variable_id("biomeIndicesTex_samplerstate", true), d3d::request_sampler(smpInfo));
      }
      ShaderGlobal::set_color4(::get_shader_variable_id("biome_indices_tex_size", true), lcTexInfo.w, lcTexInfo.h, 1.0f / lcTexInfo.w,
        1.0f / lcTexInfo.h);
      ShaderGlobal::set_color4(::get_shader_variable_id("land_detail_mul_offset", true), landClasses[i].tile, -landClasses[i].tile,
        (landClasses[i].offset.x - offset.x + 0.5f * worldLcTexelSize) * landClasses[i].tile,
        (landClasses[i].offset.y - offset.z + 0.5f * worldLcTexelSize) * -landClasses[i].tile);

      biome_query::init_land_class(landClasses[i].lcTextures[0], landClasses[i].tile, landClasses[i].lcDetailGroupNames,
        landClasses[i].lcDetailGroupIndices);
      biome_query::set_details_cb(landClasses[i].detailsCB);
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
  ShaderGlobal::set_texture(::get_shader_variable_id("biomeDetailAlbedoTexArray", true), megaDetailsArrayId[0]);
}

void LandMeshManager::loadDetailData(IGenLoad &cb)
{
  loadLandClasses(cb);

  detailMap.load(cb, baseDataOffset, toolsInternal);
}

PhysMap *LandMeshManager::loadPhysMap(IGenLoad &loadCb, bool lmp2)
{
  PhysMap *physMap = nullptr;
  for (;;)
  {
    if (loadCb.tell() == loadCb.getTargetDataSize())
      break;
    int tag = loadCb.beginTaggedBlock();

    debug("[BIN] tag %c%c%c%c sz %dk", _DUMP4C(tag), loadCb.getBlockLength() >> 10);

    if (tag == _MAKE4C('LMpm') || tag == _MAKE4C('LMp2'))
    {
      debug("Loading LMpm");
      physMap = ::load_phys_map_with_decals(loadCb, tag == _MAKE4C('LMp2'));
      loadCb.endBlock();
      break;
    }
    loadCb.endBlock();
  }

  if (physMap)
    make_grid_decals(*physMap, 32);
  return physMap;
}

bool LandMeshManager::loadDump(IGenLoad &loadCb, IMemAlloc *rayTracerAllocator, bool load_render_data)
{
  dagor_set_sm_tex_load_ctx_type('LMAP');
  dagor_set_sm_tex_load_ctx_name(NULL);
  textag_clear_tag(TEXTAG_LAND);
  textag_mark_begin(TEXTAG_LAND);
  DAGOR_TRY
  {
    if (loadCb.readInt() != _MAKE4C('lndm'))
      DAGOR_THROW(IGenLoad::LoadException("Invalid file format", loadCb.tell()));

    fileVersion = loadCb.readInt();

    if (fileVersion != 4)
      DAGOR_THROW(IGenLoad::LoadException("Unsupported file version", loadCb.tell()));

    loadCb.readReal(gridCellSize);
    loadCb.readReal(landCellSize);
    loadCb.readInt(mapSizeX);
    loadCb.readInt(mapSizeY);
    loadCb.readInt(origin.x);
    loadCb.readInt(origin.y);
    offset.x = offset.y = offset.z = 0;

    int useTile = 0;

    loadCb.readInt(useTile);

    baseDataOffset = loadCb.tell();
    int meshMapOfs, detailDataOfs, tileDataOfs, rayTracerOfs;
    loadCb.readInt(meshMapOfs);
    loadCb.readInt(detailDataOfs);
    loadCb.readInt(tileDataOfs);
    loadCb.readInt(rayTracerOfs);

    meshMapOfs += baseDataOffset;
    detailDataOfs += baseDataOffset;
    tileDataOfs += baseDataOffset;
    rayTracerOfs += baseDataOffset;
    if (load_render_data)
    {
      fatal_context_push("landMesh::mesh");
      G_ASSERT(loadCb.tell() == meshMapOfs);
      // loadCb.seekto(meshMapOfs);
      srcFileMeshMapOfs = meshMapOfs;
      srcFileName = dagor_fname_map_add_fn(loadCb.getTargetName());
      loadMeshData(loadCb);
      fatal_context_pop();
      fatal_context_push("landMesh::detail");
      G_ASSERT(loadCb.tell() == detailDataOfs);
      // loadCb.seekto(detailDataOfs);
      loadDetailData(loadCb);
      fatal_context_pop();
    }
    else if (meshMapOfs >= baseDataOffset + 16 && detailDataOfs > meshMapOfs)
    {
      clear_and_resize(cellBoundings, mapSizeX * mapSizeY);
      clear_and_resize(cellBoundingsRadius, mapSizeX * mapSizeY);
      G_ASSERT(detailDataOfs - data_size(cellBoundings) - data_size(cellBoundingsRadius) >= meshMapOfs);

      loadCb.seekto(detailDataOfs - data_size(cellBoundings) - data_size(cellBoundingsRadius));
      loadCb.read(cellBoundings.data(), data_size(cellBoundings));
      loadCb.read(cellBoundingsRadius.data(), data_size(cellBoundingsRadius));

      landBbox.setempty();
      for (int i = 0; i < cellBoundings.size(); i++)
        landBbox += cellBoundings[i];
      if (hmapHandler)
        landBbox += hmapHandler->getWorldBox();
    }

    loadCb.seekto(tileDataOfs);
    if (useTile)
    {
      G_ASSERT(loadCb.tell() == tileDataOfs);
      // loadCb.seekto(tileDataOfs);

      String nm;
      loadCb.readString(nm);
      if (load_render_data)
      {
        tileTexId = ::get_managed_texture_id_pp(nm);
        ::acquire_managed_tex(tileTexId);
      }

      loadCb.readReal(tileXSize);
      loadCb.readReal(tileYSize);

      if (tileXSize < MIN_TILE_SIZE || tileYSize < MIN_TILE_SIZE)
      {
        logerr("tileSize=(%f, %f) < %f", tileXSize, tileYSize, MIN_TILE_SIZE);
        tileXSize = tileYSize = MIN_TILE_SIZE;
      }
    }
    if (rayTracerOfs > baseDataOffset)
    {
      G_ASSERTF(rayTracerOfs == loadCb.tell() + 4, "rayTracerOfs=%d, tell()=%d", rayTracerOfs, loadCb.tell());
      GlobalSharedMemStorage *sm = LandRayTracer::sharedMem;
      void *dump = NULL;
      int dump_sz = 0;
      bool need_load = true;

      if (sm && (dump = sm->findPtr(loadCb.getTargetName(), _MAKE4C('LRT'))) != NULL)
      {
        dump_sz = (int)sm->getPtrSize(dump);
        logmessage(_MAKE4C('SHMM'), "reusing LRT dump from shared mem: : %p, %dK, '%s'", dump, dump_sz >> 10, loadCb.getTargetName());
        landTracer = new LandRayTracer(rayTracerAllocator);
        landTracer->load(dump, dump_sz);
        loadCb.beginBlock();
        loadCb.endBlock();
      }
      else
      {
        unsigned fmt = 0;
        int c_sz = loadCb.beginBlock(&fmt);
        IGenLoad *zcrd_p = NULL;
        if (fmt == btag_compr::OODLE)
        {
          int src_sz = loadCb.readInt();
          zcrd_p = new (alloca(sizeof(OodleLoadCB)), _NEW_INPLACE) OodleLoadCB(loadCb, c_sz, src_sz);
        }
        else if (fmt == btag_compr::ZSTD)
          zcrd_p = new (alloca(sizeof(ZstdLoadCB)), _NEW_INPLACE) ZstdLoadCB(loadCb, c_sz);
        else
          zcrd_p = new (alloca(sizeof(LzmaLoadCB)), _NEW_INPLACE) LzmaLoadCB(loadCb, c_sz);
        IGenLoad &zcrd = *zcrd_p;

        landTracer = new LandRayTracer(rayTracerAllocator);

        dump_sz = zcrd.readInt();
        debug("LandRayTracer dump '%s' size=%d (compr %d)", loadCb.getTargetName(), dump_sz, c_sz);

        dump_sz += 7 * 16;
        if (sm)
        {
          dump = sm->allocPtr(loadCb.getTargetName(), _MAKE4C('LRT'), dump_sz);
          if (dump)
            logmessage(_MAKE4C('SHMM'), "allocated LRT dump in shared mem: %p, %dK, '%s' (mem %lluK/%lluK, rec=%d)", dump,
              dump_sz >> 10, loadCb.getTargetName(), ((uint64_t)sm->getMemUsed()) >> 10, ((uint64_t)sm->getMemSize()) >> 10,
              sm->getRecUsed());
          else
            logmessage(_MAKE4C('SHMM'),
              "failed to allocate LRT dump in shared mem: %p, %dK, '%s' (mem %lluK/%lluK, rec=%d); "
              "falling back to conventional allocator",
              dump, dump_sz >> 10, loadCb.getTargetName(), ((uint64_t)sm->getMemUsed()) >> 10, ((uint64_t)sm->getMemSize()) >> 10,
              sm->getRecUsed());
        }
        if (!dump && landTracer)
          dump = rayTracerAllocator->alloc(dump_sz);

        if (landTracer)
          landTracer->loadStreamToDump(dump, zcrd, dump_sz);

        zcrd.ceaseReading();
        zcrd.~IGenLoad();
        loadCb.endBlock();
      }
    }
    if (!load_render_data && landTracer && !cellBoundings.size())
    {
      clear_and_resize(cellBoundings, mapSizeX * mapSizeY);
      clear_and_resize(cellBoundingsRadius, mapSizeX * mapSizeY);
      landBbox = landTracer->getBBox();
      if (hmapHandler)
        landBbox += hmapHandler->getWorldBox();
      for (int i = 0, y = 0; y < mapSizeY; y++)
      {
        float cellZ = y * landCellSize + offset.z;
        for (int x = 0; x < mapSizeX; x++, i++)
        {
          float cellX = x * landCellSize + offset.x;
          cellBoundings[i] =
            BBox3(Point3(cellX, landBbox[0].y, cellZ), Point3(cellX + landCellSize, landBbox[1].y, cellZ + landCellSize));
          if (!cellBoundings[i].isempty())
            cellBoundingsRadius[i] = length(cellBoundings[i].width()) * 0.5f;
          else
            cellBoundingsRadius[i] = sqrtf(2.f) * landCellSize * 0.5f;
        }
      }
    }

    String vertDetTexName;
    String vertTexName, vertNmTexName;
    int vert_pos = loadCb.tell();
    G_UNREFERENCED(vert_pos);
    loadCb.readString(vertTexName);
    float verticalScaleXZ = 1.f, verticalScaleY = 1.f, blendFromY = 0.f, blendToY = 1.f, horizontalBlend = 0.f;
    float vertTexYOffset = 0.f, vertDetTexXZtile = 1.f, vertDetTexYtile = 1.f, vertDetTexYOffset = 0.f;
    bool has_vert_tex = vertTexName[0] != 0;

    debug("vertTexName =<%@> @0x%X has_vert_tex=%d", vertTexName, vert_pos, has_vert_tex);
    if (has_vert_tex)
    {
      verticalScaleXZ = loadCb.readReal();
      verticalScaleY = loadCb.readReal();
      blendFromY = loadCb.readReal();
      blendToY = loadCb.readReal();
      horizontalBlend = loadCb.readReal();

      vertTexYOffset = 0.f;
      vertDetTexXZtile = 1.f;
      vertDetTexYtile = 1.f;
      vertDetTexYOffset = 0.f;
      vertTexYOffset = loadCb.readReal();
      loadCb.readString(vertDetTexName);
      debug("vertDetTexName =<%@> @0x%X", vertDetTexName, loadCb.tell());
      vertDetTexXZtile = loadCb.readReal();
      vertDetTexYtile = loadCb.readReal();
      vertDetTexYOffset = loadCb.readReal();
    }
    if (load_render_data && has_vert_tex)
    {
      dagor_set_sm_tex_load_ctx_name("vertTex");
      if (vertTexName.length() && vertTexName[vertTexName.length() - 1] == '!')
      {
        useVertTexforHMAP = false;
        erase_items(vertTexName, vertTexName.length() - 1, 1);
      }
      else
        useVertTexforHMAP = true;
      if (char *p = strchr(vertTexName, ':'))
      {
        // new code path: both texture names are written to one string using ':' separator
        //                both texture names are always terminated with '*'
        vertNmTexName = p + 1;
        *p = 0;
        vertTexName.resize(p + 1 - vertTexName.data());
      }
      else
      {
        vertNmTexName = vertTexName;

        if (vertNmTexName.find("*"))
          vertNmTexName.replace("*", "_nm*");
        else
          vertNmTexName += "_nm*";

        if (vertTexName.length() > 0 && vertTexName[vertTexName.length() - 1] != '*')
          vertTexName += "*";
      }

      // if 'LC_SWAP_VERTICAL_DETAIL', then we should swap vertical detail textures (vertDetTexName instead of vertTexName)
      if (!(getRenderDataNeeded() & LC_SWAP_VERTICAL_DETAIL))
      {
        vertTexId = ::get_managed_texture_id_pp(vertTexName);
        ::acquire_managed_tex(vertTexId);
        vertTexSmp = d3d::request_sampler({});
      }

      vertNmTexId = BAD_TEXTUREID;
      vertDetTexId = BAD_TEXTUREID;
      if (getRenderDataNeeded() & (LC_DETAIL_DATA | LC_NORMAL_DATA))
      {
        vertNmTexId = ::get_managed_texture_id_pp(vertNmTexName);
        BaseTexture *tex = ::acquire_managed_tex(vertNmTexId);
        if (tex)
        {
          add_anisotropy_exception(vertNmTexId);
          d3d::SamplerInfo smpInfo;
          smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Wrap;
          smpInfo.anisotropic_max = 1;
          vertNmTexSmp = d3d::request_sampler(smpInfo);
        }

        vertDetTexId = ::get_managed_texture_id_pp(vertDetTexName);
        tex = ::acquire_managed_tex(vertDetTexId);
        if (tex)
        {
          d3d::SamplerInfo smpInfo;
          smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Wrap;
          vertDetTexSmp = d3d::request_sampler(smpInfo);
        }
      }
      else if (getRenderDataNeeded() & LC_SWAP_VERTICAL_DETAIL)
      {
        vertTexId = ::get_managed_texture_id_pp(vertDetTexName);
        BaseTexture *tex = ::acquire_managed_tex(vertTexId);
        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Wrap;
        vertTexSmp = d3d::request_sampler(smpInfo);
      }

      dagor_set_sm_tex_load_ctx_name(NULL);

      ShaderGlobal::set_color4(::get_shader_variable_id("vertical_param"), verticalScaleXZ, -verticalScaleY,
        1.f / (blendFromY - blendToY), -blendToY / (blendFromY - blendToY));

      ShaderGlobal::set_color4(::get_shader_variable_id("vertical_param_2"), horizontalBlend, vertTexYOffset, 0.f, 0.f);

      ShaderGlobal::set_color4(::get_shader_variable_id("vertical_param_3", true), // in compatibility this param does not exist
        vertDetTexXZtile, -vertDetTexYtile, vertDetTexYOffset, 0.f);


      if (getRenderDataNeeded() & LC_SWAP_VERTICAL_DETAIL)
      {
        ShaderGlobal::set_color4(::get_shader_variable_id("vertical_param"), vertDetTexXZtile, -vertDetTexYtile,
          1.f / (blendFromY - blendToY), -blendToY / (blendFromY - blendToY));
      }
    }
    else
    {
      if (load_render_data)
      {
        ShaderGlobal::set_color4(::get_shader_variable_id("vertical_param", true), 0.f, 0.f, 0.f, 1.f);
        ShaderGlobal::set_color4(::get_shader_variable_id("vertical_param_2", true), 0.f, 0.f, 0.f, 1.f);
        ShaderGlobal::set_color4(::get_shader_variable_id("vertical_param_3", true), 0.f, 0.f, 0.f, 1.f);
      }
      vertTexId = BAD_TEXTUREID;
      vertNmTexId = BAD_TEXTUREID;
      vertDetTexId = BAD_TEXTUREID;
    }
  }
  DAGOR_CATCH(const IGenLoad::LoadException &e)
  {
#ifdef DAGOR_EXCEPTIONS_ENABLED
    textag_mark_end();
    dagor_reset_sm_tex_load_ctx();
    logerr("Land dump load error: %s", e.excDesc);
    return false;
#endif
  }
  textag_mark_end();
  dagor_reset_sm_tex_load_ctx();
  if (is_managed_textures_streaming_load_on_demand())
    prefetch_managed_textures_by_textag(TEXTAG_LAND);

  return true;
}

bool LandMeshManager::loadHeightmapDump(IGenLoad &loadCb, bool load_render_data)
{
  del_it(hmapHandler);
  hmapHandler = new HeightmapHandler;
  if (load_render_data)
    hmapHandler->init();
  if (!hmapHandler->loadDump(loadCb, load_render_data, LandRayTracer::sharedMem))
  {
    del_it(hmapHandler);
    return false;
  }
  cullingState.exclBox = hmapHandler->getExcludeBounding();
  cullingState.exclBox[0] += getCellOrigin();
  cullingState.exclBox[1] += getCellOrigin();
  landBbox += hmapHandler->getWorldBox();
  cullingState.useExclBox = true;
  debug("load hmapHandler %@ %@", cullingState.exclBox, getCellOrigin());
  return true;
}

bool LandMeshManager::loadDump(const char *fname, int start_offset, bool load_render_data)
{
  close();

  const char *filename = ::df_get_real_name(fname);
  if (!filename)
  {
    logerr("Can't find real name for '%s'", fname);
    return false;
  }

  FullFileLoadCB loadCb(filename, DF_READ);
  if (!loadCb.fileHandle)
  {
    logerr("Can't open land dump file '%s'", filename);
    return false;
  }
  loadCb.seekto(start_offset);
  if (!loadDump(loadCb, midmem, load_render_data))
  {
    logerr("Can't load dump file '%s' at %d", filename, start_offset);
    return false;
  }
  return true;
}


LandMeshRenderer *LandMeshManager::createRenderer()
{
  LandMeshRenderer *renderer = new LandMeshRenderer(*this, landClasses, vertTexId, vertTexSmp, vertNmTexId, vertNmTexSmp, vertDetTexId,
    vertDetTexSmp, tileTexId, get_texture_separate_sampler(tileTexId), tileXSize, tileYSize);

  return renderer;
}


void LandMeshManager::getLandDetailTexture(int x, int y, TEXTUREID &tex1, TEXTUREID &tex2, uint8_t detail_tex_ids[DET_TEX_NUM])
{
  x -= origin.x;
  y -= origin.y;
  if (x < 0 || x >= detailMap.sizeX)
    return;
  if (y < 0 || y >= detailMap.sizeY)
    return;
  detailMap.getLandDetailTexture(x + y * detailMap.sizeX, tex1, tex2, detail_tex_ids);
}

BBox3 LandMeshManager::getBBox(int x, int y, float *sphere_radius)
{
  x -= origin.x;
  y -= origin.y;
  if (x < 0 || x >= mapSizeX || y < 0 || y >= mapSizeY)
    return BBox3();
  int i = x + y * mapSizeX;
  if (sphere_radius)
    *sphere_radius = cellBoundingsRadius[i];
  return cellBoundings[i];
}


//! Get height and normal
/// return false if not hit, true otherwise
bool LandMeshManager::getHeight(const Point2 &p, real &ht, Point3 *normal)
{
  if (holesMgr && holesMgr->check(p))
    return false;
  bool res = false;
  float landHt = MIN_REAL;
  if (landTracer && landTracer->getHeight(p, landHt, normal))
    res = true;
  float hmapHt = MIN_REAL;
  if (hmapHandler && hmapHandler->getHeight(p, hmapHt, normal))
    res = true;
  if (res)
    ht = max(hmapHt, landHt);
  return res;
}

//! Get max height below point and normal (trace ray down)
/// return false if not hit, true otherwise, return height of the point
bool LandMeshManager::getHeightBelow(const Point3 &p, float &ht, Point3 *normal)
{
  if (holesMgr && holesMgr->check(Point2::xz(p)))
    return false;
  bool res = false;
  float landHt = MIN_REAL;
  Point3 landNorm;
  if (landTracer && landTracer->getHeightBelow(p, landHt, normal ? &landNorm : NULL))
    res = true;
  float hmapHt = MIN_REAL;
  Point3 hmapNorm;
  if (hmapHandler && hmapHandler->getHeightBelow(p, hmapHt, normal ? &hmapNorm : NULL))
    res = true;
  if (res)
  {
    if (normal)
      *normal = hmapHt > landHt ? hmapNorm : landNorm;
    ht = max(hmapHt, landHt);
  }
  return res;
}

// return 0 if no intersect, 1 if intersect, -1 if error
int LandMeshManager::traceDownHmapMultiRay(dag::Span<Trace> traces)
{
  const int MAX_RAYS_COUNT = 80;
  carray<vec4f, MAX_RAYS_COUNT> rayStartPosVec;
  carray<vec3f, MAX_RAYS_COUNT> v_outNorm;
  G_ASSERT((traces.size() + 3) <= MAX_RAYS_COUNT);

  if (traces.size() + 3 > MAX_RAYS_COUNT || !traces.size() || !hmapHandler)
    return -1;

  bbox3f rayBox;
  trace_utils::prepare_traces(traces, rayBox, make_span(rayStartPosVec), make_span(v_outNorm));

  int ret = 0;
  if (hmapHandler)
  {
    int ret1 = hmapHandler->traceDownMultiRay(rayBox, rayStartPosVec.data(), v_outNorm.data(), traces.size());
    if (ret1 == -1)
      return -1;
    ret |= ret1;
  }
  if (ret)
  {
    trace_utils::store_traces(traces, make_span(rayStartPosVec), make_span(v_outNorm));
    return 1;
  }
  return 0;
}

//! Tests ray hit to closest object and returns parameters of hit (if happen)
/// return false if not hit, true otherwise
bool LandMeshManager::traceray(const Point3 &p, const Point3 &dir, real &t_out, Point3 *normal_out, bool cull)
{
  Point3 normal;
  float t = t_out;
  bool ret = landTracer && landTracer->traceRay(p, dir, t, &normal);
  if (hmapHandler && hmapHandler->traceray(p, dir, t, &normal, cull))
    ret = true;
  if (ret)
  {
    if (holesMgr)
    {
      Point3 intersection = p + dir * t;
      if (holesMgr->check(Point2::xz(intersection)))
      {
        const float EPS = 1e-2;
        float t_second = t_out - (t + EPS);
        ret = traceray(intersection + dir * EPS, dir, t_second, normal_out, cull);
        if (ret)
          t_out = t + EPS + t_second;
        return ret;
      }
    }
    if (normal_out)
      *normal_out = normal;
    t_out = t;
  }
  return ret;
}

//! Tests ray hit to any object and returns parameters of hit (if happen)
bool LandMeshManager::rayhitNormalized(const Point3 &p, const Point3 &normDir, real t)
{
  if (landTracer && landTracer->rayHit(p, normDir, t))
    return true;
  if (hmapHandler && hmapHandler->rayhitNormalized(p, normDir, t))
    return true;
  return false;
}

//! Tests ray hit to landmesh and tests if ray lies under landscape in any case and returns true is it is.
bool LandMeshManager::rayUnderNormalized(const Point3 &p, const Point3 &normDir, real t, real hmap_height_offs)
{
  if (hmapHandler && hmapHandler->rayUnderHeightmapNormalized(p + Point3(0.f, hmap_height_offs, 0.f), normDir, t))
    return true;
  if (landTracer && landTracer->rayHit(p, normDir, t))
    return true;
  return false;
}

void LandMeshManager::setGrassMaskBlk(const DataBlock &blk) { grassMaskBlk = blk; }

void LandMeshManager::setHmapLodDistance(int lodD)
{
  if (hmapHandler)
    hmapHandler->setLodDistance(lodD);
}

int LandMeshManager::getHmapLodDistance() const { return hmapHandler ? hmapHandler->getLodDistance() : 1; }

BBox3 LandMeshManager::getBBoxWithHMapWBBox() const
{
  BBox3 result = landBbox;
  if (getHmapHandler())
  {
    BBox3 hmapBox = getHmapHandler()->getWorldBox();
    // since we don't walk outside heightmap, let's limit maximum hmap to heightmap.
    result[0].y = hmapBox[0].y;
    result[1].y = hmapBox[1].y;
  }
  return result;
}
