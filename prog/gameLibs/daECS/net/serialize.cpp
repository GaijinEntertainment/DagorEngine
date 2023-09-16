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
#include <limits.h>

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

InternedStrings::InternedStrings()
{
  index.emplace("", 0);
  strings.emplace_back("");
}

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
static const char *read_istring(const danet::BitStream &cb, InternedStrings *istrs, uint32_t short_bits)
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
  else if (!istrs)
    return nullptr;
  uint32_t str;
  if (!read_string_no(cb, str, short_bits))
    return nullptr;
  InternedStrings &all = *istrs;
  if (str < all.strings.size() && (str == 0 || all.strings[str].length()))
  {
    return all.strings[str].c_str();
  }
  if (str >= all.strings.size())
    all.strings.resize(str + 1);
  G_ASSERT(all.strings.size() <= uint32_t(1 << short_bits));
  eastl::string &it = all.strings[str];
  if (read_string(cb, it) < 0)
    return nullptr;
  return it.c_str();
}

static void write_raw_string(danet::BitStream &cb, const eastl::string &pStr)
{
  cb.Write(true);
  cb.Write(pStr.c_str(), pStr.length() + 1);
}

static void write_string(danet::BitStream &cb, const eastl::string &pStr, InternedStrings &all, uint32_t short_bits)
{
  auto it = all.index.find(pStr);
  if (it == all.index.end())
  {
    if (all.strings.size() >= uint32_t(1 << short_bits))
    {
      write_raw_string(cb, pStr);
      return;
    }
    cb.Write(false);
    write_string_no(cb, all.strings.size(), short_bits);
    all.strings.emplace_back(pStr);
    cb.Write(all.strings.back().c_str(), all.strings.back().length() + 1);
    all.index.emplace(all.strings.back(), all.strings.size() - 1);
  }
  else
  {
    cb.Write(false);
    write_string_no(cb, it->second, short_bits);
  }
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
    obj.reserve(cnt);
    for (uint32_t i = 0; i < cnt; ++i)
    {
      const char *str = read_istring(bs, objectKeys, OBJECT_KEY_BITS);
      if (!str)
        return false;
      auto &item = obj.insert(str); // insert before deserialization since 'str' might be reused in next calls
      if (ecs::MaybeChildComponent maybeComp = ecs::deserialize_child_component(*this))
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
  auto &componentTypes = g_entity_mgr->getComponentTypes();
  const ecs::ComponentType componentTypeInfo = componentTypes.getTypeInfo(compInfo.componentType);
  const bool isPod = ecs::is_pod(componentTypeInfo.flags);
  ecs::ComponentSerializer *typeIO = nullptr;
  if (compInfo.flags & ecs::DataComponent::HAS_SERIALIZER)
    typeIO = g_entity_mgr->getDataComponents().getComponentIO(cidx);
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
    ctm->create(tempData, *g_entity_mgr.get(), ecs::INVALID_ENTITY_ID, ecs::ComponentsMap(), compInfo.componentType);
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
    if (objectKeys)
    {
      for (auto &it : obj)
      {
        write_string(bs, ecs::get_key_string(it.first), *objectKeys, OBJECT_KEY_BITS);
        ecs::serialize_child_component(it.second, *this);
      }
    }
    else
    {
      for (auto &it : obj)
      {
        write_raw_string(bs, ecs::get_key_string(it.first));
        ecs::serialize_child_component(it.second, *this);
      }
    }
  }
  else if (user_type == ecs::ComponentTypeInfo<ecs::string>::type)
    ecs::write_string(*this, ((const ecs::string *)from)->c_str(), ecs::MAX_STRING_LENGTH);
  else
    bs.WriteBits((const uint8_t *)from, sz_in_bits);
}

