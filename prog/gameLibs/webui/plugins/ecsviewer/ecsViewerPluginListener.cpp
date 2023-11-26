#include <ctype.h>
#include <stdlib.h>
#include <debug/dag_debug.h>
#include <generic/dag_tab.h>
#include <math/dag_mathBase.h>
#include <math/dag_e3dColor.h>
#include <memory/dag_framemem.h>
#include <memory/dag_mem.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_clipboard.h>
#include <perfMon/dag_autoFuncProf.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <util/dag_base64.h>
#include <util/dag_watchdog.h>
#include <util/dag_simpleString.h>
#include <webui/httpserver.h>
#include <webui/helpers.h>
#include <webui/bsonUtils.h>
#include <webui/websocket/webSocketStream.h>

#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/internal/performQuery.h>
#include <daECS/core/sharedComponent.h>
#include <daECS/io/blk.h>

#include <ecs/scripts/sq/queryExpression.h>

#include <EASTL/string.h>

extern ecs::EntityManager *ecsViewerEntityManager;

static constexpr size_t BSON_RESERVE_BYTES = 4 << 20;       // 4 MB
static constexpr size_t BSON_STACK_RESERVE_BYTES = 1 << 10; // 1 KB

static inline BsonStream bson_ctor() { return BsonStream(BSON_RESERVE_BYTES, BSON_STACK_RESERVE_BYTES); }

template <typename T>
static void bson_ecs_list(BsonStream &bson, const char *name, const ecs::EntityComponentRef &comp);
static void bson_ecs_array(BsonStream &bson, const char *name, const ecs::EntityComponentRef &comp);
static void bson_ecs_object(BsonStream &bson, const char *name, const ecs::EntityComponentRef &comp);
static void bson_ecs_shared_array(BsonStream &bson, const char *name, const ecs::EntityComponentRef &comp);
static void bson_ecs_shared_object(BsonStream &bson, const char *name, const ecs::EntityComponentRef &comp);

template <typename T>
static void bson_ecs_component(BsonStream &bson, const char *name, const T &comp)
{
  bson.add(name, comp);
}

template <>
void bson_ecs_component(BsonStream &bson, const char *name, const int &comp)
{
  bson.begin(name);
  bson.add("i", comp);
  bson.end();
}

template <>
void bson_ecs_component(BsonStream &bson, const char *name, const ecs::string &comp)
{
  bson.add(name, comp.c_str());
}

template <>
void bson_ecs_component(BsonStream &bson, const char *name, const ecs::EntityId &comp)
{
  bson.begin(name);
  bson.add("eid", (uint32_t)comp);
  bson.end();
}

template <>
void bson_ecs_component(BsonStream &bson, const char *name, const float &comp)
{
  bson.begin(name);
  bson.add("r", comp);
  bson.end();
}

template <>
void bson_ecs_component(BsonStream &bson, const char *name, const bool &comp)
{
  bson.add(name, comp ? "true" : "false");
}

