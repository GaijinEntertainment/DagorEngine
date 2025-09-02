//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_texMetaData.h>
#include <drv/3d/dag_samplerHandle.h>
#include <3d/dag_resMgr.h>
#include <generic/dag_tabFwd.h>

namespace d3d
{
struct SamplerInfo;
}

//! \copydoc register_managed_res
D3DRESID register_managed_tex(const char *name, BaseTexture *res);


//! \copydoc is_managed_res_factory_set
static inline bool is_managed_tex_factory_set(D3DRESID id) { return is_managed_res_factory_set(id); }
//! \copydoc get_managed_res_name
static inline const char *get_managed_texture_name(D3DRESID id) { return get_managed_res_name(id); }
//! \copydoc get_managed_res_id
static inline D3DRESID get_managed_texture_id(const char *res_name) { return get_managed_res_id(res_name); }
//! \copydoc get_managed_res_refcount
static inline int get_managed_texture_refcount(D3DRESID id) { return get_managed_res_refcount(id); }
//! \copydoc enable_res_mgr_mt
static inline void enable_tex_mgr_mt(bool enable, int max_res_entry_count) { enable_res_mgr_mt(enable, max_res_entry_count); }

//! \brief Texture factory provides custom creation mechanism
class TextureFactory
{
public:
  TextureFactory() = default;
  TextureFactory(TextureFactory &&) = default;
  TextureFactory &operator=(TextureFactory &&) = default;
  virtual ~TextureFactory() {}

  //! \brief Create texture a with the specified ID
  //! \warning Make sure to store the result to release it in releaseTexture()
  //! \param id ID of the texture to create
  virtual BaseTexture *createTexture(TEXTUREID id) = 0;

  //! \brief Release a texture that was created using createTexture
  //! \param texture Texture to release
  //! \param id ID of the texture to release
  virtual void releaseTexture(BaseTexture *texture, TEXTUREID id) = 0;

  //! \brief A callback that is called on install/uninstall
  virtual void texFactoryActiveChanged(bool /*active*/) {}

  //! \brief Schedules loading of specified texture (if applicable)
  //! \note Thread-safe
  //! \param id ID of the texture to load
  //! \param req_ql Required quality level
  //! \returns `true` when loading scheduled successfully
  virtual bool scheduleTexLoading(TEXTUREID /*id*/, TexQL /*req_ql*/) { return false; }

  //! \brief Loads and returns contents of texture as DDSx data
  //! \warning This function is synchronous and should be used with caution
  //! \param id ID of the texture to load
  //! \param out_ddsx Output buffer for DDSx data
  //! \returns `true` when texture was loaded successfully
  virtual bool getTextureDDSx(TEXTUREID /*id*/, Tab<char> & /*out_ddsx*/) { return false; }

  //! \brief Callback for notification of texture being unregistered from the manager
  //! \param id ID of the texture that was unregistered
  virtual void onUnregisterTexture(TEXTUREID /*id*/) {}

  //! \brief Determines whether the string \p nm has static lifetime and can be referenced at any time without copying
  //! \param nm String to check
  //! \returns `true` if the string is persistent
  virtual bool isPersistentTexName(const char * /*nm*/) { return false; }

  //! \brief When a texture factory instance is destroyed at runtime, this function **must** be called to notify the manager about it.
  //! \warning This should not be called from factories which are static
  //! singletons or are destroyed after the manager was already de-initialized.
  static void onTexFactoryDeleted(TextureFactory *f);
};

//! \brief Adds entry for managed texture name and returns TEXTUREID for it
//! \details When entry for 'name' already exists, it just returns TEXTUREID.
//! For new entry 'factory' is set as creator, for existing entry 'factory' must be nullptr.
//! \warning Texture name is case-insensitive and must be unique.
//! \param name Name that uniquely identifies the texture
//! \param factory Custom texture creation mechanism, must be nullptr for existing textures
//! \returns Managed ID of the texture
TEXTUREID add_managed_texture(const char *name, TextureFactory *factory = nullptr);