void serialize_comp_nameless(ecs::component_t name, const ecs::EntityComponentRef &comp, danet::BitStream &bs)
{
  BitstreamSerializer serializer(bs);
  serializer.bs.Write(name);
  ecs::component_type_t userType = comp.getUserType();
  serializer.write(&userType, sizeof(userType) * 8, 0);
  // todo: write and read component index
  serialize_entity_component_ref_typeless(comp.getRawData(), ecs::INVALID_COMPONENT_INDEX, comp.getUserType(), comp.getTypeId(),
    serializer);
}

ecs::MaybeChildComponent deserialize_comp_nameless(ecs::component_t &name, const danet::BitStream &bs) // todo: replace with component
                                                                                                       // index
{
  BitstreamDeserializer deserializer(bs);
  ecs::component_type_t userType = 0;
  // todo: write and read component index
  if (deserializer.bs.Read(name) && deserializer.read(&userType, sizeof(userType) * CHAR_BIT, 0))
    return ecs::deserialize_init_component_typeless(userType, ecs::INVALID_COMPONENT_INDEX, deserializer);
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
      g_entity_mgr->getDataComponents().getComponentNameById(comp.getComponentId()),
      g_entity_mgr->getComponentTypes().getTypeNameById(comp.getTypeId()), (ecs::entity_id_t)eid,
      g_entity_mgr->getEntityTemplateName(eid));
    return false;
  }
  write_component_index(comp.getComponentId(), bs);
  BitstreamSerializer serializer(bs);
  ecs::serialize_entity_component_ref_typeless(comp, serializer);
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
  BitstreamDeserializer bsds(bs, &objectKeys);
  ecs::EntityComponentRef cref = g_entity_mgr->getComponentRefRW(eid, clientCidx);
  bool crefIsNull = cref.isNull();
  if (EASTL_LIKELY(!crefIsNull && deserialize_component_typeless(cref, bsds)))
  {
    replicated_component_on_client_deserialize(eid, clientCidx);
    return true;
  }
  ecs::DataComponent compInfo = g_entity_mgr->getDataComponents().getComponentById(clientCidx);
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
    g_entity_mgr->getDataComponents().getComponentNameById(clientCidx),
    g_entity_mgr->getDataComponents().getComponentTpById(clientCidx), clientCidx, serverCidx,
    g_entity_mgr->getComponentTypes().getTypeNameById(compInfo.componentType), compInfo.componentTypeName, compInfo.componentType,
    (ecs::entity_id_t)eid, g_entity_mgr->getEntityTemplateName(eid));
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
  ecs::component_index_t clientCidx = g_entity_mgr->getDataComponents().findComponentId(name); // try to immediately resolve component
  if (clientCidx == ecs::INVALID_COMPONENT_INDEX)                                              // component is missing on client
  {
    const ecs::type_index_t typeIdx = g_entity_mgr->getComponentTypes().findType(type);
    if (error)
    {
      int loglev = typeIdx != ecs::INVALID_COMPONENT_TYPE_INDEX ? LOGLEVEL_WARN : LOGLEVEL_ERR;
      G_UNUSED(loglev);
#if DAECS_EXTENSIVE_CHECKS
      loglev = LOGLEVEL_ERR;
#endif
      logmessage(loglev, "component scidx=%d, name=0x%X type=0x%X(%s) is missing in template <%s> on client", serverCidx, name, type,
        g_entity_mgr->getComponentTypes().getTypeNameById(typeIdx), serverTemplates[templateId].c_str());
    }
    if (typeIdx != ecs::INVALID_COMPONENT_TYPE_INDEX)
      clientCidx =
        g_entity_mgr->createComponent(ecs::HashedConstString{nullptr, name}, typeIdx, dag::Span<ecs::component_t>(), nullptr, 0);
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

  const int templId = g_entity_mgr->buildTemplateIdByName(serverTemplates[templateId].c_str());
  if (templId >= 0)
    g_entity_mgr->instantiateTemplate(templId);
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

#define NET_STAT_PROFILE_INITIAL_SIZES (DAECS_EXTENSIVE_CHECKS)
#if NET_STAT_PROFILE_INITIAL_SIZES
static eastl::vector_map<ecs::template_t, uint32_t> templatesSize;
static ska::flat_hash_map<uint32_t, uint32_t> templatesComponentSize;
void dump_initial_construction_stats()
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
    debug("template %d (%s) total kbytes =%.2f", t.first, g_entity_mgr->getTemplateName(t.first), t.second / (8. * 1024));
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
      debug("    component %s bits =%d", g_entity_mgr->getDataComponents().getComponentNameById(templCompSizes[j].first),
        templCompSizes[j].second);
    }
  }
  templatesSize.clear();
  templatesComponentSize.clear();
}
#else
void dump_initial_construction_stats() {}
#endif

