//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_resId.h>
#include <util/dag_oaHashNameMap.h>

namespace eastl
{
class allocator;
template <typename T, typename A>
class basic_string;
template <typename R, typename... Args>
class function<R(Args...)>;
} // namespace eastl


// Generic class to make typed pointer to abstract resource object.
// Cast it to expected resource type.
class GameResource;

// GameResource deleter suitable for use with unique_ptr, i.e. "eastl::unique_ptr<GameResource, GameResDeleter>" (for RAII)
struct GameResDeleter
{
  void operator()(GameResource *res);
};

// Resource handle type, use GAMERES_ID macro to get resource handles.
typedef const class GameResourceDummyHandleType *GameResHandle;


// Macro that must be used to specify resource handle,
// name parameter should follow C identifier rules.
// This is done to allow using enum as resource handle in release build,
// while using character strings as resource handles in development.
#define GAMERES_HANDLE(name) ((GameResHandle) #name)


#define NULL_GAMERES_HANDLE   NULL
#define NULL_GAMERES_CLASS_ID (0u)


// Avoid using this macro at all, if possible.
// See note for GAMERES_HANDLE macro above.
#define GAMERES_HANDLE_FROM_STRING(str) ((GameResHandle)(const char *)(str))


void set_no_gameres_factory_fatal(bool no_factory_fatal);

// Set log level for "undefined res" case in addReqResListReferences
void set_gameres_undefined_res_loglevel(int8_t level);

// Get resource (load if not loaded), increment its reference counter.
GameResource *get_game_resource(GameResHandle handle);
GameResource *get_game_resource(GameResHandle handle, bool no_factory_fatal);

// Increment resource reference counter.
void game_resource_add_ref(GameResource *resource);

// Decrement resource reference counter.
void release_game_resource(GameResource *resource);

// Decrement resource reference counter; can handle TEXTUREIDs and identical to release_game_resource
void release_gameres_or_texres(GameResource *resource);

// Decrement resource reference counter.
void release_game_resource(GameResHandle handle);


//! resolves texture resource and return textureid
TEXTUREID get_tex_gameres(const char *resname, bool add_ref_tex = true);

//! returns resource restricted by type id
GameResource *get_game_resource_ex(GameResHandle handle, unsigned type_id);

//! Load one resource ignoring restriction list. Slow in the case of heavy use on DVD.
GameResource *get_one_game_resource_ex(GameResHandle handle, unsigned type_id);

//! returns true when resource is already loaded by gameressys
//! *_nolock version expected to have locked gameres_main_cs
//! *_trylock version might return false if gameres_main_cs is failed to lock (i.e. even if resource already loaded)
bool is_game_resource_loaded(GameResHandle handle, unsigned type_id);
bool is_game_resource_loaded_nolock(GameResHandle handle, unsigned type_id);
bool is_game_resource_loaded_trylock(GameResHandle handle, unsigned type_id);

//! returns true is resource pointer is valid
bool is_game_resource_valid(GameResource *res);


// Free game resources that are not referenced.
// NOTE: unused resources from packs with refcount > 0 will not be freed
void free_unused_game_resources();
// smooth multithreaded - multiple WinAutoLock lock(&gameres_cs) with intervals
void free_unused_game_resources_mt();

// Resource pack management
// addref/delref will be used explicitly only in project code; by default refcount stays equal zero
int get_game_resource_pack_id_by_resource(GameResHandle handle);
int get_game_resource_pack_id_by_name(const char *fname);

const char *get_game_resource_pack_fname(int res_pack_id);

void addref_game_resource_pack(int res_pack_id);
void delref_game_resource_pack(int res_pack_id);
int get_refcount_game_resource_pack(int res_pack_id);

inline void addref_game_resource_pack_by_name(const char *fname)
{
  addref_game_resource_pack(get_game_resource_pack_id_by_name(fname));
}
inline void delref_game_resource_pack_by_name(const char *fname)
{
  delref_game_resource_pack(get_game_resource_pack_id_by_name(fname));
}

// Load resource pack containing specified game resource.
// NOTE: Loading pack loads all resources from it.
void load_game_resource_pack(GameResHandle handle);

// Load all resource packs for which fname coinsides with GRP fpath trail
// (e.g. lion.grp will cause loading of both char/lion.grp and /xlion.grp)
// NOTE: Loading pack loads all resources from it.
void load_game_resource_pack_by_name(const char *fname, bool only_one_gameres_pack = false);

bool is_game_resource_pack_loaded(const char *fname);

inline void enable_gameres_pack_loading(bool) {} // Legacy API. Now gameres pack loading is always on

// enabled finer res loading (allowing fiber/thread switches after each resource, not only after GRP); disabled by default
bool enable_gameres_finer_load(bool en);

void enable_one_gameres_pack_loading(bool en, bool check = true);
void ignore_unavailable_resources(bool ignore);
void logging_missing_resources(bool logging);

// for debug purpose: create unit from console etc.
void debug_enable_free_gameres_pack_loading();
void debug_disable_free_gameres_pack_loading();

// get list of gameres with some class_id
// internally iterates over all gameres; execution time depends on count of all registered gameres, that can be huge;
// be careful, if use in perf critical contexts
void iterate_gameres_names_by_class(unsigned class_id, const eastl::function<void(const char *)> &cb);

bool is_ignoring_unavailable_resources();


enum ReqResListOpt
{
  RRL_setOnly,
  RRL_setAndPreload,
  RRL_setAndPreloadAndReset
};
//! sets list of required resources, so that only these resources will be read from GRP, any other will be skipped
bool set_required_res_list_restriction(const FastNameMap &list, ReqResListOpt opt = RRL_setOnly);
bool set_required_res_list_restriction(const eastl::basic_string<char, eastl::allocator> *begin,
  const eastl::basic_string<char, eastl::allocator> *end, ReqResListOpt opt = RRL_setOnly, size_t str_stride = 0);

// ! calls set_required_res_list_restriction, then loads all resources into out_resources list,
// ! in the same order their names were present in input list, then resets restriction list (RRL_setAndPreloadAndReset)
// ! NOTE: out_resources must be same size as input list, caller must release all loaded resources manually
bool load_game_resource_list(const FastNameMap &list, dag::Span<GameResource *> out_resources);
bool load_game_resource_list(const eastl::basic_string<char, eastl::allocator> *begin,
  const eastl::basic_string<char, eastl::allocator> *end, size_t str_stride, dag::Span<GameResource *> out_resources);

//! resets list of required resources, so any resource in GRP will be loaded during GRP loading
//! initially, gameResSys doesn't have restriction list, so this function will effectively reset to default state
void reset_required_res_list_restriction();

//! tries to load all required resources, if they are not loaded yet; also load resources that required ones depend on;
//! returns false if one or more required resources could not be found/loaded
bool preload_all_required_res();

//! tries to release all resources that are not included in required list and those that they depend on;
//! returns false if one or more not required resources cannot be released due to non-zero refcount
bool release_all_not_required_res();

inline void GameResDeleter::operator()(GameResource *res) { res ? release_game_resource(res) : (void)0; }
