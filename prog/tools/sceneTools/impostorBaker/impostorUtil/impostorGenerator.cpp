// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "impostorGenerator.h"

#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_stdGameResId.h>
#include <generic/dag_span.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_vromfs.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_base64.h>
#include <util/dag_strUtil.h>
#include <util/dag_texMetaData.h>
#include <de3_interface.h>
#include <de3_objEntity.h>
#include <rendInst/rendInstGen.h>
#include <EditorCore/ec_wndGlobal.h>
#include <osApiWrappers/dag_files.h>

#include <libTools/shaderResBuilder/processMat.h>
#include <libTools/shaderResBuilder/shaderMeshData.h>

#include <assets/asset.h>
#include <assets/assetExpCache.h>
#include <assets/assetExporter.h>
#include <assets/assetHlp.h>
#include <assets/assetMgr.h>
#include <assets/assetRefs.h>

#include <fstream>

#define CSV_HEADER                                                                                                              \
  "Asset;Type;Width;Height;Slice Width;Slice Height;Quality improvement;X scale; Min Y scale; Max Y scale; Albedo Similarity, " \
  "Albedo Baked, Normal Similarity, Normal Baked, AO Similarity, AO Baked\n"

volatile bool ImpostorGenerator::interrupted = false;

#define GLOBAL_VARS_LIST VAR(impostor_normal_mip)

#define VAR(a) static ShaderVariableInfo a##VarId(#a);
GLOBAL_VARS_LIST
#undef VAR

static struct Context
{
  IDagorAssetRefProvider *rendintsRefProvider = nullptr;
  DagorAssetMgr *assetMgr = nullptr;
  ImpostorGenerator *app = nullptr; // used for the callback
} context;

static void close_acquired_texture(TEXTUREID &id)
{
  if (id == BAD_TEXTUREID)
    return;
  ShaderGlobal::reset_from_vars(id);
  release_managed_tex(id);
  id = BAD_TEXTUREID;
}

extern TEXTUREID load_texture_array_immediate(const char *name, const char *param_name, const DataBlock &blk, int &count);

void ImpostorGenerator::loadCharacterMicroDetails(const DataBlock *micro)
{
  closeCharacterMicroDetails();

  if (!micro)
    return;

  int microDetailCount = 0;
  characterMicrodetailsId = load_texture_array_immediate("character_micro_details*", "micro_detail", *micro, microDetailCount);
  ShaderGlobal::set_texture(get_shader_variable_id("character_micro_details"), characterMicrodetailsId);
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.anisotropic_max = ::dgs_tex_anisotropy;
    ShaderGlobal::set_sampler(get_shader_variable_id("character_micro_details_samplerstate", true), d3d::request_sampler(smpInfo));
  }
  ShaderGlobal::set_int(get_shader_variable_id("character_micro_details_count", true), microDetailCount);
}

void ImpostorGenerator::closeCharacterMicroDetails() { close_acquired_texture(characterMicrodetailsId); }

