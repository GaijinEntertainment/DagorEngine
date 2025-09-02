// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../av_plugin.h"
#include "../av_appwnd.h"
#include <assets/asset.h>
#include <de3_interface.h>
#include <debug/dag_debug.h>

#include <propPanel/control/container.h>
#include <propPanel/c_control_event_handler.h>
#include <daECS/core/entityManager.h>
#include <daECS/io/blk.h>
#include <ecsPropPanel/ecsEditableObjectPropertyPanel.h>
#include <ecsPropPanel/ecsBasicObjectEditor.h>

static inline ecs::ComponentsInitializer complist_to_compinitializer(ecs::ComponentsList &&clist)
{
  ecs::ComponentsMap cmap;
  for (auto &&a : clist)
    cmap[ECS_HASH_SLOW(a.first.c_str())] = eastl::move(a.second);
  return ecs::ComponentsInitializer(eastl::move(cmap));
}

static inline void add_component(ecs::ComponentsList &clist, const ecs::EntityComponentRef &component, const char *cname)
{
  ecs::component_type_t ctype = component.getUserType();
#define ADD_COMP_TYPE(T)                                                             \
  if (ctype == ecs::ComponentTypeInfo<T>::type)                                      \
  {                                                                                  \
    clist.emplace_back(cname, eastl::move(ecs::ChildComponent(component.get<T>()))); \
    return;                                                                          \
  }
  ADD_COMP_TYPE(bool);
  ADD_COMP_TYPE(int);
  ADD_COMP_TYPE(float);
  ADD_COMP_TYPE(Point2);
  ADD_COMP_TYPE(Point3);
  ADD_COMP_TYPE(Point4);
  ADD_COMP_TYPE(E3DCOLOR);
  ADD_COMP_TYPE(ecs::string);
  ADD_COMP_TYPE(ecs::EntityId);
  if (strcmp(cname, "transform") != 0)
    ADD_COMP_TYPE(TMatrix);
#undef ADD_COMP_TYPE
}

static inline void add_component_to_blk(DataBlock &blk, const ecs::EntityComponentRef &component, const char *cname)
{
  ecs::component_type_t ctype = component.getUserType();
#define ADD_COMP_TYPE(T, METHOD)                \
  if (ctype == ecs::ComponentTypeInfo<T>::type) \
  {                                             \
    blk.METHOD(cname, component.get<T>());      \
    return;                                     \
  }
#define ADD_COMP_TYPE_EX(T, METHOD, CAST)       \
  if (ctype == ecs::ComponentTypeInfo<T>::type) \
  {                                             \
    blk.METHOD(cname, component.get<T>().CAST); \
    return;                                     \
  }
  ADD_COMP_TYPE(bool, setBool);
  ADD_COMP_TYPE(int, setInt);
  ADD_COMP_TYPE(float, setReal);
  ADD_COMP_TYPE(Point2, setPoint2);
  ADD_COMP_TYPE(Point3, setPoint3);
  ADD_COMP_TYPE(Point4, setPoint4);
  ADD_COMP_TYPE(E3DCOLOR, setE3dcolor);
  ADD_COMP_TYPE_EX(ecs::string, setStr, c_str());
  ADD_COMP_TYPE(TMatrix, setTm);
#undef ADD_COMP_TYPE
#undef ADD_COMP_TYPE_EX
}

template <class F, class E>
static inline void execute_safe(F func, E catch_func)
{
  DAEDITOR3.setFatalHandler(false);
  try
  {
    func();
  }
  catch (...)
  {
    catch_func();
  }
  DAEDITOR3.popFatalHandler();
}

class EcsTemplatePreviewPlugin : public IGenEditorPlugin, public PropPanel::ControlEventHandler
{
public:
  EcsTemplatePreviewPlugin() {}
  ~EcsTemplatePreviewPlugin() override { g_entity_mgr->destroyEntity(entity); }

  const char *getInternalName() const override { return "EcsTemplatePreviewer"; }

  void registered() override {}
  void unregistered() override {}

  bool begin(DagorAsset *asset) override
  {
    templateName = asset->props.getStr("template", asset->getName());
    ecs::ComponentsList clist;
    clist.emplace_back("transform", eastl::move(ecs::ChildComponent(getInitialTm())));
    if (const DataBlock *b = asset->props.getBlockByName("components"))
      ecs::load_comp_list_from_blk(*g_entity_mgr, *b, clist);

    execute_safe( //
      [&]() {
        entity = g_entity_mgr->createEntityAsync(templateName, complist_to_compinitializer(eastl::move(clist)));
        g_entity_mgr->tick(true);
      },
      [&]() {
        logerr("exception while creating entity using template: %s", templateName);
        entity = ecs::INVALID_ENTITY_ID;
      });
    fillPluginPanel();
    return true;
  }
  bool end() override
  {
    if (spEditor)
      spEditor->destroyPanel();
    g_entity_mgr->destroyEntity(entity);
    return true;
  }

  void clearObjects() override {}
  void onSaveLibrary() override {}
  void onLoadLibrary() override {}

