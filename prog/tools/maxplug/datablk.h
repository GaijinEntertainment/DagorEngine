// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _GAIJIN_DAGOR_DATABLK_H
#define _GAIJIN_DAGOR_DATABLK_H
#pragma once

#include <max.h>
#include "math3d.h"
#include "e3dcolor.h"
// #include "tab.h"
// #include "dyntab.h"
// #include "dobject.h"
#include "str.h"

// #include "define_COREIMPORT.h"

#define INLINE __forceinline
#define real   float

class GeneralLoadCB;
class GeneralSaveCB;
class NameMap;

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
/// Parameter or sub-block name can be non-unique for a given DataBlock object.
/// This is useful for enumerating data that can be repeated arbitrary number of times.
///
/// For clarity, names are restricted to C indentifier rules.
///
/// Actual names are stored in NameMap that is shared by all DataBlocks in the tree.
/// Blocks and parameters use integer ids to address names in the NameMap, so
/// there are methods that take name ids and those that take character strings.
/// You can use name ids when you look for multiple blocks or parameters with
/// the same name for performance gain.
///
/// DataBlock tree contents can be serialized in binary or text form.
///
/// Text files of this format usually have extension ".blk".
class DataBlock
{
public:
  // DAG_DECLARE_NEW(tmpmem)

  static DataBlock *emptyBlock;

  /// Parameter types enum.
  enum ParamType
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

  enum DataSrc
  {
    SRC_UNKNOWN, ///< Unknown data source
    SRC_TEXT,    ///< Data was loaded from text file
    SRC_BINARY,  ///< Data was loaded from binary file
  };


  /// Default constructor, constructs empty block.
  DataBlock();

  /// Destructor, destroys all sub-blocks.
  ~DataBlock();

  /// Copy constructor.
  DataBlock(const DataBlock &);

  /// Constructor that loads DataBlock tree from specified file.
  /// If you want error checking, use default constructor and loadFile().
  DataBlock(const char *filename);

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
  bool loadText(Tab<char> &text, const char *filename = NULL);

  /// Load DataBlock tree from arbitrary stream
  /// Data may be presented like text, binary or stream data
  /// created by function beginTaggedBlock(_MAKE4C('blk'))
  /// fname uses if loading from text file to right parse include directives
  bool loadFromStream(FILE *crd, const char *fname = NULL);

  /// Load DataBlock tree from any type of file, binary or text
  /// First function try to load file as binary, in fail case it
  /// try to load file as text
  bool load(const char *fname);

  /// @}


  /// @name Saving
  /// @{

  /// Save this DataBlock (and its sub-tree) to the specified file (text form)
  bool saveToTextFile(const char *filename) const;

  /// Save this DataBlock (and its sub-tree) to arbitrary stream (binary form)
  void saveToStream(GeneralSaveCB &cwr) const;

  /// Save this DataBlock (and its sub-tree) to the specified file (binary form)
  bool saveToBinaryFile(const char *filename) const;

  /// @}


  /// @name Names
  /// @{

  /// Returns name id from NameMap, or -1 if there's no such name in the NameMap.
  int getNameId(const char *name) const;

  /// Returns name by name id, uses NameMap.
  /// Returns NULL if name id is not valid.
  const char *getName(int name_id) const;

  void fillNameMap(NameMap *stringMap) const;

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
  INLINE int blockCount() const { return blocks.Count(); }

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
      emptyBlock = new /*(inimem)*/ DataBlock;
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
  INLINE int paramCount() const { return params.Count(); }

  /// Returns type of i-th parameter. See ParamType enum.
  int getParamType(int param_number) const;

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

  const char *getStr(int param_number) const;
  bool getBool(int param_number) const;
  int getInt(int param_number) const;
  real getReal(int param_number) const;
  Point2 getPoint2(int param_number) const;
  Point3 getPoint3(int param_number) const;
  Point4 getPoint4(int param_number) const;
  IPoint2 getIPoint2(int param_number) const;
  IPoint3 getIPoint3(int param_number) const;
  E3DCOLOR getE3dcolor(int param_number) const;
  TMatrix getTm(int param_number) const;

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
  E3DCOLOR getE3dcolor(const char *name, E3DCOLOR def) const;
  TMatrix getTm(const char *name, TMatrix &def) const;

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

  /// Returns true if data in DataBlock are valid
  /// Data can be invalid if error occured while loading file
  inline bool isValid() const { return valid; }

  // Returns data mode, which means kind of data source for current DataBlock
  inline DataSrc getDataSrc() const { return dataSrc; }
  /// @}

protected:
  /// @cond
  friend class DataBlockParser;

  DataBlock(const DataBlock *);

  void setBlockName(const char *name);

  int addBlock(DataBlock *);

  int addParam(const char *name, int type, const char *value, int line, const char *filename);

  void shrink();

  /// Save this DataBlock (and its sub-tree) in the text form.
  /// @b level is used for text indentation.
  void saveText(FILE *cb, int level = 0) const;
  /// helper routine to save data tree
  void save(FILE *cb, NameMap &stringMap) const;
  /// helper routine to load data tree
  void load(FILE *cb, NameMap &stringMap);
  /// Loads binary only data from stream without version check
  void doLoadFromStream(FILE *crd);

  NameMap *nameMap;

  union Value
  {
    char *s;
    int i;
    real r;
    struct
    {
      Point2 p2;
    };
    struct
    {
      Point3 p3;
    };
    struct
    {
      Point4 p4;
    };
    struct
    {
      IPoint2 ip2;
    };
    struct
    {
      IPoint3 ip3;
    };
    bool b;
    struct
    {
      E3DCOLOR c;
    };
    struct
    {
      TMatrix tm;
    };

    Value() {}
  };

  struct Param
  {
    int nameId;
    Value value;
    int type;

    Param();
    ~Param();
  };

  int nameId;
  Tab<DataBlock *> blocks;
  /*Dyn*/ Tab<Param> params;

  bool valid;
  DataSrc dataSrc;
  /// @endcond

private:
  /// Load DataBlock tree from specified file (text form)
  /// Calls only from "load" function; do not use this function directly
  /// To load BLK use load(const char *filename) function
  //   bool loadFile(const char *filename);

  /// Load DataBlock tree from specified file (binary form)
  /// Calls only from "load" function; do not use this function directly
  /// To load BLK use load(const char *filename) function
  //   bool loadBinaryFile(const char *filename, bool& can_process_file);
};

#undef INLINE

// #include "undef_.h"

/// @}

/// @}

#endif // _GAIJIN_DAGOR_DATABLK_H