ImpostorGenerator::ImpostorGenerator(const char *app_dir, DataBlock &app_blk, DagorAssetMgr *asset_manager, ILogWriter *log_writer,
  bool display, ExportImpostorsCB _callback) :
  assetManager(asset_manager), callback(_callback), impostorBaker(log_writer, display)
{
  appBlkCopy.setFrom(&app_blk, app_blk.resolveFilename());
  context.app = this;
  assetsBlk = appBlkCopy.getBlockByNameEx("assets");
  const DataBlock *game_blk = appBlkCopy.getBlockByName("game");
  const DataBlock *impostorBlock = assetsBlk->getBlockByName("impostor");

  appDir = app_dir;

  String folder;
  if (impostorBlock)
  {
    G_ASSERTF(impostorBlock->paramExists("data_folder"), "Add data_folder:t to the assets/impostor block in application.blk");
    folder = String(0, "%s/%s/", app_dir, impostorBlock->getStr("data_folder"));
    int readParams = 1; // for data_folder
    int readBlocks = 0;
    if (impostorBlock->paramExists("splitAt"))
    {
      impostorBaker.setSplitAt(impostorBlock->getInt("splitAt"));
      readParams++;
    }
    if (impostorBlock->paramExists("splitAtMobile"))
    {
      impostorBaker.setSplitAtMobile(impostorBlock->getInt("splitAtMobile"));
      readParams++;
    }
    if (impostorBlock->paramExists("smallestMip"))
    {
      impostorBaker.setSmallestMip(impostorBlock->getInt("smallestMip"));
      readParams++;
    }
    if (impostorBlock->paramExists("preshadowEnabled"))
    {
      impostorBaker.setEnabledPreshadow(impostorBlock->getBool("preshadowEnabled"));
      readParams++;
    }
    if (impostorBlock->paramExists("normalMipOffset"))
    {
      impostorBaker.setNormalMipOffset(impostorBlock->getInt("normalMipOffset"));
      readParams++;
    }
    if (impostorBlock->paramExists("aoSmoothnessMipOffset"))
    {
      impostorBaker.setAOSmoothnessMipOffset(impostorBlock->getInt("aoSmoothnessMipOffset"));
      readParams++;
    }
    if (impostorBlock->paramExists("mipOffsets_hq_mq_lq"))
    {
      impostorBaker.setMipOffsets(impostorBlock->getIPoint3("mipOffsets_hq_mq_lq"));
      readParams++;
    }
    if (impostorBlock->paramExists("mobileMipOffsets_hq_mq_lq"))
    {
      impostorBaker.setMobileMipOffsets(impostorBlock->getIPoint3("mobileMipOffsets_hq_mq_lq"));
      readParams++;
    }
    bool hasDefaultHeight = impostorBlock->paramExists("defaultTextureHeight");
    if (hasDefaultHeight)
    {
      ImpostorTextureQualityLevel grade;
      grade.minHeight = 0;
      grade.textureHeight = impostorBlock->getInt("defaultTextureHeight");
      impostorBaker.setDefaultTextureQualityLevels({grade});
      readParams++;
    }
    if (impostorBlock->paramExists("aoBrightness"))
    {
      impostorBaker.setAOBrightness(impostorBlock->getReal("aoBrightness"));
      readParams++;
    }
    if (impostorBlock->paramExists("aoFalloffStart"))
    {
      impostorBaker.setAOFalloffStart(impostorBlock->getReal("aoFalloffStart"));
      readParams++;
    }
    if (impostorBlock->paramExists("aoFalloffStop"))
    {
      impostorBaker.setAOFalloffStop(impostorBlock->getReal("aoFalloffStop"));
      readParams++;
    }
    if (impostorBlock->paramExists("similarityThreshold"))
    {
      impostorBaker.setSimilarityThreshold(impostorBlock->getReal("similarityThreshold"));
      readParams++;
    }
    if (const DataBlock *textureQualityLevels = impostorBlock->getBlockByName("textureQualityLevels"))
    {
      G_ASSERTF(!hasDefaultHeight,
        "impostor/defaultTextureHeight and impostor/textureQualityLevels should not be specified at the same time");
      int count = textureQualityLevels->blockCount();
      eastl::vector<ImpostorTextureQualityLevel> qualityLevels;
      qualityLevels.reserve(count);
      for (int i = 0; i < count; ++i)
      {
        const DataBlock *qualityLevelBlk = textureQualityLevels->getBlock(i);
        G_ASSERTF(strcmp(qualityLevelBlk->getBlockName(), "qualityLevel") == 0 && qualityLevelBlk->paramCount() == 2 &&
                    qualityLevelBlk->paramExists("minHeight") && qualityLevelBlk->paramExists("textureHeight"),
          "The block impostor/textureQualityLevels should only contain blocks of 'grade {minHeight:r=...; textureHeight:i=... }'");
        ImpostorTextureQualityLevel qualityLevel;
        qualityLevel.textureHeight = qualityLevelBlk->getInt("textureHeight");
        qualityLevel.minHeight = qualityLevelBlk->getReal("minHeight");
        qualityLevels.push_back(qualityLevel);
      }
      impostorBaker.setDefaultTextureQualityLevels(eastl::move(qualityLevels));
      readBlocks++;
    }
    if (const DataBlock *ddsxPackNamePatterns = impostorBlock->getBlockByName("ddsxPackNamePatterns"))
    {
      int count = ddsxPackNamePatterns->blockCount();
      eastl::vector<ImpostorBaker::ImpostorPackNamePattern> packNamePatterns;
      packNamePatterns.reserve(count);
      for (int i = 0; i < count; ++i)
      {
        const DataBlock *packNamePatternsBlk = ddsxPackNamePatterns->getBlock(i);
        ImpostorBaker::ImpostorPackNamePattern packNamePattern;
        packNamePattern.regex = std::regex(packNamePatternsBlk->getStr("regex", ""), std::regex::extended | std::regex::icase);
        packNamePattern.packNameSuffix = packNamePatternsBlk->getStr("suffix", "");
        packNamePatterns.push_back(packNamePattern);
      }
      impostorBaker.setDefaultPackNamePatterns(eastl::move(packNamePatterns));
      readBlocks++;
    }
    if (impostorBlock->paramExists("ddsxMaxTexturesNum"))
    {
      impostorBaker.setDDSXMaxTexturesNum(impostorBlock->getInt("ddsxMaxTexturesNum"));
      readParams++;
    }
    if (impostorBlock->paramCount() != readParams || impostorBlock->blockCount() != readBlocks)
      impostorBaker.conerror("There are unknown param(s) or block(s) in the application.blk assets/impostor block");
  }
  else
  {
    G_ASSERTF(assetsBlk->paramExists("impostor_data_folder"), "Add the assets/impostor block to application.blk");
    folder = String(0, "%s/%s/", app_dir, assetsBlk->getStr("impostor_data_folder"));
  }

  context.assetMgr = assetManager;
  context.rendintsRefProvider = context.assetMgr->getAssetRefProvider(context.assetMgr->getAssetTypeId("rendinst"));
  G_ASSERT(context.rendintsRefProvider);
  simplify_fname(folder);
  impostorDataFile = String(0, "%simpostor_data.impostorData.blk", folder.c_str());
  if (::dd_file_exists(impostorDataFile.c_str()))
    impostorDataBlk.load(impostorDataFile.c_str());

  // Character microdetails are not used currently
  // loadCharacterMicroDetails(character_micro_detailsBlock.getBlockByName("character_micro_details"));
}

