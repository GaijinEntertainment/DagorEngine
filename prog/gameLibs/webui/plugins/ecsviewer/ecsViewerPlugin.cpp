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
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <util/dag_base64.h>
#include <util/dag_watchdog.h>
#include <util/dag_simpleString.h>
#include <webui/httpserver.h>
#include <webui/helpers.h>

#include <daECS/core/entityComponent.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/template.h>
#include <daECS/core/component.h>
#include <daECS/core/internal/performQuery.h>
#include <ska_hash_map/flat_hash_map2.hpp>

using namespace webui;
using namespace ecs;
extern ecs::EntityManager *ecsViewerEntityManager;

#define NOT_ATTRIB_NAME "auto"

static bool plugin_inited = false;
static String prev_state;
static String templates_cache;

static void init_ecs_plugin()
{
  plugin_inited = true;
  clear_and_shrink(prev_state);
  clear_and_shrink(templates_cache);
}

static void close_ecs_plugin()
{
  plugin_inited = false;
  clear_and_shrink(prev_state);
  clear_and_shrink(templates_cache);
}

static String &collect_systems(bool &changed)
{
  String s(tmpmem);
  s.reserve(1 << 20);

  const ecs::ComponentTypes &componentTypes = ecsViewerEntityManager->getComponentTypes();
  const ecs::DataComponents &dataComponents = ecsViewerEntityManager->getDataComponents();

  //  sys = [
  //    { name: "main_es", compRW:[ {name:"health", mandatory:1, attribute:1}, {name:"mana", mandatory:0, attribute:1} ], compRO:[],
  //    events:[], used:1, id:1 },
  //  ];

  s += "{\"systems\":[";
  String compRW(tmpmem), compRO(tmpmem);

  auto process_system = [&](const EntitySystemDesc *sysDesc, int id, bool used) {
    if (id != 0)
      s += ",";

    compRW.clear();
    compRO.clear();

    for (int r = 0; r < 2; r++)
    {
      String &res = r ? compRW : compRO;
      for (int j = 0; j < (r ? sysDesc->componentsRW.size() : sysDesc->componentsRO.size()); j++)
      {
        const ComponentDesc &compDesc = r ? sysDesc->componentsRW[j] : sysDesc->componentsRO[j];
        if (j > 0)
          res += ",";

        const char *n = componentTypes.findTypeName(compDesc.type);
        const char *compNameStr = dataComponents.findComponentName(compDesc.name);
#if DAGOR_DBGLEVEL > 0
        if (!compNameStr)
        {
          // FIXME: component desc strings are broken (points to some dangling memory), this code tries at least mitigate that
          compNameStr = compDesc.nameStr;
          for (const char *c = compNameStr; *c; ++c)
            if (!::isprint(*c) || *c == '"' || *c == '\\' || (c - compNameStr) > 256)
            {
              compNameStr = nullptr;
              break;
            }
        }
#endif
        char tmpBuf[32];
        if (!compNameStr)
        {
          SNPRINTF(tmpBuf, sizeof(tmpBuf), "0x%x", compDesc.name);
          compNameStr = tmpBuf;
        }

        res.aprintf(128, "{\"name\":\"%s\", \"optional\":%d, \"attrType\":\"%s\" }", compNameStr,
          int(!!(compDesc.flags & CDF_OPTIONAL)), (n ? n : NOT_ATTRIB_NAME));
      }
    }

    int entitiesCount = id >= 0 ? ecsViewerEntityManager->getEntitySystemSize(id) : -1;
    s.aprintf(1024, "{\"name\":\"%s\", \"used\":%d, \"entities\":%d, \"id\":%d, \"compRW\":[%s], \"compRO\":[%s]}", sysDesc->name,
      int(used ? 1 : 0), entitiesCount, id, compRW.str(), compRO.str());
  };

  auto systems = ecsViewerEntityManager->getSystems();

  ska::flat_hash_set<const EntitySystemDesc *> usedSystems;
  for (int id = 0; id < systems.size(); id++)
  {
    process_system(systems[id], id, true);
    usedSystems.insert(systems[id]);
  }
  ecs::iterate_systems([&](const EntitySystemDesc *sys) {
    if (usedSystems.find(sys) == usedSystems.end())
    {
      process_system(sys, -1, false);
    }
  });

  s += "], ";


  if (templates_cache.empty())
  {
    templates_cache.reserve(150000);
    String &c = templates_cache;
    c += "\"templates\":[";

    const TemplateDB &templates = ecsViewerEntityManager->getTemplateDB();

    bool tcomma = false;
    for (auto &temp : templates)
    {
      if (tcomma)
        c += ",";
      tcomma = true;

      c += "{";

      auto ancestry = temp.getParents();

      c.aprintf(128, "\"name\":\"%s\", \"ancestry\":[", temp.getName());
      for (int i = 0; i < ancestry.size(); i++)
        c.aprintf(128, "%s\"%s\"", i == 0 ? "" : ",", templates.getTemplateById(ancestry[i])->getName()); //-V522
      c += "], \"components\":[";

      bool acomma = false;
      char compNameBuf[16];
      for (auto citer = temp.getComponentsMap().begin(); citer != temp.getComponentsMap().end(); ++citer)
      {
        auto attrElem = *citer;
        const ChildComponent &attr = attrElem.second;

        const uint32_t flags = temp.getRegExpInheritedFlags(attrElem.first, templates.data());

        String flagsStr;
        if ((flags & FLAG_CHANGE_EVENT) != 0)
          flagsStr += "\"ch_event\":1, ";
        if ((flags & FLAG_REPLICATED) != 0)
          flagsStr += "\"replicated\":1, ";
        // if ((attr.getFlags() & Attribute::FLAG_HIDDEN) != 0)
        //   flagsStr += "\"hidden\":1, ";

        if (flagsStr.length() > 2)
          flagsStr[flagsStr.length() - 2] = '\0';

        ecs::component_type_t tp = attr.getUserType();
        if (!tp)
          tp = templates.getComponentType(attrElem.first);
        if (!tp)
          tp = ecsViewerEntityManager->getDataComponents().findComponent(attrElem.first).componentTypeName;
        const char *typeName = componentTypes.findTypeName(tp);
        const char *compName = templates.getComponentName(attrElem.first);
        if (!compName)
        {
          snprintf(compNameBuf, sizeof(compNameBuf), "#%X", attrElem.first);
          compName = compNameBuf;
        }

        c.aprintf(128, "%s{\"name\":\"%s\", \"optional\":%d, \"attrType\":\"%s\"%s %s }", acomma ? "," : "", compName,
          0 /* int(!!(attr.getFlags() & CDF_OPTIONAL)) */, (typeName ? typeName : NOT_ATTRIB_NAME), flagsStr.empty() ? "" : ",",
          flagsStr.str());

        acomma = true;
      }
      c += "]";

      c += "}";
    }
    c += "]";
  }

  s += templates_cache;
  s += "}";


  if (!strcmp(s, prev_state.str()))
  {
    changed = false;
  }
  else
  {
    changed = true;
    prev_state = s;
  }

  return prev_state;
}


