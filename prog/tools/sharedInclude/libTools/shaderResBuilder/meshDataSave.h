// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shaderCommon.h>
#include <shaders/dag_shaders.h>
#include <generic/dag_tab.h>
#include <generic/dag_ptrTab.h>
#include <util/dag_string.h>

class ShaderElement;
class ShaderMaterial;
class GlobalVertexDataSrc;

namespace mkbindump
{
class BinDumpSaveCB;
}


class ShaderMaterialSaveCB
{
public:
  virtual int gettexindex(TEXTUREID) = 0;
};

class ShaderMeshDataSaveCB
{
public:
  // get material index
  virtual int getmatindex(ShaderMaterial *m) = 0;

  // add global vertex data
  virtual void addGlobVData(GlobalVertexDataSrc *v) = 0;

  // get global vertex data index
  virtual int getGlobVData(GlobalVertexDataSrc *v) = 0;
};

// Helper class that saves textures.
class ShaderTexturesSaver : public ShaderMaterialSaveCB
{
public:
  Tab<TEXTUREID> textures;
  Tab<String> texNames;

  ShaderTexturesSaver();

  int addTexture(TEXTUREID);

  void prepareTexNames(dag::ConstSpan<String> tex_names);

  void writeTexStr(mkbindump::BinDumpSaveCB &cwr);
  void writeTexIdx(mkbindump::BinDumpSaveCB &cwr, dag::Span<TEXTUREID> map);
  void writeTexToBlk(DataBlock &dest);

  int gettexindex(TEXTUREID);
};

// Helper class that saves shader materials.
class ShaderMaterialsSaver : public ShaderMeshDataSaveCB
{
public:
  struct VdataHdr
  {
    uint32_t vertNum, vertStride : 8, packedIdxSizeLo : 24, idxSize : 28, packedIdxSizeHi : 4, flags;
  };
  struct MatData
  {
    Ptr<ShaderMaterial> mat;
    String par;
  };

  Tab<MatData> mats;
  PtrTab<GlobalVertexDataSrc> vdata;

  ShaderMaterialsSaver();

  int addMaterial(ShaderMaterial *, MaterialData *src);

  // Add material and its textures, if it's new.
  int addMaterial(ShaderMaterial *, ShaderTexturesSaver &, MaterialData *src);

  void writeMatVdataHdr(mkbindump::BinDumpSaveCB &, ShaderTexturesSaver &tex);
  void writeMatVdata(mkbindump::BinDumpSaveCB &, ShaderTexturesSaver &tex);

  void writeMatVdataCombinedNoMat(mkbindump::BinDumpSaveCB &);
  void writeMatToBlk(DataBlock &dest, ShaderTexturesSaver &texSaver, const char *mat_block_name);

  int getmatindex(ShaderMaterial *);

  // add global vertex data
  virtual void addGlobVData(GlobalVertexDataSrc *v);

  // get global vertex data index
  virtual int getGlobVData(GlobalVertexDataSrc *v);

protected:
  int hdrPos;
};