  bool getSelectionBox(BBox3 &box) const override
  {
    if (entity == ecs::INVALID_ENTITY_ID)
      return false;
    TMatrix tm = ECS_GET_OR(entity, transform, TMatrix::IDENT);
    BBox3 localBbox(Point3(-0.5f, -0.5f, -0.5f), Point3(+0.5f, +0.5f, +0.5f));
    localBbox[0] = ECS_GET_OR(entity, ri_extra__bboxMin, localBbox[0] * 0.5f) * 2.f;
    localBbox[1] = ECS_GET_OR(entity, ri_extra__bboxMax, localBbox[1] * 0.5f) * 2.f;
    box = tm * localBbox;
    return true;
  }

  void actObjects(float dt) override {}
  void beforeRenderObjects() override {}
  void renderObjects() override {}
  void renderTransObjects() override {}

  bool supportAssetType(const DagorAsset &asset) const override { return strcmp(asset.getTypeStr(), "ecs_template") == 0; }

  void fillPropPanel(PropPanel::ContainerPropertyControl &panel) override
  {
    panel.setEventHandler(this);
    panel.createButton(PID_RECREATE_ENTITY, "Re-create entity");
    panel.createButton(PID_SAVE_ENTITY_ASSET, "Save as ECS entity asset...");
    panel.createSeparator();
    ECSEditableObjectPropertyPanel editableObjectPropertyPanel(panel);
    editableObjectPropertyPanel.fillProps(entity, &objEd);
  }
  void postFillPropPanel() override {}

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    ECSEditableObjectPropertyPanel editableObjectPropertyPanel(*panel);
    editableObjectPropertyPanel.onPPChange(pcb_id, /*edit_finished*/ true, entity, &objEd);
  }
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    if (pcb_id == PID_RECREATE_ENTITY)
    {
      if (entity == ecs::INVALID_ENTITY_ID)
        return;
      String cur_templ(g_entity_mgr->getEntityFutureTemplateName(entity));
      ecs::ComponentsList clist;
      clist.emplace_back("transform", eastl::move(ecs::ChildComponent(getInitialTm())));
      for (ecs::ComponentsIterator it = g_entity_mgr->getComponentsIterator(entity); it; ++it)
        add_component(clist, (*it).second, (*it).first);

      g_entity_mgr->destroyEntity(entity);
      g_entity_mgr->tick(true);

      execute_safe( //
        [&]() {
          entity = g_entity_mgr->createEntityAsync(cur_templ, complist_to_compinitializer(eastl::move(clist)));
          g_entity_mgr->tick(true);
        },
        [&]() {
          logerr("exception while creating entity using template: %s", cur_templ);
          entity = ecs::INVALID_ENTITY_ID;
        });

      fillPluginPanel();
      return;
    }
    else if (pcb_id == PID_SAVE_ENTITY_ASSET)
    {
      if (entity == ecs::INVALID_ENTITY_ID)
        return;
      String fn(0, "%s/assets/%s_1.ecs_template.blk", ::get_app().getWorkspace().getSdkDir(), templateName);
      dd_simplify_fname_c(fn);
      fn = wingw::file_save_dlg(EDITORCORE->getWndManager()->getMainWindow(), "Export preconfigured ECS entity...",
        "ECS entity templates|*.ecs_template.blk|All files|*.*", "ecs_template.blk", ::get_app().getWorkspace().getSdkDir(), fn);
      if (!fn.empty())
      {
        DataBlock blk;
        blk.setStr("template", g_entity_mgr->getEntityFutureTemplateName(entity));
        DataBlock &cblk = *blk.addBlock("components");
        for (ecs::ComponentsIterator it = g_entity_mgr->getComponentsIterator(entity); it; ++it)
          add_component_to_blk(cblk, (*it).second, (*it).first);
        dblk::save_to_text_file(blk, fn);
      }
      return;
    }

    String cur_templ(g_entity_mgr->getEntityTemplateName(entity));
    ECSEditableObjectPropertyPanel editableObjectPropertyPanel(*panel);
    execute_safe(
      [&]() {
        editableObjectPropertyPanel.onPPBtnPressed(pcb_id, entity, &objEd);
        g_entity_mgr->tick(true);
      },
      []() {});
    String new_templ(g_entity_mgr->getEntityFutureTemplateName(entity));
    if (strcmp(cur_templ, new_templ) != 0)
      fillPluginPanel();
  }

  enum
  {
    PID_RECREATE_ENTITY = 10000,
    PID_SAVE_ENTITY_ASSET,
  };

private:
  ecs::EntityId entity = ecs::INVALID_ENTITY_ID;
  ECSBasicObjectEditor objEd;
  String templateName;

  TMatrix getInitialTm() const
  {
    TMatrix tm = TMatrix::IDENT;
    tm.setcol(3, Point3(0, 0.1, 0)); // to prevent assert in FX on IDENT matrix
    return tm;
  }
};

static InitOnDemand<EcsTemplatePreviewPlugin> plugin;


void init_plugin_ecs_templates()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();
  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
