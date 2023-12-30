#include <render/waterRipples.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <shaders/dag_shaders.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>

static const float STEP_DT = 1.0f / 60.0f;
static const float STEP_HYSTERESIS = 0.75f;
static const float STEP_0_HYSTERESIS = 0.9f;
static const float STEP_1_HYSTERESIS = 0.75f;
static const float MIN_JITTER_THRESHOLD = 0.1f;
static const float MAX_STEPS_NUM = 2.0f;
static const float MAX_DROPS_ALIVE_TIME = 10.0f;
static const int MAX_DROPS_PER_BATCH = 53;
static const int DROPS_REGISTER_NO = 21;


WaterRipples::WaterRipples(float world_size, int tex_size) :
  worldSize(world_size), texSize(tex_size), steps(), texBuffers(), drops(midmem)
{
  drops.reserve(MAX_DROPS_PER_BATCH);
  texBuffers[0] = dag::create_tex(NULL, texSize, texSize, TEXFMT_G16R16F | TEXCF_RTARGET, 1, "water_ripples_0");
  texBuffers[1] = dag::create_tex(NULL, texSize, texSize, TEXFMT_G16R16F | TEXCF_RTARGET, 1, "water_ripples_1");
  texBuffers[0]->texfilter(TEXFILTER_SMOOTH);
  texBuffers[0]->texaddr(TEXADDR_CLAMP);
  texBuffers[1]->texfilter(TEXFILTER_SMOOTH);
  texBuffers[1]->texaddr(TEXADDR_CLAMP);

  normalTexture = dag::create_tex(NULL, texSize, texSize, TEXFMT_R8G8 | TEXCF_RTARGET, 1, "water_ripples_normal");
  normalTexture->texfilter(TEXFILTER_SMOOTH);
  normalTexture->texaddr(TEXADDR_CLAMP);

  updateRenderer.init("water_ripples_update");

  waterRipplesOnVarId = ::get_shader_glob_var_id("water_ripples_on", true);
  ShaderGlobal::set_real(waterRipplesOnVarId, 0.0f);

  ShaderGlobal::set_real(::get_shader_glob_var_id("water_ripples_pixel_size"), 1.0f / texSize);
  ShaderGlobal::set_real(::get_shader_glob_var_id("water_ripples_size"), worldSize);
}

WaterRipples::~WaterRipples() { ShaderGlobal::set_real(waterRipplesOnVarId, 0.0f); }

void WaterRipples::placeDrop(const Point2 &pos, float strength, float radius)
{
  G_ASSERT_RETURN(radius > 0.0f, );

  Point4 &drop = drops.push_back();
  drop = Point4(pos.x, pos.y, radius / texSize, strength);
  addDropInst(1);
}

void WaterRipples::placeSolidBox(const Point2 &pos, const Point2 &size, const Point2 &local_x, const Point2 &local_y, float strength,
  float radius)
{
  G_ASSERT(radius > 0.0f);

  Point4 &drop = drops.push_back();
  drop = Point4(pos.x, pos.y, -radius / texSize, strength);
  Point4 &localXY = drops.push_back();
  localXY = Point4(local_x.x * size.x, local_x.y * size.x, local_y.x * size.y, local_y.y * size.y) * 0.5f / worldSize;
  addDropInst(2);
}

// Corrected means the radius should be divided by the world size, instead of the texture size.
// Cant change this in the original functions as that would affect WT, and as the values were
// set up with this error in mind, it would mess up WT ripples.
// This should be fixen in WT later.
void WaterRipples::placeDropCorrected(const Point2 &pos, float strength, float radius)
{
  placeDrop(pos, strength, (radius / worldSize) * texSize);
}

void WaterRipples::placeSolidBoxCorrected(const Point2 &pos, const Point2 &size, const Point2 &local_x, const Point2 &local_y,
  float strength, float radius)
{
  placeSolidBox(pos, size, local_x, local_y, strength, (radius / worldSize) * texSize);
}

void WaterRipples::reset()
{
  firstStep = true;
  dropsAliveTime = 0;
  curBuffer = 0;
  mem_set_0(steps);
  curStepNo = 0;
  accumStep = 0.0f;
  curNumSteps = 1;
}

void WaterRipples::updateSteps(float dt)
{
  if (inSleep)
  {
    curNumSteps = 1;
    nextNumSteps = 1;
    accumStep = 0.0f;
    curStepNo = 0;
    return;
  }

  steps[(curStepNo++) % steps.size()] = dt;
  float stepAvg = 0.0f;
  for (int stepNo = 0; stepNo < steps.size(); ++stepNo)
    stepAvg += steps[stepNo];
  stepAvg /= steps.size();

  float fNumSteps = min(stepAvg / STEP_DT, MAX_STEPS_NUM);
  if (curNumSteps == 0 && fNumSteps > STEP_0_HYSTERESIS)
  {
    curNumSteps = 1;
    accumStep = 0.0f;
  }
  else if (curNumSteps == 1 && fNumSteps < STEP_1_HYSTERESIS)
  {
    curNumSteps = 0;
    accumStep = 1.0f;
  }
  else if (fabsf((float)curNumSteps - fNumSteps) > STEP_HYSTERESIS)
  {
    curNumSteps = floorf(fNumSteps + 0.5f);
    accumStep = 0.0f;
  }

  nextNumSteps = curNumSteps;
  if (nextNumSteps == 0)
  {
    accumStep += fNumSteps;
    nextNumSteps = floorf(accumStep);
    accumStep -= nextNumSteps;
  }
}

