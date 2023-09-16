#include <phys/dag_physDebug.h>
#include <debug/dag_debug3d.h>

class BulletDbgRender : public btIDebugDraw
{
  int m_debugMode = 0;
  bool non_buffered = true;

public:
  const Point3 *camPos = nullptr;
  float renderDistSq = SQR(1000.0f);

public:
  void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) override
  {
    if (camPos && lengthSq(to_point3(from) - *camPos) > renderDistSq && lengthSq(to_point3(to) - *camPos) > renderDistSq)
      return;
    if (non_buffered)
      draw_cached_debug_line(to_point3(from), to_point3(to), col(color));
    else
      draw_debug_line_buffered(to_point3(from), to_point3(to), col(color));
  }
  void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &fromColor, const btVector3 & /*toColor*/) override
  {
    drawLine(from, to, fromColor);
  }

  void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime,
    const btVector3 &color) override
  {}

  void reportErrorWarning(const char *warningString) override { logwarn("BULLET: %s", warningString); }
  void draw3dText(const btVector3 &location, const char *textString) override {}

  void setDebugMode(int debugMode) override { m_debugMode = debugMode; }
  bool setDebugMode(int dm, bool nb)
  {
    bool pnb = non_buffered;
    m_debugMode = dm;
    non_buffered = nb;
    return pnb;
  }
  int getDebugMode() const override { return m_debugMode; }

  E3DCOLOR col(const btVector3 &v) { return E3DCOLOR(v.x() * 255, v.y() * 255, v.z() * 255); }
} bullet_dbg_render;

template <>
void physdbg::init<PhysWorld>()
{
  bullet_dbg_render.setDebugMode(btIDebugDraw::DBG_DrawWireframe);
}
template <>
void physdbg::term<PhysWorld>()
{}

void physdbg::renderWorld(PhysWorld *pw, physdbg::RenderFlags rflg, const Point3 *cam_pos, float render_dist, const Frustum *)
{
  if (!pw)
    return;
  pw->fetchSimRes(/*wait */ true);
  if (auto *scn = pw->getScene())
  {
    int dbgm0 = bullet_dbg_render.getDebugMode(), dbgm = dbgm0;
    if (rflg & RenderFlag::CONSTRAINTS)
      dbgm |= btIDebugDraw::DBG_DrawConstraints;
    if (rflg & RenderFlag::CONSTRAINT_LIMITS)
      dbgm |= btIDebugDraw::DBG_DrawConstraintLimits;
    if (rflg & RenderFlag::CONTACT_POINTS)
      dbgm |= btIDebugDraw::DBG_DrawContactPoints;
    if (rflg & RenderFlag::BODY_BBOX)
      dbgm |= btIDebugDraw::DBG_DrawAabb;

    bool pnb = bullet_dbg_render.setDebugMode(dbgm, true);
    auto prev_drawer = scn->getDebugDrawer();
    scn->setDebugDrawer(&bullet_dbg_render);

    float prev_dist_sq = bullet_dbg_render.renderDistSq;
    bullet_dbg_render.renderDistSq = SQR(render_dist);
    bullet_dbg_render.camPos = cam_pos;

    ::begin_draw_cached_debug_lines((rflg & RenderFlag::USE_ZTEST) ? true : false, false);
    scn->debugDrawWorld();
    ::end_draw_cached_debug_lines();

    scn->setDebugDrawer(prev_drawer);
    bullet_dbg_render.setDebugMode(dbgm0, pnb);
    bullet_dbg_render.renderDistSq = prev_dist_sq;
    bullet_dbg_render.camPos = nullptr;
  }
}
void physdbg::renderOneBody(PhysWorld *pw, const PhysBody *pb, physdbg::RenderFlags rflg, unsigned col)
{
  physdbg::renderOneBody(pw, pb, to_tmatrix(pb->getActor()->getWorldTransform()), rflg, col);
}
void physdbg::renderOneBody(PhysWorld *pw, const PhysBody *pb, const TMatrix &tm, physdbg::RenderFlags rflg, unsigned col)
{
  if (!pw)
    return;
  if (auto *scn = pw->getScene())
  {
    int dbgm0 = bullet_dbg_render.getDebugMode(), dbgm = dbgm0;
    if (rflg & RenderFlag::CONSTRAINTS)
      dbgm |= btIDebugDraw::DBG_DrawConstraints;
    if (rflg & RenderFlag::CONSTRAINT_LIMITS)
      dbgm |= btIDebugDraw::DBG_DrawConstraintLimits;
    if (rflg & RenderFlag::CONTACT_POINTS)
      dbgm |= btIDebugDraw::DBG_DrawContactPoints;
    if (rflg & RenderFlag::BODY_BBOX)
      dbgm |= btIDebugDraw::DBG_DrawAabb;

    bool pnb = bullet_dbg_render.setDebugMode(dbgm, false);
    auto prev_drawer = scn->getDebugDrawer();
    scn->setDebugDrawer(&bullet_dbg_render);
    scn->debugDrawObject(to_btTransform(tm), pb->getActor()->getCollisionShape(),
      btVector3(((col >> 16) & 0xFF) / 255., ((col >> 8) & 0xFF) / 255., ((col)&0xFF) / 255.));
    scn->setDebugDrawer(prev_drawer);
    bullet_dbg_render.setDebugMode(dbgm0, pnb);
  }
}
void physdbg::setBufferedForcedDraw(PhysWorld *pw, bool forced)
{
  if (!pw)
    return;
  if (auto *scn = pw->getScene())
  {
    if (forced)
    {
      bullet_dbg_render.setDebugMode(btIDebugDraw::DBG_MAX_DEBUG_DRAW_MODE, false);
      scn->setDebugDrawer(&bullet_dbg_render);
    }
    else
    {
      bullet_dbg_render.setDebugMode(0, true);
      scn->setDebugDrawer(nullptr);
    }
  }
}
bool physdbg::isInDebugMode(PhysWorld *pw)
{
  return pw->getScene()->getDebugDrawer() && pw->getScene()->getDebugDrawer()->getDebugMode();
}
