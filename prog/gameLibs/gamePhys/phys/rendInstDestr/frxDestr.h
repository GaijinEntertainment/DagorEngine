// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point3.h>
#include <rendInst/riexHandle.h>


class RenderableInstanceLodsResource;
namespace rendinst
{
struct RendInstBufferData;
}
namespace frx
{
struct DestrContext;
struct DestrSystem;
struct ShatterImpactProfile;
struct MeshRenderList;
} // namespace frx

namespace rendinstdestr
{

struct FrxDestrProps;

bool perform_procedural_destruction(const rendinst::RendInstBufferData &buffer, const Point3 &impact_pos,
  const Point3 &impact_impulse);
void update_fracture_jobs();
void term_fracture_jobs();

frx::ShatterImpactProfile make_shatter_impact_profile(const Point3 &impact_pos);
bool frx_destroy_ri(frx::DestrContext &ctx, frx::DestrSystem &sys, const RenderableInstanceLodsResource *res,
  const rendinst::RendInstBufferData &buffer, const FrxDestrProps &frx_destr, const frx::ShatterImpactProfile &impact, int &seed);

void frx_preview_select(const rendinst::RendInstBufferData &buffer, const Point3 &impact_pos);
bool frx_preview_intercept_destruction(const rendinst::RendInstBufferData &buffer, rendinst::riex_handle_t destroyed_riex_handle,
  const Point3 &impact_pos);
void frx_preview_clear();
void frx_preview_add_meshes(frx::MeshRenderList &list);

} // namespace rendinstdestr
