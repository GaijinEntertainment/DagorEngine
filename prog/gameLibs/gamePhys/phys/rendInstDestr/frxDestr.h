// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <dag/dag_vector.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/riexHandle.h>
#include <daFracture/core/meshSlicing.h>


class RenderableInstanceLodsResource;
namespace frx
{
struct DestrContext;
struct DestrSystem;
struct MeshRenderList;
} // namespace frx

namespace rendinstdestr
{

struct FrxDestrProps;

struct FrxDestrInput
{
  struct Mesh
  {
    const RenderableInstanceLodsResource *res = nullptr;
    TMatrix tm = TMatrix::IDENT;
    uint32_t seed = 0;
    bool invertWinding = false;
  };
  rendinst::RendInstBufferData buffer = {};
  const FrxDestrProps *frxProps = nullptr;
  frx::ShatterImpactProfile impact;
  dag::Vector<Mesh> meshes;
};

bool perform_procedural_destruction(const rendinst::RendInstBufferData &buffer, const Point3 &impact_pos,
  const Point3 &impact_impulse);
void update_fracture_jobs();
void term_fracture_jobs();

void frx_prepare_destr_input(const rendinst::RendInstBufferData &buffer, const Point3 &impact_pos, FrxDestrInput &out);
bool frx_destroy_ri(frx::DestrContext &ctx, frx::DestrSystem &sys, const FrxDestrInput &input, int &seed);

void frx_preview_select(const rendinst::RendInstBufferData &buffer, const Point3 &impact_pos);
bool frx_preview_intercept_destruction(const rendinst::RendInstBufferData &buffer, rendinst::riex_handle_t destroyed_riex_handle,
  const Point3 &impact_pos);
void frx_preview_clear();
void frx_preview_add_meshes(frx::MeshRenderList &list);

} // namespace rendinstdestr
