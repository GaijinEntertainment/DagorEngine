//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_resMgr.h>
#include <generic/dag_tabFwd.h>

D3DRESID register_managed_tex(const char *name, BaseTexture *res);
static inline bool is_managed_tex_factory_set(D3DRESID id) { return is_managed_res_factory_set(id); }
static inline const char *get_managed_texture_name(D3DRESID id) { return get_managed_res_name(id); }
static inline D3DRESID get_managed_texture_id(const char *res_name) { return get_managed_res_id(res_name); }
static inline int get_managed_texture_refcount(D3DRESID id) { return get_managed_res_refcount(id); }
static inline void enable_tex_mgr_mt(bool enable, int max_res_entry_count) { enable_res_mgr_mt(enable, max_res_entry_count); }

// Texture factory provides custom creation mechanism
class TextureFactory
{
public:
  virtual ~TextureFactory() {}

  // Create texture, be sure to store it to release it in releaseTexture()
  virtual BaseTexture *createTexture(TEXTUREID id) = 0;

  // Release created texture
  virtual void releaseTexture(BaseTexture *texture, TEXTUREID id) = 0;

  // Called on install/uninstall
  virtual void texFactoryActiveChanged(bool /*active*/) {}

  //! schedules loading of specified texture (if applicable); thread-safe; return true when loading scheduled
  virtual bool scheduleTexLoading(TEXTUREID /*id*/, TexQL /*req_ql*/) { return false; }
  //! loads and returns contents of texture as DDSx data
  virtual bool getTextureDDSx(TEXTUREID /*id*/, Tab<char> & /*out_ddsx*/) { return false; }
  //! notification of texture unregister from manager
  virtual void onUnregisterTexture(TEXTUREID /*id*/) {}
  //! returns true when 'nm' points to persistent storage and may be referenced without duplicating string
  virtual bool isPersistentTexName(const char * /*nm*/) { return false; }

  static void onTexFactoryDeleted(TextureFactory *f);
};


// Adds entry for managed texture name and returns TEXTUREID for it;
// When entry for 'name' already exists, it just returns TEXTUREID;
// For new entry 'factory' is set as creator, for existing entry 'factory' must be NULL;
// Texture name is not case-sensitive.
TEXTUREID add_managed_texture(const char *name, TextureFactory *factory = NULL);

// Adds or updates entry for managed array texture name, stores creation properties and returns TEXTUREID for it;
// Array texture will be composed of texture names 'tex_slice_nm';
// Textures in slices must have the same format and size and have some restrictions of DDSx flags.
TEXTUREID add_managed_array_texture(const char *name, dag::ConstSpan<const char *> tex_slice_nm);

// Updates entry for managed array texture name, requires same number of slices.
// it just calls add_managed_texture, additional function is just for readability.
TEXTUREID update_managed_array_texture(const char *name, dag::ConstSpan<const char *> tex_slice_nm);


// removes specified texId and entry when refCount=0 (or schedule later removal) and sets id to BAD_TEXTUREID
// can be used to remove entries created with register_managed_tex(), add_managed_texture(), add_managed_array_texture()
bool evict_managed_tex_id(TEXTUREID &id);


// Checks whether contents of managed texture was loaded
bool check_managed_texture_loaded(TEXTUREID id, bool fq_loaded = false);
// Checks whether contents of all managed textures in list was loaded (skips checks for textures with refcount=0 by default)
bool check_all_managed_textures_loaded(dag::ConstSpan<TEXTUREID> id, bool fq_loaded = false, bool ignore_unref_tex = true);

// change texture by id - return true, if all is OK & false if error
bool change_managed_texture(TEXTUREID id, BaseTexture *new_texture, TextureFactory *factory = NULL);

// Discards unused texture: unload texture when refCount=0
void discard_unused_managed_texture(TEXTUREID id);

// Discards all unused textures: unload texture when refCount=0
void discard_unused_managed_textures();

