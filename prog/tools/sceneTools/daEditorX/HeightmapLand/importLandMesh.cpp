// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <de3_interface.h>
#include <de3_genObjUtil.h>
#include <de3_rasterizePoly.h>
#include <assets/asset.h>
#include "editorLandRayTracer.h"
#include <landMesh/lmeshRenderer.h>
#include <landMesh/lmeshManager.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_btagCompr.h>
#include <image/dag_texPixel.h>
#include <libTools/staticGeom/matFlags.h>
#include <libTools/staticGeom/geomObject.h>
#include <libTools/util/makeBindump.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <math/dag_mesh.h>
#include <libTools/dagFileRW/dagExporter.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <coolConsole/coolConsole.h>
#include <EditorCore/ec_IEditorCore.h>
#include <sceneRay/dag_sceneRay.h>
#include <sceneRay/dag_sceneRayBuildable.h>
#include <libTools/staticGeom/staticGeometryContainer.h>

#include <oldEditor/de_workspace.h>

#include <de3_hmapService.h>

#include <hash/md5.h>

#include "hmlPlugin.h"
#include "hmlSplineObject.h"
#include "landMeshMap.h"
#include <libTools/delaunay/delaunay.h>
#include <math/DelaunayTriangulator/source/DelaunayTriangulation.h>
#include <math/DelaunayTriangulator/source/TIN.h>

#include <heightMapLand/dag_hmlGetHeight.h>
#include <heightMapLand/dag_hmlTraceRay.h>
#include <gameRes/dag_gameResSystem.h>
#include <util/dag_bitArray.h>
#include <util/dag_texMetaData.h>
#include <obsolete/dag_cfg.h>

#include <debug/dag_debug.h>

using editorcore_extapi::dagGeom;
using editorcore_extapi::dagRender;
using editorcore_extapi::dagTools;

static const int MD5_LEN = 16;
extern bool allow_debug_bitmap_dump;
extern bool guard_det_border;
#define BOTTOM_LOD_HEIGHT 20
#define ABOVE_LOD_HEIGHT  2.f
enum
{
  BOTTOM_ABOVE = 0,
  BOTTOM_BELOW = 1,
  BOTTOM_COUNT
};

LandMeshMap::LandMeshMap() : size(0, 0), origin(0, 0), cellSize(0), editorLandTracer(NULL), materials(midmem), textures(tmpmem)
{
  gameLandTracer = NULL;
  landMatSG = new StaticGeometryMaterial;
  landMatSG->name = "LandMeshGenerated@hmap";
  landMatSG->className = "land_mesh";

  landBottomMatSG = new StaticGeometryMaterial;
  landBottomMatSG->name = "LandMeshBottomGenerated";
  landBottomMatSG->className = "land_mesh";
  landBottomMatSG->scriptText = "bottom=1";

  landBottomDeepMatSG = new StaticGeometryMaterial;
  landBottomDeepMatSG->name = "LandMeshBottomLodGenerated";
  landBottomDeepMatSG->className = "land_mesh";
  landBottomDeepMatSG->scriptText = "bottom=2";

  landBottomBorderMatSG = new StaticGeometryMaterial;
  landBottomBorderMatSG->name = "LandMeshBottomLodGenerated";
  landBottomBorderMatSG->className = "land_mesh";
  landBottomBorderMatSG->scriptText = "bottom=-1";

  addMaterial(landMatSG, NULL, NULL);
  addMaterial(landBottomMatSG, NULL, NULL);
  addMaterial(landBottomDeepMatSG, NULL, NULL);
  addMaterial(landBottomBorderMatSG, NULL, NULL);
  collisionNode = NULL;
}

LandMeshMap::~LandMeshMap() { clear(true); }

void LandMeshMap::gatherExportToDagGeometry(StaticGeometryContainer &container)
{
  MaterialDataList *mdl = new MaterialDataList;
  for (int materialNo = 0; materialNo < getMaterialsCount(); materialNo++)
    mdl->addSubMat(new MaterialData(getMaterialData(materialNo)));

  for (unsigned int cellNo = 0; cellNo < getCells().size(); cellNo++)
  {
    String nodeName(100, "LandMesh%02d", cellNo);
    StaticGeometryContainer *tmpContainer = dagGeom->newStaticGeometryContainer(); // This horrors are taken from
                                                                                   // SplineObject::PolyGeom::createMeshes.
    Mesh *mesh = dagGeom->newMesh();
    *mesh = *getCells()[cellNo].land_mesh;
    dagGeom->objCreator3dAddNode(nodeName, mesh, mdl, *tmpContainer);
    for (unsigned int nodeNo = 0; nodeNo < tmpContainer->nodes.size(); nodeNo++)
    {
      StaticGeometryNode *node = tmpContainer->nodes[nodeNo];
      dagGeom->staticGeometryNodeCalcBoundBox(*node);
      dagGeom->staticGeometryNodeCalcBoundSphere(*node);
      container.addNode(dagGeom->newStaticGeometryNode(*node, tmpmem));
    }
    dagGeom->deleteStaticGeometryContainer(tmpContainer);
  }
}

void LandMeshMap::buildCollisionNode(Mesh &m) {}

void LandMeshMap::addStaticCollision(StaticGeometryContainer &cont) {}

void LandMeshMap::getStats(int &min_faces, int &max_faces, int &total_faces)
{
  max_faces = 0;
  min_faces = 1 << 30;
  total_faces = 0;

  for (int i = 0; i < cells.size(); ++i)
  {
    int nf = 0;
    if (cells[i].land_mesh)
      nf = cells[i].land_mesh->face.size();
    if (cells[i].combined_mesh)
      nf += cells[i].combined_mesh->face.size();

    if (nf > max_faces)
      max_faces = nf;

    if (nf < min_faces)
      min_faces = nf;

    total_faces += nf;
  }
}

void LandMeshMap::clear(bool clear_tracer)
{
  dagGeom->deleteStaticGeometryNode(collisionNode);

  for (int i = 0; i < cells.size(); ++i)
  {
    if (cells[i].land_mesh)
      delete (cells[i].land_mesh);
    if (cells[i].combined_mesh)
      delete (cells[i].combined_mesh);
    if (cells[i].decal_mesh)
      delete (cells[i].decal_mesh);
    if (cells[i].patches_mesh)
      delete (cells[i].patches_mesh);
  }
  clear_and_shrink(cells);
  if (clear_tracer)
  {
    del_it(editorLandTracer);
    del_it(gameLandTracer);
  }

  clear_and_shrink(materials);
  addMaterial(landMatSG, NULL, NULL);
  addMaterial(landBottomMatSG, NULL, NULL);
  addMaterial(landBottomDeepMatSG, NULL, NULL);
  addMaterial(landBottomBorderMatSG, NULL, NULL);
  clear_and_shrink(textures);

  size = IPoint2(0, 0);
}


BBox3 LandMeshMap::getBBox(int x, int y, float *sphere_radius)
{
  x -= origin.x;
  y -= origin.y;

  if (x < 0 || x >= size.x || y < 0 || y >= size.y)
    return BBox3();
  if (sphere_radius)
    *sphere_radius = 0.5 * length(cells[x + y * size.x].box.width());
  return cells[x + y * size.x].box;
}

// traceRay
bool LandMeshMap::traceRay(const Point3 &p, const Point3 &dir, real &t, Point3 *normal) const
{
  return editorLandTracer && editorLandTracer->traceRay(p, dir, t, normal);
}

bool LandMeshMap::getHeight(const Point2 &p, real &ht, Point3 *normal) const
{
  return editorLandTracer && editorLandTracer->getHeight(p, ht, normal);
}

void LandMeshMap::setEditorLandRayTracer(EditorLandRayTracer *lrt)
{
  if (editorLandTracer)
    delete editorLandTracer;
  editorLandTracer = lrt;
}
void LandMeshMap::setGameLandRayTracer(LandRayTracer *lrt)
{
  if (gameLandTracer)
    delete gameLandTracer;
  gameLandTracer = lrt;
}

MaterialData LandMeshMap::getMaterialData(uint32_t matNo) const
{
  MaterialData matNull;
  if (matNo >= materials.size() && !materials[matNo])
  {
    matNull.className = "land_mesh";
    matNull.matName = "default_lmesh";
    return matNull;
  }
  StaticGeometryMaterial &gm = *materials[matNo];

  matNull.matName = gm.name;
  matNull.matScript = gm.scriptText;
  matNull.className = gm.className;

  if ((gm.flags & (MatFlags::FLG_2SIDED | MatFlags::FLG_REAL_2SIDED)) == (MatFlags::FLG_2SIDED | MatFlags::FLG_REAL_2SIDED))
    matNull.flags = 0;
  else
    matNull.flags = (gm.flags & MatFlags::FLG_2SIDED) ? MATF_2SIDED : 0;

  matNull.mat.amb = gm.amb;
  matNull.mat.diff = gm.diff;
  matNull.mat.spec = gm.spec;
  matNull.mat.emis = gm.emis;
  matNull.mat.power = gm.power;

  for (int ti = 0; ti < StaticGeometryMaterial::NUM_TEXTURES && ti < MAXMATTEXNUM; ++ti) //-V560
  {
    if (!gm.textures[ti])
      continue;
    matNull.mtex[ti] = dagRender->addManagedTexture(gm.textures[ti]->fileName);
  }
  //==
  // fixme: todo valid
  // data.className="land_mesh";
  return matNull;
}

int LandMeshMap::addTexture(StaticGeometryTexture *texture)
{
  if (!texture)
    return -1;
  for (int i = 0; i < textures.size(); ++i)
    if (textures[i] == texture)
      return i;
  textures.push_back(texture);
  return textures.size() - 1;
}

int LandMeshMap::addMaterial(StaticGeometryMaterial *material, bool *clipmap_only, bool *has_vertex_opacity, int node_layer,
  bool *is_height_patch)
{
  if (clipmap_only)
    *clipmap_only = false;
  if (!material)
    return -1;

  if (!strstr(material->className.str(), "land_mesh"))
    return -1;
  if (strstr(material->className.str(), "land_mesh_clipmap"))
    return -1;
  bool is_decal = false;
  bool is_patch = false;
  if (strstr(material->className.str(), "height_patch"))
    is_patch = true;
  else if (strstr(material->className.str(), "decal"))
    is_decal = true;
  else if (strstr(material->scriptText.str(), "render_landmesh_combined"))
  {
    CfgReader c;
    c.getdiv_text(String(128, "[q]\r\n%s\r\n", material->scriptText.str()), "q");
    if (!c.getbool("render_landmesh_combined", 1))
      is_decal = true;
  }
  if (has_vertex_opacity && strstr(material->scriptText.str(), "vertex_opacity"))
  {
    CfgReader c;
    c.getdiv_text(String(128, "[q]\r\n%s\r\n", material->scriptText.str()), "q");
    if (c.getbool("vertex_opacity", 0))
      *has_vertex_opacity = true;
    else
      *has_vertex_opacity = false;
  }
  if (clipmap_only)
    *clipmap_only = is_decal;
  if (is_height_patch)
    *is_height_patch = is_patch;

  Ptr<StaticGeometryMaterial> copyMaterial = new StaticGeometryMaterial(*material);
  if (node_layer != -1 && !strstr(copyMaterial->scriptText, "node_layer=")) // Add spline layer only for comparison to avoid joining of
                                                                            // the meshes from different layers.
    copyMaterial->scriptText = String(0, "%snode_layer=%d\r\n", copyMaterial->scriptText.str(), node_layer);

  for (int i = 0; i < materials.size(); ++i)
    if (materials[i]->equal(*copyMaterial))
      return i;

  for (int i = 0; i < StaticGeometryMaterial::NUM_TEXTURES; ++i)
    addTexture(material->textures[i]);

  if (strcmp("land_mesh", material->className.str()) != 0)
  {
    if (is_decal && !strstr(material->className.str(), "decal"))
      copyMaterial->className = String(128, "%s_decal", material->className.str());
  }
  debug("adding material = %s", copyMaterial->className.str());
  materials.push_back(copyMaterial);
  return materials.size() - 1;
}

static void getWorldMeshes(Node &n, Tab<Node *> &meshes)
{
  if ((n.flags & NODEFLG_RENDERABLE) && n.obj && n.obj->isSubOf(OCID_MESHHOLDER))
  {
    MeshHolderObj &mh = *(MeshHolderObj *)n.obj;
    if (mh.mesh)
    {
      Mesh &m = *mh.mesh;
      for (int i = 0; i < m.vert.size(); ++i)
        m.vert[i] = n.wtm * m.vert[i];

      meshes.push_back(&n);
    }
  }

  for (int i = 0; i < n.child.size(); ++i)
    if (n.child[i])
      getWorldMeshes(*n.child[i], meshes);
}

//-----------------------------------------------------------------------------

