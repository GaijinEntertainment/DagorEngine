// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "assetStats.h"
#include <EditorCore/ec_ViewportWindow.h>

class BaseTexture;

class AssetViewerViewportWindow : public ViewportWindow
{
public:
  AssetStats &getAssetStats() { return assetStats; }
  bool needShowAssetStats() const { return showStats && showAssetStats; }

private:
  void load(const DataBlock &blk) override;
  void save(DataBlock &blk) const override;
  void paint(int w, int h) override;
  void fillStatSettingsDialog(ViewportWindowStatSettingsDialog &dialog) override;
  void handleStatSettingsDialogChange(int pcb_id, bool value) override;
  bool canStartInteractionWithViewport() override;

  int getAssetStatByIndex(int index);
  BaseTexture *getDepthBuffer() override;

  static int getAssetStatIndexByName(const char *name);
  static void formatGeometryStat(String &statText, const char *stat_name, const AssetStats::GeometryStat &geometry);

  AssetStats assetStats;
  bool showAssetStats = false;
};
