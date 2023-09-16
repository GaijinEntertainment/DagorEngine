// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include <libTools/dagFileRW/dagFileExport.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/dagFileRW/dagExporter.h>

#include <math/dag_math3d.h>
#include <math/dag_mesh.h>

#include <ioSys/dag_memIo.h>
#include <memory/dag_memBase.h>

#include <ioSys/dag_dataBlock.h>

#include "blk2dag.h"
#include "cfg.h"

#include <direct.h>


#define CONFIG_NAME "/dagorShaders.cfg"


Blk2Dag::Blk2Dag() : maxPrecision(false) {}


Blk2Dag::~Blk2Dag() {}


Mesh *getMeshObj(Node *node)
{
  MeshHolderObj *p = (MeshHolderObj *)node->obj;
  if (!p)
  {
    Mesh *mesh = new Mesh;
    p = new MeshHolderObj(mesh);
    node->setobj(p);
  }

  assert(p->mesh);
  return p->mesh;
}


void Blk2Dag::loadVertexData(DataBlock &blk, Node *node)
{
  Mesh *mesh = getMeshObj(node);

  mesh->vert.reserve(blk.paramCount());

  for (int parID = 0, nid = blk.getNameId("vert"); parID < blk.paramCount(); parID++)
  {
    if (blk.getParamNameId(parID) != nid)
      continue;

    const char *sz = blk.getStr(parID);
    int ind;
    Point3 v;
    sscanf(sz, "[%d] x:%f y:%f z:%f", &ind, &v.x, &v.y, &v.z);

    mesh->vert.push_back(v);
  }
}


void Blk2Dag::loadFaceData(DataBlock &blk, Node *node)
{
  Mesh *mesh = getMeshObj(node);

  mesh->face.reserve(blk.paramCount());

  for (int parID = 0, nid = blk.getNameId("face"); parID < blk.paramCount(); parID++)
  {
    if (blk.getParamNameId(parID) != nid)
      continue;

    const char *sz = blk.getStr(parID);
    int ind;
    int a, b, c;
    int smgrp;
    int mat;

    sscanf(sz, "[%d] i1:%d i2:%d i3:%d  smgrp:%d mat:%d", &ind, &a, &b, &c, &smgrp, &mat);

    Face fc;
    fc.set(a, b, c, mat, smgrp);
    mesh->face.push_back(fc);
  }
}


void Blk2Dag::loadNormalVertData(DataBlock &blk, Node *node)
{
  Mesh *mesh = getMeshObj(node);

  mesh->vertnorm.reserve(blk.paramCount());

  for (int parID = 0, nid = blk.getNameId("nvert"); parID < blk.paramCount(); parID++)
  {
    if (blk.getParamNameId(parID) != nid)
      continue;

    const char *sz = blk.getStr(parID);
    int ind;
    Point3 v;
    sscanf(sz, "[%d] x:%f y:%f z:%f", &ind, &v.x, &v.y, &v.z);

    mesh->vertnorm.push_back(v);
  }
}


void Blk2Dag::loadNormalFaceData(DataBlock &blk, Node *node)
{
  Mesh *mesh = getMeshObj(node);

  mesh->vertnorm.reserve(blk.paramCount());

  for (int parID = 0, nid = blk.getNameId("nface"); parID < blk.paramCount(); parID++)
  {
    if (blk.getParamNameId(parID) != nid)
      continue;

    const char *sz = blk.getStr(parID);
    int ind;
    FaceNGr f;

    sscanf(sz, "[%d] i1:%d i2:%d i3:%d", &ind, &f[0], &f[1], &f[2]);

    mesh->facengr.push_back(f);
  }
}


void Blk2Dag::loadChannelData(DataBlock &blk_root, Node *node, int uv_channel)
{
  if (uv_channel >= NUMMESHTCH)
    return;

  DataBlock *blk_vert = blk_root.getBlockByName("uv_vert");
  if (!blk_vert)
  {
    printf("#Blk2Dag::loadChannelData : block \"uv_vert\" not found");
    return;
  }

  Mesh *mesh = getMeshObj(node);

  TVertTab &vt = mesh->tvert[uv_channel];
  vt.reserve(blk_vert->paramCount());

  for (int parID = 0, nid = blk_root.getNameId("uv"); parID < blk_vert->paramCount(); parID++)
  {
    if (blk_vert->getParamNameId(parID) != nid)
      continue;

    const char *sz = blk_vert->getStr(parID);
    int ind;
    Point2 p;

    sscanf(sz, "[%d] x:%f y:%f", &ind, &p.x, &p.y);
    vt.push_back(p);
  }

  DataBlock *blk_face = blk_root.getBlockByName("uv_face");
  if (!blk_face)
  {
    int count = vt.size() / 3;
    assert(vt.size() % 3 == 0);
    TFaceTab &ft = mesh->tface[uv_channel];
    ft.resize(count);
    for (int i = 0, ind = 0; i < count; i++, ind += 3)
    {
      ft[i].t[0] = ind;
      ft[i].t[1] = ind + 1;
      ft[i].t[2] = ind + 2;
    }
  }
  else
  {
    TFaceTab &ft = mesh->tface[uv_channel];
    ft.reserve(blk_face->paramCount());
    for (int parID = 0, nid = blk_root.getNameId("tface"); parID < blk_face->paramCount(); parID++)
    {
      if (blk_face->getParamNameId(parID) != nid)
        continue;

      const char *sz = blk_face->getStr(parID);

      int ind;
      TFace f;

      sscanf(sz, "[%d] i1:%d i2:%d i3:%d", &ind, &f.t[0], &f.t[1], &f.t[2]);
      ft.push_back(f);
    }
  }
}


