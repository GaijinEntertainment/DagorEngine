#include <ecs/input/rumbleEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <math/dag_mathUtils.h>
#include <forceFeedback/forceFeedback.h>

#include <daInput/input_api.h>

#define RUMBLE_ECS_EVENT(x, ...) ECS_REGISTER_EVENT_NS(rumble, x)
RUMBLE_ECS_EVENTS
#undef RUMBLE_ECS_EVENT

ECS_TAG(input)
ECS_REQUIRE(ecs::Tag human_input__rumbleEnabled)
static void process_rumble_es_event_handler(const rumble::CmdRumble &evt, const ecs::Object &human_input__rumbleEvents)
{
  const ecs::HashedConstString eventName = ECS_HASH_SLOW(evt.get<0>().c_str());
  const ecs::Object *eventDesc = human_input__rumbleEvents.getNullable<ecs::Object>(eventName);
  if (!eventDesc)
    return;
  const int overrideDuration = evt.get<1>();

  const bool isHighBand = eventDesc->getMemberOr(ECS_HASH("isHighBand"), false);
  const float freq = eventDesc->getMemberOr(ECS_HASH("freq"), 0.f);
  const int durationMsec = overrideDuration ? overrideDuration : eventDesc->getMemberOr(ECS_HASH("durationMsec"), 0);

  using namespace force_feedback::rumble;
  const enum EventParams::Band band = isHighBand ? EventParams::HI : EventParams::LO;
  add_event({band, freq, durationMsec});
}

ECS_TAG(input)
ECS_REQUIRE(ecs::Tag human_input__rumbleEnabled)
static void process_scaled_rumble_es_event_handler(const rumble::CmdScaledRumble &evt, const ecs::Object &human_input__rumbleEvents)
{
  const ecs::HashedConstString eventMinName = ECS_HASH_SLOW(evt.get<0>().c_str());
  const ecs::HashedConstString eventMaxName = ECS_HASH_SLOW(evt.get<1>().c_str());
  const ecs::Object *eventMinDesc = human_input__rumbleEvents.getNullable<ecs::Object>(eventMinName);
  const ecs::Object *eventMaxDesc = human_input__rumbleEvents.getNullable<ecs::Object>(eventMaxName);
  if (!eventMinDesc || !eventMaxDesc)
    return;
  const float scale = evt.get<2>();
  if (scale <= 0.f)
    return;

  const bool isHighBand = eventMinDesc->getMemberOr(ECS_HASH("isHighBand"), false);
  if (isHighBand != eventMaxDesc->getMemberOr(ECS_HASH("isHighBand"), false))
    logerr("scaled rumble event '%s-%s' has mixed low/high band type", evt.get<0>().c_str(), evt.get<1>().c_str());

  const float freq = lerp(eventMinDesc->getMemberOr(ECS_HASH("freq"), 0.f), eventMaxDesc->getMemberOr(ECS_HASH("freq"), 0.f), scale);
  const int durationMsec =
    lerp(eventMinDesc->getMemberOr(ECS_HASH("durationMsec"), 0), eventMaxDesc->getMemberOr(ECS_HASH("durationMsec"), 0), scale);

  using namespace force_feedback::rumble;
  const enum EventParams::Band band = isHighBand ? EventParams::HI : EventParams::LO;
  add_event({band, freq, durationMsec});
}