class DelaunayHeightMap : public delaunay::Map
{
  HeightMapStorage &heightmap;

public:
  DelaunayHeightMap(HeightMapStorage &map) : heightmap(map) {}
  virtual int width() { return heightmap.getInitialMap().getMapSizeX(); }
  virtual int height() { return heightmap.getInitialMap().getMapSizeY(); }

  virtual float eval(int i, int j) { return heightmap.getFinalData(i, j); }
};

class DelaunayImportanceMask : public delaunay::ImportMask
{
public:
  DelaunayImportanceMask(IBitMaskImageMgr::BitmapMask *in_mask, objgenerator::HugeBitmask *excl_mask, float mask_scale, int hmap_width,
    int hmap_height) :
    mask(in_mask), exclMask(excl_mask), maskScale(0.125f * mask_scale / 255.0), hmapWidth(hmap_width), hmapHeight(hmap_height)
  {
    G_ASSERT(mask);
    G_ASSERT(mask->getBitsPerPixel() == 8);
  }

  virtual float apply(int x, int y, float val)
  {
    if (exclMask && x >= 0 && y >= 0 && x < exclMask->getW() && y < exclMask->getH() && exclMask->get(x, y))
      return -0.1f;

    float maskVal =
      maskScale * mask->getMaskPixel8(x * mask->getWidth() / hmapWidth, mask->getHeight() - y * mask->getHeight() / hmapHeight - 1);

    return val * (maskVal + 1.f);
  }

  void addHash(md5_state_t *state) const
  {
    md5_append(state, (unsigned char *)&hmapWidth, sizeof(int));
    md5_append(state, (unsigned char *)&hmapHeight, sizeof(int));
    md5_append(state, (unsigned char *)&maskScale, sizeof(float));
    HmapLandPlugin::bitMaskImgMgr->calcBitMaskMD5(*mask, state);
    if (exclMask)
    {
      int w = exclMask->getW();
      unsigned char *row = new unsigned char[w];
      for (int y = 0, ye = exclMask->getH(); y < ye; y++)
      {
        for (int x = 0; x < w; x++)
          row[x] = exclMask->get(x, y);
        md5_append(state, row, w);
      }
      delete[] row;
    }
  }

protected:
  IBitMaskImageMgr::BitmapMask *mask;
  objgenerator::HugeBitmask *exclMask;
  int hmapWidth, hmapHeight;
  float maskScale;
};

static void saveCache(const char *fnameData, MemoryChainedData *mem, unsigned char *hash)
{
  if (!mem)
    return;

  unsigned char md5Hash[MD5_LEN];

  // Data
  FullFileSaveCB fileData(fnameData);
  if (!fileData.fileHandle)
    return;
  write_zeros(fileData, MD5_LEN * 2);

  md5_state_t hashState;
  md5_init(&hashState);

  MemoryChainedData *ptr = mem;
  while ((ptr) && (ptr->used > 0))
  {
    fileData.write(ptr->data, ptr->used);
    md5_append(&hashState, (unsigned char *)ptr->data, ptr->used);
    ptr = ptr->next;
  }

  md5_finish(&hashState, md5Hash);
  // Hash
  fileData.seekto(0);
  fileData.write(md5Hash, MD5_LEN);
  fileData.write(hash, MD5_LEN);
}

static void loadCache(const char *fnameData, MemoryChainedData *&mem, unsigned char *cur_hash)
{
  if (!::dd_file_exist(fnameData))
    return;

  unsigned char md5Hash[MD5_LEN];
  unsigned char hash[MD5_LEN];

  // Hash
  FullFileLoadCB fileData(fnameData);
  fileData.tryRead(md5Hash, MD5_LEN);
  fileData.tryRead(hash, MD5_LEN);
  if (fileData.tell() != MD5_LEN * 2)
    return;

  if (memcmp(cur_hash, hash, MD5_LEN) != 0)
    return;

  // Data
  MemorySaveCB memSave(NULL, false);

  static const int BUFFER_SIZE = 32 << 10;
  unsigned char buf[BUFFER_SIZE];

  md5_state_t hashState;
  md5_init(&hashState);

  for (;;)
  {
    int read = fileData.tryRead(buf, BUFFER_SIZE);
    if (!read)
      break;
    memSave.write(buf, read);
    md5_append(&hashState, buf, read);
  }

  md5_finish(&hashState, hash);

  if (memcmp(hash, md5Hash, MD5_LEN) != 0)
  {
    MemoryChainedData::deleteChain(memSave.getMem());
    CoolConsole &con = DAGORED2->getConsole();
    con.addMessage(ILogWriter::NOTE, "cache file is damaged!");
    return;
  }

  mem = memSave.getMem();
}

static void calculateHashForFile(md5_state_t &state, const char *file_path)
{
  FullFileLoadCB file(file_path);
  if (!file.fileHandle)
    return;

  const int BUFFER_SIZE = 65536;
  SmallTab<unsigned char, TmpmemAlloc> buff;
  clear_and_resize(buff, BUFFER_SIZE);

  int used = 1;
  while (used)
  {
    used = file.tryRead(buff.data(), BUFFER_SIZE);
    md5_append(&state, buff.data(), used);
  }
}

void LandMeshMap::getMd5HashForGenerate(unsigned char *md5_hash, HeightMapStorage &heightmap, float error_threshold, int point_limit,
  float cell, const Point3 &ofs, const DelaunayImportanceMask *mask, dag::ConstSpan<Point3> add_pts,
  dag::ConstSpan<Point3> water_border_polys)
{
  md5_state_t state;
  md5_init(&state);

  md5_append(&state, (unsigned char *)&error_threshold, sizeof(float));
  md5_append(&state, (unsigned char *)&point_limit, sizeof(int));
  md5_append(&state, (unsigned char *)&cell, sizeof(float));
  md5_append(&state, (unsigned char *)&ofs, sizeof(Point3));

  md5_append(&state, (unsigned char *)&heightmap.heightScale, sizeof(float));
  md5_append(&state, (unsigned char *)&heightmap.heightOffset, sizeof(float));

  if (mask)
    mask->addHash(&state);
  else
    md5_append(&state, ZERO_PTR<unsigned char>(), 8);

  float *row = new float[heightmap.getMapSizeX()];
  for (int y = 0, ey = heightmap.getMapSizeY(); y < ey; y++)
  {
    for (int x = 0, ex = heightmap.getMapSizeX(); x < ex; x++)
      row[x] = heightmap.getFinalData(x, y);
    md5_append(&state, (unsigned char *)row, sizeof(float) * heightmap.getMapSizeX());
  }
  delete[] row;

  md5_append(&state, (unsigned char *)add_pts.data(), data_size(add_pts));
  md5_append(&state, (unsigned char *)water_border_polys.data(), data_size(water_border_polys));
  md5_finish(&state, md5_hash);
}

// return true if loaded from cache
static bool delaunayGenerateCached(LandMeshMap &land, HeightMapStorage &heightmap, float error_threshold, int point_limit,
  delaunay::Map *hmap, Mesh &mesh_out, float cell, const Point3 &ofs, DelaunayImportanceMask *mask, dag::ConstSpan<Point3> add_pts,
  dag::ConstSpan<Point3> water_border_polys)
{
  CoolConsole &con = DAGORED2->getConsole();

  String fnameData(DAGORED2->getPluginFilePath(HmapLandPlugin::self, "delaunayGen.cache.bin"));

  unsigned char currentHash[MD5_LEN];
  land.getMd5HashForGenerate(currentHash, heightmap, error_threshold, point_limit, cell, ofs, mask, add_pts, water_border_polys);

  MemoryChainedData *cachedData = NULL;

  ::loadCache(fnameData, cachedData, currentHash);

  if (!cachedData)
  {
    con.addMessage(ILogWriter::REMARK, "triangulation. processing...");

    delaunay::load(error_threshold, point_limit, hmap, mesh_out, cell, ofs, mask);
    Tab<Point3> add_bottom_pts(tmpmem);

    bool need_re_delaunay = water_border_polys.size() + add_pts.size() > 0;
    int re_delaunay_iter = 0;
    float close_bottom_pt_dist2 = 2 * 2;

    int old_fc = mesh_out.face.size();
    int old_vc = mesh_out.vert.size();

    ctl::PointList boundary;
    if (need_re_delaunay)
    {
      float min_y = 1e6f;
      for (const auto &v : mesh_out.vert)
        inplace_min(min_y, v.y);

      boundary.push_back({ofs.x, ofs.z, min_y});
      boundary.push_back({ofs.x + (heightmap.getMapSizeX() - 1) * cell, ofs.z, min_y});
      boundary.push_back({ofs.x + (heightmap.getMapSizeX() - 1) * cell, ofs.z + (heightmap.getMapSizeY() - 1) * cell, min_y});
      boundary.push_back({ofs.x, ofs.z + (heightmap.getMapSizeY() - 1) * cell, min_y});

      for (const auto &v : mesh_out.vert)
        for (auto &bv : boundary)
          if (lengthSq(Point2::xz(v) - Point2::xy(bv)) < 1e-4f)
            bv.z = v.y;

      DEBUG_DUMP_VAR(min_y);
      DEBUG_DUMP_VAR(boundary.size());
      for (const auto &v : boundary)
        DEBUG_DUMP_VAR(Point3::xzy(v));
    }

    while (need_re_delaunay && re_delaunay_iter < 10)
    {
      debug_flush(true);

      int time0l = dagTools->getTimeMsec();
      mesh_out.face.resize(0);

      ctl::DelaunayTriangulation dt{boundary, 64 << 10};
      bool detected_errors = false;

      // add land boundary points first to force proper sides
      for (const auto &v : mesh_out.vert)
        if (fabs(v.x - boundary[0].x) < 1e-6 || fabs(v.z - boundary[0].y) < 1e-6 || //
            fabs(v.x - boundary[2].x) < 1e-6 || fabs(v.z - boundary[2].y) < 1e-6)
          dt.InsertConstrainedPoint({v.x, v.z, v.y});

      ctl::PointList poly_pts;
      for (int i = 0; i < water_border_polys.size(); i++)
        if (water_border_polys[i].x > 1e12f)
        {
          auto points = make_span_const(&water_border_polys[i + 1], unsigned(water_border_polys[i].y));
          if (water_border_polys[i].z >= 2)
          {
            debug("skip corrupted border %d pts", points.size());
            continue;
          }
          bool closed = water_border_polys[i].z < 0.5f;
          // debug("add new border %d pts, closed=%d", points.size(), closed);
          poly_pts.clear();
          poly_pts.reserve(points.size());
          for (const auto &p : points)
            poly_pts.push_back({p.x, p.z, p.y});
          if (closed)
            poly_pts.push_back({poly_pts[0]});
          dt.InsertConstrainedLineString(poly_pts);
          if (dt.error())
          {
            detected_errors = true;
            logerr("detected corrupted border %d pts, closed=%d", points.size(), closed);
            const_cast<float &>(water_border_polys[i].z) = 2; // mark corrupted
            // reinit DelaunayTriangulation
            dt.~DelaunayTriangulation();
            new (&dt, _NEW_INPLACE) ctl::DelaunayTriangulation{boundary, 64 << 10};
          }
          i += points.size();
        }
      if (detected_errors) // restart with skipping corrupted borders
        continue;

      for (const auto &v : mesh_out.vert)
        dt.InsertConstrainedPoint({v.x, v.z, v.y});
      for (const auto &v : add_pts)
        dt.InsertConstrainedPoint({v.x, v.z, v.y});
      for (const auto &v : add_bottom_pts)
        dt.InsertConstrainedPoint({v.x, v.z, v.y});

      ctl::TIN tin(&dt);

      debug("");
      debug("out: %d vert, %d tri", tin.verts.size(), tin.triangles.size() / 3);
      debug_flush(false);
      double water_lev = HmapLandPlugin::self->getWaterSurfLevel();

      need_re_delaunay = false;
      int prev_bottom_pts = add_bottom_pts.size();
      for (int i = 0; i < tin.triangles.size(); i += 3)
      {
        int vi0 = tin.triangles[i + 0];
        int vi1 = tin.triangles[i + 1];
        int vi2 = tin.triangles[i + 2];
        double h0 = tin.verts[vi0].z - water_lev;
        double h1 = tin.verts[vi1].z - water_lev;
        double h2 = tin.verts[vi2].z - water_lev;
        if (fabs(h0) > 0.2 || fabs(h1) > 0.2 || fabs(h2) > 0.2)
          continue;
        if (h0 < -0.1 && h1 < -0.1 && h2 < -0.1)
          continue;

        Point2 cp = (Point2::xy(tin.verts[vi0]) + Point2::xy(tin.verts[vi1]) + Point2::xy(tin.verts[vi2])) / 3.0f;

        if (!HmapLandPlugin::self->isPointUnderWaterSurf(cp.x, cp.y))
          continue;

        float cph = water_lev - 0.3;
        if (get_height_midpoint_heightmap(*HmapLandPlugin::self, cp, cph, NULL))
          cph = (cph > water_lev - 0.3) ? water_lev - 0.3 : (cph + water_lev - 0.3) * 0.5;

        // debug("tri %d: " FMT_P3 " (cp=" FMT_P2 ", h=%.3f)", i, h0, h1, h2, P2D(cp), cph);

        bool found_close = false;
        for (int j = 0; j < add_bottom_pts.size(); j++)
          if (lengthSq(Point2::xz(add_bottom_pts[j]) - cp) < close_bottom_pt_dist2)
          {
            found_close = true;
            break;
          }
        if (!found_close)
          add_bottom_pts.push_back().set(cp.x, cph, cp.y);
        need_re_delaunay = true;
      }
      if (need_re_delaunay && prev_bottom_pts == add_bottom_pts.size())
      {
        need_re_delaunay = false;
        DAEDITOR3.conWarning("Triangulation gives inconsistent heights, but iteration is useless");
      }

      if (need_re_delaunay)
      {
        re_delaunay_iter++;
        debug("reapply delaunay %d, bottom_pts=%d", re_delaunay_iter, add_bottom_pts.size());
        close_bottom_pt_dist2 /= 4;
        continue;
      }

      mesh_out.vert.resize(tin.verts.size());
      mesh_out.face.reserve(tin.triangles.size() / 3);
      for (int i = 0; i < mesh_out.vert.size(); i++)
        mesh_out.vert[i].set_xzy(tin.verts[i]);

      int bottom_faces = 0, deep_bottom_faces = 0, border_faces = 0;
      for (int i = 0; i < tin.triangles.size(); i += 3)
      {
        Face &f = mesh_out.face.push_back();
        f.v[0] = tin.triangles[i + 0];
        f.v[2] = tin.triangles[i + 1];
        f.v[1] = tin.triangles[i + 2];
        f.mat = BOTTOM_ABOVE;
        f.smgr = 1;
        float upper_lev = water_lev + ABOVE_LOD_HEIGHT;
        if (tin.verts[f.v[0]].z <= upper_lev || tin.verts[f.v[1]].z <= upper_lev || tin.verts[f.v[2]].z <= upper_lev)
          if (tin.verts[f.v[0]].z <= water_lev && tin.verts[f.v[1]].z <= water_lev && tin.verts[f.v[2]].z <= water_lev)
            bottom_faces++;
      }
      debug("deduced water_lev=%.3f, bottom faces: %d(deep=%d)/%d, border=%d", water_lev, bottom_faces, deep_bottom_faces,
        border_faces, mesh_out.face.size());

      DAEDITOR3.conNote("re-applied delaunay for %d points for %.1f sec (%d extra verts, %d extra faces, %d re-iter)",
        mesh_out.vert.size(), (dagTools->getTimeMsec() - time0l) / 1000.f, mesh_out.vert.size() - old_vc,
        mesh_out.face.size() - old_fc, re_delaunay_iter);
    }

    MemorySaveCB memSave(NULL, false);
    mesh_out.saveData(memSave);
    ::saveCache(fnameData, memSave.getMem(), currentHash);
    debug_flush(false);
    return false;
  }
  else
  {
    con.addMessage(ILogWriter::REMARK, "triangulation. getting from cache...");

    MemoryLoadCB memLoad(cachedData, true);
    mesh_out.loadData(memLoad);
    return true;
  }
}