ImpostorGenerator::~ImpostorGenerator()
{
  closeCharacterMicroDetails();
  if (context.app == this)
    context.app = nullptr;
}

ImpostorGenerator::GenerateResponse ImpostorGenerator::asset_export_callback(const ImpostorBaker *impostor_baker, DagorAsset *asset,
  unsigned int ind, unsigned int count)
{
  if (ImpostorGenerator::interrupted)
    return GenerateResponse::ABORT;
  String title(0, "[%d/%d] Processing %s", ind + 1, count, asset->getName());
  ::win32_set_window_title(title);
  impostor_baker->conlog("%s", title.c_str());
  return GenerateResponse::PROCESS;
}

void ImpostorGenerator::cleanUp(const ImpostorOptions &options)
{
  dag::ConstSpan<DagorAsset *> assets = assetManager->getAssets();
  impostorBaker.clean(assets, options);
  for (int i = 0; i < assets.size(); ++i)
  {
    if (!impostorBaker.hasSupportedType(assets[i]))
      continue;
    if (!impostorBaker.isSupported(assets[i]))
    {
      if (impostorDataBlk.getBlockByName(assets[i]->getName()) != nullptr)
      {
        impostorBaker.conlog("Removing impostor data <%s> from impostor_data.bin", assets[i]->getName());
        if (!options.dryMode)
          impostorDataBlk.removeBlock(assets[i]->getName());
      }
    }
  }
  saveImpostorData();
}

dag::Vector<DagorAsset *> ImpostorGenerator::gatherListForBaking(bool forceRebake)
{
  dag::ConstSpan<DagorAsset *> assets = assetManager->getAssets();
  dag::Vector<DagorAsset *> assetsForBaking = {};
  for (auto &asset : assets)
    if (impostorBaker.isSupported(asset) && (hasAssetChanged(asset) || forceRebake))
      assetsForBaking.push_back(asset);
  return assetsForBaking;
}

