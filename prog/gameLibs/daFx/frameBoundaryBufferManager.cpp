// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "frameBoundaryBufferManager.h"
#include "common.h"
#include <vecmath/dag_vecMath.h>
#include <shaders/dag_shaders.h>
#include <osApiWrappers/dag_critSec.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_info.h>

#include <util/dag_console.h>
#include <gui/dag_imgui.h>
#include <imgui/imgui.h>
#include <ioSys/dag_dataBlock.h>


CONSOLE_BOOL_VAL("dafx_frameboundary", disable_frameboundary, false);
CONSOLE_BOOL_VAL("dafx_frameboundary", use_optimized_boundary_calc, true);
CONSOLE_BOOL_VAL("dafx_frameboundary", use_experimental_boundary_calc, true);

CONSOLE_BOOL_VAL("dafx_frameboundary", debug_update, false);
CONSOLE_INT_VAL("dafx_frameboundary", debug_texture_id, 0, 0, 64);
CONSOLE_INT_VAL("dafx_frameboundary", debug_frame_id, 0, 0, DAFX_FLIPBOOK_MAX_KEYFRAME_DIM *DAFX_FLIPBOOK_MAX_KEYFRAME_DIM);


#define FRAME_BOUNDARY_VARS_LIST          \
  VAR(dafx_frame_boundary_buffer)         \
  VAR(dafx_fill_boundary_tex)             \
  VAR(dafx_fill_boundary_params)          \
  VAR(dafx_fill_boundary_offset)          \
  VAR(dafx_fill_boundary_count)           \
  VAR(dafx_fill_boundary_frame_id)        \
  VAR(dafx_frame_boundary_debug_tex)      \
  VAR(dafx_frame_boundary_tmp)            \
  VAR(dafx_frame_boundary_buffer_enabled) \
  VAR(dafx_use_experimental_boundary_calc)
#define VAR(a) static int a##VarId = -1;
FRAME_BOUNDARY_VARS_LIST
#undef VAR

static void init_shader_vars()
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, false);
  FRAME_BOUNDARY_VARS_LIST
#undef VAR
}


static WinCritSec g_frame_boundary_cs;


static bool checkHWSupport()
{
  // UAV, intrinsics etc.
  return d3d::get_driver_desc().shaderModel >= 5.0_sm;
}


static bool get_texture_size(TEXTUREID texture_id, IPoint2 &texture_size)
{
  Texture *texture = acquire_managed_tex(texture_id);
  if (texture)
  {
    TextureInfo textureInfo;
    texture->getinfo(textureInfo);
    texture_size.x = textureInfo.w;
    texture_size.y = textureInfo.h;
    release_managed_tex(texture_id);
  }
  return texture;
}


static dag::Vector<FrameBoundaryBufferManager *> managers; // debug-only


FrameBoundaryBufferManager::FrameBoundaryBufferManager()
{
  WinAutoLock lock(g_frame_boundary_cs);

  managers.push_back(this);
}
FrameBoundaryBufferManager::~FrameBoundaryBufferManager()
{
  WinAutoLock lock(g_frame_boundary_cs);

  auto it = eastl::find(managers.begin(), managers.end(), this);
  if (it != managers.end())
    managers.erase(it);
}

FrameBoundaryBufferManager::FrameBoundaryBufferManager(FrameBoundaryBufferManager &&rhs)
{
  WinAutoLock lock(g_frame_boundary_cs);

  auto it = eastl::find(managers.begin(), managers.end(), &rhs);
  *it = this;

  isSupported = rhs.isSupported;

  frameBoundaryAllocator = eastl::move(rhs.frameBoundaryAllocator);
  frameBoundaryElemArr = eastl::move(rhs.frameBoundaryElemArr);
  dirtyElemArr = eastl::move(rhs.dirtyElemArr);

  bufferElemCnt = rhs.bufferElemCnt;

  frameBoundaryBuffer = eastl::move(rhs.frameBoundaryBuffer);
  frameBoundaryBufferTmp = eastl::move(rhs.frameBoundaryBufferTmp);
  debugTexture = eastl::move(rhs.debugTexture);

  fillBoundaryLegacyCs = eastl::move(rhs.fillBoundaryLegacyCs);
  fillBoundaryOptCs = eastl::move(rhs.fillBoundaryOptCs);
  fillBoundaryOptStartCs = eastl::move(rhs.fillBoundaryOptStartCs);
  fillBoundaryOptEndCs = eastl::move(rhs.fillBoundaryOptEndCs);
  clearBoundaryCs = eastl::move(rhs.clearBoundaryCs);
  updateDebugTexCs = eastl::move(rhs.updateDebugTexCs);
}