static void split(Mesh &from, Mesh &pos, float A, float B, float C, float D, float eps)
{
  Mesh neg;
  from.split(neg, pos, A, B, C, D, eps);
  from = neg; //==can be optimized
  /*from.optimize_tverts();
  pos.optimize_tverts();*/
}

class OptimizeJob : public cpujobs::IJob
{
  landmesh::Cell &cell;

public:
  OptimizeJob(landmesh::Cell &c) : cell(c) {}
  static BBox3 calc_mesh_bbox(const Mesh &m)
  {
    BBox3 box;
    for (int fi = 0; fi < m.getFace().size(); ++fi)
    {
      box += m.getVert()[m.getFace()[fi].v[0]];
      box += m.getVert()[m.getFace()[fi].v[1]];
      box += m.getVert()[m.getFace()[fi].v[2]];
    }
    return box;
  }
  virtual void doJob()
  {
    // fixme: check if land mesh can has unused vertex
    cell.land_mesh->kill_unused_verts(0.0005f); // only saves few vertices
    cell.land_mesh->kill_bad_faces();

    cell.box = calc_mesh_bbox(*cell.land_mesh);

    if (cell.combined_mesh)
    {
      cell.combined_mesh->kill_unused_verts(0.0005f);
      cell.combined_mesh->kill_bad_faces();
      cell.box += calc_mesh_bbox(*cell.combined_mesh);
    }
    if (cell.patches_mesh)
    {
      cell.patches_mesh->kill_unused_verts(0.0005f);
      cell.patches_mesh->kill_bad_faces();
      cell.box += calc_mesh_bbox(*cell.patches_mesh);
    }
  }
  virtual void releaseJob() { delete this; }
};

class Point2Deref : public Point2
{
public:
  Point3 getPt() const { return Point3::x0y(*this); }
};

#if 1
static void write_tif(objgenerator::HugeBitmask &m, const char *fname)
{
  IBitMaskImageMgr::BitmapMask img;
  int w = m.getW(), h = m.getH();
  HmapLandPlugin::bitMaskImgMgr->createBitMask(img, w, h, 1);

  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      img.setMaskPixel1(x, y, m.get(x, y) ? 128 : 0);

  HmapLandPlugin::bitMaskImgMgr->saveImage(img, ".", fname);
  HmapLandPlugin::bitMaskImgMgr->destroyImage(img);
}
#endif

struct LandMeshCombinedMat
{
  int id;
  bool clipmapOnly;
  bool has_vertex_opacity;
};

static EditorLandRayTracer *generate_editor_land_tracer(Mesh &mesh, Point3 offset)
{
  int t0 = dagTools->getTimeMsec();
  EditorLandRayTracer *lrt = new EditorLandRayTracer;
  int minGrid = 511, maxGrid = 511;
  BBox3 landBox;
  for (int i = 0; i < mesh.vert.size(); i++)
    landBox += mesh.vert[i];
  Mesh *pMesh = &mesh;
  bool built = lrt->build(1, 1, max(landBox.width().x, landBox.width().z), offset, landBox, make_span(&pMesh, 1),
    make_span((Mesh **)NULL, 0), minGrid, maxGrid, false);
  if (!built)
  {
    DAEDITOR3.conError("Can't build editor landtracer.");
    delete lrt;
    return NULL;
  }
  DAEDITOR3.conRemark("build editor landtracer for landmesh for %dms", dagTools->getTimeMsec() - t0);
  return lrt;
}