dag::Vector<DagorAsset *> ImpostorGenerator::gatherListForBakingWithPacks(const eastl::set<eastl::string> &packs, bool forceRebake)
{
  dag::ConstSpan<DagorAsset *> assets = assetManager->getAssets();
  dag::Vector<DagorAsset *> assetsForBaking = {};
  for (auto &asset : assets)
    if (impostorBaker.isSupported(asset) && packs.count(eastl::string{asset->getDestPackName()}) > 0 &&
        (hasAssetChanged(asset) || forceRebake))
      assetsForBaking.push_back(asset);
  return assetsForBaking;
}

dag::Vector<DagorAsset *> ImpostorGenerator::gatherListForBakingFromArgs(const eastl::vector<eastl::string> &assetsToBuild)
{
  dag::Vector<DagorAsset *> assetsForBaking = {};
  for (const eastl::string &assetName : assetsToBuild)
  {
    DagorAsset *asset = DAEDITOR3.getAssetByName(assetName.c_str(), DAEDITOR3.getAssetTypeId("rendInst"));
    if (!asset)
    {
      impostorBaker.conerror("Asset could not be found: %s", assetName.c_str());
    }
    else
    {
      if (!impostorBaker.isSupported(asset))
      {
        impostorBaker.conerror("Asset not supported: %s", assetName.c_str());
      }
      else
      {
        impostorBaker.conlog("Baking asset <%s>, because it's requested in args", asset->getName());
        assetsForBaking.push_back(asset);
      }
    }
  }
  return assetsForBaking;
}

bool ImpostorGenerator::run(const ImpostorOptions &options)
{
  interrupted = false;
  if (options.clean)
    cleanUp(options);
  unsigned int count = 0;
  unsigned int ind = 0;

  impostorBaker.computeDDSxPackSizes(assetManager->getAssets());
  {
    int a_num = 0, a_changed = 0;
    for (auto *asset : assetManager->getAssets())
      if (impostorBaker.isSupported(asset))
        impostorBaker.updateAssetTexBlk(asset, a_num, a_changed);
    impostorBaker.conlog("Force update *.tex.blk: %d (%d changed)", a_num, a_changed);
    if (options.skipGen)
      return true;
  }

  const auto processAsset = [&, this](DagorAsset *asset, const char *name) {
    const GenerateResponse response =
      callback ? callback(asset, ind++, count) : asset_export_callback(getImpostorBaker(), asset, ind++, count);
    if (response != GenerateResponse::ABORT)
      impostorBaker.generateFolderBlk(asset, options);
    switch (response)
    {
      case GenerateResponse::ABORT: return false;
      case GenerateResponse::SKIP: return true;
      case GenerateResponse::PROCESS: break;
    }
    DataBlock *impostorBlk = impostorDataBlk.addBlock(asset->getName());
    int impostorNormalMipNumber = 0;
    if (DataBlock *assetImpostorBlock = asset->props.getBlockByName("impostor"))
      if (assetImpostorBlock->paramExists("impostorNormalMip"))
        impostorNormalMipNumber = assetImpostorBlock->getInt("impostorNormalMip");
    ShaderGlobal::set_int(impostor_normal_mipVarId, impostorNormalMipNumber);
    TexturePackingProfilingInfo quality = impostorBaker.exportImpostor(asset, options, impostorBlk->addBlock("content"));
    if (!quality)
    {
      impostorBaker.conerror("Could not process the rendinst: %s", name);
      return false;
    }
    qualitySummary.push_back(quality);
    riDataBlock(asset, *impostorBlk);
    return true;
  };

  dag::Vector<DagorAsset *> assets = {};
  if (options.assetsToBuild.size() > 0)
  {
    assets = gatherListForBakingFromArgs(options.assetsToBuild);
  }
  if (options.packsToBuild.size() > 0)
  {
    eastl::set<eastl::string> packs;
    eastl::copy(eastl::begin(options.packsToBuild), eastl::end(options.packsToBuild), eastl::inserter(packs, eastl::end(packs)));
    assets = gatherListForBakingWithPacks(packs, options.forceRebake);
  }

  if (options.assetsToBuild.size() == 0 && options.packsToBuild.size() == 0)
  {
    assets = gatherListForBaking(options.forceRebake);
  }
  impostorBaker.conlog("Total number assets for baking: %d", assets.size());
  count = assets.size();
  if (!options.dryMode)
    for (int i = 0; i < assets.size(); i++)
    {
      if (!processAsset(assets[i], assets[i]->getName()))
        return false;
    }
  else
    impostorBaker.conlog("In dry mode we don't proccess the assets");

  if (!options.changedFileOutput.empty())
  {
    if (!dumpChangedFiles(options.changedFileOutput.c_str()))
    {
      impostorBaker.conerror("Could not export list of changed files");
      return false;
    }
  }
  impostorBaker.conlog("ImpostorBaker: Baking proccess is ended!");
  return saveImpostorData();
}