extern const ecs::component_index_t *get_template_ignored_initial_components(ecs::template_t);

template <typename S>
static const char *replace_local_to_remote(const char *templ_name, S &tmps)
{
  static const char LOCAL[] = "_local";
  static const char REMOTE[] = "_remote";
  const char *lpos = strstr(templ_name, LOCAL);
  if (EASTL_LIKELY(!lpos))
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
  uint32_t archetype = g_entity_mgr->getArchetypeByTemplateId(templateId);

  auto iterateReplicatable = [&](auto fn) {
    int componentsCount = g_entity_mgr->getArchetypeNumComponents(archetype);
    for (int cid = 0; cid < componentsCount; ++cid)
    {
      ecs::component_index_t cidx = g_entity_mgr->getArchetypeComponentIndex(archetype, cid);
      if (ecs::should_replicate(cidx)) // never written
        fn(cidx);
    }
  };

  bs.WriteCompressed(templateIdx); // ref to template
  {
    eastl::fixed_string<char, 128, true, framemem_allocator> tmps;
    bs.Write(replace_local_to_remote(g_entity_mgr->getTemplateName(templateId), tmps)); // _local -> _remote
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
      bs.Write(g_entity_mgr->getDataComponents().getComponentTpById(cidx));
      bs.Write(g_entity_mgr->getDataComponents().getComponentById(cidx).componentTypeName);
      componentsSyncedTmp.set(cidx, true);
    }
  });
  bs.WriteAt(componentsInTemplate, blockSizePos);
}

