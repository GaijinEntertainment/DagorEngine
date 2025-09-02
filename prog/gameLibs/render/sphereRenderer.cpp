// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/sphereRenderer.h>
#include <render/primitiveObjects.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <3d/dag_lockSbuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_draw.h>
#include <perfMon/dag_statDrv.h>

void SphereRenderer::init(int slices_, int stacks_)
{
  close();

  slices = slices_;
  stacks = stacks_;
  calc_sphere_vertex_face_count(slices, stacks, false, vertexCount, faceCount);

  maxRelativeRadiusError = calc_sphere_max_radius_error(1.f, slices, stacks);

  createAndFillBuffers();
}

void SphereRenderer::close()
{
  slices = 0;
  stacks = 0;
  vertexCount = 0;
  faceCount = 0;
  maxRelativeRadiusError = 0.f;
  sphereVb.close();
  sphereIb.close();
}

bool SphereRenderer::isInited() const { return vertexCount > 0 && faceCount > 0; }

void SphereRenderer::createAndFillBuffers()
{
  G_ASSERT(isInited());

  if (!sphereVb)
    sphereVb = dag::create_vb(vertexCount * sizeof(Point3), 0, "sphereRenderer_VB");
  if (!sphereIb)
    sphereIb = dag::create_ib(faceCount * 6, 0, "sphereRenderer_IB");

  auto indices = lock_sbuffer<uint8_t>(sphereIb.getBuf(), 0, 6 * faceCount, VBLOCK_WRITEONLY);
  auto vertices = lock_sbuffer<uint8_t>(sphereVb.getBuf(), 0, vertexCount * sizeof(Point3), VBLOCK_WRITEONLY);
  if (indices && vertices)
  {
    create_sphere_mesh(dag::Span<uint8_t>(vertices.get(), vertexCount * sizeof(Point3)),
      dag::Span<uint8_t>(indices.get(), faceCount * 6), 1.0f, slices, stacks, sizeof(Point3), false, false, false, false);
  }
}

void SphereRenderer::render(DynamicShaderHelper &shader, TMatrix view_tm) const
{
  G_ASSERT(isInited());

  SCOPE_VIEW_PROJ_MATRIX;
  view_tm.setcol(3, Point3::ZERO);
  d3d::settm(TM_VIEW, view_tm);

  TIME_D3D_PROFILE(applyOnSphere);
  shader.shader->setStates();
  d3d::setind(sphereIb.getBuf());
  d3d::setvsrc(0, sphereVb.getBuf(), sizeof(Point3));
  d3d::drawind(PRIM_TRILIST, 0, faceCount, 0);
}

float SphereRenderer::getMaxRadiusError(float radius) const { return radius * maxRelativeRadiusError; }