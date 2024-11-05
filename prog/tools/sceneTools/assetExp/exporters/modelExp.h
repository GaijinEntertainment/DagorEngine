// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tabFwd.h>
#include <libTools/util/iLogWriter.h>
#include <assets/assetRefs.h>
#include <assets/daBuildExpPluginChain.h>

struct IProcessMaterialData;
class IDagorAssetExporter;
class DagorAsset;
class DagorAssetMgr;
class DataBlock;
class SimpleString;


BEGIN_DABUILD_PLUGIN_NAMESPACE(modelExp)
extern IDagorAssetExporter *create_rendinst_exporter();
extern IDagorAssetExporter *create_dynmodel_exporter();
extern IDagorAssetExporter *create_skeleton_exporter();
extern IDagorAssetExporter *create_rndgrass_exporter();
extern IDagorAssetRefProvider *create_rendinst_ref_provider();
extern IDagorAssetRefProvider *create_dynmodel_ref_provider();
extern IDagorAssetRefProvider *create_rndgrass_ref_provider();

extern DataBlock appBlkCopy;

extern DagorAsset *cur_asset;
extern ILogWriter *cur_log;
extern Tab<IDagorAssetRefProvider::Ref> tmp_refs;

inline void set_context(DagorAsset &a, ILogWriter &l)
{
  cur_asset = &a;
  cur_log = &l;
}
inline void reset_context()
{
  cur_asset = NULL;
  cur_log = NULL;
}

struct AutoContext
{
  AutoContext(DagorAsset &a, ILogWriter &l) { set_context(a, l); }
  ~AutoContext() { reset_context(); }
};

void add_dag_texture_and_proxymat_refs(const char *dag_fn, Tab<IDagorAssetRefProvider::Ref> &tmpRefs, DagorAsset &a,
  IProcessMaterialData *pm = nullptr);
String validate_texture_types(const char *tex_name, const char *class_name, int slot, DagorAsset &a);

void setup_tex_subst(const DataBlock &a_props);
void reset_tex_subst();

void add_shdump_deps(Tab<SimpleString> &files);
void load_shaders_for_target(unsigned tc);

const DataBlock &get_process_mat_blk(const DataBlock &a_props, const char *asset_type);

bool add_proxymat_dep_files(const char *dag_fn, Tab<SimpleString> &files, DagorAssetMgr &mgr);

END_DABUILD_PLUGIN_NAMESPACE(modelExp)
USING_DABUILD_PLUGIN_NAMESPACE(modelExp)
