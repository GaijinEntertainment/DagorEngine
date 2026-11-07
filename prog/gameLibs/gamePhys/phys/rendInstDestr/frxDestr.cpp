// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frxDestr.h"

#include <EASTL/unique_ptr.h>
#include <dag/dag_vector.h>
#include <math/dag_mathUtils.h>
#include <math/dag_plane3.h>
#include <math/dag_TMatrix.h>
#include <math/random/dag_random.h>
#include <vecmath/dag_vecMath.h>
#include <debug/dag_debug3d.h>
#include <generic/dag_span.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_statDrv.h>
#include <phys/dag_physics.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_shaderMesh.h>
#include <util/dag_convar.h>
#include <util/dag_threadPool.h>

#include <daFracture/core/meshCommon.h>
#include <daFracture/core/meshSlicing.h>
#include <daFracture/render/renderMesh.h>
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/phys/destructableObject.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>

#include "riDestrInternal.h"
#include "riDestrProps.h"


namespace convar
{
CONSOLE_BOOL_VAL("ridestr", ri_debug_fracture_slicing, false);
}

static frx::ShatterImpactProfile make_shatter_impact_profile(const Point3 &impact_pos)
{
  frx::ShatterImpactProfile impactProfile;
  impactProfile.pos = impact_pos;
  // TODO: pass more parameters from gameplay logic to compute falloff curve
  impactProfile.radiusRange = Point2(0.f, 2.f);
  impactProfile.powerRange = Point2(1.f, 0.f);
  impactProfile.falloffPow = 3.f;
  if (impactProfile.pos == Point3::ZERO)
    impactProfile.powerRange = Point2::ZERO;
  return impactProfile;
}

void rendinstdestr::frx_prepare_destr_input(const rendinst::RendInstBufferData &buffer, const Point3 &impact_pos, FrxDestrInput &out)
{
  const RiDestrProps *props = get_ri_destr_props(buffer.desc);
  out.buffer = buffer;
  out.frxProps = props ? props->frxDestr : nullptr;
  out.impact = make_shatter_impact_profile(impact_pos);
  out.meshes.clear();
  const RenderableInstanceLodsResource *res = rendinst::getRIGenExtraRes(buffer.desc.pool);
  G_ASSERT_RETURN(res, );
  out.meshes.push_back({.res = res, .tm = TMatrix::IDENT, .seed = uint32_t(buffer.riExUserData[0])});

  if (!props)
    return;
  TMatrix riTm;
  v_mat_43cu_from_mat44(riTm.array, buffer.tm);
  const TMatrix riItm = inverse(riTm);
  for_each_nearby_ri(props->nearbyGroups, {.frxInclude = 1}, buffer,
    [&](rendinst::riex_handle_t handle, RiNearbyGroupsProps::Actions group_actions) {
      mat44f tm44;
      rendinst::getRIGenExtra44(handle, tm44);
      TMatrix tm;
      v_mat_43cu_from_mat44(tm.array, tm44);
      out.meshes.push_back({.res = rendinst::getRIGenExtraRes(rendinst::handle_to_ri_type(handle)),
        .tm = riItm * tm,
        .seed = rendinst::get_riextra_instance_seed(handle),
        .invertWinding = group_actions.frxInvert != 0});
    });
}