static void bson_ecs_component(BsonStream &bson, const char *name, const ecs::EntityComponentRef &comp)
{
  if (comp.is<bool>())
    bson_ecs_component(bson, name, comp.get<bool>());
  else if (comp.is<ecs::string>())
    bson_ecs_component(bson, name, comp.get<ecs::string>());
  else if (comp.is<ecs::EntityId>())
    bson_ecs_component(bson, name, comp.get<ecs::EntityId>());
  else if (comp.is<float>())
    bson_ecs_component(bson, name, comp.get<float>());
  else if (comp.is<int>())
    bson_ecs_component(bson, name, comp.get<int>());
  else if (comp.is<uint8_t>())
  {
    const int ival = comp.get<uint8_t>();
    bson_ecs_component(bson, name, ival);
  }
  else if (comp.is<E3DCOLOR>())
    bson.add(name, comp.get<E3DCOLOR>());
  else if (comp.is<Point2>())
    bson.add(name, comp.get<Point2>());
  else if (comp.is<Point3>())
    bson.add(name, comp.get<Point3>());
  else if (comp.is<DPoint3>())
    bson.add(name, comp.get<DPoint3>());
  else if (comp.is<Point4>())
    bson.add(name, comp.get<Point4>());
  else if (comp.is<IPoint2>())
    bson.add(name, comp.get<IPoint2>());
  else if (comp.is<IPoint3>())
    bson.add(name, comp.get<IPoint3>());
  else if (comp.is<IPoint4>())
    bson.add(name, comp.get<IPoint4>());
  else if (comp.is<TMatrix>())
    bson.add(name, comp.get<TMatrix>());
  else if (comp.is<ecs::Object>())
    bson_ecs_object(bson, name, comp);
  else if (comp.is<ecs::Array>())
    bson_ecs_array(bson, name, comp);
  else if (comp.is<ecs::IntList>())
    bson_ecs_list<ecs::IntList>(bson, name, comp);
  else if (comp.is<ecs::UInt8List>())
    bson_ecs_list<ecs::UInt8List>(bson, name, comp);
  else if (comp.is<ecs::UInt16List>())
    bson_ecs_list<ecs::UInt16List>(bson, name, comp);
  else if (comp.is<ecs::StringList>())
    bson_ecs_list<ecs::StringList>(bson, name, comp);
  else if (comp.is<ecs::EidList>())
    bson_ecs_list<ecs::EidList>(bson, name, comp);
  else if (comp.is<ecs::FloatList>())
    bson_ecs_list<ecs::FloatList>(bson, name, comp);
  else if (comp.is<ecs::Point2List>())
    bson_ecs_list<ecs::Point2List>(bson, name, comp);
  else if (comp.is<ecs::Point3List>())
    bson_ecs_list<ecs::Point3List>(bson, name, comp);
  else if (comp.is<ecs::Point4List>())
    bson_ecs_list<ecs::Point4List>(bson, name, comp);
  else if (comp.is<ecs::IPoint2List>())
    bson_ecs_list<ecs::IPoint2List>(bson, name, comp);
  else if (comp.is<ecs::IPoint3List>())
    bson_ecs_list<ecs::IPoint3List>(bson, name, comp);
  else if (comp.is<ecs::BoolList>())
    bson_ecs_list<ecs::BoolList>(bson, name, comp);
  else if (comp.is<ecs::TMatrixList>())
    bson_ecs_list<ecs::TMatrixList>(bson, name, comp);
  else if (comp.is<ecs::ColorList>())
    bson_ecs_list<ecs::ColorList>(bson, name, comp);
  else if (comp.is<ecs::Int64List>())
    bson_ecs_list<ecs::Int64List>(bson, name, comp);
  else if (comp.is<ecs::SharedComponent<ecs::Object>>())
    bson_ecs_shared_object(bson, name, comp);
  else if (comp.is<ecs::SharedComponent<ecs::Array>>())
    bson_ecs_shared_array(bson, name, comp);
  else
    bson.add(name, "<unknown>");
}

static void bson_ecs_object(BsonStream &bson, const char *name, const ecs::EntityComponentRef &comp)
{
  bson.begin(name);
  for (const auto &obj : comp.get<ecs::Object>())
    bson_ecs_component(bson, obj.first.c_str(), obj.second.getEntityComponentRef());
  bson.end();
}

static void bson_ecs_array(BsonStream &bson, const char *name, const ecs::EntityComponentRef &comp)
{
  bson.beginArray(name);
  for (BsonStream::StringIndex i; i.idx() < comp.get<ecs::Array>().size(); i.increment())
    bson_ecs_component(bson, i.str(), comp.get<ecs::Array>()[i.idx()].getEntityComponentRef());
  bson.end();
}

template <typename T>
static void bson_ecs_list(BsonStream &bson, const char *name, const ecs::EntityComponentRef &comp)
{
  bson.beginArray(name);
  for (BsonStream::StringIndex i; i.idx() < comp.get<T>().size(); i.increment())
    bson_ecs_component(bson, i.str(), comp.get<T>()[i.idx()]);
  bson.end();
}

static void bson_ecs_shared_object(BsonStream &bson, const char *name, const ecs::EntityComponentRef &comp)
{
  bson.begin(name);
  for (const auto &obj : *comp.get<ecs::SharedComponent<ecs::Object>>().get())
    bson_ecs_component(bson, obj.first.c_str(), obj.second.getEntityComponentRef());
  bson.end();
}

