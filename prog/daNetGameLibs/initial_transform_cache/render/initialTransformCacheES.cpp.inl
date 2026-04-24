// Copyright (C) Gaijin Games KFT.  All rights reserved.

// gamelibs
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <ecs/rendInst/riExtra.h>
#include <ecs/render/updateStageRender.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtra.h>

// dagorincludes
#include <render/world/global_vars.h>

// DNG includes
#include <camera/sceneCam.h>
#include <render/renderEvent.h>

// Debug
#if DAGOR_DBGLEVEL > 0
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#endif

constexpr float MAX_DIST = 100.0f;
constexpr float MAX_DIST_SQR = MAX_DIST * MAX_DIST;

class ITC_Entry
{
public:
  ITC_Entry() = default;
  ITC_Entry(rendinst::riex_handle_t _ri_handle, const TMatrix &tm) : isValid(true), position(tm.getcol(3)), ri_handle(_ri_handle)
  {
    rendinst::RiExtraPerInstanceGpuItem data[4];

    data[0] = {bitwise_cast<uint32_t>(tm.m[0][0]), bitwise_cast<uint32_t>(tm.m[0][1]), bitwise_cast<uint32_t>(tm.m[0][2]),
      bitwise_cast<uint32_t>(0.0f)};
    data[1] = {bitwise_cast<uint32_t>(tm.m[1][0]), bitwise_cast<uint32_t>(tm.m[1][1]), bitwise_cast<uint32_t>(tm.m[1][2]),
      bitwise_cast<uint32_t>(0.0f)};
    data[2] = {bitwise_cast<uint32_t>(tm.m[2][0]), bitwise_cast<uint32_t>(tm.m[2][1]), bitwise_cast<uint32_t>(tm.m[2][2]),
      bitwise_cast<uint32_t>(0.0f)};
    data[3] = {bitwise_cast<uint32_t>(tm.m[3][0]), bitwise_cast<uint32_t>(tm.m[3][1]), bitwise_cast<uint32_t>(tm.m[3][2]),
      bitwise_cast<uint32_t>(1.0f)};

    uint64_t posDataId = rendinst::setRiExtraPerInstanceRenderAdditionalData(ri_handle,
      rendinst::RiExtraPerInstanceDataType::INITIAL_TM_POS, rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT, data + 3, 1);
    G_UNUSED(posDataId); // used in assert
    uint64_t rotDataId = rendinst::setRiExtraPerInstanceRenderAdditionalData(ri_handle,
      rendinst::RiExtraPerInstanceDataType::INITIAL_TM_3X3, rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT, data, 3);
    G_ASSERTF(posDataId == rotDataId, "Initial transform data IDs should be the same for both data types");
    dataId = rotDataId;
  }
  ITC_Entry(const ITC_Entry &) = delete;
  ITC_Entry &operator=(const ITC_Entry &) = delete;
  ITC_Entry(ITC_Entry &&other) noexcept : isValid(other.isValid), position(eastl::move(other.position)), dataId(other.dataId)
  {
    other.isValid = false;
  }
  ITC_Entry &operator=(ITC_Entry &&other) noexcept
  {
    if (this == &other)
      return *this;
    close();
    isValid = other.isValid;
    position = eastl::move(other.position);
    ri_handle = eastl::move(other.ri_handle);
    dataId = eastl::move(other.dataId);
    other.isValid = false;
    return *this;
  }
  ~ITC_Entry() { close(); }

  [[nodiscard]] rendinst::riex_handle_t getHandle() const { return ri_handle; }
  [[nodiscard]] const Point3 &getPos() const { return position; }

private:
  bool isValid = false;
  Point3 position;
  rendinst::riex_handle_t ri_handle;
  uint64_t dataId = 0;

  void close()
  {
    if (isValid)
    {
      rendinst::removeRiExtraPerInstanceRenderAdditionalData(dataId, rendinst::RiExtraPerInstanceDataType::INITIAL_TM_POS);
      rendinst::removeRiExtraPerInstanceRenderAdditionalData(dataId, rendinst::RiExtraPerInstanceDataType::INITIAL_TM_3X3);
      isValid = false;
    }
  }
};

namespace dag
{
// hack for enable move semantics for ozz::animation::SamplingJob::Context
template <>
struct is_type_relocatable<ITC_Entry>
{
  static constexpr bool value = true;
};
}; // namespace dag

// size is 64 since RI extra instance data is maximum 1024 bytes, and we store 16 bytes per handle
using PositionsVector = dag::RelocatableFixedVector<ITC_Entry, 64, true>;

