#pragma once

#include <de3_landMeshData.h>

#include <math/dag_mesh.h>
#include <libTools/staticGeom/staticGeometryContainer.h>

class IBaseLoad;
class IBaseSave;
class ShaderMesh;
class ShaderMaterial;
class LandRayTracer;
class EditorLandRayTracer;
class MaterialData;
class StaticGeometryTexture;
class StaticGeometryMaterial;
class DelaunayImportanceMask;

class LandMeshMap
{
public:
  LandMeshMap();
  ~LandMeshMap();

  void gatherExportToDagGeometry(StaticGeometryContainer &container);

  void getStats(int &min_faces, int &max_faces, int &total_faces);

  void clear(bool clear_tracer);

  bool isEmpty() const { return cells.empty(); }

  void resize(int x0, int y0, int x1, int y1, float cell_size)
  {
    clear(false);

    size.x = x1 - x0;
    G_ASSERT(size.x >= 0);
    if (size.x == 0)
      ++size.x;

    size.y = y1 - y0;
    G_ASSERT(size.y >= 0);
    if (size.y == 0)
      ++size.y;

    origin.x = x0;
    origin.y = y0;

    clear_and_resize(cells, size.x * size.y);
    mem_set_0(cells);

    land_rmesh.resize(size.x * size.y);
    mem_set_0(land_rmesh);

    cellSize = cell_size;
  }

  int getNumCellsX() const { return size.x; }
  int getNumCellsY() const { return size.y; }
  enum LandType
  {
    LAND_LAND,
    LAND_DECAL,
    LAND_COMBINED,
    LAND_PATCHES
  };
  template <LandType land_type>
  Mesh *getCellTypedMesh(int x, int y, bool do_create = false)
  {
    if (x < 0 || y < 0 || x >= size.x || y >= size.y)
      return NULL;

    Mesh **m = NULL;
    if (land_type == LAND_LAND)
      m = &cells[y * size.x + x].land_mesh;
    else if (land_type == LAND_DECAL)
      m = &cells[y * size.x + x].decal_mesh;
    else if (land_type == LAND_COMBINED)
      m = &cells[y * size.x + x].combined_mesh;
    else if (land_type == LAND_PATCHES)
      m = &cells[y * size.x + x].patches_mesh;

    if (do_create && !*m)
      *m = new (midmem) Mesh();

    return *m;
  }
  Mesh *getCellLandMesh(int x, int y, bool do_create = false) { return getCellTypedMesh<LAND_LAND>(x, y, do_create); }
  Mesh *getCellDecalMesh(int x, int y, bool do_create = false) { return getCellTypedMesh<LAND_DECAL>(x, y, do_create); }
  Mesh *getCellCombinedMesh(int x, int y, bool do_create = false) { return getCellTypedMesh<LAND_COMBINED>(x, y, do_create); }
  Mesh *getCellPatchesMesh(int x, int y, bool do_create = false) { return getCellTypedMesh<LAND_PATCHES>(x, y, do_create); }

  float getCellSize() const { return cellSize; }

  IPoint2 calcCellCoords(const Point3 &p) const { return IPoint2(int(floorf(p.x / cellSize)), int(floorf(p.z / cellSize))); }

  IPoint2 getOrigin() const { return origin; }

  ShaderMesh *getLandShaderMesh(int x, int y, bool do_create, bool offseted);
  ShaderMesh *getCombinedShaderMesh(int x, int y, bool do_create, bool offseted) { return NULL; }
  ShaderMesh *getDecalShaderMesh(int x, int y, bool do_create, bool offseted) { return NULL; }
  BBox3 getBBox(int x, int y, float *sphere_radius = NULL);

  dag::Span<landmesh::Cell> getCells() { return make_span(cells); }
  EditorLandRayTracer *getEditorLandRayTracer() const { return editorLandTracer; }
  LandRayTracer *getGameLandRayTracer() const { return gameLandTracer; }

  void setEditorLandRayTracer(EditorLandRayTracer *lrt);
  void setGameLandRayTracer(LandRayTracer *lrt);
  MaterialData getMaterialData(uint32_t matNo) const;

  // traceRay
  bool traceRay(const Point3 &p, const Point3 &dir, real &t, Point3 *normal) const;

  //! Get maximum height at point (and normal, if needed)
  bool getHeight(const Point2 &p, real &ht, Point3 *normal) const;

  int addMaterial(StaticGeometryMaterial *material, bool *clipmap_only, bool *has_vertex_opacity, int node_layer = -1,
    bool *is_height_patch = NULL);
  int addTexture(StaticGeometryTexture *texture);

  void addStaticCollision(StaticGeometryContainer &cont);

  int getMaterialsCount() const { return materials.size(); }
  const char *textureName(int i) const { return textures[i]->fileName; }
  int getTexturesCount() const { return textures.size(); }

  void getMd5HashForGenerate(unsigned char *md5_hash, HeightMapStorage &heightmap, float error_threshold, int point_limit, float cell,
    const Point3 &ofs, const DelaunayImportanceMask *mask, dag::ConstSpan<Point3> add_pts, dag::ConstSpan<Point3> water_border_polys);

  void setOffset(const Point3 &ofs) { offset = ofs; }
  Point3 getOffset() const { return offset; }

  void buildCollisionNode(Mesh &m);

protected:
  SmallTab<landmesh::Cell, MidmemAlloc> cells;
  IPoint2 size, origin;
  float cellSize;
  Point3 offset;
  EditorLandRayTracer *editorLandTracer;
  LandRayTracer *gameLandTracer;
  PtrTab<StaticGeometryMaterial> materials;
  PtrTab<StaticGeometryTexture> textures;

  Ptr<StaticGeometryMaterial> landMatSG;
  Ptr<StaticGeometryMaterial> landBottomMatSG;
  Ptr<StaticGeometryMaterial> landBottomDeepMatSG;
  Ptr<StaticGeometryMaterial> landBottomBorderMatSG;
  StaticGeometryNode *collisionNode;

  Ptr<ShaderMaterial> landMat;
  Tab<ShaderMesh *> land_rmesh;
};
