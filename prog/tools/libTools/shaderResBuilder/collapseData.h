// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/dagFileRW/dagFileNode.h>
#include <math/dag_mesh.h>
#include <3d/dag_materialData.h>
#include <math/dag_meshBones.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_tab.h>
#include <util/dag_oaHashNameMap.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>

class ShaderMaterial;


static void collapse_nodes(Node *n, Node *link_to, bool allow_sep_flags, bool all_animated, const char *curDagFname,
  const OAHashNameMap<true> &link_nodes, bool process_ri)
{
  DataBlock blk;
  if (!dblk::load_text(blk, make_span_const(n->script), dblk::ReadFlag::ROBUST, curDagFname))
    logwarn("skip reading BLK from %s : node[\"%s\"].script=\n%s", curDagFname, n->name, n->script);

  const char *normalsType = blk.getStr("normals", NULL);
  bool specialNormals = false;
  if (normalsType && (strcmp(normalsType, "spherical") == 0 || strcmp(normalsType, "world") == 0 || strcmp(normalsType, "local") == 0))
    specialNormals = true;

  if (link_to == n || specialNormals || blk.getBool("billboard", false))
    link_to = n;
  else if (allow_sep_flags && (blk.getBool("animated_node", false) || blk.getBool("separate_mesh_node", false)))
    link_to = n;
  else if (link_nodes.getNameId(n->name) != -1)
    link_to = n;
  else if (all_animated)
    link_to = n;
  else if (blk.getBool("point_cloud", false))
    link_to = n;
  else if ((n->flags & NODEFLG_RENDERABLE) && n->mat && n->mat->subMatCount() && n->obj && n->obj->isSubOf(OCID_MESHHOLDER))
  {
    MeshHolderObj &mh = *(MeshHolderObj *)n->obj;
    if (!process_ri)
      if ((link_to->flags & NODEFLG_RENDERABLE) && link_to->obj && link_to->obj->isSubOf(OCID_MESHHOLDER) &&
          ((MeshHolderObj *)link_to->obj)->bones)
      {
        // debug("skip collapsing node obj <%s> to skin_node <%s>", n->name, link_to->name);
        goto skip_collapse;
      }

    if ((process_ri && mh.bones) || !mh.mesh)
    {
      // debug("removed node obj <%s>", n->name.str());
      del_it(n->obj);
      goto skip_collapse;
    }

    if (!(link_to->flags & NODEFLG_RENDERABLE))
    {
      // clear non-renderable target node
      del_it(link_to->obj);
      del_it(link_to->mat);
      link_to->flags |= NODEFLG_RENDERABLE;
    }

    if (link_to->obj && !link_to->obj->isSubOf(OCID_MESHHOLDER))
    {
      // remove non-mesh object if present
      del_it(link_to->obj);
      del_it(link_to->mat);
    }

    // allocate mesh/mat if not present
    if (!link_to->obj)
      link_to->obj = new MeshHolderObj(new Mesh);
    if (!link_to->mat)
      link_to->mat = new MaterialDataList;

    MeshHolderObj &mh_to = *(MeshHolderObj *)link_to->obj;
    if (!mh_to.mesh)
      mh_to.mesh = new Mesh;

    // collapse mesh to 'link_to' node
    Mesh &mesh_to = *mh_to.mesh;
    Mesh &mesh = *mh.mesh;
    TMatrix tm = inverse(link_to->wtm) * n->wtm;
    if (!mesh_to.vertnorm.size())
    {
      G_VERIFY(mesh_to.calc_ngr());
      if (mesh_to.face.size())
        G_VERIFY(mesh_to.calc_vertnorms());
    }
    if (!mesh.vertnorm.size())
    {
      G_VERIFY(mesh.calc_ngr());
      if (mesh.face.size())
        G_VERIFY(mesh.calc_vertnorms());
    }
    if (process_ri && tm.det() < 0) // invert normals only for rendInst case (when skin_nodes passed nullptr)
    {
      for (int i = 0; i < mesh.vertnorm.size(); ++i)
        mesh.vertnorm[i] = -mesh.vertnorm[i];
      for (int i = 0; i < mesh.facenorm.size(); ++i)
        mesh.facenorm[i] = -mesh.facenorm[i];
    }
    mesh.transform(tm);

    int mat_num = n->mat->list.size();
    int org_mat = append_items(link_to->mat->list, n->mat->list.size(), n->mat->list.data());
    mesh_to.clampMaterials(org_mat - 1);
    mesh.clampMaterials(mat_num - 1);
    for (int i = 0; i < mesh.getFace().size(); ++i)
      mesh.setFaceMaterial(i, mesh.getFaceMaterial(i) + org_mat);

    int t0 = get_time_msec();
    mesh.optimize_tverts();
    if (get_time_msec() > t0 + 1000)
      logwarn("node=\"%s\" mesh.optimize_tverts() takes %.2f sec; ", n->name, (get_time_msec() - t0) / 1000.f);
    mesh_to.attach(mesh);
    mesh_to.ngr.clear();

    del_it(n->mat);
    del_it(n->obj);
    // debug("collapsed node mesh: <%s> -> <%s>", n->name.str(), link_to->name.str());
  }

skip_collapse:
  for (int i = n->child.size() - 1; i >= 0; i--)
    collapse_nodes(n->child[i], link_to, allow_sep_flags, all_animated, curDagFname, link_nodes, process_ri);
}

static void collapse_materials(Mesh &m, dag::ConstSpan<ShaderMaterial *> mat)
{
  if (mat.size() <= 1)
    return;
  static Tab<int> remap(inimem);
  int num_remap = 0;

  remap.resize(mat.size());
  mem_set_ff(remap);
  for (int i = 0; i < mat.size(); i++)
  {
    remap[i] = i;
    for (int j = 0; j < i; j++)
      if (mat[i] == mat[j])
      {
        remap[i] = j;
        num_remap++;
        break;
      }
  }
  if (num_remap)
  {
    for (int i = 0; i < m.getFace().size(); i++)
      m.setFaceMaterial(i, remap[m.getFaceMaterial(i)]);
    // debug("collapsed mats: %d -> %d", mat.size(), mat.size()-num_remap);
  }
}

static void add_skin_node_names(Node *n, OAHashNameMap<true> &skinNodes)
{
  if (n->obj && n->obj->isSubOf(OCID_MESHHOLDER))
  {
    MeshHolderObj &mh = *(MeshHolderObj *)n->obj;
    if (mh.bones)
    {
      skinNodes.addNameId(n->name);
      for (int i = 0; i < mh.bones->boneNames.size(); i++)
        skinNodes.addNameId(mh.bones->boneNames[i]);
    }
  }

  for (int i = 0; i < n->child.size(); i++)
    add_skin_node_names(n->child[i], skinNodes);
}
