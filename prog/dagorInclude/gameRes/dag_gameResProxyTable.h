//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_resourceNameResolve.h>

struct GameResProxyTable
{
  static constexpr unsigned GRPT_API_VERSION = 1;
  unsigned apiVersion = 0;

  GameResourceFactory *(*find_gameres_factory)(unsigned classid) = nullptr;
  void (*free_unused_game_resources)(gameres_rrl_cptr_t) = nullptr;
  void (*set_gameres_undefined_res_loglevel)(int8_t level) = nullptr;
  WinCritSec &(*get_gameres_main_cs)() = nullptr;
  bool (*resolve_game_resource_name1)(String &, const DynamicRenderableSceneLodsResource *) = nullptr;
  bool (*resolve_game_resource_name2)(String &, const RenderableInstanceLodsResource *) = nullptr;

  unsigned (*get_resource_type_id)(char const *) = nullptr;
  void (*get_game_resource_name)(int, String &) = nullptr;
  bool (*is_game_resource_loaded)(const char *, unsigned) = nullptr;
  bool (*is_game_resource_loaded_nolock)(const char *, unsigned) = nullptr;
  bool (*is_ignoring_unavailable_resources)() = nullptr;

  GameResource *(*get_game_resource)(int, gameres_rrl_cptr_t) = nullptr;
  GameResource *(*get_game_resource_ex)(const char *, unsigned, gameres_rrl_cptr_t) = nullptr;
  GameResource *(*get_one_game_resource_ex)(const char *, unsigned) = nullptr;
  D3DRESID (*get_tex_gameres)(char const *, bool) = nullptr;
  void (*game_resource_add_ref_ex)(const void *, unsigned) = nullptr;
  void (*release_game_resource_ex)(const void *, unsigned) = nullptr;
  void (*release_game_resource)(int) = nullptr;

  gameres_rrl_ptr_t (*create_res_restriction_list)(char const *, unsigned int, gameres_rrl_ptr_t) = nullptr;
  int (*extend_res_restriction_list)(gameres_rrl_ptr_t, char const *) = nullptr;
  bool (*preload_all_required_res)(gameres_rrl_cptr_t) = nullptr;
  void (*free_res_restriction_list)(gameres_rrl_ptr_t &) = nullptr;
  bool (*is_res_required)(gameres_rrl_cptr_t, int) = nullptr;
};

static inline void fill_gameres_proxy_table(GameResProxyTable &ptable)
{
  memset(&ptable, 0, sizeof(ptable));
  ptable.apiVersion = ptable.GRPT_API_VERSION;
#define FILL_PROXY_AS(NM, NM_AS) ptable.NM_AS = &NM
#define FILL_PROXY(NM)           FILL_PROXY_AS(::NM, NM)
  FILL_PROXY(find_gameres_factory);
  FILL_PROXY(free_unused_game_resources);
  FILL_PROXY(set_gameres_undefined_res_loglevel);
  FILL_PROXY(get_gameres_main_cs);
  FILL_PROXY_AS(resolve_game_resource_name, resolve_game_resource_name1);
  FILL_PROXY_AS(resolve_game_resource_name, resolve_game_resource_name2);

  FILL_PROXY(get_resource_type_id);
  FILL_PROXY(get_game_resource_name);
  FILL_PROXY(is_game_resource_loaded);
  FILL_PROXY(is_game_resource_loaded_nolock);
  FILL_PROXY(is_ignoring_unavailable_resources);

  FILL_PROXY_AS(GameResourceFactory::get_game_resource, get_game_resource);
  FILL_PROXY(get_game_resource_ex);
  FILL_PROXY(get_one_game_resource_ex);
  FILL_PROXY(get_tex_gameres);
  FILL_PROXY(game_resource_add_ref_ex);
  FILL_PROXY(release_game_resource_ex);
  FILL_PROXY_AS(GameResourceFactory::release_game_resource_nolock, release_game_resource);

  FILL_PROXY(create_res_restriction_list);
  FILL_PROXY(extend_res_restriction_list);
  FILL_PROXY(preload_all_required_res);
  FILL_PROXY(free_res_restriction_list);
  FILL_PROXY(is_res_required);
#undef FILL_PROXY_AS
#undef FILL_PROXY
}
