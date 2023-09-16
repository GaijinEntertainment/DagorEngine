#include <libTools/ObjCreator3d/objCreator3d.h>
#include <math/dag_math2d.h>
#include <debug/dag_debug.h>

inline bool lines_intersection(const Point2 &p1, const Point2 &p2, const Point2 &p3, const Point2 &p4)
{
  float x1, x2, x3, x4, y1, y2, y3, y4, tmp;

  if (p1.x > p2.x)
  {
    x1 = p2.x;
    x2 = p1.x;
  }
  else
  {
    x1 = p1.x;
    x2 = p2.x;
  }
  if (p3.x > p4.x)
  {
    x3 = p4.x;
    x4 = p3.x;
  }
  else
  {
    x3 = p3.x;
    x4 = p4.x;
  }
  if (p1.y > p2.y)
  {
    y1 = p2.y;
    y2 = p1.y;
  }
  else
  {
    y1 = p1.y;
    y2 = p2.y;
  }
  if (p3.y > p4.y)
  {
    y3 = p4.y;
    y4 = p3.y;
  }
  else
  {
    y3 = p3.y;
    y4 = p4.y;
  }

  if ((x2 >= x3) && (x4 >= x1) && (y2 >= y3) && (y4 >= y1))
  {
    x1 = p2.x - p1.x;
    y1 = p2.y - p1.y;
    x2 = p4.x - p1.x;
    y2 = p4.y - p1.y;
    x3 = p3.x - p1.x;
    y3 = p3.y - p1.y;
    if (((x1 * y2 - x2 * y1) * (x1 * y3 - x3 * y1)) > 0)
      return false;
    x1 = p3.x - p4.x;
    y1 = p3.y - p4.y;
    x2 = p2.x - p4.x;
    y2 = p2.y - p4.y;
    x3 = p1.x - p4.x;
    y3 = p1.y - p4.y;
    if (((x1 * y2 - x2 * y1) * (x1 * y3 - x3 * y1)) > 0)
      return false;
    return true;
  }
  return false;
}

struct PosIndex
{
  Point2 pos;
  int index;
  PosIndex(const Point2 &p, int id)
  {
    pos = p;
    index = id;
  }
};

void ObjCreator3d::generatePolyPlaneFaces(const Tab<Point2> &poses, int vertex_offset, Tab<Face> &face, int &face_id, bool is_extern)
{
  Tab<PosIndex> p(tmpmem);
  p.reserve(poses.size());
  for (int i = 0; i < poses.size(); ++i)
    p.push_back(PosIndex(poses[i], vertex_offset + i));
  Tab<PosIndex> origin(p);

  while (p.size() > 2)
  {
    Point3 v1(p[1].pos.x - p[0].pos.x, 0, p[1].pos.y - p[0].pos.y);
    Point3 v2(p[2].pos.x - p[1].pos.x, 0, p[2].pos.y - p[1].pos.y);
    if ((v1 % v2).y < 0.0f)
    {
      int i;
      for (i = 1; i < origin.size(); ++i)
        if ((origin[i - 1].index != p[0].index) && (origin[i - 1].index != p[2].index) && (origin[i].index != p[0].index) &&
            (origin[i].index != p[2].index) && ::lines_intersection(p[0].pos, p[2].pos, origin[i - 1].pos, origin[i].pos))
          break;
      if (i == origin.size())
      {
        Tab<Point2> triangle(tmpmem);
        triangle.reserve(3);
        triangle.push_back(p[0].pos);
        triangle.push_back(p[1].pos);
        triangle.push_back(p[2].pos);
        for (i = 3; i < p.size(); ++i)
          if (::is_point_in_conv_poly(p[i].pos, &triangle[0], 3))
            break;
        if (i == p.size())
        {
          appendTriangle(p[0].index, p[1].index, p[2].index, face, face_id, is_extern);
          erase_items(p, 1, 1);
          continue;
        }
      }
    }
    PosIndex item(p[0].pos, p[0].index);
    erase_items(p, 0, 1);
    p.push_back(item);
  }
}

Mesh *ObjCreator3d::generatePolyMesh(const Tab<Point2> &points, int height_segments)
{
  if ((points.size() < 2) || (height_segments < 1))
    return NULL;

  Mesh *retMesh = new (midmem) Mesh;
  MeshData *mesh = (MeshData *)retMesh;
  if (!mesh)
    return NULL;

  Tab<Point2> poses(tmpmem);
  poses.reserve(points.size() + 1);
  poses = points;
  const int pointCount = poses.size();
  mesh->vert.reserve(pointCount * (height_segments + 1));
  mesh->face.resize(((pointCount - 2) + pointCount * height_segments) * 2);

  // generate vertex
  const float heightStep = 1.0f / (float)height_segments;
  for (int i = height_segments; i >= 0; --i)
  {
    const float y = heightStep * (float)i;
    for (int j = 0; j < poses.size(); ++j)
      mesh->vert.push_back(Point3(poses[j].x, y, poses[j].y));
  }

  mesh->tvert[0].resize(mesh->vert.size());
  for (int i = 0; i < mesh->vert.size(); ++i)
  {
    mesh->tvert[0][i].x = mesh->vert[i].x;
    mesh->tvert[0][i].y = mesh->vert[i].y;
  }


  // generate faces
  int faceId = 0;
  int x1, x2, x3, x4;
  for (int i = 0; i < height_segments; ++i)
  {
    x1 = pointCount * i;
    x2 = x1 + pointCount;
    for (int j = 0; j < pointCount - 1; ++j)
    {
      x3 = x2 + 1;
      x4 = x1 + 1;
      appendQuad(x1, x2, x3, x4, mesh->face, faceId, true);
      x1 = x4;
      x2 = x3;
    }
    appendQuad(x1, x2, pointCount * (i + 1), pointCount * i, mesh->face, faceId, true);
  }

  generatePolyPlaneFaces(poses, 0, mesh->face, faceId, true);
  generatePolyPlaneFaces(poses, pointCount * height_segments, mesh->face, faceId, false);

  //  for( int i=faceId; i<mesh->face.size(); ++i )
  //    mesh->face[i] = mesh->face[0];
  G_ASSERT(faceId == mesh->face.size());

  mesh->tface[0].resize(mesh->face.size());
  for (int i = 0; i < mesh->face.size(); ++i)
  {
    mesh->tface[0][i].t[0] = mesh->face[i].v[0];
    mesh->tface[0][i].t[1] = mesh->face[i].v[1];
    mesh->tface[0][i].t[2] = mesh->face[i].v[2];
  }
  return retMesh;
}
