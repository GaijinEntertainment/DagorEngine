// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <cassert>
#include <string>
#include <vector>
#include <fstream>
#include <memory>

#include <max.h>
#include "math3d.h"
#include "e3dcolor.h"
#include "namemap.h"

#define INLINE __forceinline
typedef float real;

/// @addtogroup utility_classes
/// @{

/// @addtogroup serialization
/// @{


/// @file
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
  // DAG_DECLARE_NEW(tmpmem)

  static DataBlock *emptyBlock;

  /// Parameter types enum.
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

  struct Param
  {
    int nameId;
    ParamType type;

    static const size_t max_size = ((sizeof(TMatrix) > sizeof(std::string)) ? sizeof(TMatrix) : sizeof(std::string));
    std::aligned_storage<max_size>::type data;

    Param(int id, const std::string &s);
    Param(int id, int i);
    Param(int id, real r);
    Param(int id, const Point2 &p2);
    Param(int id, const Point3 &p3);
    Param(int id, const Point4 &p4);
    Param(int id, const IPoint2 &ip2);
    Param(int id, const IPoint3 &ip3);
    Param(int id, bool b);
    Param(int id, const E3DCOLOR &c);
    Param(int id, const TMatrix &tm);

    Param(const Param &);
    ~Param();

    const char *as_c_str() const;
    int as_int() const;
    real as_real() const;
    const Point2 &as_pt2() const;
    const Point3 &as_pt3() const;
    const Point4 &as_pt4() const;
    const IPoint2 &as_ipt2() const;
    const IPoint3 &as_ipt3() const;
    bool as_bool() const;
    const E3DCOLOR &as_color() const;
    const TMatrix &as_tm() const;

    void set_str(const std::string &s);
    void set_int(int i);
    void set_real(real r);
    void set_pt2(const Point2 &p2);
    void set_pt3(const Point3 &p3);
    void set_pt4(const Point4 &p4);
    void set_ipt2(const IPoint2 &ip2);
    void set_ipt3(const IPoint3 &ip3);
    void set_bool(bool b);
    void set_color(const E3DCOLOR &c);
    void set_tm(const TMatrix &tm);
  };

  static ParamType deserialize_param_type(const std::string &s);

  DataBlock(std::shared_ptr<NameMap> nameMap);
  ~DataBlock();

private: // old C++ doesn't know =delete
  DataBlock(const DataBlock &) : nameId(-1) {};
  DataBlock &operator=(const DataBlock &) {};

