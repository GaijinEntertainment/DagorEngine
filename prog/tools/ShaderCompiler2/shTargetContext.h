// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shCompContext.h"
#include "varMap.h"
#include "hashStrings.h"
#include "globVar.h"
#include "samplers.h"
#include "cppStcodeAssembly.h"
#include "cppStcodeUtils.h"
#include "shlexterm.h"
#include "shTargetStorage.h"
#include "shaderBytecodeCache.h"
#include "stcodeBytecode.h"
#include "boolVar.h"
#include "hlslStage.h"
#include "namedConst.h"
#include "shAssumes.h"
#include "globalConfig.h"
#include <EASTL/optional.h>


namespace shc
{

class ShaderContext;

class TargetContext
{
  const char *mFname;
  VarNameMap mVarNameMap{};
  HashStrings mIntervalNameMap{}; // @TODO: unify name map technology
  ShaderGlobal::VarTable mGvarTable;
  SamplerTable mSamplerTable;
  StcodeShader mCppstcodeAccum;
  BoolVarTable mBoolVarTable{};
  PerHlslStage<String> mGlobalHlslSource{};
  ShaderBlockTable mStateBlockTable{};
  ShaderAssumesTable mGlobalAssumes;
  SCFastNameMap mGlobalMessages{};
  ShaderTargetStorage mDataStorage{};
  ShaderBytecodeCache mShBytecodeCache{}; // @TODO: this lifetime conforms with previous impl. But maybe we want a persistent cache?
  StcodeBytecodeCache mStcodeBytecodeCache{};

  //...

  // Only valid when target is source file -> obj
  eastl::optional<SourceFileParseState> mSourceFileState{};

  const CompilationContext &mParent;

public:
  PINNED_TYPE(TargetContext)

  const CompilationContext &compCtx() const { return mParent; }

  const char *fname() const { return mFname; }

  VarNameMap &varNameMap() { return mVarNameMap; }
  const VarNameMap &varNameMap() const { return mVarNameMap; }

  HashStrings &intervalNameMap() { return mIntervalNameMap; }
  const HashStrings &intervalNameMap() const { return mIntervalNameMap; }

  ShaderGlobal::VarTable &globVars() { return mGvarTable; }
  const ShaderGlobal::VarTable &globVars() const { return mGvarTable; }

  SamplerTable &samplers() { return mSamplerTable; }
  const SamplerTable &samplers() const { return mSamplerTable; }

  StcodeShader &cppStcode() { return mCppstcodeAccum; }
  const StcodeShader &cppStcode() const { return mCppstcodeAccum; }

  BoolVarTable &globBoolVars() { return mBoolVarTable; }
  const BoolVarTable &globBoolVars() const { return mBoolVarTable; }

  auto &globHlslSrc() { return mGlobalHlslSource; }
  const auto &globHlslSrc() const { return mGlobalHlslSource; }

  ShaderBlockTable &blocks() { return mStateBlockTable; }
  const ShaderBlockTable &blocks() const { return mStateBlockTable; }

  ShaderAssumesTable &globAssumes() { return mGlobalAssumes; }
  const ShaderAssumesTable &globAssumes() const { return mGlobalAssumes; }

  SCFastNameMap &globMessages() { return mGlobalMessages; }
  const SCFastNameMap &globMessages() const { return mGlobalMessages; }

  ShaderTargetStorage &storage() { return mDataStorage; }
  const ShaderTargetStorage &storage() const { return mDataStorage; }

  ShaderBytecodeCache &bytecodeCache() { return mShBytecodeCache; }
  const ShaderBytecodeCache &bytecodeCache() const { return mShBytecodeCache; }

  StcodeBytecodeCache &stcodeCache() { return mStcodeBytecodeCache; }
  const StcodeBytecodeCache &stcodeCache() const { return mStcodeBytecodeCache; }

  SourceFileParseState &sourceParseState()
  {
    G_ASSERT(mSourceFileState);
    return *mSourceFileState;
  }
  const SourceFileParseState &sourceParseState() const
  {
    G_ASSERT(mSourceFileState);
    return *mSourceFileState;
  }

  class SourceFileScope
  {
    TargetContext &master;

  public:
    explicit SourceFileScope(TargetContext &master) : master{master} { master.mSourceFileState.emplace(master.mFname, master); }
    ~SourceFileScope() { master.mSourceFileState.reset(); }

    PINNED_TYPE(SourceFileScope)
  };

  SourceFileScope initSourceFile() { return SourceFileScope{*this}; }

  ShaderContext makeShaderContext(const char *shname, ShaderBlockLevel block_level, Terminal *shname_term = nullptr);

private:
  friend class CompilationContext;

  explicit TargetContext(const char *fname, const CompilationContext &parent) :
    mFname{fname},
    mGvarTable{mVarNameMap, mIntervalNameMap},
    mSamplerTable{mVarNameMap, mGvarTable, mCppstcodeAccum},
    mGlobalAssumes{*shc::config().assumedVarsConfig},
    mParent{parent}
  {
    if (fname)
      mCppstcodeAccum.shaderName = stcode::extract_shader_name_from_path(fname);
  }
};

inline TargetContext CompilationContext::makeTargetContext(const char *fname) const { return TargetContext{fname, *this}; }

} // namespace shc