void Connection::serializeConstruction(ecs::EntityId eid, danet::BitStream &bs, CanSkipInitial canSkipInitial)
{
#if NET_STAT_PROFILE_INITIAL_SIZES
  BitSize_t beginWr = bs.GetWriteOffset();
#endif
  const int componentsCount = g_entity_mgr->getNumComponents(eid);
  const ecs::template_t templateId = g_entity_mgr->getEntityTemplateId(eid);

  Object *object = Object::getByEid(eid);
  ObjectReplica *replica = getReplicaByEid(eid);

  auto iterateReplicatable = [&, componentsCount](auto fn) {
    for (int cid = 0; cid < componentsCount; ++cid)
    {
      auto comp = g_entity_mgr->getEntityComponentRef(eid, cid);
      if (should_replicate(comp)) // never written
        fn(comp, cid);
    }
  };
  ecs::template_t serverWrittenIdx =
    templateId < serverTemplatesIdx.size() ? serverTemplatesIdx[templateId] : ecs::INVALID_TEMPLATE_INDEX;
  if (serverWrittenIdx != ecs::INVALID_TEMPLATE_INDEX)
    bs.WriteCompressed(serverWrittenIdx); // ref to template
  else                                    // not yet synced/known to client
  {
    // it is too expensive to always serialize template name, so we just rely on construction being reliable ordered, and write name
    // once.
    if (serverTemplatesIdx.size() <= templateId)
      serverTemplatesIdx.resize(templateId + 1, ecs::INVALID_TEMPLATE_INDEX);
    serverWrittenIdx = syncedTemplate++;
    serverIdxToTemplates.resize(syncedTemplate);
    G_FAST_ASSERT(serverWrittenIdx != ecs::INVALID_TEMPLATE_INDEX);
    serverTemplatesIdx[templateId] = serverWrittenIdx;
    serverIdxToTemplates[serverWrittenIdx] = templateId;
    bs.WriteCompressed(serverWrittenIdx); // ref to template
    {
      eastl::fixed_string<char, 128, true, framemem_allocator> tmps;
      bs.Write(replace_local_to_remote(g_entity_mgr->getTemplateName(templateId), tmps)); // _local -> _remote
    }
    // todo: actually, we'd better serialize template, if it is different on server and client. Would require some state to check.
    const BitSize_t blockSizePos = bs.GetWriteOffset();
    uint16_t componentsInTemplate = 0;
    bs.Write(componentsInTemplate);
    iterateReplicatable([&](ecs::EntityComponentRef comp, uint16_t) {
      componentsInTemplate++;
      const ecs::component_index_t cidx = comp.getComponentId();
      write_component_index(cidx, bs);
      if (!componentsSynced.test(cidx, false))
      {
        // 8 bytes, if component is first time synced
        bs.Write(g_entity_mgr->getDataComponents().getComponentTpById(cidx));
        bs.Write(comp.getUserType());
        componentsSynced.set(cidx, true);
      }
    });
    bs.WriteAt(componentsInTemplate, blockSizePos);
    if (serverWrittenIdx >= serverTemplateComponentsCount.size())
      serverTemplateComponentsCount.resize(serverWrittenIdx + 1);
    serverTemplateComponentsCount[serverWrittenIdx] = componentsInTemplate;
  }

  const ecs::component_index_t *ignoredComponents = get_template_ignored_initial_components(templateId), *ignoredComponentsE = nullptr;
  if (ignoredComponents)
  {
    size_t nComp = *ignoredComponents++;
    ignoredComponentsE = ignoredComponents + nComp;
  }

  BitstreamSerializer serializer(bs, &objectKeys);
  const BitSize_t countSizePos = bs.GetWriteOffset();
  const bool lessThan256 = serverTemplateComponentsCount[serverWrittenIdx] < 256;
  uint16_t componentsInTemplate = 0, prevComponent = 0, writtenComponents = 0;
  if (lessThan256)
    bs.Write(uint8_t(writtenComponents));
  else
    bs.Write(writtenComponents);
  iterateReplicatable([&, eid, object, replica](ecs::EntityComponentRef comp, uint16_t cid) {
#if NET_STAT_PROFILE_INITIAL_SIZES
    auto beginWrComp = serializer.bs.GetWriteOffset();
#endif
    if ((ignoredComponents != nullptr && eastl::binary_search(ignoredComponents, ignoredComponentsE, comp.getComponentId())) ||
        g_entity_mgr->isEntityComponentSameAsTemplate(eid, comp, cid) ||
        (canSkipInitial == CanSkipInitial::Yes && object->skipInitialReplication(comp.getComponentId(), this, replica)))
    {
      // skip
    }
    else
    {
      // first one is not diff
      const uint16_t ofs = (writtenComponents++ == 0) ? componentsInTemplate : uint16_t(componentsInTemplate - prevComponent - 1);
      if (lessThan256) // write one byte
      {
        G_FAST_ASSERT(ofs <= UCHAR_MAX);
        serializer.bs.Write(uint8_t(ofs));
      }
      else
        serializer.bs.WriteCompressed(ofs); // write compressed ofs
      ecs::serialize_entity_component_ref_typeless(comp, serializer);
      prevComponent = componentsInTemplate;
    }
    componentsInTemplate++;
#if NET_STAT_PROFILE_INITIAL_SIZES
    const uint32_t bits = serializer.bs.GetWriteOffset() - beginWrComp;
    if (bits && !isBlackHole())
      templatesComponentSize[templateId | (comp.getComponentId() << 16)] += bits;
#endif
  });
  G_ASSERT(lessThan256 == (componentsInTemplate < 256));

  // Remove this check when we sure enough that it's not happens and use bitmap for storing info about count < 256 instead
  if (EASTL_UNLIKELY(componentsInTemplate != serverTemplateComponentsCount[serverWrittenIdx]))
    logerr("Inconsistent replication components count %d (cur) != %d (initial) in template %d<%s>", componentsInTemplate,
      serverTemplateComponentsCount[serverWrittenIdx], templateId, g_entity_mgr->getEntityTemplateName(eid));

  if (lessThan256)
    bs.WriteAt(uint8_t(writtenComponents), countSizePos);
  else
    bs.WriteAt(uint16_t(writtenComponents), countSizePos);
#if NET_STAT_PROFILE_INITIAL_SIZES
  if (!isBlackHole())
    templatesSize[templateId] += bs.GetWriteOffset() - beginWr;
#endif
}