bool HmapLandPlugin::generateLandMeshMap(LandMeshMap &map, CoolConsole &con, bool import_sgeom, LandRayTracer **game_tracer,
  bool strip_det_hmap_from_tracer)
{
  // landMeshMap.clear();
  if (!heightMap.isFileOpened())
    return true;
  bool generate_ok = true;

  guard_det_border = false;
  con.startLog();
  int time0 = dagTools->getTimeMsec();

  con.setActionDesc("delaunay triangulate...");

  DelaunayHeightMap hmap(heightMap);


  DelaunayImportanceMask *mask = NULL;
  IBitMaskImageMgr::BitmapMask impLayerMask;
  objgenerator::HugeBitmask *exclMask = NULL;

  int time0l = dagTools->getTimeMsec();
  Tab<Point3> loft_pt_cloud(tmpmem);
  Tab<Point3> water_border_polys(tmpmem);
  Tab<Point2> hmap_sweep_polys(tmpmem);
  Tab<Point3> det_hmap_border(tmpmem);

  loft_pt_cloud.reserve(4096);

  IDagorEdCustomCollider *coll = this;
  int prev_exp_type = exportType;

  DAGORED2->setColliders(make_span(&coll, 1), 0x7FFFFFFF);
  exportType = EXPORT_HMAP;

  rebuildWaterSurface(&loft_pt_cloud, &water_border_polys, &hmap_sweep_polys);
  if (!detDivisor)
    strip_det_hmap_from_tracer = false;
  if (strip_det_hmap_from_tracer)
  {
    int time0l = dagTools->getTimeMsec();

    con.setActionDesc("build delaunay constraints around detailed HMAP area...");
    int step = 32;
    int w = (detRectC[1].x - detRectC[0].x) / step, h = (detRectC[1].y - detRectC[0].y) / step;
    // debug("detRectC=%@ div=%d  step=%d", detRectC, detDivisor, step);
    float csz = gridCellSize / detDivisor;

    int base = append_items(water_border_polys, w * 2 + h * 2 + 2);
    mem_set_0(make_span(water_border_polys).subspan(base));
    // debug("%dx%d  base=%d sz=%d", w, h, base, water_border_polys.size()-base);
    water_border_polys[base].set(1.1e12f, w * 2 + h * 2 + 1, 0.f);
    for (int x = detRectC[0].x, i = 0; x <= detRectC[1].x; x += step, i++)
    {
      water_border_polys[base + 1 + i].set_xVy(heightMapOffset + Point2(x, detRectC[0].y) * csz,
        heightMap.getFinalData(x / detDivisor, detRectC[0].y / detDivisor));
      water_border_polys[base + 1 + 2 * w + h - i].set_xVy(heightMapOffset + Point2(x, detRectC[1].y) * csz,
        heightMap.getFinalData(x / detDivisor, detRectC[1].y / detDivisor));
    }
    for (int y = detRectC[0].y + step, i = 0; y < detRectC[1].y; y += step, i++)
    {
      water_border_polys[base + 1 + 2 * w + 2 * h - 1 - i].set_xVy(heightMapOffset + Point2(detRectC[0].x, y) * csz,
        heightMap.getFinalData(detRectC[0].x / detDivisor, y / detDivisor));
      water_border_polys[base + 1 + w + 1 + i].set_xVy(heightMapOffset + Point2(detRectC[1].x, y) * csz,
        heightMap.getFinalData(detRectC[1].x / detDivisor, y / detDivisor));
    }
    water_border_polys[base + w * 2 + h * 2 + 1] = water_border_polys[base + 1];
    det_hmap_border = make_span_const(water_border_polys).subspan(base + 1);

    // for (int i = base; i < water_border_polys.size(); i ++)
    //   debug("border[%d] %@", i, water_border_polys[i]);

    con.setActionDesc("backsync border of detailed HMAP with landmesh...");
    float ht, t;

    for (int x = detRectC[0].x, i = 0; x < detRectC[1].x; x++, i++)
    {
      t = x + 1 < detRectC[1].x ? float(i % step) / step : 1.0f;
      ht = lerp(water_border_polys[base + 1 + i / step].y, water_border_polys[base + 1 + i / step + 1].y, t);
      // heightMapDet.setInitialData(x,detRectC[0].y, ht);
      heightMapDet.setFinalData(x, detRectC[0].y, ht);
      if ((x % detDivisor) == 0)
      {
        // heightMap.setInitialData(x/detDivisor, detRectC[0].y/detDivisor, ht);
        heightMap.setFinalData(x / detDivisor, detRectC[0].y / detDivisor, ht);
      }

      ht = lerp(water_border_polys[base + 1 + 2 * w + h - i / step].y, water_border_polys[base + 1 + 2 * w + h - i / step - 1].y, t);
      // heightMapDet.setInitialData(x,detRectC[1].y-1, ht);
      heightMapDet.setFinalData(x, detRectC[1].y - 1, ht);
      if ((x % detDivisor) == 0)
      {
        // heightMap.setInitialData(x/detDivisor, detRectC[1].y/detDivisor, ht);
        heightMap.setFinalData(x / detDivisor, detRectC[1].y / detDivisor, ht);
      }
    }
    for (int y = detRectC[0].y, i = 0; y < detRectC[1].y; y++, i++)
    {
      t = y + 1 < detRectC[1].y ? float(i % step) / step : 1.0f;
      ht = lerp(water_border_polys[base + 1 + 2 * w + 2 * h - i / step].y,
        water_border_polys[base + 1 + 2 * w + 2 * h - i / step - 1].y, t);
      // heightMapDet.setInitialData(detRectC[0].x, y, ht);
      heightMapDet.setFinalData(detRectC[0].x, y, ht);
      if ((y % detDivisor) == 0)
      {
        // heightMap.setInitialData(detRectC[0].x/detDivisor, y/detDivisor, ht);
        heightMap.setFinalData(detRectC[0].x / detDivisor, y / detDivisor, ht);
      }

      ht = lerp(water_border_polys[base + 1 + w + i / step].y, water_border_polys[base + 1 + w + i / step + 1].y, t);
      // heightMapDet.setInitialData(detRectC[1].x-1, y, ht);
      heightMapDet.setFinalData(detRectC[1].x - 1, y, ht);
      if ((y % detDivisor) == 0)
      {
        // heightMap.setInitialData(detRectC[1].x/detDivisor, y/detDivisor, ht);
        heightMap.setFinalData(detRectC[1].x / detDivisor, y / detDivisor, ht);
      }
    }
    DAEDITOR3.conNote("updated heightMap/heightMapDet finalData for %.2f sec", (dagTools->getTimeMsec() - time0l) / 1000.f);
    updateHeightMapTex(false);
    updateHeightMapTex(true);
  }

  exportType = prev_exp_type;
  DAGORED2->restoreEditorColliders();

  if (loft_pt_cloud.size())
    DAEDITOR3.conNote("gathered %d points from lofts for %.1f sec", loft_pt_cloud.size(), (dagTools->getTimeMsec() - time0l) / 1000.f);

  if (hmap_sweep_polys.size() || water_border_polys.size() || loft_pt_cloud.size())
    exclMask = new objgenerator::HugeBitmask(hmap.width(), hmap.height());
  if (hmap_sweep_polys.size() && exclMask)
  {
    Tab<Point2Deref *> pts(tmpmem);

    for (int i = 0; i < hmap_sweep_polys.size(); i++)
      if (hmap_sweep_polys[i].x > 1e12f)
      {
        pts.resize(hmap_sweep_polys[i].y);
        debug("add new sweep %d pts", pts.size());
        i++;
        for (int j = 0; j < pts.size(); j++, i++)
          pts[j] = static_cast<Point2Deref *>(&hmap_sweep_polys[i]);

        rasterize_poly_2_nz(*exclMask, pts, heightMapOffset.x, heightMapOffset.y, 1.0f / gridCellSize);
        i--;
      }
  }
  for (int i = 0; i < loft_pt_cloud.size() && exclMask; i++)
  {
    int cx = floorf((loft_pt_cloud[i].x - heightMapOffset.x) / gridCellSize + 0.5f);
    int cy = floorf((loft_pt_cloud[i].z - heightMapOffset.y) / gridCellSize + 0.5f);
    if (fabs(cx * gridCellSize + heightMapOffset.x - loft_pt_cloud[i].x) +
          fabs(cy * gridCellSize + heightMapOffset.y - loft_pt_cloud[i].z) >
        0.2)
      continue;
    if (cx >= 0 && cy >= 0 && cx < hmap.width() && cy < hmap.height())
      exclMask->set(cx, cy);
  }

  for (int i = 0; i < water_border_polys.size() && exclMask; i++)
    if (water_border_polys[i].x > 1e12f)
    {
      i++;
      int s_i = i, e_i = s_i + water_border_polys[i - 1].y - 1;
      float eps = max(gridCellSize / 8.0f, 0.6f);
      for (; i <= e_i; i++)
      {
        int cx = floorf((water_border_polys[i].x - heightMapOffset.x) / gridCellSize + 0.5f);
        int cy = floorf((water_border_polys[i].z - heightMapOffset.y) / gridCellSize + 0.5f);
        if (cx >= 0 && cy >= 0 && cx < hmap.width() && cy < hmap.height() &&
            fabs(cx * gridCellSize + heightMapOffset.x - water_border_polys[i].x) +
                fabs(cy * gridCellSize + heightMapOffset.y - water_border_polys[i].z) <
              eps)
          exclMask->set(cx, cy);

        if (i > s_i)
        {
          Point2 p0(water_border_polys[i].x, water_border_polys[i].z), pt;
          Point2 dir(water_border_polys[i - 1].x - p0.x, water_border_polys[i - 1].z - p0.y);
          float len = dir.length();
          for (float t = 0.1, len = dir.length(); t < len; t += 0.1) //-V1034
          {
            pt.set(p0.x + dir.x * t / len, p0.y + dir.y * t / len);
            cx = floorf((pt.x - heightMapOffset.x) / gridCellSize + 0.5f);
            cy = floorf((pt.y - heightMapOffset.y) / gridCellSize + 0.5f);
            if (cx >= 0 && cy >= 0 && cx < hmap.width() && cy < hmap.height() &&
                fabs(cx * gridCellSize + heightMapOffset.x - pt.x) + fabs(cy * gridCellSize + heightMapOffset.y - pt.y) < eps)
              exclMask->set(cx, cy);
          }
        }
      }

      if (water_border_polys[s_i - 1].z < 0.5f)
      {
        Point2 p0(water_border_polys[s_i].x, water_border_polys[s_i].z), pt;
        Point2 dir(water_border_polys[e_i].x - p0.x, water_border_polys[e_i].z - p0.y);
        float len = dir.length();
        for (float t = 0.1, len = dir.length(); t < len; t += 0.1) //-V1034
        {
          pt.set(p0.x + dir.x * t / len, p0.y + dir.y * t / len);
          int cx = floorf((pt.x - heightMapOffset.x) / gridCellSize + 0.5f);
          int cy = floorf((pt.y - heightMapOffset.y) / gridCellSize + 0.5f);
          if (cx >= 0 && cy >= 0 && cx < hmap.width() && cy < hmap.height() &&
              fabs(cx * gridCellSize + heightMapOffset.x - pt.x) + fabs(cy * gridCellSize + heightMapOffset.y - pt.y) < eps)
            exclMask->set(cx, cy);
        }
      }
      i--;
    }
  if (allow_debug_bitmap_dump && exclMask)
    write_tif(*exclMask, "../../exclMask.tif");


  if (impLayerIdx >= 0)
  {
    bitMaskImgMgr->createBitMask(impLayerMask, landClsMap.getMapSizeX(), landClsMap.getMapSizeY(), 8);
    for (int y = 0, d = impLayerMask.getHeight() - 1; y < impLayerMask.getHeight(); y++, d--)
      for (int x = 0; x < impLayerMask.getWidth(); x++)
        impLayerMask.setMaskPixel8(x, d, getImpLayerDesc().getLayerData(landClsMap.getData(x, y)));
    bitMaskImgMgr->saveImage(impLayerMask, getInternalName(), "importanceMask_lc");

    mask = new DelaunayImportanceMask(&impLayerMask, exclMask, importanceMaskScale, hmap.width(), hmap.height());
  }
  else
    for (unsigned int imageNo = 0; imageNo < scriptImages.size(); imageNo++)
    {
      if (stricmp(scriptImages[imageNo]->getName(), "importanceMask") == 0)
      {
        mask = new DelaunayImportanceMask(scriptImages[imageNo], exclMask, importanceMaskScale, hmap.width(), hmap.height());

        break;
      }
    }

  Point3 ofs(heightMapOffset[0], 0, heightMapOffset[1]);
  Mesh mesh;
  bool loadedFromCache = ::delaunayGenerateCached(map, heightMap, meshErrorThreshold, numMeshVertices, &hmap, mesh, gridCellSize, ofs,
    mask, loft_pt_cloud, water_border_polys);

  delete mask;
  del_it(exclMask);
  if (impLayerIdx >= 0)
    bitMaskImgMgr->destroyImage(impLayerMask);

  con.addMessage(ILogWriter::REMARK, "Triangulated in %g seconds", (dagTools->getTimeMsec() - time0) / 1000.0f);
  debug("Triangulated in %g seconds (%d faces, %d verts)", (dagTools->getTimeMsec() - time0) / 1000.0f, mesh.face.size(),
    mesh.vert.size());
  if (!mesh.face.size())
  {
    DAEDITOR3.conError("Failed to triangulate land; try changing settings (HMAP height, ocean, etc.)");
    landMeshMap.resize(0, 0, 0, 0, 0);
    generate_ok = false;
  }

  if (!loadedFromCache || !landMeshMap.getEditorLandRayTracer())
  {
    EditorLandRayTracer *lrt = generate_ok ? ::generate_editor_land_tracer(mesh, ofs) : NULL;
    landMeshMap.setEditorLandRayTracer(lrt);
  }
  if (generate_ok && applyHtConstraintBitmask(mesh))
  {
    EditorLandRayTracer *lrt = ::generate_editor_land_tracer(mesh, ofs);
    landMeshMap.setEditorLandRayTracer(lrt);
  }

  if (strip_det_hmap_from_tracer && generate_ok)
  {
    int time0l = dagTools->getTimeMsec();
    float csz = gridCellSize / detDivisor;

    con.setActionDesc("smoothing borders (%.0f m) of detailed HMAP with landmesh %@...", gridCellSize * 2, detRectC);

    BuildableStaticSceneRayTracer lrt(Point3(256, 128, 256), 4);
    lrt.setCullFlags(StaticSceneRayTracer::CULL_BOTH);
    G_VERIFY(lrt.addmesh(mesh.vert.data(), mesh.vert.size(), (unsigned *)mesh.face.data(), elem_size(mesh.face), mesh.face.size(),
      NULL, true));
    for (int y = detRectC[0].y; y < detRectC[1].y; y++)
      for (int x = detRectC[0].x; x < detRectC[1].x; x++)
      {
        int min_border_dist = min<int>(y - detRectC[0].y, detRectC[1].y - y - 1);
        min_border_dist = min<int>(min_border_dist, x - detRectC[0].x);
        min_border_dist = min<int>(min_border_dist, detRectC[1].x - x - 1);
        if (min_border_dist > detDivisor * 2)
          continue;

        float ht0 = heightMapDet.getInitialData(x, y), ht = ht0 - 200;
        Point3 pos = ofs + Point3::xVy(Point2(x, y) * csz, ht0 + 200);
        if (x + 1 == detRectC[1].x)
          pos.x += csz;
        if (y + 1 == detRectC[1].y)
          pos.z += csz;

        if (lrt.getHeightBelow(pos, ht) < 0)
          ht = ht0;
        ht = lerp(ht, ht0, max(min_border_dist - 1, 0) * 0.5f / detDivisor);
        // heightMapDet.setInitialData(x, y, ht);
        heightMapDet.setFinalData(x, y, ht);
      }
    DAEDITOR3.conNote("updated heightMap/heightMapDet finalData for %.2f sec", (dagTools->getTimeMsec() - time0l) / 1000.f);
    updateHeightMapTex(false);
    updateHeightMapTex(true);
  }

  float epsilon = 0.00001f;
  con.setActionDesc("Setting normals...");

  BBox3 mapBox;
  int totalMeshFaces = 0, totalMeshVerts = 0;

  for (int vi = 0; vi < mesh.vert.size(); ++vi)
    mapBox += mesh.vert[vi] - ofs;

  if (hasWaterSurface)
  {
    if (mapBox[1].y < waterSurfaceLevel + 0.01 - ofs.y)
      DAEDITOR3.conError("upper land mesh point is %.3f, maybe underwater", mapBox[1].y + ofs.y);
    else if (mapBox[1].y < waterSurfaceLevel + 1 - ofs.y)
      DAEDITOR3.conWarning("upper land mesh point is %.3f, nearly underwater", mapBox[1].y + ofs.y);
  }
  totalMeshFaces += mesh.face.size();
  totalMeshVerts += mesh.vert.size();
  //
  mesh.calc_ngr();
  mesh.calc_vertnorms();
  // for (int i = 0; i < mesh.vertnorm.size(); ++i)
  //   mesh.vertnorm[i] = -mesh.vertnorm[i];
  mesh.facengr.resize(mesh.face.size());
  mesh.vertnorm.resize(mesh.vert.size());
  for (int i = 0; i < mesh.vert.size(); ++i)
  {
    real h, hl, hr, hu, hd, hr2, hu2;
    int x = (mesh.vert[i].x - ofs.x) / gridCellSize;
    int y = (mesh.vert[i].z - ofs.z) / gridCellSize;
    h = heightMap.getFinalData(x, y);
    hl = heightMap.getFinalData(x - 1, y);
    hr = heightMap.getFinalData(x + 1, y);
    hu = heightMap.getFinalData(x, y + 1);
    hd = heightMap.getFinalData(x, y - 1);
    mesh.vertnorm[i] = -normalize(Point3(hr - hl, -2 * gridCellSize, hu - hd));
  }

  for (int i = 0; i < mesh.face.size(); ++i)
  {
    mesh.facengr[i][0] = mesh.face[i].v[0];
    mesh.facengr[i][1] = mesh.face[i].v[1];
    mesh.facengr[i][2] = mesh.face[i].v[2];
  }

  float meshCellSize = getMeshCellSize();

  int mapx0 = int(floorf(mapBox[0].x / meshCellSize));
  int mapy0 = int(floorf(mapBox[0].z / meshCellSize));
  int mapx1 = int(ceilf(mapBox[1].x / meshCellSize));
  int mapy1 = int(ceilf(mapBox[1].z / meshCellSize));

  LandMeshMap &cells = landMeshMap;
  cells.resize(mapx0, mapy0, mapx1, mapy1, meshCellSize);

  Tab<Mesh *> meshes(tmpmem);
  Bitarray should_collide(tmpmem);
  Tab<StaticGeometryNode *> nodes(tmpmem);
  Tab<SmallTab<LandMeshCombinedMat, TmpmemAlloc>> matUsed(tmpmem);
  Tab<bool> shouldRemoveUndegroundFaces(tmpmem);
  Tab<bool> isDecal(tmpmem);
  Tab<bool> isPatch(tmpmem);
  int combined_meshes = 0, decal_meshes = 0;

  StaticGeometryContainer *geoCont = dagGeom->newStaticGeometryContainer();
  int reported_has_vertex_opacity = 0;

  if (import_sgeom)
  {
    con.setActionDesc("gather static visual...");
    con.startProgress();

    DataBlock applicationBlk(DAGORED2->getWorkspace().getAppPath());
    bool joinSplineMeshes = applicationBlk.getBlockByNameEx("heightMap")->getBool("joinSplineMeshes", true);

    con.setTotal(DAGORED2->getPluginCount());
    // DEBUG_DUMP_VAR(geomLoftBelowAll);
    if (geomLoftBelowAll)
    {
      int oldCount = should_collide.size();
      // debug("%s.gatherStaticVisualGeometry", self->getInternalName());
      self->gatherStaticVisualGeometry(*geoCont);
      should_collide.resize(geoCont->nodes.size() + 1);
      should_collide.set(oldCount, should_collide.size() - oldCount, 0);
    }
    for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
    {
      con.setDone(i);
      IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);

      IGatherStaticGeometry *geom = plugin->queryInterface<IGatherStaticGeometry>();
      if (!geom)
        continue;
      if (geomLoftBelowAll && plugin == self)
        continue;
      int oldCount = should_collide.size();

      // debug("%s.gatherStaticVisualGeometry", plugin->getInternalName());
      geom->gatherStaticVisualGeometry(*geoCont);
      should_collide.resize(geoCont->nodes.size() + 1);

      unsigned v = 0;
      if (stricmp(plugin->getInternalName(), "_prefabEntMgr") == 0)
      {
        con.addMessage(ILogWriter::REMARK, "adding only %s to collision", (const char *)plugin->getInternalName());
        v = 1;
      }
      should_collide.set(oldCount, should_collide.size() - oldCount, v);
    }
    con.endProgress();
    meshes.reserve(geoCont->nodes.size() + 1);
    nodes.reserve(geoCont->nodes.size() + 1);
    matUsed.reserve(geoCont->nodes.size() + 1);

    con.setActionDesc("finding valid static geom...");
    con.startProgress();

    con.setTotal(geoCont->nodes.size() / 32);

    StaticGeometryMaterial *prevSplineMat = NULL;
    int splineBatch = 0;

    for (int i = 0; i < geoCont->nodes.size(); ++i)
    {
      if (!(i & 31))
        con.setDone(i / 32);

      StaticGeometryNode *node = geoCont->nodes[i];

      if (!node)
      {
        erase_items(geoCont->nodes, i, 1);
        --i;
        continue;
      }
      if (!node->mesh->mesh.check_mesh())
      {
        con.addMessage(ILogWriter::WARNING, "node mesh <%s> is invalid.", (const char *)node->name);
        erase_items(geoCont->nodes, i, 1);
        --i;
        continue;
      }
      SmallTab<LandMeshCombinedMat, TmpmemAlloc> matsUsedNode;
      clear_and_resize(matsUsedNode, node->mesh->mats.size());
      int mats_used = 0, vertex_opacity_used = 0;
      bool hasClipmapOnly = false;
      bool hasCombined = false;

      int nodeLayer = -1;
      int splineLayer = node->script.getInt("splineLayer", -1);
      if (splineLayer != -1) // Each spline has splineLayer >= 0.
      {
        if (joinSplineMeshes)
        {
          if (node->mesh->mats.size() && node->mesh->mats[0] && (!prevSplineMat || !node->mesh->mats[0]->equal(*prevSplineMat)))
          {
            prevSplineMat = node->mesh->mats[0];
            splineBatch++;
          }
          int loftLayer = node->script.getInt("layer", 0);
          nodeLayer = loftLayer * LAYER_ORDER_MAX + splineLayer          // Split meshes by layers.
                      + splineBatch * LAYER_ORDER_MAX * LAYER_ORDER_MAX; // Do not join meshes if another mesh is between them to avoid
                                                                         // changing the render order.
        }
        else
          nodeLayer = i;
      }

      bool isHeightPatch = false;
      bool hasPatchNodesOnly = true;
      for (int mi = 0; mi < node->mesh->mats.size(); ++mi)
      {
        bool isPatchNode = false;
        matsUsedNode[mi].id = landMeshMap.addMaterial(node->mesh->mats[mi], &matsUsedNode[mi].clipmapOnly,
          &matsUsedNode[mi].has_vertex_opacity, nodeLayer, &isPatchNode);

#if 0
        debug("node=%d, name='%s', splineLayer=%d, splineBatch=%d, nodeLayer=0x%08X, mi=%d, matId=%d",
          i, node->name.str(), splineLayer, splineBatch, nodeLayer, mi, matsUsedNode[mi].id);
#endif

        if (matsUsedNode[mi].id >= 0)
        {
          mats_used++;
          if (matsUsedNode[mi].has_vertex_opacity)
            vertex_opacity_used++;
        }
        hasClipmapOnly |= matsUsedNode[mi].clipmapOnly;
        hasCombined |= !matsUsedNode[mi].clipmapOnly;
        isHeightPatch |= isPatchNode;
        if (!isPatchNode)
          hasPatchNodesOnly = false;
      }

      if (isHeightPatch && !hasPatchNodesOnly)
        con.addMessage(ILogWriter::ERROR, "node mesh <%s> has both height patches and other nodes!", (const char *)node->name);
      if (hasClipmapOnly && hasCombined)
        con.addMessage(ILogWriter::ERROR, "node mesh <%s> has both decals and landmesh_combined nodes. Currently not supported!",
          (const char *)node->name);
      if (vertex_opacity_used != 0 && vertex_opacity_used != mats_used)
      {
        if (reported_has_vertex_opacity < 16)
          con.addMessage(ILogWriter::NOTE, "node mesh <%s> has partial vertex opacity!", node->name);
        reported_has_vertex_opacity++;
      }

      if (!mats_used)
      {
        erase_items(geoCont->nodes, i, 1);
        --i;
        continue;
      }

      nodes.push_back(node);

      meshes.push_back(&node->mesh->mesh);
      matUsed.push_back(matsUsedNode);
      shouldRemoveUndegroundFaces.push_back(!hasClipmapOnly);
      isDecal.push_back(hasClipmapOnly);
      isPatch.push_back(isHeightPatch);
      if (hasClipmapOnly)
        decal_meshes++;
      else
        combined_meshes++;
    }
    con.endProgress();
  }
  if (reported_has_vertex_opacity)
    con.addMessage(ILogWriter::NOTE, "encountered %d errors with vertex opacity!", reported_has_vertex_opacity);
  meshes.push_back(&mesh);
  nodes.push_back(NULL);
  con.addMessage(ILogWriter::NOTE, "%d decal meshes, %d combined meshes found", decal_meshes, combined_meshes);
  if (import_sgeom)
    con.addMessage(ILogWriter::NOTE, "Meshes joining: %d -> %d", geoCont->nodes.size(), landMeshMap.getMaterialsCount());

  // for (int i = 0; i < mesh.face.size(); ++i)
  //   mesh.face[i].mat = 0;

  int totalAllFaces = 0;
  int totalFaces = 0, totalVerts = 0;
  for (int mi = 0; mi < meshes.size(); ++mi)
  {
    if (!meshes[mi])
      continue;
    totalAllFaces += meshes[mi]->face.size();
    // if (!meshes[mi] ||
    //   (nodes[mi] && !(nodes[mi]->flags&StaticGeometryNode::FLG_COLLIDABLE && should_collide[mi])))
    //   continue;
    totalVerts += meshes[mi]->vert.size();
    totalFaces += meshes[mi]->face.size();
  }

  int time1 = dagTools->getTimeMsec();
  con.setActionDesc("cutting meshes...");
  con.startProgress();
  con.setTotal(totalAllFaces);
  Tab<int> vert2Cell(tmpmem);
  Mesh tmpmesh;
  for (int mi = 0, cfaces = 0; mi < meshes.size(); ++mi)
  {
    // Node &node=*meshes[mi];
    Mesh *usemesh = meshes[mi];
    if (nodes[mi])
    {
      tmpmesh = nodes[mi]->mesh->mesh;
      usemesh = &tmpmesh;
      if (tmpmesh.vertnorm.empty())
      {
        // Generate normals for cliffs, castles and road ends.
        tmpmesh.calc_ngr();
        tmpmesh.calc_vertnorms();
      }
      /*else
      {
        // Invert road normals.
        //fixme: should not be needed if uses mesh.transform()
        for (unsigned int normNo = 0; normNo < usemesh->vertnorm.size(); normNo++)
          usemesh->vertnorm[normNo] = -usemesh->vertnorm[normNo];
      }*/
      Mesh &mesh = tmpmesh;
      mesh.transform(nodes[mi]->wtm);
      bool has_vertex_opacity = false;
      if (usemesh->tface[1].size())
        for (int mat = 0; mat < matUsed[mi].size(); ++mat)
          if (matUsed[mi][mat].has_vertex_opacity)
          {
            has_vertex_opacity = true;
            break;
          }
      int opaque_tvert = -1;
      for (int vi = 0; vi < mesh.face.size(); ++vi)
      {
        int mat = mesh.face[vi].mat;
        if (mat >= matUsed[mi].size())
          mat = matUsed[mi].size() - 1;
        mesh.face[vi].mat = matUsed[mi][mat].id;
        if (has_vertex_opacity && !matUsed[mi][mat].has_vertex_opacity)
        {
          if (opaque_tvert < 0)
          {
            opaque_tvert = usemesh->tvert[1].size();
            usemesh->tvert[1].push_back(Point2(1, 1));
          }
          mesh.tface[1][vi].t[0] = mesh.tface[1][vi].t[1] = mesh.tface[1][vi].t[2] = opaque_tvert;
        }
      }
      if (!has_vertex_opacity)
      {
        mesh.tface[1].resize(mesh.face.size());
        mem_set_0(mesh.tface[1]);
        mesh.tvert[1].push_back(Point2(1, 1));
      }
      if (shouldRemoveUndegroundFaces[mi])
      {
        Bitarray facesAbove;
        if (markUndergroundFaces(tmpmesh, facesAbove, NULL))
          tmpmesh.removeFacesFast(facesAbove);
      }
    }

    Mesh &mesh = *usemesh;

    int numRemoved = 0;
    int origFaceCount = mesh.face.size();
    BBox3 meshBox;
    for (int vi = 0; vi < mesh.vert.size(); ++vi)
      meshBox += mesh.vert[vi];
    if (nodes[mi])
    {
      for (int fi = mesh.face.size() - 1; fi >= 0; --fi)
        if (mesh.face[fi].mat >= landMeshMap.getMaterialsCount())
        {
          int j = fi - 1;
          for (; j >= 0; --j)
            if (mesh.face[j].mat < landMeshMap.getMaterialsCount())
              break;
          j++;
          mesh.removeFacesFast(j, fi - j + 1);
          fi = j;
        }
    }
    if (!mesh.face.size())
    {
      cfaces += origFaceCount;
      con.setDone(cfaces);
      continue;
    }

    // can be optimized by using binary(quad) tree of splits.
    Mesh *prevY = new Mesh, *nextY = NULL;
    split(mesh, *prevY, 0, 0, 1.f, -ofs.z, epsilon);

    if (!prevY->face.size())
    {
      cfaces += origFaceCount;
      con.setDone(cfaces);
      continue;
    }

    for (int cellY = 0; cellY < cells.getNumCellsY(); ++cellY)
    {
      if (nextY)
      {
        del_it(prevY);
        prevY = nextY;
        nextY = NULL;
      }
      if (!prevY->face.size())
        break;
      nextY = new Mesh;
      split(*prevY, *nextY, 0.f, 0.f, 1.f,
        -((cellY + 1) * meshCellSize - ((cellY == cells.getNumCellsY() - 1) ? gridCellSize : 0) + ofs.z), epsilon);
      if (!prevY->face.size())
        continue;
      Mesh *prevX = new Mesh, *nextX = NULL;
      split(*prevY, *prevX, 1.f, 0.f, 0.f, -ofs.x, epsilon);
      if (!prevX->face.size())
      {
        del_it(prevX);
        continue;
      }
      prevX->optimize_tverts(-1.0);
      for (int cellX = 0; cellX < cells.getNumCellsX(); ++cellX)
      {
        if (nextX)
        {
          del_it(prevX);
          prevX = nextX;
          nextX = NULL;
        }
        BBox3 cellBox;
        cellBox[0] = ofs + Point3(cellX * cells.getCellSize(), MIN_REAL, cellY * cells.getCellSize());
        cellBox[1] = cellBox[0];
        cellBox[1].y = MAX_REAL;
        cellBox[1] += Point3(cells.getCellSize(), 0, cells.getCellSize());
        if (!(meshBox & cellBox))
          continue;

        Mesh *destmesh = NULL;
        if (!nodes[mi])
          destmesh = cells.getCellLandMesh(cellX, cellY, true);
        else if (isPatch[mi])
          destmesh = cells.getCellPatchesMesh(cellX, cellY, true);
        else
        {
          // fixme: we can support meshes with both decals and combineds!
          if (isDecal[mi])
            destmesh = cells.getCellDecalMesh(cellX, cellY, true);
          else
            destmesh = cells.getCellCombinedMesh(cellX, cellY, true);
        }

        if (!destmesh)
          continue;

        Mesh &cellm = *destmesh;
        if (!prevX->face.size())
          break;
        nextX = new Mesh;
        split(*prevX, *nextX, 1.f, 0.f, 0.f,
          -((cellX + 1) * meshCellSize - ((cellX == cells.getNumCellsX() - 1) ? gridCellSize : 0) + ofs.x), epsilon);
        if (!prevX->face.size())
          continue;
        if (nodes[mi])
          prevX->optimize_tverts(-1.0);
        cellm.attach(*prevX);
      }
      del_it(prevX);
      del_it(nextX);
    }
    del_it(prevY);
    del_it(nextY);
    cfaces += mesh.face.size();
    con.setDone(cfaces);
  }

  con.endProgress();
  con.addMessage(ILogWriter::REMARK, "Cutted in %g seconds", (dagTools->getTimeMsec() - time1) / 1000.0f);

  dagGeom->deleteStaticGeometryContainer(geoCont);

  int minFaces, maxFaces;
  cells.getStats(minFaces, maxFaces, totalFaces);

  con.addMessage(ILogWriter::NOTE, "%dx%d cells, %d min, %d max, %d total faces", cells.getNumCellsX(), cells.getNumCellsY(), minFaces,
    maxFaces, totalFaces);

  dag::Span<landmesh::Cell> cm = cells.getCells();

  con.setActionDesc("optimizing meshes...");
  int time2 = dagTools->getTimeMsec();
  // multithreading:
  const int max_cores = 128;
  bool started[max_cores];
  int coresId[max_cores];
  int cores_count = 0;
  for (int ci = 0; ci < min(max_cores, cpujobs::get_core_count()); ++ci)
    if (!cpujobs::start_job_manager(ci, 32768))
      started[ci] = false;
    else
    {
      started[ci] = true;
      coresId[cores_count] = ci;
      cores_count++;
    }
  int current_core = 0;
  if (generate_ok && cores_count)
  {
    for (int i = 0; i < cm.size(); ++i)
    {
      cpujobs::add_job(coresId[current_core], new OptimizeJob(cm[i]));
      current_core++;
      current_core %= cores_count;
    }
    int iterations = 1000;
    for (int ci = cores_count - 1; ci >= 0; --ci)
    {
      while (cpujobs::is_job_manager_busy(coresId[ci]))
      {
        ::sleep_msec(10);
        iterations--;
        if (!iterations)
        {
          cores_count = 0;
          break;
        }
      }
      cpujobs::stop_job_manager(coresId[ci], cores_count ? true : false);
    }
    if (!cores_count)
      con.addMessage(ILogWriter::WARNING, "infinite run");
  }
  if (generate_ok && !cores_count)
  {
    con.startProgress();
    con.setTotal(cm.size());
    for (int i = 0; i < cm.size(); ++i)
    {
      OptimizeJob job(cm[i]);
      job.doJob();
      con.setDone(i + 1);
    }
    con.endProgress();
  }
  con.addMessage(ILogWriter::REMARK, "Optimized in %g seconds", (dagTools->getTimeMsec() - time2) / 1000.0f);
  int totalOutSize = 0;
  landBox.setempty();
  SmallTab<Mesh *, TmpmemAlloc> cellLandMeshes;
  clear_and_resize(cellLandMeshes, cm.size());
  SmallTab<Mesh *, TmpmemAlloc> cellCombinedMeshes;
  clear_and_resize(cellCombinedMeshes, cm.size());
  for (int i = 0; i < cm.size(); ++i)
  {
    if (strip_det_hmap_from_tracer && cm[i].box[0].x + gridCellSize >= detRect[0].x && cm[i].box[1].x - gridCellSize <= detRect[1].x &&
        cm[i].box[0].z + gridCellSize >= detRect[0].y && cm[i].box[1].z - gridCellSize <= detRect[1].y)
    {
      debug("strip landMesh cell[%d] %@ (replaced by detaled HMAP)", i, cm[i].box);
      cellLandMeshes[i] = NULL;
      cellCombinedMeshes[i] = NULL;
      if (game_tracer && cm[i].combined_mesh && cm[i].combined_mesh->getFaceCount())
      {
        cellCombinedMeshes[i] = cm[i].combined_mesh;
        for (int j = 0; j < cellCombinedMeshes[i]->vert.size(); j++)
          landBox += cellCombinedMeshes[i]->vert[j];
      }
    }
    else
    {
      cellLandMeshes[i] = cm[i].land_mesh;
      cellCombinedMeshes[i] = cm[i].combined_mesh;
      landBox += cm[i].box;
    }
  }

  {
    con.setActionDesc("building land tracer...");
    LandRayTracer *lrt = new LandRayTracer;
    landMeshMap.setGameLandRayTracer(lrt);
    int minGrid = 8, maxGrid = 32;
    bool built = lrt->build(cells.getNumCellsX(), cells.getNumCellsY(), meshCellSize, ofs, landBox, cellLandMeshes, cellCombinedMeshes,
      minGrid, maxGrid, true);
    if (!built)
    {
      landMeshMap.setGameLandRayTracer(NULL);
      DAEDITOR3.conError("Can't build landtracer.");
    }
    if (game_tracer)
    {
      *game_tracer = lrt = new LandRayTracer;
      built = lrt->build(cells.getNumCellsX(), cells.getNumCellsY(), meshCellSize, ofs, landBox, cellLandMeshes, cellCombinedMeshes,
        minGrid, maxGrid, false);
      if (!built)
      {
        del_it(game_tracer[0]);
        DAEDITOR3.conError("Can't build game landtracer.");
        generate_ok = false;
      }
    }
  }
  if (strip_det_hmap_from_tracer)
  {
    mem_set_0(cellCombinedMeshes);

    int minGrid = 8, maxGrid = 32;
    EditorLandRayTracer *lrt = new EditorLandRayTracer;
    if (lrt->build(cells.getNumCellsX(), cells.getNumCellsY(), meshCellSize, ofs, landBox, cellLandMeshes, cellCombinedMeshes, minGrid,
          maxGrid, true))
      landMeshMap.setEditorLandRayTracer(lrt);
    else
    {
      delete lrt;
      DAEDITOR3.conError("Can't build landtracer for editor (with stripped DET HMAP), resetting main landtracer.");
      landMeshMap.setEditorLandRayTracer(nullptr);
    }
  }

  landBox.setempty();
  for (int cellY = 0; cellY < cells.getNumCellsY(); ++cellY)
    for (int cellX = 0; cellX < cells.getNumCellsX(); ++cellX)
      landBox += cells.getBBox(cellX, cellY);

  float y0 = 0.5 * (landBox[1].y + landBox[0].y);
  float denominator = landBox[1].y - landBox[0].y;
  float inv_scale = denominator > REAL_EPS ? 2.f / denominator : 0.f;
  Point3 cellScale(2.f / cells.getCellSize(), inv_scale, 2.f / cells.getCellSize());
  Point3 invCellScale(1.0f / cellScale.x, 1.0f / cellScale.y, 1.0f / cellScale.z);
  debug("compress landMesh ht=(%.3f, %.3f), eps=%.4f ofs=%@", landBox[0].y, landBox[1].y, denominator / 65536, ofs);
  for (int cellY = 0; cellY < cells.getNumCellsY(); ++cellY)
    for (int cellX = 0; cellX < cells.getNumCellsX(); ++cellX)
    {
      Point3 cellOfs((cellX + 0.5) * cells.getCellSize(), y0, (cellY + 0.5) * cells.getCellSize());
      cellOfs += ofs;

      Mesh *cellm = cells.getCellLandMesh(cellX, cellY, true);
      if (cellm)
        compress_land_position(cellOfs, cellScale, *cellm, false);

      cellm = cells.getCellDecalMesh(cellX, cellY, false);
      if (cellm)
        compress_land_position(cellOfs, cellScale, *cellm, true); //== get from material

      cellm = cells.getCellCombinedMesh(cellX, cellY, false);
      if (cellm)
        compress_land_position(cellOfs, cellScale, *cellm, true); //== get from material

      cellm = cells.getCellPatchesMesh(cellX, cellY, false);
      if (cellm)
        compress_land_position(cellOfs, cellScale, *cellm, true); //== get from material
    }

  // resync border taking into account landmesh vertex quantization
  if (strip_det_hmap_from_tracer)
  {
    int step = 32, w = (detRectC[1].x - detRectC[0].x) / step, h = (detRectC[1].y - detRectC[0].y) / step;

    for (int i = 0; i < det_hmap_border.size(); i++)
    {
      short sy = real2int(clamp((det_hmap_border[i].y - y0) * inv_scale, -1.f, 1.f) * 32767.0);
      det_hmap_border[i].y = (sy / 32767.0) * denominator / 2 + y0;
    }

    float ht, t;
    for (int x = detRectC[0].x, i = 0; x < detRectC[1].x; x++, i++)
    {
      t = x + 1 < detRectC[1].x ? float(i % step) / step : 1.0f;
      ht = lerp(det_hmap_border[i / step].y, det_hmap_border[i / step + 1].y, t);
      heightMapDet.setFinalData(x, detRectC[0].y, ht);

      ht = lerp(det_hmap_border[2 * w + h - i / step].y, det_hmap_border[2 * w + h - i / step - 1].y, t);
      heightMapDet.setFinalData(x, detRectC[1].y - 1, ht);
    }
    for (int y = detRectC[0].y, i = 0; y < detRectC[1].y; y++, i++)
    {
      t = y + 1 < detRectC[1].y ? float(i % step) / step : 1.0f;
      ht = lerp(det_hmap_border[2 * w + 2 * h - i / step].y, det_hmap_border[2 * w + 2 * h - i / step - 1].y, t);
      heightMapDet.setFinalData(detRectC[0].x, y, ht);

      ht = lerp(det_hmap_border[w + i / step].y, det_hmap_border[w + i / step + 1].y, t);
      heightMapDet.setFinalData(detRectC[1].x - 1, y, ht);
    }
    updateHeightMapTex(true);
  }

  cells.setOffset(ofs);
  con.endProgress();
  con.endLog();
  if (!generate_ok)
    landMeshMap.clear(true);
  guard_det_border = true;

  if (generate_ok)
    rebuildLandmeshDump();
  return generate_ok;
}

