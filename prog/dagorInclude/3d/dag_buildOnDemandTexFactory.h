//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resMgr.h>
#include <generic/dag_span.h>

// forwards for used classes
class TextureFactory;
class IGenLoad;
class String;
class TextureIdSet;
struct TextureMetaData;
namespace ddsx
{
struct Header;
}


//! helper interface to build texture on demand (to be used by texture factory implementation)
struct ITexBuildOnDemandHelper
{
  struct asset_handle_t;

  //! checks whether texture name matches one supported by build-on-demand helper
  virtual bool checkTexNameHandled(const char *tex_fn) = 0;
  //! resolves texture name to asset handle (that can be used in other API); fills out_tmd if texture resolved successfuly
  virtual asset_handle_t *resolveTex(const char *tex_fn, TextureMetaData &out_tmd) = 0;

  //! fills DDSx header for texture asset (along with level and nameId for each TQL); returns false when headed cannot be deduced
  virtual bool getConvertibleTexHeader(asset_handle_t *h, ddsx::Header &out_hdr, unsigned &out_q_lev_desc, unsigned *out_name_ids) = 0;
  //! fills target filepath and configured gamma for texture asset; returns false when bad assed handle passed
  virtual bool getSimpleTexProps(asset_handle_t *h, String &out_fn, float &out_gamma) = 0;

  //! resolved nameId to texture asset name (used mainly for reporting)
  virtual const char *getTexName(unsigned nameId) = 0;

  //! prebuilds DDSx to cache for given texture asset nameId; returns false on error
  virtual bool prebuildTex(unsigned nameId, bool &out_cache_was_updated) = 0;

  //! returns prebuilt (or cached) DDSx contents for given texture asset nameId; returns empty span on error
  virtual dag::Span<char> getDDSxTexData(unsigned nameId) = 0;
  //! releases data previously got with getDDSxTexData(), needed in order to match allocator pool
  virtual void releaseDDSxTexData(dag::Span<char> data) = 0;
};

//! state of current async loading (reported by texture factory)
struct AsyncTextureLoadingState
{
  unsigned pendingLdCount[TQL__COUNT] = {0}, pendingLdTotal = 0;
  unsigned totalLoadedCount = 0, totalLoadedSizeKb = 0, totalCacheUpdated = 0;
};

//! combined description of managed texture entry (gathered from RMGR fields)
struct ManagedTexEntryDesc
{
  uint16_t width = 0, height = 0, depth = 0, mipLevels = 0; //< dimensions of texture
  unsigned qResSz[TQL__COUNT] = {0};                        //< resource size for each QL available
  uint8_t qLev[TQL__COUNT] = {0};                           //< quality levels for each QL available
  uint8_t maxLev = 0, ldLev = 0, maxQL = 0, curQL = 0;      //< max quality level and current loaded state
  uint8_t isLoading : 1 = 0, isLoadedWithErrors : 1 = 0;
};


namespace build_on_demand_tex_factory
{
TextureFactory *get();
//! initializes texture factory to manage textures using specified build-helper
TextureFactory *init(ITexBuildOnDemandHelper *h);
//! terminates texture factory and releases allocated resources
void term(TextureFactory *f);
//! stops tex loading job and resets job queue (and optionally disables async tex loading after that); returns previous allow_loading
bool cease_loading(TextureFactory *f, bool allow_further_tex_loading);

//! schedule tex prebuilding job with specified quality
void schedule_prebuild_tex(TextureFactory *f, unsigned tex_nameId, TexQL ql);

//! fills current state of async textures loading for given factory; returns false for bad factory
bool get_current_loading_state(const TextureFactory *f, AsyncTextureLoadingState &out_state);

//! limit texture quality to max specified level for textures in list
void limit_tql(TextureFactory *f, TexQL max_tql, const TextureIdSet &tid);
//! limit texture quality to max specified level for all registered textures)
void limit_tql(TextureFactory *f, TexQL max_tql);
}; // namespace build_on_demand_tex_factory

//! fills description for managed texture entry; returns false for bad entry
bool get_managed_tex_entry_desc(TEXTUREID tid, ManagedTexEntryDesc &out_desc);

//! downgrades/discards texture to specified level (1 means stub)
bool downgrade_managed_tex_to_lev(TEXTUREID tid, int req_lev);

//! dumps texture state (streaming and other) to debug
void dump_managed_tex_state(TEXTUREID tid);
