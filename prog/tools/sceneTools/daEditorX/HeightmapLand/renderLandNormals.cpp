// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <obsolete/dag_cfg.h>
#include <coolConsole/coolConsole.h>
#include <EditorCore/ec_IEditorCore.h>
#include "hmlPlugin.h"
#include "landMeshMap.h"
#include "libTools/math/gouraud.h"
#include "renderLandNormals.h"
#include <util/dag_bitArray.h>
#include <debug/dag_debug.h>

using editorcore_extapi::dagGeom;

float *NormalFrameBuffer::start_ht = 0;


static uint16_t isLandMesh = 0;
class OpaqueShader
{
public:
  static inline void shade(const NormalFrameBuffer &pixel, const Point3 &val)
  {
    if (*pixel.ht > val.x)
      return;
    *pixel.ht = val.x;
    if (isLandMesh)
    { // do not calculate normal, save only height - we'll use heightmap normal instead
      // ugly hack: do not even save invalid normal - we render landmesh first,
      //   so we don't need to write invalid value
      //*pixel.normal = 0xFFFF;
      return;
    }
    Point3 nrm(val.y, sqrtf(1 - val.y * val.y - val.z * val.z), val.z);
    float lg = length(nrm);
    float scale = 0.5f / lg;
    pixel.normal[pixel.ht - pixel.start_ht] = real2uchar(nrm.x * scale + 0.5) | (real2uchar(nrm.z * scale + 0.5) << 8);
    // makes unparallelable!!!
  }
};


static void render_mesh(Gouraud<OpaqueShader, Point3, NormalFrameBuffer> &shading, Mesh &m, float sx, float sy, float scale,
  Bitarray *used_mats = NULL)
{
  for (int i = 0; i < m.face.size(); ++i)
  {
    Face &face = m.face[i];
    if (used_mats)
    {
      int mat = face.mat;
      if (mat >= used_mats->size())
        mat = used_mats->size() - 1;
      if (mat < 0 || !used_mats->get(mat))
        continue;
    }
    Point2 scr[3];
    Point3 interp[3];
    for (int vi = 0; vi < 3; vi++)
    {
      Point3 v = m.vert[face.v[vi]];
      scr[vi] = Point2(v.x - sx, v.z - sy) * scale;
      if (!isLandMesh)
      {
        Point3 vn = -m.vertnorm[m.facengr[i][vi]];
        if (vn.y < 0)
          vn = -vn;
        interp[vi] = Point3(v.y, vn.x, vn.z);
      }
    }
    shading.draw_triangle(scr, interp);
  }
}

bool create_lmesh_normal_map(LandMeshMap &land, NormalFrameBuffer map, float sx, float sy, float scale, int wd, int ht)
{
  CoolConsole &con = DAGORED2->getConsole();

  con.setActionDesc("gather static visual...");
  con.setTotal(DAGORED2->getPluginCount());
  StaticGeometryContainer *geoCont = dagGeom->newStaticGeometryContainer();
  for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
  {
    con.setDone(i);
    IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);

    IGatherStaticGeometry *geom = plugin->queryInterface<IGatherStaticGeometry>();
    if (!geom)
      continue;

    geom->gatherStaticVisualGeometry(*geoCont);
  }
  con.endProgress();
  if (!geoCont->nodes.size())
  {
    con.addMessage(ILogWriter::NOTE, "No other meshes - not rendering anything");
    return false;
  }


  con.setActionDesc("rendering lmesh geom...");
  Gouraud<OpaqueShader, Point3, NormalFrameBuffer> shading;
  shading.init(map, wd, ht);
  int faces = 0;
  isLandMesh = 1;
  for (int y = 0; y < land.getNumCellsY(); ++y)
    for (int x = 0; x < land.getNumCellsX(); ++x)
    {
      Mesh *m = land.getCellLandMesh(x, y);
      if (!m)
        continue;
      faces += m->face.size();
      render_mesh(shading, *m, sx, sy, scale);
    }
  con.setActionDesc("rendering static geom...");
  con.setTotal(geoCont->nodes.size() / 32);

  isLandMesh = 0;
  Bitarray used_mats;
  for (int i = 0; i < geoCont->nodes.size(); ++i)
  {
    if (!(i & 31))
      con.setDone(i / 32);

    StaticGeometryNode *node = geoCont->nodes[i];

    if (!node)
      continue;

    used_mats.resize(node->mesh->mats.size());
    int any_landmesh = 0;
    for (int mi = 0; mi < node->mesh->mats.size(); ++mi)
    {
      //==fixme: name could be moved to application.blk
      bool iscombined = false;
      if (strstr(node->mesh->mats[mi]->className.str(), "land_mesh") && !strstr(node->mesh->mats[mi]->className.str(), "decal") &&
          !strstr(node->mesh->mats[mi]->className.str(), "land_mesh_clipmap"))
      {
        CfgReader c;
        c.getdiv_text(String(128, "[q]\r\n%s\r\n", node->mesh->mats[mi]->scriptText.str()), "q");
        if (c.getbool("render_landmesh_combined", 1))
          iscombined = true;
      }

      if (iscombined)
      {
        used_mats.set(mi, 1);
        any_landmesh++;
      }
      else
        used_mats.set(mi, 0);
    }
    if (!any_landmesh)
      continue;
    Mesh m = node->mesh->mesh;
    if (m.vertnorm.empty())
    {
      // Generate normals for cliffs, castles and road ends.
      m.calc_ngr();
      m.calc_vertnorms();
    };
    m.transform(node->wtm);
    faces += m.face.size();
    if (m.vertnorm.empty())
      ; // no-op
    else
    {
      // fixme: - should not be needed any more, since we use mesh.transform()
      //  Invert road normals.
      // for (unsigned int normNo = 0; normNo < m.vertnorm.size(); normNo++)
      //   m.vertnorm[normNo] = -m.vertnorm[normNo];
    }
    render_mesh(shading, m, sx, sy, scale, &used_mats);
  }
  dagGeom->deleteStaticGeometryContainer(geoCont);
  con.endProgress();
  con.endLog();
  con.addMessage(ILogWriter::REMARK, "%d total faces rendered", faces);
  return true;
}
