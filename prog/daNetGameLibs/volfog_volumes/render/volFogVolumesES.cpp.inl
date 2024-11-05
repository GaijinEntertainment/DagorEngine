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
inline void gather_volfog_spheres_ecs_query(Callable c);
template <typename Callable>
inline void update_volfog_buffers_ecs_query(Callable c);

struct VolFogSpheresManager
{
  struct VolFogSphere
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

ECS_DECLARE_RELOCATABLE_TYPE(VolFogSpheresManager);
ECS_REGISTER_RELOCATABLE_TYPE(VolFogSpheresManager, nullptr);

using TemporarySphereVec = eastl::vector<VolFogSpheresManager::VolFogSphere, framemem_allocator>;
using TemporaryHashMap =
  ska::flat_hash_map<ecs::string, TemporarySphereVec, eastl::hash<ecs::string>, eastl::equal_to<ecs::string>, framemem_allocator>;

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
static void update_volfog_volumes_es(const UpdateStageInfoBeforeRender &evt)
{
  TIME_D3D_PROFILE(volfog_update_spheres);

  TemporaryHashMap spheres;

  gather_volfog_spheres_ecs_query(
    [&](const ecs::string &volfog_sphere__class_name, const TMatrix &transform, const float volfog_sphere__center_density,
      const float volfog_sphere__radius, const float volfog_sphere__edge_density) {
      static constexpr float EPS = 1e-5f;
      const Point3 &position = transform.getcol(3);
      if (volfog_sphere__radius < EPS || !evt.froxelFogCullingFrustum.testSphereB(position, volfog_sphere__radius))
        return;

      VolFogSpheresManager::VolFogSphere sphere;
      sphere.sphere = Point4::xyzV(position, 1.0f / volfog_sphere__radius);
      sphere.centerDensity = volfog_sphere__center_density;
      sphere.edgeDensity = volfog_sphere__edge_density;
      spheres[volfog_sphere__class_name].push_back(sphere);
    });

  update_volfog_buffers_ecs_query(
    [&](VolFogSpheresManager &volfog_spheres_manager, const ecs::string &volfog_spheres_manager__class_name) {
      int count = 0;
      auto managerIt = spheres.find(volfog_spheres_manager__class_name);
      if (managerIt != spheres.end())
      {
        const TemporarySphereVec &sphereVec = managerIt->second;
        if (!volfog_spheres_manager.spheresBuffer.getBuf() ||
            sphereVec.size() > volfog_spheres_manager.spheresBuffer.getBuf()->getNumElements())
        {
          String name(128, "volfog_spheres_%s", volfog_spheres_manager__class_name.c_str());
          volfog_spheres_manager.counterVarId = ::get_shader_variable_id(name + "_counter", true);
          get_world_renderer()->invalidateNodeBasedResources();
          volfog_spheres_manager.spheresBuffer.close();
          volfog_spheres_manager.spheresBuffer = dag::create_sbuffer(sizeof(VolFogSpheresManager::VolFogSphere), sphereVec.size(),
            SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED, 0, name.c_str());
        }
        volfog_spheres_manager.spheresBuffer.getBuf()->updateData(0, sizeof(VolFogSpheresManager::VolFogSphere) * sphereVec.size(),
          sphereVec.data(), VBLOCK_WRITEONLY);
        count = sphereVec.size();
      }
      if (count != volfog_spheres_manager.lastCount)
      {
        volfog_spheres_manager.lastCount = count;
        ShaderGlobal::set_int(volfog_spheres_manager.counterVarId, count);
      }
    });
}

ECS_ON_EVENT(on_disappear)
static inline void destroy_volfog_es_event_handler(const ecs::Event &, VolFogSpheresManager &volfog_spheres_manager)
{
  get_world_renderer()->invalidateNodeBasedResources();
  ShaderGlobal::set_int(volfog_spheres_manager.counterVarId, 0);
  volfog_spheres_manager.spheresBuffer.close();
}
