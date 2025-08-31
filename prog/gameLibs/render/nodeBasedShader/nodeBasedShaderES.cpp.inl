// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <daECS/core/entitySystem.h>
#include <render/nodeBasedShader.h>
#include <EASTL/vector_set.h>


template <typename Callable>
inline void gather_nbs_sphere_managers_ecs_query(Callable c);

eastl::vector<String> nodebasedshaderutils::getAvailableVolumeChannels()
{
  eastl::vector_set<String> managers;

  gather_nbs_sphere_managers_ecs_query([&](const ecs::string &nbs_spheres_manager__class_name) {
    managers.push_back_unsorted(String(128, "nbs_spheres_%s", nbs_spheres_manager__class_name.c_str()));
  });

  return managers;
}