// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "shShaderContext.h"
#include "shSemCode.h"
#include "boolVar.h"
#include "shLocVar.h"
#include <memory/dag_regionMemAlloc.h>


namespace shc
{

class VariantContext
{
  const ShaderVariant::VariantInfo mVariant;
  ShaderSemCode &mParsedSemcodeRef;
  ShaderSemCode::PassTab *mParsedPassRef;
  NamedConstBlock mNamedConstsTable{};
  BoolVarTable mBoolVarTable;
  LocalVarTable mLocVars;
  StcodeBytecodeAccumulator mStcodeBytecode;
  StcodePass mCppStcode{};

  ShaderContext &mParent;
  TargetContext &mTgtParent;
  const CompilationContext &mCompParent;

public:
  PINNED_TYPE(VariantContext)

  ShaderContext &shCtx() { return mParent; }
  const ShaderContext &shCtx() const { return mParent; }
  TargetContext &tgtCtx() { return mTgtParent; }
  const TargetContext &tgtCtx() const { return mTgtParent; }
  const CompilationContext &compCtx() const { return mCompParent; }

  const ShaderVariant::VariantInfo &variant() const { return mVariant; }

  ShaderSemCode &parsedSemCode() { return mParsedSemcodeRef; }
  const ShaderSemCode &parsedSemCode() const { return mParsedSemcodeRef; }

  bool hasParsedPass() const { return mParsedPassRef != nullptr; }

  auto &parsedPass() { return *mParsedPassRef; }
  const auto &parsedPass() const { return *mParsedPassRef; }

  NamedConstBlock &namedConstTable() { return mNamedConstsTable; }
  const NamedConstBlock &namedConstTable() const { return mNamedConstsTable; }

  auto &stBytecode() { return mStcodeBytecode; }
  const auto &stBytecode() const { return mStcodeBytecode; }
  auto &cppStcode() { return mCppStcode; }
  const auto &cppStcode() const { return mCppStcode; }

  // @TODO: bool vars should be remade to adequate semantics so that we don't have them separately __per_every__ variant, but this is
  // for feature parity.
  BoolVarTable &localBoolVars() { return mBoolVarTable; }
  const BoolVarTable &localBoolVars() const { return mBoolVarTable; }

  LocalVarTable &localStcodeVars() { return mLocVars; }
  const LocalVarTable &localStcodeVars() const { return mLocVars; }

private:
  friend class ShaderContext;

  VariantContext(ShaderVariant::VariantInfo variant, ShaderSemCode &parsed_code, ShaderSemCode::PassTab *parsed_pass,
    bool use_branches_for_cppstcode, ShaderContext &parent, TargetContext &tgt_parent, const CompilationContext &comp_parent) :
    mVariant{variant},
    mParsedSemcodeRef{parsed_code},
    mParsedPassRef{parsed_pass},
    mBoolVarTable{tgt_parent.globBoolVars().maxId()},
    mLocVars{tgt_parent},
    mStcodeBytecode{tgt_parent.sourceParseState().parser},
    mCppStcode{use_branches_for_cppstcode},
    mParent{parent},
    mTgtParent{tgt_parent},
    mCompParent{comp_parent}
  {}
};

inline VariantContext ShaderContext::makeVariantContext(ShaderVariant::VariantInfo variant, ShaderSemCode &target_code,
  ShaderSemCode::PassTab *target_pass, bool use_branches_for_cppstcode)
{
  return VariantContext{variant, target_code, target_pass, use_branches_for_cppstcode, *this, this->tgtCtx(), this->compCtx()};
}

} // namespace shc
