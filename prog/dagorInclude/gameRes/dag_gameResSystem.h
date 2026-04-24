//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gameRes/dag_gameResources.h>

#include <util/dag_string.h>
#include <generic/dag_tab.h>

// This header file is not needed while just using game resources,
// it should be used when implementing new game resource factories only.


class IGenLoad;
class DataBlock;
class WinCritSec;


#define NULL_GAMERES_ID -1


class GameResourceFactory
{
public:
  typedef gameres_rrl_cptr_t RRL;
  virtual ~GameResourceFactory() {}

  // Returns class id for resources managed by this factory.
  virtual unsigned getResClassId() = 0;


  // Get resource class name for debugging.
  virtual const char *getResClassName() = 0;

  // tests whether resource is loaded and ready to be get immediately
  virtual bool isResLoaded(int res_id) = 0;
  // tests whether resource pointer managed by this factory
  virtual bool checkResPtr(GameResource *res) = 0;

  // Factory manages reference counters for GameResources, so it must
  // increment reference counter for specified resource and return it.
  //
  // If resource is not loaded, factory must call load_game_resource_pack()
  // to load pack that contains this resource. That should result in adding
  // requested GameResource to the factory. Then this resource can be handled
  // as described above.
  virtual GameResource *getGameResource(RRL rrl, int res_id) = 0;

  // Increment resource reference counter.
  virtual bool addRefGameResource(GameResource *resource) = 0;

  // Decrement resource reference counter.
  virtual void releaseGameResource(int res_id) = 0;
  virtual bool releaseGameResource(GameResource *resource) = 0;

  // Free resources with zero reference counters. return true, if some res. released
  virtual bool freeUnusedResources(RRL rrl, bool forced_free_unref_packs, bool once = false) = 0;


  // Load specified GameResource data from stream.
  // Called while loading resource pack.
  // Initial reference counter is zero.
  virtual void loadGameResourceData(int res_id, IGenLoad &cb) = 0;

  // Create specified GameResource from provided data.
  // Called while loading resource pack.
  // Initial reference counter is zero.
  virtual void createGameResource(RRL rrl, int res_id, const int *reference_ids, int num_refs) = 0;

  // Iterate over all GameResource stored in this factory, calling cb with pointer to each one.
  virtual void iterateGameResources(const eastl::function<void(GameResource *)> &)
  {
    G_ASSERTF(0, "iterateGameResources is not implemented for GameResourceFactory '%s'", getResClassName());
  }

  // full reset (destroy all resources)
  virtual void reset() = 0;

  virtual void dumpResourcesRefCount() {}

  // if resource with 'res_id' is already created move it to obsolete list and return pointer to it;
  // subsequent call to getGameResource() will reload/recreate resource anew (from updated data);
  // when 'res_id' is not used/created yet just returns nullptr;
  virtual GameResource *discardOlderResAfterUpdate([[maybe_unused]] int res_id) { return nullptr; }

public:
  static GameResource *get_game_resource(int res_id, RRL rrl);
  static void release_game_resource_nolock(int res_id);
  static void load_game_resource_pack(int res_id, RRL rrl);
  static void load_game_resource_pack_gameres_main_cs_locked(int res_id, RRL rrl);
};

#define FATAL_ON_UNLOADING_USED_RES(resId, refcnt)                     \
  do                                                                   \
  {                                                                    \
    String name;                                                       \
    get_game_resource_name(resId, name);                               \
    DAG_FATAL("unloading res <%s> which is in use: %d", name, refcnt); \
  } while (0)


void add_factory(GameResourceFactory *factory);

void remove_factory(GameResourceFactory *factory);

GameResourceFactory *find_gameres_factory(unsigned classid);


// validate game-res-id and return res_id or NULL_GAMERES_ID
int validate_game_res_id(int res_id);


// Get class id of specified game-res.
unsigned get_game_res_class_id(int res_id);
unsigned get_resource_type_id(const char *res_name);


// Get name for game-res to be used in debugging.
void get_game_resource_name(int res_id, String &name);


// sets up target BLK and path prefixes used to record all scanned resources/textures
void set_gameres_scan_recorder(DataBlock *rootBlk, const char *grp_pref, const char *tex_pref);

// Scan for game resource packs in specified location.
// Path must end with slash.
void scan_for_game_resources(const char *path, bool scan_subdirs, bool scan_dxp = false, bool allow_override = false,
  bool scan_vromfs = false);

// Load pack list BLK from specified filename and load respacks according to list
// when res_base_dir==NULL, base directory is extracted from pack_list_blk_fname path
void load_res_packs_from_list(const char *pack_list_blk_fname, bool load_grp = true, bool load_tex = true,
  const char *res_base_dir = NULL);
void load_res_packs_from_list_blk(const DataBlock &list_blk, const char *ref_pack_list_blk_fname, bool load_grp, bool load_tex,
  const char *res_base_dir);

// Patch resource viad loading pack list BLK from specified filename and
// load-update respacks according to list;
// patching is allowed only when base_grp_vromfs file mathes the one used to build patch;
bool load_res_packs_patch(const char *patch_pack_list_blk_fn, const char *base_grp_vromfs_fn, bool load_grp, bool load_tex);

// add prefix_path to all respack paths
void patch_res_pack_paths(const char *prefix_path);

// release all resources and reset game resource packs list
void reset_game_resources();

void dump_game_resources_refcount(unsigned cls = 0);

//! returns gameres critical section (used to syncronize access to resources in factories)
WinCritSec &get_gameres_main_cs();

//! resets list of available files in resource folders; resets usage of that list
void reset_gameres_available_files_list();
//! adds files scanned in 'root_dir' (recursively) to list of available files; enables usage of that list
void add_gameres_available_files_list(const char *root_dir);