void WaterRipples::advance(float dt, const Point2 &origin)
{
  static int waterRipplesInfoTextureVarId = ::get_shader_glob_var_id("water_ripples_info_texture");
  static int waterRipplesDropCountVarId = ::get_shader_glob_var_id("water_ripples_drop_count");
  static int waterRipplesOriginVarId = ::get_shader_glob_var_id("water_ripples_origin");
  static int waterRipplesOriginDeltaVarId = ::get_shader_glob_var_id("water_ripples_origin_delta");
  static int waterRipplesDisplaceMaxVarId = ::get_shader_glob_var_id("water_ripples_displace_max", true);

  const bool cleared = firstStep;
  if (firstStep)
  {
    firstStep = false;
    clearRts();
  }

  dropsAliveTime = drops.size() > 0 ? MAX_DROPS_ALIVE_TIME : dropsAliveTime;
  if (dropsAliveTime <= 0.0f)
  {
    if (!inSleep)
    {
      inSleep = true;
      if (!cleared)
        clearRts();
      dropsAliveTime = 0.0f;
      ShaderGlobal::set_real(waterRipplesOnVarId, 0.0f);
    }
    return;
  }
  else if (inSleep)
  {
    inSleep = false;
    ShaderGlobal::set_real(waterRipplesOnVarId, 1.0f);
  }
  dropsAliveTime -= dt;

  ShaderGlobal::set_color4(waterRipplesOriginVarId, Color4(origin.x, 0.0f, origin.y, 0.0f));
  normalTexture.setVar();

  SCOPE_RENDER_TARGET;

  for (int stepNo = 0; stepNo < nextNumSteps; ++stepNo)
  {
    Point2 originDelta(0.0f, 0.0f);
    if (stepNo == 0)
    {
      originDelta = (origin - lastStepOrigin) / worldSize;
      lastStepOrigin = origin;
    }

    if (dropsInst.size() > 0)
    {
      d3d::set_ps_const(DROPS_REGISTER_NO, (float *)drops.data(), dropsInst[0]);
      ShaderGlobal::set_int(waterRipplesDropCountVarId, dropsInst[0]);
      erase_items(drops, 0, dropsInst[0]);
      erase_items(dropsInst, 0, 1);
    }
    else
      ShaderGlobal::set_int(waterRipplesDropCountVarId, 0);

    if (lengthSq(originDelta) < sqr(MIN_JITTER_THRESHOLD / texSize))
      originDelta = Point2(0.25f, 0.25f) / texSize * (curBuffer ? 1.0f : -1.0f);
    ShaderGlobal::set_color4(waterRipplesOriginDeltaVarId, Color4::xyz0(Point3::x0y(originDelta)));

    texBuffers[curBuffer]->texaddr(TEXADDR_CLAMP);
    ShaderGlobal::set_texture(waterRipplesInfoTextureVarId, texBuffers[curBuffer].getTexId());

    d3d::set_render_target(0, texBuffers[1 - curBuffer].getTex2D(), 0);
    d3d::set_render_target(1, normalTexture.getTex2D(), 0);
    updateRenderer.render();
    d3d::resource_barrier({texBuffers[1 - curBuffer].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    d3d::resource_barrier({normalTexture.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

    texBuffers[curBuffer]->texaddr(TEXADDR_BORDER);
    texBuffers[curBuffer]->texbordercolor(0);
    curBuffer = 1 - curBuffer;
  }

  ShaderGlobal::set_real(waterRipplesDisplaceMaxVarId, 0.125f);
}

void WaterRipples::addDropInst(int reg_num)
{
  if (dropsInst.size() == 0 || (dropsInst.back() + reg_num) > MAX_DROPS_PER_BATCH)
    dropsInst.push_back(0);
  dropsInst.back() += reg_num;
}

void WaterRipples::clearRts()
{
  SCOPE_RENDER_TARGET;
  d3d::set_render_target(0, texBuffers[0].getTex2D(), 0);
  d3d::set_render_target(1, texBuffers[1].getTex2D(), 0);
  d3d::clearview(CLEAR_TARGET, 0, 0, 0);
  d3d::set_render_target(normalTexture.getTex2D(), 0);
  d3d::clearview(CLEAR_TARGET, E3DCOLOR(127, 127, 0), 0, 0);
  d3d::resource_barrier({texBuffers[0].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  d3d::resource_barrier({texBuffers[1].getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  d3d::resource_barrier({normalTexture.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
}