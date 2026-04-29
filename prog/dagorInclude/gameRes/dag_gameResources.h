//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_resId.h>
#include <gameRes/dag_rrl.h>
#include <generic/dag_span.h>
#include <EASTL/functional.h>


//! Opaque class to make typed pointer to abstract resource object. Cast it to expected resource type.
class GameResource;

//! GameResource deleter suitable for use with unique_ptr, i.e. "eastl::unique_ptr<GameResource, GameResDeleter>" (for RAII)
struct GameResDeleter
{
  void operator()(GameResource *res);
};


void set_no_gameres_factory_fatal(bool no_factory_fatal);

//! Set log level for "undefined res" case in addReqResListReferences
void set_gameres_undefined_res_loglevel(int8_t level);

//! Increments resource reference counter (type_id=0 may be passed if type of resource is not known)
void game_resource_add_ref_ex(const void *resource_ptr, unsigned type_id);

//! Decrements resource reference counter (type_id=0 may be passed if type of resource is not known)
void release_game_resource_ex(const void *resource_ptr, unsigned type_id);


//! resolves texture resource and returns texID (use release_managed_tex() to release reference later)
TEXTUREID get_tex_gameres(const char *resname, bool add_ref_tex = true);

//! returns resource restricted by type id
GameResource *get_game_resource_ex(const char *resname, unsigned type_id, gameres_rrl_cptr_t rrl = nullptr);

//! Load one resource ignoring restriction list. Slow in the case of heavy use on DVD.
GameResource *get_one_game_resource_ex(const char *resname, unsigned type_id);

//! returns true when resource is already loaded by gameressys
//! *_nolock version expected to have locked gameres_main_cs
//! *_trylock version might return false if gameres_main_cs is failed to lock (i.e. even if resource already loaded)
bool is_game_resource_loaded(const char *resname, unsigned type_id);
bool is_game_resource_loaded_nolock(const char *resname, unsigned type_id);
bool is_game_resource_loaded_trylock(const char *resname, unsigned type_id);

//! returns true is resource pointer is valid
bool is_game_resource_valid(GameResource *res);


//! Free game resources that are not referenced (resources mentioned by optional RRL will not be freed)
void free_unused_game_resources(gameres_rrl_cptr_t rrl = nullptr);

// Resource pack management
int get_game_resource_pack_id_by_resource(const char *resname);
int get_game_resource_pack_id_by_name(const char *fname);
const char *get_game_resource_pack_fname(int res_pack_id);
bool is_game_resource_pack_loaded(const char *fname);

void ignore_unavailable_resources(bool ignore);
void logging_missing_resources(bool logging);

// get list of gameres with some class_id
// internally iterates over all gameres; execution time depends on count of all registered gameres, that can be huge;
// be careful, if use in perf critical contexts
void iterate_gameres_names_by_class(unsigned class_id, const eastl::function<void(const char *)> &cb);

bool is_ignoring_unavailable_resources();


//! loads bunch of resources ('inout_resId_gameRes' contains res_id as input and will contain GameResource* as output)
//! returns false if one or more resources could not be found/loaded; caller must release all loaded resources manually
bool load_game_resources(dag::Span<GameResource *> inout_resId_gameRes, gameres_rrl_cptr_t rrl);

//! loads bunch of resources to 'out_resources' using 'get_resname(idx)' for each resource (uses RRL to load only needed res)
//! returns false if one or more resources could not be found/loaded; caller must release all loaded resources manually
template <class CB>
bool load_game_resource_list(dag::Span<GameResource *> out_resources, CB get_resname /*(unsigned idx)*/)
{
  if (!out_resources.size())
    return true;
  ScopedGameResRestrictionListHolder holder(create_res_restriction_list(nullptr, out_resources.size()));
  for (unsigned idx = 0; idx < out_resources.size(); idx++)
    out_resources[idx] = (GameResource *)(void *)(intptr_t)extend_res_restriction_list(holder.rrl, get_resname(idx));
  return load_game_resources(out_resources, holder.rrl);
}


//! tries to load all required resources, if they are not loaded yet; also load resources that required ones depend on;
//! returns false if one or more required resources could not be found/loaded
bool preload_all_required_res(gameres_rrl_cptr_t rrl);

//! preloads only one game resource for resname (uses RRL to avoid loading other stuff)
//! returns false if required resource could not be found/loaded
inline bool preload_game_resource(const char *resname)
{
  ScopedGameResRestrictionListHolder holder(create_res_restriction_list(resname));
  return preload_all_required_res(holder.rrl);
}

//! preloads resources from NameMap-like list with resource names (uses RRL to avoid loading unneeded resources)
//! returns false if one or more required resources could not be found/loaded
template <class NM>
inline bool preload_game_resources(const NM &list)
{
  if (!list.nameCount())
    return true;
  ScopedGameResRestrictionListHolder holder(create_res_restriction_list(list));
  return preload_all_required_res(holder.rrl);
}

//! preloads resources with names passed as get_resname(idx) lamba (uses RRL to avoid loading unneeded resources)
//! returns false if one or more required resources could not be found/loaded
template <class CB>
inline bool preload_game_resources(unsigned resname_count, CB get_resname /*(unsigned idx)*/)
{
  if (!resname_count)
    return true;
  ScopedGameResRestrictionListHolder holder(create_res_restriction_list<CB>(resname_count, get_resname));
  return preload_all_required_res(holder.rrl);
}


inline void GameResDeleter::operator()(GameResource *res) { res ? release_game_resource_ex(res, 0) : (void)0; }
