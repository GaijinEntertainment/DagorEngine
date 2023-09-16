#include <math/dag_color.h>
#include <ecs/core/entityManager.h>
#include <ecs/render/shaderVar.h>
#include <daECS/core/coreEvents.h>
#include "shader_var_from_component.h"

#define SHADER_VARS_OBJECT 1

ECS_TAG(render)
ECS_NO_ORDER
ECS_ON_EVENT(on_appear)
static __forceinline void shader_vars_es_event_handler(const ecs::Event &, const ecs::Object &shader_vars__vars,
  ecs::Object *shader_vars__original_vars)
{
  if (shader_vars__original_vars)
    shader_vars__original_vars->clear();
  for (auto &var : shader_vars__vars)
    set_shader_var(var.first.data(), var.second.getEntityComponentRef(), shader_vars__original_vars);
}

ECS_TAG(render)
ECS_TRACK(shader_vars__vars)
ECS_NO_ORDER
static __forceinline void shader_vars_track_es_event_handler(const ecs::Event &, const ecs::Object &shader_vars__vars)
{
  for (auto &var : shader_vars__vars)
    set_shader_var(var.first.data(), var.second.getEntityComponentRef(), nullptr);
}

ECS_NO_ORDER
ECS_ON_EVENT(on_disappear)
static __forceinline void shader_vars_destroyed_es_event_handler(const ecs::Event &, const ecs::Object &shader_vars__original_vars)
{
  for (const auto &var : shader_vars__original_vars)
    set_shader_var(var.first.data(), var.second.getEntityComponentRef(), nullptr);
}


struct ShaderVarConstruct final : public ShaderVar
{
public:
  ShaderVarConstruct() = default;
  ShaderVarConstruct(ecs::EntityManager &mgr, ecs::EntityId eid, ecs::component_index_t index)
  {
    const char *component_name = mgr.getDataComponents().getComponentNameById(index);
    eastl::string tmpName(eastl::string::CtorSprintf(), "%s_value", component_name);
    auto hash = ECS_HASH_SLOW(tmpName.c_str());

    *((ShaderVar *)this) = ShaderVar(component_name);

    auto compRef = mgr.getComponentRef(eid, hash);
    if (compRef.isNull())
      return;
    if (compRef.is<int>())
    {
      set_int(compRef.get<int>());
    }
    else if (compRef.is<float>())
    {
      set_real(compRef.get<float>());
    }
    else if (compRef.is<Point4>())
    {
      set_color4(compRef.get<Point4>());
    }
    else if (compRef.is<Point3>())
    {
      set_color4(compRef.get<Point3>());
    }
    else if (compRef.is<E3DCOLOR>())
    {
      set_color4(compRef.get<E3DCOLOR>());
    }
    else
    {
      logerr("%s has unsupported type <%s|0x%X>", tmpName.c_str(), mgr.getComponentTypes().findTypeName(compRef.getUserType()),
        compRef.getUserType());
    }
  }
};

ECS_REGISTER_MANAGED_TYPE(ShaderVar, nullptr, typename ecs::CreatorSelector<ShaderVar ECS_COMMA ShaderVarConstruct>::type);
