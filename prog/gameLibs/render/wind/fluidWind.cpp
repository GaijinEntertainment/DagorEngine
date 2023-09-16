#include "render/wind/fluidWind.h"
#include <EASTL/utility.h>
#include <3d/dag_drv3d.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_computeShaders.h>
#include <math/dag_mathUtils.h>

static const char *DRAW_MOTOR_SHADERS[FluidWind::MOTOR_TYPE_END] = {
  "fluid_motor_omni", "fluid_motor_vortex", "fluid_motor_directional"};

static const int FLUID_RT_WIDTH = 512;
static const int FLUID_RT_HEIGHT = 512;
static const int DIFFUSE_NUM_ITERATIONS = 5;
static const float MAX_SLEEP_TIME = 1.0f;
static const float FIXED_TIME_STEP = 100;

#define DEBUG_OUTPUT DAGOR_DBGLEVEL > 0


FluidWind::FluidWind(const Desc &in_desc) : desc(in_desc) { init(); }

FluidWind::~FluidWind() { close(); }

static float lilrand() { return (rand() / float(RAND_MAX) - 0.5f) * 2.0f; }

void FluidWind::update(float dt, const Point3 &origin)
{
  totalSimTime += dt;
  Point3 originDelta = toLocalVector(origin - lastOrigin);
  lastOrigin = origin;

  int dx1 = (desc.dimX + 15) / 16;
  int dy1 = desc.dimY / 4;
  int dz1 = desc.dimZ / 4;
  int dx2 = (desc.dimX + 63) / 64;
  int dy2 = desc.dimY / 4;
  int dz2 = desc.dimZ / 4;
  fixedTimeStep = clamp(lerp(fixedTimeStep, dt * FIXED_TIME_STEP, saturate(dt * 0.5f)), 1.0f, FIXED_TIME_STEP);

  ShaderGlobal::set_color4(fluidWindOriginDeltaVarId, Color4(originDelta.x, originDelta.y, originDelta.z, 0));
  ShaderGlobal::set_color4(fluidWindOriginVarId, Color4(lastOrigin.x, lastOrigin.y, lastOrigin.z, 0));
  ShaderGlobal::set_real(fluidWindTimeStepVarId, 1.0f / fixedTimeStep);

  if (!isInSleep())
  {
    // advect
    d3d::set_tex(STAGE_CS, 0, speedTexCur[0]->getVolTex());
    d3d::set_rwtex(STAGE_CS, 0, speedTexCur[1]->getVolTex(), 0, 0);
    advect->dispatch(dx1, dy1, dz1);
    d3d::set_rwtex(STAGE_CS, 0, NULL, 0, 0);
    d3d::set_tex(STAGE_CS, 0, NULL);
    d3d::resource_barrier({speedTexCur[1]->getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  }

  bool hasMotor = false;
  for (int motorNo = 0; motorNo < motors.size(); ++motorNo)
  {
    Motor &motor = motors[motorNo];
    if (motor.stopped)
      continue;
    if (motor.duration && motor.time > motor.duration)
    {
      stopMotor(motorNo);
      continue;
    }
    motor.time += dt;

    if (!isContained(motor.center, motor.radius))
      continue;

    if (hasMotor)
    {
      eastl::swap(speedTexCur[0], speedTexCur[1]);
    }
    // Check for initial clear data
    else if (isInSleep())
    {
      d3d::set_rwtex(STAGE_CS, 0, speedTexCur[1]->getVolTex(), 0, 0);
      d3d::set_rwtex(STAGE_CS, 2, pressureTexCur[1]->getVolTex(), 0, 0);
      clearData->dispatch(dx1, dy1, dz1);
      d3d::set_rwtex(STAGE_CS, 0, NULL, 0, 0);
      d3d::set_rwtex(STAGE_CS, 2, NULL, 0, 0);
      d3d::resource_barrier({speedTexCur[1]->getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
      d3d::resource_barrier({pressureTexCur[1]->getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    }
    hasMotor = true;

    Point3 centerLocal = toLocalPoint(motor.center);
    if (motor.shake)
      centerLocal += Point3(lilrand(), lilrand(), lilrand());
    Point3 radiusLocal = toLocalVector(Point3(motor.radius, motor.radius, motor.radius));
    float radiusLocalMax = max(radiusLocal.x, max(radiusLocal.y, radiusLocal.z));
    Point3 directionLocal = normalize(toLocalVector(motor.direction));
    Point3 strengthLocal = toLocalVector(Point3(motor.strength, motor.strength, motor.strength));
    float strengthLocalMax = max(strengthLocal.x, max(strengthLocal.y, strengthLocal.z));

    float phase = 1.0f;
    if (motor.phaseFade.enabled)
    {
      // return -cos(time / duration * 2 * PI) * 0.5 + 0.5;
      // return 1.0 - abs(cos(time / duration * PI));
      float fade = min(motor.duration * 0.5f, motor.phaseFade.maxFadeTime);
      phase *= motor.duration > 0.001 ? (1.0 /*- SQR(1.0 - saturate(motor.time / fade))*/) -
                                          (1.0 - SQR(1.0 - saturate(motor.time - motor.duration + fade) / fade))
                                      : 1;
    }
    if (motor.phaseSin.enabled && motor.duration > 0.001)
    {
      phase *= sin(motor.time / motor.duration * motor.phaseSin.numWaves * PI);
      phase *= SQR(1.0 - motor.time / motor.duration);
    }

    ShaderGlobal::set_color4(fluidMotorCenterRadiusVarId, Color4(centerLocal.x, centerLocal.y, centerLocal.z, radiusLocalMax));
    ShaderGlobal::set_color4(fluidMotorDirectionDurationVarId,
      Color4(directionLocal.x, directionLocal.y, directionLocal.z, motor.duration));
    ShaderGlobal::set_real(fluidMotorStrengthVarId, strengthLocalMax * phase);
    ShaderGlobal::set_real(fluidMotorTimeVarId, motor.duration > 0 ? min(motor.time, motor.duration) : motor.time);

    // draw source
    d3d::set_tex(STAGE_CS, 0, speedTexCur[1]->getVolTex());
    d3d::set_rwtex(STAGE_CS, 0, speedTexCur[0]->getVolTex(), 0, 0);
    // d3d::set_tex(STAGE_CS, 2, pressureTexCur[1]->getVolTex());
    // d3d::set_rwtex(STAGE_CS, 2, pressureTexCur[0]->getVolTex(), 0, 0);
    drawMotors[motor.type]->dispatch(dx1, dy1, dz1);
    d3d::set_rwtex(STAGE_CS, 0, NULL, 0, 0);
    d3d::set_tex(STAGE_CS, 0, NULL);
    d3d::resource_barrier({speedTexCur[0]->getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    // d3d::set_rwtex(STAGE_CS, 2, NULL, 0, 0);
    // d3d::set_tex(STAGE_CS, 2, NULL);
    // eastl::swap(pressureTexCur[0], pressureTexCur[1]);
  }

  if (hasMotor)
    sleepTime = MAX_SLEEP_TIME;
  else if (!isInSleep())
  {
    sleepTime = max(sleepTime - dt, 0.0f);
    eastl::swap(speedTexCur[0], speedTexCur[1]);
  }

  if (!isInSleep())
  {
    // calc speed divergence
    d3d::set_tex(STAGE_CS, 0, speedTexCur[0]->getVolTex());
    d3d::set_rwtex(STAGE_CS, 1, divergenceTex.getVolTex(), 0, 0);
    divergence->dispatch(dx2, dy2, dz2);
    d3d::set_rwtex(STAGE_CS, 1, NULL, 0, 0);
    d3d::set_tex(STAGE_CS, 0, NULL);
    d3d::resource_barrier({divergenceTex.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});

    // jaccobi
    for (int i = 0; i < DIFFUSE_NUM_ITERATIONS; ++i)
    {
      d3d::set_tex(STAGE_CS, 1, divergenceTex.getVolTex());
      d3d::set_tex(STAGE_CS, 2, pressureTexCur[1]->getVolTex());
      d3d::set_rwtex(STAGE_CS, 2, pressureTexCur[0]->getVolTex(), 0, 0);
      jaccobi3d->dispatch(dx2, dy2, dz2);
      d3d::resource_barrier({pressureTexCur[0]->getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
      d3d::set_tex(STAGE_CS, 2, pressureTexCur[0]->getVolTex());
      d3d::set_rwtex(STAGE_CS, 2, pressureTexCur[1]->getVolTex(), 0, 0);
      jaccobi3d->dispatch(dx2, dy2, dz2);
      d3d::resource_barrier({pressureTexCur[1]->getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
      d3d::set_rwtex(STAGE_CS, 2, NULL, 0, 0);
      d3d::set_tex(STAGE_CS, 2, NULL);
      d3d::set_tex(STAGE_CS, 1, NULL);
    }

    // project
    d3d::set_tex(STAGE_CS, 0, speedTexCur[0]->getVolTex());
    d3d::set_tex(STAGE_CS, 2, pressureTexCur[1]->getVolTex());
    d3d::set_rwtex(STAGE_CS, 0, speedTexCur[1]->getVolTex(), 0, 0);
    project3d->dispatch(dx2, dy2, dz2);
    d3d::resource_barrier({speedTexCur[1]->getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    d3d::set_rwtex(STAGE_CS, 0, NULL, 0, 0);
    d3d::set_tex(STAGE_CS, 2, NULL);
    d3d::set_tex(STAGE_CS, 0, NULL);

    eastl::swap(speedTexCur[0], speedTexCur[1]);
    ShaderGlobal::set_texture(fluidWindTexVarId, *speedTexCur[0]);
  }

  ShaderGlobal::set_int(fluidWindStatusVarId, isInSleep() ? 0 : 1);
}

void FluidWind::renderDebug()
{
#if DEBUG_OUTPUT
  d3d::set_tex(STAGE_CS, 0, speedTexCur[0]->getVolTex());
  d3d::set_tex(STAGE_CS, 1, divergenceTex.getVolTex());
  d3d::set_tex(STAGE_CS, 2, pressureTexCur[1]->getVolTex());
  d3d::set_rwtex(STAGE_CS, 3, outputTex.getTex2D(), 0, 0);
  render->dispatch((FLUID_RT_WIDTH + 15) / 16, (FLUID_RT_HEIGHT + 15) / 16, 1);
  d3d::set_rwtex(STAGE_CS, 3, NULL, 0, 0);
  d3d::set_tex(STAGE_CS, 2, NULL);
  d3d::set_tex(STAGE_CS, 1, NULL);
  d3d::set_tex(STAGE_CS, 0, NULL);
#endif
}

bool FluidWind::isInSleep() const { return sleepTime <= 0.0f; }

int FluidWind::pushOmni(const OmniMotor &omni) { return pushMotor(MOTOR_OMNI, omni); }

int FluidWind::pushVortex(const VortexMotor &vortex) { return pushMotor(MOTOR_VORTEX, vortex); }

int FluidWind::pushDirectional(const DirectionalMotor &directional)
{
  int motorId = pushMotor(MOTOR_DIRECTIONAL, directional);
  Motor &motor = motors[motorId];
  motor.direction = directional.direction;
  return motorId;
}

void FluidWind::stopMotor(int id) { motors[id].stopped = true; }

void FluidWind::stopMotors() { clear_and_shrink(motors); }

void FluidWind::setMotorCenter(int id, const Point3 &center) { motors[id].center = center; }

dag::ConstSpan<FluidWind::Motor> FluidWind::getMotors() const { return motors; }

void FluidWind::init()
{
  fluidWindTexVarId = ::get_shader_glob_var_id("fluid_wind_tex");
  fluidMotorCenterRadiusVarId = ::get_shader_glob_var_id("fluid_motor_center_radius");
  fluidMotorDirectionDurationVarId = ::get_shader_glob_var_id("fluid_motor_direction_duration");
  fluidMotorStrengthVarId = ::get_shader_glob_var_id("fluid_motor_strength");
  fluidMotorTimeVarId = ::get_shader_glob_var_id("fluid_motor_time");
  fluidWindOriginDeltaVarId = ::get_shader_glob_var_id("fluid_wind_origin_delta");
  fluidWindOriginVarId = ::get_shader_glob_var_id("fluid_wind_origin");
  fluidWindStatusVarId = ::get_shader_glob_var_id("fluid_wind_status");
  fluidWindTimeStepVarId = ::get_shader_glob_var_id("fluid_wind_timestep");

  ShaderGlobal::set_color4(::get_shader_glob_var_id("fluid_wind_dim"), Color4(desc.dimX, desc.dimY, desc.dimZ));
  ShaderGlobal::set_int(::get_shader_glob_var_id("fluid_wind_width"), FLUID_RT_WIDTH);
  ShaderGlobal::set_int(::get_shader_glob_var_id("fluid_wind_height"), FLUID_RT_HEIGHT);
  ShaderGlobal::set_color4(::get_shader_glob_var_id("fluid_wind_world_size"),
    Color4(desc.worldSize.x, desc.worldSize.y, desc.worldSize.z, 0));

  clearData.reset(new_compute_shader("fluid_wind_clear"));
  advect.reset(new_compute_shader("fluid_wind_advect"));
  for (int drawMotorNo = 0; drawMotorNo < drawMotors.size(); ++drawMotorNo)
    drawMotors[drawMotorNo].reset(new_compute_shader(DRAW_MOTOR_SHADERS[drawMotorNo]));
  divergence.reset(new_compute_shader("fluid_wind_divergence"));
  jaccobi3d.reset(new_compute_shader("fluid_wind_jaccobi3d"));
  project3d.reset(new_compute_shader("fluid_wind_project3d"));
  render.reset(new_compute_shader("fluid_wind_render"));

  for (int i = 0; i < 2; ++i)
  {
    String texName(0, "windSpeedTex%d", i);
    speedTex[i] = dag::create_voltex(desc.dimX, desc.dimY, desc.dimZ, TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, 1, texName.str());
    speedTex[i].getVolTex()->texfilter(TEXFILTER_SMOOTH);
    speedTex[i].getVolTex()->texaddr(TEXADDR_BORDER);
    speedTex[i].getVolTex()->texbordercolor(0);

    texName = String(0, "windPressureTex%d", i);
    pressureTex[i] = dag::create_voltex(desc.dimX / 4, desc.dimY, desc.dimZ, TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, 1, texName.str());
    pressureTex[i].getVolTex()->texfilter(TEXFILTER_SMOOTH);
    pressureTex[i].getVolTex()->texaddr(TEXADDR_BORDER);
    pressureTex[i].getVolTex()->texbordercolor(0);
  }

  divergenceTex =
    dag::create_voltex(desc.dimX / 4, desc.dimY, desc.dimZ, TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, 1, "windDivergenceTex");
  divergenceTex.getVolTex()->texfilter(TEXFILTER_SMOOTH);
  divergenceTex.getVolTex()->texaddr(TEXADDR_BORDER);
  divergenceTex.getVolTex()->texbordercolor(0);

#if DEBUG_OUTPUT
  outputTex = dag::create_tex(NULL, FLUID_RT_WIDTH, FLUID_RT_HEIGHT, TEXFMT_A16B16G16R16F | TEXCF_UNORDERED, 1, "windOutputTex");
  outputTex.getTex2D()->texfilter(TEXFILTER_SMOOTH);
  outputTex.getTex2D()->texaddr(TEXADDR_BORDER);
  outputTex.getTex2D()->texbordercolor(0);
#endif
}

void FluidWind::close()
{
  for (int i = 0; i < 2; ++i)
  {
    speedTex[i].close();
    pressureTex[i].close();
  }
  divergenceTex.close();
#if DEBUG_OUTPUT
  outputTex.close();
#endif

  clearData.reset();
  advect.reset();
  for (auto &motor : drawMotors)
    motor.reset();
  divergence.reset();
  jaccobi3d.reset();
  project3d.reset();
  render.reset();

  ShaderGlobal::set_int(fluidWindStatusVarId, 0);
}

int FluidWind::pushMotor(MotorType type, const MotorBase &base)
{
  int motorId = INVALID_MOTOR_ID;
  for (int motorNo = 0; motorNo < motors.size(); ++motorNo)
    if (motors[motorNo].stopped)
    {
      motorId = motorNo;
      break;
    }

  Motor &motor = motorId != INVALID_MOTOR_ID ? motors[motorId] : motors.push_back();
  motor.type = type;
  motor.stopped = false;
  motor.time = 0;

  motor.center = base.center;
  motor.radius = base.radius;
  motor.duration = base.duration;
  motor.strength = base.strength;
  motor.shake = base.shake;
  motor.phaseFade = base.phaseFade;
  motor.phaseSin = base.phaseSin;

  motor.direction = Point3(1, 0, 0);

  return motorId != INVALID_MOTOR_ID ? motorId : motors.size() - 1;
}

Point3 FluidWind::toLocalPoint(const Point3 &point) const
{
  Point3 coord = point - lastOrigin;
  return Point3((coord.x / desc.worldSize.x + 0.5f) * desc.dimX, (coord.z / desc.worldSize.z + 0.5f) * desc.dimY,
    (coord.y / desc.worldSize.y + 0.5f) * desc.dimZ);
}

Point3 FluidWind::toLocalVector(const Point3 &vector) const
{
  return Point3(vector.x / desc.worldSize.x * desc.dimX, vector.z / desc.worldSize.z * desc.dimY,
    vector.y / desc.worldSize.y * desc.dimZ);
}

bool FluidWind::isContained(const Point3 &center, float radius) const
{
  Point3 regionSize = max(abs(center - lastOrigin) - Point3(radius, radius, radius), Point3(0, 0, 0));
  return regionSize.x < desc.worldSize.x || regionSize.y < desc.worldSize.y || regionSize.z < desc.worldSize.z;
}