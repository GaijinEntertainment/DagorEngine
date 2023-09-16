#include <daECS/net/schemelessEventSerialize.h>
#include <daECS/core/schemelessEvent.h>
#include <daECS/net/serialize.h>
#include <daNet/bitStream.h>
#include <daECS/core/componentTypes.h>

namespace ecs
{

void serialize_to(const SchemelessEvent &evt, danet::BitStream &bs)
{
  bs.Write(evt.getType());
  net::BitstreamSerializer cb(bs);
  ecs::serialize_entity_component_ref_typeless(&evt.getData(), ecs::ComponentTypeInfo<ecs::Object>::type, cb);
}

MaybeSchemelessEvent deserialize_from(const danet::BitStream &bs)
{
  ecs::event_type_t evtt = 0;
  if (!bs.Read(evtt))
    return MaybeSchemelessEvent{};
  net::BitstreamDeserializer cb(bs);
  constexpr ecs::component_type_t objType = ecs::ComponentTypeInfo<ecs::Object>::type;
  if (ecs::MaybeChildComponent mbcomp = deserialize_init_component_typeless(objType, ecs::INVALID_COMPONENT_INDEX, cb))
    if (mbcomp->getUserType() == objType)
      return SchemelessEvent(evtt, eastl::move(mbcomp->getRW<ecs::Object>()));
  return MaybeSchemelessEvent{};
}

} // namespace ecs
