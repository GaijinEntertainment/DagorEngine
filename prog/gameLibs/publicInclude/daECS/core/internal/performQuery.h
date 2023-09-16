#pragma once

/*
Do not use this header directly. Let code gen write all boilerplate code.
Use _ecs_query(Callable fn); with lambda functions instead of direct perform_query.
Search for examples with "_ecs_query" keyword.
*/

#include <daECS/core/entityManager.h>

namespace ecs
{
// use min_quant of more than 0 for parallel for execution (each job will take ata least min_quant of data)
inline QueryCbResult perform_query(EntityManager *__restrict em, QueryId query, const stoppable_query_cb_t &__restrict fun,
  void *__restrict user_data = nullptr)
{
  return em->performQueryStoppable(query, fun, user_data);
}

inline void perform_query(EntityManager *__restrict em, QueryId query, const query_cb_t &__restrict fun,
  void *__restrict user_data = nullptr, int min_quant = 0)
{
  return em->performQuery(query, fun, user_data, min_quant);
}

template <typename Fn>
__forceinline bool perform_query(EntityManager *__restrict em, ecs::EntityId eid, QueryId query, Fn &&__restrict fun,
  void *__restrict user_data)
{
  return em->performEidQuery(eid, query, eastl::move(fun), user_data);
}
template <typename Fn>
__forceinline bool perform_query(EntityManager *__restrict em, ecs::EntityId eid, QueryId query, Fn &&__restrict fun)
{
  return perform_query(em, eid, query, eastl::move(fun), nullptr);
}
} // namespace ecs