void HmapLandPlugin::clearTexCache()
{
  if (landMeshRenderer)
    hmlService->resetTexturesLandMesh(*landMeshRenderer);

  if (lmlightmapTexId != BAD_TEXTUREID)
    dagRender->releaseManagedTexVerified(lmlightmapTexId, lmlightmapTex);
  if (landMeshManager)
    for (int i = 0; i < landMeshManager->getDetailMap().cells.size(); i++)
      mem_set_ff(landMeshManager->getDetailMap().cells[i].detTexIds);
}
void HmapLandPlugin::refillTexCache()
{
  if (!landMeshManager)
    return;
  for (int i = 0; i < landMeshManager->getDetailMap().cells.size(); i++)
    updateLandDetailTexture(i);
}

void HmapLandPlugin::updateLandDetailTexture(unsigned i)
{
  if (!landMeshManager || landMeshManager->getDetailMap().cells.size() <= i)
    return;

  LoadElement &element = landMeshManager->getDetailMap().cells[i];
  int num_elems_x = landMeshManager->getDetailMap().sizeX;
  int elemSize = landMeshManager->getDetailMap().texElemSize;
  int texSize = landMeshManager->getDetailMap().texSize;

  bool done;
  loadLandDetailTexture(i % num_elems_x, i / num_elems_x, (Texture *)element.tex1, (Texture *)element.tex2, element.detTexIds, &done,
    texSize, elemSize);
}

