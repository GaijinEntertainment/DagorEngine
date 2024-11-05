// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentTypes.h>
#include <dasModules/aotEcs.h>
#include <libTools/dagFileRW/dagMatRemapUtil.h>
#include <libTools/util/fileUtils.h>
#include <osApiWrappers/basePath.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include "../main/assetManager.h"

const Tab<IDagorAssetRefProvider::Ref> &get_asset_dependencies(DagorAsset &asset);

namespace bind_dascript
{
inline const char *asset_getSrcFilePath(const DagorAsset &a, das::Context *context, das::LineInfoArg *at)
{
  const String filePath = a.getSrcFilePath();
  return context->allocateString(filePath.c_str(), filePath.length(), at);
}

inline void iterate_dag_textures(
  const char *path_to_dag, const das::TBlock<void, const char *> &block, das::Context *context, das::LineInfoArg *at)
{
  DagData data;
  G_ASSERT_RETURN(load_scene(path_to_dag, data), );
  for (const String &tex : data.texlist)
  {
    vec4f arg = das::cast<const char *>::from(tex.c_str());
    context->invoke(block, &arg, nullptr, at);
  }
  for (DagData::Block &b : data.blocks)
    memfree(b.data, tmpmem); // todo: free somewhere in DagData dtor
}

inline bool copy_file(const char *src, const char *dest)
{
  // avoid relative path since it can be different but refer to single file. In this case copy will destroy its content
  if (!is_path_abs(src) || !is_path_abs(dest))
    return false;
  return dag_copy_file(src, dest);
}

inline void get_filtered_assets(const DagorAssetMgr &asset__manager,
  int asset_type_id,
  const das::TBlock<void, const das::TArray<int>> &block,
  das::Context *context,
  das::LineInfoArg *at)
{
  dag::ConstSpan<int> filteredAssets = asset__manager.getFilteredAssets(dag::ConstSpan<int>(&asset_type_id, 1));

  das::Array arr{(char *)filteredAssets.data(), (uint32_t)filteredAssets.size(), (uint32_t)filteredAssets.size(), 1, 0};

  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void get_asset_source_files(
  const DagorAsset &asset, const das::TBlock<void, const das::TArray<char *>> &block, das::Context *context, das::LineInfoArg *at)
{
  if (auto assetMgr = get_asset_manager())
  {
    if (IDagorAssetExporter *exporter = assetMgr->getAssetExporter(asset.getType()))
    {
      Tab<SimpleString> files(framemem_ptr());
      exporter->gatherSrcDataFiles(asset, files);
      Tab<const char *> filesSrc(framemem_ptr());
      filesSrc.resize(files.size());
      for (int i = 0, n = files.size(); i < n; i++)
        filesSrc[i] = files[i].c_str();

      das::Array arr{(char *)filesSrc.data(), (uint32_t)filesSrc.size(), (uint32_t)filesSrc.size(), 1, 0};

      vec4f arg = das::cast<das::Array *>::from(&arr);
      context->invoke(block, &arg, nullptr, at);
    }
  }
}

inline void get_asset_dependencies(
  DagorAsset &asset, const das::TBlock<void, const das::TArray<char *>> &block, das::Context *context, das::LineInfoArg *at)
{
  const auto &refList = get_asset_dependencies(asset);

  Tab<const char *> assetNames(framemem_ptr());
  assetNames.resize(refList.size());
  for (int i = 0, n = refList.size(); i < n; ++i)
  {
    if (auto refAsset = refList[i].getAsset())
      assetNames[i] = refAsset->getName();
    else
      assetNames[i] = refList[i].getBrokenRef();
  }

  das::Array arr{(char *)assetNames.data(), (uint32_t)assetNames.size(), (uint32_t)assetNames.size(), 1, 0};
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

void os_shell_execute_hidden(const char *op, const char *file, const char *params, const char *dir);

} // namespace bind_dascript
