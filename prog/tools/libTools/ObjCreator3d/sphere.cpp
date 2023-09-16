#include <libTools/ObjCreator3d/objCreator3d.h>


//==============================================================================
void ObjCreator3d::generateRing(Tab<Point3> &vert, Tab<Point2> &tvert, int segments, real y)
{
  const real radius = 0.5 * sqrtf(1 - y * y * 4);

  vert.resize(segments);
  tvert.resize(segments + 1);

  const real step = TWOPI / segments;
  const real ty = acos(y * 2) / PI;
  real angle = 0;

  for (int i = 0; i < segments; ++i, angle += step)
  {
    vert[i] = Point3(radius * cos(angle), y, radius * sin(angle));
    tvert[i] = Point2(angle / TWOPI, ty);
  }

  tvert[segments] = Point2(angle / TWOPI, ty);
}


//==============================================================================
Mesh *ObjCreator3d::generateSphereMesh(int segments, Color4 col)
{
  if (segments < 4)
    segments = 4;

  Mesh *retMesh = new (midmem) Mesh;
  MeshData *mesh = (MeshData *)retMesh;
  if (!mesh)
    return NULL;

  const int resizeStep = segments * 10;
  const int endRing = segments / 2 + 1;
  const real dy = 2.0 / segments;

  const real segAngle = PI / (endRing - 1);

  Tab<Point3> vert(tmpmem);
  Tab<Point2> tvert(tmpmem);

  Face face;
  face.smgr = 1;

  TFace tface;

  for (int ring = 0; ring < endRing; ++ring)
  {
    real y = 0.5 * cosf(segAngle * ring);
    // north pole
    if (!ring)
    {
      mesh->vert.push_back(Point3(0, 0.5, 0));
      mesh->tvert[0].push_back(Point2(0, 1));

      continue;
    }

    // south pole
    if (ring == endRing - 1)
    {
      mesh->vert.push_back(Point3(0, -0.5, 0));
      mesh->tvert[0].push_back(Point2(0, 0));

      continue;
    }

    // add ring vertexes
    generateRing(vert, tvert, segments, y);
    append_items(mesh->vert, segments, vert.data());
    append_items(mesh->tvert[0], segments + 1, tvert.data());

    if (ring > 1 && ring < endRing - 1)
    {
      for (int vi = mesh->vert.size() - segments, ti = mesh->tvert[0].size() - segments - 1; vi < mesh->vert.size(); ++vi, ++ti)
      {
        face.v[0] = vi;
        face.v[1] = vi - segments;
        face.v[2] = vi + 1;

        tface.t[0] = ti;
        tface.t[1] = ti - segments - 1;
        tface.t[2] = ti + 1;

        if (vi == mesh->vert.size() - 1)
          face.v[2] = mesh->vert.size() - segments;

        mesh->face.push_back(face);
        mesh->tface[0].push_back(tface);

        face.v[0] = vi - segments;
        face.v[1] = vi - segments + 1;
        face.v[2] = vi + 1;

        tface.t[0] = ti - segments - 1;
        tface.t[1] = ti - segments;
        tface.t[2] = ti + 1;

        if (vi == mesh->vert.size() - 1)
        {
          face.v[1] = mesh->vert.size() - segments * 2;
          face.v[2] = mesh->vert.size() - segments;
        }

        mesh->face.push_back(face);
        mesh->tface[0].push_back(tface);
      }
    }

    // next ring after north pole
    if (ring == 1)
    {
      for (int vi = 1; vi <= segments; ++vi)
      {
        face.v[0] = tface.t[0] = vi;
        face.v[1] = tface.t[1] = 0;
        face.v[2] = tface.t[2] = vi + 1;

        if (vi == segments)
          face.v[2] = 1;

        mesh->face.push_back(face);
        mesh->tface[0].push_back(tface);
      }
    }

    // ring before south pole
    if (ring == endRing - 2)
    {
      for (int vi = mesh->vert.size() - segments, ti = mesh->tvert[0].size() - segments - 1; vi < mesh->vert.size(); ++vi, ++ti)
      {
        face.v[0] = mesh->vert.size();
        face.v[1] = vi;
        face.v[2] = vi + 1;

        if (vi == mesh->vert.size() - 1)
          face.v[2] = mesh->vert.size() - segments;

        tface.t[0] = mesh->tvert[0].size();
        tface.t[1] = ti;
        tface.t[2] = ti + 1;

        mesh->face.push_back(face);
        mesh->tface[0].push_back(tface);
      }
    }
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

  mesh->vert.shrink_to_fit();
  mesh->face.shrink_to_fit();
  mesh->tface[0].shrink_to_fit();
  mesh->facenorm.shrink_to_fit();

  mesh->calc_ngr();
  mesh->calc_vertnorms();

  return retMesh;
}
