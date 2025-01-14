// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  scripted shader material class
************************************************************************/
#ifndef __SCRIPTSMAT_H
#define __SCRIPTSMAT_H

#include "shadersBinaryData.h"
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderMatData.h>
#include <3d/dag_materialData.h>
#include <generic/dag_ptrTab.h>
#include <generic/dag_smallTab.h>

/************************************************************************
   forwards
************************************************************************/
class ScriptedShaderElement;


struct ShaderMaterialProperties
{
  D3dMaterial d3dm;
  int sclassNameOfs;
  PATCHABLE_DATA64(const shaderbindump::ShaderClass *, sclass);

  TEXTUREID textureId[MAXMATTEXNUM];

  PatchableTab<ShaderMatData::VarValue> stvar;

  uint32_t legacy; // to be removed, unfortunately still

  int16_t matflags;
  int8_t legacy2;

private:
  bool secondDump = false;

public:
  ShaderMaterialProperties() {}

  static ShaderMaterialProperties *create(const shaderbindump::ShaderClass *sc, const MaterialData &m, bool sec_dump_for_exp = false);

  void patchNonSharedData(void *base, dag::Span<TEXTUREID> texMap);
  void recreateMat();

  const String getInfo() const;

  bool isEqual(const ShaderMaterialProperties &p) const;
  bool isEqualWithoutTex(const ShaderMaterialProperties &p) const;

  int execInitCode();

  __forceinline const ScriptedShadersBinDump &shBinDump() const { return ::shBinDumpEx(!secondDump); }
  __forceinline const ScriptedShadersBinDumpOwner &shBinDumpOwner() const { return ::shBinDumpExOwner(!secondDump); }

protected:
  ShaderMaterialProperties(const shaderbindump::ShaderClass *sc, const MaterialData &m, bool sec_dump_for_exp);
};


/*********************************
 *
 * class ScriptedShaderMaterial
 *
 *********************************/
class ScriptedShaderMaterial final : public ShaderMaterial
{
public:
  DAG_DECLARE_NEW(midmem)

  ShaderMaterialProperties props;

public:
  static ScriptedShaderMaterial *create(const ShaderMaterialProperties &p, bool clone_mat = false);

  ShaderMaterial *clone() const override;
  void recreateMat(bool delete_programs = true);

  bool set_int_param(const int variable_id, const int v) override;
  bool set_real_param(const int variable_id, const real v) override;
  bool set_color4_param(const int variable_id, const Color4 &val) override;
  bool set_texture_param(const int variable_id, const TEXTUREID v) override;
  bool set_sampler_param(const int variable_id, d3d::SamplerHandle v) override;

  bool hasVariable(const int variable_id) const override;
  bool getColor4Variable(const int variable_id, Color4 &value) const override;
  bool getRealVariable(const int variable_id, real &value) const override;
  bool getIntVariable(const int variable_id, int &value) const override;
  bool getTextureVariable(const int variable_id, TEXTUREID &value) const override;
  bool getSamplerVariable(const int variable_id, d3d::SamplerHandle &value) const override;

  int get_flags() const override { return props.matflags; };

  void set_flags(int value, int mask);

  const shaderbindump::ShaderCode *find_variant() const;

  bool enum_channels(ShaderChannelsEnumCB &cb, int &ret_code_flags) const override;

  bool isPositionPacked() const override;

  ShaderElement *make_elem(bool acq_tex_refs, const char *info) override;

  // return true, if channels are valid for this material & specified render mode
  bool checkChannels(CompiledShaderChannelId *ch, int ch_count) const override;

  Color4 get_color4_stvar(int i) const;
  real get_real_stvar(int i) const;
  int get_int_stvar(int i) const;
  TEXTUREID get_tex_stvar(int i) const;
  d3d::SamplerHandle get_sampler_stvar(int i) const;

  /*virtual*/ inline int get_num_textures() const { return MAXMATTEXNUM; };

  TEXTUREID get_texture(int i) const override;
  void set_texture(int, TEXTUREID) override;
  void gatherUsedTex(TextureIdSet &tex_id_list) const override;
  bool replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new) override;

  void getMatData(ShaderMatData &out_data) const override;
  void buildMaterialData(MaterialData &out_data, const char *orig_mat_script = nullptr) override;

  // get shader script name
  const char *getShaderClassName() const override { return props.sclass ? (const char *)props.sclass->name : "<sclass==null>"; }

  bool isSelectedForDebug() const override
  {
#if DAGOR_DBGLEVEL > 0
    return props.sclass == shaderbindump::shClassUnderDebug;
#else
    return false;
#endif
  }

  const String getInfo() const override { return props.getInfo(); }

  ScriptedShaderElement *getElem() const { return varElem.get(); }

  void updateStVar(int prop_stvar_id);

  void setNonSharedRefCount(int rc) { nonSharedRefCount = rc; }
  bool isNonShared() const { return ref_count <= nonSharedRefCount; }

private:
  Ptr<ScriptedShaderElement> varElem;
  int nonSharedRefCount;

  ScriptedShaderMaterial(const ShaderMaterialProperties &p);
  ~ScriptedShaderMaterial();

  // to prevent accidental copying
  ScriptedShaderMaterial(const ScriptedShaderMaterial &s);
  ScriptedShaderMaterial &operator=(const ScriptedShaderMaterial &s);

  friend bool shader_mats_are_equal(ShaderMaterial *m1, ShaderMaterial *m2);
  __forceinline const ScriptedShadersBinDump &shBinDump() const { return props.shBinDump(); }
};


#endif //__SCRIPTSMAT_H
