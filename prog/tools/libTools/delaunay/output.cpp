#include <generic/dag_tab.h>
#include <math/dag_Point3.h>
#include <math/dag_mesh.h>
#include "terra.h"

// using namespace std;
namespace delaunay
{
static Mesh *out = NULL;

static void dag_face(Triangle &T, void *closure)
{
  array2<int> &vert_id = *(array2<int> *)closure;

  const Vec2 &p1 = T.point1();
  const Vec2 &p2 = T.point2();
  const Vec2 &p3 = T.point3();
  Face f;
  f.mat = 0;
  f.smgr = 1;
  f.v[0] = vert_id((int)p1[X], (int)p1[Y]);
  f.v[2] = vert_id((int)p2[X], (int)p2[Y]);
  f.v[1] = vert_id((int)p3[X], (int)p3[Y]);
  out->face.push_back(f);
}

void generate_output(Mesh &out_mesh, float cellSize, const Point3 &offset, Map *DEM)
{
  int width = DEM->width();
  int height = DEM->height();
  array2<int> vert_id(width, height);
  int index = 1;
  out_mesh.vert.reserve(mesh->pointCount());
  out_mesh.face.reserve(mesh->pointCount());
  out = &out_mesh;
  float scaleX = cellSize;
  float scaleY = cellSize;
  for (int y = 0; y < height; y++)
    for (int x = 0; x < width; x++)
    {
      if (mesh->is_used(x, y) == DATA_POINT_USED)
      {
        vert_id(x, y) = out_mesh.vert.size();
        out_mesh.vert.push_back(Point3(x * scaleX, DEM->eval(x, y), y * scaleY) + offset);
      }
      else
        vert_id(x, y) = -1;
    }
  mesh->overFaces(dag_face, &vert_id);
}

}; // namespace delaunay