void FrameBoundaryBufferManager::init(bool use_sbuffer)
{
  WinAutoLock lock(g_frame_boundary_cs);

  isSupported = use_sbuffer && checkHWSupport();

  fillBoundaryLegacyCs = new_compute_shader("fill_fx_keyframe_boundary_legacy", true);
  isSupported = isSupported && fillBoundaryLegacyCs;

  fillBoundaryOptCs = new_compute_shader("fill_fx_keyframe_boundary_opt", true);
  isSupported = isSupported && fillBoundaryOptCs;

  fillBoundaryOptStartCs = new_compute_shader("fill_fx_keyframe_boundary_opt_start", true);
  isSupported = isSupported && fillBoundaryOptStartCs;

  fillBoundaryOptEndCs = new_compute_shader("fill_fx_keyframe_boundary_opt_end", true);
  isSupported = isSupported && fillBoundaryOptEndCs;

  clearBoundaryCs = new_compute_shader("clear_fx_keyframe_boundary", true);
  isSupported = isSupported && clearBoundaryCs;

  updateDebugTexCs = new_compute_shader("frame_boundary_debug_update", true);
  isSupported = isSupported && updateDebugTexCs;

  if (isSupported)
  {
    debug("dafx: frameboundary init, success");

    init_shader_vars();

    frameBoundaryBufferInitialSize = ::dgs_get_settings()->getBlockByNameEx("graphics")->getInt("frameBoundaryBufferInitialSize", 0);
    if (frameBoundaryBufferInitialSize)
      frameBoundaryAllocator.resize(frameBoundaryBufferInitialSize);
  }
  else
  {
    debug("dafx: frameboundary init, not supported");
    ShaderGlobal::set_buffer(::get_shader_variable_id("dafx_frame_boundary_buffer", true), BAD_D3DRESID);
  }
}

int FrameBoundaryBufferManager::acquireFrameBoundary(TEXTUREID texture_id, IPoint2 frame_dim)
{
  WinAutoLock lock(g_frame_boundary_cs);

  if (!isSupported)
    return DAFX_INVALID_BOUNDARY_OFFSET;

  G_ASSERT_RETURN(texture_id != BAD_TEXTUREID, DAFX_INVALID_BOUNDARY_OFFSET);
  G_ASSERT_RETURN(frame_dim.x > 0 && frame_dim.y > 0, DAFX_INVALID_BOUNDARY_OFFSET);

  for (auto &&elem : frameBoundaryElemArr)
    if (elem.textureId == texture_id && elem.frameDim == frame_dim)
      return frameBoundaryAllocator.get(elem.regionId).offset;

  int elemIndex = frameBoundaryAllocator.regionsCount();

  FrameBoundaryElem elem = createElem(texture_id, frame_dim);
  Region region = frameBoundaryAllocator.get(elem.regionId);

  Texture *texture = acquire_managed_tex(texture_id);
  if (!texture)
  {
    logerr("dafx: frameboundary insert error, texture not found: %ld, (%d, %d)", (uint32_t)texture_id, frame_dim.x, frame_dim.y);
    return DAFX_INVALID_BOUNDARY_OFFSET;
  }
  debug("dafx: unique frameboundary inserted at (%d, %d): %ld (%s), (%d, %d)", elemIndex, region.offset, (uint32_t)texture_id,
    texture->getTexName(), frame_dim.x, frame_dim.y);
  G_UNUSED(elemIndex);
  release_managed_tex(texture_id);

  return region.offset;
}

void FrameBoundaryBufferManager::afterDeviceReset()
{
  WinAutoLock lock(g_frame_boundary_cs);

  if (!isSupported)
    return;

  resetFrameBoundaryResult();
}