static void bson_ecs_shared_array(BsonStream &bson, const char *name, const ecs::EntityComponentRef &comp)
{
  const auto &arr = *comp.get<ecs::SharedComponent<ecs::Array>>().get();
  bson.beginArray(name);
  for (BsonStream::StringIndex i; i.idx() < arr.size(); i.increment())
    bson_ecs_component(bson, i.str(), arr[i.idx()].getEntityComponentRef());
  bson.end();
}

static bool is_shared_array(const char *name)
{
  return name &&
         (::strcmp(name, "ecs::SharedComponent<ecs::Array>") == 0 || ::strcmp(name, "ecs::SharedComponent< ::ecs::Array>") == 0);
}

static bool is_shared_object(const char *name)
{
  return name &&
         (::strcmp(name, "ecs::SharedComponent<ecs::Object>") == 0 || ::strcmp(name, "ecs::SharedComponent< ::ecs::Object>") == 0);
}

struct ECSMessageListener : public websocket::MessageListener
{
  ECSMessageListener()
  {
    addCommand("getComponents", (Method)&ECSMessageListener::getComponents);
    addCommand("getSystems", (Method)&ECSMessageListener::getSystems);
    addCommand("getQueries", (Method)&ECSMessageListener::getQueries);
    addCommand("getEvents", (Method)&ECSMessageListener::getEvents);
    addCommand("getTemplates", (Method)&ECSMessageListener::getTemplates);
    addCommand("getEntities", (Method)&ECSMessageListener::getEntities);
    addCommand("getEntityAttributes", (Method)&ECSMessageListener::getEntityAttributes);
    addCommand("setEntityAttribute", (Method)&ECSMessageListener::setEntityAttribute);
    addCommand("performDynamicQuery", (Method)&ECSMessageListener::performDynamicQuery);
  }

  void getComponents(const DataBlock & /* blk */)
  {
    AutoFuncProfT<AFP_MSEC> _prof("[WEBUI] ECSMessageListener::getComponents %d msec");

    const ecs::ComponentTypes &componentTypes = ecsViewerEntityManager->getComponentTypes();
    const ecs::DataComponents &dataComponents = ecsViewerEntityManager->getDataComponents();

    BsonStream bson = bson_ctor();

    BsonStream::StringIndex i;

    bson.beginArray("components");

    for (int compNo = 0; compNo < dataComponents.size(); ++compNo)
    {
      auto comp = dataComponents.getComponentById(compNo);
      if (comp.flags & ecs::DataComponent::IS_COPY)
        continue;
      const char *compName = dataComponents.getComponentNameById(compNo);

      bson.begin(i.str());

      bson.add("name", compName);
      const char *typeName = componentTypes.getTypeNameById(comp.componentType);
      bson.add("type", typeName ? typeName : "<unknown>");
      bson.add("size", componentTypes.getTypeInfo(comp.componentType).size);

      bson.end(); // i
      i.increment();
    }

    bson.end(); // components

    websocket::send_binary(bson);
  }

