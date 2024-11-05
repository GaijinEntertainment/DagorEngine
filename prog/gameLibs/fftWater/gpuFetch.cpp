// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_query.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_tex3d.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_Point3.h>
#include <generic/dag_smallTab.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_postFxRenderer.h>
#include <fftWater/gpuFetch.h>

namespace fft_water
{
static int check_water_posVarId = -1;
static int check_water_screenVarId = -1;
static PostFxRenderer queryPostFx;
static ShaderMaterial *waterRequestMat = NULL;
static ShaderElement *waterRequestElem = NULL;

void global_init_queries()
{
  check_water_posVarId = get_shader_variable_id("check_water_pos", true);
  check_water_screenVarId = get_shader_variable_id("check_water_screen", true);
  queryPostFx.init("is_underwater");
}

class GpuOnePointQuery
{
public:
  GpuOnePointQuery() : query(0), resultHeight(-100), gotResults(false), maxHeight(1), rt(NULL) { global_init_queries(); }
  void destroy()
  {
    d3d::driver_command(Drv3dCommand::RELEASE_QUERY, &query);
    query = NULL;
    gotResults = false;
    resultHeight = -100;
    del_d3dres(rt);
  }
  bool updateQueryStatus()
  {
    if (!query)
      return true;
    if (gotResults)
      return true;
    int pixels = d3d::driver_command(Drv3dCommand::GETVISIBILITYCOUNT, query);
    if (pixels >= 0)
    {
      gotResults = true;
      resultHeight = pixels ? (pixels - 1) * maxHeight / 7.0 : -1000;
      return true;
    }
    return false;
  }
  void updateQuery(const Point3 &pos, float height)
  {
    Driver3dRenderTarget prevRt;
    d3d::get_render_target(prevRt);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

    if (rt == NULL)
    {
      rt = d3d::create_tex(NULL, pixels_to_draw, 1, TEXFMT_A8R8G8B8 | TEXCF_RTARGET, 1, "gpu water one point query");
      d3d_err(rt);
      d3d::set_render_target(rt, 0);
      d3d::clearview(CLEAR_TARGET, 0, 0, 0);
    }
    else
      d3d::set_render_target(rt, 0);

    ShaderGlobal::set_color4(check_water_posVarId, pos.x, pos.y, pos.z, maxHeight = height);
    int x, y;
    d3d::get_target_size(x, y);
    ShaderGlobal::set_color4(check_water_screenVarId, 1. / x * pixels_to_draw, 1. / y, 0.5f / pixels_to_draw, 0);
    d3d::driver_command(Drv3dCommand::GETVISIBILITYBEGIN, &query);
    queryPostFx.render();
    d3d::driver_command(Drv3dCommand::GETVISIBILITYEND, query);
    gotResults = false;
    d3d::set_render_target(prevRt);
  }

  float getResult(bool *is_last)
  {
    if (is_last)
      *is_last = gotResults;
    return resultHeight;
  }
  ~GpuOnePointQuery() { destroy(); }
  void beforeReset() { destroy(); }

protected:
  void *query;
  float maxHeight, resultHeight;
  bool gotResults;
  Texture *rt;

  static const uint32_t pixels_to_draw = 8;
};

class GpuFetchQuery
{
public:
  GpuFetchQuery(int max_number) :
    maxNumber(max_number), gotResults(false), eventWaitCount(-1), lastNumber(0), event(NULL), gpuTexture(NULL), vb(NULL)
  {}
  bool init()
  {
    G_ASSERT(maxNumber > 0 && maxNumber <= 4096);
    destroy();
    maxNumber = clamp(maxNumber, (int)0, (int)4096);
    gpuTexture = d3d::create_tex(NULL, maxNumber, 1, TEXCF_RTARGET | TEXCF_LINEAR_LAYOUT | TEXFMT_R32F, 1, "water_fetch_gpu");
    vb = d3d::create_vb(maxNumber * sizeof(float) * 4, SBCF_DYNAMIC, "histogram");
    event = d3d::create_event_query();
    clear_and_resize(results, maxNumber);
    for (int i = 0; i < maxNumber; ++i)
      results[i] = -1000;
    gotResults = false;
    return gpuTexture && vb && event;
  }
  void destroy()
  {
    d3d::release_event_query(event);
    event = 0;
    del_d3dres(gpuTexture);
    del_d3dres(vb);
    eventWaitCount = -1;
  }
  bool updateQueryStatus()
  {
    if (eventWaitCount < 0)
      return true;
    if (!event)
      return false;
    if (gotResults)
      return true;
    if (d3d::get_event_query_status(event, (++eventWaitCount) > 10))
    {
      eventWaitCount = 1;
      float *data = NULL;
      int stride;
      if (gpuTexture->lockimg((void **)&data, stride, 0, TEXLOCK_READ | TEXLOCK_RAWDATA))
      {
        eventWaitCount = 0;
        G_ASSERT(lastNumber <= results.size());
        memcpy(results.data(), data, lastNumber * sizeof(float));
        gotResults = true;
        gpuTexture->unlockimg();
        return true;
      }
    }
    return false;
  }
  void updateQuery(const Point3 *pos, int count)
  {
    if (count <= 0)
      return;
    count = min(count, maxNumber);
    lastNumber = count;
    float *vertices;
    if (!vb->lock(0, lastNumber * sizeof(float) * 4, (void **)&vertices,
          VBLOCK_WRITEONLY | VBLOCK_NOSYSLOCK | (gotResults ? VBLOCK_NOOVERWRITE : VBLOCK_DISCARD)))
      return;
    for (int i = 0; i < lastNumber; ++i, vertices += 4, pos++)
    {
      vertices[0] = pos->x;
      vertices[1] = pos->y;
      vertices[2] = pos->z;
      vertices[3] = i;
    }
    vb->unlock();


    Driver3dRenderTarget prevRt;
    d3d::get_render_target(prevRt);
    SCOPE_RESET_SHADER_BLOCKS;
    ShaderGlobal::set_color4(check_water_screenVarId, 1. / maxNumber, 1, maxNumber, 1);
    d3d::set_render_target(gpuTexture, 0);
    waterRequestElem->setStates(0, true);
    d3d::setvsrc(0, vb, sizeof(float) * 4);
    d3d::draw(PRIM_POINTLIST, 0, lastNumber);
    gotResults = false;
    d3d::set_render_target(prevRt);
    {
      TIME_D3D_PROFILE(copy)
      int stride;
      if (gpuTexture->lockimg(NULL, stride, 0, TEXLOCK_READ | TEXLOCK_RAWDATA))
        gpuTexture->unlockimg();
      d3d::issue_event_query(event);
    }
    eventWaitCount = 1;
  }