// set current frame as 'last frame used' for texture
inline void mark_managed_tex_lfu(TEXTUREID id, unsigned req_lev = 15) { D3dResManagerData::markResLFU(id, req_lev); }

// request texture content load (returns true when texture already loaded)
bool prefetch_managed_texture(TEXTUREID id);

// request content load for list of textures (returns true when all textures already loaded)
bool prefetch_managed_textures(dag::ConstSpan<TEXTUREID> id);

// request content load for list of textures by TEXTAG (returns true when all textures already loaded)
bool prefetch_managed_textures_by_textag(int textag);

// marks list of textures as important (to force loading content upto requested quality level)
void mark_managed_textures_important(dag::ConstSpan<TEXTUREID> id, unsigned add_importance = 1);

// resets streaming state of textures so that they can be reloaded respecting the global tex quality settings changes
void reset_managed_textures_streaming_state();

// access to default texture factory
void set_default_tex_factory(TextureFactory *tf);
TextureFactory *get_default_tex_factory();


TextureFactory *get_symbolic_tex_factory();
TextureFactory *get_stub_tex_factory();


//! init data and callbacks to support streaming; settings are taken from dgs_get_settings()->getBlockByName("texStreaming")
//! NOTE: reload_jobmgr_id=-2 means auto creation of jobmanager; reload_jobmgr_id=-1 means no jobmanager
void init_managed_textures_streaming_support(int reload_jobmgr_id = -2);

//! returns true if texture streaming is inited and active
bool is_managed_textures_streaming_active();
//! returns true if texture streaming loads textures only when used
bool is_managed_textures_streaming_load_on_demand();

//! returns true when specified texture is split BQ/HQ with missing HQ part
bool is_managed_texture_incomplete(TEXTUREID id);

void load_anisotropy_from_settings();
void add_anisotropy_exception(TEXTUREID id);
void reset_anisotropy(const char *tex_name_filter = nullptr);


inline bool prefetch_and_check_managed_texture_loaded(TEXTUREID id, bool fq_loaded = false)
{
  if (check_managed_texture_loaded(id, fq_loaded))
    return true;
  bool loaded = is_managed_textures_streaming_load_on_demand() ? prefetch_managed_texture(id) : false;
  mark_managed_tex_lfu(id);
  return (loaded && fq_loaded) ? check_managed_texture_loaded(id, fq_loaded) : loaded;
}

bool prefetch_and_check_managed_textures_loaded(dag::ConstSpan<TEXTUREID> tex_list, bool fq_loaded = false);
void prefetch_and_wait_managed_textures_loaded(dag::ConstSpan<TEXTUREID> tex_list, bool fq_loaded = false);

//! iterates managed d3dres; returns ID of next res after 'after_rid' that has refCount>=min_ref_count;
//! returns BAD_TEXTUREID when iterate is done
TEXTUREID iterate_all_managed_textures(TEXTUREID after_tid, int min_ref_count);

//! inline helpers for pattern like:
//!   for (TEXTUREID id = first_managed_texture(); id != BAD_TEXTUREID; id = next_managed_texture(id))
inline TEXTUREID first_managed_texture(int min_rc = 0) { return iterate_all_managed_textures(BAD_TEXTUREID, min_rc); }
inline TEXTUREID next_managed_texture(TEXTUREID prev_id, int min_rc = 0) { return iterate_all_managed_textures(prev_id, min_rc); }

//! returns highest valid texture ID
TEXTUREID get_max_managed_texture_id();

//! returns true when tid resembles texture ID and (optionally checked) contains valid value (non-BAD and has proper generation)
inline bool is_managed_texture_id_valid(TEXTUREID tid, bool validate_value)
{
  if (!tid.checkMarkerBit())
    return false;
  return validate_value ? D3dResManagerData::isValidID(tid, nullptr) : true;
}

//! read-only value contains current texture quality
extern int dgs_tex_quality;

//! read-only value contains current texture anisotorpy
extern int dgs_tex_anisotropy;
