// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shTargetContext.h"
#include "boolVar.h"
#include "shaderVariantSrc.h"
#include "shAssumes.h"
#include "shcode.h"
#include "shsem.h"
#include "shMessages.h"
#include "shaderVariant.h"
#include "shSemCode.h"


namespace shc
{

class VariantContext;

struct TypeTables
{
  ShaderVariant::TypeTable allStaticTypes;
  ShaderVariant::TypeTable allDynamicTypes;
  ShaderVariant::TypeTable referencedTypes;

  explicit TypeTables(shc::TargetContext &ctx) : allStaticTypes{ctx}, allDynamicTypes{ctx}, referencedTypes{ctx} {}
};

class ShaderContext
{
  const char *mName;
  const ShaderBlockLevel mBlockLevel;
  Terminal *mDeclTerm;
  eastl::unique_ptr<ShaderClass> mShaderClass;
  TypeTables mTypes;
  IntervalList mIntervals{};
  BoolVarTable mBoolVarTable{};
  PerHlslStage<String> mLocalHlslSource{};
  ShaderAssumesTable mLocalAssumes;
  ShaderMessages mMessages{};
  bool mDebugModeEnabled = false;

  TargetContext &mParent;
  const CompilationContext &mCompParent;

public:
  PINNED_TYPE(ShaderContext)

  TargetContext &tgtCtx() { return mParent; }
  const TargetContext &tgtCtx() const { return mParent; }
  const CompilationContext &compCtx() const { return mCompParent; }

  const char *name() const { return mName; }
  ShaderBlockLevel blockLevel() const { return mBlockLevel; }

  Terminal *declTerm() { return mDeclTerm; }
  const Terminal *declTerm() const { return mDeclTerm; }

  ShaderClass &compiledShader() { return *mShaderClass; }
  const ShaderClass &compiledShader() const { return *mShaderClass; }

  ShaderClass *releaseCompiledShader() { return mShaderClass.release(); }

  TypeTables &typeTables() { return mTypes; }
  const TypeTables &typeTables() const { return mTypes; }

  IntervalList &intervals() { return mIntervals; }
  const IntervalList &intervals() const { return mIntervals; }

  BoolVarTable &localBoolVars() { return mBoolVarTable; }
  const BoolVarTable &localBoolVars() const { return mBoolVarTable; }

  auto &localHlslSrc() { return mLocalHlslSource; }
  const auto &localHlslSrc() const { return mLocalHlslSource; }

  ShaderAssumesTable &assumes() { return mLocalAssumes; }
  const ShaderAssumesTable &assumes() const { return mLocalAssumes; }

  ShaderMessages &messages() { return mMessages; }
  const ShaderMessages &messages() const { return mMessages; }

  bool isDebugModeEnabled() const { return mDebugModeEnabled; }
  void setDebugModeEnabled(bool val) { mDebugModeEnabled = val; }

  VariantContext makeVariantContext(ShaderVariant::VariantInfo variant, ShaderSemCode &target_code,
    ShaderSemCode::PassTab *target_pass = nullptr, bool use_branches_for_cppstcode = false);

private:
  friend class TargetContext;

  // @TODO: include option to not initialize shader-only systems (for blocks)

  explicit ShaderContext(const char *shname, ShaderBlockLevel block_level, Terminal *shname_term, TargetContext &parent,
    const CompilationContext &comp_parent) :
    mName{shname},
    mBlockLevel{block_level},
    mDeclTerm{shname_term},
    mShaderClass{new ShaderClass{shname}},
    mTypes{parent}, // @TODO: a more precise reference to a needed resource?
    mBoolVarTable{parent.globBoolVars().maxId()},
    mLocalHlslSource{parent.globHlslSrc()},
    mLocalAssumes{*shc::config().assumedVarsConfig->getBlockByNameEx(shname), &parent.globAssumes(), shname},
    mParent{parent},
    mCompParent{comp_parent}
  {
    mTypes.allStaticTypes.setIntervalList(&mIntervals);
    mTypes.allDynamicTypes.setIntervalList(&mIntervals);
    mTypes.referencedTypes.setIntervalList(&mIntervals);
  }
};

inline ShaderContext TargetContext::makeShaderContext(const char *shname, ShaderBlockLevel block_level, Terminal *shname_term)
{
  return ShaderContext{shname, block_level, shname_term, *this, this->compCtx()};
}

} // namespace shc
