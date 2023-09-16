#pragma once
#include "assetStats.h"
#include <EditorCore/ec_ViewportWindow.h>

class AssetViewerViewportWindow : public ViewportWindow
{
public:
  AssetViewerViewportWindow(TEcHandle parent, int left, int top, int w, int h);

  AssetStats &getAssetStats() { return assetStats; }
  bool needShowAssetStats() const { return showStats && showAssetStats; }

private:
  virtual void load(const DataBlock &blk) override;
  virtual void save(DataBlock &blk) const override;
  virtual void paint(int w, int h) override;
  virtual void fillStatSettingsDialog(PropertyContainerControlBase &tab_panel) override;
  virtual void handleStatSettingsDialogChange(int pcb_id) override;

  int getAssetStatByIndex(int index);
  static int getAssetStatIndexByName(const char *name);
  static void formatGeometryStat(String &statText, const char *stat_name, const AssetStats::GeometryStat &geometry);

  AssetStats assetStats;
  bool showAssetStats = false;
};
