// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../av_plugin.h"
// #include "../av_interface.h"
#include "materialRec.h"
#include <propPanel/c_control_event_handler.h>


class OLAppWindow;
class MaterialParamDescr;
class MatShader;
class MatParam;
class MatTripleInt;
class MatTripleReal;
class MatE3dColor;
class MatCombo;
class MatTexture;
class GeomObject;
class MatCustom;
class Mat2Sided;


class MaterialsPlugin : public IGenEditorPlugin, public PropPanel::ControlEventHandler
{
public:
  MaterialsPlugin();
  ~MaterialsPlugin() override;

  // IGenEditorPlugin
  const char *getInternalName() const override { return "materialsEditor"; }

  void registered() override;
  void unregistered() override;

  bool begin(DagorAsset *asset) override;
  bool end() override;
  IGenEventHandler *getEventHandler() override { return NULL; }

  void clearObjects() override {}
  void onSaveLibrary() override;
  void onLoadLibrary() override;

  bool getSelectionBox(BBox3 &box) const override;

  void actObjects(float dt) override {}
  void beforeRenderObjects() override {}
  void renderObjects() override {}
  void renderTransObjects() override {}
  void renderGeometry(Stage stage) override;

  bool supportAssetType(const DagorAsset &asset) const override;

  void fillPropPanel(PropPanel::ContainerPropertyControl &panel) override;
  void postFillPropPanel() override {}

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

private:
  DagorAsset *curAsset;
  MaterialRec *curMat;

  GeomObject *previewGeom;

  Tab<const MaterialParamDescr *> matParamsDescr;
  Tab<const MatShader *> matShaders;
  Tab<MatParam *> matParams;
  Tab<String> shaderParamsNames;

  void initMatParamsDescr(const DataBlock &app_blk);
  void clearMatParamsDescr();

  void initMatShaders(const DataBlock &app_blk);
  void clearMatShaders();

  void createParamsList();
  void addParamToList(const char *name, const E3DCOLOR &color);
  void clearParamsList();

  void updatePreview();

  void getShadersNames(Tab<String> &names) const;

  const MaterialParamDescr *findParameterDescr(const char *name) const;
  const MatShader *findShader(const char *name) const;
  // get existent parameter or create new one
  MatParam &findMatParam(const char *name);

  void fillShaderPanel(PropPanel::ContainerPropertyControl *panel);
  void fillShaderParams(PropPanel::ContainerPropertyControl *panel, const MaterialRec &mat, const MatShader &shader);

  void addTexture(PropPanel::ContainerPropertyControl *panel, const MaterialRec &mat, const char *caption, int ctrl_idx,
    int tex_idx) const;
  void addTripleInt(PropPanel::ContainerPropertyControl *panel, const char *caption, const char *script_param,
    const IPoint2 &constrains, int ctrl_idx) const;
  void addTripleReal(PropPanel::ContainerPropertyControl *panel, const char *caption, const char *script_param,
    const Point2 &constrains, int ctrl_idx) const;
  void addE3dColor(PropPanel::ContainerPropertyControl *panel, const char *caption, const char *script_param, int col_idx,
    int ctrl_idx) const;
  void addCombo(PropPanel::ContainerPropertyControl *panel, const char *caption, const char *script_param, const Tab<String> &vals,
    const char *def, int ctrl_idx);
  void addCustom(PropPanel::ContainerPropertyControl *panel, const char *script_param, int ctrl_idx) const;

  void onShaderParamChange(int pid, PropPanel::ContainerPropertyControl *panel);
  void onTripleIntChange(PropPanel::ContainerPropertyControl *panel, const IPoint2 &param_idx, MatTripleInt &param) const;
  void onTripleRealChange(PropPanel::ContainerPropertyControl *panel, const IPoint2 &param_idx, MatTripleReal &param) const;
  void onE3dColorChange(PropPanel::ContainerPropertyControl *panel, const IPoint2 &paramIdx, MatE3dColor &param) const;
  void onComboChange(PropPanel::ContainerPropertyControl *panel, const IPoint2 &param_idx, MatCombo &param) const;
  void onTextureChange(PropPanel::ContainerPropertyControl *panel, const IPoint2 &param_idx, MatTexture &param) const;
  void onCustomChange(PropPanel::ContainerPropertyControl *panel, const IPoint2 &param_idx, MatCustom &param) const;
  void onTwoSidedChange(PropPanel::ContainerPropertyControl *panel, const IPoint2 &param_idx, Mat2Sided &param) const;

  real getCorrectRealVal(real val, const Point2 &constrains) const;

  // x -- param idx, y -- control idx
  IPoint2 getParamIdxFromPid(int pid) const;
};