class InitialTransformCacheManager
{
private:
  PositionsVector handles;

public:
  InitialTransformCacheManager() = default;
  InitialTransformCacheManager(const InitialTransformCacheManager &) = delete;
  InitialTransformCacheManager &operator=(const InitialTransformCacheManager &) = delete;
  InitialTransformCacheManager(InitialTransformCacheManager &&) = default;
  InitialTransformCacheManager &operator=(InitialTransformCacheManager &&) = default;

  void add(rendinst::riex_handle_t handle, const TMatrix &tm, const Point3 &heroPos)
  {
    if (rendinst::getRiExtraPerInstanceRenderAdditionalDataFlags(handle) &
        static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::INITIAL_TM_POS))
    {
      G_ASSERT(rendinst::getRiExtraPerInstanceRenderAdditionalDataFlags(handle) &
               static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::INITIAL_TM_3X3));
      return;
    }

    Point3 diff = heroPos - tm.getcol(3);
    float distSqr = diff * diff;

    if (handles.size() >= handles.capacity())
    {
      auto furthest = eastl::max_element(handles.begin(), handles.end(), [&](const ITC_Entry &a, const ITC_Entry &b) {
        Point3 diffA = heroPos - a.getPos();
        Point3 diffB = heroPos - b.getPos();
        return (diffA * diffA) < (diffB * diffB);
      });

      Point3 furthestDiff = heroPos - furthest->getPos();
      float furthestDistSqr = furthestDiff * furthestDiff;

      if (distSqr >= furthestDistSqr)
        return;

      handles.erase(furthest);
    }

    handles.emplace_back(handle, tm);
  }

  void update(const Point3 &heroPos)
  {
    TIME_PROFILE(initial_transform_cache__remove_far_ri);
    handles.erase(eastl::remove_if(handles.begin(), handles.end(),
                    [&](const ITC_Entry &p) {
                      Point3 diff = heroPos - p.getPos();
                      float distSqr = diff * diff;
                      return distSqr > MAX_DIST_SQR;
                    }),
      handles.end());
  }

  const auto &getHandles() const { return handles; }
};

template <class T>
inline void initial_transform_cache_get_manager_ecs_query(ecs::EntityManager &manager, T b);

ECS_TAG(render)
ECS_ON_EVENT(OnLevelLoaded)
static void initial_transform_cache_create_manager_es(const ecs::Event &, ecs::EntityManager &manager)
{
  manager.getOrCreateSingletonEntity(ECS_HASH("initial_transform_cache_manager"));
}

ECS_TAG(render)
static void initial_transform_cache_add_fix_globtm_es(
  const EventOnRendinstMovement &event, ecs::EntityManager &manager, ecs::EntityId eid)
{
  rendinst::riex_handle_t handle = event.get<0>();
  TMatrix prevTransform = event.get<1>();

  const Point3 &camPos = get_cam_itm().getcol(3);
  Point3 diff = camPos - prevTransform.getcol(3);

  if (diff * diff > MAX_DIST_SQR)
    return;

  initial_transform_cache_get_manager_ecs_query(manager, [&](InitialTransformCacheManager &initial_transform_cache_manager) {
    initial_transform_cache_manager.add(handle, prevTransform, camPos);
  });

  G_UNREFERENCED(eid);
}


ECS_DECLARE_RELOCATABLE_TYPE(InitialTransformCacheManager);
ECS_REGISTER_RELOCATABLE_TYPE(InitialTransformCacheManager, nullptr);
ECS_AUTO_REGISTER_COMPONENT(InitialTransformCacheManager, "initial_transform_cache_manager", nullptr, 0);

#if DAGOR_DBGLEVEL > 0
namespace initialtransformcache_imgui
{
static void imguiWindow()
{
  static bool displayData = false;
  ImGui::Checkbox("Display data?", &displayData);

  if (displayData)
  {
    initial_transform_cache_get_manager_ecs_query(*g_entity_mgr, [&](InitialTransformCacheManager &initial_transform_cache_manager) {
      const auto &handles = initial_transform_cache_manager.getHandles();

      for (int i = 0; i < handles.size(); i++)
      {
        ImGui::Text("Handle %llu", (unsigned long long)handles[i].getHandle());
        Point3 pos = handles[i].getPos();
        ImGui::Text("Position: %f %f %f", pos.x, pos.y, pos.z);
        uint32_t offset, dataFlags, optionalFlags;
        rendinst::getRiExtraPerInstanceRenderAdditionalData(handles[i].getHandle(), offset, dataFlags, optionalFlags);
        ImGui::Text("Offset: %u", offset);
        ImGui::Separator();
      }
    });
  }
}

REGISTER_IMGUI_WINDOW("Render", "InitialTransformCache", imguiWindow);
} // namespace initialtransformcache_imgui
#endif // DAGOR_DBGLEVEL > 0
