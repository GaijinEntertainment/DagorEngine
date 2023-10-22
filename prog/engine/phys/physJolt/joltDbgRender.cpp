#include <phys/dag_physDebug.h>
#include <phys/dag_joltHelpers.h>

#if JPH_DEBUG_RENDERER
#include <Jolt/Renderer/DebugRenderer.h>
#include <math/dag_frustum.h>
#include <debug/dag_debug3d.h>

static const Point3 *camPos = nullptr;
static const Frustum *frustum = nullptr;
static float renderDistSq = SQR(1000.0f);
static bool forcedBufferedDebugDrawMode = false;

class JoltDebugRendererImp final : public JPH::DebugRenderer
{
public:
  struct BatchImpl : public JPH::RefTargetVirtual
  {
    Tab<Vertex> vert;
    Tab<JPH::uint32> indx;
    int mRefCount = 0;

    BatchImpl(const Vertex *inVert, int inVertCount, const JPH::uint32 *inIdx, int inIdxCount)
    {
      vert = make_span_const(inVert, inVertCount);
      if (inIdx)
        indx = make_span_const(inIdx, inIdxCount);
      else
      {
        indx.resize(inIdxCount);
        for (int i = 0; i < inIdxCount; i++)
          indx[i] = i;
      }
    }
    void AddRef() override { mRefCount++; }
    void Release() override
    {
      if (--mRefCount == 0)
        delete this;
    }
  };

  JoltDebugRendererImp() { Initialize(); }

  /// Implementation of DebugRenderer interface
  void DrawLine(JPH::RVec3 inFrom, JPH::RVec3 inTo, JPH::ColorArg inColor) override
  {
    if (forcedBufferedDebugDrawMode)
      draw_debug_line_buffered(to_point3(inFrom), to_point3(inTo), E3DCOLOR(inColor.r, inColor.g, inColor.b, inColor.a));
    else
      draw_cached_debug_line(to_point3(inFrom), to_point3(inTo), E3DCOLOR(inColor.r, inColor.g, inColor.b, inColor.a));
  }
  void DrawTriangle(JPH::Vec3Arg inV1, JPH::Vec3Arg inV2, JPH::Vec3Arg inV3, JPH::ColorArg inColor, ECastShadow) override {}
  Batch CreateTriangleBatch(const Triangle *inTriangles, int inTriangleCount) override
  {
    return new BatchImpl(inTriangles->mV, inTriangleCount * 3, nullptr, inTriangleCount * 3);
  }
  Batch CreateTriangleBatch(const Vertex *inVert, int inVertCount, const JPH::uint32 *inIdx, int inIdxCount) override
  {
    return new BatchImpl(inVert, inVertCount, inIdx, inIdxCount);
  }
  void DrawGeometry(JPH::Mat44Arg inModelMatrix, const JPH::AABox &inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor,
    const GeometryRef &inGeometry, ECullMode inCullMode, ECastShadow inCastShadow, EDrawMode inDrawMode) override
  {
    if (camPos && inWorldSpaceBounds.GetSqDistanceTo(to_jVec3(*camPos)) > renderDistSq)
      return;
    if (frustum && !frustum->testBoxB(v_ld((float *)&inWorldSpaceBounds.mMin), v_ld((float *)&inWorldSpaceBounds.mMax)))
      return;
    const BatchImpl &b = *static_cast<const BatchImpl *>(inGeometry->mLODs[0].mTriangleBatch.GetPtr());
    for (int fi = 0; fi < b.indx.size(); fi += 3)
    {
      JPH::Float3 v0, v1, v2;
      (inModelMatrix * JPH::Vec3(b.vert[b.indx[fi + 0]].mPosition)).StoreFloat3(&v0);
      (inModelMatrix * JPH::Vec3(b.vert[b.indx[fi + 1]].mPosition)).StoreFloat3(&v1);
      (inModelMatrix * JPH::Vec3(b.vert[b.indx[fi + 2]].mPosition)).StoreFloat3(&v2);
      JPH::Color c0 = b.vert[b.indx[fi + 0]].mColor;
      JPH::Color c1 = b.vert[b.indx[fi + 1]].mColor;
      JPH::Color c2 = b.vert[b.indx[fi + 2]].mColor;
      if (forcedBufferedDebugDrawMode)
      {
        draw_debug_line_buffered(Point3::xyz(v0), Point3::xyz(v1), E3DCOLOR(c0.r, c0.g, c0.b, c0.a));
        draw_debug_line_buffered(Point3::xyz(v1), Point3::xyz(v2), E3DCOLOR(c1.r, c1.g, c1.b, c1.a));
        draw_debug_line_buffered(Point3::xyz(v2), Point3::xyz(v0), E3DCOLOR(c2.r, c2.g, c2.b, c2.a));
      }
      else
      {
        draw_cached_debug_line(Point3::xyz(v0), Point3::xyz(v1), E3DCOLOR(c0.r, c0.g, c0.b, c0.a));
        draw_cached_debug_line(Point3::xyz(v1), Point3::xyz(v2), E3DCOLOR(c1.r, c1.g, c1.b, c1.a));
        draw_cached_debug_line(Point3::xyz(v2), Point3::xyz(v0), E3DCOLOR(c2.r, c2.g, c2.b, c2.a));
      }
    }
    // draw_debug_solid_mesh
  }
  void DrawText3D(JPH::Vec3Arg inPosition, const JPH::string_view &inString, JPH::ColorArg inColor, float inHeight) override {}
};

