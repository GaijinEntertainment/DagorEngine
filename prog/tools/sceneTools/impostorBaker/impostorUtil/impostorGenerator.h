// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>
#include <libTools/util/conLogWriter.h>
#include <EASTL/vector.h>

#include "impostorBaker.h"
#include "options.h"

class DataBlock;
class DagorAsset;
class DagorAssetMgr;

class ImpostorGenerator
{
public:
  // Simple way to interrupt from process from elsewhere
  static volatile bool interrupted;

  enum class GenerateResponse
  {
    PROCESS,
    SKIP,
    ABORT
  };
  // Callback: (asset, index, count) -> <continue_processing>
  using ExportImpostorsCB = eastl::function<GenerateResponse(DagorAsset *, uint32_t, uint32_t)>;

  ImpostorGenerator(const char *app_dir, DataBlock &app_blk, DagorAssetMgr *assetManager, ILogWriter *log_writer, bool display = true,
    ExportImpostorsCB callback = nullptr);
  ~ImpostorGenerator();

  bool run(const ImpostorOptions &options);
  void cleanUp(const ImpostorOptions &options);
  bool saveImpostorData();

  const ImpostorBaker *getImpostorBaker() const { return &impostorBaker; }
  ImpostorBaker *getImpostorBaker() { return &impostorBaker; }

  dag::Vector<DagorAsset *> gatherListForBaking(bool forceRebake = false);
  dag::Vector<DagorAsset *> gatherListForBakingWithPacks(const eastl::set<eastl::string> &packs, bool forceRebake = false);
  dag::Vector<DagorAsset *> gatherListForBakingFromArgs(const eastl::vector<eastl::string> &assetsToBuild);
  bool hasAssetChanged(DagorAsset *asset) const;
  // returns true if all data was gathered successfully (including baked textures)
  bool riDataBlock(DagorAsset *asset, DataBlock &blk) const;
  void gatherSourceFiles(DagorAsset *asset, eastl::set<String, ImpostorBaker::StrLess> &files) const;
  bool generateQualitySummary(const char *filename) const noexcept;
  bool logSkippedAssets() const;
  bool dumpChangedFiles(const char *filename) const;

private:
  DagorAssetMgr *assetManager;
  ExportImpostorsCB callback;
  ImpostorBaker impostorBaker;
  String impostorDataFile;
  String appDir;
  DataBlock appBlkCopy;
  DataBlock impostorDataBlk;
  const DataBlock *assetsBlk = nullptr;
  eastl::vector<TexturePackingProfilingInfo> qualitySummary;

  TEXTUREID characterMicrodetailsId = BAD_TEXTUREID;

  bool process(DagorAsset *asset);
  bool initAssetBase(const char *app_dir);

  static GenerateResponse asset_export_callback(const ImpostorBaker *impostor_baker, DagorAsset *asset, unsigned int ind,
    unsigned int count);

  void loadCharacterMicroDetails(const DataBlock *micro);
  void closeCharacterMicroDetails();

  void addProxyMatTextureSourceFiles(const char *src_file_path, const char *owner_asset_name,
    eastl::set<String, ImpostorBaker::StrLess> &files) const;
};
