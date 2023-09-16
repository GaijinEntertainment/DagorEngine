#include "materials.h"
#include "mat_cm.h"
#include "mat_paramDesc.h"
#include "mat_shader.h"
#include "mat_param.h"
#include "mat_util.h"
#include <assets/asset.h>
#include <assets/assetMgr.h>

#include "../av_appwnd.h"
#include "../av_util.h"
#include <de3_interface.h>

#include <sepGui/wndGlobal.h>

#include <generic/dag_sort.h>
#include <libTools/util/strUtil.h>
#include <libTools/containers/tab_sort.h>
#include <libTools/ObjCreator3d/objCreator3d.h>

#include <libTools/staticGeom/geomObject.h>
#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/staticGeom/staticGeometryContainer.h>

#include <shaders/dag_dynSceneRes.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_debug.h>
#include <math/dag_color.h>


//==================================================================================================
MaterialsPlugin::MaterialsPlugin() :
  curAsset(NULL),
  curMat(NULL),
  matParamsDescr(midmem),
  matShaders(midmem),
  matParams(tmpmem),
  shaderParamsNames(tmpmem),
  previewGeom(NULL)
{
  // initScriptPanelEditor("mat.scheme.nut", "material by scheme");
}
MaterialsPlugin::~MaterialsPlugin() { del_it(curMat); }


//==================================================================================================
void MaterialsPlugin::registered()
{
  ::init_geom_object_lighting();

  previewGeom = new GeomObject;

  TMatrix tm;
  tm.setcol(0, Point3(10, 0, 0));
  tm.setcol(1, Point3(0, 10, 0));
  tm.setcol(2, Point3(0, 0, 10));
  tm.setcol(3, Point3(0, 0, 0));

  previewGeom->setTm(tm);

  DataBlock appBlk(::get_app().getWorkspace().getAppPath());
  initMatParamsDescr(appBlk);
  initMatShaders(appBlk);
}


//==================================================================================================
void MaterialsPlugin::unregistered()
{
  clearMatParamsDescr();
  clearMatShaders();

  del_it(previewGeom);
}


//==================================================================================================
bool MaterialsPlugin::begin(DagorAsset *asset)
{
  if (curMat)
    curMat->saveChangesToAsset();

  del_it(curMat);
  curAsset = asset;
  curMat = new MaterialRec(*curAsset);

  createParamsList();

  updatePreview();

  if (spEditor && asset)
    spEditor->load(asset);
  return true;
}
bool MaterialsPlugin::end()
{
  if (spEditor)
    spEditor->destroyPanel();
  if (curMat)
    curMat->saveChangesToAsset();

  del_it(curMat);
  return true;
}


void MaterialsPlugin::renderGeometry(Stage stage)
{
  if (!getVisible() || !previewGeom)
    return;

  switch (stage)
  {
    case STG_RENDER_STATIC_OPAQUE:
    case STG_RENDER_SHADOWS: previewGeom->render(); break;
    case STG_RENDER_STATIC_TRANS: previewGeom->renderTrans(); break;
  }
}

//==================================================================================================
bool MaterialsPlugin::getSelectionBox(BBox3 &box) const
{
  box = previewGeom->getBoundBox();
  return true;
}


//==================================================================================================
void MaterialsPlugin::onSaveLibrary()
{
  if (curMat)
    curMat->saveChangesToAsset();
}


//==================================================================================================
void MaterialsPlugin::onLoadLibrary() {}


//==================================================================================================
bool MaterialsPlugin::supportAssetType(const DagorAsset &asset) const
{
  return strcmp(asset.getMgr().getAssetTypeName(asset.getType()), "mat") == 0;
}


//==================================================================================================
void MaterialsPlugin::initMatParamsDescr(const DataBlock &app_blk)
{
  clearMatParamsDescr();

  String descrPath = ::make_full_path(::get_app().getWorkspace().getAppDir(), get_app().getMatParamsPath());
  if (!::dd_file_exist(descrPath.str()))
    descrPath = ::make_full_path(sgg::get_exe_path(), "../commonData/mat_params.blk");

  DataBlock matParamsBlk;
  if (app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("mat")->getBool("useCommonMatParams", true))
    matParamsBlk.load(descrPath);
  if (const DataBlock *b = app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("mat")->getBlockByName("matParams"))
    for (int i = 0; i < b->blockCount(); ++i)
      matParamsBlk.addNewBlock(b->getBlock(i));

  for (int i = 0; i < matParamsBlk.blockCount(); ++i)
  {
    const DataBlock *paramBlk = matParamsBlk.getBlock(i);

    if (paramBlk)
    {
      const MaterialParamDescr *descr = new (midmem) MaterialParamDescr(*paramBlk);
      G_ASSERT(descr);

      matParamsDescr.push_back(descr);
    }
  }
}


//==================================================================================================
void MaterialsPlugin::clearMatParamsDescr()
{
  for (int i = 0; i < matParamsDescr.size(); ++i)
    del_it(matParamsDescr[i]);

  matParamsDescr.clear();
}


//==================================================================================================
void MaterialsPlugin::initMatShaders(const DataBlock &app_blk)
{
  clearMatShaders();

  String shPath = ::make_full_path(sgg::get_exe_path(), "../commonData/mat_shaders.blk");

  DataBlock shadersBlk;
  if (app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("mat")->getBool("useCommonMatShaders", true))
    shadersBlk.load(shPath);
  if (const DataBlock *b = app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("mat")->getBlockByName("matShaders"))
    for (int i = 0; i < b->blockCount(); ++i)
      shadersBlk.addNewBlock(b->getBlock(i));


  for (int i = 0; i < shadersBlk.blockCount(); ++i)
  {
    const DataBlock *shBlk = shadersBlk.getBlock(i);

    if (shBlk)
    {
      const MatShader *shader = new (midmem) MatShader(*shBlk);
      G_ASSERT(shader);

      matShaders.push_back(shader);
    }
  }
}


