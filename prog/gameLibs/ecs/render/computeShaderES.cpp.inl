#include <ecs/render/compute_shader.h>
#include <daECS/core/componentTypes.h>

struct ComputeShaderConstruct final : public ComputeShader
{
  static const char *get_shader_name(ecs::EntityManager &mgr, ecs::EntityId eid, ecs::component_index_t index)
  {
    const char *component_name = mgr.getDataComponents().getComponentNameById(index);
    eastl::string tmpName(eastl::string::CtorSprintf(), "%s_name", component_name);
    if (const ecs::string *shader_name = mgr.getNullable<ecs::string>(eid, ECS_HASH_SLOW(tmpName.c_str())))
      return shader_name->c_str();
    return nullptr;
  }

public:
  ComputeShaderConstruct() = default;
  ComputeShaderConstruct(ecs::EntityManager &mgr, ecs::EntityId eid, ecs::component_index_t index) :
    ComputeShader(get_shader_name(mgr, eid, index))
  {}
};

ECS_REGISTER_MANAGED_TYPE(ComputeShader, nullptr, typename ecs::CreatorSelector<ComputeShader ECS_COMMA ComputeShaderConstruct>::type);
