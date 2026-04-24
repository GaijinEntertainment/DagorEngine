//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabFwd.h>
#include <gameRes/dag_rrl.h>

class GameResource;
class GameResourceFactory;
class String;


//! gameres hooks are used to customize resource creation/destruction and for logging;
//! if hook returns false, default implementation is used
namespace gamereshooks
{
//! called to resolve res_name:class_id to unique res_id;
extern bool (*resolve_res_handle)(const char *res_name, unsigned class_id, int &out_res_id);

//! called to get references needed to create res_id;
extern bool (*get_res_refs)(int res_id, Tab<int> &out_refs);


//! called when res_id validation is needed;
extern bool (*on_validate_game_res_id)(int res_id, int &out_res_id);

//! called when resource class id is needed;
extern bool (*on_get_game_res_class_id)(int res_id, unsigned &out_class_id);

//! called from preload_all_required_res() to pre-process/validate RRL, return true when updated RRL still not empty;
extern bool (*on_preload_all_required_res)(gameres_rrl_ptr_t rrl);

//! called from get_game_resource() and get_game_resource_ex();
extern bool (*on_get_game_resource)(int res_id, gameres_rrl_cptr_t rrl, dag::Span<GameResourceFactory *> f, GameResource *&out_res);

//! called from release_game_resource_nolock();
extern bool (*on_release_game_resource)(int res_id, dag::Span<GameResourceFactory *> f);

//! called from load_game_resource_pack() when res_id is about to be loaded
extern bool (*on_load_game_resource_pack)(int res_id, dag::Span<GameResourceFactory *> f);

//! called to get name from res_id;
extern bool (*on_get_res_name)(int res_id, String &out_res_name);

//! not really hook, but helper function to be used in tools
int aux_game_res_handle_to_id(const char *res_name, unsigned class_id);

//! called for each gameres (or texture) pack before scanning it to get confirmation; by default all packs are allowed to be scanned
extern bool (*on_gameres_pack_load_confirm)(const char *pack_fname, bool is_tex_pack);
//! called at the end of load_res_packs_from_list()
extern void (*on_load_res_packs_from_list_complete)(const char *pack_list_blk_fname, bool load_grp, bool load_tex);
} // namespace gamereshooks
