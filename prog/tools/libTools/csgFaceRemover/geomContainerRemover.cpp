// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <math/dag_math3d.h>
#include <math/dag_mesh.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <stdio.h>
#include <libtools/csgFaceRemover/csgFaceRemover.h>
#include <libtools/csgFaceRemover/geomContainerRemover.h>

struct CSGNode
{
  StaticGeometryNode *node;
  void *poly;
  bool owned; // poly should be removed
  int id;
  BBox3 box;
  CSGNode(StaticGeometryNode *n, void *p, int i, BBox3 &b) : node(n), poly(p), id(i), box(b), owned(true) {}
};

void remove_inside_faces(StaticGeometryContainer &cont, RemoverNotificationCB *cb)
{
  BaseCSGRemoval *csg = make_new_csg();
  Tab<CSGNode> polys(tmpmem);
  polys.reserve(cont.nodes.size());
  if (cb)
    cb->note("Start CSG face removing");

  BBox3 bbox;
  int removedFaces = 0, totalFaces = 0;

  // mark all nodes as unvisited
  for (int i = 0; i < cont.nodes.size(); ++i)
  {
    StaticGeometryNode &node = *cont.nodes[i];
    if (!node.mesh)
      continue;
    node.mesh->user_data = 0;
  }
  int facesinInvalidNodes = 0;
  for (int i = 0; i < cont.nodes.size(); ++i)
  {
    StaticGeometryNode &node = *cont.nodes[i];
    if (!node.mesh || node.mesh->user_data == (void *)1) // invalid mesh
      continue;
    void *poly = NULL;

    node.mesh->mesh.kill_unused_verts(0.0001f);
    node.mesh->mesh.kill_bad_faces();
    totalFaces += node.mesh->mesh.getFaceCount();

    DAGOR_TRY { poly = csg->create_poly(&node.wtm, node.mesh->mesh, i, false, bbox); }
    DAGOR_CATCH(...)
    {
      poly = NULL;
      if (cb)
      {
        char buf[256];
        _snprintf(buf, 256, "Error: An unknown error has occurred. Skip node:%s.", (const char *)node.name);
        cb->warning(buf);
      }
    }

    node.mesh->user_data = poly ? (void *)2 : (void *)1;

    if (poly)
    {
      polys.push_back(CSGNode(&node, poly, i, bbox));
      if (node.mesh->getRefCount() > 1)
      {
        // clone mesh when it is used by more than one node
        StaticGeometryMesh *m = new StaticGeometryMesh;
        m->mats = node.mesh->mats;
        m->mesh = node.mesh->mesh;
        node.mesh = m;
      }
    }
    else
    {
      facesinInvalidNodes += node.mesh->mesh.face.size();
      char buf[256];
      _snprintf(buf, 255, "CSG:Skipping node:%s.", (const char *)node.name);
      if (cb)
        cb->warning(buf);
    }
  }
  SmallTab<int, TmpmemAlloc> polysMainNode;
  clear_and_resize(polysMainNode, polys.size());
  mem_set_ff(polysMainNode);

  if (cb && facesinInvalidNodes)
  {
    char buf[256];
    _snprintf(buf, 255, "<%d> faces in invalid nodes.", facesinInvalidNodes);
    cb->note(buf);
  }

  Tab<Tabint> polysLists(tmpmem);
  polysLists.reserve(polys.size());
  // form different chains of mutually intersecting nodes (potentially intersecting, via bbox)
  for (int i = 0; i < polys.size(); ++i)
  {
    if (polysMainNode[i] >= 0) // already in some chain
      continue;
    // start a new chain
    polysMainNode[i] = i;
    int newListId = append_items(polysLists, 1);
    Tabint &currentList = polysLists[newListId];
    currentList.push_back(i);
    for (;;)
    {
      bool nodeAdded = false;
      for (int j = 0; j < polys.size(); ++j)
      {
        if (polysMainNode[j] >= 0) // already in some chain
          continue;
        bool intersects = false;
        for (int k = 0; k < currentList.size(); ++k)
        {
          G_ASSERT(polysMainNode[currentList[k]] == i);
          if ((polys[currentList[k]].box & polys[j].box)) // intersecting nodes
          {
            intersects = true;
            break;
          }
        }
        if (intersects)
        {
          currentList.push_back(j);
          polysMainNode[j] = i;
          nodeAdded = true;
        }
      }
      if (!nodeAdded)
        break;
    }
  }

  SmallTab<void *, TmpmemAlloc> unionResults;
  clear_and_resize(unionResults, polysLists.size());

  // this cycle could be parallalized/multithreaded
  for (int i = 0; i < polysLists.size(); ++i)
  {
    if (polysLists[i].size() <= 1)
    {
      unionResults[i] = NULL;
      continue;
    }
    void *left = polys[polysLists[i][0]].poly;
    for (int j = 1; j < polysLists[i].size(); ++j)
    {
      void *right = polys[polysLists[i][j]].poly;
      void *result = NULL;
      DAGOR_TRY { result = csg->makeUnion(left, right); }
      DAGOR_CATCH(...)
      {
        if (cb)
          cb->warning("An unknown error has occurred. Can't union some nodes");
      }
      if (result)
      {
        csg->delete_poly(left);
        csg->delete_poly(right);
        polys[polysLists[i][j]].poly = NULL;
        left = result;
      }
    }
    unionResults[i] = left;
    if (polys[polysLists[i][0]].poly != unionResults[i])
      polys[polysLists[i][0]].poly = NULL;
    for (int j = 0; j < polysLists[i].size(); ++j)
      if (!polys[polysLists[i][j]].poly)
      {
        polys[polysLists[i][j]].poly = unionResults[i];
        polys[polysLists[i][j]].owned = false;
      }
  }

  // this cycle could be parallalized/multithreaded
  for (int i = 0; i < polys.size(); ++i)
    if (polys[i].poly)
      removedFaces += csg->removeUnusedFaces(polys[i].node->mesh->mesh, polys[i].poly, polys[i].id);
    else
      debug("wtf??? empty");

  char buf[256];
  _snprintf(buf, 256, "Removed %d out of %d total faces (%d percent)", removedFaces, totalFaces,
    totalFaces ? removedFaces * 100 / totalFaces : 0);
  cb->note(buf);

  for (int i = 0; i < polys.size(); ++i)
    if (polys[i].owned)
      csg->delete_poly(polys[i].poly);

  for (int i = 0; i < unionResults.size(); ++i)
    csg->delete_poly(unionResults[i]);

  // csg->delete_poly(result);
  delete_csg(csg);

  if (cb)
    cb->note("End CSG face removing");
}
