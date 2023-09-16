#include <rendInst/rendInstGen.h>
#include <shaders/dag_dynSceneRes.h>
#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_gameResHooks.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_grpMgr.h>
#include <3d/dag_drv3d.h>
#include <startup/dag_startupTex.h>
#include <util/dag_texMetaData.h>
#include <util/dag_oaHashNameMap.h>
#include <debug/dag_debug.h>
#include <stdio.h>

namespace gameresprivate
{
extern int curRelFnOfs;

void scanGameResPack(const char *filename);
void scanDdsxTexPack(const char *filename);
} // namespace gameresprivate

static FastNameMapEx refTexAll;
static FastNameMapEx refResAll;

static bool on_get_game_resource(int res_id, dag::Span<GameResourceFactory *> f, GameResource *&out_res)
{
  if (res_id >= 0)
  {
    String name;
    get_game_resource_name(res_id, name);
    refResAll.addNameId(name);
  }
  return false;
}
static bool on_load_game_resource_pack(int res_id, dag::Span<GameResourceFactory *> f) { return false; }

static void acquire_dynmodel_tex_refs(const char *name, Tab<DynamicRenderableSceneLodsResource *> &stor)
{
  if (get_resource_type_id(name) == DynModelGameResClassId)
  {
    auto *res = (DynamicRenderableSceneLodsResource *)::get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(name), DynModelGameResClassId);
    if (res)
    {
      res->addInstanceRef();
      stor.push_back(res);
    }
  }
}
static void release_dynmodel_tex_refs(Tab<DynamicRenderableSceneLodsResource *> &stor)
{
  for (auto *res : stor)
  {
    res->delInstanceRef();
    release_game_resource((GameResource *)res);
  }
  stor.clear();
}

bool dump_grp_tex_refs(const char *grp_fn, bool detailed)
{
  enable_tex_mgr_mt(true, 64 << 10);
  set_default_tex_factory(get_stub_tex_factory());
  /*
    ::register_geom_node_tree_gameres_factory();
    ::register_character_gameres_factory();
    ::register_fast_phys_gameres_factory();
    ::register_phys_sys_gameres_factory();
    ::register_phys_obj_gameres_factory();
    ::register_ragdoll_gameres_factory();
    ::register_animchar_gameres_factory();
    ::register_a2d_gameres_factory();
  */
  register_material_gameres_factory();
  rendinst::register_land_gameres_factory();
  CollisionResource::registerFactory();

  gameresprivate::scanGameResPack(grp_fn);

  gamereshooks::on_get_game_resource = &on_get_game_resource;
  gamereshooks::on_load_game_resource_pack = &on_load_game_resource_pack;

  Tab<String> names;
  get_loaded_game_resource_names(names);
  printf("=== %s: %d resources\n%s", grp_fn, names.size(), detailed ? "\n" : "");

  Tab<DynamicRenderableSceneLodsResource *> dm;
  if (detailed)
  {
    for (int ri = 0; ri < names.size(); ri++)
    {
      switch (get_resource_type_id(names[ri]))
      {
        case DynModelGameResClassId:
        case RendInstGameResClassId:
        case EffectGameResClassId:
        case MaterialGameResClassId:
        case rendinst::HUID_LandClassGameRes:
          FastNameMap res_list;
          res_list.addNameId(names[ri]);
          set_required_res_list_restriction(res_list);
          preload_all_required_res();
          acquire_dynmodel_tex_refs(names[ri], dm);

          for (TEXTUREID i = first_managed_texture(1); i != BAD_TEXTUREID; i = next_managed_texture(i, 1))
            refTexAll.addNameId(TextureMetaData::decodeFileName(get_managed_texture_name(i)));

          printf("[%3d] \"%s\"   %08X\n", ri, names[ri].str(), get_resource_type_id(names[ri]));
          printf("  referenced %d DDSx:\n", refTexAll.nameCount());
          for (int i = 0; i < refTexAll.nameCount(); i++)
            printf("    %s\n", refTexAll.getName(i));
          if (refResAll.nameCount() > 1)
          {
            printf("  referenced %d resources:\n", refResAll.nameCount() - 1);
            for (int i = 0; i < refResAll.nameCount(); i++)
              if (strcmp(refResAll.getName(i), names[ri]) != 0)
                printf("    %s\n", refResAll.getName(i));
          }
          printf("\n");

          reset_required_res_list_restriction();
          release_dynmodel_tex_refs(dm);
          free_unused_game_resources();
          refTexAll.reset();
          refResAll.reset();
          break;
      }
    }
  }
  else
  {
    FastNameMap res_list;
    for (int ri = 0; ri < names.size(); ri++)
      switch (get_resource_type_id(names[ri]))
      {
        case DynModelGameResClassId:
        case RendInstGameResClassId:
        case EffectGameResClassId:
        case MaterialGameResClassId:
        case rendinst::HUID_LandClassGameRes: res_list.addNameId(names[ri]); break;
      }

    set_required_res_list_restriction(res_list);
    preload_all_required_res();

    for (int ri = 0; ri < names.size(); ri++)
      acquire_dynmodel_tex_refs(names[ri], dm);

    for (TEXTUREID i = first_managed_texture(1); i != BAD_TEXTUREID; i = next_managed_texture(i, 1))
      refTexAll.addNameId(TextureMetaData::decodeFileName(get_managed_texture_name(i)));

    printf("\nrelevant resources: %d\n", refResAll.nameCount());
    for (int i = 0; i < refResAll.nameCount(); i++)
      printf("  %s\n", refResAll.getName(i));
    printf("\nreferenced %d DDSx:\n", refTexAll.nameCount());
    for (int i = 0; i < refTexAll.nameCount(); i++)
      printf("  %s\n", refTexAll.getName(i));

    reset_required_res_list_restriction();
    release_dynmodel_tex_refs(dm);
    free_unused_game_resources();
    refTexAll.reset();
    refResAll.reset();
  }

  gamereshooks::on_get_game_resource = NULL;
  gamereshooks::on_load_game_resource_pack = NULL;
  return true;
}