bool rendinstdestr::frx_destroy_ri(frx::DestrContext &ctx, frx::DestrSystem &sys, const FrxDestrInput &input, int &seed)
{
  G_ASSERT_RETURN(input.frxProps, false);
  const FrxDestrProps &frxProps = *input.frxProps;

  TMatrix riTm;
  v_mat_43cu_from_mat44(riTm.array, input.buffer.tm);

  frx::DestrMesh riMesh;
  dag::Vector<frx::ShaderGeomLoadRequest, framemem_allocator> geomLoadReqs;
  for (const FrxDestrInput::Mesh &mesh : input.meshes)
  {
    const dag::ConstSpan<RenderableInstanceLodsResource::Lod> lods = mesh.res->getUsedLods();
    G_ASSERT_CONTINUE(!lods.empty() && lods[0].scene != nullptr && lods[0].scene->getMesh() != nullptr);
    const TMatrix worldTm = riTm * mesh.tm;
    frx::PerInstRenderData instData;
    instData.basePosAndHash = v_perm_xyzd(v_ldu(&worldTm.getcol(3).x), v_cast_vec4f(v_splatsi(mesh.seed)));
    geomLoadReqs.push_back({.outMesh = &riMesh,
      .smvd = mesh.res->getSmvd()[0],
      .shaderMesh = lods[0].scene->getMesh()->getMesh()->getMesh(),
      .tm = mesh.tm,
      .instData = instData,
      .invertWinding = mesh.invertWinding});
  }
  frx::add_destr_geometry(ctx, make_span(geomLoadReqs));
  riMesh.tm = riTm;
  if (riMesh.faces.empty())
    return false;

  int16_t interiorMatId = -1;
  if (const RiDestrMaterial *interiorMat = frxProps.interiorMat)
    interiorMatId = frx::add_render_material(ctx, interiorMat->shMat,
      {.shElem = interiorMat->shElem, .stage = ShaderMesh::STG_opaque, .instData = {}});

  // cut off rendinst bottom and discard it
  {
    frx::mesh_slice(ctx, riMesh,
      {
        .upMesh = &sys.pieces[0],
        .cutPlane = frx::PlaneBasis(Plane3(Point3(0, 1, 0), riMesh.tm.getcol(3) + Point3(0, 0.05f, 0))),
        .cutMatId = interiorMatId,
      });
    if (sys.pieces[0].faces.empty())
      return false;
  }
  frx::shatter_into_pieces(ctx, sys, /* start piece */ 0, interiorMatId, input.impact, frxProps.shatterProfile, seed);
  return true;
}

namespace rendinstdestr
{

struct AsyncFractureJob : cpujobs::IJob
{
  volatile int isTermRequested = 0;
  int seed = grnd();

  rendinst::riex_handle_t sentinelHandle = rendinst::RIEX_HANDLE_NULL;
  FrxDestrInput input;
  destructables::DestructableCreationParams destr;

  Point3 impactImpulse = Point3::ZERO;
  PhysMat::MatID physMatId = PHYSMAT_DEFAULT;

  const char *getJobName(bool &) const override { return DAPROFILER_STRING("AsyncFractureJob"); }

  void doJob() override { doAsyncFracture(); }

  void doAsyncFracture()
  {
    FRAMEMEM_REGION;

    if (interlocked_relaxed_load(isTermRequested))
      return;

    frx::DestrContext ctx;
    ctx.dbgDraw.drawCutEdgeGraph = convar::ri_debug_fracture_slicing;
    ctx.dbgDraw.heightScale = 1.f;
    frx::DestrSystem sys;
    if (!frx_destroy_ri(ctx, sys, input, seed))
      return;

    if (interlocked_relaxed_load(isTermRequested))
      return;

    auto *physWorld = dacoll::get_phys_world();
    dag::Vector<frx::MeshUploadRequest> meshesToUpload;
    meshesToUpload.reserve(sys.pieces.size());

    {
      TIME_PROFILE(create_fracture_bodies)
      dag::Vector<Point3, framemem_allocator> convexHull;
      for (auto &[id, piece] : sys.pieces)
      {
        G_ASSERT_CONTINUE(!piece.faces.empty());
        auto &obj = destr.fracturePhysObjects.push_back();
        obj.prevTm = obj.tm = piece.tm;

        PhysBodyCreationData pbcd;
        pbcd.materialId = PhysMat::getPhysBodyMaterial(physMatId);
        pbcd.autoInertia = true;
        pbcd.addToWorld = false;
        pbcd.group = destructables::DEFAULT_PHYS_GROUP;
        pbcd.mask = destructables::DEFAULT_PHYS_MASK;
        frx::mesh_prepare_convex_hull(ctx, piece, 0.1f, convexHull);
        if (!convexHull.empty())
        {
          PhysConvexHullCollision hullColl(convexHull, true, 0.01f);
          obj.body.reset(new PhysBody(physWorld, 1.f, &hullColl, obj.tm, pbcd));
        }
        else
        {
          PhysBoxCollision boxColl(0.1f, 0.1f, 0.1f);
          obj.body.reset(new PhysBody(physWorld, 1.f, &boxColl, obj.tm, pbcd));
        }
        obj.mesh = eastl::make_unique<frx::RenderMesh>();
        if (!obj.body->isValid())
        {
          destr.fracturePhysObjects.pop_back();
          continue;
        }
        meshesToUpload.push_back({&piece, obj.mesh.get()});
      }
    }
    frx::upload_mesh_batch(ctx, make_span(meshesToUpload));
  }

