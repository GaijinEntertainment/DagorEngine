// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tabFwd.h>
#include <libTools/util/iLogWriter.h>
#include <assets/assetExpCache.h>
#include <assets/assetRefs.h>
#include <assets/daBuildExpPluginChain.h>

struct IProcessMaterialData;
class IDagorAssetExporter;
class DagorAsset;
class DagorAssetMgr;
class DataBlock;
class SimpleString;

namespace AlphaTestValidation
{
struct Context;
}


BEGIN_DABUILD_PLUGIN_NAMESPACE(modelExp)
extern IDagorAssetExporter *create_rendinst_exporter();
extern IDagorAssetExporter *create_dynmodel_exporter();
extern IDagorAssetExporter *create_skeleton_exporter();
extern IDagorAssetExporter *create_rndgrass_exporter();
extern IDagorAssetRefProvider *create_rendinst_ref_provider();
extern IDagorAssetRefProvider *create_dynmodel_ref_provider();
extern IDagorAssetRefProvider *create_rndgrass_ref_provider();

extern DataBlock appBlkCopy;
extern AlphaTestValidation::Context *atest_validation_ctx;

extern DagorAsset *cur_asset;
extern ILogWriter *cur_log;

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

// Appends the texture and proxyMat references of one .dag file.
// Returns false when the references could not be gathered, the .dag file could not be read or the
// material processing failed.
bool add_dag_texture_and_proxymat_refs(const char *dag_fn, Tab<IDagorAssetRefProvider::Ref> &tmpRefs, DagorAsset &a,
  IProcessMaterialData *pm = nullptr);

// Gather the LOD .dag paths of a model asset, in the order defined by DagorAsset::props.
// files: output parameter. The path of the .dag files will be appended to it.
void gather_model_lod_dags(const DagorAsset &a, Tab<SimpleString> &files);

String validate_texture_types(const char *tex_name, const char *class_name, int slot, DagorAsset &a);

// Make sure to always call setup_tex_subst() and reset_tex_subst() together.
void setup_tex_subst(const DataBlock &a_props);
void reset_tex_subst();

void add_shdump_deps(Tab<SimpleString> &files);

// Get the hash of the shader dump that the reference gathering uses.
// The gathered reference list depends on the compiled shader dump. With a material processor active,
// processMaterial() -> validate_texture_types() reads the shader dump to decide a texture's slot type.
// That is the only shader read of the gathering, and it always loads the PC dump.
// A dump is loaded once per process, so a file replaced while the process lives does not change what the
// gathering reads, and must not change the cache key either.
// may_process: mayProcess() of the processor the caller gathers the references with.
// out_hash: storage for out_hash_span. It is owned by the caller.
// out_hash_span: output parameter. It is empty when the gathering does not read the shader dump.
// Returns false when the dump is not loaded yet and its file cannot be read. The reference cache must not
// be used then, because its key would not describe the shader dump that the gathering reads.
bool get_shdump_id_for_ref_gather(bool may_process, uint8_t out_hash[AssetExportCache::HASH_SZ],
  dag::ConstSpan<uint8_t> &out_hash_span);

void load_shaders_for_target(unsigned tc);

const DataBlock &get_process_mat_blk(const DataBlock &a_props, const char *asset_type);

bool add_proxymat_dep_files(const char *dag_fn, Tab<SimpleString> &files, DagorAssetMgr &mgr);

END_DABUILD_PLUGIN_NAMESPACE(modelExp)
USING_DABUILD_PLUGIN_NAMESPACE(modelExp)
