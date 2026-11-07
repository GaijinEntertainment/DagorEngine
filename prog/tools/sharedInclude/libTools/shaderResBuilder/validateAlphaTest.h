// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/functional.h>
#include <generic/dag_tab.h>

class DataBlock;
class Mesh;
class MaterialData;
class ShaderMaterial;
class ILogWriter;
class DagorAssetMgr;
class TexImage32;
class IMemAlloc;

namespace AlphaTestValidation
{
using AssetMgrProvider = eastl::function<DagorAssetMgr *()>;
using ImageLoader = eastl::function<TexImage32 *(const char *, IMemAlloc *)>;

enum class BakeResult
{
  Error,
  Cutout,
  FullyOpaque,
  FullyTransparent,
  Mixed,
};

struct Context;

Context *init(const DataBlock &blk, AssetMgrProvider provider, ImageLoader loader);
void shutdown(Context *ctx);
void validate(Context *ctx, const Mesh &mesh, int mat_id, const MaterialData &mat, const ShaderMaterial &sh_mat, const char *dag_fn,
  const char *node_name, ILogWriter &log);
// appends to out_faces
BakeResult retrieve_invisible_faces(Context *ctx, const Mesh &mesh, int mat_id, const MaterialData &mat, const ShaderMaterial &sh_mat,
  Tab<int> &out_faces);
}; // namespace AlphaTestValidation
