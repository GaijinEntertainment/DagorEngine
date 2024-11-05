// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_tex3d.h>
#include <3d/dag_textureIDHolder.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint3.h>
#include <util/dag_baseDef.h>
#include <util/dag_string.h>
#include <render/deferredRenderer.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <3d/dag_resPtr.h>
#include <libTools/util/conLogWriter.h>

#include <EASTL/set.h>
#include <EASTL/vector.h>
#include <EASTL/unordered_map.h>

#include <rendInst/impostorTextureMgr.h>

#include "options.h"
#include "impostorTexturePacker.h"
#include <regex>

#define CALL_AT_END_OF_SCOPE_IMPL(func, ClassName, objName) \
  class ClassName                                           \
  {                                                         \
  public:                                                   \
    ~ClassName()                                            \
    {                                                       \
      func;                                                 \
    }                                                       \
  } objName;

#define CALL_AT_END_OF_SCOPE(func) \
  CALL_AT_END_OF_SCOPE_IMPL(func, DAG_CONCAT(ScopedFunction, __LINE__), DAG_CONCAT(scopedFunction, __LINE__))

class RenderableInstanceLodsResource;
class Sbuffer;
class DagorAsset;
class ILogWriter;

struct AOData
{
  Color4 ao1PosScale = Color4(0, 0, 0, 0);
  Color4 ao2PosScale = Color4(0, 0, 0, 0);
  Point4 crownCenter1 = {0, 0, 0, 0};
  Point4 crownRad1 = {0, 0, 0, 0};
  Point4 crownCenter2 = {0, 0, 0, 0};
  Point4 crownRad2 = {0, 0, 0, 0};
};

class ImpostorBaker
{
public:
  static const unsigned int NUM_SCALED_VERTICES = 8;

  explicit ImpostorBaker(ILogWriter *log_writer, bool displayExportedImages = false) noexcept;
  ~ImpostorBaker();

  struct ImpostorPackNamePattern
  {
    std::regex regex;
    String packNameSuffix;
  };

  struct ImpostorTextureData
  {
    UniqueTex albedo_alpha;
    UniqueTex normal_translucency;
    UniqueTex ao_smoothness;
    operator bool() const { return albedo_alpha && normal_translucency && ao_smoothness; }
  };
  struct ImpostorData
  {
    TexturePackingProfilingInfo quality;
    ImpostorTextureData textures;
  };

  struct StrLess
  {
    bool operator()(const String &lhs, const String &rhs) const { return strcmp(lhs.c_str(), rhs.c_str()) < 0; }
  };

  ImpostorData generate(DagorAsset *asset, const ImpostorTextureManager::GenerationData &gen_data, RenderableInstanceLodsResource *res,
    const String &asset_name, DataBlock *impostor_blk = nullptr);
  ImpostorData exportToFile(const ImpostorTextureManager::GenerationData &gen_data, DagorAsset *asset,
    RenderableInstanceLodsResource *res, bool force_rebake, DataBlock *impostor_blk = nullptr);
  TexturePackingProfilingInfo exportImpostor(DagorAsset *asset, const ImpostorOptions &options, DataBlock *impostor_blk = nullptr);
  void generateFolderBlk(DagorAsset *asset, const ImpostorOptions &options);
  bool generateAssetTexBlk(DagorAsset *asset, const ImpostorTextureManager::GenerationData &gen_data, const char *folder_path,
    const char *name);
  bool updateAssetTexBlk(DagorAsset *asset, int &out_processed, int &out_changed);
  bool hasSupportedType(DagorAsset *asset) const;
  bool isSupported(DagorAsset *asset) const;
  ska::flat_hash_map<eastl::string, int> getHashes(DagorAsset *asset) const;
  void gatherExportedFiles(DagorAsset *asset, eastl::set<String, StrLess> &files) const;

  // Removes all impostor textures that do not belong to any of the assets
  void clean(dag::ConstSpan<DagorAsset *> assets, const ImpostorOptions &options);

  static constexpr uint32_t GBUF_DIFFUSE_FMT = TEXFMT_A8R8G8B8;
  static constexpr uint32_t GBUF_NORMAL_FMT = TEXFMT_A8R8G8B8;
  static constexpr uint32_t GBUF_SHADOW_FMT = TEXFMT_A8R8G8B8;
  static constexpr uint32_t GBUF_DEPTH_FMT = TEXFMT_DEPTH16;
  static constexpr uint32_t EXP_ALBEDO_ALPHA_FMT = TEXFMT_A8R8G8B8;
  static constexpr uint32_t EXP_NORMAL_TRANSLUCENCY_FMT = TEXFMT_A8R8G8B8;
  static constexpr uint32_t EXP_AO_SMOOTHNESS_FMT = TEXFMT_R8G8;

  void set_objects_buffer();
  void setSplitAt(int split_at) { splitAt = split_at; }
  void setSplitAtMobile(int split_at) { splitAtMobile = split_at; }
  void setEnabledPreshadow(bool enabled) { preshadowsEnabled = enabled; }
  void setSmallestMip(int mip) { smallestMipSize = mip; }
  void setNormalMipOffset(int offset) { normalMipOffset = offset; }
  void setAOSmoothnessMipOffset(int offset) { aoSmoothnessMipOffset = offset; }
  void setMipOffsets(const IPoint3 &mip_offset_hq_mq_lq) { mipOffsets_hq_mq_lq = mip_offset_hq_mq_lq; }
  void setMobileMipOffsets(const IPoint3 &mobile_mip_offset_hq_mq_lq) { mobileMipOffsets_hq_mq_lq = mobile_mip_offset_hq_mq_lq; }
  void setDefaultTextureQualityLevels(eastl::vector<ImpostorTextureQualityLevel> default_texture_quality_levels);
  void setDefaultPackNamePatterns(eastl::vector<ImpostorPackNamePattern> default_pack_name_patterns);