template <>
void physdbg::init<PhysWorld>()
{
  if (!JPH::DebugRenderer::sInstance)
    new JoltDebugRendererImp;
}
template <>
void physdbg::term<PhysWorld>()
{
  if (JPH::DebugRenderer::sInstance)
    delete JPH::DebugRenderer::sInstance;
}
void physdbg::renderWorld(PhysWorld *pw, physdbg::RenderFlags rflg, const Point3 *cam_pos, float render_dist, const Frustum *f)
{
  if (!pw || !JPH::DebugRenderer::sInstance)
    return;

  if (auto *scn = jolt_api::physicsSystem)
    if (auto *dbgRend = JPH::DebugRenderer::sInstance)
    {
      JPH::BodyManager::DrawSettings mBodyDrawSettings;
      mBodyDrawSettings.mDrawBoundingBox = bool(rflg & RenderFlag::BODY_BBOX);
      mBodyDrawSettings.mDrawCenterOfMassTransform = bool(rflg & RenderFlag::BODY_CENTER);
      mBodyDrawSettings.mDrawGetSupportingFace = bool(rflg & RenderFlag::CONTACT_POINTS);
      float prev_dist_sq = ::renderDistSq;
      ::camPos = cam_pos;
      ::frustum = f;
      ::renderDistSq = SQR(render_dist);

      ::begin_draw_cached_debug_lines((rflg & RenderFlag::USE_ZTEST) ? true : false, false);
      if (rflg & RenderFlag::BODIES)
        scn->DrawBodies(mBodyDrawSettings, dbgRend);
      if (rflg & RenderFlag::CONSTRAINTS)
        scn->DrawConstraints(dbgRend);
      if (rflg & RenderFlag::CONSTRAINT_LIMITS)
        scn->DrawConstraintLimits(dbgRend);
      if (rflg & RenderFlag::CONSTRAINT_REFSYS)
        scn->DrawConstraintReferenceFrame(dbgRend);
      ::end_draw_cached_debug_lines();
      ::renderDistSq = prev_dist_sq;
      ::camPos = nullptr;
      ::frustum = nullptr;
    }
}
void physdbg::renderOneBody(PhysWorld *pw, const PhysBody *pb, RenderFlags rflg, unsigned col)
{
  physdbg::renderOneBody(pw, pb, to_tmatrix(jolt_api::body_api().GetWorldTransform(pb->bodyId)), rflg, col);
}
void physdbg::renderOneBody(PhysWorld *pw, const PhysBody *pb, const TMatrix &tm, RenderFlags rflg, unsigned col)
{
  auto *scn = jolt_api::physicsSystem;
  auto *dbgRend = JPH::DebugRenderer::sInstance;
  if (!scn || !dbgRend)
    return;

  bool pmode = forcedBufferedDebugDrawMode;
  forcedBufferedDebugDrawMode = true;

  pb->lockRO([&](const JPH::Body &body) {
    JPH::Color color((col >> 16) & 0xFF, (col >> 8) & 0xFF, (col)&0xFF);

    if (rflg & RenderFlag::BODY_BBOX)
      dbgRend->DrawWireBox(body.GetWorldSpaceBounds(), color);

    if (rflg & RenderFlag::BODY_CENTER)
      dbgRend->DrawCoordinateSystem(to_jMat44(tm), 0.2f);

    auto shape = body.GetShape();
    if (shape->GetSubType() != JPH::EShapeSubType::HeightField)
      shape->Draw(dbgRend, to_jMat44(tm).PreTranslated(shape->GetCenterOfMass()), JPH::Vec3::sReplicate(1.0f), color, false, true);
  });
  forcedBufferedDebugDrawMode = pmode;
}

void physdbg::setBufferedForcedDraw(PhysWorld *pw, bool forced)
{
  if (pw)
    forcedBufferedDebugDrawMode = forced;
}
bool physdbg::isInDebugMode(PhysWorld *pw) { return JPH::DebugRenderer::sInstance && forcedBufferedDebugDrawMode; }
#else
template <>
void physdbg::init<PhysWorld>()
{}
template <>
void physdbg::term<PhysWorld>()
{}
void physdbg::renderWorld(PhysWorld *, physdbg::RenderFlags, const Point3 *cam_pos, float render_dist, const Frustum *f) {}
void physdbg::renderOneBody(PhysWorld *, const PhysBody *, RenderFlags, unsigned) {}
void physdbg::renderOneBody(PhysWorld *, const PhysBody *, const TMatrix &, RenderFlags, unsigned) {}

void physdbg::setBufferedForcedDraw(PhysWorld *, bool) {}
bool physdbg::isInDebugMode(PhysWorld *) { return false; }
#endif
