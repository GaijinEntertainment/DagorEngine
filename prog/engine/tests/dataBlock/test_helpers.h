// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>
#include <EASTL/vector.h>
#include <catch2/catch_tostring.hpp>
#include <string>

template <>
struct Catch::StringMaker<DataBlock>
{
  static std::string convert(const DataBlock &blk)
  {
    return std::string("DataBlock{params=") + std::to_string(blk.paramCount()) + ", blocks=" + std::to_string(blk.blockCount()) + "}";
  }
};

template <typename E>
struct Catch::StringMaker<BitFlagsMask<E>>
{
  static std::string convert(const BitFlagsMask<E> &flags) { return std::string("0x") + std::to_string(flags.asInteger()); }
};

struct ErrorEntry
{
  String text;
  bool serious;
};

struct ErrorCollector : DataBlock::IErrorReporterPipe
{
  eastl::vector<ErrorEntry> errors;

  void reportError(const char *error_text, bool serious_err) override { errors.push_back({String(error_text), serious_err}); }

  void clear() { errors.clear(); }
  int count() const { return (int)errors.size(); }
  bool empty() const { return errors.empty(); }

  bool hadSerious() const
  {
    for (auto &e : errors)
      if (e.serious)
        return true;
    return false;
  }

  bool hadNonSerious() const
  {
    for (auto &e : errors)
      if (!e.serious)
        return true;
    return false;
  }
};

struct FatalFlagsGuard
{
  bool savedMissingFile;
  bool savedLoadFailed;
  bool savedBadVarType;
  bool savedMissingVar;
  bool savedStrongTypeChecking;
  bool savedAllowVarTypeChange;

  FatalFlagsGuard()
  {
    savedMissingFile = DataBlock::fatalOnMissingFile;
    savedLoadFailed = DataBlock::fatalOnLoadFailed;
    savedBadVarType = DataBlock::fatalOnBadVarType;
    savedMissingVar = DataBlock::fatalOnMissingVar;
    savedStrongTypeChecking = DataBlock::strongTypeChecking;
    savedAllowVarTypeChange = DataBlock::allowVarTypeChange;
  }

  ~FatalFlagsGuard()
  {
    DataBlock::fatalOnMissingFile = savedMissingFile;
    DataBlock::fatalOnLoadFailed = savedLoadFailed;
    DataBlock::fatalOnBadVarType = savedBadVarType;
    DataBlock::fatalOnMissingVar = savedMissingVar;
    DataBlock::strongTypeChecking = savedStrongTypeChecking;
    DataBlock::allowVarTypeChange = savedAllowVarTypeChange;
  }

  void setAllFatal()
  {
    DataBlock::fatalOnMissingFile = true;
    DataBlock::fatalOnLoadFailed = true;
    DataBlock::fatalOnBadVarType = true;
    DataBlock::fatalOnMissingVar = true;
  }

  void setAllNonFatal()
  {
    DataBlock::fatalOnMissingFile = false;
    DataBlock::fatalOnLoadFailed = false;
    DataBlock::fatalOnBadVarType = false;
    DataBlock::fatalOnMissingVar = false;
  }
};