  static void add_query_data(BsonStream &bson, const ecs::BaseQueryDesc *desc)
  {
    const ecs::DataComponents &dataComponents = ecsViewerEntityManager->getDataComponents();
    const ecs::ComponentTypes &componentTypes = ecsViewerEntityManager->getComponentTypes();

    int16_t eidNo = -1;
    for (int16_t compNo = 0; compNo < desc->componentsRO.size(); ++compNo)
      if (desc->componentsRO[compNo].name == ECS_HASH("eid").hash)
      {
        eidNo = compNo;
        break;
      }

    Tab<ecs::ComponentDesc> componentsRO(desc->componentsRO, framemem_ptr());
    if (eidNo < 0)
    {
      eidNo = componentsRO.size();
      componentsRO.push_back() = {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()};
    }

    ecs::NamedQueryDesc queryDescWithEid{"temp_query", make_span_const(desc->componentsRW), make_span_const(componentsRO),
      make_span_const(desc->componentsRQ), make_span_const(desc->componentsNO)};

    const bool isEmptyQuery =
      componentsRO.size() == 1 && desc->componentsRW.empty() && desc->componentsRQ.empty() && desc->componentsNO.empty();

    eastl::vector<ecs::EntityId, framemem_allocator> eids;
    if (!isEmptyQuery)
    {
      ecs::QueryId queryDescWithEidId = ecsViewerEntityManager->createQuery(queryDescWithEid);
      ecs::perform_query(ecsViewerEntityManager, queryDescWithEidId, [&](const ecs::QueryView &qv) {
        for (auto iter = qv.begin(), iterE = qv.end(); iter != iterE; ++iter)
          eids.push_back(qv.getComponentRO<ecs::EntityId>(eidNo + qv.getRoStart(), iter));
      });
      ecsViewerEntityManager->destroyQuery(queryDescWithEidId);
    }

    bson.add("entitiesCount", (int)eids.size());
    bson_array(bson, "entities", eids, [&](int, const ecs::EntityId &v) -> uint32_t { return (uint32_t)v; });
    auto compDescType = [&](const ecs::ComponentDesc &v) -> const char * {
      const char *typeName = componentTypes.findTypeName(v.type);
      return typeName ? typeName : (v.type == ecs::ComponentTypeInfo<ecs::auto_type>::type ? "auto" : "<unknown>");
    };
    auto compDescName = [&](const ecs::ComponentDesc &v) -> const char * {
#if DAGOR_DBGLEVEL > 0
      if (v.nameStr)
        return v.nameStr;
#endif
      const char *n = dataComponents.findComponentName(v.name);
      return n ? n : "<unknown>";
    };

    bson_array_of_documents(bson, "componentsRW", desc->componentsRW, [&](int, const ecs::ComponentDesc &v) {
      bson.add("name", compDescName(v));
      bson.add("type", compDescType(v));
      bson.add("optional", !!(v.flags & ecs::CDF_OPTIONAL));
    });

    bson_array_of_documents(bson, "componentsRO", desc->componentsRO, [&](int, const ecs::ComponentDesc &v) {
      bson.add("name", compDescName(v));
      bson.add("type", compDescType(v));
      bson.add("optional", !!(v.flags & ecs::CDF_OPTIONAL));
    });

    bson_array_of_documents(bson, "componentsRQ", desc->componentsRQ, [&](int, const ecs::ComponentDesc &v) {
      bson.add("name", compDescName(v));
      bson.add("type", compDescType(v));
      bson.add("optional", !!(v.flags & ecs::CDF_OPTIONAL));
    });

    bson_array_of_documents(bson, "componentsNO", desc->componentsNO, [&](int, const ecs::ComponentDesc &v) {
      bson.add("name", compDescName(v));
      bson.add("type", compDescType(v));
      bson.add("optional", !!(v.flags & ecs::CDF_OPTIONAL));
    });
  }

  void getEvents(const DataBlock & /* blk */)
  {
    AutoFuncProfT<AFP_MSEC> _prof("[WEBUI] ECSMessageListener::getEvents %d msec");

    BsonStream bson = bson_ctor();

    const auto &eventsDb = ecsViewerEntityManager->getEventsDb();

    bson.beginArray("events");

    for (BsonStream::StringIndex i; i.idx() < eventsDb.getEventsCount(); i.increment())
    {
      bson.begin(i.str());

      bson.add("id", i.idx());
      bson.add("name", eventsDb.getEventName(i.idx()));
      bson.add("size", eventsDb.getEventSize(i.idx()));
      bson.add("isUnicast", (eventsDb.getEventFlags(i.idx()) & ecs::EVCAST_UNICAST) == ecs::EVCAST_UNICAST);
      bson.add("isBroadcast", (eventsDb.getEventFlags(i.idx()) & ecs::EVCAST_BROADCAST) == ecs::EVCAST_BROADCAST);

      bson.end(); // i
    }

    bson.end(); // events

    websocket::send_binary(bson);
  }

  void getQueries(const DataBlock & /* blk */)
  {
    AutoFuncProfT<AFP_MSEC> _prof("[WEBUI] ECSMessageListener::getQueries %d msec");

    BsonStream bson = bson_ctor();
    BsonStream::StringIndex i;

    bson.beginArray("queries");

    for (int queryNo = 0; queryNo < ecsViewerEntityManager->getQueriesCount(); ++queryNo)
    {
      ecs::QueryId q = ecsViewerEntityManager->getQuery(queryNo);
      if (!q)
        continue;
      const char *name = ecsViewerEntityManager->getQueryName(q);
      if (!name)
        continue;

      bson.begin(i.str());

      bson.add("id", i.idx());
      bson.add("name", name);

      ecs::BaseQueryDesc desc = ecsViewerEntityManager->getQueryDesc(q);
      add_query_data(bson, &desc);

      bson.end(); // i
      i.increment();
    }

    bson.end(); // queries

    websocket::send_binary(bson);
  }