void HmapLandPlugin::resetTexCacheRect(const IBBox2 &box_)
{
  if (!landMeshManager)
    return;

  int num_elems_x = landMeshManager->getDetailMap().sizeX;
  int elemSize = landMeshManager->getDetailMap().texElemSize;

  IBBox2 box = box_;
  if (detDivisor)
    box[0] /= detDivisor, box[1] /= detDivisor;

  for (int y = box[0].y; y <= box[1].y; y += elemSize)
    for (int x = box[0].x; x <= box[1].x; x += elemSize)
    {
      int i = (x / elemSize) + (y / elemSize) * num_elems_x;
      updateLandDetailTexture(i);
    }
}

float HmapLandPlugin::getLandCellSize() const { return landMeshMap.getCellSize(); }

Point3 HmapLandPlugin::getOffset() const { return landMeshMap.getOffset(); }

int HmapLandPlugin::getNumCellsX() const { return landMeshMap.getNumCellsX(); }

int HmapLandPlugin::getNumCellsY() const { return landMeshMap.getNumCellsY(); }

IPoint2 HmapLandPlugin::getCellOrigin() const { return landMeshMap.getOrigin(); }

const IBBox2 *HmapLandPlugin::getExclCellBBox()
{
  static IBBox2 bb;
  if (!detDivisor)
    return NULL;
  bb[0].set_xy((detRect[0] - heightMapOffset) / getLandCellSize());
  bb[1].set_xy((detRect[1] - heightMapOffset) / getLandCellSize());
  return &bb;
}