struct QualityStats
{
  double bestValue = -1;
  double worstValue = 1;
  double qualitySum = 0;
  uint32_t count = 0;
  uint64_t arrayPixelCountSum = 0;
  uint64_t packedPixelCountSum = 0;
  double totalQualityImprovement = 0;
  void apply(const TexturePackingProfilingInfo &quality)
  {
    worstValue = eastl::min(worstValue, quality.qualityImprovement);
    bestValue = eastl::max(bestValue, quality.qualityImprovement);
    qualitySum += quality.qualityImprovement;
    count++;
    arrayPixelCountSum += quality.arrayTexPixelCount;
    packedPixelCountSum += quality.packedTexPixelCount;
    totalQualityImprovement = sqrt(double(arrayPixelCountSum) / double(packedPixelCountSum)) - 1.0;
  }
};

static void print_quality_stat_summary(const QualityStats &stats, file_ptr_t csvFile)
{
  df_printf(csvFile, "Best quality;Worst quality;Avg quality;Overall quality gain\n");
  df_printf(csvFile, "%s%f%c;%s%f%c;%s%f%c;%s%f%c\n", stats.bestValue < 0 ? "" : "+", float(stats.bestValue * 100.0), '%',
    stats.worstValue < 0 ? "" : "+", float(stats.worstValue * 100.0), '%', stats.qualitySum < 0 ? "" : "+",
    float((stats.qualitySum / stats.count) * 100.0), '%', stats.totalQualityImprovement < 0 ? "" : "+",
    float(stats.totalQualityImprovement * 100.0), '%');
  df_printf(csvFile, "\n");
}

