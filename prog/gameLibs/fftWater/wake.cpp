// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <math/dag_mathUtils.h>
#include <fftWater/wake.h>

namespace fft_water
{

static int wake_vd_texVarId = -1, wake_pht_texVarId = -1, wake_ht_texVarId = -1, wake_cht_texVarId = -1;
static int wake_dtVarId = -1, wake_alphaVarId = -1;
static int wake_resolutionVarId = -1;
static int wake_move_ofsVarId = -1;
static int wake_gradients_texVarId = -1;
static int next_wake_ofsVarId = -1;
static int world_to_wakeVarId = -1;
static int wake_impulses_const_no = -1;
static int wake_obstacles_const_no = -1;
static int wake_impulses_countVarId = -1;
static int wake_obstacles_countVarId = -1;
static int wake_main_positionVarId = -1;
static int wake_main_dirVarId = -1;
static int wake_water_levelVarId = -1;

Wake::Wake()
{
  autoOffTimeThreshold = 15.0f; // default threshold
  lastNotEmptyPos = Point2(0, 0);
  wakeOn = false;
  wakeAutoOff = false;
  mainPos = Point3(0, 10000, 0);
  mainSpeed = 0;
  mainDir = Point2(0, 1);
  dirSize = Point2(1, 1);
  waterLevel = 0;
  resolution = 0;
  alpha = 0.3f;
  wake_impulses_countVarId = get_shader_variable_id("wake_impulses_count", true);
  wake_obstacles_countVarId = get_shader_variable_id("wake_obstacles_count", true);
  wake_water_levelVarId = get_shader_variable_id("wake_water_level", false);
  if (get_shader_variable_id("wake_impulses_const_no", true) >= 0)
  {
    wake_impulses_const_no = ShaderGlobal::get_int_fast(get_shader_variable_id("wake_impulses_const_no", true));
    impulses.resize(ShaderGlobal::get_int_fast(get_shader_variable_id("wake_impulses_max_count")));
    impulses.clear();
  }
  if (get_shader_variable_id("wake_obstacles_const_no", true) >= 0)
  {
    wake_obstacles_const_no = ShaderGlobal::get_int_fast(get_shader_variable_id("wake_obstacles_const_no", true));
    obstacles.resize(ShaderGlobal::get_int_fast(get_shader_variable_id("wake_obstacles_max_count")));
    obstacles.clear();
  }
  wake_main_positionVarId = get_shader_variable_id("wake_main_position", true);
  wake_main_dirVarId = get_shader_variable_id("wake_main_dir", true);

  wake_vd_texVarId = get_shader_variable_id("wake_vd_tex", false);
  wake_pht_texVarId = get_shader_variable_id("wake_pht_tex", false);
  wake_ht_texVarId = get_shader_variable_id("wake_ht_tex", false);
  wake_cht_texVarId = get_shader_variable_id("wake_cht_tex", false);
  wake_dtVarId = get_shader_variable_id("wake_dt", false);
  wake_alphaVarId = get_shader_variable_id("wake_alpha", false);
  wake_resolutionVarId = get_shader_variable_id("wake_resolution", false);
  wake_move_ofsVarId = get_shader_variable_id("wake_move_ofs", false);
  wake_gradients_texVarId = get_shader_variable_id("wake_gradients_tex", false);
  next_wake_ofsVarId = get_shader_variable_id("next_wake_ofs", false);
  world_to_wakeVarId = get_shader_variable_id("world_to_wake", false);
  regionSize = 48.0f;
  lastPos = Point2(0, 0);
  worldToWake = Point4(0, 0, 0, 0);

  currentHeight = 0;
}


bool Wake::init(int res)
{
  if (res == resolution)
    return true;

  close();
  wakeOn = false;
  uint32_t fmt = TEXFMT_G16R16F;
  uint32_t gradFmt = TEXFMT_G16R16F;
  unsigned int workingFlags = d3d::USAGE_FILTER | d3d::USAGE_RTARGET;
  if ((d3d::get_texformat_usage(fmt) & workingFlags) != workingFlags ||
      (d3d::get_texformat_usage(gradFmt) & workingFlags) != workingFlags)
    return false;

  wakeOn = true;
  lastNotEmptyPos = Point2(0, 0);
  wakeAutoOff = false;
  autoOffTimer = 0;
  resolution = res;
  const char *name = "wake"; // fixme: name prefix is wake param
  uint32_t vdfmt = fmt;      // use the same format for simplicity but still some optimization might be done
  for (int i = 0; i < height.size(); ++i)
  {
    String texName(128, "%s_height%d", name, i);
    height[i] = dag::create_tex(NULL, resolution, resolution, TEXCF_RTARGET | fmt, 1, texName.str());
    height[i].getTex2D()->texaddr(TEXADDR_BORDER);
    height[i].getTex2D()->texbordercolor(0);
  }

  String texName(128, "%s_gradients", name);
  wakeGradients = dag::create_array_tex(resolution, resolution, 2, TEXCF_RTARGET | gradFmt, 1, texName.str());
  wakeGradients.getTex2D()->texaddr(TEXADDR_BORDER);
  wakeGradients.getTex2D()->texbordercolor(0);

  ShaderGlobal::set_int(wake_resolutionVarId, resolution);
  cleared_flag = false;
  movedFlag = true;
  currentHeight = 0;
  moveOfs = Point2(0, 0);

  // common for all wakes (if ever many)!
  verticalDerivative = dag::create_tex(NULL, resolution, resolution, TEXCF_RTARGET | vdfmt, 1, "wakeVdTex"); // todo: if there are many
                                                                                                             // wakes, we still need
                                                                                                             // only one vdTex
  // vdTex->texaddr(TEXADDR_CLAMP);
  verticalDerivative.getTex2D()->texfilter(TEXFILTER_POINT);
  makeDerivatives.init("make_derivatives");
  makeHeightmap.init("make_wake_heightmap");
  copyHeightmap.init("copy_wake_heightmap");
  calculateGradients.init("calc_wake_gradients");
  return true;
}

void Wake::close()
{
  for (int i = 0; i < height.size(); ++i)
    height[i].close();
  ShaderGlobal::set_texture(wake_vd_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(wake_pht_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(wake_ht_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(wake_cht_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(wake_gradients_texVarId, BAD_TEXTUREID);

  makeDerivatives.clear();
  makeHeightmap.clear();
  copyHeightmap.clear();
  calculateGradients.clear();
  verticalDerivative.close();
  wakeGradients.close();

  resolution = 0;
}

void Wake::setLevel(float water_level)
{
  waterLevel = water_level;
  ShaderGlobal::set_real(wake_water_levelVarId, waterLevel);
}

Wake::~Wake() { close(); }

void Wake::calculateKernel(int halfKernelSize)
{
  double dk = 0.01;
  double sigma = 1.0;
  double norm = 0;
  for (double k = 0; k < 10; k += dk) //-V1034
  {
    norm += k * k * exp(-sigma * k * k);
  }
  float sum = 0;
  String kernelStr(128, "float kernel[%d][%d]= {\n", halfKernelSize * 2 + 1, 2 * halfKernelSize + 1);
  String shaderKernelStr(128, "float kernel[%d]= {\n  ", (halfKernelSize + 1) * (halfKernelSize + 1));
  String shaderDX11(128, "//the_type vd=texelFetch(theTex, int2(i,j)).attr;\n");
  for (int i = -halfKernelSize; i <= halfKernelSize; i++)
  {
    kernelStr.aprintf(128, "{");
    for (int j = -halfKernelSize; j <= halfKernelSize; j++)
    {
      double r = sqrt((float)(i * i + j * j));
      double kern = 0;
      for (double k = 0; k < 10; k += dk) //-V1034
      {
        kern += k * k * exp(-sigma * k * k) * j0(r * k);
      }

      // kernel[i + HALF_KERNEL_SIZE][j + HALF_KERNEL_SIZE] = kern / norm;
      double val = (kern / norm);
      sum += val;

      kernelStr.aprintf(128, "%g%s", val, j != halfKernelSize ? "," : "");
      if (i <= 0 && j <= 0)
        shaderKernelStr.aprintf(128, i == 0 && j == 0 ? "%g" : "%g,", val);

      if (i <= 0 && j < 0 && j >= i)
      {
        shaderDX11.aprintf(128,
          "vd += %g*(GET(%d, %d)+GET(%d, %d)+GET(%d, %d)+GET(%d, %d)+GET(%d, %d)+GET(%d, %d)+GET(%d, %d)+GET(%d, %d));\n", val, +i, +j,
          +j, +i, -j, +i, -i, +j, +i, -j, +j, -i, -i, -j, -j, -i);
      }
    }
    if (i <= 0)
      shaderKernelStr.aprintf(128, i < 0 ? "\n  " : "\n");
    kernelStr.aprintf(128, "}%s\n", i < halfKernelSize ? "," : "");
  }
  kernelStr.aprintf(128, "};\n");
  shaderKernelStr.aprintf(128, "};\n");
  debug("shaderStr=\n%s;//float total_sum=%g;", shaderDX11.str(), sum);

  shaderKernelStr.aprintf(128, "static const float kernel_sum = %g;\n", sum);
  debug("(%g)kernelStr=\n%s", sum, kernelStr.str());
  debug("shader=\n%s\n", shaderKernelStr.str());
}

void Wake::renderMove(const Point2 &ofs)
{
  if (fabsf(ofs.x) < 1. / 16384 && fabsf(ofs.y) < 1. / 16384)
    return;
  if (!height[0].getTex2D())
    return;
  ShaderGlobal::set_color4(wake_move_ofsVarId, ofs.x, ofs.y, 0, 0);

  d3d::set_render_target(height[currentHeight].getTex2D(), 0);
  for (int i = 0; i < height.size(); ++i)
  {
    if (i == currentHeight)
      continue;
    ShaderGlobal::set_texture(wake_ht_texVarId, height[i]);
    copyHeightmap.render();
    height[i].getTex2D()->update(height[currentHeight].getTex2D());
  }
}

void Wake::simulateDt(const Point2 &origin, float dt)
{
  if (!height[0])
    return;
  if (dt > 0.1f)
    cleared_flag = false;
  bool hasSource = obstacles.size() || impulses.size() || (mainPos.y <= waterLevel && (regionBox & Point2::xz(mainPos)));
  if (hasSource)
  {
    autoOffTimer = 0;
    wakeAutoOff = false;
  }
  else
  {
    autoOffTimer += dt;
    if (autoOffTimer > autoOffTimeThreshold)
    {
      if (!wakeAutoOff)
        cleared_flag = false;
      wakeAutoOff = true;
    }
  }
  float texelSize = 2 * regionSize / getResolution(); // half texel alignment for 0 cascade, texel for 1 cascade
  Point2 alignedPos(floorf(origin.x / texelSize) * texelSize, floorf(origin.y / texelSize) * texelSize);
  if (lengthSq(lastPos - alignedPos) > (0.25 * 0.25 * regionSize * regionSize))
    moveRegion(alignedPos);

  if (hasSource)
    lastNotEmptyPos = lastPos;
  else if (max(fabsf(lastNotEmptyPos.x - lastPos.x), fabsf(lastNotEmptyPos.y - lastPos.y)) > regionSize)
  {
    if (!wakeAutoOff)
      cleared_flag = false;
    wakeAutoOff = true;
  }

  worldToWake = Point4(1.0 / regionSize, -(lastPos.x - regionSize * 0.5) / regionSize, -(lastPos.y - regionSize * 0.5) / regionSize,
    2.0f * texelSize);

  ShaderGlobal::set_color4(next_wake_ofsVarId, (origin.x - lastPos.x) / regionSize, (origin.y - lastPos.y) / regionSize, 0, 0);
  ShaderGlobal::set_color4(world_to_wakeVarId, Color4::xyzw(worldToWake));

  Driver3dRenderTarget prevRt;
  d3d::get_render_target(prevRt);
  //
  bool shouldClear = !cleared_flag;
  if (shouldClear)
  {
    for (int i = 0; i < height.size(); ++i)
    {
      d3d::set_render_target(height[i].getTex2D(), 0);
      d3d::clearview(CLEAR_TARGET, 0, 0.f, 0);
    }
    cleared_flag = true;
    movedFlag = true;
    moveOfs = Point2(0, 0);
  }
  ShaderGlobal::set_int(wake_resolutionVarId, resolution);
  if (!movedFlag)
  {
    renderMove(moveOfs);
    moveOfs = Point2(0, 0);
    movedFlag = true;
  }
  if (dt > 0.1f || dt < 1.e-4f || wakeAutoOff || !wakeOn)
  {
    if (shouldClear)
    {
      d3d::set_render_target(0, wakeGradients.getArrayTex(), 0, 0);
      d3d::set_render_target(1, wakeGradients.getArrayTex(), 1, 0);
      d3d::clearview(CLEAR_TARGET, 0, 0.f, 0);
    }
    if (dt >= 1.e-4f)
      setNullTex();
    d3d::set_render_target(prevRt);
    impulses.clear();
    obstacles.clear();
    return;
  }
  d3d::set_render_target(verticalDerivative.getTex2D(), 0);
  int nextheight = currentHeight;
  int prevheight = (nextheight - 2 + 3) % height.size();
  int curheight = (nextheight - 1 + 3) % height.size();
  float actualDt = dt;
  actualDt = min(0.03f, actualDt);

  ShaderGlobal::set_texture(wake_ht_texVarId, height[curheight]);
  makeDerivatives.render(); // todo:we can totally separate and call it somewhere else, to save cache flush gpu wait time after it

  ShaderGlobal::set_real(wake_alphaVarId, cvt(actualDt, 0.02, 0.03f, alpha, 2 * alpha)); // to fight with instabilities
  ShaderGlobal::set_texture(wake_vd_texVarId, verticalDerivative);
  ShaderGlobal::set_texture(wake_pht_texVarId, height[prevheight]);
  ShaderGlobal::set_texture(wake_cht_texVarId, height[curheight]);
  ShaderGlobal::set_real(wake_dtVarId, actualDt);
  ShaderGlobal::set_int(wake_obstacles_countVarId, obstacles.size());
  ShaderGlobal::set_color4(wake_main_positionVarId, mainPos.x, mainPos.y, mainPos.z, mainSpeed);
  ShaderGlobal::set_color4(wake_main_dirVarId, mainDir.x, mainDir.y, 1.0f / max(0.01f, dirSize.x), 1.0 / max(0.01f, dirSize.y));

  if (obstacles.size() > 0)
    d3d::set_ps_const(wake_obstacles_const_no, (float *)obstacles.data(), obstacles.size());
  obstacles.clear();

  ShaderGlobal::set_int(wake_impulses_countVarId, impulses.size());
  if (impulses.size() > 0)
    d3d::set_ps_const(wake_impulses_const_no, (float *)impulses.data(), impulses.size());
  impulses.clear();

  d3d::set_render_target(height[nextheight].getTex2D(), 0);
  makeHeightmap.render();
  ShaderGlobal::set_texture(wake_ht_texVarId, height[nextheight]);
  d3d::set_render_target(0, wakeGradients.getArrayTex(), 0, 0);
  d3d::set_render_target(1, wakeGradients.getArrayTex(), 1, 0);
  calculateGradients.render();
  d3d::set_render_target(prevRt);

  ShaderGlobal::set_texture(wake_gradients_texVarId, wakeGradients);
  // height[nextheight].getTex()->texfilter(TEXFILTER_LINEAR);

  currentHeight = (1 + currentHeight) % height.size();
}
// float  tmep = height[i]
// height[i] = height[i] * ((2.0 - adt)*adt2) - (previous_height[i] + gravity * vertical_derivative[i])*adt2;
// height[i] += source[i];
// height[i] *= obstruction[i];
// previous_height[i] = temp;


#define TOO_DEEP_THRESHOLD       4.0f
#define HEIGHT_ABOVE_WATER_LIMIT 3.f

void Wake::addImpulse(const Point3 &pos, float str)
{
  if ((str >= -0.01f && str <= 0.01f) || !wakeOn)
    return;
  if (pos.y - waterLevel > HEIGHT_ABOVE_WATER_LIMIT || pos.y < waterLevel - TOO_DEEP_THRESHOLD ||
      !(regionBox & Point2::xz(pos))) // fixme: we should actualy if (str/Squared_distance_to_box) > threshold
    return;
  Point4 imp(pos.x, pos.y, pos.z, str);
  if (impulses.size() < impulses.capacity())
  {
    impulses.push_back(imp);
    return;
  }
  float distSq = lengthSq(lastPos - Point2::xz(pos));
  float minStr = MAX_REAL;
  int minStrIdx = -1;
  for (int i = 0; i < impulses.size(); ++i)
  {
    float cdistSq = lengthSq(lastPos - Point2::xz(impulses[i]));
    if (distSq < cdistSq && fabsf(imp.w) >= fabsf(impulses[i].w))
    {
      impulses[i] = imp;
      return;
    }
    if (minStr > fabsf(impulses[i].w))
    {
      minStr = fabsf(impulses[i].w);
      minStrIdx = i;
    }
  }

  if (minStr < fabsf(str) && minStrIdx >= 0)
    impulses[minStrIdx] = imp;
}


void Wake::addObstacle(const Point3 &pos, float rad)
{
  if (!wakeOn || rad < 0.01f || pos.y > waterLevel || pos.y < waterLevel - TOO_DEEP_THRESHOLD ||
      !(regionBox & Point2::xz(pos))) // fixme: we should actualy if (sphere intersects box)
    return;
  Point4 imp(pos.x, pos.y, pos.z, 1.0f / rad);
  if (obstacles.size() < obstacles.capacity())
  {
    obstacles.push_back(imp);
    return;
  }
  float distSq = lengthSq(lastPos - Point2::xz(pos));
  for (int i = 0; i < obstacles.size(); ++i)
  {
    float cdistSq = lengthSq(lastPos - Point2::xz(obstacles[i]));
    if (distSq < cdistSq)
    {
      obstacles[i] = imp;
      return;
    }
  }
}

void Wake::setMainObstacle(const Point3 &pos, const Point2 &dir, const Point2 &dir_size, float speed)
{
  mainPos = pos;
  mainSpeed = speed;
  mainDir = dir;
  dirSize = dir_size;
}

void Wake::setNullTex()
{
  ShaderGlobal::set_texture(wake_gradients_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(wake_ht_texVarId, BAD_TEXTUREID);
}
}; // namespace fft_water