void FrameBoundaryBufferManager::update(unsigned int current_frame)
{
  WinAutoLock lock(g_frame_boundary_cs);

  if (!isSupported)
    return;

  ShaderGlobal::set_int(dafx_frame_boundary_buffer_enabledVarId, isSupported && !disable_frameboundary); //-V560

  int maxTargetCapacity = frameBoundaryAllocator.getHeapSize();
  if (bufferElemCnt == 0 || bufferElemCnt < maxTargetCapacity)
  {
    bufferElemCnt = maxTargetCapacity;
    resetFrameBoundaryResult();
    // skip 1 frame update (was cleared this frame)
  }
  else if (use_optimized_boundary_calc.pullValueChange() || use_experimental_boundary_calc.pullValueChange())
  {
    resetFrameBoundaryResult();
  }
  else
  {
    updateFrameElems(current_frame);
  }

#if DAGOR_DBGLEVEL > 0
  updateDebugTexture();
#endif
}

void FrameBoundaryBufferManager::prepareRender()
{
  WinAutoLock lock(g_frame_boundary_cs);

  if (!isSupported)
    return;

  ShaderGlobal::set_buffer(dafx_frame_boundary_bufferVarId, frameBoundaryBuffer);
}

void FrameBoundaryBufferManager::resizeIncrement(size_t min_size_increment)
{
  int sizeIncrement = frameBoundaryAllocator.getHeapSize() / 2; // +50% capcacity
  int newCapacity = frameBoundaryAllocator.getHeapSize() + max<int>(min_size_increment, sizeIncrement);

  if (frameBoundaryBufferInitialSize)
    logwarn("FrameBoundaryBufferManager: frameBoundaryAllocator resize from %d to %d", frameBoundaryAllocator.getHeapSize(),
      newCapacity);

  frameBoundaryAllocator.resize(newCapacity);
}

FrameBoundaryElem FrameBoundaryBufferManager::createElem(TEXTUREID texture_id, IPoint2 frame_dim)
{
  int frameCnt = frame_dim.x * frame_dim.y;
  if (!frameBoundaryAllocator.canAllocate(frameCnt))
    resizeIncrement(frameCnt);

  RegionId regionId = frameBoundaryAllocator.allocateInHeap(frameCnt);

  FrameBoundaryElem elem(regionId, texture_id, frame_dim);

  frameBoundaryElemArr.push_back(elem);
  dirtyElemArr.push_back(elem);

  return elem;
}

