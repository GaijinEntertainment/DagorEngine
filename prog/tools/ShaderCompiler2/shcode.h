// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_globDef.h>
#include <util/dag_stdint.h>
#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>
#include <math/dag_color.h>
#include <math/integer/dag_IPoint4.h>
#include <EASTL/bitvector.h>

#include "shaderSave.h"
#include "shaderVariant.h"
#include "varMap.h"
#include "namedConst.h"
#include <shaders/dag_shaderCommon.h>
#include "shVarVecTypes.h"

#define codemem midmem

class SerializableSlice
{
  bindump::Address<int> mPtr;
  uint32_t mCount;

public:
  void set(int *p, intptr_t n)
  {
    mPtr = p;
    mCount = n;
  }
  void reset()
  {
    mPtr = nullptr;
    mCount = 0;
  }
  const int *data() const { return mPtr; }
  intptr_t size() const { return mCount; }

  const int &operator[](int idx) const { return mPtr.get()[idx]; }
  int &operator[](int idx) { return mPtr.get()[idx]; }
};

class ShaderCode
{
public:
  DAG_DECLARE_NEW(codemem)

  ShaderVariant::VariantTable dynVariants;

  SerializableTab<ShaderChannelId> channel;

  int varsize, regsize;

  SerializableTab<int> initcode;
  SerializableTab<ShaderVarTextureType> staticTextureTypes;

  struct StVarMap
  {
    int v, sv;
  };
  SerializableTab<StVarMap> stvarmap;

  int flags;

  unsigned int getVertexStride() const;

  struct Pass
  {
    int32_t fsh = -1;
    int32_t vprog = -1;
    int stcodeNo = -1;
    int stblkcodeNo = -1;
    int renderStateNo = -1;
    eastl::array<uint16_t, 3> threadGroupSizes = {};
    bool scarlettWave32 = false;
  };

  // for each variant create it's own passes
  struct PassTab
  {
    bindump::Address<Pass> rpass;
    SerializableTab<bindump::Address<ShaderStateBlock>> suppBlk;

    PassTab() : suppBlk(codemem) {}

    // trasformation between Index and Pointer modes
    void toPtr(dag::Span<Pass> all_pass) { rpass = (intptr_t)rpass.get() == -1 ? NULL : &all_pass[(intptr_t)rpass.get()]; }
  };

  SerializableTab<Pass> allPasses;
  SerializableTab<PassTab *> passes;

  // create if needed passes for dynamic variant #n
  PassTab *createPasses(int variant);

  ShaderCode(IMemAlloc *mem = codemem);

  ~ShaderCode()
  {
    for (int i = 0; i < passes.size(); i++)
      del_it(passes[i]);
    clear_and_shrink(passes);
  }

  void link();
};

class ShaderClass
{
public:
  DAG_DECLARE_NEW(codemem)

  ShaderVariant::VariantTable staticVariants;

  SerializableTab<ShaderCode *> code;

  union StVarValue
  {
    shc::Col4 c4;
    shc::Int4 i4;
    real r;
    int i;
    unsigned texId;
    d3d::SamplerInfo samplerInfo;
    StVarValue() : i4{0, 0, 0, 0} {}
  };

  class Var
  {
  public:
    NameId<VarMapAdapter> nameId;
    ShaderVarType type;
    StVarValue defval;
    Var() { memset(&defval, 0, sizeof(defval)); }

    inline const char *getName() const { return VarMap::getName(nameId); }
  };
  SerializableTab<Var> stvar;
  SerializableTab<int> shInitCode;
  BINDUMP_NON_SERIALIZABLE(eastl::bitvector<> stvarsAreDynamic);

  bindump::string name;
  int64_t timestamp = -1;
  bool isSelectedForDebug;

  bindump::vector<bindump::string> messages;
  BINDUMP_NON_SERIALIZABLE(SCFastNameMap uniqueStrings);

  struct AssumedIntervalInfo
  {
    bindump::string name;
    uint8_t value = 0;
  };
  bindump::vector<AssumedIntervalInfo> assumedIntervals;

  ShaderClass(const char *n = "") : code(codemem), stvar(codemem), name(n), shInitCode(midmem), isSelectedForDebug(false) {}

  ~ShaderClass()
  {
    for (int i = 0; i < code.size(); i++)
      del_it(code[i]);
    clear_and_shrink(code);
  }

  // First all static, then all dynamic to reduce posibility of material and shader layout mismatches
  // After this function, stvarsAreDynamic field is invalidated, so it should be done after all var additions
  void sortStaticVarsByMode();

  int find_static_var(const int variable_name_id);
};

#undef codemem