  void getSystems(const DataBlock & /* blk */)
  {
    AutoFuncProfT<AFP_MSEC> _prof("[WEBUI] ECSMessageListener::getSystems %d msec");

    const auto &eventsDb = ecsViewerEntityManager->getEventsDb();
    auto systems = ecsViewerEntityManager->getSystems();

    BsonStream bson = bson_ctor();

    BsonStream::StringIndex i;

    bson.beginArray("systems");

    for (int sysNo = 0; sysNo < systems.size(); ++sysNo)
    {
      const ecs::EntitySystemDesc *sys = systems[sysNo];

      bson.begin(i.str());

      bson.add("id", i.idx());
      bson.add("name", sys->name);
      bson.add("tags", sys->getTags() ? sys->getTags() : "");

      bson.beginArray("events");
      BsonStream::StringIndex j;
      const auto &evSet = sys->getEvSet();
      for (const auto &ev : evSet)
      {
        const char *evName = eventsDb.findEventName(ev);
        bson.add(j.str(), evName ? evName : "<unknown>");
        j.increment();
      }
      bson.end(); // events

      add_query_data(bson, sys);

      bson.end(); // i
      i.increment();
    }

    bson.end(); // systems

    websocket::send_binary(bson);
  }

  void getTemplates(const DataBlock & /* blk */)
  {
    AutoFuncProfT<AFP_MSEC> _prof("[WEBUI] ECSMessageListener::getTemplates %d msec");

    const ecs::ComponentTypes &componentTypes = ecsViewerEntityManager->getComponentTypes();

    BsonStream bson = bson_ctor();

    BsonStream::StringIndex i;

    bson.beginArray("templates");

    const auto &db = ecsViewerEntityManager->getTemplateDB();
    for (const auto &templ : db)
    {
      bson.begin(i.str());

      bson.add("name", templ.getName());

      auto ancestry = templ.getParents();
      bson_array_of_documents(bson, "parents", ancestry, [&](int, uint32_t v) {
        bson.add("name", db.getTemplateById(v)->getName()); //-V522
      });

      bson.beginArray("components");
      BsonStream::StringIndex j;
      char compNameBuf[16];
      for (const auto &attrElem : templ.getComponentsMap())
      {
        const ecs::ChildComponent &comp = attrElem.second;

        bson.begin(j.str());

        const char *compName = db.getComponentName(attrElem.first);
        if (!compName)
        {
          snprintf(compNameBuf, sizeof(compNameBuf), "#%X", attrElem.first);
          compName = compNameBuf;
        }
        bson.add("name", compName);

        bool isShared = false;
        const char *typeName = componentTypes.findTypeName(comp.getUserType());
        if (is_shared_array(typeName))
        {
          isShared = true;
          bson.add("type", "ecs::Array");
        }
        else if (is_shared_object(typeName))
        {
          isShared = true;
          bson.add("type", "ecs::Object");
        }
        else
          bson.add("type", typeName ? typeName : "<unknown>");

        bson_ecs_component(bson, "value", comp.getEntityComponentRef());

        const uint32_t flags = templ.getRegExpInheritedFlags(attrElem.first, db.data());
        bson.add("isTracked", (flags & ecs::FLAG_CHANGE_EVENT) ? true : false);
        bson.add("isReplicated", (flags & ecs::FLAG_REPLICATED) ? true : false);
        bson.add("isShared", isShared);

        bson.end(); // j

        j.increment();
      }
      bson.end(); // components

      bson.end(); // i
      i.increment();
    }

    bson.end(); // templates

    websocket::send_binary(bson);
  }

