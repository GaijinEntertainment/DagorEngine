// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/vector.h>

#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entitySystem.h>
#include <math/dag_hlsl_floatx.h>
#include <render/renderer.h>
#include <ecs/render/updateStageRender.h>
#include <perfMon/dag_statDrv.h>

template <typename Callable>
inline void gather_nbs_spheres_ecs_query(Callable c);
template <typename Callable>
inline void update_nbs_buffers_ecs_query(Callable c);

struct NBSSpheresManager
{
  struct NBSSphere
  {
    float4 sphere;
    float centerDensity;
    float edgeDensity;
    float2 padding;
  };
  UniqueBufHolder spheresBuffer;
  int counterVarId = -1;
  int lastCount = 0;
};

ECS_DECLARE_RELOCATABLE_TYPE(NBSSpheresManager);
ECS_REGISTER_RELOCATABLE_TYPE(NBSSpheresManager, nullptr);

using TemporarySphereVec = eastl::vector<NBSSpheresManager::NBSSphere, framemem_allocator>;
using TemporaryHashMap =
  ska::flat_hash_map<ecs::string, TemporarySphereVec, eastl::hash<ecs::string>, eastl::equal_to<ecs::string>, framemem_allocator>;

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
static void update_nbs_volumes_es(const UpdateStageInfoBeforeRender &evt)
{
  TIME_D3D_PROFILE(nbs_update_spheres);

  TemporaryHashMap spheres;

  gather_nbs_spheres_ecs_query(
    [&](const ecs::string &nbs_sphere__class_name, const TMatrix &transform, const float nbs_sphere__center_density,
      const float nbs_sphere__radius, const float nbs_sphere__edge_density) {
      static constexpr float EPS = 1e-5f;
      const Point3 &position = transform.getcol(3);
      if (nbs_sphere__radius < EPS || !evt.froxelFogCullingFrustum.testSphereB(position, nbs_sphere__radius))
        return;

      NBSSpheresManager::NBSSphere sphere;
      sphere.sphere = Point4::xyzV(position, 1.0f / nbs_sphere__radius);
      sphere.centerDensity = nbs_sphere__center_density;
      sphere.edgeDensity = nbs_sphere__edge_density;
      spheres[nbs_sphere__class_name].push_back(sphere);
    });

  update_nbs_buffers_ecs_query([&](NBSSpheresManager &nbs_spheres_manager, const ecs::string &nbs_spheres_manager__class_name) {
    int count = 0;
    auto managerIt = spheres.find(nbs_spheres_manager__class_name);
    if (managerIt != spheres.end())
    {
      const TemporarySphereVec &sphereVec = managerIt->second;
      if (
        !nbs_spheres_manager.spheresBuffer.getBuf() || sphereVec.size() > nbs_spheres_manager.spheresBuffer.getBuf()->getNumElements())
      {
        String name(128, "nbs_spheres_%s", nbs_spheres_manager__class_name.c_str());
        nbs_spheres_manager.counterVarId = ::get_shader_variable_id(name + "_counter", true);
        get_world_renderer()->invalidateNodeBasedResources();
        nbs_spheres_manager.spheresBuffer.close();
        nbs_spheres_manager.spheresBuffer = dag::create_sbuffer(sizeof(NBSSpheresManager::NBSSphere), sphereVec.size(),
          SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED, 0, name.c_str());
      }
      nbs_spheres_manager.spheresBuffer.getBuf()->updateData(0, sizeof(NBSSpheresManager::NBSSphere) * sphereVec.size(),
        sphereVec.data(), VBLOCK_WRITEONLY);
      count = sphereVec.size();
    }
    if (count != nbs_spheres_manager.lastCount)
    {
      nbs_spheres_manager.lastCount = count;
      ShaderGlobal::set_int(nbs_spheres_manager.counterVarId, count);
    }
  });
}

ECS_ON_EVENT(on_disappear)
static inline void destroy_nbs_es_event_handler(const ecs::Event &, NBSSpheresManager &nbs_spheres_manager)
{
  get_world_renderer()->invalidateNodeBasedResources();
  ShaderGlobal::set_int(nbs_spheres_manager.counterVarId, 0);
  nbs_spheres_manager.spheresBuffer.close();
}