//! \brief Adds or updates entry for managed array texture name, stores creation properties and returns TEXTUREID for it.
//! \details Array texture will be composed of slices with texture names \p tex_slice_nm.
//! \warning Textures in slices must have the same format and size, and furthermore, some restrictions of DDSx flags apply.
//! \param name Name that uniquely identifies the new array texture
//! \param tex_slice_nm Array of texture names to use as slices
//! \returns Managed ID of the texture
TEXTUREID add_managed_array_texture(const char *name, dag::ConstSpan<const char *> tex_slice_nm);

//! \brief Updates an existing array texture entry with new slices.
//! \warning The number of slices must match the original array texture.
//! \note Simply calls add_managed_texture() under the hood, this function exists purely for readability purposes.
//! \param name Name of the array texture to update
//! \param tex_slice_nm Array of new texture names to use as slices
//! \returns Managed ID of the array texture that was updated
TEXTUREID update_managed_array_texture(const char *name, dag::ConstSpan<const char *> tex_slice_nm);

//! \brief Checks all registered array textures and reloads those referencing specified slice
//! \param slice_tex_name Name of changed texture that can be used as slice in some array texture
//! \returns Number of reloaded array textures (or 0 if no array texture reference that slice)
unsigned reload_managed_array_textures_for_changed_slice(const char *slice_tex_name);

//! \brief Schedules removal of the managed entry for the specified \p id
//! and sets \p id to BAD_TEXTUREID (unconditionally, even if the operation fails or is deferred).
//! \details When refCount = 0, removal is performed immediately, otherwise it is deferred.
//! Can be used to remove entries created with register_managed_tex(), add_managed_texture(), add_managed_array_texture()
//! \param id ID of the texture to remove
//! \returns `true` if the texture was removed immediately, `false` otherwise, including if the operation was invalid.
bool evict_managed_tex_id(TEXTUREID &id);

//! \brief Checks whether contents of managed texture was loaded
//! \param id ID of the texture to check
//! \param fq_loaded `true` to check for full quality, `false` for any quality
//! \returns `true` if the texture is loaded, `false` otherwise
bool check_managed_texture_loaded(TEXTUREID id, bool fq_loaded = false);

//! \brief Checks whether contents of all managed textures in list was loaded,
//! analogous to calling check_managed_texture_loaded() multiple times.
//! \note Skips checks for textures with refcount=0 by default.
//! \param id List of texture IDs to check.
//! \param fq_loaded `true` to check for full quality, `false` for any quality
//! \param ignore_unref_tex `true` to skip textures with refcount=0
//! \returns `true` if all textures (excluding those with refcount=0 if \p ignore_unref_tex is true) are loaded, `false` otherwise.
bool check_all_managed_textures_loaded(dag::ConstSpan<TEXTUREID> id, bool fq_loaded = false, bool ignore_unref_tex = true);

//! \brief Replaces the texture for managed id \p id.
//! \param id ID of the texture to replace
//! \param new_texture New driver texture to use for this \p id
//! \param factory Custom texture creation mechanism
//! \returns `true` if the texture was replaced successfully, `false` otherwise
bool change_managed_texture(TEXTUREID id, BaseTexture *new_texture, TextureFactory *factory = NULL);

//! \brief Discards unused texture: unloads texture when refCount=0, does nothing otherwise.
//! \param id ID of the texture to discard
void discard_unused_managed_texture(TEXTUREID id);

//! \brief Iterate all managed textures and unload those with refCount=0
void discard_unused_managed_textures();

//! \brief Set current frame as 'last frame used' for texture
//! \param id ID of the texture to mark
//! \param req_lev Quality level that was used on this frame
inline void mark_managed_tex_lfu(TEXTUREID id, unsigned req_lev = 15) { D3dResManagerData::markResLFU(id, req_lev); }

//! \brief Request texture content load
//! \param id ID of the texture to load
//! \returns true when texture already loaded, false otherwise
bool prefetch_managed_texture(TEXTUREID id);

//! \brief Requests texture content load for a list of textures, the same as calling prefetch_managed_texture() multiple times.
//! \param id List of texture IDs to load
//! \returns `true` when all textures are already loaded
bool prefetch_managed_textures(dag::ConstSpan<TEXTUREID> id);

