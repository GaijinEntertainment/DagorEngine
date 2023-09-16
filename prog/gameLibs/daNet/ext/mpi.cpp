#include <daNet/mpi.h>

using namespace danet;

namespace mpi
{
#define MAX_CALL_DEPTH (16)
static IMessageListener *message_listeners = NULL;
static object_dispatcher obj_dispatcher = NULL;
static thread_local int curr_call_depth = 0;

IObject *dispatch_object(ObjectID oid)
{
  return static_cast<IObject *>((oid != INVALID_OBJECT_ID && obj_dispatcher) ? obj_dispatcher(oid) : NULL);
}

Message *dispatch(const BitStream &bs, bool copy_payload)
{
  ObjectID oid;
  MessageID mid;
  if (obj_dispatcher && bs.Read(oid) && bs.Read(mid))
  {
    IObject *o = dispatch_object(oid);
    if (o)
    {
      Message *m = (mid != INVALID_MESSAGE_ID) ? o->dispatchMpiMessage(mid) : NULL;
      if (m)
      {
        m->payload.~BitStream();
        G_ASSERT(!(bs.GetReadOffset() & 7)); // bit offset
        uint32_t bytesReaded = BITS_TO_BYTES(bs.GetReadOffset());
        new (&m->payload, _NEW_INPLACE)
          danet::BitStream(bs.GetData() + bytesReaded, bs.GetNumberOfBytesUsed() - bytesReaded, copy_payload);
        if (m->readPayload())
          return m;
        else
          m->destroy();
      }
    }
  }
  return NULL;
}

void sendto(Message *m, SystemID receiver)
{
  if (++curr_call_depth >= MAX_CALL_DEPTH)
  {
    G_ASSERTF(0, "%s call depth is %d! Infinite recursion?!", __FUNCTION__, (int)curr_call_depth);
    debug("%s call depth is %d! Infinite recursion?!", __FUNCTION__, (int)curr_call_depth);
    --curr_call_depth;
    return;
  }

  if (!m)
    ;
  else if (!m->obj)
    debug("%s ignore attempt to send message 0x%p (0x%x) without recepient object", __FUNCTION__, m, m->id);
  else
    for (IMessageListener *l = message_listeners; l; l = l->next)
      if (m->isApplicable(l))
        l->receiveMpiMessage(m, receiver);

  --curr_call_depth;
}

void register_listener(IMessageListener *l)
{
  l->next = NULL;
  IMessageListener *prev = NULL;
  for (IMessageListener *ll = message_listeners; ll; prev = ll, ll = ll->next)
    ;
  (prev ? prev->next : message_listeners) = l;
}

void unregister_listener(IMessageListener *l)
{
  IMessageListener *prev = NULL;
  for (IMessageListener *ll = message_listeners; ll; prev = ll, ll = ll->next)
    if (l == ll)
    {
      if (prev)
        prev->next = l->next;
      if (l == message_listeners)
        message_listeners = l->next;
      break;
    }
}

void register_object_dispatcher(object_dispatcher od) { obj_dispatcher = od; }
}; // namespace mpi