  void setAOBrightness(float ao_brightness);
  void setAOFalloffStart(float ao_falloff_start);
  void setAOFalloffStop(float ao_falloff_stop);
  void setDDSXMaxTexturesNum(int ddsx_max_textures_num);

  void setSimilarityThreshold(float similarity_threshold);

  void computeDDSxPackSizes(dag::ConstSpan<DagorAsset *> assets);


  const eastl::set<eastl::string> &getModifiedFiles() const { return modifiedFiles; }
  const eastl::set<eastl::string> &getRemovedFiles() const { return removedFiles; }

  template <typename... Args>
  void conlog(const char *fmt, Args &&...args) const
  {
    logWriter->addMessage(ILogWriter::MessageType::NOTE, fmt, eastl::forward<Args>(args)...);
    debug(fmt, eastl::forward<Args>(args)...);
  }

  template <typename... Args>
  void conwarning(const char *fmt, Args &&...args) const
  {
    logWriter->addMessage(ILogWriter::MessageType::WARNING, fmt, eastl::forward<Args>(args)...);
    logwarn(fmt, eastl::forward<Args>(args)...);
  }

  template <typename... Args>
  void conerror(const char *fmt, Args &&...args) const
  {
    logWriter->addMessage(ILogWriter::MessageType::ERROR, fmt, eastl::forward<Args>(args)...);
    logerr(fmt, eastl::forward<Args>(args)...);
  }

private:
  ILogWriter *logWriter = nullptr;
  UniqueBuf treeCrown;
  eastl::unique_ptr<DeferredRenderTarget> rt;
  eastl::unique_ptr<DeferredRenderTarget> maskRt;
  UniqueTexHolder impostorBranchMaskTex;
  eastl::set<eastl::string> exportedFolderBlks;
  eastl::set<eastl::string> modifiedFiles;
  eastl::set<eastl::string> removedFiles;
  int dynamic_impostor_texture_const_no;

  struct SliceExportData
  {
    // | tm.x 0    tm.z |
    // | 0    tm.y tm.w |
    // | 0    0    1    |
    Point4 transform;
    // in normalized slice space
    // (intersection(leftLine, Y=0).x, intersection(leftLine, Y=1).x, intersection(rightLine, Y=0).x, intersection(rightLine, Y=1).x)
    Point4 clippingLines;
  };

  struct ImpostorExportData
  {
    eastl::vector<Point2> vertexOffset;
    eastl::vector<SliceExportData> sliceData;
  };

  void exportAssetBlk(DagorAsset *asset, RenderableInstanceLodsResource *res, const ImpostorExportData &export_data, DataBlock &blk,
    const AOData &ao_data) const;
  void prepareTextures(const ImpostorTextureManager::GenerationData &gen_data) noexcept;
  ImpostorTextureData prepareRt(IPoint2 extent, String asset_name, int mips) noexcept;
  static IPoint2 get_extent(const ImpostorTextureManager::GenerationData &gen_data);
  static IPoint2 get_rt_extent(const ImpostorTextureManager::GenerationData &gen_data);
  SaveResult saveImage(const char *filename, Texture *tex, int mip_offset, int num_channels, float &similarity, float threshold,
    bool force_rebake);
  float compareImages(TexImage32 *img1, TexImage32 *img2, int num_channels);

  bool displayExportedImages = false;
  // TODO set default quality options after one of the games is configured properly
  eastl::vector<ImpostorTextureQualityLevel> defaultTextureQualityLevels = {{0.f, 256}, {8.f, 512}};
  eastl::vector<ImpostorPackNamePattern> defaultPackNamePatterns = {};

  eastl::unordered_map<eastl::string, uint32_t> ddsxPackSizes;
  bool preshadowsEnabled = true;
  float bottomGradient = 0.0;
  int smallestMipSize = 4;
  int normalMipOffset = 0;
  int aoSmoothnessMipOffset = 0;
  int splitAt = 2048;
  int splitAtMobile = 512;
  int ddsxMaxTexturesNum = 128;
  IPoint3 mipOffsets_hq_mq_lq = IPoint3(0, 1, 2);
  IPoint3 mobileMipOffsets_hq_mq_lq = IPoint3(0, 1, 2);
  float aoBrightness = 0.7, aoFalloffStart = 0.25, aoFalloffStop = 1.0;

  // Similarity metric is PSNR, measured in dB.
  // Min value is 0.0, theoretical max is inf, but usually less 100.0.
  // Greater values mean more similar images.
  // 30.0 is a good default threshold value for 8-bit textures.
  float similarityThreshold = 30.0f;

  static constexpr int SLICE_SIZE = 1024;
  String getAssetFolder(const DagorAsset *asset) const;
  void generateFolderBlk(const char *folder, const char *dxp_prefix, const ImpostorOptions &options);
  eastl::string getDDSxPackName(const char *folder_path);
  AOData generateAOData(RenderableInstanceLodsResource *res, const DataBlock &blk_in);
};
