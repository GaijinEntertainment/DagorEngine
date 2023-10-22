//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EditorCore/ec_workspace.h>

#include <util/dag_string.h>
#include <generic/dag_tab.h>

#include <libTools/containers/dag_StrMap.h>


class DeWorkspace : public EditorWorkspace
{
public:
  struct Workspace
  {
    Tab<String> exportIgnore;

    String graphiteDir;
    String collision;
    String additionalPlugins;

    Tab<String> recents;

    Tab<String> enabledColliders;

    Workspace();

    void reset();
  };

  class TabInt : public Tab<int>
  {
  public:
    TabInt() : Tab<int>(midmem_ptr()) {}
    TabInt(const TabInt &from) : Tab<int>(from) {}
  };

  DeWorkspace();

  inline const char *getGraphiteDir() const { return params.graphiteDir; }
  inline Tab<String> &getRecents() { return params.recents; }
  inline Tab<String> &getExportIgnore() { return params.exportIgnore; };
  inline const Workspace &getParams() { return params; }
  inline const Tab<String> &getF11Tags() const { return f11Tags; }
  inline const StriMap<TabInt> &getPluginTags() const { return plugTags; }
  inline const char *getAdditionalPluginsPath() const { return params.additionalPlugins; }

  inline void getEnabledColliders(Tab<String> &coll) { coll = params.enabledColliders; }

  bool getMetricsBlk(DataBlock &blk) const;

  inline void setParams(const Workspace &_params) { params = _params; }

  void clear();

  const DataBlock &getShGlobVarsScheme() const { return shGlobVarsScheme; }
  const DataBlock *findWspBlk(const char *app_blk_path);

  void getHmapSettings(bool &hmap_tiletex, bool &hmap_colortex, bool &hmap_lightmaptex, bool &hmap_usemeshsurface,
    bool &hmap_usenormalmap) const
  {
    hmap_tiletex = hmapRequireTileTex;
    hmap_colortex = hmapHasColorTex;
    hmap_lightmaptex = hmapHasLightmapTex;
    hmap_usemeshsurface = hmapUseMeshSurface;
    hmap_usenormalmap = hmapUseNormalMap;
  }
  const DataBlock &getHmapLandClsLayersDesc() const { return hmapLandClsLayersDesc; }

  void getObjGenerationStreamingSettings(float &fwd_rad, float &back_rad, float &discard_rad) const
  {
    fwd_rad = genFwdRadius;
    back_rad = genBackRadius;
    discard_rad = genDiscardRadius;
  }

private:
  Workspace params;

  Tab<String> f11Tags;
  StriMap<TabInt> plugTags;

  DataBlock shGlobVarsScheme;

  DataBlock hmapLandClsLayersDesc;
  bool hmapRequireTileTex, hmapHasColorTex, hmapHasLightmapTex, hmapUseMeshSurface;
  bool hmapUseNormalMap;
  float genFwdRadius, genBackRadius, genDiscardRadius;

  virtual bool loadSpecific(const DataBlock &blk);
  virtual bool loadAppSpecific(const DataBlock &blk);
  virtual bool saveSpecific(DataBlock &blk);

  void setDefaultLightPresets();
};
