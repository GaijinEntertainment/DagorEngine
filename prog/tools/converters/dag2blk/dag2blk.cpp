// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include <libTools/dagFileRW/dagFileExport.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/dagFileRW/dagExporter.h>

#include <math/dag_math3d.h>
#include <math/dag_mesh.h>

#include <ioSys/dag_dataBlock.h>

#include "dag2blk.h"

Dag2Blk::Dag2Blk() : maxPrecision(false) {}

Dag2Blk::~Dag2Blk() {}

void Dag2Blk::addNode(Node *node, DataBlock &blk_parent)
{
  DataBlock *blk = blk_parent.addNewBlock("node");
  assert(blk);

  blk->setStr("name", node->name);

  blk->setInt("id", node->id);
  blk->setInt("parent_id", node->parent ? node->parent->id : INVALID_INDEX);
  blk->setBool("renderable", (node->flags & NODEFLG_RENDERABLE) != 0);
  blk->setBool("castshadow", (node->flags & NODEFLG_CASTSHADOW) != 0);
  blk->setBool("rcvshadow", (node->flags & NODEFLG_RCVSHADOW) != 0);

  blk->setTm("tm", node->tm);


  if (node->mat && node->mat->subMatCount())
  {
    for (int i = 0; i < node->mat->subMatCount(); i++)
    {
      MaterialData *mdata = node->mat->getSubMat(i);
      assert(mdata);

      bool found = false;
      for (int j = 0; j < mat_list.size(); j++)
      {
        if (mat_list[j] == mdata)
          found = true;
      }

      if (!found)
        mat_list.push_back(mdata);
    }
  }


  // Mesh
  MeshHolderObj *obj = (MeshHolderObj *)node->obj;
  if (obj && obj->mesh)
  {
    Mesh *mesh = obj->mesh;


    // Vertexes
    DataBlock *blk_vert = blk->addNewBlock("vert_data");

    for (int i = 0; i < mesh->vert.size(); i++)
    {
      char buffer[MAX_PATH];

      sprintf(buffer, "[%d] x:%f y:%f z:%f", i, mesh->vert[i].x, mesh->vert[i].y, mesh->vert[i].z);
      blk_vert->addStr("vert", buffer);

      // blk->addPoint3( "vert", mesh->vert[ i ] );
    }

    // Faces

    DataBlock *blk_face = blk->addNewBlock("face_data");

    for (int i = 0; i < mesh->face.size(); i++)
    {
      char buffer[MAX_PATH];
      Face &f = mesh->face[i];

      sprintf(buffer, "[%d] i1:%d i2:%d i3:%d smgrp:%d mat:%d", i, f.v[0], f.v[1], f.v[2], f.smgr, f.mat);
      blk_face->addStr("face", buffer);
    }

    // UV

    DataBlock *blk_uvdata = blk->addNewBlock("uv_data");

    for (int j = 0; j < NUMMESHTCH; j++)
    {
      TVertTab &verts = mesh->tvert[j];
      TFaceTab &faces = mesh->tface[j];
      if (!verts.size())
        break;

      DataBlock *blk_layer = blk_uvdata->addNewBlock("layer");
      DataBlock *blk_uvvert = blk_layer->addNewBlock("uv_vert");

      blk_layer->setInt("layer", j);
      for (int i = 0; i < verts.size(); i++)
      {
        char buffer[MAX_PATH];

        sprintf(buffer, "[%d] x:%f y:%f", i, verts[i].x, verts[i].y);
        blk_uvvert->addStr("uv", buffer);
      }

      DataBlock *blk_tface = blk_layer->addNewBlock("uv_face");
      for (int i = 0; i < faces.size(); i++)
      {
        TFace &f = faces[i];
        char buffer[MAX_PATH];
        sprintf(buffer, "[%d] i1:%d i2:%d i3:%d", i, f.t[0], f.t[1], f.t[2]);
        blk_tface->addStr("tface", buffer);
      }
    }

    // Normals
    if (mesh->vertnorm.size())
    {
      DataBlock *blk_nrm_ver = blk->addNewBlock("normal_vert");

      for (int i = 0; i < mesh->vertnorm.size(); i++)
      {
        char buffer[MAX_PATH];
        sprintf(buffer, "[%d] x:%f y:%f z:%f", i, mesh->vertnorm[i].x, mesh->vertnorm[i].y, mesh->vertnorm[i].z);
        blk_nrm_ver->addStr("nvert", buffer);
      }

      DataBlock *blk_nrm_faces = blk->addNewBlock("normal_faces");
      for (int i = 0; i < mesh->facengr.size(); i++)
      {
        char buffer[MAX_PATH];
        FaceNGr &f = mesh->facengr[i];
        sprintf(buffer, "[%d] i1:%d i2:%d i3:%d", i, f[0], f[1], f[2]);
        blk_nrm_faces->addStr("nface", buffer);
      }
    }
  }


  // DAG_NODE_SCRIPT
  if (!node->script.empty())
  {
    DataBlock *scr = blk->addNewBlock("script");
    DataBlock sb;
    if (!dblk::load_text(sb, make_span_const(node->script), dblk::ReadFlag::ROBUST))
    {
      scr->addStr("node_script_str", node->script);
    }
    else
    {
      scr->setFrom(&sb);
    }
  }

  // add child nodes
  for (int i = 0; i < node->child.size(); i++)
  {
    addNode(node->child[i], blk_parent);
  }
}


void Dag2Blk::addMaterials(DataBlock &blk_parent)
{
  for (int i = 0; i < mat_list.size(); i++)
  {
    DataBlock *blk = blk_parent.addNewBlock("material");
    assert(blk);

    MaterialData *mdata = mat_list[i];

    blk->setInt("id", i);
    blk->setStr("name", mdata->matName);
    blk->setStr("class", mdata->className);
    blk->setStr("script", mdata->matScript);

    char buffer[MAX_PATH];

    sprintf(buffer, "r:%f g:%f b:%f", mdata->mat.diff.r, mdata->mat.diff.g, mdata->mat.diff.b);
    blk->setStr("diffuse", buffer);

    sprintf(buffer, "r:%f g:%f b:%f", mdata->mat.spec.r, mdata->mat.spec.g, mdata->mat.spec.b);
    blk->setStr("specular", buffer);

    sprintf(buffer, "r:%f g:%f b:%f", mdata->mat.amb.r, mdata->mat.amb.g, mdata->mat.amb.b);
    blk->setStr("ambient", buffer);

    blk->setReal("power", mdata->mat.power);

    int max_chan = MAXMATTEXNUM;
    while (max_chan > 0 && mdata->mtex[max_chan - 1] == BAD_TEXTUREID)
      max_chan--;
    for (int ch = 0; ch < max_chan; ch++)
    {
      auto index = mdata->mtex[ch];
      if (index == BAD_TEXTUREID)
      {
        blk->addStr("texture", "");
        continue;
      }

      const char *tex_name = ::get_managed_texture_name(index);
      if (!tex_name)
      {
        printf("Can't get texture name by ID");
        break;
      }

      blk->addStr("texture", tex_name);
    }
  }
}


bool Dag2Blk::work(const char *input_filename, const char *output_filename)
{
  AScene dagScene;
  load_ascene(input_filename, dagScene, LASF_MATNAMES);

  DataBlock blk;

  char header[MAX_PATH];
  sprintf(header, "BLK(%s)", maxPrecision ? "int" : "float");
  blk.addStr("HEAD", header);

  addNode(dagScene.root, blk);

  addMaterials(blk);

  if (!blk.saveToTextFile(output_filename))
    return false;

  return true;
}