bool HmapLandPlugin::validateBuild(int target, ILogWriter &rep, PropPanel::ContainerPropertyControl *params)
{
  if (!useMeshSurface || exportType == EXPORT_PSEUDO_PLANE)
    return true;
#if defined(USE_LMESH_ACES)
  int ox = landMeshMap.getOffset().x / landMeshMap.getCellSize();
  int oz = landMeshMap.getOffset().z / landMeshMap.getCellSize();
  if (landMeshMap.getOrigin().x || landMeshMap.getOrigin().y)
  {
    DAEDITOR3.conError("Landmesh: origin must be zero, but is %d,%d", landMeshMap.getOrigin().x || landMeshMap.getOrigin().y);
    return false;
  }
  if (fabs(landMeshMap.getOffset().x - ox * landMeshMap.getCellSize()) +
        fabs(landMeshMap.getOffset().z - oz * landMeshMap.getCellSize()) >
      0.1)
  {
    DAEDITOR3.conError("Landmesh: offset=" FMT_P3 " is not multiple of cellSize=%.3f", P3D(landMeshMap.getOffset()),
      landMeshMap.getCellSize());
    return false;
  }
#endif
  return true;
}

bool HmapLandPlugin::addUsedTextures(ITextureNumerator &tn)
{
  if (!useMeshSurface || exportType == EXPORT_PSEUDO_PLANE)
    return true;

  String prj;
  DAGORED2->getProjectFolderPath(prj);
  String ltmap_fn = ::make_full_path(prj, "builtScene/lightmap.tga");

  DataBlock app_blk;
  if (!app_blk.load(DAGORED2->getWorkspace().getAppPath()))
    DAEDITOR3.conError("cannot read <%s>", DAGORED2->getWorkspace().getAppPath());
  const DataBlock *cvtBlk = app_blk.getBlockByNameEx("heightMap")->getBlockByName("lightmapCvtProps");

  if (cvtBlk)
  {
    calcGoodLandLighting();
    resetRenderer();
  }
  if (::dd_file_exist(ltmap_fn) && cvtBlk)
  {
    String a_name(0, "%s_lightmap", dd_get_fname(prj));
    DataBlock a_props;
    TextureMetaData tmd;
    ddsx::Buffer b;

    a_props = *cvtBlk;
    a_props.setBool("convert", true);
    a_props.setStr("name", ltmap_fn);
    tmd.read(a_props, mkbindump::get_target_str(tn.getTargetCode()));
    tmd.mqMip -= tmd.hqMip;
    tmd.lqMip -= tmd.hqMip;
    tmd.hqMip = 0;

    if (DAEDITOR3.getTexAssetBuiltDDSx(a_name + "~lev", a_props, b, tn.getTargetCode(), NULL, &DAGORED2->getConsole()))
      tn.addDDSxTexture(tmd.encode("$_lightmap*"), b);
    else
      DAEDITOR3.conError("failed to build lightmap for export: %s", ltmap_fn);
  }

  // bool use_sgeom = false; (code was unused and removed)

  return true;
}

extern bool hmap_export_tex(mkbindump::BinDumpSaveCB &cb, const char *tex_name, const char *fname, bool clamp, bool gamma1);

extern bool aces_export_detail_maps(mkbindump::BinDumpSaveCB &cb, int det_map_w, int det_map_h, int tex_elem_size,
  const Tab<SimpleString> &landclass_names, int base_ofs, bool optimize_size = false, bool tools_internal = false);


class LandRaySaver
{
public:
  static void save(mkbindump::BinDumpSaveCB &cb, LandRayTracer &lt)
  {
    cb.writeRaw((void *)"LTdump", 6);
    cb.writeInt32e(lt.numCellsX);
    cb.writeInt32e(lt.numCellsY);
    cb.writeReal(lt.cellSize);
    cb.writeReal(lt.offset.x);
    cb.writeReal(lt.offset.y);
    cb.writeReal(lt.offset.z);
    cb.writeReal(lt.bbox[0].x);
    cb.writeReal(lt.bbox[0].y);
    cb.writeReal(lt.bbox[0].z);
    cb.writeReal(lt.bbox[1].x);
    cb.writeReal(lt.bbox[1].y);
    cb.writeReal(lt.bbox[1].z);

    Tab<LandRayTracer::LandRayCellD> cellsD(tmpmem);
    lt.convertCellsToOut(cellsD);

    cb.writeInt32e(cellsD.size());
    cb.write32ex(cellsD.data(), data_size(cellsD));
    cb.writeInt32e(lt.grid.size());
    cb.writeTabData32e(lt.grid);
    cb.writeInt32e(lt.gridHt.size());
    cb.write32ex(lt.gridHt.data(), data_size(lt.gridHt));
    cb.writeInt32e(lt.allFaces.size());
    cb.writeTabData16e(lt.allFaces);
    cb.writeInt32e(lt.allVerts.size());
    cb.write32ex(lt.allVerts.data(), data_size(lt.allVerts));
    cb.writeInt32e(lt.faceIndices.size());
    cb.writeTabData16e(lt.faceIndices);
  }
};

static void exportRayTracersToGame(mkbindump::BinDumpSaveCB &cwr, LandRayTracer *lrt)
{
  int time0 = dagTools->getTimeMsec();
  CoolConsole &con = DAGORED2->getConsole();
  con.setActionDesc("saving lmesh tracers...");
  cwr.beginBlock();
  if (lrt)
  {
    // todo: add zlib compression
    int size = cwr.tell();
    LandRaySaver::save(cwr, *lrt);
    size = cwr.tell() - size;
    con.addMessage(ILogWriter::NOTE, "landray tracer saved %d bytes", size);
  }
  else
    con.addMessage(ILogWriter::WARNING, "No landray tracer");
  cwr.endBlock();
}

extern int exportImageAsDds(mkbindump::BinDumpSaveCB &cb, TexPixel32 *image, int size, int format, int mipmap_count, bool gamma1);

extern void buildLightTexture(TexPixel32 *image, MapStorage<uint32_t> &lightmap, int tex_data_sizex, int tex_data_sizey, int stride,
  int map_x, int map_y, bool use_normal_map);

TEXTUREID HmapLandPlugin::getLightMap()
{
  if (!hasLightmapTex)
    return BAD_TEXTUREID;

  if (lmlightmapTexId != BAD_TEXTUREID) //==
    return lmlightmapTexId;
  if (!lmlightmapTex)
  {
    lmlightmapTex =
      d3d::create_tex(NULL, lightMapScaled.getMapSizeX(), lightMapScaled.getMapSizeY(), TEXFMT_A8R8G8B8, 1, "lmesh_lightmap");

    d3d_err(lmlightmapTex);
    lmlightmapTex->texaddr(TEXADDR_CLAMP);
    lmlightmapTexId = dagRender->registerManagedTex(String(32, "lightmap_hmap_%p", lmlightmapTex), lmlightmapTex);
  }

  SmallTab<TexPixel32, TmpmemAlloc> image;
  clear_and_resize(image, lightMapScaled.getMapSizeX() * lightMapScaled.getMapSizeY());
  buildLightTexture(&image[0], lightMapScaled, lightMapScaled.getMapSizeX(), lightMapScaled.getMapSizeY(),
    lightMapScaled.getMapSizeX(), 0, 0, useNormalMap);

  int stride;
  uint8_t *p;
  if (((Texture *)lmlightmapTex)->lockimg((void **)&p, stride, 0, TEXLOCK_WRITE))
  {
    memcpy(p, image.data(), lightMapScaled.getMapSizeX() * lightMapScaled.getMapSizeY() * 4);
    ((Texture *)lmlightmapTex)->unlockimg();
  }
  return lmlightmapTexId;
}

int HmapLandPlugin::markUndergroundFaces(MeshData &mesh, Bitarray &facesAbove, TMatrix *wtm)
{
  struct HMDetTR2
  {
    int lodDiv;
    HMDetTR2() : lodDiv(1) {}

    static Point3 getHeightmapOffset() { return HmapLandPlugin::self->getHeightmapOffset(); }
    int getHeightmapSizeX() const { return HMDetTR::getHeightmapSizeX() / lodDiv; }
    int getHeightmapSizeY() const { return HMDetTR::getHeightmapSizeY() / lodDiv; }
    real getHeightmapCellSize() const { return HMDetTR::getHeightmapCellSize() * lodDiv; }
    bool getHeightmapCell5Pt(const IPoint2 &_cell, real &h0, real &hx, real &hy, real &hxy, real &hmid) const
    {
      IPoint2 cell = _cell * lodDiv;
      if (HmapLandPlugin::self->insideDetRectC(cell) && HmapLandPlugin::self->insideDetRectC(cell.x + lodDiv, cell.y + lodDiv))
      {
        h0 = HmapLandPlugin::self->heightMapDet.getFinalData(cell.x, cell.y);
        hx = HmapLandPlugin::self->heightMapDet.getFinalData(cell.x + lodDiv, cell.y);
        hy = HmapLandPlugin::self->heightMapDet.getFinalData(cell.x, cell.y + lodDiv);
        hxy = HmapLandPlugin::self->heightMapDet.getFinalData(cell.x + lodDiv, cell.y + lodDiv);

        hmid = (h0 + hx + hy + hxy) * 0.25f;
        return true;
      }
      return false;
    }
  };

  if (!mesh.face.size())
    return 0;
  EditorLandRayTracer *tracer = landMeshMap.getEditorLandRayTracer();
  if (!tracer)
    return 0;
  Bitarray vertsBelow;
  vertsBelow.resize(mesh.vert.size());
  vertsBelow.reset();
  const float heightEps = max(tracer->getBBox().width().y * 0.0001f, 0.00001f);
  const float maxHeight = tracer->getBBox()[1].y + 1.f;
  int vertsBelowCnt = 0;
  SmallTab<Point3, TmpmemAlloc> vertMem;
  Point3 *vertPtr = mesh.vert.data();
  if (wtm)
  {
    clear_and_resize(vertMem, mesh.vert.size());
    vertPtr = vertMem.data();
  }
  dag::Span<Point3> vert(vertPtr, mesh.vert.size());
  HMDetTR2 det_hm;
  BBox2 landBoundsXZ(Point2::xz(tracer->getBBox()[0]), Point2::xz(tracer->getBBox()[1]));
  for (int vi = 0; vi < mesh.vert.size(); ++vi)
  {
    if (wtm)
      vert[vi] = (*wtm) * mesh.vert[vi];
    float ht = vert[vi].y + heightEps;
    float det_ht = ht;

    if (detDivisor && (detRect & Point2::xz(vert[vi])))
    {
      if (get_height_midpoint_heightmap(det_hm, Point2::xz(vert[vi]), det_ht, NULL))
        if (det_ht >= ht)
        {
          vertsBelow.set(vi);
          vertsBelowCnt++;
        }
    }
    else if ((landBoundsXZ & Point2::xz(vert[vi])) && tracer->getHeightBelow(Point3::xVz(vert[vi], maxHeight), det_ht, NULL))
    {
      if (det_ht >= ht)
      {
        vertsBelow.set(vi);
        vertsBelowCnt++;
      }
    }
  }
  if (vertsBelowCnt == 0) // nothing to remove
    return 0;
  facesAbove.resize(mesh.face.size());
  facesAbove.set();
  int facesBelowCnt = 0;
  // fixme: optimize, instead of checking each edge as many times, as there adjacent faces, better do this once
  for (int fi = 0; fi < mesh.face.size(); ++fi)
  {
    Face &f = mesh.face[fi];
    if (!vertsBelow[f.v[0]] || !vertsBelow[f.v[1]] || !vertsBelow[f.v[2]])
      continue;
    Point3 v0 = vert[f.v[0]], v1 = vert[f.v[1]], v2 = vert[f.v[2]];
    float mint;
    mint = 1.00001f;
    if (tracer->traceRay(v0, v1 - v0, mint, NULL))
      continue;
    mint = 1.00001f;
    if (tracer->traceRay(v0, v2 - v0, mint, NULL))
      continue;
    mint = 1.00001f;
    if (tracer->traceRay(v1, v2 - v1, mint, NULL))
      continue;
    if (detDivisor)
    {
      bool cross = false;
      det_hm.lodDiv = 1;
      for (int lod = 0; lod < 6; lod++)
      {
        mint = length(v1 - v0) * 1.00001f;
        if (mint >= 1e-6 && ::trace_ray_midpoint_heightmap(det_hm, v0, (v1 - v0) / mint, mint, NULL))
        {
          cross = true;
          break;
        }
        mint = length(v2 - v0) * 1.00001f;
        if (mint >= 1e-6 && ::trace_ray_midpoint_heightmap(det_hm, v0, (v2 - v0) / mint, mint, NULL))
        {
          cross = true;
          break;
        }
        mint = length(v2 - v1) * 1.00001f;
        if (mint >= 1e-6 && ::trace_ray_midpoint_heightmap(det_hm, v1, (v2 - v1) / mint, mint, NULL))
        {
          cross = true;
          break;
        }
        det_hm.lodDiv <<= 1;
      }
      if (cross)
        continue;
    }

    facesAbove.reset(fi);
    facesBelowCnt++;
  }
  return facesBelowCnt;
}

