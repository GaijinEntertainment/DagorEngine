// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/net/serialize.h>
#include <daECS/core/component.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/template.h>
#include <daECS/core/entityManager.h>
#include <daECS/net/object.h>
#include <daECS/net/connection.h>
#include <daECS/net/compBlacklist.h>
#include <EASTL/fixed_string.h>
#include <daNet/bitStream.h>
#include <util/dag_simpleString.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>
#include <math/dag_lsbVisitor.h>
#include <limits.h>
#include <supp/dag_alloca.h>
#include "sharedReplicationCache.h"
#include "componentListsCache.h"

namespace ecs
{
extern const int MAX_STRING_LENGTH;
extern const uint32_t MAX_RESERVED_EID_IDX;
} // namespace ecs

namespace net
{

static int read_string(const danet::BitStream &cb, eastl::string &to)
{
  char buf[1024];
  buf[0] = 0;
  for (char *str = buf;;)
  {
    if (!cb.Read(str, 1))
      return -1;
    if (!*str)
      break;
    if (str == buf + countof(buf) - 2)
    {
      str[1] = 0;
      to += buf;
      str = buf;
    }
    else
      str++;
  };
  to += buf;
  return to.length();
}

InternedStringsShared::InternedStringsShared()
{
  index.emplace("", 0);
  strings.emplace_back("");
}

InternedStringsRepl::InternedStringsRepl(InternedStringsShared *shared) : shared(shared) { serialized.set(0, true); }

inline bool read_string_no(const danet::BitStream &cb, uint32_t &str, const uint32_t short_bits)
{
  // assume Little endian
  str = 0;
  return cb.ReadBits((uint8_t *)&str, short_bits);
}

inline void write_string_no(danet::BitStream &cb, uint32_t str, const uint32_t short_bits)
{
  // assume Little endian
  G_ASSERT(str < uint32_t(1 << short_bits));
  cb.WriteBits((const uint8_t *)&str, short_bits);
}

static eastl::string oneString;
static const char *read_istring(const danet::BitStream &cb, InternedStringsRepl *istrs, uint32_t short_bits)
{
  bool rawString = false;
  if (!cb.Read(rawString))
    return nullptr;
  if (rawString)
  {
    oneString.clear();
    if (read_string(cb, oneString) < 0)
      return nullptr;
    return oneString.c_str();
  }
  if (!istrs)
    return nullptr;
  uint32_t str;
  if (!read_string_no(cb, str, short_bits))
    return nullptr;
  InternedStringsShared &all = *istrs->shared;
  if (str >= all.strings.size() || (all.strings[str].empty() && str != 0))
  {
    logerr("Interned string idx %u referenced but not in pool (size=%u). Missing from prefix?", str, (unsigned)all.strings.size());
    return nullptr;
  }
  return all.strings[str].c_str();
}

static void write_raw_string(danet::BitStream &cb, const eastl::string &pStr)
{
  cb.Write(true);
  cb.Write(pStr.c_str(), pStr.length() + 1);
}

static void write_istring(danet::BitStream &cb, const eastl::string &pStr, InternedStringsShared &all, uint32_t short_bits,
  eastl::bitvector<framemem_allocator, uint64_t> &out_used)
{
  auto it = all.index.find(pStr);
  if (it == all.index.end())
  {
    if (DAGOR_UNLIKELY(all.strings.size() >= uint32_t(1 << short_bits)))
    {
      write_raw_string(cb, pStr);
      return;
    }
    all.strings.emplace_back(pStr);
    it = all.index.emplace(all.strings.back(), all.strings.size() - 1).first;
  }
  cb.Write(false);
  write_string_no(cb, it->second, short_bits);
  out_used.set(it->second, true);
}

static constexpr int OBJECT_KEY_BITS = 10;

bool BitstreamDeserializer::read(void *to, size_t sz_in_bits, ecs::component_type_t user_type) const
{
  if (user_type == 0)
    return bs.ReadBits((uint8_t *)to, sz_in_bits);
  else if (user_type == ecs::ComponentTypeInfo<ecs::EntityId>::type)
  {
    DAECS_EXT_ASSERT(sz_in_bits == sizeof(ecs::EntityId) * CHAR_BIT);
    return read_eid(bs, *(ecs::EntityId *)to);
  }
  else if (user_type == ecs::ComponentTypeInfo<bool>::type) // bool optimization. Bool is actually one bit
  {
    G_ASSERT(sz_in_bits == CHAR_BIT);
    return bs.Read(*(bool *)to);
  }
  else if (user_type == ecs::ComponentTypeInfo<ecs::Object>::type) // intern strings for Objects
  {
    ecs::Object &obj = *((ecs::Object *)to);
    obj.clear();
    uint32_t cnt;
    if (!ecs::read_compressed(*this, cnt))
      return false;
    // Every entry consumes at least a key plus a child component, so more entries than
    // unread bytes is impossible; reject before reserving to avoid a huge hostile alloc.
    if (cnt > bs.GetNumberOfUnreadBits() / CHAR_BIT)
      return false;
    obj.reserve(cnt);
    for (uint32_t i = 0; i < cnt; ++i)
    {
      const char *str = read_istring(bs, objectKeys, OBJECT_KEY_BITS);
      if (!str)
        return false;
      auto &item = obj.insert(str); // insert before deserialization since 'str' might be reused in next calls
      if (ecs::MaybeChildComponent maybeComp = ecs::deserialize_child_component(*this, mgr))
        item = eastl::move(*maybeComp);
      else
        return false;
    }
    return true;
  }
  else if (user_type == ecs::ComponentTypeInfo<ecs::string>::type)
  {
    Tab<char> tmp(framemem_ptr());
    tmp.resize(ecs::MAX_STRING_LENGTH);
    if (ecs::read_string(*this, tmp.data(), ecs::MAX_STRING_LENGTH) < 0)
      return false;
    *((ecs::string *)to) = tmp.data();
    return true;
  }
  else
    return bs.ReadBits((uint8_t *)to, sz_in_bits);
}

bool BitstreamDeserializer::skip(ecs::component_index_t cidx, const ecs::DataComponent &compInfo)
{
  if (compInfo.componentType == ecs::INVALID_COMPONENT_TYPE_INDEX)
    return false;
  auto &componentTypes = mgr.getComponentTypes();
  const ecs::ComponentType componentTypeInfo = componentTypes.getTypeInfo(compInfo.componentType);
  const bool isPod = ecs::is_pod(componentTypeInfo.flags);
  ecs::ComponentSerializer *typeIO = nullptr;
  if (compInfo.flags & ecs::DataComponent::HAS_SERIALIZER)
    typeIO = mgr.getDataComponents().getComponentIO(cidx);
  if (!typeIO && has_io(componentTypeInfo.flags))
    typeIO = componentTypes.getTypeIO(compInfo.componentType);
  void *tempData = alloca(componentTypeInfo.size);
  ecs::ComponentTypeManager *ctm = NULL;
  if (need_constructor(componentTypeInfo.flags))
  {
    ctm = const_cast<ecs::ComponentTypes &>(componentTypes).createTypeManager(compInfo.componentType);
    G_ASSERTF(ctm, "type manager for type 0x%X (%d) missing", compInfo.componentTypeName, compInfo.componentType);
  }
  if (ctm)
    ctm->create(tempData, mgr, ecs::INVALID_ENTITY_ID, ecs::ComponentsMap(), compInfo.componentType);
  else if (!isPod)
    memset(tempData, 0, componentTypeInfo.size);
  bool isBoxed = (componentTypeInfo.flags & ecs::COMPONENT_TYPE_BOXED) != 0;
  bool ret =
    typeIO ? typeIO->deserialize(*this, isBoxed ? *(void **)tempData : tempData, componentTypeInfo.size, compInfo.componentTypeName)
           : read(tempData, componentTypeInfo.size * CHAR_BIT, compInfo.componentTypeName);
  if (ctm)
    ctm->destroy(tempData);
  return ret;
}

void BitstreamSerializer::write(const void *from, size_t sz_in_bits, ecs::component_type_t user_type)
{
  if (user_type == 0)
    bs.WriteBits((const uint8_t *)from, sz_in_bits);
  else if (user_type == ecs::ComponentTypeInfo<ecs::EntityId>::type)
  {
    DAECS_EXT_ASSERT(sz_in_bits == sizeof(ecs::entity_id_t) * CHAR_BIT);
    write_server_eid(*(const ecs::entity_id_t *)from, bs);
  }
  else if (user_type == ecs::ComponentTypeInfo<bool>::type) // bool optimization
  {
    G_ASSERT(sz_in_bits == CHAR_BIT);
    bs.Write(*(bool *)from); // optimization
  }
  else if (user_type == ecs::ComponentTypeInfo<ecs::Object>::type) // intern strings for Objects
  {
    const ecs::Object &obj = *((const ecs::Object *)from);
    ecs::write_compressed(*this, obj.size());
    if (objectKeys && outObjectKeysUsed)
    {
      for (auto &it : obj)
      {
        write_istring(bs, ecs::get_key_string(it.first), *objectKeys->shared, OBJECT_KEY_BITS, *outObjectKeysUsed);
        ecs::serialize_child_component(it.second, *this, mgr);
      }
    }
    else
    {
      G_ASSERT(!objectKeys && !outObjectKeysUsed);
      for (auto &it : obj)
      {
        write_raw_string(bs, ecs::get_key_string(it.first));
        ecs::serialize_child_component(it.second, *this, mgr);
      }
    }
  }
  else if (user_type == ecs::ComponentTypeInfo<ecs::string>::type)
    ecs::write_string(*this, ((const ecs::string *)from)->c_str(), ecs::MAX_STRING_LENGTH);
  else
    bs.WriteBits((const uint8_t *)from, sz_in_bits);
}

void serialize_comp_nameless(ecs::EntityManager &mgr, ecs::component_t name, const ecs::EntityComponentRef &comp, danet::BitStream &bs)
{
  BitstreamSerializer serializer(mgr, bs);
  serializer.bs.Write(name);
  ecs::component_type_t userType = comp.getUserType();
  serializer.write(&userType, sizeof(userType) * 8, 0);
  // todo: write and read component index
  serialize_entity_component_ref_typeless(comp.getRawData(), ecs::INVALID_COMPONENT_INDEX, comp.getUserType(), comp.getTypeId(),
    serializer, mgr);
}

ecs::MaybeChildComponent deserialize_comp_nameless(ecs::EntityManager &mgr, ecs::component_t &name,
  const danet::BitStream &bs) // todo: replace with component
                              // index
{
  BitstreamDeserializer deserializer(mgr, bs);
  ecs::component_type_t userType = 0;
  // todo: write and read component index
  if (deserializer.bs.Read(name) && deserializer.read(&userType, sizeof(userType) * CHAR_BIT, 0))
    return ecs::deserialize_init_component_typeless(userType, ecs::INVALID_COMPONENT_INDEX, deserializer, mgr);
  else
    return ecs::MaybeChildComponent();
}

static void write_component_index(ecs::component_index_t cidx, danet::BitStream &bs)
{
  bs.WriteCompressed(cidx); //==todo: we only need 12 bits really. Write compressed form of.
}

static bool read_component_index(ecs::component_index_t &cidx, const danet::BitStream &bs)
{
  return bs.ReadCompressed(cidx); //==todo: we only need 12 bits really. Write compressed form of.
}

bool Connection::serializeComponentReplication(ecs::EntityId eid, const ecs::EntityComponentRef &comp, danet::BitStream &bs) const
{
  if (!componentsSynced.test(comp.getComponentId(), false)) // FIXME: race on components replication & re-creation
  {
    G_UNUSED(eid);
    logerr("Attempt to serialize not-yet synced component <%s> of type <%s>, was entity %d<%s> re-created?",
      mgr.getDataComponents().getComponentNameById(comp.getComponentId()), mgr.getComponentTypes().getTypeNameById(comp.getTypeId()),
      (ecs::entity_id_t)eid, mgr.getEntityTemplateName(eid));
    return false;
  }
  write_component_index(comp.getComponentId(), bs);
  BitstreamSerializer serializer(mgr, bs);
  ecs::serialize_entity_component_ref_typeless(comp, serializer, mgr);
  return true;
}

bool Connection::deserializeComponentReplication(ecs::EntityId eid, const danet::BitStream &bs)
{
  ecs::component_index_t serverCidx = 0;
  if (!read_component_index(serverCidx, bs))
    return false;

  G_ASSERT(componentsSynced.test(serverCidx, false)); // should never happen, no need for sanity check in release
  const ecs::component_index_t clientCidx = serverToClientCidx[serverCidx];
  if (clientCidx == ecs::INVALID_COMPONENT_INDEX) // we can't deserialize it, which means type was unknown!
    return false;
  BitSize_t beforeReadPos = bs.GetReadOffset();
  BitstreamDeserializer bsds(mgr, bs, &objectKeysRepl);
  ecs::EntityComponentRef cref = mgr.getComponentRefRW(eid, clientCidx);
  bool crefIsNull = cref.isNull();
  if (DAGOR_LIKELY(!crefIsNull && deserialize_component_typeless(cref, bsds, mgr)))
  {
    replicated_component_on_client_deserialize(mgr, eid, clientCidx);
    return true;
  }
  ecs::DataComponent compInfo = mgr.getDataComponents().getComponentById(clientCidx);
  int loglev;
  const char *logmsg;
  if (crefIsNull)
  {
    // Note: if component is missing then it's might be out-of-sync templates DB (e.g. server has different templates from client)
    // or race on re-create/replication due to different network channels. Both of this cases are not bugs per-se
    // (not from POV of this code at least), therefore report it as warning, not as an error.
    // The only exception when type is also unknown (in which case we can't skip it either)
    loglev = (compInfo.componentType != ecs::INVALID_COMPONENT_TYPE_INDEX) ? LOGLEVEL_WARN : LOGLEVEL_ERR;
    logmsg = "Unknown/missing component";
  }
  else // deserialize failed
  {
    loglev = LOGLEVEL_ERR;
    logmsg = "Failed to deserialize component";
    bs.SetReadOffset(beforeReadPos);
  }
  logmessage(loglev, "%s: %s <%s|#%X>(ccidx=%d|scidx=%d) of type <%s>(%#X|%d) entity %d<%s>", __FUNCTION__, logmsg,
    mgr.getDataComponents().getComponentNameById(clientCidx), mgr.getDataComponents().getComponentTpById(clientCidx), clientCidx,
    serverCidx, mgr.getComponentTypes().getTypeNameById(compInfo.componentType), compInfo.componentTypeName, compInfo.componentType,
    (ecs::entity_id_t)eid, mgr.getEntityTemplateName(eid));
  return bsds.skip(clientCidx, compInfo);
}

bool Connection::syncReadComponent(ecs::component_index_t serverCidx, const danet::BitStream &bs, ecs::template_t templateId,
  bool error)
{
  G_UNUSED(templateId);
  // 8 bytes. However, can write only two bytes, if we preserve component table (which we should)
  ecs::component_t name = 0;
  ecs::component_type_t type = 0;
  if (!(bs.Read(name) && bs.Read(type)))
    return false;
  ecs::component_index_t clientCidx = mgr.getDataComponents().findComponentId(name); // try to immediately resolve component
  if (clientCidx == ecs::INVALID_COMPONENT_INDEX)                                    // component is missing on client
  {
    const ecs::type_index_t typeIdx = mgr.getComponentTypes().findType(type);
    if (error)
    {
      int loglev = typeIdx != ecs::INVALID_COMPONENT_TYPE_INDEX ? LOGLEVEL_WARN : LOGLEVEL_ERR;
      G_UNUSED(loglev);
#if DAECS_EXTENSIVE_CHECKS
      loglev = LOGLEVEL_ERR;
#endif
      logmessage(loglev, "component scidx=%d, name=0x%X type=0x%X(%s) is missing in template <%s> on client", serverCidx, name, type,
        mgr.getComponentTypes().getTypeNameById(typeIdx), serverTemplates[templateId].c_str());
    }
    if (typeIdx != ecs::INVALID_COMPONENT_TYPE_INDEX)
      clientCidx = mgr.createComponent(ecs::HashedConstString{nullptr, name}, typeIdx, dag::Span<ecs::component_t>(), nullptr, 0);
  }
  if (serverCidx >= serverToClientCidx.size())
    serverToClientCidx.resize(serverCidx + 1, ecs::INVALID_COMPONENT_INDEX);
  serverToClientCidx[serverCidx] = clientCidx;
  componentsSynced.set(serverCidx, true);
  return true;
}

bool Connection::syncReadTemplate(const danet::BitStream &bs, ecs::template_t templateId)
{
  G_ASSERT(templateId == serverTemplates.size()); // We rely on templateId being monotonically increased counter here

  serverTemplates.resize(templateId + 1);
  clientTemplatesComponents.resize(templateId + 1);

  if (!bs.Read(serverTemplates[templateId])) // ref to template
    return false;

  const int templId = mgr.buildTemplateIdByName(serverTemplates[templateId].c_str());
  ecs::template_t instantiated = ecs::INVALID_TEMPLATE_INDEX;
  if (templId >= 0)
    instantiated = mgr.instantiateTemplate(templId, false);
  else
  {
    // todo: create this template instead! we know everything from server side!
    logerr("template <%s> is not in database, and so can't be created", serverTemplates[templateId].c_str());
  }

  uint16_t componentsInTemplate = 0;
  if (!bs.Read(componentsInTemplate))
    return false;
  clientTemplatesComponents[templateId].resize(componentsInTemplate);
  for (uint16_t cid = 0; cid != componentsInTemplate; ++cid)
  {
    ecs::component_index_t serverCidx = 0;
    if (!read_component_index(serverCidx, bs))
      return false;
    G_ASSERT(serverCidx != ecs::INVALID_COMPONENT_INDEX);
    ecs::component_index_t cliCidx =
      serverCidx < serverToClientCidx.size() ? serverToClientCidx[serverCidx] : ecs::INVALID_COMPONENT_INDEX;
    if (cliCidx != ecs::INVALID_COMPONENT_INDEX)
    {
      G_ASSERT(componentsSynced.test(serverCidx, false));
      clientTemplatesComponents[templateId][cid] = cliCidx;
    }
    else if (!componentsSynced.test(serverCidx, false))
    {
      if (!syncReadComponent(serverCidx, bs, templateId, templId >= 0))
        return false;
      clientTemplatesComponents[templateId][cid] = serverToClientCidx[serverCidx];
    }
  }
  // serverToClientCidx/componentsSynced are connection-wide and the server sends each
  // component description once, so the mapping above is consumed even on failure
  if (templId >= 0 && instantiated == ecs::INVALID_TEMPLATE_INDEX)
  {
    logerr("template <%s> could not be instantiated, entities of it can't be created", serverTemplates[templateId].c_str());
    return false;
  }
  return true;
}

const char *Connection::deserializeTemplate(const danet::BitStream &bs, ecs::template_t &templateId, bool &tpl_deserialized)
{
  // it is too expensive to always serialize template name, so we just rely on construction being reliable ordered, and write name
  // once.
  templateId = ecs::INVALID_TEMPLATE_INDEX;
  if (!bs.ReadCompressed(templateId))
    return nullptr;
  if (templateId >= serverTemplates.size())
  {
    if (!syncReadTemplate(bs, templateId))
      return nullptr;
    tpl_deserialized = true;
  }
  G_ASSERT(serverTemplates.size() > templateId && serverTemplates[templateId].length());
  return serverTemplates[templateId].c_str();
}

#define NET_STAT_PROFILE_INITIAL_SIZES (DAGOR_DBGLEVEL > 0)
#if NET_STAT_PROFILE_INITIAL_SIZES
static eastl::vector_map<ecs::template_t, uint32_t> templatesSize;
static ska::flat_hash_map<uint32_t, uint32_t> templatesComponentSize;
void dump_initial_construction_stats(ecs::EntityManager &mgr)
{
  if (!templatesSize.size())
    return;
  G_STATIC_ASSERT(sizeof(ecs::template_t) + sizeof(ecs::component_index_t) == sizeof(uint32_t)); // if ever happen, can be replaced
                                                                                                 // with uint64_t
  debug("dumping_templates");
  eastl::vector<eastl::pair<ecs::template_t, size_t>> templSizes;
  eastl::vector<eastl::pair<ecs::component_index_t, size_t>> templCompSizes;
  templSizes.reserve(templatesSize.size());
  for (auto &t : templatesSize)
    templSizes.push_back(t);
  eastl::sort(templSizes.begin(), templSizes.end(), [](auto a, auto b) { return a.second > b.second; });
  for (size_t i = 0; i < min(templSizes.size(), size_t(20)); ++i)
  {
    auto &t = templSizes[i];
    debug("template %d (%s) total kbytes =%.2f", t.first, mgr.getTemplateName(t.first), t.second / (8. * 1024));
    templCompSizes.clear();
    size_t totalCompSizes = 0;
    for (auto &c : templatesComponentSize)
    {
      if ((c.first & 0xFFFF) == t.first)
      {
        templCompSizes.emplace_back(eastl::move(c.first >> 16), c.second);
        totalCompSizes += c.second;
      }
    }
    eastl::sort(templCompSizes.begin(), templCompSizes.end(), [](auto a, auto b) { return a.second > b.second; });

    debug("  template components total kb =%.2f, count = %d", totalCompSizes / (8. * 1024), templCompSizes.size());
    for (size_t j = 0; j < min(templCompSizes.size(), size_t(20)); ++j)
    {
      debug("    component %s bits =%d", mgr.getDataComponents().getComponentNameById(templCompSizes[j].first),
        templCompSizes[j].second);
    }
  }
  templatesSize.clear();
  templatesComponentSize.clear();
}
#else
void dump_initial_construction_stats(ecs::EntityManager &) {}
#endif

template <typename S>
static const char *replace_local_to_remote(const char *templ_name, S &tmps)
{
  static const char LOCAL[] = "_local";
  static const char REMOTE[] = "_remote";
  const char *lpos = strstr(templ_name, LOCAL);
  if (DAGOR_LIKELY(!lpos))
    return templ_name;
  tmps = templ_name;
  lpos = tmps.c_str() + (lpos - templ_name);
  do
  {
    const char &eolc = lpos[sizeof(LOCAL) - 1];
    if (!eolc || eolc == '+')
      tmps.replace(lpos, &eolc, REMOTE, sizeof(REMOTE) - 1);
    lpos = eolc ? strstr(&eolc + 1, LOCAL) : nullptr;
  } while (lpos);
  return tmps.c_str();
}

void Connection::serializeTemplate(danet::BitStream &bs, ecs::template_t templateIdx, eastl::bitvector<> &componentsSyncedTmp) const
{
  ecs::template_t templateId = serverIdxToTemplates[templateIdx];
  uint32_t archetype = mgr.getArchetypeByTemplateId(templateId);

  // iteration order must be in sync with serializeConstruction
  const TemplateCachedCompLists &compLists = get_template_cached_comp_lists(templateId);
  auto iterateReplicatable = [&](auto fn) {
    for (int cid : compLists.getReplicatedLocalIdx())
      fn(mgr.getArchetypeComponentIndex(archetype, cid));
    for (int cid : compLists.getReplicatedFilteredLocalIdx())
      fn(mgr.getArchetypeComponentIndex(archetype, cid));
  };

  bs.WriteCompressed(templateIdx); // ref to template
  {
    eastl::fixed_string<char, 128, true, framemem_allocator> tmps;
    bs.Write(replace_local_to_remote(mgr.getTemplateName(templateId), tmps)); // _local -> _remote
  }

  const BitSize_t blockSizePos = bs.GetWriteOffset();
  uint16_t componentsInTemplate = 0;
  bs.Write(componentsInTemplate);
  iterateReplicatable([&](ecs::component_index_t cidx) {
    componentsInTemplate++;
    write_component_index(cidx, bs);
    if (!componentsSyncedTmp.test(cidx, false))
    {
      // 8 bytes, if component is first time synced
      bs.Write(mgr.getDataComponents().getComponentTpById(cidx));
      bs.Write(mgr.getDataComponents().getComponentById(cidx).componentTypeName);
      componentsSyncedTmp.set(cidx, true);
    }
  });
  bs.WriteAt(componentsInTemplate, blockSizePos);
}

// see writeClientReplayKeyFrame
void Connection::serializeTemplateForClientReplay(danet::BitStream &bs, ecs::template_t templateIdx,
  eastl::bitvector<> &componentsSyncedTmp, dag::ConstSpan<ecs::component_index_t> clientToServerCidx) const
{
  bs.WriteCompressed(templateIdx); // ref to template
  bs.Write(serverTemplates[templateIdx].c_str());

  const BitSize_t blockSizePos = bs.GetWriteOffset();
  uint16_t componentsInTemplate = 0;
  bs.Write(componentsInTemplate);
  for (ecs::component_index_t cidx : clientTemplatesComponents[templateIdx])
  {
    const ecs::component_index_t serverCidx =
      cidx < clientToServerCidx.size() ? clientToServerCidx[cidx] : ecs::INVALID_COMPONENT_INDEX;
    G_ASSERT_CONTINUE(serverCidx != ecs::INVALID_COMPONENT_INDEX);
    componentsInTemplate++;
    write_component_index(serverCidx, bs);
    if (!componentsSyncedTmp.test(cidx, false))
    {
      // 8 bytes, if component is first time synced
      bs.Write(mgr.getDataComponents().getComponentTpById(cidx));
      bs.Write(mgr.getDataComponents().getComponentById(cidx).componentTypeName);
      componentsSyncedTmp.set(cidx, true);
    }
  }
  bs.WriteAt(componentsInTemplate, blockSizePos);
}

void Connection::serializeConstruction(ecs::EntityId eid, danet::BitStream &bs, CanSkipInitial canSkipInitial,
  SharedReplicationCache *cache)
{
#if NET_STAT_PROFILE_INITIAL_SIZES
  int beginWr = !isBlackHole() ? (int)bs.GetWriteOffset() : -1;
#endif
  const ecs::template_t templateId = mgr.getEntityTemplateId(eid);
  const TemplateCachedCompLists &compLists = get_template_cached_comp_lists(templateId);

  auto iterateReplicatable = [&](bool not_filtered, bool filtered, auto fn) {
    if (not_filtered)
      for (int cid : compLists.getReplicatedLocalIdx())
        fn(mgr.getEntityComponentRef(eid, cid), cid);
    if (filtered)
      for (int cid : compLists.getReplicatedFilteredLocalIdx())
        fn(mgr.getEntityComponentRef(eid, cid), cid);
  };

  // write template (full component list first time, then only id)
  ecs::template_t serverWrittenIdx =
    templateId < serverTemplatesIdx.size() ? serverTemplatesIdx[templateId] : ecs::INVALID_TEMPLATE_INDEX;
  if (serverWrittenIdx != ecs::INVALID_TEMPLATE_INDEX)
    bs.WriteCompressed(serverWrittenIdx);
  else
  {
    if (serverTemplatesIdx.size() <= templateId)
      serverTemplatesIdx.resize(templateId + 1, ecs::INVALID_TEMPLATE_INDEX);
    serverWrittenIdx = syncedTemplate++;
    serverIdxToTemplates.resize(syncedTemplate);
    G_FAST_ASSERT(serverWrittenIdx != ecs::INVALID_TEMPLATE_INDEX);
    serverTemplatesIdx[templateId] = serverWrittenIdx;
    serverIdxToTemplates[serverWrittenIdx] = templateId;
    bs.WriteCompressed(serverWrittenIdx);
    {
      eastl::fixed_string<char, 128, true, framemem_allocator> tmps;
      bs.Write(replace_local_to_remote(mgr.getTemplateName(templateId), tmps)); // _local -> _remote
    }
    const uint16_t componentsInTemplate = compLists.getReplicatedLocalIdx().size() + compLists.getReplicatedFilteredLocalIdx().size();
    bs.Write(componentsInTemplate);
    if (serverWrittenIdx >= serverTemplateComponentsCount.size())
      serverTemplateComponentsCount.resize(serverWrittenIdx + 1);
    serverTemplateComponentsCount[serverWrittenIdx] = componentsInTemplate;
    iterateReplicatable(true, true, [&](ecs::EntityComponentRef comp, uint16_t) {
      const ecs::component_index_t cidx = comp.getComponentId();
      write_component_index(cidx, bs);
      if (!componentsSynced.test(cidx, false))
      {
        bs.Write(mgr.getDataComponents().getComponentTpById(cidx));
        bs.Write(comp.getUserType());
        componentsSynced.set(cidx, true);
      }
    });
  }

  // write component data into separate bitstream + keep bitvector of used object key strings
  auto *cacheEntry = cache ? &cache->construction[eid] : nullptr;
  SharedReplicationCache::SerializerState state;
  const bool lessThan256 = serverTemplateComponentsCount[serverWrittenIdx] < 256;

  Object *object = nullptr;
  ObjectReplica *replica = nullptr;
  const auto ensureObjectAndReplica = [&] {
    if (object == nullptr)
    {
      object = Object::getByEid(mgr, eid);
      replica = getReplicaByEid(eid);
    }
  };

  const auto writeNextComp = [&](BitstreamSerializer &ser, ecs::EntityComponentRef comp, uint16_t cid,
                               auto &&skip_initial_replication) {
#if NET_STAT_PROFILE_INITIAL_SIZES
    auto beginWrComp = ser.bs.GetWriteOffset();
#endif
    if (compLists.isIgnored(comp.getComponentId()) || mgr.isEntityComponentSameAsTemplate(eid, comp, cid) ||
        (canSkipInitial == CanSkipInitial::Yes && skip_initial_replication(comp.getComponentId())))
    {
      // skip
    }
    else
    {
      // first written gets absolute index, rest are deltas
      const uint16_t ofs =
        (state.writtenComponents++ == 0) ? state.componentsInTemplate : uint16_t(state.componentsInTemplate - state.prevComponent - 1);
      if (lessThan256)
      {
        G_FAST_ASSERT(ofs <= UCHAR_MAX);
        ser.bs.Write(uint8_t(ofs));
      }
      else
        ser.bs.WriteCompressed(ofs);
      ecs::serialize_entity_component_ref_typeless(comp, ser, mgr);
      state.prevComponent = state.componentsInTemplate;
    }
    state.componentsInTemplate++;
#if NET_STAT_PROFILE_INITIAL_SIZES
    if (uint32_t bits = (beginWr >= 0) ? (ser.bs.GetWriteOffset() - beginWrComp) : 0)
      templatesComponentSize[templateId | (comp.getComponentId() << 16)] += bits;
#endif
  };

  danet::BitStream localStagingBs(framemem_ptr());
  danet::BitStream &stagingBs = cache ? cache->stagingBs : localStagingBs;
  stagingBs.ResetWritePointer();
  eastl::bitvector<framemem_allocator, uint64_t> usedKeys;
  const danet::BitStream *blobBs = &stagingBs;
  if (cacheEntry && cacheEntry->valid)
  {
    state = cacheEntry->state;
    usedKeys = cacheEntry->objectKeysUsed;
    blobBs = &cacheEntry->bs;
    if (!cacheEntry->forceReplicaVersionComps.empty())
    {
      ensureObjectAndReplica();
      for (auto [cidx, version] : cacheEntry->forceReplicaVersionComps)
        object->forceReplicaVersion(cidx, replica, version);
    }
  }
  else if (cacheEntry)
  {
    ensureObjectAndReplica();
    cacheEntry->bs.ResetWritePointer();
    BitstreamSerializer serializer(mgr, cacheEntry->bs, &objectKeysRepl, &usedKeys);
    iterateReplicatable(true, false, [&](ecs::EntityComponentRef comp, uint16_t cid) {
      writeNextComp(serializer, comp, cid, [&](ecs::component_index_t cidx) {
        if (int version = object->forceReplicaVersion(cidx, replica); version != -1)
          cacheEntry->forceReplicaVersionComps.emplace_back(cidx, net::compver_t(version));
        return false;
      });
    });
    cacheEntry->state = state;
    cacheEntry->objectKeysUsed = usedKeys;
    cacheEntry->valid = true;
    blobBs = &cacheEntry->bs;
  }
  else
  {
    ensureObjectAndReplica();
    BitstreamSerializer serializer(mgr, stagingBs, &objectKeysRepl, &usedKeys);
    iterateReplicatable(true, false, [&](ecs::EntityComponentRef comp, uint16_t cid) {
      writeNextComp(serializer, comp, cid, [&](ecs::component_index_t cidx) {
        object->forceReplicaVersion(cidx, replica);
        return false;
      });
    });
  }

  if (!compLists.getReplicatedFilteredLocalIdx().empty())
  {
    ensureObjectAndReplica();
    if (blobBs != &stagingBs)
    {
      stagingBs.reserveBits(blobBs->GetNumberOfBitsUsed());
      if (const uint32_t blobBytes = blobBs->GetNumberOfBytesUsed())
        memcpy(stagingBs.GetData(), blobBs->GetData(), blobBytes);
      stagingBs.SetWriteOffset(blobBs->GetNumberOfBitsUsed());
      blobBs = &stagingBs;
    }
    BitstreamSerializer serializer(mgr, stagingBs, &objectKeysRepl, &usedKeys);
    iterateReplicatable(false, true, [&](ecs::EntityComponentRef comp, uint16_t cid) {
      writeNextComp(serializer, comp, cid,
        [&](ecs::component_index_t cidx) { return object->skipInitialReplication(cidx, this, replica); });
    });
  }

  // write new object keys, that are needed for this construction packed, and were not replicated yet
  {
    const auto &usedKeysWords = usedKeys.get_container();
    const auto &replKeysWords = objectKeysRepl.serialized.get_container();
    for (uint32_t wordI = 0; wordI < usedKeysWords.size(); wordI++)
    {
      const uint64_t cur = wordI < replKeysWords.size() ? replKeysWords[wordI] : 0;
      const uint64_t missing = ~cur & usedKeysWords[wordI];
      if (DAGOR_LIKELY(missing == 0))
        continue;
      for (const uint32_t bitI : LsbVisitor{missing})
      {
        const uint32_t i = wordI * 64u + bitI;
        G_ASSERT(i < uint32_t(1 << OBJECT_KEY_BITS));
        const eastl::string &str = objectKeysRepl.shared->strings[i];
        bs.Write(str.c_str(), str.length() + 1);
        write_string_no(bs, i, OBJECT_KEY_BITS);
        objectKeysRepl.serialized.set(i, true);
      }
    }
    bs.Write(uint8_t(0)); // empty string
  }

  // component count and component data
  if (lessThan256)
    bs.Write(uint8_t(state.writtenComponents));
  else
    bs.Write(uint16_t(state.writtenComponents));
  // ensure alignment of the blob matches bs
  bs.WriteAlignedBytes(blobBs->GetData(), blobBs->GetNumberOfBytesUsed());

  G_ASSERT(lessThan256 == (state.componentsInTemplate < 256));

  // Remove this check when we sure enough that it's not happens and use bitmap for storing info about count < 256 instead
  if (DAGOR_UNLIKELY(state.componentsInTemplate != serverTemplateComponentsCount[serverWrittenIdx]))
    logerr("Inconsistent replication components count %d (cur) != %d (initial) in template %d<%s>", state.componentsInTemplate,
      serverTemplateComponentsCount[serverWrittenIdx], templateId, mgr.getEntityTemplateName(eid));

#if NET_STAT_PROFILE_INITIAL_SIZES
  if (beginWr >= 0)
    templatesSize[templateId] += bs.GetWriteOffset() - beginWr;
#endif
}

bool Connection::deserializeComponentConstruction(ecs::template_t server_template, const danet::BitStream &bs,
  ecs::ComponentsInitializer &init, int &out_ncomp)
{
  G_ASSERT(objectKeysRepl.shared == &objectKeysLocal);
  BitstreamDeserializer deserializer(mgr, bs, &objectKeysRepl);
  uint16_t compCount = 0;
  const uint16_t templateComponentsCount = clientTemplatesComponents[server_template].size();
  if (templateComponentsCount < 256)
  {
    uint8_t compCount8 = 0;
    if (!bs.Read(compCount8))
      return false;
    compCount = compCount8;
  }
  else if (!bs.Read(compCount))
    return false;
  // component data is aligned at the start
  bs.AlignReadToByteBoundary();
  for (uint16_t comp = 0, i = 0; i < compCount; ++i)
  {
    uint16_t ofs;
    if (templateComponentsCount < 256)
    {
      uint8_t ofs8;
      if (!bs.Read(ofs8))
        return false;
      ofs = ofs8;
    }
    else if (!bs.ReadCompressed(ofs))
      return false;
    comp = (i == 0) ? ofs : (comp + ofs + 1);
    if (comp >= templateComponentsCount)
    {
      logerr("Invalid template component index %d for template local idx %d<%s> (count %d)", comp, server_template,
        serverTemplates[server_template].c_str(), templateComponentsCount);
      return false;
    }
    ecs::component_index_t cidx = clientTemplatesComponents[server_template][comp];
    if (cidx == ecs::INVALID_COMPONENT_INDEX)
      return false;
    ecs::component_type_t componentTypeName = mgr.getDataComponents().getComponentById(cidx).componentTypeName;
    ecs::component_t componentNameHash = mgr.getDataComponents().getComponentTpById(cidx);
    if (ecs::MaybeChildComponent mbcomp = deserialize_init_component_typeless(componentTypeName, cidx, deserializer, mgr))
    {
      if (!mbcomp->isNull())
        init[ecs::HashedConstString({"!net_replicated!", componentNameHash})] = eastl::move(*mbcomp);
    }
    else
      return false;
  }
  out_ncomp = compCount;
  bs.AlignReadToByteBoundary();
  return true;
}

ecs::EntityId Connection::deserializeConstruction(const danet::BitStream &bs, ecs::entity_id_t serverId, uint32_t sz, float cratio,
  ecs::create_entity_async_cb_t &&cb)
{
  G_ASSERT(serverId != ecs::ECS_INVALID_ENTITY_ID_VAL);
  G_UNUSED(sz);
  G_UNUSED(cratio);

  ecs::template_t serverTemplate = ecs::INVALID_TEMPLATE_INDEX;
  bool templDeserialized = false;
  const char *templName = deserializeTemplate(bs, serverTemplate, templDeserialized);
  if (!templName)
  {
    logerr("Failed to deserialize template for server entity <%d>", serverId);
    return ecs::INVALID_ENTITY_ID;
  }

  // read new object key strings
  {
    while (true)
    {
      uint32_t idx = 0;
      eastl::string str;
      if (read_string(bs, str) < 0)
        return ecs::INVALID_ENTITY_ID;
      if (str.empty())
        break;
      if (!read_string_no(bs, idx, OBJECT_KEY_BITS))
        return ecs::INVALID_ENTITY_ID;
      InternedStringsShared &all = *objectKeysRepl.shared;
      if (idx >= all.strings.size())
        all.strings.resize(idx + 1);
      G_ASSERT(all.strings.size() <= uint32_t(1 << OBJECT_KEY_BITS));
      all.strings[idx] = eastl::move(str);
    }
  }

  ecs::ComponentsInitializer ainit;
  int ncomp = 0;
  if (!deserializeComponentConstruction(serverTemplate, bs, ainit, ncomp))
    return ecs::INVALID_ENTITY_ID;

  const ecs::Template *templ = mgr.buildTemplateByName(templName);
  if (!templ)
  {
    logerr("Unknown template <%s> for server entity <%d>", templName, serverId);
    return ecs::INVALID_ENTITY_ID;
  }

  ecs::ComponentsMap amap;
  amap[ECS_HASH("serverEid")] = ecs::ChildComponent((int)serverId); // temp component for client only

  ecs::EntityId srvEid(serverId);
  DAECS_EXT_ASSERTF(srvEid.index() <= ecs::MAX_RESERVED_EID_IDX, "%d", serverId);
  mgr.forceServerEidGeneration(srvEid);

#if DAGOR_DBGLEVEL > 0
  G_ASSERT(mgr.doesEntityExist(srvEid));
  G_ASSERTF(!mgr.getEntityTemplateName(srvEid), "entity %d, server %d already has a template <%s> while trying to create <%s>!",
    ecs::entity_id_t(srvEid), serverId, mgr.getEntityTemplateName(srvEid), templName);
#endif
#if DAECS_EXTENSIVE_CHECKS
  if (!templ->hasComponent(ECS_HASH("noECSDebug"), mgr.getTemplateDB().data()))
    debug("create <%d> of server<%d> %stemplate <%s> %d bytes in %d/%d comps (cratio=%.3f,cpacket_seq=%u)", ecs::entity_id_t(srvEid),
      serverId, templDeserialized ? "initial " : "", templName, sz, ncomp, clientTemplatesComponents[serverTemplate].size(), cratio,
      constructionPacketSequence);
#endif
  G_VERIFY(srvEid == mgr.reCreateEntityFromAsync(srvEid, templName, eastl::move(ainit), eastl::move(amap), eastl::move(cb)));
  return srvEid;
}


#undef IMPL_IT
} // namespace net
