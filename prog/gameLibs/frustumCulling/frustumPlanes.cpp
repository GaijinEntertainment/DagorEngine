#include <3d/dag_drv3d.h>
#include <shaders/dag_shaders.h>
#include <math/dag_frustum.h>

void set_frustum_planes(const Frustum &frustum)
{
#define GLOBAL_VARS_LIST \
  VAR(frustumPlane03X)   \
  VAR(frustumPlane03Y)   \
  VAR(frustumPlane03Z)   \
  VAR(frustumPlane03W)   \
  VAR(frustumPlane4)     \
  VAR(frustumPlane5)

#define VAR(a) static int a##VarId = ::get_shader_variable_id(#a);
  GLOBAL_VARS_LIST
#undef VAR
#undef GLOBAL_VARS_LIST

  ShaderGlobal::set_color4(frustumPlane03XVarId, Color4((float *)&frustum.plane03X));
  ShaderGlobal::set_color4(frustumPlane03YVarId, Color4((float *)&frustum.plane03Y));
  ShaderGlobal::set_color4(frustumPlane03ZVarId, Color4((float *)&frustum.plane03Z));
  ShaderGlobal::set_color4(frustumPlane03WVarId, Color4((float *)&frustum.plane03W));
  ShaderGlobal::set_color4(frustumPlane4VarId, Color4((float *)&frustum.camPlanes[4]));
  ShaderGlobal::set_color4(frustumPlane5VarId, Color4((float *)&frustum.camPlanes[5]));
}