bool ImpostorGenerator::generateQualitySummary(const char *filename) const noexcept
{
  file_ptr_t csvFile = df_open(filename, DF_WRITE | DF_CREATE);
  if (!csvFile)
    return false;
  if (qualitySummary.empty())
  {
    df_printf(csvFile, "No impostors were exported");
    df_close(csvFile);
    return true;
  }
  QualityStats bushStats;
  QualityStats treeStats;
  for (const TexturePackingProfilingInfo &quality : qualitySummary)
  {
    if (quality.type == TexturePackingProfilingInfo::TREE)
      treeStats.apply(quality);
    else if (quality.type == TexturePackingProfilingInfo::BUSH)
      bushStats.apply(quality);
  }
  if (treeStats.count > 0)
  {
    df_printf(csvFile, "Trees\n");
    print_quality_stat_summary(treeStats, csvFile);
  }
  if (bushStats.count > 0)
  {
    df_printf(csvFile, "Bushes\n");
    print_quality_stat_summary(bushStats, csvFile);
  }
  if (treeStats.count > 0 || bushStats.count > 0)
    df_printf(csvFile, "\n");
  df_printf(csvFile, CSV_HEADER);
  for (const TexturePackingProfilingInfo &quality : qualitySummary)
  {
    const char *typeStr = "Unknown";
    if (quality.type == TexturePackingProfilingInfo::TREE)
      typeStr = "Tree";
    else if (quality.type == TexturePackingProfilingInfo::BUSH)
      typeStr = "Bush";
    df_printf(csvFile, "%s;%s;%d;%d;%d;%d;%s%f%c;%f;%f;%f;%f;%d;%f;%d;%f;%d;\n", quality.assetName.c_str(), typeStr, quality.texWidth,
      quality.texHeight, quality.sliceWidth, quality.sliceHeight, quality.qualityImprovement < 0 ? "" : "+",
      float(quality.qualityImprovement * 100.0), '%', quality.sliceStretchX, quality.sliceStretchY_min, quality.sliceStretchY_max,
      quality.albedoAlphaSimilarity, int(quality.albedoAlphaBaked), quality.normalTranslucencySimilarity,
      int(quality.normalTranslucencyBaked), quality.aoSmoothnessSimilarity, int(quality.aoSmoothnessBaked));
  }
  df_close(csvFile);

  int albedoSkippedCounter = 0;
  int normalSkippedCounter = 0;
  int aoSkippedCounter = 0;
  for (const TexturePackingProfilingInfo &quality : qualitySummary)
  {
    if (quality.albedoAlphaBaked == SaveResult::SKIPPED)
      albedoSkippedCounter++;
    if (quality.normalTranslucencyBaked == SaveResult::SKIPPED)
      normalSkippedCounter++;
    if (quality.aoSmoothnessBaked == SaveResult::SKIPPED)
      aoSkippedCounter++;
  }
  impostorBaker.conlog("Albedo skipped: %d/%d", albedoSkippedCounter, qualitySummary.size());
  impostorBaker.conlog("Normal skipped: %d/%d", normalSkippedCounter, qualitySummary.size());
  impostorBaker.conlog("AO skipped: %d/%d", aoSkippedCounter, qualitySummary.size());

  return true;
}

bool ImpostorGenerator::saveImpostorData() { return impostorDataBlk.saveToTextFile(impostorDataFile.c_str()); }

void ImpostorGenerator::addProxyMatTextureSourceFiles(const char *src_file_path, const char *owner_asset_name,
  eastl::set<String, ImpostorBaker::StrLess> &files) const
{
  static const int proxyMatTypeId = assetManager->getAssetTypeId("proxymat");
  static const int texTypeId = assetManager->getTexAssetTypeId();

  const String srcAssetName = DagorAsset::fpath2asset(src_file_path);
  if (!srcAssetName.suffix(".proxymat"))
    return;

  const String proxyMatName = srcAssetName.mk_sub_str(srcAssetName.begin(), srcAssetName.end() - 9);
  if (proxyMatName.empty())
    return;

  const DagorAsset *proxyMatAsset = assetManager->findAsset(proxyMatName, proxyMatTypeId);
  if (!proxyMatAsset)
  {
    impostorBaker.conwarning("Proxymat asset \"%s\" cannot be found.", proxyMatName.c_str());
    return;
  }

  dag::Vector<String> textureAssetNames;
  get_textures_used_by_proxymat(proxyMatAsset->props, owner_asset_name, textureAssetNames);

  for (const String &textureAssetName : textureAssetNames)
  {
    DagorAsset *textureAsset = context.assetMgr->findAsset(textureAssetName, texTypeId);
    if (!textureAsset)
    {
      impostorBaker.conwarning("Texture asset \"%s\" cannot be found.", textureAssetName.c_str());
      continue;
    }

    files.insert(textureAsset->getSrcFilePath());
  }
}

void ImpostorGenerator::gatherSourceFiles(DagorAsset *asset, eastl::set<String, ImpostorBaker::StrLess> &files) const
{
  if (!asset)
    return;
  IDagorAssetExporter *e = context.assetMgr->getAssetExporter(asset->getType());

  if (asset->isVirtual())
    files.insert(asset->getTargetFilePath());

  Tab<SimpleString> srcFiles(tmpmem);
  e->gatherSrcDataFiles(*asset, srcFiles);
  for (int i = 0; i < srcFiles.size(); ++i)
    files.insert(String(srcFiles[i]));

  for (const SimpleString &srcFilePath : srcFiles)
    addProxyMatTextureSourceFiles(srcFilePath, asset->getName(), files);
}