  void getEntities(const DataBlock &blk)
  {
    AutoFuncProfT<AFP_MSEC> _prof("[WEBUI] ECSMessageListener::getEntities %d msec");

    const char *withComponent = blk.getStr("withComponent", nullptr);

    BsonStream bson = bson_ctor();

    if (withComponent)
      bson.beginArray(String(0, "entities_%s", withComponent).str());
    else
      bson.beginArray("entities");

    BsonStream::StringIndex i;

    Tab<ecs::ComponentDesc> comps_rq;
    if (withComponent)
    {
      const ecs::DataComponents &dataComponents = ecsViewerEntityManager->getDataComponents();
      for (int compNo = 0; compNo < dataComponents.size(); ++compNo)
      {
        const char *compName = dataComponents.getComponentNameById(compNo);
        if (compName && ::strcmp(withComponent, compName) == 0)
        {
          const ecs::ComponentTypes &componentTypes = ecsViewerEntityManager->getComponentTypes();
          ecs::DataComponent comp = dataComponents.getComponentById(compNo);
          const char *tpName = componentTypes.getTypeNameById(comp.componentType);
          comps_rq.push_back() = {ECS_HASH_SLOW(withComponent), ECS_HASH_SLOW(tpName).hash};
          break;
        }
      }
    }

    ecs::ComponentDesc eidComp{ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()};
    ecs::NamedQueryDesc desc{
      "all_entities_query",
      dag::ConstSpan<ecs::ComponentDesc>(),
      dag::ConstSpan<ecs::ComponentDesc>(&eidComp, 1),
      make_span_const(comps_rq),
      dag::ConstSpan<ecs::ComponentDesc>(),
    };
    ecs::QueryId qid = ecsViewerEntityManager->createQuery(desc);

    ecs::perform_query(ecsViewerEntityManager, qid, [&](const ecs::QueryView &qv) {
      for (auto it = qv.begin(), endIt = qv.end(); it != endIt; ++it)
      {
        bson.begin(i.str());

        ecs::EntityId eid = qv.getComponentRO<ecs::EntityId>(0, it);
        bson.add("eid", (uint32_t)eid);

        const char *templName = ecsViewerEntityManager->getEntityTemplateName(eid);
        bson.add("template", templName ? templName : "<unknown>");

        bson.end(); // i

        i.increment();
      }
      return ecs::QueryCbResult::Continue;
    });
    ecsViewerEntityManager->destroyQuery(qid);

    bson.end(); // entities

    websocket::send_binary(bson);
  }

  void getEntityAttributes(const DataBlock &blk)
  {
    AutoFuncProfT<AFP_MSEC> _prof("[WEBUI] ECSMessageListener::getEntityAttributes %d msec");

    const ecs::ComponentTypes &componentTypes = ecsViewerEntityManager->getComponentTypes();
    const ecs::DataComponents &dataComponents = ecsViewerEntityManager->getDataComponents();
    const ecs::TemplateDB &db = ecsViewerEntityManager->getTemplateDB();

    ecs::EntityId eid((ecs::entity_id_t)blk.getInt("eid"));

    BsonStream bson = bson_ctor();

    bson.add("eid", (uint32_t)eid);

    bson.beginArray("components");

    if (ecsViewerEntityManager->doesEntityExist(eid))
    {
      BsonStream::StringIndex j;
      for (auto aiter = ecsViewerEntityManager->getComponentsIterator(eid); aiter; ++aiter)
      {
        const auto &comp = (*aiter).second;

        ecs::component_flags_t flags = dataComponents.getComponentById(comp.getComponentId()).flags;
        if (flags & ecs::DataComponent::IS_COPY)
          continue;

        bson.begin(j.str());
        j.increment();

        bson.add("name", (*aiter).first);

        bool isShared = false;
        const char *typeName = componentTypes.findTypeName(comp.getUserType());
        if (is_shared_array(typeName))
        {
          isShared = true;
          bson.add("type", "ecs::Array");
        }
        else if (is_shared_object(typeName))
        {
          isShared = true;
          bson.add("type", "ecs::Object");
        }
        else
          bson.add("type", typeName ? typeName : "<unknown>");

        if (const char *templName = ecsViewerEntityManager->getEntityTemplateName(eid))
        {
          const ecs::Template *templ = db.getTemplateByName(templName);
          const uint32_t templFlags = templ->getRegExpInheritedFlags(ECS_HASH_SLOW((*aiter).first), db.data());
          bson.add("isTracked", !!(templFlags & ecs::FLAG_CHANGE_EVENT));
          bson.add("isReplicated", !!(templFlags & ecs::FLAG_REPLICATED));
          bson.add("isDontReplicate", !!(flags & ecs::DataComponent::DONT_REPLICATE));
          bson.add("isShared", isShared);
        }

        bson_ecs_component(bson, "value", comp);

        bson.end(); // j
      }
    }

    bson.end(); // components
    websocket::send_binary(bson);
  }