void on_ecsviewer(RequestInfo *params)
{
  if (!plugin_inited)
  {
    // html_response_raw(params->conn, "");
    init_ecs_plugin();
  }

  if (!params->params)
  {
    close_ecs_plugin();
    const char *inlinedEcsViewer =
#include "ecsViewer.html.inl"
      ;
    html_response_raw(params->conn, inlinedEcsViewer);
  }
  else if (!strcmp(params->params[0], "attach"))
  {
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "alldata"))
  {
    if (!ecsViewerEntityManager)
      return html_response_raw(params->conn, "");

    String s(tmpmem);
    s.reserve(20000);

    bool changed = false;
    String systems = collect_systems(changed);

    if (changed)
      s.aprintf(128, "%s", systems.str());

    html_response_raw(params->conn, s.str());
  }
  else if (!strcmp(params->params[0], "set_used"))
  {
    // int idx = atoi(params->params[2]);
    // int value = atoi(params->params[4]);
    // ecsViewerEntityManager->setUsed(idx, value);
    if (ecsViewerEntityManager)
      ecsViewerEntityManager->enableES(params->params[2], atoi(params->params[4]));
    html_response_raw(params->conn, "");
  }
  else if (!strcmp(params->params[0], "state"))
  {
    html_response_raw(params->conn, "");
  }
  // else if (!strcmp(params->params[0], "detach")) {}
}

webui::HttpPlugin webui::ecsviewer_http_plugin = {"ecsviewer", "show ECS viewer", NULL, on_ecsviewer};
