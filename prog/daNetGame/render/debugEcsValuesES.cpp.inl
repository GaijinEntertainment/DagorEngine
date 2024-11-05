// Copyright (C) Gaijin Games KFT.  All rights reserved.

/// Usage:
///  Place the following code into the chosen entity template. It will display the testPoint component on the screen above the entity,
///  when debugging is turned on. To turn on debugging, use this console command: ecs.debugValues
///
///  entity_template {
///   "ecs_debug__debugValues:array" {
///     ecs_debug__debugValues:t="testPoint" // reference the component to be displayed by its name
///   }
///   testPoint:p3=1, 2, 3 // component to be displayed
///  }

#include <ecs/core/entityManager.h>
#include <util/dag_convar.h>
#include <util/dag_string.h>
#include <debug/dag_textMarks.h>

#include <ecs/render/updateStageRender.h>
#include "camera/sceneCam.h"

ConVarB ecs_debug_values("ecs.debugValues", false, nullptr);

template <typename Callable>
static void items_with_debug_values_ecs_query(Callable c);

ECS_TAG(render, dev)
ECS_NO_ORDER
static void debug_draw_ecs_values_es(const UpdateStageInfoRenderDebug &)
{
  if (!ecs_debug_values.get())
    return;

  items_with_debug_values_ecs_query([](ecs::EntityId eid, const TMatrix &transform, const ecs::Array &ecs_debug__debugValues,
                                      const Point3 &ecs_debug__debugValuesOffset = Point3(0.f, 1.f, 0.f)) {
    int line = 0;
    String valueStr{};
    for (auto &value : ecs_debug__debugValues)
    {
      if (!value.is<ecs::string>())
      {
        static bool reported = false;
        if (!reported)
        {
          logerr("Values in the ecs_debug__debugValues must be strings, referencing other components of the entity");
          reported = true;
        }
        continue;
      }
      auto &&component = g_entity_mgr->getComponentRef(eid, ECS_HASH_SLOW(value.getStr()));
      if (component.is<bool>())
        valueStr.printf(16, "%s : %@", value.getStr(), component.get<bool>());
      else if (component.is<int>())
        valueStr.printf(16, "%s : %@", value.getStr(), component.get<int>());
      else if (component.is<float>())
        valueStr.printf(16, "%s : %@", value.getStr(), component.get<float>());
      else if (component.is<ecs::string>())
        valueStr.printf(16, "%s : %@", value.getStr(), component.get<ecs::string>());
      else if (component.is<Point3>())
        valueStr.printf(16, "%s : %@", value.getStr(), component.get<Point3>());
      else if (component.is<Point2>())
        valueStr.printf(16, "%s : %@", value.getStr(), component.get<Point2>());
      else if (component.is<TMatrix>())
        valueStr.printf(32, "%s : %@", value.getStr(), component.get<TMatrix>());
      else if (component.is<ecs::Tag>())
        valueStr.printf(16, "%s : [%stag]", value.getStr(), component.getNullable<ecs::Tag>() == nullptr ? "no " : "");
      else
      {
        const char *typeInfo = g_entity_mgr->getComponentTypes().getTypeNameById(component.getTypeId());
        valueStr.printf(32, "%s : #<%s>", value.getStr(), typeInfo);
      }
      add_debug_text_mark(transform.getcol(3) + ecs_debug__debugValuesOffset, valueStr, -1, line);
      valueStr.clear();
      line++;
    }
  });
}
