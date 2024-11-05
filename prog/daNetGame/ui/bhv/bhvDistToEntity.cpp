// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvDistToEntity.h"

#include <ecs/scripts/sqEntity.h>

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_renderObject.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>

#include <3d/dag_render.h>

#include "ui/uiShared.h"

using namespace darg;


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvDistToEntity, bhv_dist_to_entity, cstr);

static const char *defFmtString = "%.f";

// Not super precise, but filters out dangerous format strings
static bool is_valid_format_string(const char *format, size_t len)
{
  size_t nPercents = 0;
  bool hasNumberHolder = false;

  for (size_t i = 0; i < len; ++i)
  {
    if (format[i] == '%')
    {
      if (i + 1 < len && format[i + 1] == '%') // escaped '%%'
      {
        ++i; // double '%'
        continue;
      }

      ++nPercents;
      if (nPercents > 1)
        return false;

      ++i;

      if (i >= len) // ends after '%'
        return false;

      // optional flags
      while (i < len && (format[i] == '+' || format[i] == '-' || format[i] == ' ' || format[i] == '#' || format[i] == '0'))
        ++i;

      while (i < len && isdigit(format[i])) // width specifier
        ++i;

      if (i < len && format[i] == '.') // precision specifier
      {
        ++i;
        while (i < len && isdigit(format[i]))
          ++i;
      }

      // floating-point format specifier
      if (i < len &&
          (format[i] == 'e' || format[i] == 'E' || format[i] == 'f' || format[i] == 'F' || format[i] == 'g' || format[i] == 'G'))
        hasNumberHolder = true;
      else
        return false;
    }
  }

  return nPercents == 1 && hasNumberHolder;
}


struct BhvDistToEntityData
{
  eastl::string formatString;
};


BhvDistToEntity::BhvDistToEntity() : Behavior(Behavior::STAGE_BEFORE_RENDER, 0) {}


void BhvDistToEntity::onAttach(Element *elem)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  BhvDistToEntityData *bhvData = new BhvDistToEntityData();
  elem->props.storage.SetValue(strings->distToEntityData, bhvData);

  Sqrat::Object fmtStringObj = elem->props.scriptDesc.RawGetSlot(strings->formatString);
  if (fmtStringObj.GetType() == OT_STRING)
  {
    Sqrat::Var<const char *> fmtString = fmtStringObj.GetVar<const char *>();
    if (is_valid_format_string(fmtString.value, fmtString.valueLen))
      bhvData->formatString = fmtString.value;
    else
    {
      LOGERR_ONCE("Invalid format string '%s' for DistToEntity behavior, using default '%s'", fmtString.value, defFmtString);
      bhvData->formatString = defFmtString;
    }
  }
  else
    bhvData->formatString = defFmtString;
}

void BhvDistToEntity::onDetach(Element *elem, DetachMode)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  BhvDistToEntityData *bhvData = elem->props.storage.RawGetSlotValue<BhvDistToEntityData *>(strings->distToEntityData, nullptr);
  if (bhvData)
  {
    elem->props.storage.DeleteSlot(strings->distToEntityData);
    delete bhvData;
  }
}


int BhvDistToEntity::update(UpdateStage /*stage*/, Element *elem, float /*dt*/)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  BhvDistToEntityData *bhvData = elem->props.storage.RawGetSlotValue<BhvDistToEntityData *>(strings->distToEntityData, nullptr);
  G_ASSERT_RETURN(bhvData, 0);

  bool wasHidden = elem->isHidden();

  Sqrat::Table &scriptDesc = elem->props.scriptDesc;
  ecs::EntityId targetEid = scriptDesc.RawGetSlotValue<ecs::EntityId>(strings->targetEid, ecs::INVALID_ENTITY_ID);
  if (targetEid == ecs::INVALID_ENTITY_ID || !uishared::has_valid_world_3d_view())
    elem->setHidden(true);
  else if (auto transform = g_entity_mgr->getNullable<TMatrix>(targetEid, ECS_HASH("transform")))
  {
    Point3 fromPos = uishared::view_itm.getcol(3);
    ecs::EntityId fromEid = scriptDesc.RawGetSlotValue<ecs::EntityId>(strings->fromEid, ecs::INVALID_ENTITY_ID);
    if (auto fromTransform = g_entity_mgr->getNullable<TMatrix>(fromEid, ECS_HASH("transform")))
      fromPos = fromTransform->getcol(3);
    float dist = length(fromPos - transform->getcol(3));
    float minDistance = scriptDesc.RawGetSlotValue(strings->minDistance, -100);
    if (dist > minDistance)
    {
      int newStrMaxLen = bhvData->formatString.size() + 8;
      char *newStr = (char *)alloca(newStrMaxLen);
      snprintf(newStr, newStrMaxLen, bhvData->formatString.c_str(), dist);
      if (strcmp(elem->props.text.c_str(), newStr) != 0)
      {
        discard_text_cache(elem->robjParams);
        elem->props.text = newStr;
      }

      elem->setHidden(false);
    }
    else
      elem->setHidden(true);
  }
  else
    elem->setHidden(true);

  return (elem->isHidden() != wasHidden) ? darg::R_REBUILD_RENDER_AND_INPUT_LISTS : 0;
}