void Blk2Dag::loadScriptData(DataBlock &blk, Node *node)
{
  if (!blk.paramCount() && !blk.blockCount())
    return;

  // std::string s;
  int param = blk.findParam("node_script_str");
  if (param != INVALID_INDEX && blk.getParamType(param) == DataBlock::TYPE_STRING)
  {
    node->script = blk.getStr(param);
  }
  else
  {
    DataBlock nblk;
    nblk.setParamsFrom(&blk);

    DynamicMemGeneralSaveCB cwr(tmpmem);
    nblk.saveToTextStream(cwr);
    cwr.write("\0", 1);
    node->script = (const char *)cwr.data();
  }
}


void Blk2Dag::getParamsForMaterialClass(const char *name, std::vector<std::string> &params)
{
  // fixme: read config once
  char buffer[MAX_PATH];
  ::_getcwd(buffer, MAX_PATH);
  strncat(buffer, CONFIG_NAME, MAX_PATH);
  CfgShader cfg(buffer);

  StringVector *data = cfg.GetGlobalParams();

  for (int p = 0; p < cfg.global_params.size(); ++p)
  {
    std::string param;
    param = cfg.global_params.at(p);
    params.push_back(param);
  }

  StringVector *data1 = cfg.GetShaderNames();

  for (int pos = 0; pos < cfg.shader_names.size(); ++pos)
  {
    const std::string &sh = cfg.shader_names.at(pos);
    if (sh != name)
      continue;

    StringVector *data2 = cfg.GetShaderParams(sh);
    for (int j = 0; j < cfg.shader_params.size(); ++j)
    {
      std::string param;
      param = cfg.shader_params.at(j);
      params.push_back(param);
    }
    break;
  }
}


int getTextureTypeToSlotId(const char *type)
{
  char buffer[MAX_PATH];
  ::_getcwd(buffer, MAX_PATH);
  strncat(buffer, CONFIG_NAME, MAX_PATH);
  CfgShader cfg(buffer);

  StringVector *data = cfg.GetSettingsParams();

  for (int p = 0; p < cfg.settings_params.size(); ++p)
  {
    std::string value = cfg.GetParamValue(CFG_SETTINGS_NAME, cfg.settings_params.at(p));
    if (value.find(type) != 0)
      continue;

    std::string tex_slot = cfg.settings_params.at(p);

    if (!isdigit(tex_slot[3]))
      return INVALID_INDEX;

    char s[2];

    memcpy(s, &tex_slot[3], 1);
    s[1] = 0;

    return atoi(s);
  }

  return INVALID_INDEX;
}


void loadParam(DataBlock *blk, StrMap &params)
{
  G_ASSERT(blk);
  const char *name = blk->getStr("name");
  const char *val = blk->getStr("value");

  G_ASSERT(name);
  G_ASSERT(val);
  if (!*name)
    return;
  params[name] = val;
}


void Blk2Dag::loadTexture(DataBlock *blk, MaterialData *mdata)
{
  G_ASSERT(blk);
  G_ASSERT(mdata);

  TEXTUREID texIndex = BAD_TEXTUREID;

  std::map<std::string, std::string> map_params;

  for (int i = 0; i < blk->blockCount(); i++)
  {
    DataBlock *blk_param = blk->getBlock(i);
    G_ASSERT(blk_param);

    if (strcmp(blk_param->getBlockName(), "param"))
      continue;

    loadParam(blk_param, map_params);
  }

  for (int parID = 0; parID < blk->paramCount(); parID++)
  {
    std::string param(blk->getParamName(parID));

    if (param == "filepath")
    {
      const char *filename = blk->getStr(parID);
      texIndex = add_managed_texture(filename);
    }
  }

  StrMap::iterator it = map_params.find("type");
  if (it == map_params.end())
  {
    printf("%s unknown texture type\n", __FUNCTION__);
    return;
  }

  int index = getTextureTypeToSlotId(it->second.c_str());
  if (index < 1)
  {
    printf("%s unknown texture slot for texture type \"%s\"\n", __FUNCTION__, it->second.c_str());
    return;
  }
  index--;
  mdata->mtex[index] = texIndex;
}


