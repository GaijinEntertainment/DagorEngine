// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>

#include "assets/asset.h"
#include "assets/assetRefs.h"
#include "libTools/util/iLogWriter.h"
#include "options.h"
#include "pointCloudGen.h"

extern bool interrupted;

namespace plod
{

class Baker : public PointCloudGenerator
{
public:
  Baker(const char *app_dir, DataBlock &app_blk, DagorAssetMgr *asset_manager, ILogWriter *log_writer, const AppOptions &options);
  int generatePLODs();

private:
  void gatherAssetsFromList(dag::Vector<DagorAsset *> &assets, dag::ConstSpan<plod::String> asset_names);
  void gatherAssetsFromPackages(dag::Vector<DagorAsset *> &assets, dag::ConstSpan<plod::String> packages);
  void gatherAllAssets(dag::Vector<DagorAsset *> &assets);
  bool isAssetSupported(DagorAsset *asset);

  AppOptions options = {};
  int rendinstTypeId = -1;
  IDagorAssetRefProvider *rendinstRefProfider = nullptr;

  template <typename... Args>
  void conlog(const char *fmt, Args &&...args) const
  {
    log->addMessage(ILogWriter::MessageType::NOTE, fmt, eastl::forward<Args>(args)...);
    debug(fmt, eastl::forward<Args>(args)...);
  }

  template <typename... Args>
  void conwarning(const char *fmt, Args &&...args) const
  {
    log->addMessage(ILogWriter::MessageType::WARNING, fmt, eastl::forward<Args>(args)...);
    logwarn(fmt, eastl::forward<Args>(args)...);
  }

  template <typename... Args>
  void conerror(const char *fmt, Args &&...args) const
  {
    log->addMessage(ILogWriter::MessageType::ERROR, fmt, eastl::forward<Args>(args)...);
    logerr(fmt, eastl::forward<Args>(args)...);
  }
};

} // namespace plod
