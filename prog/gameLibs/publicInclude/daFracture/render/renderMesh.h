//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "../core/destrMesh.h"
#include "material.h"


namespace frx
{

struct DestrMesh;
struct DestrContext;


struct RenderMesh
{
  struct RElem
  {
    RenderMatElem mat;
    int vStride, fCnt;
    int vbAlloc, ibAlloc;
  };
  dag::Vector<RElem> elems;

  float bSphereRad = 0.f;
  Point3 initialPos = Point3::ZERO;

  RenderMesh() = default;
  RenderMesh(const RenderMesh &) = delete;
  RenderMesh &operator=(const RenderMesh &) = delete;
  RenderMesh(RenderMesh &&) = default;
  RenderMesh &operator=(RenderMesh &&) = default;
  ~RenderMesh();
};

struct MeshUploadRequest
{
  const DestrMesh *srcGeom;
  RenderMesh *dstMesh;
};
void upload_mesh_batch(const DestrContext &ctx, dag::Span<MeshUploadRequest> meshes);


struct ShaderGeomLoadRequest
{
  DestrMesh *outMesh = nullptr;

  ShaderMatVdata *smvd = nullptr;
  const ShaderMesh *shaderMesh = nullptr;
  TMatrix tm = TMatrix::IDENT;
  PerInstRenderData instData = {};
  bool invertWinding = false;
};
void add_destr_geometry(DestrContext &ctx, dag::Span<ShaderGeomLoadRequest> requests);

} // namespace frx