MaterialData *Blk2Dag::loadMaterialData(DataBlock &blk, int &index)
{
  MaterialData *mdata = new MaterialData;
  mdata->mat.power = 1;

  index = INVALID_INDEX;

  int tex_count = 0;

  StrMap map_params;

  for (int i = 0; i < blk.blockCount(); i++)
  {
    DataBlock *blk_param = blk.getBlock(i);
    assert(blk_param);

    if (!strcmp(blk_param->getBlockName(), "param"))
    {
      loadParam(blk_param, map_params);
    }
    else if (!strcmp(blk_param->getBlockName(), "texture"))
    {
      loadTexture(blk_param, mdata);
    }
  }


  for (int parID = 0, mtex_idx = 0; parID < blk.paramCount(); parID++)
  {
    std::string param(blk.getParamName(parID));
    if (param == "id")
    {
      index = blk.getInt(parID);
    }
    else if (param == "class")
    {
      mdata->className = blk.getStr(parID);
    }
    else if (param == "name")
    {
      mdata->matName = blk.getStr(parID);
    }
    else if (param == "script")
    {
      mdata->matScript = blk.getStr(parID);
    }
    else if (param == "power")
    {
      mdata->mat.power = blk.getReal(parID);
    }
    else if (param == "diffuse")
    {
      const char *sz = blk.getStr(parID);
      float r, g, b;
      sscanf(sz, "r:%f g:%f b:%f", &r, &g, &b);
      mdata->mat.diff.set(r, g, b, 1.f);
    }
    else if (param == "specular")
    {
      const char *sz = blk.getStr(parID);
      float r, g, b;
      sscanf(sz, "r:%f g:%f b:%f", &r, &g, &b);
      mdata->mat.spec.set(r, g, b, 1.f);
    }
    else if (param == "ambient")
    {
      const char *sz = blk.getStr(parID);
      float r, g, b;
      sscanf(sz, "r:%f g:%f b:%f", &r, &g, &b);
      mdata->mat.amb.set(r, g, b, 1.f);
    }
    else if (param == "texture")
    {
      const char *tex_nm = blk.getStr(parID);
      if (*tex_nm)
        mdata->mtex[mtex_idx] = add_managed_texture(tex_nm);
      mtex_idx++;
    }
  }

  StrMap::iterator it = map_params.find("class");
  if (it != map_params.end())
  {
    mdata->className = it->second.c_str();

    std::vector<std::string> params;
    getParamsForMaterialClass(mdata->className, params);

    for (int i = 0; i < params.size(); i++)
    {
      StrMap::iterator param_it = map_params.find(params[i]);
      if (param_it == map_params.end())
      {
        printf("%s unknown param  \"%s\"\n", __FUNCTION__, params[i].c_str());
        continue;
      }

      std::string s(mdata->matScript.str());
      s += param_it->first;
      s += "=";
      s += param_it->second.c_str();
      s += "\r\n";

      mdata->matScript = s.c_str();
    }
  }
  else if (mdata->className.empty())
  {
    // set default material
    mdata->className = "simple_aces";
    mdata->matScript = "";
    printf("%s param \"class\" not found in material \"%s\", using default material\n", __FUNCTION__, mdata->matName.str());
  }

  return mdata;
}


void Blk2Dag::loadUVData(DataBlock &blk, Node *node)
{
  int uv_channel = 0;
  for (int ch = 0; ch < blk.blockCount(); ch++)
  {
    DataBlock *blk_ch = blk.getBlock(ch);
    if (!strcmp(blk_ch->getBlockName(), "channel") || !strcmp(blk_ch->getBlockName(), "layer"))
    {
      uv_channel = blk_ch->getInt("layer", uv_channel);
      loadChannelData(*blk_ch, node, uv_channel);
      uv_channel++;
    }
  }
}