static void combine_hash(unsigned char target[AssetExportCache::HASH_SZ], const unsigned char hash[AssetExportCache::HASH_SZ])
{
  for (uint32_t i = 0; i < AssetExportCache::HASH_SZ; ++i)
    target[i] ^= hash[i];
}

static Base64 encode_hash(const unsigned char hash[AssetExportCache::HASH_SZ])
{
  Base64 base64;
  base64.encode(static_cast<const uint8_t *>(hash), AssetExportCache::HASH_SZ);
  return base64;
}

bool ImpostorGenerator::riDataBlock(DagorAsset *asset, DataBlock &blk) const
{
  if (!asset)
    return false;
  IDagorAssetExporter *e = context.assetMgr->getAssetExporter(asset->getType());
  G_ASSERT(e);
  eastl::set<String, ImpostorBaker::StrLess> exportedFiles;
  impostorBaker.gatherExportedFiles(asset, exportedFiles);
  unsigned char exportHash[AssetExportCache::HASH_SZ] = {0};

  for (const String &file : exportedFiles)
  {
    if (!::dd_file_exist(file.c_str()))
      return false;
    unsigned char h[AssetExportCache::HASH_SZ];
    AssetExportCache::getFileHash(file.c_str(), h);
    combine_hash(exportHash, h);
  }

  eastl::set<String, ImpostorBaker::StrLess> sourceFiles;
  gatherSourceFiles(asset, sourceFiles);
  unsigned char sourceHash[AssetExportCache::HASH_SZ] = {0};

  for (const String &file : sourceFiles)
  {
    unsigned char h[AssetExportCache::HASH_SZ];
    AssetExportCache::getFileHash(file.c_str(), h);
    combine_hash(sourceHash, h);
  }

  DynamicMemGeneralSaveCB cwr(tmpmem, 4 << 10, 4 << 10);
  asset->props.saveToTextStream(cwr);
  unsigned char propsHash[AssetExportCache::HASH_SZ];
  AssetExportCache::getDataHash(cwr.data(), cwr.size(), propsHash);

  blk.setStr("exportHash", encode_hash(exportHash).c_str());
  blk.setStr("sourceHash", encode_hash(sourceHash).c_str());
  blk.setStr("propsHash", encode_hash(propsHash).c_str());
  blk.setInt("impostorBakerVersion", ImpostorTextureManager::GenerationData::VERSION_NUMBER);
  DataBlock *hashesBlk = blk.addBlock("hashes");
  hashesBlk->clearData();
  for (const auto &itr : impostorBaker.getHashes(asset))
    hashesBlk->setInt(itr.first.c_str(), itr.second);
  return true;
}

