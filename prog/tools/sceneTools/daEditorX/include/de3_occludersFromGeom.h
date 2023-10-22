//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <de3_interface.h>
#include <libTools/staticGeom/staticGeometryContainer.h>

static void getOcclFromGeom(const StaticGeometryContainer &g, const char *asset_name, Tab<TMatrix> &box_occl, Tab<Point3> &quad_occl_v)
{
  box_occl.clear();
  quad_occl_v.clear();

  for (int i = 0; i < g.nodes.size(); ++i)
  {
    const StaticGeometryNode *node = g.nodes[i];

    if (node && (node->flags & StaticGeometryNode::FLG_OCCLUDER))
    {
      const char *occl_type = node->script.getStr("occluderType", "box");
      if (strcmp(occl_type, "box") == 0)
      {
        if (node->mesh.get())
        {
          const Mesh &m = node->mesh->mesh;
          BBox3 b;
          for (int v = 0; v < m.vert.size(); v++)
            b += m.vert[v];
          TMatrix &bo = box_occl.push_back();
          bo.setcol(0, node->wtm.getcol(0) * b.width().x);
          bo.setcol(1, node->wtm.getcol(1) * b.width().y);
          bo.setcol(2, node->wtm.getcol(2) * b.width().z);
          if (bo.det() < 0)
            bo.setcol(1, -bo.getcol(1));
          bo.setcol(3, node->wtm * b.center());
        }
        else
          DAEDITOR3.conError("no mesh for occluder in node <%s> of asset <%s>", node->name.str(), asset_name);
      }
      else if (strcmp(occl_type, "quad") == 0)
      {
        if (node->mesh.get())
        {
          const Mesh &m = node->mesh->mesh;
          if (m.face.size() == 2 || m.vert.size() == 4)
          {
            int i0 = m.face[0].v[0], i1 = m.face[0].v[1], i2 = -1, i3 = m.face[0].v[2];
            for (int v = 0; v < 3; v++)
              if (m.face[1].v[v] != i0 && m.face[1].v[v] != i1 && m.face[1].v[v] != i3)
                i2 = m.face[1].v[v];
            if (i2 != -1)
            {
              int base = append_items(quad_occl_v, 4);
              quad_occl_v[base + 0] = node->wtm * m.vert[i0];
              quad_occl_v[base + 1] = node->wtm * m.vert[i1];
              quad_occl_v[base + 2] = node->wtm * m.vert[i2];
              quad_occl_v[base + 3] = node->wtm * m.vert[i3];
            }
            else
              DAEDITOR3.conError("bad mesh for quad occluder in node <%s> of asset <%s>", node->name.str(), asset_name);
          }
          else
            DAEDITOR3.conError("bad mesh (%d verts, %d faces) for quad occluder "
                               "in node <%s> of asset <%s>",
              m.vert.size(), m.face.size(), node->name.str(), asset_name);
        }
        else
          DAEDITOR3.conError("no mesh for occluder in node <%s> of asset <%s>", node->name.str(), asset_name);
      }
      else
        DAEDITOR3.conError("wrong occluderType=\"%s\" in node <%s> of asset <%s>", occl_type, node->name.str(), asset_name);
    }
  }
}
