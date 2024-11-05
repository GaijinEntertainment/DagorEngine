//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <de3_interface.h>
#include <libTools/staticGeom/staticGeometryContainer.h>


enum class GetOccluderFromGeomResultType
{
  Box,
  Quad,
  Error
};

static GetOccluderFromGeomResultType getOcclFromGeomNode(const StaticGeometryNode &node, const char *asset_name, TMatrix &box_occl,
  Point3 *quad_occl_v)
{
  GetOccluderFromGeomResultType result = GetOccluderFromGeomResultType::Error;

  if (node.flags & StaticGeometryNode::FLG_OCCLUDER)
  {
    const char *occl_type = node.script.getStr("occluderType", "box");
    if (strcmp(occl_type, "box") == 0)
    {
      if (node.mesh.get())
      {
        result = GetOccluderFromGeomResultType::Box;

        const Mesh &m = node.mesh->mesh;
        BBox3 b;
        for (int v = 0; v < m.vert.size(); v++)
          b += m.vert[v];

        TMatrix &bo = box_occl;
        bo.setcol(0, node.wtm.getcol(0) * b.width().x);
        bo.setcol(1, node.wtm.getcol(1) * b.width().y);
        bo.setcol(2, node.wtm.getcol(2) * b.width().z);
        if (bo.det() < 0)
          bo.setcol(1, -bo.getcol(1));
        bo.setcol(3, node.wtm * b.center());
      }
      else
        DAEDITOR3.conError("no mesh for occluder in node <%s> of asset <%s>", node.name.str(), asset_name);
    }
    else if (strcmp(occl_type, "quad") == 0)
    {
      if (node.mesh.get())
      {
        const Mesh &m = node.mesh->mesh;
        if (m.face.size() == 2 || m.vert.size() == 4)
        {
          int i0 = m.face[0].v[0], i1 = m.face[0].v[1], i2 = -1, i3 = m.face[0].v[2];
          for (int v = 0; v < 3; v++)
            if (m.face[1].v[v] != i0 && m.face[1].v[v] != i1 && m.face[1].v[v] != i3)
              i2 = m.face[1].v[v];
          if (i2 != -1)
          {
            result = GetOccluderFromGeomResultType::Quad;

            quad_occl_v[0] = node.wtm * m.vert[i0];
            quad_occl_v[1] = node.wtm * m.vert[i1];
            quad_occl_v[2] = node.wtm * m.vert[i2];
            quad_occl_v[3] = node.wtm * m.vert[i3];
          }
          else
            DAEDITOR3.conError("bad mesh for quad occluder in node <%s> of asset <%s>", node.name.str(), asset_name);
        }
        else
          DAEDITOR3.conError("bad mesh (%d verts, %d faces) for quad occluder "
                             "in node <%s> of asset <%s>",
            m.vert.size(), m.face.size(), node.name.str(), asset_name);
      }
      else
        DAEDITOR3.conError("no mesh for occluder in node <%s> of asset <%s>", node.name.str(), asset_name);
    }
    else
      DAEDITOR3.conError("wrong occluderType=\"%s\" in node <%s> of asset <%s>", occl_type, node.name.str(), asset_name);
  }

  return result;
}

static void getOcclFromGeom(const StaticGeometryContainer &g, const char *asset_name, Tab<TMatrix> &box_occl, Tab<Point3> &quad_occl_v)
{
  box_occl.clear();
  quad_occl_v.clear();

  for (int i = 0; i < g.nodes.size(); ++i)
  {
    const StaticGeometryNode *node = g.nodes[i];

    if (node)
    {
      TMatrix box;
      Point3 quad[4];
      GetOccluderFromGeomResultType result = getOcclFromGeomNode(*node, asset_name, box, quad);
      switch (result)
      {
        case GetOccluderFromGeomResultType::Box: box_occl.push_back(box); break;
        case GetOccluderFromGeomResultType::Quad:
        {
          int base = append_items(quad_occl_v, 4);
          quad_occl_v[base + 0] = quad[0];
          quad_occl_v[base + 1] = quad[1];
          quad_occl_v[base + 2] = quad[2];
          quad_occl_v[base + 3] = quad[3];
        }
        break;
        case GetOccluderFromGeomResultType::Error: break;
      }
    }
  }
}
