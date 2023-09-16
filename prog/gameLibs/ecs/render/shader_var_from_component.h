#pragma once
#include <daECS/core/entityComponent.h>
#include <math/dag_Point4.h>
#include <shaders/dag_shaders.h>

static Color4 inline to_color4(const Point4 &p) { return Color4(p.x, p.y, p.z, p.w); }
inline void set_shader_var(int varId, const ecs::EntityComponentRef in_attr)
{
  switch (in_attr.getUserType())
  {
    case ecs::ComponentTypeInfo<E3DCOLOR>::type: ShaderGlobal::set_color4(varId, color4(in_attr.get<E3DCOLOR>())); break;
    case ecs::ComponentTypeInfo<int>::type: ShaderGlobal::set_int(varId, in_attr.get<int>()); break;
    case ecs::ComponentTypeInfo<float>::type: ShaderGlobal::set_real(varId, in_attr.get<float>()); break;
    case ecs::ComponentTypeInfo<Point4>::type: ShaderGlobal::set_color4(varId, to_color4(in_attr.get<Point4>())); break;
    case ecs::ComponentTypeInfo<ecs::string>::type:
      ShaderGlobal::set_texture(varId, get_managed_texture_id(in_attr.get<ecs::string>().c_str()));
      break;
    case ecs::ComponentTypeInfo<IPoint3>::type:
    {
      const IPoint3 &p3 = in_attr.get<IPoint3>();
      ShaderGlobal::set_color4(varId, color4(E3DCOLOR(p3.x, p3.y, p3.z), 0));
    }
    break;
    default: break;
  }
}

inline bool set_shader_var(const char *name, const ecs::EntityComponentRef var)
{
  int varId = get_shader_variable_id(name, true);
  if (varId < 0)
    return false;
  set_shader_var(varId, var);
  return true;
}

static inline void get_shader_var(int varId, ecs::ChildComponent &in_attr) // no check
{
  switch (ShaderGlobal::get_var_type(varId))
  {
    case SHVT_INT: in_attr = ShaderGlobal::get_int_fast(varId); break;
    case SHVT_REAL: in_attr = ShaderGlobal::get_real_fast(varId); break;
    case SHVT_COLOR4:
    {
      Color4 c = ShaderGlobal::get_color4_fast(varId);
      in_attr = Point4(c.r, c.g, c.b, c.a);
    }
    break;
    case SHVT_TEXTURE:
    {
      const char *nm = get_managed_texture_name(ShaderGlobal::get_tex_fast(varId));
      in_attr = ecs::string(nm ? nm : "");
    }
    break;
  }
}

inline int get_shader_var_from_component(const char *shVarName, ecs::EntityComponentRef attr)
{
  int varId = get_shader_variable_id(shVarName, true);
  if (varId >= 0)
  {
    bool validType = true;
    const int varType = ShaderGlobal::get_var_type(varId);
    const ecs::component_type_t attrType = attr.getUserType();
#define COMPARE_TYPE(AttrType, VarType)                                                                                             \
  if (attrType == ecs::ComponentTypeInfo<AttrType>::type)                                                                           \
  {                                                                                                                                 \
    if (varType != SHVT_##VarType)                                                                                                  \
    {                                                                                                                               \
      logwarn("variable <%s> exists, but has different type <shader var has %d, attribute has %s>", shVarName, varType, #AttrType); \
      validType = false;                                                                                                            \
    }                                                                                                                               \
  }                                                                                                                                 \
  else
    COMPARE_TYPE(int, INT)
    COMPARE_TYPE(E3DCOLOR, COLOR4)
    COMPARE_TYPE(Point4, COLOR4)
    COMPARE_TYPE(float, REAL)
    COMPARE_TYPE(ecs::string, TEXTURE)
    COMPARE_TYPE(IPoint3, COLOR4) // treat as E3DCOLOR(xyz,0)
    {
      logwarn("variable <%s> exists, but has unsupported attribute type %d <shader var has type>", shVarName, attrType, varType);
      validType = false;
    }
    return validType ? varId : -1;
  }
  else
  {
    logwarn("variable <%s> not exists", shVarName);
    return -1;
  }
}

inline void set_shader_var(const char *name, const ecs::EntityComponentRef var, ecs::Object *shader_vars__original_vars)
{
  int varId = get_shader_var_from_component(name, var);
  if (varId >= 0)
  {
    if (shader_vars__original_vars)
      get_shader_var(varId, shader_vars__original_vars->insert(ECS_HASH_SLOW(name)));
    set_shader_var(varId, var);
  }
}
