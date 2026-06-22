// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daFx/dafx.h>
#include "dafxModFx_decl.h"

namespace dafx_ex
{
struct SystemInfo;
struct EmitterDebug;
} // namespace dafx_ex

struct ModFxSystemDesc
{
  FxSpawn parSpawn;
  FxLife parLife;
  FxPos parPos;
  FxRadius parRadius;
  FxColor parColor;
  FxRotation parRotation;
  FxVelocity parVelocity;
  FxPlacement parPlacement;
  FxTexture parTex;
  FxEmission parColorEmission;
  FxThermalEmission parThermalEmission;
  FxLighting parLighting;
  FxRenderShape parRenderShape;
  FxBlending parBlending;
  FxDepthMask parDepthMask;
  FxRenderGroup parRenderGroup;
  FxRenderShader parRenderShader;
  FxRenderVolfogInjection parVolfogInjection;
  FxPartTrimming parPartTrimming;
  FxGlobalParams parGlobals;
  FxQuality parQuality;
};

bool dafx_modfx_system_desc_load(const char *ptr, int len, BaseParamScriptLoadCB *load_cb, ModFxSystemDesc &out_modfx_sdesc);
bool dafx_modfx_system_load(const ModFxSystemDesc &modfx_sdesc, dafx::ContextId ctx, dafx::SystemDesc &sdesc,
  dafx_ex::SystemInfo &sinfo, dafx_ex::EmitterDebug *&emitter_debug);

dafx::ContextId dafx_modfx_get_context();