//! \brief Requests texture content load for list of textures by TEXTAG
//! \param textag The tag to load textures for
//! \returns `true` when all textures with the tag are already loaded
bool prefetch_managed_textures_by_textag(int textag);

//! \brief Marks list of textures as important (to force loading content upto requested quality level as soon as possible)
//! \param id List of texture IDs to mark
//! \param add_importance Additional importance to add to the textures
//! \param min_lev_for_dyn_decrease important textures with more levels  min can be downgraded 1 mip when dyn decrease used
void mark_managed_textures_important(dag::ConstSpan<TEXTUREID> id, unsigned add_importance = 1, int min_lev_for_dyn_decrease = 16);

//! \brief Resets streaming state of textures so that they can be reloaded respecting the global tex quality settings changes
void reset_managed_textures_streaming_state();

//! \brief Sets the default texture factory to use for managed textures
void set_default_tex_factory(TextureFactory *tf);
//! \brief Gets the default texture factory
TextureFactory *get_default_tex_factory();

//! \brief Gets the symbolic texture factory which always produces nulls for texture creation
TextureFactory *get_symbolic_tex_factory();

//! \brief Gets the stub texture factory which always produces stub 1x1 textures
TextureFactory *get_stub_tex_factory();


//! \brief Init data and callbacks to support streaming.
//! \details Settings are taken from dgs_get_settings()->getBlockByName("texStreaming").
//! \note reload_jobmgr_id=-2 means auto creation of jobmanager, reload_jobmgr_id=-1 means no jobmanager
//! \param rload_jobmgr_id Either -1 or -2, controls whether to create a job manager for texture streaming
void init_managed_textures_streaming_support(int reload_jobmgr_id = -2);

//! \brief Checks whether texture streaming is inited and active
//! \returns `true` if texture streaming is active, `false` otherwise
bool is_managed_textures_streaming_active();
//! \brief Checks whether texture streaming loads textures only when used
//! \returns `true` if texture streaming is load-on-demand, `false` otherwise
bool is_managed_textures_streaming_load_on_demand();

//! \brief Dumps current memory usage (Sys/Gpu) state of texture streaming
void dump_texture_streaming_memory_state();

//! \brief Checks whether the specified texture is split BQ/HQ with missing HQ part
//! \param id ID of the texture to check
//! \returns `true` if the texture is missing HQ part, `false` otherwise
bool is_managed_texture_incomplete(TEXTUREID id);

//! \brief Re-loads anisotropy config from settings
void load_anisotropy_from_settings();

//! \brief Marks that a texture's anisotropy should not be changed automatically.
//! \param id ID of the texture to mark
void add_anisotropy_exception(TEXTUREID id);

//! \brief Resets anisotropy for all textures whose name contains
//! \p tex_name_filter as a substring.
//! \param tex_name_filter Substring filter to match texture names
void reset_anisotropy(const char *tex_name_filter = nullptr);

//! \brief Attempts to load the texture for usage on the current frame and simply checks for immediate success.
//! \param id ID of the texture to load and check
//! \param fq_loaded `true` to check for full quality, `false` for any quality
//! \returns `true` if the texture is loaded, `false` otherwise
inline bool prefetch_and_check_managed_texture_loaded(TEXTUREID id, bool fq_loaded = false)
{
  if (check_managed_texture_loaded(id, fq_loaded))
    return true;
  bool loaded = is_managed_textures_streaming_load_on_demand() ? prefetch_managed_texture(id) : false;
  mark_managed_tex_lfu(id);
  return (loaded && fq_loaded) ? check_managed_texture_loaded(id, fq_loaded) : loaded;
}

//! \brief Attempts to load multiple textures for usage on the current frame and check for immediate success.
//! \param tex_list List of texture IDs to load and check
//! \param fq_loaded `true` to check for full quality, `false` for any quality
//! \returns `true` if all textures are loaded, `false` otherwise
bool prefetch_and_check_managed_textures_loaded(dag::ConstSpan<TEXTUREID> tex_list, bool fq_loaded = false);

//! \brief Synchronously loads a list of textures, blocking the current thread until all textures are loaded.
//! \param tex_list List of texture IDs to load
//! \param fq_loaded `true` to wait for full quality, `false` for any quality
void prefetch_and_wait_managed_textures_loaded(dag::ConstSpan<TEXTUREID> tex_list, bool fq_loaded = false);

