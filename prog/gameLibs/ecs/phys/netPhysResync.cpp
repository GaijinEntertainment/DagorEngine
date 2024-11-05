// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/phys/netPhysResync.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/component.h>

ECS_REGISTER_BOXED_TYPE(ECSCustomPhysStateSyncer, nullptr);

#define NR_LOG(...)         ((void)0) // debug(__VA_ARGS__)
#define NR_LOG_VERBOSE(...) /* NR_LOG(__VA_ARGS__) // */ ((void)0)

template <typename T>
static inline constexpr uint32_t dtype() // To make most common case (float) zero for faster compare
{
  return ecs::ComponentTypeInfo<float>::type - ecs::ComponentTypeInfo<T>::type;
}

void ECSCustomPhysStateSyncer::init(ecs::EntityId eid_, int resv)
{
  eid = eid_;
  G_ASSERT(eid);
  if (resv)
  {
    G_ASSERT(scheme.empty());
    scheme.reserve(resv);
  }
}

template <typename T>
void ECSCustomPhysStateSyncer::regSyncComp(const char *cn, T &cr)
{
  static_assert(sizeof(T) <= sizeof(data[0])); // Serialization & compare assumption
  G_UNUSED(cr);
  G_ASSERTF(eid, "Invalid eid. <init> wasn't called?");
  G_ASSERTF(history.empty() && data.size() == scheme.size(), "Attempt to change scheme after update?");
  auto ch = ECS_HASH_SLOW(cn);
  G_ASSERT(g_entity_mgr->get<T>(eid, ch) == cr); // Sanity check
  G_ASSERTF(eastl::find(scheme.begin(), scheme.end(), ch.hash, [](auto v, auto h) { return v.first == h; }) == scheme.end(),
    "Dup key <%s>", cn);
  scheme.push_back(eastl::make_pair(ch.hash, dtype<T>()));
  data.resize(scheme.size(), 0); // for AAS
  numDataBits += (dtype<T>() != dtype<bool>()) ? sizeof(data[0]) * CHAR_BIT : 1;
  NR_LOG("%s eid=%d [%d]=%s <%#X>", __FUNCTION__, (unsigned)eid, scheme.size() - 1, cn, ch.hash);
}

void ECSCustomPhysStateSyncer::registerSyncComponent(const char *comp_name, float &comp_ref)
{
  return regSyncComp(comp_name, comp_ref);
}
void ECSCustomPhysStateSyncer::registerSyncComponent(const char *comp_name, int &comp_ref) { return regSyncComp(comp_name, comp_ref); }
void ECSCustomPhysStateSyncer::registerSyncComponent(const char *comp_name, bool &comp_ref)
{
  return regSyncComp(comp_name, comp_ref);
}

void ECSCustomPhysStateSyncer::save(const eastl::pair<int, int> &state)
{
  int *pv = &data[state.second];
  for (auto &s : scheme)
    if (DAGOR_LIKELY(s.second == dtype<float>()))
    {
      *pv++ = bitwise_cast<int>(g_entity_mgr->get<float>(eid, {nullptr, s.first})); // TODO: rework to getFast
      NR_LOG_VERBOSE("%s <%#X> eid=%d tick=%d [%d]=%.9f/%#x", __FUNCTION__, s.first, (unsigned)eid, state.first,
        &state == &AAS ? -1 : (&state - history.data()), bitwise_cast<float>(pv[-1]), pv[-1]);
    }
    else if (s.second == dtype<int>())
    {
      *pv++ = g_entity_mgr->get<int>(eid, {nullptr, s.first}); // TODO: rework to getFast
      NR_LOG_VERBOSE("%s <%#X> eid=%d tick=%d [%d]=%d/%#x", __FUNCTION__, s.first, (unsigned)eid, state.first,
        &state == &AAS ? -1 : (&state - history.data()), pv[-1], pv[-1]);
    }
    else if (s.second == dtype<bool>())
    {
      *pv++ = int(g_entity_mgr->get<bool>(eid, {nullptr, s.first})); // TODO: rework to getFast
      NR_LOG_VERBOSE("%s <%#X> eid=%d tick=%d [%d]=%d/%#x", __FUNCTION__, s.first, (unsigned)eid, state.first,
        &state == &AAS ? -1 : (&state - history.data()), pv[-1], pv[-1]);
    }
    else
      G_ASSERT(0);
}

