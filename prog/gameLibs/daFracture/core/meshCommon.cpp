// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/algorithm.h>
#include <EASTL/bitvector.h>
#include <dag/dag_vectorMap.h>
#include <debug/dag_debug3d.h>
#include <memory/dag_framemem.h>
#include <vecmath/dag_vecMath.h>
#include <ioSys/dag_fileIo.h>

#include <daFracture/core/meshCommon.h>
#include <daFracture/core/destrMesh.h>
#include <daFracture/render/material.h>


namespace frx
{

void DebugDrawContext::drawLine(Point3 a, Point3 b, E3DCOLOR col) const { draw_debug_line_buffered(tm * a, tm * b, col, timeout); }

void DebugDrawContext::drawArrow(Point3 a, Point3 b, E3DCOLOR col) const { draw_debug_arrow_buffered(tm * a, tm * b, col, timeout); }

void DebugDrawContext::drawPoint(Point3 a, E3DCOLOR col) const { draw_debug_sphere_buffered(tm * a, 0.0025f, col, 8, timeout); }


// --------------------------------------------------------------------------------------------


static bool convex_has_volume(dag::ConstSpan<Point3> pts, float eps = 1e-4f)
{
  if (pts.size() < 4)
    return false;

  const Point3 &p0 = pts[0];
  int i = 1;

  // a point distinct from p0 to form an edge
  Point3 edge;
  for (; i < pts.size(); i++)
  {
    edge = pts[i] - p0;
    if (lengthSq(edge) > eps * eps)
      break;
  }
  if (i == pts.size())
    return false;

  // a non-collinear point to form a plane
  Point3 normal;
  for (i++; i < pts.size(); i++)
  {
    normal = cross(edge, pts[i] - p0);
    if (lengthSq(normal) > eps * eps)
      break;
  }
  if (i == pts.size())
    return false;
  normal *= 1.f / length(normal);

  // any point off the plane => volume
  for (i++; i < pts.size(); i++)
    if (fabsf(dot(normal, pts[i] - p0)) > eps)
      return true;
  return false;
}

void mesh_prepare_convex_hull(const DestrContext &ctx, const DestrMesh &mesh, float min_dist,
  dag::Vector<Point3, framemem_allocator> &out_points)
{
  out_points.clear();
  eastl::bitvector<framemem_allocator> usedVerts;
  usedVerts.resize(mesh.verts.size());
  const float minDistSq = sqr(min_dist);

  for (const auto &face : mesh.faces)
  {
    if (!ctx.materials[face.mat].isSolid)
      continue;
    for (const uint32_t idx : face.idx)
    {
      if (usedVerts.test(idx, false))
        continue;
      usedVerts.set(idx, true);

      const Point3 v0 = mesh.verts[idx].pos;
      if (!eastl::any_of(out_points.begin(), out_points.end(), [&](const Point3 &v) { return lengthSq(v - v0) < minDistSq; }))
        out_points.push_back(v0);
    }
  }

  if (!convex_has_volume(out_points, 1e-3f))
    out_points.clear();
}


// --------------------------------------------------------------------------------------------


void mesh_normalize_transform(DestrContext &, DestrMesh &mesh)
{
  Point3 bmin(FLT_MAX, FLT_MAX, FLT_MAX), bmax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
  for (const auto &v : mesh.verts)
  {
    Point3 wp = mesh.tm * v.pos;
    bmin = min(bmin, wp);
    bmax = max(bmax, wp);
  }
  if (bmin.x > bmax.x)
    return;

  const Point3 center = (bmin + bmax) * 0.5f;
  TMatrix newTm = mesh.tm;
  newTm.orthonormalize();
  newTm.setcol(3, center);
  const TMatrix oldToNew = inverse(newTm) * mesh.tm;

  for (auto &v : mesh.verts)
    v.pos = oldToNew * v.pos;

  mesh.tm = newTm;
}


// --------------------------------------------------------------------------------------------


void dbg_file_save(const DestrContext &ctx, const DestrSystem &sys, const char *fname)
{
  FullFileSaveCB cb(fname);
  if (!cb.fileHandle)
    return;

  // Save materials (stage only)
  int numMats = ctx.renderMats.size();
  cb.write(&numMats, sizeof(numMats));
  for (const auto &mat : ctx.renderMats)
    cb.write(&mat.elem.stage, sizeof(mat.elem.stage));

  // Save pieces (id, TM, faces)
  int numPieces = sys.pieces.size();
  cb.write(&numPieces, sizeof(numPieces));
  for (const auto &[id, piece] : sys.pieces)
  {
    cb.write(&id, sizeof(id));
    cb.write(&piece.tm, sizeof(TMatrix));
    // Save verts (full Vertex struct: pos, norm, uv)
    int numVerts = piece.verts.size();
    cb.write(&numVerts, sizeof(numVerts));
    if (numVerts > 0)
      cb.write(piece.verts.data(), numVerts * sizeof(DestrMesh::Vertex));
    int numFaces = piece.faces.size();
    cb.write(&numFaces, sizeof(numFaces));
    if (numFaces > 0)
      cb.write(piece.faces.data(), numFaces * sizeof(DestrMesh::Face));
  }
}

void dbg_file_load(DestrContext &ctx, DestrSystem &sys, const char *fname, ShaderElement *sh_elem, ShaderMaterial *sh_mat)
{
  FullFileLoadCB cb(fname);
  if (!cb.fileHandle)
    return;

  // Load verts

  // Load materials: restore stage, build matDesc from given shader, assign shPtr/matPtr
  ShaderMatChannels channels = ShaderMatChannels::create(sh_mat);
  int numMats = 0;
  cb.read(&numMats, sizeof(numMats));
  ctx.renderMats.resize(numMats);
  ctx.materials.resize(numMats);
  for (int i = 0; i < numMats; i++)
  {
    cb.read(&ctx.renderMats[i].elem.stage, sizeof(ctx.renderMats[i].elem.stage));
    ctx.renderMats[i].channels = channels;
    ctx.renderMats[i].shMat = sh_mat;
    ctx.renderMats[i].elem.shElem = sh_elem;
    ctx.materials[i].isSolid = ctx.renderMats[i].elem.stage <= ShaderMesh::STG_atest;
  }

  // Load pieces
  int numPieces = 0;
  cb.read(&numPieces, sizeof(numPieces));
  for (int i = 0; i < numPieces; ++i)
  {
    int id = 0;
    cb.read(&id, sizeof(id));
    DestrMesh &piece = sys.pieces[id];
    cb.read(&piece.tm, sizeof(TMatrix));
    int numVerts = 0;
    cb.read(&numVerts, sizeof(numVerts));
    piece.verts.resize(numVerts);
    if (numVerts > 0)
      cb.read(piece.verts.data(), numVerts * sizeof(DestrMesh::Vertex));
    int numFaces = 0;
    cb.read(&numFaces, sizeof(numFaces));
    piece.faces.resize(numFaces);
    if (numFaces > 0)
      cb.read(piece.faces.data(), numFaces * sizeof(DestrMesh::Face));
  }
}


} // namespace frx
