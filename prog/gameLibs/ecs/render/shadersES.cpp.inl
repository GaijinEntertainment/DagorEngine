// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/render/shaders.h>

ShadersECS::operator bool() const { return shmat && shElem; }

struct ShadersECSConstruct final : public ShadersECS
{
public:
  ShadersECSConstruct() = default;
  ShadersECSConstruct(ecs::EntityManager &mgr, ecs::EntityId eid, ecs::component_index_t index)
  {
    const char *component_name = mgr.getDataComponents().getComponentNameById(index);
    eastl::string tmpName(eastl::string::CtorSprintf(), "%s_name", component_name);
    const ecs::string *shader_name = mgr.getNullable<ecs::string>(eid, ECS_HASH_SLOW(tmpName.c_str()));
    if (shader_name)
    {
      shmat = new_shader_material_by_name(shader_name->c_str());
      if (!shmat)
        logerr("no shader %s for %s", shader_name->c_str(), component_name);
      else
        shElem = shmat->make_elem();
    }
  }
};

ECS_REGISTER_MANAGED_TYPE(ShadersECS, nullptr, typename ecs::CreatorSelector<ShadersECS ECS_COMMA ShadersECSConstruct>::type);
