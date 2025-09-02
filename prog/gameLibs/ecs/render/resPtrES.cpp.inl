// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/render/resPtr.h>

struct SharedTexConstruct final : public SharedTex
{
public:
  SharedTexConstruct() = default;
  SharedTexConstruct(ecs::EntityManager &mgr, ecs::EntityId eid, ecs::component_index_t index)
  {
    const char *component_name = mgr.getDataComponents().getComponentNameById(index);
    eastl::string tmpName(eastl::string::CtorSprintf(), "%s_res", component_name);
    *((SharedTex *)this) = dag::get_tex_gameres(mgr.getOr(eid, ECS_HASH_SLOW(tmpName.c_str()), ""));
  }
};

ECS_REGISTER_MANAGED_TYPE(SharedTex, nullptr, typename ecs::CreatorSelector<SharedTex ECS_COMMA SharedTexConstruct>::type);

struct SharedTexHolderConstruct final : public SharedTexHolder
{
public:
  SharedTexHolderConstruct() = default;
  SharedTexHolderConstruct(ecs::EntityManager &mgr, ecs::EntityId eid, ecs::component_index_t index)
  {
    const char *component_name = mgr.getDataComponents().getComponentNameById(index);
    eastl::string tmpName(eastl::string::CtorSprintf(), "%s_res", component_name);
    const ecs::string *gameresName = mgr.getNullable<ecs::string>(eid, ECS_HASH_SLOW(tmpName.c_str()));
    tmpName = eastl::string(eastl::string::CtorSprintf(), "%s_var", component_name);
    const ecs::string *shaderVarName = mgr.getNullable<ecs::string>(eid, ECS_HASH_SLOW(tmpName.c_str()));
    if (gameresName == nullptr || shaderVarName == nullptr)
    {
      *((SharedTexHolder *)this) = SharedTexHolder();
      return;
    }
    *((SharedTexHolder *)this) = dag::get_tex_gameres(gameresName->c_str(), shaderVarName->c_str());
    tmpName = eastl::string(eastl::string::CtorSprintf(), "%s_samplerstate", shaderVarName->c_str());
    ShaderGlobal::set_sampler(get_shader_variable_id(tmpName.c_str(), true), d3d::request_sampler({}));
  }
};
ECS_REGISTER_MANAGED_TYPE(SharedTexHolder, nullptr,
  typename ecs::CreatorSelector<SharedTexHolder ECS_COMMA SharedTexHolderConstruct>::type);

// UniqueTexHolder hasn't specific constructor
ECS_REGISTER_RELOCATABLE_TYPE(UniqueTex, nullptr);
ECS_REGISTER_RELOCATABLE_TYPE(UniqueTexHolder, nullptr);
ECS_REGISTER_RELOCATABLE_TYPE(UniqueBuf, nullptr);
ECS_REGISTER_RELOCATABLE_TYPE(UniqueBufHolder, nullptr);
