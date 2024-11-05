//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_genIo.h>
#include <debug/dag_except.h>
#include <generic/dag_tab.h>

#include <supp/dag_define_KRNLIMP.h>

/// @addtogroup utility_classes
/// @{

/// @addtogroup serialization
/// @{


/// @file
/// Serialization callbacks.


/// Base implementation of interface class for writing to abstract output stream.
///
/// Implements some useful methods, but leaves a few virtual ones for
/// real implementation of output (write(), tellPos(), seekto(), seektoend()).
///
/// SaveException is thrown on write error, so there are no error return codes for methods.
class IBaseSave : public IGenSave
{
public:
  KRNLIMP IBaseSave();
  IBaseSave(const IBaseSave &) = default;
  IBaseSave(IBaseSave &&) = default;
  IBaseSave &operator=(const IBaseSave &) = default;
  IBaseSave &operator=(IBaseSave &&) = default;
  KRNLIMP virtual ~IBaseSave();

  KRNLIMP virtual void beginBlock();
  KRNLIMP virtual void endBlock(unsigned block_flags_2bits = 0);
  KRNLIMP virtual int getBlockLevel();

protected:
  /// @cond
  struct Block
  {
    int ofs;
  };

  Tab<Block> blocks;
  /// @endcond
};


/// Base implementation of interface class for reading from abstract input stream.
///
/// Implements some useful methods, but leaves a few virtual ones for
/// real implementation of input.
///
/// LoadException is thrown on read error, so there are no error return codes for methods.
class IBaseLoad : public IGenLoad
{
public:
  KRNLIMP IBaseLoad();
  IBaseLoad(const IBaseLoad &) = default;
  IBaseLoad(IBaseLoad &&) = default;
  IBaseLoad &operator=(const IBaseLoad &) = default;
  IBaseLoad &operator=(IBaseLoad &&) = default;
  KRNLIMP virtual ~IBaseLoad();

  KRNLIMP virtual int beginBlock(unsigned *out_block_flags = nullptr);
  KRNLIMP virtual void endBlock();
  KRNLIMP virtual int getBlockLength();
  KRNLIMP virtual int getBlockRest();
  KRNLIMP virtual int getBlockLevel();

protected:
  /// @cond
  struct Block
  {
    int ofs, len;
  };

  Tab<Block> blocks;
  /// @endcond
};

/// @}

/// @}

#include <supp/dag_undef_KRNLIMP.h>
