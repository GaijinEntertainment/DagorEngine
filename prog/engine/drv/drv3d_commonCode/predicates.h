#pragma once

namespace d3d
{
template <typename T, T null_value>
struct PredicateGeneric
{
  typedef T ValueType;

  static const int survey_started = 0x01;
  static const int survey_ended = 0x02;
  static const int render_started = 0x04;
  static const int render_ended = 0x08;

  uint32_t state;
  ValueType p;

  PredicateGeneric() : state(0), p(null_value) {}
  static const T nullValue() { return null_value; } // since we dont have c++11 constexpr yet
};

template <typename T>
struct PredicateStorage
{
  typedef T PredicateType;
  Tab<PredicateType> list;
  Tab<uint32_t> freePool;
};

template <typename Storate>
void close_predicates_generic(Storate &storage)
{
  typedef typename Storate::PredicateType Predicate;

  for (int i = 0; i < storage.list.size(); i++)
  {
    if (storage.list[i].p != Predicate::nullValue())
      d3d::free_predicate(i);
  }

  clear_and_shrink(storage.list);
  clear_and_shrink(storage.freePool);
}

template <typename CreateCB, typename Storate>
int create_predicate_generic(Storate &storage)
{
  typedef typename Storate::PredicateType Predicate;

  int id = -1;
  if (!storage.freePool.empty())
  {
    id = storage.freePool.back();
    storage.freePool.pop_back();
  }

  if (id < 0)
  {
    storage.list.push_back(Predicate());
    id = storage.list.size() - 1;
  }

  G_ASSERT(storage.list[id].p == Predicate::nullValue());

  storage.list[id].state = 0;
  storage.list[id].p = CreateCB()();

  if (storage.list[id].p == Predicate::nullValue())
  {
    G_ASSERTF(false, "predicate creation failed, id:%d", id);
    storage.freePool.push_back(id);
    id = -1;
  }

  return id;
}

template <typename DeleteCB, typename Storage>
void free_predicate_generic(Storage &storage, int id)
{
  if (id < 0 || id >= storage.list.size())
  {
    G_ASSERTF(false, "invalid predicate id:%d", id);
    return;
  }

  typedef typename Storage::PredicateType Predicate;
  Predicate &p = storage.list[id];

  G_ASSERTF(!(p.state & (Predicate::survey_started | Predicate::render_started)), "trying to free active predicate, state:%d",
    p.state);

  if (p.p == Predicate::nullValue())
  {
    G_ASSERTF(false, "trying to free already destroyed predicate: %d", id);
    return;
  }

  DeleteCB()(p.p);

  p.p = Predicate::nullValue();
  p.state = 0;

  storage.freePool.push_back(id);
}

template <typename Storage>
typename Storage::PredicateType::ValueType begin_survey_generic(Storage &storage, int id)
{
  typedef typename Storage::PredicateType Predicate;
  if (id < 0 || id >= storage.list.size())
  {
    G_ASSERTF(false, "invalid predicate id:%d", id);
    return Predicate::nullValue();
  }

  Predicate &p = storage.list[id];

  G_ASSERT(p.p != Predicate::nullValue());
  if (p.state & (Predicate::survey_started | Predicate::render_started))
  {
    G_ASSERTF(false, "using already busy predicate, state:%d", p.state);
    return Predicate::nullValue();
  }

  p.state = Predicate::survey_started;
  return p.p;
}

template <typename Storage>
typename Storage::PredicateType::ValueType end_survey_generic(Storage &storage, int id)
{
  typedef typename Storage::PredicateType Predicate;
  if (id < 0 || id >= storage.list.size())
  {
    G_ASSERTF(false, "invalid predicate id:%d", id);
    return Predicate::nullValue();
  }

  Predicate &p = storage.list[id];

  G_ASSERT(p.p != Predicate::nullValue());
  if (p.state != Predicate::survey_started)
  {
    G_ASSERTF(false, "survey wasnt started, state:%d", p.state);
    return Predicate::nullValue();
  }

  p.state = Predicate::survey_ended;
  return p.p;
}

template <typename Storage>
typename Storage::PredicateType::ValueType begin_conditional_render_generic(Storage &storage, int id)
{
  typedef typename Storage::PredicateType Predicate;
  if (id < 0 || id >= storage.list.size())
  {
    G_ASSERTF(false, "invalid predicate id:%d", id);
    return Predicate::nullValue();
  }

  Predicate &p = storage.list[id];

  G_ASSERT(p.p != Predicate::nullValue());
  if (!(p.state & Predicate::survey_ended))
  {
    G_ASSERTF(false, "using predicate without survey, state:%d", p.state);
    return Predicate::nullValue();
  }

  p.state &= ~Predicate::render_ended;
  p.state |= Predicate::render_started;
  return p.p;
}

template <typename Storage>
typename Storage::PredicateType::ValueType end_conditional_render_generic(Storage &storage, int id)
{
  typedef typename Storage::PredicateType Predicate;
  if (id < 0 || id >= storage.list.size())
  {
    G_ASSERTF(false, "invalid predicate id:%d", id);
    return Predicate::nullValue();
  }

  Predicate &p = storage.list[id];

  G_ASSERT(p.p != Predicate::nullValue());
  if (!(p.state & Predicate::render_started))
  {
    G_ASSERTF(false, "using end conditional render without begin, state:%d", p.state);
    return Predicate::nullValue();
  }

  p.state &= ~Predicate::render_started;
  p.state |= Predicate::render_ended;
  return p.p;
}
} // namespace d3d