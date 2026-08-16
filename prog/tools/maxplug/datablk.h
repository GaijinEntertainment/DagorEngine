// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <cassert>
#include <filesystem>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <fstream>
#include <memory>
#include <optional>
#include <functional>

#include <max.h>
#include "math3d.h"
#include "e3dcolor.h"
#include "namemap.h"

/// DataBlock class for reading and writing hierarchically structured data.

/// Class for reading and writing hierarchically structured data.
///
/// DataBlock itself is a node of the tree that has a name and hosts typified
/// named parameters and named sub-nodes.
///
/// For clarity, names are restricted to C indentifier rules.
///
/// Actual names are stored in NameMap that is shared by all DataBlocks in the tree.
/// Blocks and parameters use integer ids to address names in the NameMap, so
/// there are methods that take name ids and those that take character strings.
/// You can use name ids when you look for blocks or parameters for performance gain.
///
/// DataBlock tree contents can be serialized in text form.
///
/// Text files of this format usually have extension ".blk".
class DataBlock
{
public:
  enum class ParamType
  {
    TYPE_NONE,
    TYPE_STRING,   ///< Text string.
    TYPE_INT,      ///< Integer.
    TYPE_REAL,     ///< #real (float).
    TYPE_POINT2,   ///< Point2.
    TYPE_POINT3,   ///< Point3.
    TYPE_POINT4,   ///< Point4.
    TYPE_IPOINT2,  ///< IPoint2.
    TYPE_IPOINT3,  ///< IPoint3.
    TYPE_BOOL,     ///< Boolean.
    TYPE_E3DCOLOR, ///< E3DCOLOR.
    TYPE_MATRIX,   ///< TMatrix.
  };

  using Param = std::variant<std::string, int, real, Point2, Point3, Point4, IPoint2, IPoint3, bool, E3DCOLOR, TMatrix>;

  static ParamType deserialize_param_type(std::string_view s);

  DataBlock(std::shared_ptr<NameMap> nameMap);
  ~DataBlock() = default;

  DataBlock(const DataBlock &) = delete;
  DataBlock &operator=(const DataBlock &) = delete;

  /// Delete all sub-blocks.
  void clearData();

  /// Reset DataBlock object (clear all data & names).
  void reset();

  /// Load DataBlock tree from specified text.
  /// Filename is for error output only.
  bool loadText(const char *text, int text_length, const char *filename = NULL);

  /// Load DataBlock tree from specified text.
  /// Filename is for error output only.
  /// @note This method will modify @b text when including files.
  bool loadText(std::string &text, const char *filename = NULL);

  /// Load DataBlock tree from arbitrary stream
  /// Data may be presented like text, binary or stream data
  /// created by function beginTaggedBlock(_MAKE4C('blk'))
  /// fname uses if loading from text file to right parse include directives
  bool loadFromStream(std::ifstream &is, const char *fname = NULL);

  /// Load DataBlock tree from a text file
  bool load(const std::filesystem::path &fname);

  /// Save this DataBlock (and its sub-tree) to the specified file (text form)
  bool saveToTextFile(const std::filesystem::path &filename) const;

  /// Returns name id from NameMap, or -1 if there's no such name in the NameMap.
  int getNameId(const char *name) const;

  /// Returns name by name id, uses NameMap.
  /// Returns NULL if name id is not valid.
  const char *getName(int name_id) const;

  /// Returns name id of this DataBlock.
  int getBlockNameId() const { return nameId; }

  /// Returns name of this DataBlock.
  const char *getBlockName() const { return getName(nameId); }

  /// Returns number of sub-blocks in this DataBlock.
  /// Use for enumeration.
  int blockCount() const { return int(blocks.size()); }

  /// Returns pointer to i-th sub-block.
  DataBlock *getBlock(int block_number) const;

  /// Returns pointer to sub-block with specified name id, or NULL if not found.
  DataBlock *getBlockByName(int name_id, int start_after = -1) const;

  /// Returns pointer to sub-block with specified name, or NULL if not found.
  DataBlock *getBlockByName(const char *name, int start_after = -1) const { return getBlockByName(getNameId(name), start_after); }

  /// Returns number of parameters in this DataBlock.
  /// Use for enumeration.
  int paramCount() const { return int(params.size()); }

  /// Returns type of i-th parameter. See ParamType enum.
  ParamType getParamType(int param_number) const;

  /// Returns i-th parameter name id. See getNameId().
  int getParamNameId(int param_number) const;

  /// Returns i-th parameter name. Uses getName().
  const char *getParamName(int param_number) const { return getName(getParamNameId(param_number)); }

  /// Find parameter by name id.
  /// Returns parameter index or -1 if not found.
  int findParam(int name_id, int start_after = -1) const;

  /// Find parameter by name. Uses getNameId().
  /// Returns parameter index or -1 if not found.
  int findParam(const char *name, int start_after = -1) const { return findParam(getNameId(name), start_after); }

  /// Returns true if there is parameter with specified name in this DataBlock.
  bool paramExists(const char *name, int start_after = -1) const { return findParam(name, start_after) >= 0; }

  std::optional<std::reference_wrapper<const Param>> getParam(int param_number) const;

  const char *getStr(int param_number, const char *def = "") const;
  bool getBool(int param_number, bool def = false) const;
  int getInt(int param_number, int def = 0) const;
  real getReal(int param_number, real def = 0.f) const;
  Point3 getPoint3(int param_number, const Point3 &def = Point3(0, 0, 0)) const;

  const char *getStr(const char *name, const char *def) const;
  bool getBool(const char *name, bool def) const;
  int getInt(const char *name, int def) const;
  real getReal(const char *name, real def) const;
  Point3 getPoint3(const char *name, const Point3 &def) const;

  int setStr(const char *name, const char *value);
  int setBool(const char *name, bool value);
  int setInt(const char *name, int value);
  int setReal(const char *name, real value);
  int setPoint3(const char *name, const Point3 &value);

  int addStr(const char *name, const char *value);
  int addBool(const char *name, bool value);
  int addInt(const char *name, int value);
  int addReal(const char *name, real value);
  int addPoint3(const char *name, const Point3 &value);

protected:
  friend class DataBlockParser;

  void setBlockName(const char *name);

  int addBlock(std::unique_ptr<DataBlock>);

  int addParam(const char *name, ParamType type, const char *value, int line, const char *filename);

  /// Save this DataBlock (and its sub-tree) in the text form.
  /// level is used for text indentation.
  void saveText(std::ofstream &os, int level = 0) const;

  std::shared_ptr<NameMap> nameMap;
  int nameId;
  std::vector<std::unique_ptr<DataBlock>> blocks;
  std::vector<Param> params;
  std::vector<int> nameIds;
};

DataBlock::ParamType type(const DataBlock::Param &p);
