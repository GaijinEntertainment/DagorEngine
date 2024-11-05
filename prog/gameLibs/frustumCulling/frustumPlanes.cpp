// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaders.h>
#include <frustumCulling/frustumPlanes.h>

#define GLOBAL_VARS_LIST \
  VAR(frustumPlane03X)   \
  VAR(frustumPlane03Y)   \
  VAR(frustumPlane03Z)   \
  VAR(frustumPlane03W)   \
  VAR(frustumPlane4)     \
  VAR(frustumPlane5)

#define VAR(a) static ShaderVariableInfo a##VarId(#a);
GLOBAL_VARS_LIST
#undef VAR
#undef GLOBAL_VARS_LIST

void set_frustum_planes(const Frustum &frustum)
{
  ShaderGlobal::set_color4(frustumPlane03XVarId, Color4((float *)&frustum.plane03X));
  ShaderGlobal::set_color4(frustumPlane03YVarId, Color4((float *)&frustum.plane03Y));
  ShaderGlobal::set_color4(frustumPlane03ZVarId, Color4((float *)&frustum.plane03Z));
  ShaderGlobal::set_color4(frustumPlane03WVarId, Color4((float *)&frustum.plane03W));
  ShaderGlobal::set_color4(frustumPlane4VarId, Color4((float *)&frustum.camPlanes[4]));
  ShaderGlobal::set_color4(frustumPlane5VarId, Color4((float *)&frustum.camPlanes[5]));
}

static_assert(sizeof(Color4) == sizeof(vec4f));
static vec4f from_color4(const Color4 &color)
{
  vec4f result;
  memcpy(&result, &color, sizeof(color));
  return result;
}

static Frustum get_frustum_planes()
{
  Frustum frustum;
  // Only set the members used by set_frustum_planes.
  frustum.plane03X = from_color4(ShaderGlobal::get_color4(frustumPlane03XVarId));
  frustum.plane03Y = from_color4(ShaderGlobal::get_color4(frustumPlane03YVarId));
  frustum.plane03Z = from_color4(ShaderGlobal::get_color4(frustumPlane03ZVarId));
  frustum.plane03W = from_color4(ShaderGlobal::get_color4(frustumPlane03WVarId));
  frustum.camPlanes[4] = from_color4(ShaderGlobal::get_color4(frustumPlane4VarId));
  frustum.camPlanes[5] = from_color4(ShaderGlobal::get_color4(frustumPlane5VarId));

  return frustum;
}

ScopeFrustumPlanesShaderVars::ScopeFrustumPlanesShaderVars() : original(get_frustum_planes()) {}

ScopeFrustumPlanesShaderVars::ScopeFrustumPlanesShaderVars(const Frustum &frustum) : original(get_frustum_planes())
{
  set_frustum_planes(frustum);
}

ScopeFrustumPlanesShaderVars::~ScopeFrustumPlanesShaderVars() { set_frustum_planes(original); }