void HmapLandPlugin::removeInvisibleFaces(StaticGeometryContainer &container)
{
  if (!landMeshMap.getEditorLandRayTracer())
    return;
  CoolConsole &con = DAGORED2->getConsole();
  con.setActionDesc("removing undeground faces...");
  con.startProgress();

  con.setTotal(container.nodes.size() / 128);
  int totalFaces = 0, removedFaces = 0;
  int time0 = dagTools->getTimeMsec();
  Bitarray facesAbove;

  for (int i = 0; i < container.nodes.size(); ++i)
  {
    if (!(i & 127))
      con.setDone(i / 128);

    StaticGeometryNode *node = container.nodes[i];

    if (!node || !node->mesh)
      continue;
    if (!node->mesh->mesh.check_mesh())
    {
      con.addMessage(ILogWriter::WARNING, "node mesh <%s> is invalid.", (const char *)node->name);
      continue;
    }
    MeshData &mesh = node->mesh->mesh;
    totalFaces += mesh.face.size();
    facesAbove.eraseall();
    int facesBelowCnt = markUndergroundFaces(mesh, facesAbove, &node->wtm);
    if (!facesBelowCnt)
      continue;
    if (node->mesh->getRefCount() > 1) // clone to remove
    {
      StaticGeometryMesh *newMesh = new StaticGeometryMesh;
      newMesh->mats = node->mesh->mats;
      newMesh->mesh = node->mesh->mesh;
      newMesh->user_data = node->mesh->user_data;
      node->mesh = newMesh;
    }
    node->mesh->mesh.removeFacesFast(facesAbove);
    removedFaces += facesBelowCnt;
  }
  con.endProgress();
  if (totalFaces)
    con.addMessage(ILogWriter::NOTE, "removed %d undeground faces out of %d (%02d%%) in %dms", removedFaces, totalFaces,
      removedFaces * 100 / totalFaces, dagTools->getTimeMsec() - time0);
  else
    con.addMessage(ILogWriter::NOTE, "no faces for removal in %dms", dagTools->getTimeMsec() - time0);
}

bool HmapLandPlugin::exportLandMesh(mkbindump::BinDumpSaveCB &cb, IWriterToLandmesh *land_modifier, LandRayTracer *landRayTracer,
  bool tools_internal)
{
  int time0 = dagTools->getTimeMsec();
  CoolConsole &con = DAGORED2->getConsole();
  if (requireTileTex && tileTexId == BAD_TEXTUREID)
  {
    con.addMessage(ILogWriter::FATAL, "HeightMap tile texture is missing!");
    return false;
  }
  con.startLog();

  // write additional geometry to landmesh
  if (land_modifier)
    land_modifier->writeToLandmesh(landMeshMap);

  bool game_res_sys_v2 = false;
  {
    DataBlock app_blk;
    if (!app_blk.load(DAGORED2->getWorkspace().getAppPath()))
      DAEDITOR3.conError("cannot read <%s>", DAGORED2->getWorkspace().getAppPath());
    game_res_sys_v2 = app_blk.getBlockByNameEx("assets")->getBlockByName("export") != NULL;
  }
  G_ASSERT(game_res_sys_v2);
  cb.writeRaw("lndm", 4);
  cb.writeInt32e(4); // version

  cb.writeReal(getGridCellSize());
  cb.writeReal(landMeshMap.getCellSize());
  cb.writeInt32e(landMeshMap.getNumCellsX());
  cb.writeInt32e(landMeshMap.getNumCellsY());
  cb.writeInt32e(landMeshMap.getOffset().x / landMeshMap.getCellSize());
  cb.writeInt32e(landMeshMap.getOffset().z / landMeshMap.getCellSize());

  // write collision area settings

  cb.writeInt32e(requireTileTex ? 1 : 0); // use tile or don't use tile

  int headerOfs = cb.tell();

  cb.writeInt32e(0); // mesh data offset
  cb.writeInt32e(0); // detail data offset
  cb.writeInt32e(0); // tile data offset
  cb.writeInt32e(0); // ray tracer data offset

  // export heightmap data
  int meshOfs = cb.tell();
  // exportLandMeshToGame(cb, landMeshMap);
  {
    MaterialData mat;
    mat.className = "land_mesh";
    Ptr<ShaderMaterial> material = dagGeom->newShaderMaterial(mat);
    if (material)
    {
      struct CountChannels : public ShaderChannelsEnumCB
      {
        int cnt = 0, code_flags = 0;
        void enum_shader_channel(int, int, int, int, int, ChannelModifier, int) override { cnt++; }
      };
      CountChannels ch_cnt;
      if (!material->enum_channels(ch_cnt, ch_cnt.code_flags) || !ch_cnt.cnt)
      {
        DAEDITOR3.conNote("stub shader used for '%s', exporting lanmesh without renderable geom", "land_mesh");
        material = nullptr;
      }
    }

    int numMat = material ? landMeshMap.getMaterialsCount() : 0;
    Tab<MaterialData> materials(tmpmem);
    materials.reserve(numMat);
    for (int i = 0; i < numMat; ++i)
      materials.push_back(landMeshMap.getMaterialData(i));

    if (!hmlService->exportToGameLandMesh(cb, landMeshMap.getCells(), materials, lod1TrisDensity, tools_internal))
      return false;
  }
  DAGORED2->getConsole().addMessage(ILogWriter::REMARK, "exported lmesh geometry in %g seconds (LOD1 density=%d%%)",
    (dagTools->getTimeMsec() - time0) / 1000.0, lod1TrisDensity);
  time0 = dagTools->getTimeMsec();

  // export detail maps and textures
  int detailDataOfs = cb.tell();
  int dmw = detTexWtMap ? detTexWtMap->getMapSizeX() : landClsMap.getMapSizeX();
  int dmh = detTexWtMap ? detTexWtMap->getMapSizeY() : landClsMap.getMapSizeY();
  int dm_tex_elem_size = (landMeshMap.getCellSize() / gridCellSize) * (detTexWtMap ? 1 : lcmScale);
  if (dmw / dm_tex_elem_size != landMeshMap.getNumCellsX() || dmh / dm_tex_elem_size != landMeshMap.getNumCellsY())
  {
    DAEDITOR3.conError("bad detMapSz=%dx%d and elemSz=%d gives elems=%dx%d while cells are %dx%d\n"
                       "  (detTexWtMap=%dx%d landClsMap=%dx%d lcmScale=%d)",
      dmw, dmh, dm_tex_elem_size, dmw / dm_tex_elem_size, dmh / dm_tex_elem_size, landMeshMap.getNumCellsX(),
      landMeshMap.getNumCellsY(), detTexWtMap ? detTexWtMap->getMapSizeX() : 0, detTexWtMap ? detTexWtMap->getMapSizeY() : 0,
      landClsMap.getMapSizeX(), landClsMap.getMapSizeY(), lcmScale);

    dm_tex_elem_size = dmw / landMeshMap.getNumCellsX();
    if (dm_tex_elem_size != dmh / landMeshMap.getNumCellsY())
    {
      DAEDITOR3.conError("failed to export color/detail maps: corrected elems=%dx%d wile cells is %dx%d", dmw / dm_tex_elem_size,
        dmh / dm_tex_elem_size, landMeshMap.getNumCellsX(), landMeshMap.getNumCellsY());
      return false;
    }
  }
  if (!aces_export_detail_maps(cb, dmw, dmh, dm_tex_elem_size, detailTexBlkName, headerOfs, true, tools_internal))
  {
    DAEDITOR3.conError("failed to export color/detail maps");
    return false;
  }

  DAGORED2->getConsole().addMessage(ILogWriter::REMARK, "exported lmesh detailmaps in %g seconds",
    (dagTools->getTimeMsec() - time0) / 1000.0);
  time0 = dagTools->getTimeMsec();

  int tileDataOfs = cb.tell();
  if (requireTileTex)
  {
    // export tile map
    DAGORED2->getConsole().addMessage(ILogWriter::REMARK, "exporting tile map...");

    hmap_export_tex(cb, tileTexName, tileTexName, false, false);

    cb.writeReal(tileXSize * gridCellSize);
    cb.writeReal(tileYSize * gridCellSize);
  }

  DAGORED2->getConsole().addMessage(ILogWriter::REMARK, "exported tile tex in %g seconds", (dagTools->getTimeMsec() - time0) / 1000.0);
  time0 = dagTools->getTimeMsec();

  int rayTracerOffset = cb.tell();
  if (!landRayTracer)
    rayTracerOffset = headerOfs;
  else
  {
    mkbindump::BinDumpSaveCB cwr_lt(2 << 10, cb.getTarget(), cb.WRITE_BE);
    exportRayTracersToGame(cwr_lt, landRayTracer);

    cb.beginBlock();
    MemoryLoadCB mcrd(cwr_lt.getMem(), false);
    if (preferZstdPacking && allowOodlePacking)
    {
      cb.writeInt32e(cwr_lt.getSize());
      oodle_compress_data(cb.getRawWriter(), mcrd, cwr_lt.getSize());
    }
    else if (preferZstdPacking)
      zstd_compress_data(cb.getRawWriter(), mcrd, cwr_lt.getSize(), 1 << 20, 19);
    else
      lzma_compress_data(cb.getRawWriter(), 9, mcrd, cwr_lt.getSize());
    cb.endBlock(preferZstdPacking ? (allowOodlePacking ? btag_compr::OODLE : btag_compr::ZSTD) : btag_compr::UNSPECIFIED);
    rayTracerOffset += 4;
  }

  int resultOffset = cb.tell();
  cb.seekto(headerOfs);
  cb.writeInt32e(meshOfs - headerOfs);
  cb.writeInt32e(detailDataOfs - headerOfs);
  cb.writeInt32e(tileDataOfs - headerOfs);
  cb.writeInt32e(rayTracerOffset - headerOfs);
  cb.seekto(resultOffset);
  con.endLog();

#if defined(USE_LMESH_ACES)
  if (!useVertTex || vertTexAng0 >= vertTexAng1 || vertTexName.empty())
    cb.writeDwString("");
  else
  {
    String tex_str(256, "%s*:%s*", vertTexName.str(), vertNmTexName.str());
    if (!useVertTexForHMAP && detDivisor)
      tex_str.append("!");
    cb.writeDwString(tex_str);
    cb.writeReal(1.f / max(vertTexXZtile, 1e-3f));
    cb.writeReal(1.f / max(vertTexYtile, 1e-3f));
    cb.writeReal(cosf(DEG_TO_RAD * vertTexAng0));
    cb.writeReal(cosf(DEG_TO_RAD * vertTexAng1));
    cb.writeReal(vertTexHorBlend);
    cb.writeReal(vertTexYOffset / max(vertTexYtile, 1e-3f));
    cb.writeDwString(String(256, "%s*", vertDetTexName.str()));
    cb.writeReal(1.f / max(vertDetTexXZtile, 1e-3f));
    cb.writeReal(1.f / max(vertDetTexYtile, 1e-3f));
    cb.writeReal(vertDetTexYOffset / max(vertDetTexYtile, 1e-3f));
  }
#endif
  return true;
}