  dag::ConstSpan<float> getResults(bool *is_last) const
  {
    if (is_last)
      *is_last = gotResults;
    return dag::ConstSpan<float>(results.data(), eventWaitCount >= 0 ? lastNumber : 0);
  }
  ~GpuFetchQuery() { destroy(); }

protected:
  d3d::EventQuery *event;
  Texture *gpuTexture;
  Vbuffer *vb;
  bool gotResults;
  int lastNumber, maxNumber, eventWaitCount;
  SmallTab<float, TmpmemAlloc> results;
};

GpuFetchQuery *create_gpu_fetch_query(int max_number)
{
  if (!d3d::get_driver_desc().caps.hasAsyncCopy)
    return NULL;
  if ((d3d::get_texformat_usage(TEXFMT_R32F) & d3d::USAGE_RTARGET) != d3d::USAGE_RTARGET)
    return NULL;
  if (!init_gpu_fetch())
    return NULL;
  if (!waterRequestElem)
    return NULL;
  GpuFetchQuery *q = new GpuFetchQuery(max_number);
  if (!q->init())
  {
    del_it(q);
  }
  return q;
}


GpuOnePointQuery *create_one_point_query() { return new GpuOnePointQuery(); }
void destroy_query(GpuFetchQuery *&q) { del_it(q); }
void destroy_query(GpuOnePointQuery *&q) { del_it(q); }

void update_query(GpuOnePointQuery *q, const Point3 &pos, float height)
{
  if (!q)
    return;
  return q->updateQuery(pos, height);
}
bool update_query_status(GpuOnePointQuery *q)
{
  if (!q)
    return false;
  return q->updateQueryStatus();
}
float get_result(GpuOnePointQuery *q, bool *is_last)
{
  if (!q)
    return -1000.f;
  return q->getResult(is_last);
}

void before_reset(GpuOnePointQuery *q)
{
  if (!q)
    return;
  q->beforeReset();
}

void update_query(GpuFetchQuery *q, const Point3 *pos, int count)
{
  if (!q)
    return;
  return q->updateQuery(pos, count);
}
bool update_query_status(GpuFetchQuery *q)
{
  if (!q)
    return false;
  return q->updateQueryStatus();
}
dag::ConstSpan<float> get_results(GpuFetchQuery *q, bool *is_last)
{
  if (!q)
    return {};
  return q->getResults(is_last);
}

bool init_gpu_fetch()
{
  if (waterRequestElem)
    return true;

  if (!d3d::get_driver_desc().caps.hasAsyncCopy)
    return false;
  if ((d3d::get_texformat_usage(TEXFMT_R32F) & d3d::USAGE_RTARGET) != d3d::USAGE_RTARGET)
    return false;

  waterRequestMat = new_shader_material_by_name("get_water_height");
  G_ASSERT(waterRequestMat);
  if (waterRequestMat)
    waterRequestElem = waterRequestMat->make_elem();
  if (waterRequestElem == NULL)
    close_gpu_fetch();

  check_water_screenVarId = get_shader_variable_id("check_water_screen", true);
  return waterRequestElem != NULL;
}
void close_gpu_fetch()
{
  waterRequestElem = NULL; // deleted by refcount within constantFillerMat
  del_it(waterRequestMat);
}
} // namespace fft_water