bool Connection::deserializeComponentConstruction(ecs::template_t server_template, const danet::BitStream &bs,
  ecs::ComponentsInitializer &init, int &out_ncomp)
{
  BitstreamDeserializer deserializer(bs, &objectKeys);
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
    ecs::component_type_t componentTypeName = g_entity_mgr->getDataComponents().getComponentById(cidx).componentTypeName;
    ecs::component_t componentNameHash = g_entity_mgr->getDataComponents().getComponentTpById(cidx);
    if (ecs::MaybeChildComponent mbcomp = deserialize_init_component_typeless(componentTypeName, cidx, deserializer))
    {
      if (!mbcomp->isNull())
        init[ecs::HashedConstString({"!net_replicated!", componentNameHash})] = eastl::move(*mbcomp);
    }
    else
      return false;
  }
  out_ncomp = compCount;
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

  ecs::ComponentsInitializer ainit;
  int ncomp = 0;
  if (!deserializeComponentConstruction(serverTemplate, bs, ainit, ncomp))
    return ecs::INVALID_ENTITY_ID;

  const ecs::Template *templ = g_entity_mgr->buildTemplateByName(templName);
  if (!templ)
  {
    logerr("Unknown template <%s> for server entity <%d>", templName, serverId);
    return ecs::INVALID_ENTITY_ID;
  }

  ecs::ComponentsMap amap;
  amap[ECS_HASH("serverEid")] = ecs::ChildComponent((int)serverId); // temp component for client only

  ecs::EntityId srvEid(serverId);
  DAECS_EXT_ASSERTF(srvEid.index() <= ecs::MAX_RESERVED_EID_IDX, "%d", serverId);
  g_entity_mgr->forceServerEidGeneration(srvEid);

#if DAGOR_DBGLEVEL > 0
  G_ASSERT(g_entity_mgr->doesEntityExist(srvEid));
  G_ASSERTF(!g_entity_mgr->getEntityTemplateName(srvEid),
    "entity %d, server %d already has a template <%s> while trying to create <%s>!", ecs::entity_id_t(srvEid), serverId,
    g_entity_mgr->getEntityTemplateName(srvEid), templName);
#endif
#if DAECS_EXTENSIVE_CHECKS
  if (!templ->hasComponent(ECS_HASH("noECSDebug")))
    debug("create <%d> of server<%d> %stemplate <%s> %d bytes in %d/%d comps (cratio=%.3f,cpacket_seq=%u)", ecs::entity_id_t(srvEid),
      serverId, templDeserialized ? "initial " : "", templName, sz, ncomp, clientTemplatesComponents[serverTemplate].size(), cratio,
      constructionPacketSequence);
#endif
  G_VERIFY(srvEid == g_entity_mgr->reCreateEntityFromAsync(srvEid, templName, eastl::move(ainit), eastl::move(amap), eastl::move(cb)));
  return srvEid;
}


#undef IMPL_IT
} // namespace net
