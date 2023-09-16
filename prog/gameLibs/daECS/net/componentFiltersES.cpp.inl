#include <daECS/net/object.h>
#include <daECS/net/connection.h>
#include "objectReplica.h"
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>

namespace net
{

template <typename Cb>
static inline void mark_all_dirty_ecs_query(Cb cb);

struct ComponentFiltersHelper
{
  typedef decltype(net::Object::filteredComponentsBits) object_filter_t;
  static void mark_some_objects_as_dirty(object_filter_t mask);
};

void ComponentFiltersHelper::mark_some_objects_as_dirty(object_filter_t mask)
{
  mark_all_dirty_ecs_query([mask](net::Object &replication) // todo: filteredComponentsBits should be component (improves cache
                                                            // friendliness)!
    {
      if ((replication.filteredComponentsBits & mask) != 0)
      {
        for (ObjectReplica *repl = replication.replicasLinkList; repl; repl = repl->nextRepl)
          repl->conn->pushToDirty(repl);
      }
    });
}

void mark_some_objects_as_dirty(uint32_t mask)
{
  ComponentFiltersHelper::mark_some_objects_as_dirty(ComponentFiltersHelper::object_filter_t(mask));
}
void mark_all_objects_as_dirty() { ComponentFiltersHelper::mark_some_objects_as_dirty(~ComponentFiltersHelper::object_filter_t(0)); }

}; // namespace net