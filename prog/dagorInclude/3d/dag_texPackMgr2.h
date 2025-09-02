//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_texMgr.h>

namespace ddsx
{
enum
{
  TPRC_LOADED,
  TPRC_UNLOADED,
  TPRC_ADDREF,
  TPRC_DELREF,
  TPRC_NOTFOUND
};

//! loads (or increments ref count if already loaded) DDSx-Pack2 by name
bool load_tex_pack2(const char *pack_path, int *ret_code = NULL, bool can_override_tex = false);

//! decrements ref count (and unloads when ref count reaches zero) DDSx-Pack2 by name
bool release_tex_pack2(const char *pack_path, int *ret_code = NULL);

//! performs delayed texture data loading for requested textures; return true when anything loaded
bool tex_pack2_perform_delayed_data_loading(int prio = 0);

//! adds job to perform delayed texture data loading for requested textures;
void tex_pack2_perform_delayed_data_loading_async(int prio, int jobmgr_id, bool add_job_first, int max_job_in_flight = 2);

//! remotely cease loading inside tex_pack2_perform_delayed_data_loading() (e.g., that was called in another thread)
//! operation is rather safe since in restores rest of textures as pending and raises flag to load them later
bool cease_delayed_data_loading(int prio = 0);

//! intended for temporally stalling `reloadTex` thread within frame
void set_tex_pack_async_loading_allowed(bool on);

//! ceases loading inside tex_pack2_perform_delayed_data_loading and ensures that
//! no loading jobs are running or waiting (finishing active, cancelling queued)
//! returns true on success, false - otherwise (e.g. timeout if called from the main thread)
bool interrupt_texq_loading();
//! restores loading, should be called with the result of interrupt_texq_loading() in the first argument
void restore_texq_loading(bool was_interrupted);

struct DDSxDataPublicHdr // built on basis of ddsx::Header (engine/sharedInclude/3d/ddsxTex.h)
{
  unsigned d3dFormat;
  unsigned flags;
  unsigned short w, h, depth, bitsPerPixel;

  unsigned short tcType;
  unsigned char levels, hqPartLevels, dxtShift, lQmip, mQmip, uQmip;

  enum
  {
    FLG_HASBORDER = 0x0400,
    FLG_CUBTEX = 0x0800,
    FLG_VOLTEX = 0x1000,

    FLG_GENMIP_BOX = 0x2000,
    FLG_GENMIP_KAIZER = 0x4000,
    FLG_GAMMA_EQ_1 = 0x8000,

    FLG_NEED_PAIRED_BASETEX = 0x00020000,

    FLG_REV_MIP_ORDER = 0x00040000,
    FLG_HQ_PART = 0x00080000,
  };
};

//! reads and unpacks DDSx data into buffer
bool read_ddsx_contents(const char *tex_name, Tab<char> &out_data, DDSxDataPublicHdr &out_desc);

//! fills DDSx header for texture and returns memory size for it (or -1 when tex_name is not registered
int read_ddsx_header(const char *tex_name, DDSxDataPublicHdr &out_desc, bool full_quality = true, int tex_q = 0);

//! sets number of threads to be used to decode DDSx during load, def=0
//! wcnt <=0 means disabled
//! returns previous workers count
int set_decoder_workers_count(int wcnt);

//! streaming modes: boosted load with multi-threaded decoders, usual load with serial decoder
enum StreamingMode : int
{
  MultiDecoders,
  BackgroundSerial
};
//! setup streaming mode; returns previous state
StreamingMode set_streaming_mode(StreamingMode m);
//! returns current streaming mode
StreamingMode get_streaming_mode();

//! dumps textures distribution (genMip, diffTex, singleMip, splitTex, other)
void dump_registered_tex_distribution();

//! default (quasi-constant) priority used for HQ-part loading; =0 by default (means not streaming)
//! should be set at startup and remain constant for the rest of app life;
//! requires corresponding ddsx::tex_pack2_perform_delayed_data_loading(ddsx::hq_tex_priority) call somewhere
//! (for best results, call it from thread after main location load is complete)
extern int hq_tex_priority;

//! queue reloading of all currently referenced textures
void reload_active_textures(int prio = 0, bool interrupt = true);

//! reset explicit textures list that shall be loaded as low quality
void reset_lq_tex_list();

//! adds texture name to explicit list that shall be loaded as low quality
void add_texname_to_lq_tex_list(TEXTUREID texId);

//! reset explicit textures list that shall be loaded as medium quality (when global tex-quality >= medium)
void reset_mq_tex_list();

//! adds texture name to explicit list that shall be loaded as medium quality (when global tex-quality >= medium)
void add_texname_to_mq_tex_list(TEXTUREID texId);

//! initializes queue for separate DDSx loading
void init_ddsx_load_queue(int qsz);
//! releases queue for separate DDSx loading
void term_ddsx_load_queue();
//! enqueues loading of separate DDSx texture
void enqueue_ddsx_load(BaseTexture *t, TEXTUREID tid, const char *fn, int quality_id, TEXTUREID bt_tid = BAD_TEXTUREID);

//! process DDSx loading queue (also is called from tex_pack2_perform_delayed_data_loading() )
void process_ddsx_load_queue();

//! releases all data allocated for texture packs
void shutdown_tex_pack2_data();
} // namespace ddsx