  void onDoneSync()
  {
    rendinst::delRIGenExtra(sentinelHandle);

    if (isTermRequested)
      return;
    if (destr.fracturePhysObjects.empty())
      return;

    TMatrix riTm;
    v_mat_43cu_from_mat44(riTm.array, input.buffer.tm);
    bbox3f lbb = rendinst::riex_get_lbb(input.buffer.desc.pool);
    BBox3 riLocalBB;
    v_stu_bbox3(riLocalBB, lbb);

    PhysWorld *physWorld = dacoll::get_phys_world();
    for (auto &obj : destr.fracturePhysObjects)
    {
      PhysBody *body = obj.body.get();
      physWorld->addBody(body, false);
    }

    gamephys::DestructableObject *destrPtr = nullptr;
    destructables::addDestructable(&destrPtr, eastl::move(destr), physWorld);
    if (destrPtr)
      destrPtr->setupInitialPhysState(riTm, riLocalBB, input.impact.pos, impactImpulse);

    queue_ri_collision_for_debris_if_enabled(input.buffer.tm, lbb);
  }

  ~AsyncFractureJob() override = default;
};

static struct AsyncFractureJobMgr
{
  dag::Vector<AsyncFractureJob *> jobs;

  void addJob(AsyncFractureJob *job)
  {
    G_ASSERT(is_main_thread());
    threadpool::add(job, threadpool::PRIO_LOW);
    jobs.push_back(job);
  }

  void updateDoneJobs()
  {
    G_ASSERT(is_main_thread());
    for (auto it = jobs.begin(); it != jobs.end();)
    {
      AsyncFractureJob *job = *it;
      if (interlocked_acquire_load(job->done))
      {
        job->onDoneSync();
        delete job;
        it = jobs.erase_unsorted(it);
      }
      else
        ++it;
    }
  }

  void termAndWait()
  {
    G_ASSERT(is_main_thread());
    for (AsyncFractureJob *job : jobs)
      interlocked_release_store(job->isTermRequested, 1);
    for (AsyncFractureJob *job : jobs)
    {
      threadpool::wait(job);
      job->onDoneSync(); // do it even on termination to delete sentinel RI
      delete job;
    }
    clear_and_shrink(jobs);
  }

  ~AsyncFractureJobMgr() { termAndWait(); }
} g_async_fracture_jobs;

} // namespace rendinstdestr

bool rendinstdestr::perform_procedural_destruction(const rendinst::RendInstBufferData &buffer, const Point3 &impact_pos,
  const Point3 &impact_impulse)
{
  if (!buffer.desc.isRiExtra())
    return false;

  const RiDestrProps *props = get_ri_destr_props(buffer.desc);
  const FrxDestrProps *frxDestr = props ? props->frxDestr : nullptr;
  if (!frxDestr || !props || !(frxDestr->flags & FrxDestrFlag::Shatter))
    return false;

  {
    static float last_dbg_draw_time = 0.f;
    if (last_dbg_draw_time + 5.f < get_sync_time() || convar::ri_debug_fracture_slicing.pullValueChange())
      clear_buffered_debug_lines();
    last_dbg_draw_time = get_sync_time();
  }

  AsyncFractureJob *job = new AsyncFractureJob();
  frx_prepare_destr_input(buffer, impact_pos, job->input);
  job->destr.destrObjProps = props->destrObj;
  job->impactImpulse = impact_impulse;
  if (const auto physMatId = get_destructable_physmat_for_ri(buffer.desc); physMatId != PHYSMAT_INVALID)
    job->physMatId = physMatId;
  job->sentinelHandle = rendinst::addRIGenExtra44(buffer.desc.pool, buffer.tm, false /*has_collision*/, -1, -1,
    buffer.riExUserData[15], buffer.riExUserData.data());
  g_async_fracture_jobs.addJob(job);

  return true;
}

void rendinstdestr::update_fracture_jobs() { g_async_fracture_jobs.updateDoneJobs(); }

void rendinstdestr::term_fracture_jobs()
{
  g_async_fracture_jobs.termAndWait();
  frx_preview_clear();
}