void FrameBoundaryBufferManager::updateFrameElems(unsigned int currect_frame)
{
  if (dirtyElemArr.empty())
    return;

  if (currect_frame == lastUpdatedFrame)
    return;

  TIME_D3D_PROFILE(frame_boundary_update);


  for (auto it = dirtyElemArr.begin(); it != dirtyElemArr.end(); ++it)
  {
    const auto &elem = *it;
    if (!prefetch_and_check_managed_texture_loaded(elem.textureId, true))
      continue; // skip to another one which is potentially up-to-date

    IPoint2 textureSize;
    if (get_texture_size(elem.textureId, textureSize))
    {
      bool res = true;
      if (use_optimized_boundary_calc)
      {
        const Region &region = frameBoundaryAllocator.get(elem.regionId);

        ShaderGlobal::set_texture(dafx_fill_boundary_texVarId, elem.textureId);
        ShaderGlobal::set_buffer(dafx_frame_boundary_tmpVarId, frameBoundaryBufferTmp);
        ShaderGlobal::set_color4(dafx_fill_boundary_paramsVarId, elem.frameDim.x, elem.frameDim.y, textureSize.x, textureSize.y);
        ShaderGlobal::set_int(dafx_fill_boundary_offsetVarId, region.offset);
        ShaderGlobal::set_int(dafx_use_experimental_boundary_calcVarId, use_experimental_boundary_calc);

        {
          TIME_D3D_PROFILE(fillBoundaryOptStartCs);
          STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), frameBoundaryBufferTmp.getBuf());
          res = res && fillBoundaryOptStartCs->dispatchThreads(DAFX_FLIPBOOK_MAX_KEYFRAME_DIM * DAFX_FLIPBOOK_MAX_KEYFRAME_DIM, 1, 1);
        }
        d3d::resource_barrier({frameBoundaryBufferTmp.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
        {
          TIME_D3D_PROFILE(fillBoundaryOptCs);
          STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), frameBoundaryBufferTmp.getBuf());
          res = res && fillBoundaryOptCs->dispatchThreads(textureSize.x, textureSize.y, 1);
        }
        d3d::resource_barrier({frameBoundaryBufferTmp.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
        {
          TIME_D3D_PROFILE(fillBoundaryOptEndCs);
          STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), frameBoundaryBuffer.getBuf());
          res = res && fillBoundaryOptEndCs->dispatchThreads(elem.frameDim.x, elem.frameDim.y, 1);
          // if this one fails, the final result is still the same as before, no need to clear it
        }
      }
      else
      {
        TIME_D3D_PROFILE(fillBoundaryLegacyCs);
        STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), frameBoundaryBuffer.getBuf());

        const Region &region = frameBoundaryAllocator.get(elem.regionId);

        ShaderGlobal::set_texture(dafx_fill_boundary_texVarId, elem.textureId);
        ShaderGlobal::set_color4(dafx_fill_boundary_paramsVarId, elem.frameDim.x, elem.frameDim.y, textureSize.x, textureSize.y);
        ShaderGlobal::set_int(dafx_fill_boundary_offsetVarId, region.offset);

        res = fillBoundaryLegacyCs->dispatchThreads(elem.frameDim.x, elem.frameDim.y, 1);
      }

      if (!res)
        logerr("dafx: frameboundary update, dispatch error: %ld, (%d, %d)", (uint32_t)elem.textureId, elem.frameDim.x,
          elem.frameDim.y);

      d3d::resource_barrier({frameBoundaryBuffer.getBuf(), RB_RO_SRV | RB_STAGE_PIXEL});
    }
    else
    {
      logerr("dafx: frameboundary update, texture not found: %ld, (%d, %d)", (uint32_t)elem.textureId, elem.frameDim.x,
        elem.frameDim.y);
    }

    dirtyElemArr.erase(it);
    lastUpdatedFrame = currect_frame;
    break; // update only 1 elem per frame
  }

  ShaderGlobal::set_texture(dafx_fill_boundary_texVarId, BAD_TEXTUREID);
}


void FrameBoundaryBufferManager::updateDebugTexture()
{
  if (!debug_update)
    return;

  if (!debugTexture)
  {
    debugTexture.close();
    int fmt = TEXFMT_DEFAULT | TEXCF_UNORDERED;
    // unordered support coverage for ARGB16F is better than for ARGB8
    // use as fallback if needed
    if (!d3d::check_texformat(fmt))
      fmt = TEXFMT_A16B16G16R16F;
    debugTexture = dag::create_tex(NULL, DEBUG_TEX_SIZE, DEBUG_TEX_SIZE, fmt, 1, "dafx_frame_boundary_debug_tex");
    debugTexture->texfilter(TEXFILTER_POINT);
    debugTexture->texaddr(TEXADDR_CLAMP);
  }

  if (frameBoundaryElemArr.empty())
    return;

  const auto &elem = frameBoundaryElemArr[min<int>(debug_texture_id, frameBoundaryElemArr.size() - 1)];

  IPoint2 textureSize;
  if (!get_texture_size(elem.textureId, textureSize))
    return;

  const Region &region = frameBoundaryAllocator.get(elem.regionId);

  ShaderGlobal::set_int(dafx_fill_boundary_frame_idVarId, debug_frame_id);

  ShaderGlobal::set_texture(dafx_fill_boundary_texVarId, elem.textureId);
  ShaderGlobal::set_color4(dafx_fill_boundary_paramsVarId, elem.frameDim.x, elem.frameDim.y, textureSize.x, textureSize.y);
  ShaderGlobal::set_int(dafx_fill_boundary_offsetVarId, region.offset);

  STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), debugTexture.getTex2D());
  updateDebugTexCs->dispatchThreads(DEBUG_TEX_SIZE, DEBUG_TEX_SIZE, 1);
  d3d::resource_barrier({debugTexture.getTex2D(), RB_RO_SRV | RB_STAGE_ALL_SHADERS, 0, 0});

  ShaderGlobal::set_texture(dafx_fill_boundary_texVarId, BAD_TEXTUREID);
}