  void setEntityAttribute(const DataBlock &blk)
  {
    ecs::EntityId eid((ecs::entity_id_t)blk.getInt("eid"));

    SimpleString type(blk.getStr("type"));

#define SET_TYPE(TYPE, BLK_TYPE) \
  ecsViewerEntityManager->set<TYPE>(eid, ECS_HASH_SLOW(blk.getStr("comp")), (TYPE)blk.get##BLK_TYPE("value"))

    if (type == "float")
      SET_TYPE(float, Real);
    else if (type == "int")
      SET_TYPE(int, Int);
    else if (type == "bool")
      SET_TYPE(bool, Bool);
    else if (type == "SimpleString")
      ecsViewerEntityManager->set(eid, ECS_HASH_SLOW(blk.getStr("comp")), blk.getStr("value"));
    else if (type == "Point2")
      SET_TYPE(Point2, Point2);
    else if (type == "Point3")
      SET_TYPE(Point3, Point3);
    else if (type == "Point4")
      SET_TYPE(Point4, Point4);
    else if (type == "IPoint2")
      SET_TYPE(IPoint2, IPoint2);
    else if (type == "IPoint3")
      SET_TYPE(IPoint3, IPoint3);
    // else if (type == "IPoint4")
    //   SET_TYPE(IPoint4, IPoint4);
    else if (type == "E3DCOLOR")
      SET_TYPE(E3DCOLOR, E3dcolor);
    else if (type == "ecs::EntityId")
      SET_TYPE(ecs::EntityId, Int);
    else if (type == "TMatrix")
      SET_TYPE(TMatrix, Tm);
    else if (type == "ecs::Object")
    {
      ecs::ComponentsList alist;
      ecs::load_comp_list_from_blk(*blk.getBlockByNameEx("value:object", blk.getBlockByNameEx("value")), alist);
      if (!alist.empty())
        ecsViewerEntityManager->set(eid, ECS_HASH_SLOW(blk.getStr("comp")), eastl::move(alist[0].second));
    }
    else if (type == "ecs::Array")
    {
      ecs::ComponentsList alist;
      ecs::load_comp_list_from_blk(*blk.getBlockByNameEx("value:object", blk.getBlockByNameEx("value")), alist);
      if (!alist.empty())
        ecsViewerEntityManager->set(eid, ECS_HASH_SLOW(blk.getStr("comp")), eastl::move(alist[0].second));
    }
  }

  static void read_component(const char *name, Tab<ecs::ComponentDesc> &out_comps)
  {
    const ecs::DataComponents &dataComponents = ecsViewerEntityManager->getDataComponents();
    const ecs::ComponentTypes &componentTypes = ecsViewerEntityManager->getComponentTypes();
    auto nameHash = ECS_HASH_SLOW(name);
    auto compNo = dataComponents.findComponentId(nameHash.hash);
    if (compNo != ecs::INVALID_COMPONENT_INDEX)
    {
      ecs::DataComponent comp = dataComponents.getComponentById(compNo);
      if (const char *typeName = componentTypes.getTypeNameById(comp.componentType))
        out_comps.push_back() = {nameHash, ECS_HASH_SLOW(typeName).hash};
    }
  }

  void performDynamicQuery(const DataBlock &blk)
  {
    BsonStream bson = bson_ctor();

    bson.beginArray("dynamicQueryEntities");

    Tab<ecs::ComponentDesc> comps_ro;
    Tab<ecs::ComponentDesc> comps_rq;
    Tab<ecs::ComponentDesc> comps_no;

    comps_ro.push_back() = {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()};

    const int roNid = blk.getNameId("comps_ro");
    const int rqNid = blk.getNameId("comps_rq");
    const int noNid = blk.getNameId("comps_no");

    for (int i = 0; i < blk.paramCount(); ++i)
    {
      const int nid = blk.getParamNameId(i);
      if (nid == roNid)
        read_component(blk.getStr(i), comps_ro);
      else if (nid == rqNid)
        read_component(blk.getStr(i), comps_rq);
      else if (nid == noNid)
        read_component(blk.getStr(i), comps_no);
    }

    if (comps_ro.size() == 1 && comps_rq.empty() && comps_no.empty())
    {
      bson.end(); // dynamicQueryEntities
      websocket::send_binary(bson);
      return;
    }

    ecs::NamedQueryDesc desc{
      "dynamic_query",
      dag::ConstSpan<ecs::ComponentDesc>(),
      make_span_const(comps_ro),
      make_span_const(comps_rq),
      make_span_const(comps_no),
    };

    BsonStream::StringIndex i;

    struct TmpState
    {
      BsonStream::StringIndex &i;
      BsonStream &bson;
      const Tab<ecs::ComponentDesc> &compsRO;
      bool hasFilter;
      ExpressionTree &filterExpr;
      TmpState() = delete;
    };

    const char *filter = blk.getStr("filter", nullptr);

    ExpressionTree filterExpr;

    bool hasFilter = false;
    if (filter && *filter)
      hasFilter =
        parse_query_filter(filterExpr, filter, empty_span(), make_span((const ecs::ComponentDesc *)comps_ro.data(), comps_ro.size()));

    TmpState state{i, bson, comps_ro, hasFilter, filterExpr};

    auto qid = ecsViewerEntityManager->createQuery(desc);
    ecs::perform_query(ecsViewerEntityManager, qid, [&state](const ecs::QueryView &qv) {
      auto &i = state.i;
      auto &bson = state.bson;
      const auto &comps_ro = state.compsRO;

      PerformExpressionTree tree(nullptr, 0, nullptr);
      if (state.hasFilter && state.filterExpr.nodes.size())
      {
        tree.nodes = state.filterExpr.nodes.data();
        tree.nodesCount = state.filterExpr.nodes.size();
        tree.qv = &qv;
      }

      const ecs::DataComponents &dataComponents = ecsViewerEntityManager->getDataComponents();
      const ecs::ComponentTypes &componentTypes = ecsViewerEntityManager->getComponentTypes();

      for (auto it = qv.begin(), endIt = qv.end(); it != endIt; ++it)
      {
        if (tree.nodes && !tree.performExpression(it))
          continue;

        bson.begin(i.str());

        ecs::EntityId eid = qv.getComponentRO<ecs::EntityId>(0, it);
        bson.add("eid", (uint32_t)eid);

        const char *templName = ecsViewerEntityManager->getEntityTemplateName(eid);
        bson.add("template", templName ? templName : "<unknown>");

        bson.beginArray("components");
        BsonStream::StringIndex j;
        for (int k = 1; k < comps_ro.size(); ++k, j.increment())
        {
          bson.begin(j.str());
          const char *compName = dataComponents.findComponentName(comps_ro[k].name);
          bson.add("name", compName);
          for (auto aiter = ecsViewerEntityManager->getComponentsIterator(eid); aiter; ++aiter)
          {
            const auto &comp = (*aiter).second;
            if (::strcmp(compName, (*aiter).first) == 0) //-V575
            {
              bson.add("type", componentTypes.findTypeName(comp.getUserType()));
              bson_ecs_component(bson, "value", comp);
              break;
            }
          }
          bson.end(); // j
        }
        bson.end(); // components

        bson.end(); // i
        i.increment();
      }
      return ecs::QueryCbResult::Continue;
    });
    ecsViewerEntityManager->destroyQuery(qid);

    bson.end(); // dynamicQueryEntities

    websocket::send_binary(bson);
  }
};

static InitOnDemand<ECSMessageListener> listener;
ecs::EntityManager *ecsViewerEntityManager = nullptr;

void webui::init_ecsviewer_plugin()
{
  if (listener)
    return;

  listener.demandInit();
  websocket::add_message_listener(listener);
  set_ecsviewer_entity_manager(g_entity_mgr);
}
void webui::set_ecsviewer_entity_manager(ecs::EntityManager *manager) { ecsViewerEntityManager = manager; }