void ECSCustomPhysStateSyncer::saveHistoryState(int tick)
{
  G_ASSERT(eid);
  auto &hist = static_cast<decltype(history)::base_type &>(history);
  while (!hist.empty() && hist.back().first >= tick) // Rare case of tick delta decrease
  {
    G_ASSERT(hist.back().second + scheme.size() == data.size());
    data.resize(data.size() - scheme.size());
    hist.pop_back();
  }
  hist.push_back(eastl::make_pair(tick, (int)data.size()));
  G_ASSERT(eastl::is_sorted(hist.begin(), hist.end(), [](auto &a, auto &b) { return a.first < b.first; }));
  data.resize(data.size() + scheme.size()); // Append
  save(hist.back());
}

bool ECSCustomPhysStateSyncer::isHistoryStateEqualAuth(int htick, int aas_tick)
{
  if (aas_tick != AAS.first) // Do not force resync if auth state is outdated (e.g. server stopped to send resync data)
    return true;
  auto it = history.lower_bound(htick);
  G_ASSERT(it == history.end() || (it->second % scheme.size()) == 0);
  int eq = it == history.end() || it->first != htick;
  if (!eq)
    eq = memcmp(data.data(), &data[it->second], scheme.size() * sizeof(decltype(data)::value_type)) << int(DAGOR_DBGLEVEL > 0);
  NR_LOG("%s eid=%d tick=%d/%d/%d eq=%d hist.size=%d", __FUNCTION__, (unsigned)eid, htick, aas_tick,
    it != history.end() ? it->first : -1, eq, history.size());
  return eq == 0;
}

void ECSCustomPhysStateSyncer::serializeAuthState(danet::BitStream &bs, int tick) const
{
  G_UNUSED(tick);
  G_ASSERTF(tick == AAS.first, "%d != %d", tick, AAS.first); // Was not saved?
  bs.WriteCompressed((uint16_t)scheme.size());               // Write number of serialized comps for forward-compat
  if (isHomogeneousData())
    bs.Write((const char *)data.data(), sizeof(data[0]) * scheme.size());
  else
  {
    bs.reserveBits(numDataBits);
    for (int i = 0; i < scheme.size(); ++i)
      if (scheme[i].second != dtype<bool>())
        bs.Write((const char *)&data[i], sizeof(data[0]));
      else
        bs.Write(bool(data[i]));
    bs.AlignWriteToByteBoundary();
  }
  for (int i = 0; i < scheme.size(); ++i)
    NR_LOG("%s <%#X> eid=%d tick=%d [%d]=%.9f/%#x", __FUNCTION__, scheme[i].first, (unsigned)eid, tick, i,
      bitwise_cast<float>(data[i]), data[i]);
}

