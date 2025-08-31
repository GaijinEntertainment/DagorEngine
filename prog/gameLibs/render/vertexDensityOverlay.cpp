// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/vertexDensityOverlay.h>
#include <3d/dag_resPtr.h>
#include <util/dag_convar.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_driver.h>
#include <shaders/dag_postFxRenderer.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>

#if DAGOR_DBGLEVEL > 0

CONSOLE_INT_VAL("vertexDensity", uavSize, 4096, 0, 0xFFFF);
CONSOLE_INT_VAL("vertexDensity", acmRadius, 8, 1, 32);
CONSOLE_INT_VAL("vertexDensity", acmWeight, 1, 0, 32);
CONSOLE_FLOAT_VAL("vertexDensity", overlayBlendFactor, 0.5f);
CONSOLE_BOOL_VAL("vertexDensity", enableOverlay, false);

namespace
{
UniqueBufHolder uav;
int uavBindIdx;
int halfSzVarId;
int vertexDensityAcmRadiusVarId;
int vertexDensityAcmWeightVarId;
int vertexDensityOverlayBlendFactorVarId;
PostFxRenderer overlayPfx;
bool inited = false;
} // anonymous namespace

void VertexDensityOverlay::init()
{
  if (!::dgs_get_settings()->getBlockByNameEx("debug")->getBool("enableVertexDensityOverlay", false))
    return;

  halfSzVarId = ::get_shader_variable_id("vertexDensityHalfSize", true);
  if (!halfSzVarId)
  {
    debug("vertexDensityOverlay: off, no half size varid");
    return;
  }

  int bindIdxVarId = ::get_shader_variable_id("vertexDensityUAVReg", true);
  if (!bindIdxVarId)
  {
    debug("vertexDensityOverlay: off, no UAV reg varid");
    return;
  }

  if (!shader_exists("vertexDensityOverlay"))
  {
    debug("vertexDensityOverlay: off, no overlay shader");
    return;
  }
  overlayPfx.init("vertexDensityOverlay");

  vertexDensityAcmRadiusVarId = ::get_shader_variable_id("vertexDensityAcmRadius", true);
  vertexDensityAcmWeightVarId = ::get_shader_variable_id("vertexDensityAcmWeight", true);
  vertexDensityOverlayBlendFactorVarId = ::get_shader_variable_id("vertexDensityOverlayBlendFactor", true);

  uavBindIdx = ShaderGlobal::get_int(bindIdxVarId);
  inited = true;
  debug("vertexDensityOverlay: initialized");
}

void VertexDensityOverlay::shutdown()
{
  if (!inited)
    return;

  uav.close();
  overlayPfx.clear();
  inited = false;
  debug("vertexDensityOverlay: shutdown");
}

void VertexDensityOverlay::before_render()
{
  if (!inited)
    return;

  if (!uav || uavSize.pullValueChange())
  {
    // align user input to 2
    uavSize.set(uavSize.get() & ~1ul);
    uavSize.pullValueChange();

    ShaderGlobal::set_int(halfSzVarId, uavSize.get() >> 1);

    uav.close();
    uav = UniqueBufHolder(
      dag::create_sbuffer(1, uavSize.get() * uavSize.get(), SBCF_BIND_SHADER_RES | SBCF_BIND_UNORDERED, 0, "vertexDensityAccumulator"),
      "vertexDensityAccumulator");
    uav.setVar();
  }

  if (acmRadius.pullValueChange())
    ShaderGlobal::set_int(vertexDensityAcmRadiusVarId, acmRadius.get());
  if (acmWeight.pullValueChange())
    ShaderGlobal::set_int(vertexDensityAcmWeightVarId, acmWeight.get());
  if (overlayBlendFactor.pullValueChange())
    ShaderGlobal::set_real(vertexDensityOverlayBlendFactorVarId, overlayBlendFactor.get());

  d3d::zero_rwbufi(uav.getBuf());
}

void VertexDensityOverlay::bind_UAV()
{
  if (!inited)
    return;
  d3d::set_rwbuffer(STAGE_VS, uavBindIdx, uav.getBuf());
}

void VertexDensityOverlay::unbind_UAV()
{
  if (!inited)
    return;
  d3d::set_rwbuffer(STAGE_VS, uavBindIdx, nullptr);
}

void VertexDensityOverlay::draw_heatmap()
{
  if (!inited || !enableOverlay.get())
    return;
  overlayPfx.render();
}

#else

void VertexDensityOverlay::init() {}
void VertexDensityOverlay::shutdown() {}
void VertexDensityOverlay::before_render() {}
void VertexDensityOverlay::bind_UAV() {}
void VertexDensityOverlay::unbind_UAV() {}
void VertexDensityOverlay::draw_heatmap() {}

#endif