bool ImpostorGenerator::hasAssetChanged(DagorAsset *asset) const
{
  if (!context.rendintsRefProvider)
  {
    static bool logged = false;
    if (!logged)
    {
      impostorBaker.conerror("Missing rendintsRefProvider, rebuilding all assets");
      logged = true;
    }
    return true;
  }
  IDagorAssetExporter *e = context.assetMgr->getAssetExporter(asset->getType());
  G_ASSERT(e);
  bool verbose = true;
  const DataBlock *storedBlock = impostorDataBlk.getBlockByName(asset->getName());
  if (storedBlock == nullptr)
  {
    if (verbose)
      impostorBaker.conlog("Baking asset <%s>, because it's not registered in impostor_data.impostorData.blk", asset->getName());
    return true;
  }
  DataBlock assetBlk;
  if (!riDataBlock(asset, assetBlk))
  {
    impostorBaker.conwarning("Baking asset <%s>, because exported textures are not present "
                             "(impostor_data.impostorData.blk might "
                             "be updated in cvs, but textures not)",
      asset->getName());
    return true;
  }
  const DataBlock *storedHashBlk = storedBlock->getBlockByName("hashes");
  const DataBlock *assetHashBlk = assetBlk.getBlockByName("hashes");
  G_ASSERT(assetHashBlk != nullptr);
  if (storedHashBlk == nullptr)
  {
    if (verbose)
      impostorBaker.conlog("Baking asset <%s>, because cache stores param hashes in old style", asset->getName());
    return true;
  }
  else if (storedHashBlk->paramCount() != assetHashBlk->paramCount())
  {
    if (verbose)
      impostorBaker.conlog("Baking asset <%s>, because cache stores a different number of parameters: %d != %d", asset->getName(),
        storedHashBlk->paramCount(), assetHashBlk->paramCount());
    return true;
  }
  else
  {
    int count = assetHashBlk->paramCount();
    for (int i = 0; i < count; ++i)
    {
      G_ASSERT(assetHashBlk->getParamType(i) == DataBlock::TYPE_INT);
      const char *paramName = assetHashBlk->getParamName(i);
      if (!storedHashBlk->paramExists(paramName))
      {
        if (verbose)
          impostorBaker.conlog("Baking asset <%s>, because cache doesn't store hash for param: %s", asset->getName(), paramName);
        return true;
      }
      if (storedHashBlk->getInt(paramName) != assetHashBlk->getInt(paramName))
      {
        if (verbose)
          impostorBaker.conlog("Baking asset <%s>, because cache stores different value for param hash %s: %d -> %d", asset->getName(),
            paramName, storedHashBlk->getInt(paramName), assetHashBlk->getInt(paramName));
        return true;
      }
    }
  }
  if (strcmp(assetBlk.getStr("exportHash", ""), storedBlock->getStr("exportHash", "-")) != 0)
  {
    impostorBaker.conwarning("Baking asset <%s>, because exported textures are inconsistent with "
                             "impostor_data.impostorData.blk "
                             "(maybe they are not updated in cvs)",
      asset->getName());
    return true;
  }
  if (strcmp(assetBlk.getStr("sourceHash", ""), storedBlock->getStr("sourceHash", "-")) != 0)
  {
    if (verbose)
      impostorBaker.conlog("Baking asset <%s>, because the source files have changed", asset->getName());
    return true;
  }
  if (strcmp(assetBlk.getStr("propsHash", ""), storedBlock->getStr("propsHash", "-")) != 0)
  {
    if (verbose)
      impostorBaker.conlog("Baking asset <%s>, because asset properties have changed", asset->getName());
    return true;
  }
  if (assetBlk.getInt("impostorBakerVersion", 0) != storedBlock->getInt("impostorBakerVersion", -1))
  {
    if (verbose)
      impostorBaker.conlog("Baking asset <%s>, because impostor baker version has changed", asset->getName());
    return true;
  }
  return false;
}

bool ImpostorGenerator::dumpChangedFiles(const char *filename) const
{
  ::dd_mkpath(filename);
  if (str_ends_with_c(filename, ".blk") != 0)
  {
    DataBlock outBlk;
    DataBlock *modified = outBlk.addBlock("modified");
    DataBlock *removed = outBlk.addBlock("removed");
    modified->addStr("file", impostorDataFile);
    for (const auto &itr : impostorBaker.getModifiedFiles())
      modified->addStr("file", itr.c_str());
    for (const auto &itr : impostorBaker.getRemovedFiles())
      removed->addStr("file", itr.c_str());
    return outBlk.saveToTextFile(filename);
  }
  else
  {
    std::ofstream outFile(filename);
    if (!outFile)
      return false;
    int numChanged = impostorBaker.getModifiedFiles().size() + 1; // +1 for impostorData
    outFile << "# Number of modified files\n";
    outFile << numChanged << std::endl;
    outFile << "# Number of removed files\n";
    outFile << impostorBaker.getRemovedFiles().size() << std::endl;
    outFile << "# Following lines: N absolute path to modified file; M absolute path to removed file\n";
    outFile << impostorDataFile << std::endl;
    for (const auto &itr : impostorBaker.getModifiedFiles())
      outFile << itr.c_str() << std::endl;
    for (const auto &itr : impostorBaker.getRemovedFiles())
      outFile << itr.c_str() << std::endl;
    return true;
  }
}