bool ECSCustomPhysStateSyncer::deserializeAuthState(const danet::BitStream &bs, int tick)
{
  uint16_t n = 0;
  if (!bs.ReadCompressed(n))
    return true; // Intentionally not fail for proto compat
  decltype(data)::value_type _1;
  bool homogeneousData = isHomogeneousData();
  ecs::component_type_t pdt = dtype<int>();
  for (int i = 0; i < n; ++i)
  {
    auto &v = i < scheme.size() ? data[i] : _1; // fwd compat
    if (i < scheme.size())
      pdt = scheme[i].second;
    bool ok;
    if (homogeneousData || pdt != dtype<bool>())
      ok = bs.Read((char *)&v, sizeof(data[0]));
    else
    {
      bool b = false;
      ok = bs.Read(b);
      v = (int)b;
    }
    if (DAGOR_UNLIKELY(!ok))
    {
      AAS.first = tick | (1 << 31);
      return false;
    }
    NR_LOG("%s <%#X> eid=%d tick=%d [%d]=%.9f/%#x", __FUNCTION__, i < scheme.size() ? scheme[i].first : 0, (unsigned)eid, tick, i,
      bitwise_cast<float>(v), v);
  }
  if (!homogeneousData)
    bs.AlignReadToByteBoundary();
  AAS.first = tick;
  return true;
}

void ECSCustomPhysStateSyncer::eraseHistoryStateTail(int tick)
{
  auto it = history.lower_bound(tick);
  NR_LOG("%s tick=%d ftick=%d data[%d].size=%d", __FUNCTION__, tick, it != history.end() ? it->first : -1,
    it != history.end() ? it->second : -1, data.size());
  if (it != history.end())
  {
    G_ASSERT(it->second >= scheme.size() && it->second % scheme.size() == 0 && it->second < data.size());
    data.resize(it->second);
    history.resize(it - history.begin());
  }
}

void ECSCustomPhysStateSyncer::eraseHistoryStateHead(int tick)
{
  auto it = tick >= 0 ? history.lower_bound(tick) : history.end();
  NR_LOG("%s tick=%d ftick=%d data[%d].size=%d", __FUNCTION__, tick, it != history.end() ? it->first : -1,
    it != history.end() ? it->second : -1, data.size());
  if (it != history.end())
  {
    G_ASSERT(it->second >= scheme.size() && it->second % scheme.size() == 0 && it->second < data.size());
    int base = it->second, newbase = scheme.size();
    for (auto it2 = it; it2 != history.end(); ++it2, newbase += scheme.size()) // Rebase
      it2->second = newbase;
    data.erase(data.begin() + scheme.size(), data.begin() + base);
    G_ASSERT(newbase <= data.size());
    history.erase(history.begin(), it);
  }
  else // clear
  {
    data.resize(scheme.size());
    history.clear();
  }
}

void ECSCustomPhysStateSyncer::applyAuthState(int tick)
{
  if (tick != AAS.first) // Could be if server stopped to send us data
    return;
  for (int i = 0, n = scheme.size(); i < n; ++i)
    if (DAGOR_LIKELY(scheme[i].second == dtype<float>()))
    {
      NR_LOG("%s <%#X> eid=%d tick=%d [%d]=%.9f/%#x", __FUNCTION__, scheme[i].first, (unsigned)eid, tick, i,
        bitwise_cast<float>(data[i]), data[i]);
      g_entity_mgr->set(eid, {nullptr, scheme[i].first}, bitwise_cast<float>(data[i])); // TODO: rework to setFast
    }
    else if (scheme[i].second == dtype<int>())
    {
      NR_LOG("%s <%#X> eid=%d tick=%d [%d]=%d", __FUNCTION__, scheme[i].first, (unsigned)eid, tick, i, data[i]);
      g_entity_mgr->set(eid, {nullptr, scheme[i].first}, data[i]); // TODO: rework to setFast
    }
    else if (scheme[i].second == dtype<bool>())
    {
      NR_LOG("%s <%#X> eid=%d tick=%d [%d]=%d", __FUNCTION__, scheme[i].first, (unsigned)eid, tick, i, data[i]);
      g_entity_mgr->set(eid, {nullptr, scheme[i].first}, bool(data[i])); // TODO: rework to setFast
    }
    else
      G_ASSERT(0);
  AAS.first = -tick;
}

void ECSCustomPhysStateSyncer::clear()
{
  AAS.first = -1;
  data.resize(scheme.size());
  data.shrink_to_fit();
  clear_and_shrink(history);
}