//! \brief Utility for iterating all managed textures.
//! \details Managed textures form a linked list, so for an arbitrary ID, this function returns the "next" ID in the list.
//! Hence one is supposed to call this function in a loop until it returns BAD_TEXTUREID.
//! \param after_tid ID of the texture to get the next texture for. Use BAD_TEXTUREID to start iteration.
//! \param min_ref_count Minimum reference count filter
//! \returns Some ID that follows \p after_tid and has refCount >= \p min_ref_count OR BAD_TEXTUREID if the iteration is done.
TEXTUREID iterate_all_managed_textures(TEXTUREID after_tid, int min_ref_count);


//! \name Helper wrappers
//! \brief See iterate_all_managed_textures(). Use as follows:
//! \code for (TEXTUREID id = first_managed_texture(); id != BAD_TEXTUREID; id = next_managed_texture(id)) \endcode
//!@{
//! \brief Get the first managed texture ID for iteration
//! \param min_rc Minimum reference count filter
//! \returns First managed texture ID with refCount >= \p min_rc
inline TEXTUREID first_managed_texture(int min_rc = 0) { return iterate_all_managed_textures(BAD_TEXTUREID, min_rc); }

//! \brief Get the next managed texture ID for iteration
//! \param prev_id ID of the previous texture
//! \param min_rc Minimum reference count filter
//! \returns Next managed texture ID after \p prev_id with refCount >= \p min_rc
inline TEXTUREID next_managed_texture(TEXTUREID prev_id, int min_rc = 0) { return iterate_all_managed_textures(prev_id, min_rc); }
//!@}

//! \brief Access to the highest valid texture ID for storing properties of managed textures in arrays.
//! \returns Highest valid texture ID
TEXTUREID get_max_managed_texture_id();

//! \brief Get generation of managed texture content, which changes during streaming
//! \param tid valid ID of texture
//! \returns Generation of streaming data
uint16_t get_managed_texture_streaming_generation(TEXTUREID tid);

//! returns true when tid resembles texture ID and (optionally checked) contains valid value (non-BAD and has proper generation)
inline bool is_managed_texture_id_valid(TEXTUREID tid, bool validate_value)
{
  if (!tid.checkMarkerBit())
    return false;
  return validate_value ? D3dResManagerData::isValidID(tid, nullptr) : true;
}

//! \brief Read-only access to the current texture quality level
//! \warning This is READ-ONLY, do not modify this value
extern int dgs_tex_quality;

//! \brief Read-only access to the current texture anisotropy
//! \warning This is READ-ONLY, do not modify this value
extern int dgs_tex_anisotropy;

//! \brief Get texture meta data
//! \param id ID of the texture to get the meta data for
TextureMetaData get_texture_meta_data(TEXTUREID id);

//! \brief Get a sampler info structure from texture meta data
d3d::SamplerInfo get_sampler_info(const TextureMetaData &texture_meta_data, bool force_addr_from_tmd = true);

//! \brief Returns separate sampler for the texture ID
//! \param id ID of the texture to get the sampler for
//! \returns Sampler handle for the texture. If no separate sampler is set, returns invalid handle
//! \note Returned sampler must not be destroyed
//! \note The function is not thread-safe, and the caller must ensure that there are no concurrent setters for this texture ID
d3d::SamplerHandle get_texture_separate_sampler(TEXTUREID id);

//! \brief Sets separate sampler for the texture ID
//! \param id ID of the texture to set the sampler for
//! \param sampler_info Sampler info for sampler to set
//! \returns `true` if the sampler was set successfully, `false` otherwise
//! \note Calling a function for the same texture ID is not thread-safe
//! \note Calling a function for different texture IDs is thread-safe
bool set_texture_separate_sampler(TEXTUREID id, const d3d::SamplerInfo &sampler_info);

struct LODBiasRule
{
  const char *substring;
  float bias;
};

//! \brief Sets the LOD bias for a batch of substring-bias pairs.
//! \param rules A span of LODBiasRule objects containing substring and LOD bias pairs.
void set_add_lod_bias_batch(dag::Span<const LODBiasRule> rules);