void FrameBoundaryBufferManager::resetFrameBoundaryResult()
{
  bufferElemCnt = max(bufferElemCnt, MIN_BUFFER_SIZE);

  debug("dafx: frameboundary reset, cnt: %d", bufferElemCnt);

  {
    String bufferName(128, "fx_dafx_frame_boundary_buffer_%p", this);
    frameBoundaryBuffer.close();
    frameBoundaryBuffer = dag::buffers::create_ua_sr_structured(sizeof(float) * 4, bufferElemCnt, bufferName.c_str());
  }

  {
    String bufferName(128, "fx_dafx_frame_boundary_buffer_tmp_%p", this);
    frameBoundaryBufferTmp.close();
    frameBoundaryBufferTmp = dag::buffers::create_ua_sr_structured(sizeof(uint32_t),
      4 * DAFX_FLIPBOOK_MAX_KEYFRAME_DIM * DAFX_FLIPBOOK_MAX_KEYFRAME_DIM, bufferName.c_str());
  }

  ShaderGlobal::set_int(dafx_fill_boundary_countVarId, bufferElemCnt);
  {
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), frameBoundaryBuffer.getBuf());
    clearBoundaryCs->dispatchThreads(bufferElemCnt, 1, 1);
  }
  d3d::resource_barrier({frameBoundaryBuffer.getBuf(), RB_RO_SRV | RB_STAGE_PIXEL});

  dirtyElemArr = frameBoundaryElemArr;
}

void FrameBoundaryBufferManager::drawDebugWindow()
{
#if DAGOR_DBGLEVEL > 0
  if (!debugTexture)
    return;

  static int mode = 0;
  String command(0, "render.show_tex %s", debugTexture->getTexName());

  bool modeChanged = ImGui::Button("Show debug texture");

  ImGui::SameLine();

  const char *textureSizeModes[4] = {"Original", "1000", "2000", "3000"};
  modeChanged = ImGui::Combo("Mode", &mode, textureSizeModes, 4) || modeChanged;
  if (mode > 0)
    command += " " + String(textureSizeModes[mode]);

  if (modeChanged)
    console::command(command.c_str());

  int textureMax = frameBoundaryElemArr.size() - 1;
  int textureId = clamp<int>(debug_texture_id, 0, textureMax);
  ImGui::SliderInt("Texture ID", &textureId, 0, textureMax);
  debug_texture_id = textureId;

  const auto &elem = frameBoundaryElemArr[debug_texture_id];

  ImGui::Text("Texture name: %s", get_managed_texture_name(elem.textureId));

  int frameMax = elem.frameDim.x * elem.frameDim.y - 1;
  int frameId = clamp<int>(debug_frame_id, 0, frameMax);
  ImGui::SliderInt("Frame ID", &frameId, 0, elem.frameDim.x * elem.frameDim.y - 1);
  debug_frame_id = frameId;
#endif
}

static void frameboundary_debug_window()
{
#if DAGOR_DBGLEVEL > 0
  WinAutoLock lock(g_frame_boundary_cs);

  bool bOptimize = use_optimized_boundary_calc;
  ImGui::Checkbox("Use optimized boundary calc", &bOptimize);
  use_optimized_boundary_calc = bOptimize;

  ImGui::SetNextItemOpen(debug_update.get());
  debug_update = ImGui::CollapsingHeader("Debug frameboundary");

  if (!debug_update)
    return;

  if (managers.size() == 0)
    return;

  static int managerIndex = 0;
  int managersMax = managers.size() - 1;
  managerIndex = clamp<int>(managerIndex, 0, managersMax);
  ImGui::SliderInt("Manager ID", &managerIndex, 0, managersMax);

  managers[managerIndex]->drawDebugWindow();
#endif
}


REGISTER_IMGUI_WINDOW("FX", "Frameboundary Debug", frameboundary_debug_window);