bool Blk2Dag::work(const char *input_filename, const char *output_filename)
{
  DataBlock script;

  if (!script.load(input_filename))
  {
    printf("%s : can't load file \"%s\"\n", __FUNCTION__, input_filename);
    return false;
  }

  struct NodeHelper
  {
    Node *node;
    int parent_id;
  };

  std::vector<NodeHelper *> nodes;

  const char *sz = script.getStr("HEAD");
  if (sz)
  {
    char type[MAX_PATH];
    sscanf(sz, "BLK(%s)", type);

    maxPrecision = !strcmp(type, "int");
  }

  bool save_normal = false;

  std::vector<MaterialData *> mat_list;


  for (int i = 0; i < script.blockCount(); i++)
  {
    DataBlock *blk = script.getBlock(i);
    assert(blk);

    if (!strcmp(blk->getBlockName(), "node"))
    {
      NodeHelper *hlp = new NodeHelper;
      Node *node = new Node;
      hlp->node = node;
      hlp->parent_id = INVALID_INDEX;

      nodes.push_back(hlp);

      // set default params;

      for (int j = 0; j < blk->blockCount(); j++)
      {
        DataBlock *blk_in = blk->getBlock(j);
        assert(blk_in);

        std::string blk_name(blk_in->getBlockName());
        printf("Block: %s\n", blk_name.c_str());

        if (blk_name == "vert_data")
        {
          loadVertexData(*blk_in, node);
        }
        else if (blk_name == "face_data")
        {
          loadFaceData(*blk_in, node);
        }
        else if (blk_name == "normal_vert")
        {
          save_normal = true;
          loadNormalVertData(*blk_in, node);
        }
        else if (blk_name == "normal_faces")
        {
          loadNormalFaceData(*blk_in, node);
        }
        else if (blk_name == "script")
        {
          loadScriptData(*blk_in, node);
          blk->setBool("renderable", blk_in->getBool("renderable", blk->getBool("renderable", true)));
        }
        else if (blk_name == "uv_data")
        {
          loadUVData(*blk_in, node);
        }
      }

      for (int parID = 0; parID < blk->paramCount(); parID++)
      {
        std::string param(blk->getParamName(parID));
        printf("Param: %s\n", param.c_str());

        if (param == "name")
        {
          node->name = blk->getStr(parID);
        }
        else if (param == "renderable")
        {
          node->flags |= blk->getBool(parID) ? NODEFLG_RENDERABLE : 0;
        }
        else if (param == "castshadow")
        {
          node->flags |= blk->getBool(parID) ? NODEFLG_CASTSHADOW : 0;
        }
        else if (param == "rcvshadow")
        {
          node->flags |= blk->getBool(parID) ? NODEFLG_RCVSHADOW : 0;
        }
        else if (param == "tm")
        {
          node->tm = blk->getTm(parID);
        }
        else if (param == "id")
        {
          node->id = blk->getInt(parID);
        }
        else if (param == "parent_id")
        {
          hlp->parent_id = blk->getInt(parID);
        }
      }
    }
    else if (!strcmp(blk->getBlockName(), "material"))
    {
      int index;
      MaterialData *mdata = loadMaterialData(*blk, index);

      if (index == INVALID_INDEX)
      {
        G_ASSERT(mdata);
        printf("%s : Unknown material id (\"%s\")", __FUNCTION__, mdata->matName.str());
        continue;
      }

      if (mat_list.size() <= index)
        mat_list.resize(index + 1);

      mat_list[index] = mdata;
    }
  }

  // build hierarchy objects
  Node *root = nodes[0]->node;

  for (int i = 1; i < nodes.size(); i++)
  {
    int parentID = nodes[i]->parent_id;
    for (int j = 0; j < nodes.size(); j++)
    {
      if (nodes[j]->node->id != parentID)
        continue;

      nodes[j]->node->child.push_back(nodes[i]->node);
      nodes[i]->node->parent = nodes[j]->node;
      break;
    }
  }

  // build materials
  std::vector<int> remap;

  for (int i = 1; i < nodes.size(); i++)
  {
    Node *node = nodes[i]->node;
    MeshHolderObj *p = (MeshHolderObj *)node->obj;
    if (!p || !p->mesh)
      continue;

    remap.clear();
    for (int i = 0; i < mat_list.size(); i++)
      remap.push_back(INVALID_INDEX);

    Mesh *mesh = p->mesh;
    for (int j = 0; j < mesh->face.size(); j++)
    {
      Face &face = mesh->face[j];
      int old_id = face.mat;

      if (mat_list.size() <= old_id)
      {
        printf("%s : Invalid material index(%d) in node \"%s\"", __FUNCTION__, old_id, node->name.str());
        face.mat = INVALID_INDEX;
        continue;
      }

      if (remap[old_id] == INVALID_INDEX)
      {
        if (!node->mat)
          node->mat = new MaterialDataList;
        int new_id = node->mat->addSubMat(mat_list[old_id]);
        remap[old_id] = new_id;
      }

      face.mat = remap[old_id];
    }
  }

  root->calc_wtm();

  DagExporter exp;

  if (!exp.exportGeometry(output_filename, root, false, false, save_normal))
  {
    printf("Export geometry failed\n");
    return false;
  }

  return true;
}