//==================================================================================================
void MaterialsPlugin::clearMatShaders()
{
  for (int i = 0; i < matShaders.size(); ++i)
    del_it(matShaders[i]);

  matShaders.clear();
}


//==================================================================================================
const MaterialParamDescr *MaterialsPlugin::findParameterDescr(const char *name) const
{
  if (name && *name)
  {
    for (int i = 0; i < matParamsDescr.size(); ++i)
      if (!stricmp(matParamsDescr[i]->getName(), name))
        return matParamsDescr[i];
  }

  return NULL;
}


//==================================================================================================
const MatShader *MaterialsPlugin::findShader(const char *name) const
{
  if (name && *name)
  {
    for (int i = 0; i < matShaders.size(); ++i)
      if (!stricmp(matShaders[i]->getName(), name))
        return matShaders[i];
  }

  return NULL;
}


//==================================================================================================
MatParam &MaterialsPlugin::findMatParam(const char *name)
{
  G_ASSERT(name);

  for (int i = 0; i < matParams.size(); ++i)
    if (!stricmp(matParams[i]->getParamName(), name))
      return *matParams[i];

  String paramVal(::get_script_param(curMat->script, name));
  const MaterialParamDescr *desc = findParameterDescr(name);
  if (!desc)
    fatal("Couldn't find description for parameter \"%s\"", name);

  MatParam *parameter = NULL;
  MatTripleInt *pi = NULL;
  MatTripleReal *pf = NULL;

  switch (desc->getType())
  {
    case MaterialParamDescr::PT_TEXTURE: parameter = new (tmpmem) MatTexture(*desc); break;

    case MaterialParamDescr::PT_TRIPLE_INT:
      pi = new (tmpmem) MatTripleInt(*desc);
      pi->val = paramVal.length() ? atoi(paramVal) : 0;
      parameter = pi;
      break;

    case MaterialParamDescr::PT_TRIPLE_REAL:
      pf = new (tmpmem) MatTripleReal(*desc);
      pf->val = paramVal.length() ? atof(paramVal) : 0;
      parameter = pf;
      break;

    case MaterialParamDescr::PT_COMBO: parameter = new (tmpmem) MatCombo(*desc); break;

    case MaterialParamDescr::PT_CUSTOM:
      if (!stricmp(desc->getName(), "two_sided"))
        parameter = new (tmpmem) Mat2Sided(*desc);
      break;
  }

  G_ASSERT(parameter);

  matParams.push_back(parameter);

  return *parameter;
}


//==================================================================================================
void MaterialsPlugin::getShadersNames(Tab<String> &names) const
{
  for (int i = 0; i < matShaders.size(); ++i)
    names.push_back() = matShaders[i]->getName();

  sort(names, &tab_sort_stringsi);
}


//==================================================================================================
void MaterialsPlugin::clearParamsList()
{
  for (int i = 0; i < matParams.size(); ++i)
    del_it(matParams[i]);

  matParams.clear();
}


//==================================================================================================
void MaterialsPlugin::addParamToList(const char *name, const E3DCOLOR &color)
{
  const MaterialParamDescr *desc = findParameterDescr(name);
  if (desc)
  {
    MatE3dColor *parameter = new (tmpmem) MatE3dColor(*desc);

    parameter->val = color;
    matParams.push_back(parameter);
  }
  else
    DAEDITOR3.conError("cant find desc for <%s>", name);
}


//==================================================================================================
void MaterialsPlugin::createParamsList()
{
  clearParamsList();

  addParamToList("mat_diff", e3dcolor(curMat->mat.diff));
  addParamToList("mat_amb", e3dcolor(curMat->mat.amb));
  addParamToList("mat_spec", e3dcolor(curMat->mat.spec));
  addParamToList("mat_emis", e3dcolor(curMat->mat.emis));
}


//==================================================================================================
void MaterialsPlugin::updatePreview()
{
  previewGeom->clear();
  StaticGeometryContainer *cont = previewGeom->getGeometryContainer();
  if (cont)
  {
    MaterialDataList mat;
    mat.addSubMat(curMat->getMaterial());

    ObjCreator3d::generateSphere(32, *cont, &mat);

    if (cont->nodes.size())
    {
      Mesh &mesh = cont->nodes[0]->mesh->mesh;
      cont->nodes[0]->flags |= StaticGeometryNode::FLG_NO_RECOMPUTE_NORMALS | StaticGeometryNode::FLG_CASTSHADOWS;

      for (int ci = 1; ci < NUMMESHTCH; ++ci)
      {
        mesh.tface[ci] = mesh.tface[0];
        mesh.tvert[ci] = mesh.tvert[0];
      }
      mesh.calc_ngr();
      mesh.calc_vertnorms();
      for (int i = 0; i < mesh.vertnorm.size(); ++i)
        mesh.vertnorm[i] = -mesh.vertnorm[i];
      for (int i = 0; i < mesh.facenorm.size(); ++i)
        mesh.facenorm[i] = -mesh.facenorm[i];
    }

    previewGeom->recompile();
    repaintView();
  }
}