public:
  /// Delete all sub-blocks.
  void clearData();

  /// Reset DataBlock object (clear all data & names).
  void reset();

  /// @name Loading
  /// @{

  /// Load DataBlock tree from specified text.
  /// Filename is for error output only.
  bool loadText(char *text, int text_length, const char *filename = NULL);

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
  bool load(const std::wstring &fname);

  /// @}


  /// @name Saving
  /// @{

  /// Save this DataBlock (and its sub-tree) to the specified file (text form)
  bool saveToTextFile(const std::wstring &filename) const;

  /// @}


  /// @name Names
  /// @{

  /// Returns name id from NameMap, or -1 if there's no such name in the NameMap.
  int getNameId(const char *name) const;

  /// Returns name by name id, uses NameMap.
  /// Returns NULL if name id is not valid.
  const char *getName(int name_id) const;

  /// @}


  /// @name Block Name
  /// @{

  /// Returns name id of this DataBlock.
  INLINE int getBlockNameId() const { return nameId; }

  /// Returns name of this DataBlock.
  INLINE const char *getBlockName() const { return getName(nameId); }

  /// @}


  /// @name Sub-blocks
  /// @{

  /// Returns number of sub-blocks in this DataBlock.
  /// Use for enumeration.
  INLINE int blockCount() const { return int(blocks.size()); }

  /// Returns pointer to i-th sub-block.
  DataBlock *getBlock(int block_number) const;

  /// Returns pointer to sub-block with specified name id, or NULL if not found.
  DataBlock *getBlockByName(int name_id, int start_after = -1) const;

  /// Returns pointer to sub-block with specified name, or NULL if not found.
  INLINE DataBlock *getBlockByName(const char *name, int start_after = -1) const
  {
    return getBlockByName(getNameId(name), start_after);
  }

  /// Get sub-block by name, returns @b def_blk, if not found.
  INLINE DataBlock *getBlockByNameEx(const char *name, DataBlock *def_blk) const
  {
    DataBlock *blk = getBlockByName(getNameId(name), -1);
    return blk ? blk : def_blk;
  }

  /// Get block by name, returns (always valid) @b emptyBlock, if not found.
  INLINE DataBlock *getBlockByNameEx(const char *name) const
  {
    if (!emptyBlock)
      emptyBlock = new /*(inimem)*/ DataBlock(nameMap);
    emptyBlock->reset();
    return getBlockByNameEx(name, emptyBlock);
  }

  /// Add block or get existing one.
  /// See also addNewBlock() and getBlockByNameEx(const char *name) const.
  DataBlock *addBlock(const char *name);

  /// Add new block. See also addBlock().
  DataBlock *addNewBlock(const char *name);

  /// Copies all parameters (not sub-blocks!) from specified DataBlock.
  void setParamsFrom(const DataBlock *copy_from);

  /// Create new block as a copy of specified DataBlock, with copy of all its sub-tree.
  /// @note Specified DataBlock can be from different DataBlock tree.
  DataBlock *addNewBlock(const DataBlock *copy_from, const char *as_name = NULL);

  /// Remove all sub-blocks with specified name.
  /// Returns false if no sub-blocks were removed.
  bool removeBlock(const char *name);

  /// Similar to addNewBlock(const DataBlock *copy_from, const char *as_name),
  /// but removes existing sub-blocks with the same name first.
  DataBlock *setBlock(const DataBlock *blk, const char *as_name = NULL)
  {
    if (!blk)
      return NULL;
    removeBlock(as_name ? as_name : blk->getBlockName());
    return addNewBlock(blk, as_name);
  }

  /// Clears data, then copies all parameters and sub-blocks from specified DataBlock.
  void setFrom(const DataBlock *from);

  /// @}


  /// @name Parameters - Getting and Enumeration
  /// @{

  /// Returns number of parameters in this DataBlock.
  /// Use for enumeration.
  INLINE int paramCount() const { return int(params.size()); }

  /// Returns type of i-th parameter. See ParamType enum.
  ParamType getParamType(int param_number) const;

  /// Returns i-th parameter name id. See getNameId().
  int getParamNameId(int param_number) const;

  /// Returns i-th parameter name. Uses getName().
  INLINE const char *getParamName(int param_number) const { return getName(getParamNameId(param_number)); }

  /// Find parameter by name id.
  /// Returns parameter index or -1 if not found.
  int findParam(int name_id, int start_after = -1) const;

  /// Find parameter by name. Uses getNameId().
  /// Returns parameter index or -1 if not found.
  INLINE int findParam(const char *name, int start_after = -1) const { return findParam(getNameId(name), start_after); }

  /// Returns true if there is parameter with specified name id in this DataBlock.
  INLINE bool paramExists(int name_id, int start_after = -1) const { return findParam(name_id, start_after) >= 0; }

  /// Returns true if there is parameter with specified name in this DataBlock.
  INLINE bool paramExists(const char *name, int start_after = -1) const { return findParam(name, start_after) >= 0; }

  const Param *getParam(int name_id) const;

  /// @}


  /// @name Parameters - Getting by Index
  /// These methods get parameter value by parameter index in the block,
  /// the index number is from @b 0 to @ref DataBlock::paramCount() "paramCount()" -1.
  ///
  /// On parameter type mismatch, zero-like value is returned.
  ///
  /// Use them for enumerating parameters together with DataBlock::getParamNameId() /
  /// DataBlock::getParamName() and DataBlock::getParamType().
  /// @{

  const char *getStr(int param_number, const char *def = "") const;
  bool getBool(int param_number, bool def = false) const;
  int getInt(int param_number, int def = 0) const;
  real getReal(int param_number, real def = 0.f) const;
  Point2 getPoint2(int param_number, const Point2 &def = Point2(0, 0)) const;
  Point3 getPoint3(int param_number, const Point3 &def = Point3(0, 0, 0)) const;
  Point4 getPoint4(int param_number, const Point4 &def = Point4(0, 0, 0, 0)) const;
  IPoint2 getIPoint2(int param_number, const IPoint2 &def = IPoint2(0, 0)) const;
  IPoint3 getIPoint3(int param_number, const IPoint3 &def = IPoint3(0, 0, 0)) const;
  E3DCOLOR getE3dcolor(int param_number, const E3DCOLOR &def = E3DCOLOR(0, 0, 0, 0)) const;
  TMatrix getTm(int param_number, const TMatrix &def = TMatrix::IDENT) const;

  /// @}


  /// @name Parameters - Getting by Name
  /// These methods get parameter value by parameter name.
  ///
  /// If there is no parameter with specified name in this DataBlock,
  /// or it has different type, @b default value is returned.
  ///
  /// If there is more than one matching parameter, value of the first one
  /// is returned.
  /// @{

  const char *getStr(const char *name, const char *def) const;
  bool getBool(const char *name, bool def) const;
  int getInt(const char *name, int def) const;
  real getReal(const char *name, real def) const;
  Point2 getPoint2(const char *name, const Point2 &def) const;
  Point3 getPoint3(const char *name, const Point3 &def) const;
  Point4 getPoint4(const char *name, const Point4 &def) const;
  IPoint2 getIPoint2(const char *name, const IPoint2 &def) const;
  IPoint3 getIPoint3(const char *name, const IPoint3 &def) const;
  E3DCOLOR getE3dcolor(const char *name, const E3DCOLOR &def) const;
  TMatrix getTm(const char *name, const TMatrix &def) const;

  /// @}


  /// @name Parameters - Setting
  /// These methods set value of the parameter with the specified name.
  ///
  /// If parameter of requested type doesn't exist, it's added,
  /// otherwise its value is set to the specified one.
  /// @{

  int setStr(const char *name, const char *value);
  int setBool(const char *name, bool value);
  int setInt(const char *name, int value);
  int setReal(const char *name, real value);
  int setPoint2(const char *name, const Point2 &value);
  int setPoint3(const char *name, const Point3 &value);
  int setPoint4(const char *name, const Point4 &value);
  int setIPoint2(const char *name, const IPoint2 &value);
  int setIPoint3(const char *name, const IPoint3 &value);
  int setE3dcolor(const char *name, const E3DCOLOR value);
  int setTm(const char *name, const TMatrix &value);

  /// @}


  /// @name Parameters - Adding
  /// These methods add new parameter with the specified name.
  ///
  /// It's possible to add multiple parameters with the same name and type.
  ///
  /// If you want just to add / set single parameter value, use @b set* methods.
  /// @{

  int addStr(const char *name, const char *value);
  int addBool(const char *name, bool value);
  int addInt(const char *name, int value);
  int addReal(const char *name, real value);
  int addPoint2(const char *name, const Point2 &value);
  int addPoint3(const char *name, const Point3 &value);
  int addPoint4(const char *name, const Point4 &value);
  int addIPoint2(const char *name, const IPoint2 &value);
  int addIPoint3(const char *name, const IPoint3 &value);
  int addE3dcolor(const char *name, const E3DCOLOR value);
  int addTm(const char *name, const TMatrix &value);

  /// @}


  /// @name Parameters - Removing
  /// @{

  /// Remove all parameters with the specified name.
  /// Returns false if no parameters were removed.
  bool removeParam(const char *name);

  /// @}

  /// @name Other methods
  /// @{

protected:
  /// @cond
  friend class DataBlockParser;

  void setBlockName(const char *name);

  int addBlock(DataBlock *);

  int addParam(const char *name, ParamType type, const char *value, int line, const char *filename);

  /// Save this DataBlock (and its sub-tree) in the text form.
  /// @b level is used for text indentation.
  void saveText(std::ofstream &os, int level = 0) const;

  std::shared_ptr<NameMap> nameMap;
  int nameId;
  std::vector<std::unique_ptr<DataBlock>> blocks;
  std::vector<Param> params;

  /// @endcond
};

#undef INLINE

// #include "undef_.h"

/// @}

/// @}
