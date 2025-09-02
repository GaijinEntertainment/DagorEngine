//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabFwd.h>

// extern classes forward declaration
class BBox3;
class RenderScene;
class IGenLoad;
class D3DRESID;
typedef D3DRESID TEXTUREID;

//
// Dagor Binary Level Data loader client interface
//
class IBinaryDumpLoaderClient
{
public:
  virtual ~IBinaryDumpLoaderClient() {}
  virtual bool bdlGetPhysicalWorldBbox(BBox3 &bbox) = 0;


  virtual bool bdlConfirmLoading(unsigned bindump_id, int tag) = 0;

  virtual void bdlTextureMap(unsigned bindump_id, dag::ConstSpan<TEXTUREID> texId) = 0;

  virtual void bdlEnviLoaded(unsigned bindump_id, RenderScene *envi) = 0;
  virtual void bdlSceneLoaded(unsigned bindump_id, RenderScene *scene) = 0;
  virtual void bdlBinDumpLoaded(unsigned bindump_id) = 0;
  virtual void bdlBinDumpVerLoaded(unsigned /*bindump_id*/, const char * /*eVer*/, const char * /*target_fn*/) {}

  virtual bool bdlCustomLoad(unsigned bindump_id, int tag, IGenLoad &crd, dag::ConstSpan<TEXTUREID> texId) = 0;

  virtual const char *bdlGetBasePath() = 0;
};

//
// Loades binary level with guidance by client
//
class BinaryDump;

// full binary dump loading
BinaryDump *load_binary_dump(const char *fname, IBinaryDumpLoaderClient &client, unsigned bindump_id = -1);
// partial binary dump loading (using external texId)
BinaryDump *load_binary_dump_async(const char *fname, IBinaryDumpLoaderClient &client, unsigned bindump_id = -1);
// unload allocatec binary dump data
bool unload_binary_dump(BinaryDump *handle, bool delete_render_scenes);

//! unloads all binary dumps scheduled for unloading
void delayed_binary_dumps_unload();

//! disables/enables automatical call of ddsx::tex_pack2_perform_delayed_data_loading() after level binary is loaded (enabled by
//! default)
void disable_auto_load_tex_after_binary_dump_loaded(bool dis = true);

// sets base path for binary dumps
void set_binary_dump_base_path(const char *path);
