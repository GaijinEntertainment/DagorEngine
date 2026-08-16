// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "metronome_detail.h"

#include <EASTL/utility.h>
#include <debug/dag_log.h>
#include <debug/dag_assert.h>
#include <generic/dag_expected.h>
#include <generic/dag_fixedVectorSet.h>


namespace dafg::metronome
{
using detail::Scheduler;
using detail::StoredNode;
using detail::SubgraphState;

namespace
{

Scheduler &scheduler()
{
  static Scheduler instance;
  return instance;
}

enum class GetError
{
  InvalidId,
};

dag::Expected<eastl::reference_wrapper<SubgraphState>, GetError> get_subgraph(SubgraphId id)
{
  SubgraphState *sg = scheduler().subgraphs.get(id);
  if (!sg)
    return dag::Unexpected(GetError::InvalidId);
  return eastl::ref(*sg);
}

void activate(SubgraphState &sg)
{
  for (size_t i = sg.liveHandles.size(); i < sg.nodes.size(); ++i)
  {
    StoredNode &node = sg.nodes[i];
    sg.liveHandles.push_back(node.activator(node.ns, node.name.c_str(), node.sourceLocation.c_str()));
  }
  sg.state = UpdateStatus::Running;
}

} // anonymous namespace

void detail::register_node_impl(SubgraphId subgraph_id, NameSpace ns, const char *name, const char *source_location,
  detail::NodeActivator activator)
{
  get_subgraph(subgraph_id)
    .and_then([&](eastl::reference_wrapper<SubgraphState> sg) -> dag::Expected<void, GetError> {
      sg.get().nodes.push_back(StoredNode{ns, eastl::string{name}, eastl::string{source_location}, eastl::move(activator)});
      return {};
    })
    .or_else([&](GetError err) -> dag::Expected<void, GetError> {
      if (err == GetError::InvalidId)
        logerr("dafg::metronome: register_node: invalid subgraph id");
      return {};
    })
    .has_value();
}

SubgraphHandle make_subgraph(const char *name, uint32_t update_max_delay_frames)
{
  G_ASSERT(name != nullptr);
  G_ASSERT(update_max_delay_frames > 0);

  SubgraphId id = scheduler().subgraphs.emplaceOne();
  G_ASSERT(static_cast<bool>(id));

  SubgraphState *sg = scheduler().subgraphs.get(id);
  G_ASSERT(sg != nullptr);

  sg->name = name ? name : "<unnamed>";
  sg->maxDelayFrames = eastl::max(update_max_delay_frames, 1u);
  return SubgraphHandle{id};
}

SubgraphHandle::SubgraphHandle(SubgraphHandle &&other) noexcept : id{other.id} { other.id = SubgraphId{}; }

SubgraphHandle &SubgraphHandle::operator=(SubgraphHandle &&other) noexcept
{
  if (this == &other)
    return *this;

  if (valid())
    scheduler().subgraphs.destroyReference(id);

  id = other.id;
  other.id = SubgraphId{};
  return *this;
}

SubgraphHandle::~SubgraphHandle()
{
  if (valid())
    scheduler().subgraphs.destroyReference(id);
}

UpdateToken SubgraphHandle::schedule()
{
  auto sg = get_subgraph(id);
  G_ASSERT_RETURN(sg.has_value(), UpdateToken{});

  SubgraphState &s = sg.value().get();

  if (DAGOR_UNLIKELY(s.pendingSinceTick == scheduler().tick))
    logerr("dafg::metronome: schedule(): subgraph '%s' scheduled more than once in the same frame", s.name.c_str());

  ++s.scheduleGeneration;
  s.state = UpdateStatus::Pending;
  s.pendingSinceTick = scheduler().tick;

  return UpdateToken{id, s.scheduleGeneration};
}

UpdateStatus UpdateToken::status() const
{
  if (!subgraphId)
    return UpdateStatus::NotScheduled;

  SubgraphState *s = scheduler().subgraphs.get(subgraphId);
  if (!s)
    return UpdateStatus::Superseded;

  if (scheduleGeneration != s->scheduleGeneration)
    return UpdateStatus::Superseded;

  return s->state;
}

void update()
{
  Scheduler &sched = scheduler();
  ++sched.tick;

  // Find next subgraphs to run
  SubgraphId bestId;
  uint32_t bestSlack = eastl::numeric_limits<uint32_t>::max();
  dag::FixedVectorSet<SubgraphId, 4> forced = {};
  for (uint32_t i = 0; i < sched.subgraphs.totalSize(); ++i)
  {
    SubgraphState *sg = sched.subgraphs.getByIdx(i);
    if (!sg || sg->state != UpdateStatus::Pending)
      continue;

    const uint32_t age = sched.tick - sg->pendingSinceTick;
    if (DAGOR_LIKELY(age < sg->maxDelayFrames))
    {
      const uint32_t slack = sg->maxDelayFrames - age;
      if (slack < bestSlack)
      {
        bestId = sched.subgraphs.getRefByIdx(i);
        bestSlack = slack;
      }
    }
    else
    {
      forced.insert(sched.subgraphs.getRefByIdx(i));
    }
  }

  // More than one subgraph will cause spike and should be avoided
  if (forced.size() > 1)
    for (auto &id : forced)
    {
      logerr("dafg::metronome: subgraph '%s' forced to run (%d forced this frame); "
             "reconsider its update cadence",
        sched.subgraphs.get(id)->name.c_str(), (int)forced.size());
    }

  // Finish any alive subgraphs that are not selected to run this frame
  for (uint32_t i = 0; i < sched.subgraphs.totalSize(); ++i)
  {
    SubgraphState *sg = sched.subgraphs.getByIdx(i);
    if (!sg)
      continue;

    SubgraphId id = sched.subgraphs.getRefByIdx(i);

    if (bestId == id)
      continue;

    if (forced.contains(id))
      continue;

    sg->liveHandles.clear();

    if (sg->state == UpdateStatus::Running)
      sg->state = UpdateStatus::Complete;
  }

  // Activate newly selected subgraphs
  if (DAGOR_LIKELY(forced.empty()))
  {
    if (bestId)
      activate(*sched.subgraphs.get(bestId));
  }
  else
  {
    if (bestId)
      sched.subgraphs.get(bestId)->liveHandles.clear();
    for (auto &id : forced)
    {
      SubgraphState *sg = sched.subgraphs.get(id);
      if (sg)
        activate(*sg);
    }
  }
}

} // namespace dafg::metronome
