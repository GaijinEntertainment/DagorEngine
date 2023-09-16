#include <libTools/ObjCreator3d/objCreator3d.h>
#include <math/dag_math2d.h>
#include <debug/dag_debug.h>
#include <math/dag_bezier.h>


//==============================================================================================
// return nearest point on spline (return position index)
static float getSplinePos(real beg, real end, real delta, const Point3 &world_pos, BezierSpline3d &spline, Point3 *ret_pos)
{
  real rdc = MAX_REAL;
  int baseroad_pos = -1;

  Point3 p;
  Point3 minPoint(0, 0, 0);
  real l;

  if (beg < 0.0f)
    beg = 0.0f;
  if (end > spline.leng)
    end = spline.leng;

  bool _found = false;

  float totalLen = 0;
  for (int iSeg = 0; iSeg < spline.segs.size(); iSeg++)
  {
    BezierSplineInt3d &seg = spline.segs[iSeg];

    if (totalLen + seg.len < beg)
    {
      totalLen += seg.len;
      continue;
    }

    for (float pos = 0; (pos <= seg.len) && (totalLen + pos <= end); pos += delta)
    {
      p = seg.point(pos / seg.len);
      l = lengthSq(p - world_pos);

      if (l < rdc)
      {
        rdc = l;
        minPoint = p;
        baseroad_pos = totalLen + pos;
        _found = true;
      }
    }
    totalLen += seg.len;
    if (totalLen > end)
      break;
  }

  if (!_found)
    return -1;
  //  G_ASSERT(_found);

  if (ret_pos)
    *ret_pos = minPoint;

  return baseroad_pos;
}


//==============================================================================================
// return nearest point on spline (return position index)
static float getSplinePos(const Point3 &world_pos, BezierSpline3d &spline, Point3 *ret_pos)
{
  real rdc = MAX_REAL;
  int baseroad_pos = -1;
  real delta = spline.leng * 0.0001f;
  if (delta < 1)
    delta = 1;

  return getSplinePos(0, spline.leng, delta, world_pos, spline, ret_pos);
}


//=====================================================================================================
static void interpolateSection(Point3 *prev, Point3 *next, int count, real pos, Tab<Point3> &section, BezierSpline3d *path)
{
  // find proportions:
  Point3 prevCenter(0, 0, 0), nextCenter(0, 0, 0);
  for (int i = 0; i < count; i++)
    prevCenter += prev[i];
  prevCenter /= count;
  real prevPos = getSplinePos(prevCenter, *path, &prevCenter);

  for (int i = 0; i < count; i++)
    nextCenter += next[i];
  nextCenter /= count;
  real nextPos = getSplinePos(nextCenter, *path, &nextCenter);

  // interpolate
  for (int i = 0; i < count; i++)
  {
    Point3 p = path->get_pt(pos);

    real percent = (pos - prevPos) / (nextPos - prevPos);
    p += (prev[i] - prevCenter) * (1 - percent);
    p += (next[i] - nextCenter) * percent;

    section.push_back(p);
  }
}

//=====================================================================================================
static void addSection(MeshData *mesh, Point3 *section, int count, int close_type)
{
  int startVertId = mesh->vert.size();
  append_items(mesh->vert, count, section);

  if (close_type == -1)
    for (int i = 1; i < count - 1; i++)
    {
      Face face;
      face.set(startVertId, startVertId + i + 1, startVertId + i);
      mesh->face.push_back(face);
    }

  if (startVertId == 0)
    return;

  for (int i = 0; i < count; i++)
  {
    int next = (i + 1) % count;
    Face face;
    face.set(startVertId + i, startVertId - count + i, startVertId - count + next);
    mesh->face.push_back(face);
    face.set(startVertId + i, startVertId - count + next, startVertId + next);
    mesh->face.push_back(face);
  }

  if (close_type == 1)
    for (int i = 1; i < count - 1; i++)
    {
      Face face;
      face.set(startVertId, startVertId + i, startVertId + i + 1);
      mesh->face.push_back(face);
    }
}


//=====================================================================================================
// MeshData *ObjCreator3d::generateLoftMesh(const Tab<Point3> &points, const Tab<Point3*> &sections, int points_count,
//    real  step)
Mesh *ObjCreator3d::generateLoftMesh(BezierSpline3d &path, const Tab<Point3 *> &sections, int points_count, real step, bool closed)
{
  Color4 col = Color4(1, 1, 1);

  Tab<real> offsets(tmpmem);

  for (int i = 0; i < sections.size(); i++)
  {
    Point3 center(0, 0, 0);
    for (int j = 0; j < points_count; j++)
      center += sections[i][j];
    center /= points_count;
    real pos = getSplinePos(center, path, &center);
    offsets.push_back(pos);
  }

  // check offsets
  for (int i = 1; i < offsets.size(); i++)
    if (offsets[i - 1] > offsets[i])
      return NULL;

  // create & buld mesh
  Mesh *retMesh = new (midmem) Mesh;
  MeshData *mesh = (MeshData *)retMesh;
  if (!mesh)
    return NULL;

  for (real pos = offsets[0]; pos < offsets.back(); pos += step)
  {
    int closeType = 0;
    if (closed)
    {
      if (pos == offsets[0])
        closeType = -1;
      else if (pos + step >= offsets.back())
        closeType = 1;
    }

    Tab<Point3> section(tmpmem);
    for (int i = 0; i < offsets.size(); i++)
      if (offsets[i] > pos)
      {
        interpolateSection(sections[i - 1], sections[i], points_count, pos, section, &path);
        addSection(mesh, section.data(), points_count, closeType);
        break;
      }
  }

  mesh->tvert[0].resize(mesh->vert.size());
  for (int i = 0; i < mesh->vert.size(); ++i)
  {
    mesh->tvert[0][i].x = mesh->vert[i].x;
    mesh->tvert[0][i].y = mesh->vert[i].y;
  }

  mesh->tface[0].resize(mesh->face.size());
  for (int i = 0; i < mesh->face.size(); ++i)
  {
    mesh->tface[0][i].t[0] = mesh->face[i].v[0];
    mesh->tface[0][i].t[1] = mesh->face[i].v[1];
    mesh->tface[0][i].t[2] = mesh->face[i].v[2];
  }
  // set colors
  mesh->cvert.resize(1);
  mesh->cvert[0] = col;

  mesh->cface.resize(mesh->face.size());
  for (int fi = 0; fi < mesh->face.size(); ++fi)
  {
    mesh->cface[fi].t[0] = 0;
    mesh->cface[fi].t[1] = 0;
    mesh->cface[fi].t[2] = 0;
  }

  return retMesh;
}
