//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <EASTL/unique_ptr.h>
#include <generic/dag_span.h>
#include <generic/dag_carray.h>
#include <dag/dag_relocatable.h>
#include <util/dag_bitFlagsMask.h>

#include <supp/dag_define_KRNLIMP.h>

class DataBlock;
struct DataBlockShared;
struct DataBlockOwned;
struct DBNameMap;

class Point2;
class Point3;
class Point4;
class IPoint2;
class IPoint3;
class TMatrix;
struct E3DCOLOR;

class IGenLoad;
class IGenSave;
class String;
struct RoDataBlock;
struct ZSTD_CDict_s;
struct ZSTD_DDict_s;
struct VirtualRomFsData;


/// @addtogroup utility_classes
/// @{

/// @addtogroup serialization
/// @{


/// @file
/// DataBlock class for reading and writing hierarchically structured data.


namespace dblk
{
enum class ReadFlag : uint8_t
{
  ROBUST = 1,        //< robust data load (sticky flag)
  BINARY_ONLY = 2,   //< don't try to parse text files
  RESTORE_FLAGS = 4, //< restore sticky flags state after load() call
  ALLOW_SS = 8,      //< allow simple string during load call (see DataBlock::allowSimpleString)

#if DAGOR_DBGLEVEL < 1
  ROBUST_IN_REL = ROBUST,
#else
  ROBUST_IN_REL = 0,
#endif
};

using ReadFlags = BitFlagsMask<ReadFlag>;
BITMASK_DECLARE_FLAGS_OPERATORS(ReadFlag);

template <typename Cb>
static inline void iterate_child_blocks(const DataBlock &db, Cb cb);
template <typename Cb>
static inline void iterate_blocks(const DataBlock &db, Cb cb);
template <typename Cb>
static inline void iterate_blocks_lev(const DataBlock &db, Cb cb, int lev = 0);
template <typename Cb>
static inline void iterate_params(const DataBlock &db, Cb cb);

// will compare blocks as in operator==, but float values will be compared with epsilon
bool are_approximately_equal(const DataBlock &lhs, const DataBlock &rhs, float eps = 1e-5f);

} // namespace dblk


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
  KRNLIMP static const DataBlock emptyBlock;

  KRNLIMP static bool strongTypeChecking;  // def= false;
  KRNLIMP static bool singleBlockChecking; // def= false;
  KRNLIMP static bool allowVarTypeChange;  // def= false;
  KRNLIMP static bool fatalOnMissingFile;  // def= true;
  KRNLIMP static bool fatalOnLoadFailed;   // def= true;
  KRNLIMP static bool fatalOnBadVarType;   // def= true;
  KRNLIMP static bool fatalOnMissingVar;   // def= true;

  //! when true, allows parsing 'param=val' just like 'param:t=val'
  KRNLIMP static bool allowSimpleString; // def= false;

  //! when true, includes are not resolved but added as 'special' string params
  KRNLIMP static bool parseIncludesAsParams; // def= false;
  //! when true, special commands (@override:, @delete:, etc.) are not resolved but added as params/blocks
  KRNLIMP static bool parseOverridesNotApply; // def= false;
  //! when true, special commands (@override:, @delete:, etc.) are fully ignored
  KRNLIMP static bool parseOverridesIgnored; // def= false;
  //! when true, comments are not removed but added as 'special' string params
  KRNLIMP static bool parseCommentsAsParams; // def= false;

  //! error reporting pipe
  struct IErrorReporterPipe
  {
    virtual void reportError(const char *error_text, bool serious_err) = 0;
  };
  struct InstallReporterRAII
  {
    IErrorReporterPipe *prev = nullptr;
    KRNLIMP InstallReporterRAII(IErrorReporterPipe *rep);
    KRNLIMP ~InstallReporterRAII();
  };

  struct IFileNotify
  {
    virtual void onFileLoaded(const char *fname) = 0;
  };
  struct IIncludeFileResolver
  {
    virtual bool resolveIncludeFile(String &inout_fname) = 0;
  };

  //! installs custom include resolver
  static KRNLIMP void setIncludeResolver(IIncludeFileResolver *f_resolver);
  //! installs simple root include resolver (resolves "#name.inc" as $(Root)/name.inc)
  static KRNLIMP void setRootIncludeResolver(const char *root);
  //! resolve include path based on include resolver, returns true if can resolve path
  static KRNLIMP bool resolveIncludePath(String &inout_fname);


public:
  /// Parameter types enum.
  enum ParamType : uint8_t
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
    TYPE_INT64,    ///< int64_t
    TYPE_COUNT
  };

  template <class T>
  struct TypeOf
  {
    static constexpr int type = TYPE_NONE;
  };
  template <class T>
  struct TypeOf<T &>
  {
    static constexpr int type = TypeOf<T>::type;
  };
  template <class T>
  struct TypeOf<T &&>
  {
    static constexpr int type = TypeOf<T>::type;
  };

  /// Default constructor, constructs empty block.
  KRNLIMP DataBlock(IMemAlloc *allocator = tmpmem);

  /// Constructor that loads DataBlock tree from specified file.
  /// If you want error checking, use default constructor and load().
  explicit KRNLIMP DataBlock(const char *filename, IMemAlloc *allocator = tmpmem);

  /// Destructor, destroys all sub-blocks.
  KRNLIMP ~DataBlock();

  /// Copy constructor.
  KRNLIMP DataBlock(const DataBlock &);
  // copy contructor with move semantic
  KRNLIMP DataBlock(DataBlock &&b);

  // setFrom.
  DataBlock &operator=(const DataBlock &right)
  {
    setFrom(&right);
    return *this;
  }
  // assignment with move-semantic
  KRNLIMP DataBlock &operator=(DataBlock &&right);

  // copy from read-only data block.
  DataBlock &operator=(const RoDataBlock &right)
  {
    setFrom(right);
    return *this;
  }

  KRNLIMP bool operator==(const DataBlock &rhs) const;
  bool operator!=(const DataBlock &rhs) const { return !(*this == rhs); }


  // install (or reset when owner_of_shared_namemap=nullptr) shared namemap (may be reused for several BLKs)
  KRNLIMP void setSharedNameMapAndClearData(DataBlock *owner_of_shared_namemap);

  /// Delete all sub-blocks.
  KRNLIMP void clearData();

  /// Reset DataBlock object (clear all data & names).
  KRNLIMP void reset();

  /// Reset DataBlock object (clear all data & names), also releases RO namemap if used any.
  KRNLIMP void resetAndReleaseRoNameMap();

  /// @name Loading
  /// @{

  /// Load DataBlock tree from specified text.
  /// fname is used only for error reporting.
  inline bool loadText(const char *text, int text_len, const char *fname = nullptr)
  {
    return loadText(text, text_len, fname, nullptr);
  }

  /// Load DataBlock tree from arbitrary stream
  /// Data may be presented like text, binary or stream data
  /// fname is used to correctly parse include directives when loading from a text file
  inline bool loadFromStream(IGenLoad &crd, const char *fname = nullptr, unsigned hint_size = 128)
  {
    return loadFromStream(crd, fname, nullptr, hint_size);
  }

  /// Load DataBlock tree from any type of file, binary or text
  /// First function try to load file as binary, in fail case it
  /// try to load file as text
  inline bool load(const char *fname) { return load(fname, nullptr); }

  /// @}


  /// @name Saving
  /// @{

  /// Save this DataBlock (and its sub-tree) to the specified file (text form)
  inline bool saveToTextFile(const char *filename) const;

  /// Save this DataBlock (and its sub-tree) to the specified file (text compact form)
  inline bool saveToTextFileCompact(const char *filename) const;

  /// Save this DataBlock (and its sub-tree) to the arbitrary stream (text form)
  KRNLIMP bool saveToTextStream(IGenSave &cwr) const;

  /// Save this DataBlock (and its sub-tree) to the arbitrary stream in compact form (text form)
  KRNLIMP bool saveToTextStreamCompact(IGenSave &cwr) const;

  /// Save this DataBlock (and its sub-tree) to arbitrary stream (binary form)
  KRNLIMP bool saveToStream(IGenSave &cwr) const;

  /// Print this DataBlock (and its sub-tree) to the arbitrary text stream with limitations; returns true when whole BLK is written
  inline bool printToTextStreamLimited(IGenSave &cwr, int max_out_line_num = -1, int max_level_depth = -1, int init_indent = 0) const;


  /// @}


  /// @name General methods (applicable for root block)
  /// @{

  /// Returns true if data in DataBlock are valid
  /// Data can be invalid if error occured while loading file
  KRNLIMP bool isValid() const; //== deprecated, to be deleted soon; use result of load*() instead

  /// Resolves filename if it was stored during BLK load
  KRNLIMP const char *resolveFilename(bool file_only = false) const;

  /// do compact all data (useful when BLK construction is done and we need lesser memory consumption)
  void compact() { shrink(); }
  KRNLIMP void shrink();

  /// Returns true when this DataBlock is empty (no params and blocks exist)
  bool isEmpty() const { return (blockCount() + paramCount()) == 0; }

  /// returns true for root BLK that owns 'data'
  bool topMost() const;

  /// computes and returns memory consumed by full BLK tree
  size_t memUsed() const;
  /// @}


  /// @name Names
  /// @{

  /// Returns name id from NameMap, or -1 if there's no such name in the NameMap.
  KRNLIMP int getNameId(const char *name) const;

  /// Returns name by name id, uses NameMap.
  /// Returns NULL if name id is not valid.
  KRNLIMP const char *getName(int name_id) const;

  // add name to NameMap
  // Returns new name id from NameMap
  KRNLIMP int addNameId(const char *name);

  /// @}


  /// @name Block Name
  /// @{

  /// Returns name id of this DataBlock.
  bool hasNoNameId() const { return getNameIdIncreased() == 0; }
  int getNameId() const { return getNameIdIncreased() - 1; }
  int getBlockNameId() const { return getNameId(); }

  /// Returns name of this DataBlock.
  const char *getBlockName() const { return getName(getNameId()); }
  /// Changes name of i-th parameter
  KRNLIMP void changeBlockName(const char *name);

  /// @}


  /// @name Sub-blocks
  /// @{

  /// Returns number of sub-blocks in this DataBlock.
  /// Use for enumeration.
  uint32_t blockCount() const { return blocksCount; }

  KRNLIMP int blockCountById(int name_id) const;
  int blockCountByName(const char *name) const { return blockCountById(getNameId(name)); }

  /// Returns pointer to i-th sub-block.
  KRNLIMP const DataBlock *getBlock(uint32_t i) const;
  KRNLIMP DataBlock *getBlock(uint32_t i);

  /// Returns pointer to sub-block with specified name id, or NULL if not found.
  KRNLIMP const DataBlock *getBlockByName(int name_id, int start_after = -1, bool expect_single = false) const;
  KRNLIMP DataBlock *getBlockByName(int name_id, int start_after = -1, bool expect_single = false);

  const DataBlock *getBlockByNameId(int name_id) const { return getBlockByName(name_id); }
  DataBlock *getBlockByNameId(int name_id) { return getBlockByName(name_id); }

  /// Finds block by name id
  KRNLIMP int findBlock(int name_id, int start_after = -1) const;
  int findBlock(const char *name, int start_after = -1) const { return findBlock(getNameId(name), start_after); }

  /// Finds block by name id (in reverse order)
  KRNLIMP int findBlockRev(int name_id, int start_before) const;

  /// Returns pointer to sub-block with specified name, or NULL if not found.
  const DataBlock *getBlockByName(const char *nm, int _after) const { return getBlockByName(getNameId(nm), _after, false); }
  DataBlock *getBlockByName(const char *name, int _after) { return getBlockByName(getNameId(name), _after, false); }

  /// functions that take only name now check that only one such block exists
  const DataBlock *getBlockByName(const char *name) const { return getBlockByName(getNameId(name), -1, true); }
  DataBlock *getBlockByName(const char *name) { return getBlockByName(getNameId(name), -1, true); }

  bool blockExists(const char *name) const { return getBlockByName(getNameId(name), -1, false) != NULL; }

  /// Get sub-block by name, returns @b def_blk, if not found.
  const DataBlock *getBlockByNameEx(const char *name, const DataBlock *def_blk) const
  {
    const DataBlock *blk = getBlockByName(getNameId(name), -1, true);
    return blk ? blk : def_blk;
  }

  /// Get block by name, returns (always valid) @b emptyBlock, if not found.
  const DataBlock *getBlockByNameEx(const char *name) const { return getBlockByNameEx(name, &emptyBlock); }

  /// Add block or get existing one.
  /// See also addNewBlock() and getBlockByNameEx(const char *name) const.
  KRNLIMP DataBlock *addBlock(const char *name);

  /// Add new block. See also addBlock().
  KRNLIMP DataBlock *addNewBlock(const char *name);

  /// Copies all parameters (not sub-blocks!) from specified DataBlock.
  void setParamsFrom(const DataBlock *copy_from)
  {
    clearParams();
    copy_from->addParamsTo(*this);
  }

  /// Copies all parameters (not sub-blocks!) from specified DataBlock.
  KRNLIMP void setParamsFrom(const RoDataBlock &copy_from);

  /// Copies all parameters (not sub-blocks!) from specified DataBlock without erasing old ones.
  KRNLIMP void appendParamsFrom(const DataBlock *copy_from);

  /// Create new block as a copy of specified DataBlock, with copy of all its sub-tree.
  /// @note Specified DataBlock can be from different DataBlock tree.
  KRNLIMP DataBlock *addNewBlock(const DataBlock *copy_from, const char *as_name = NULL);

  /// Remove all sub-blocks with specified name.
  /// Returns false if no sub-blocks were removed.
  KRNLIMP bool removeBlock(const char *name);

  /// Remove sub-block with specified index.
  /// Returns false on error.
  KRNLIMP bool removeBlock(uint32_t i);

  /// Swaps two blocks with specified indices.
  /// Returns false on error.
  KRNLIMP bool swapBlocks(uint32_t i1, uint32_t i2);

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
  KRNLIMP void setFrom(const DataBlock *from, const char *fname = nullptr);

  /// Clears data, then copies all parameters and sub-blocks from specified DataBlock.
  KRNLIMP void setFrom(const RoDataBlock &from);
  /// @}


  /// @name Parameters - Getting and Enumeration
  /// @{

  /// Returns number of parameters in this DataBlock.
  /// Use for enumeration.
  uint32_t paramCount() const { return paramsCount; }

  KRNLIMP int paramCountById(int name_id) const;
  int paramCountByName(const char *name) const { return paramCountById(getNameId(name)); }

  /// Returns type of i-th parameter. See ParamType enum.
  KRNLIMP int getParamType(uint32_t i) const;

  /// Returns i-th parameter name id. See getNameId().
  KRNLIMP int getParamNameId(uint32_t i) const;

  /// Returns i-th parameter name. Uses getName().
  const char *getParamName(uint32_t i) const { return getName(getParamNameId(i)); }

  /// Changes name of i-th parameter
  KRNLIMP void changeParamName(uint32_t i, const char *name);

  /// Find parameter by name id.
  /// Returns parameter index or -1 if not found.
  KRNLIMP int findParam(int name_id) const;
  KRNLIMP int findParam(int name_id, int start_after) const;

  /// Finds parameter by name id (in reverse order), returns parameter index or -1 if not found.
  KRNLIMP int findParamRev(int name_id, int start_before) const;

  /// Find parameter by name. Uses getNameId().
  /// Returns parameter index or -1 if not found.
  int findParam(const char *name, int _after = -1) const { return findParam(getNameId(name), _after); }

  /// Returns true if there is parameter with specified name id in this DataBlock.
  bool paramExists(int name_id, int _after = -1) const { return findParam(name_id, _after) >= 0; }

  /// Returns true if there is parameter with specified name in this DataBlock.
  bool paramExists(const char *name, int _after = -1) const { return findParam(name, _after) >= 0; }

  /// @}


  template <class T>
  T getByName(const char *name) const;
  template <class T>
  T getByName(const char *name, const T &def) const;
  template <class T>
  T getByNameId(int paramNameId, const T &def) const;

#define TYPE_FUNCTION_3(CppType, CRefType, ApiName)                            \
  KRNLIMP CppType get##ApiName(int param_idx) const;                           \
  KRNLIMP CppType get##ApiName(const char *name, CRefType def) const;          \
  KRNLIMP CppType get##ApiName(const char *name) const;                        \
  KRNLIMP CppType get##ApiName##ByNameId(int paramNameId, CRefType def) const; \
  KRNLIMP bool set##ApiName(int param_idx, CRefType value);                    \
  KRNLIMP int set##ApiName##ByNameId(int name_id, CRefType value);             \
  KRNLIMP int set##ApiName(const char *name, CRefType value);                  \
  KRNLIMP int add##ApiName(const char *name, CRefType value);                  \
  KRNLIMP int addNew##ApiName##ByNameId(int name_id, CRefType value);

#define TYPE_FUNCTION(CppType, ApiName)    TYPE_FUNCTION_3(CppType, CppType, ApiName)
#define TYPE_FUNCTION_CR(CppType, ApiName) TYPE_FUNCTION_3(CppType, const CppType &, ApiName)

  typedef const char *string_t;
  TYPE_FUNCTION(string_t, Str)
  TYPE_FUNCTION(int, Int)
  TYPE_FUNCTION(E3DCOLOR, E3dcolor)
  TYPE_FUNCTION(int64_t, Int64)
  TYPE_FUNCTION(float, Real)
  TYPE_FUNCTION(bool, Bool)
  TYPE_FUNCTION_CR(Point2, Point2)
  TYPE_FUNCTION_CR(Point3, Point3)
  TYPE_FUNCTION_CR(Point4, Point4)
  TYPE_FUNCTION_CR(IPoint2, IPoint2)
  TYPE_FUNCTION_CR(IPoint3, IPoint3)
  TYPE_FUNCTION_CR(TMatrix, Tm)
#undef TYPE_FUNCTION
#undef TYPE_FUNCTION_CR
#undef TYPE_FUNCTION_3

  /// @name Parameters - Removing
  /// @{

  /// Remove all parameters with the specified name.
  /// Returns false if no parameters were removed.
  KRNLIMP bool removeParam(const char *name);

  /// Remove param by index
  KRNLIMP bool removeParam(uint32_t i);

  /// @}

  /// Swaps two blocks with specified indices.
  /// Returns false on error.
  KRNLIMP bool swapParams(uint32_t i1, uint32_t i2);

  KRNLIMP void appendNamemapToSharedNamemap(DBNameMap &shared_nm, const DBNameMap *skip = nullptr) const;
  KRNLIMP bool saveBinDumpWithSharedNamemap(IGenSave &cwr, const DBNameMap *shared_nm, bool pack = false,
    const ZSTD_CDict_s *dict = nullptr) const;
  KRNLIMP bool loadBinDumpWithSharedNamemap(IGenLoad &crd, const DBNameMap *shared_nm, const ZSTD_DDict_s *dict = nullptr);

  /// Save DataBlock tree to binary stream (optionally using namemap stored separately)
  KRNLIMP bool saveDumpToBinStream(IGenSave &cwr, const DBNameMap *ro) const;
  /// Load DataBlock tree from binary stream using namemap stored separately
  KRNLIMP bool loadFromBinDump(IGenLoad &cr, const DBNameMap *ro);

protected:
  static constexpr int INPLACE_PARAM_SIZE = 4;
  struct Param
  {
    uint32_t nameId : 24;
    uint32_t type : 8;
    uint32_t v;
  };
  typedef DataBlock *block_id_t;

  DataBlockShared *shared = nullptr;
  static constexpr uint32_t NAME_ID_MASK = (1 << 30) - 1;
  static constexpr uint32_t IS_TOPMOST = (1 << 31); // owner
  uint32_t nameIdAndFlags = 0;
  uint16_t paramsCount = 0, blocksCount = 0;
  uint32_t firstBlockId = 0;
  uint32_t ofs = 0; // RO param data starts here.
  DataBlockOwned *data = nullptr;

  friend struct DbUtils;
  friend class DataBlockParser;
  template <typename Cb>
  friend void dblk::iterate_child_blocks(const DataBlock &db, Cb cb);
  template <typename Cb>
  friend void dblk::iterate_blocks(const DataBlock &db, Cb cb);
  template <typename Cb>
  friend void dblk::iterate_blocks_lev(const DataBlock &db, Cb cb, int lev);
  template <typename Cb>
  friend void dblk::iterate_params(const DataBlock &db, Cb cb);
  friend bool dblk::are_approximately_equal(const DataBlock &lhs, const DataBlock &rhs, float eps);
  uint32_t getNameIdIncreased() const { return nameIdAndFlags & NAME_ID_MASK; }

protected:
  DataBlock(DataBlockShared *s, const char *nm);
  DataBlock(DataBlockShared *s, int nid, uint16_t pcnt, uint16_t bcnt, uint32_t fBlock, uint32_t ofs_);

  void saveToBinStreamWithoutNames(const DataBlockShared &names, IGenSave &cwr) const;
  template <bool print_with_limits>
  bool writeText(IGenSave &cwr, int lev, int *max_ln, int max_lev) const;

  KRNLIMP bool loadText(const char *text, int text_length, const char *fname, DataBlock::IFileNotify *fnotify);
  KRNLIMP bool loadFromStream(IGenLoad &crd, const char *fname, DataBlock::IFileNotify *fnotify, unsigned hint_size);
  KRNLIMP bool load(const char *fname, DataBlock::IFileNotify *fnotify);
  bool doLoadFromStreamBBF3(IGenLoad &cwr);
  bool loadFromStreamBBF3(IGenLoad &cwr, const int *cnv, const int *cnvEnd, const DBNameMap &strings);

  int addParam(const char *name, int type, const char *value, const char *val_end, int line, const char *filename, int at = -1);
  KRNLIMP void addParamsTo(DataBlock &dest) const;
  KRNLIMP void clearParams();

  size_t memUsed_() const;

  KRNLIMP const DataBlock *const *getBlockRWPtr() const;
  KRNLIMP const DataBlock *getBlockROPtr() const;

  DataBlock *getBlockRW(uint32_t i);
  DataBlock *getBlockRO(uint32_t i);
  int findBlockRO(int nid, int after) const;
  int findBlockRW(int nid, int after) const;

  template <bool rw>
  int findParam(int name_id) const;
  template <bool rw>
  int findParam(int name_id, int start_after) const;

  template <class T, bool check_name_id>
  int setByNameId(int paramNameId, const T &val);
  template <class T>
  int setByName(const char *name, const T &val)
  {
    return setByNameId<T, false>(addNameId(name), val);
  }
  template <class T>
  int addByName(const char *name, const T &val)
  {
    return addByNameId<T, false>(addNameId(name), val);
  }

  template <class T, bool check_name_id>
  int addByNameId(int name_id, const T &val);
  template <class T>
  int insertParamAt(uint32_t at, uint32_t name_id, const T &v);

  template <class T, bool rw>
  T getByName(const char *name) const
  {
    const int nid = getNameId(name);
    const int pid = findParam<rw>(nid);
    if (pid >= 0)
      return get<T, rw>(pid);
    issue_error_missing_param(name, TypeOf<T>::type);
    return T();
  }
  template <class T, bool rw>
  T getByNameId(int paramNameId, const T &def) const;

  template <class T>
  T get(uint32_t param_idx, const T &def) const
  {
    return isOwned() ? get<T, true>(param_idx, def) : get<T, false>(param_idx, def);
  }
  template <class T>
  T get(uint32_t param_idx) const
  {
    return isOwned() ? get<T, true>(param_idx) : get<T, false>(param_idx);
  }

  template <class T, bool rw>
  T get(uint32_t param_idx) const;
  template <class T, bool rw>
  T get(uint32_t param_idx, const T &def) const;
  template <class T>
  bool set(uint32_t param_idx, const T &v);

  uint32_t allocateNewString(string_t v, size_t vlen); // returns raw data ofs in rw
  uint32_t insertNewString(string_t v, size_t vlen);   // returns string id
  template <class T, bool rw>
  T getParamString(int) const;

  template <bool rw>
  const Param *getCParams() const
  {
    return (const Param *)(rw ? rwDataAt(0) : roDataAt(ofs));
  }
  template <bool rw>
  Param *getParams()
  {
    return (Param *)(rw ? rwDataAt(0) : roDataAt(ofs));
  }
  template <bool rw>
  const Param *getParams() const
  {
    return getCParams<rw>();
  }
  template <bool rw>
  Param &getParam(uint32_t i);
  template <bool rw>
  const Param &getParam(uint32_t i) const;

  __forceinline Param *getParamsImpl();
  __forceinline const Param *getParamsImpl() const;
  KRNLIMP const Param *getParamsPtr() const;

  static uint32_t &getParamV(Param &p) { return p.v; }
  static const uint32_t &getParamV(const Param &p) { return p.v; }
  const char *getParamData(const Param &p) const;
  __forceinline void insertNewParamRaw(uint32_t pid, uint32_t nameId, uint32_t type, size_t type_size, const char *data);

  const char *roDataAt(uint32_t at) const;
  const char *rwDataAt(uint32_t at) const;
  char *rwDataAt(uint32_t at);

  template <class T>
  inline T return_string(const char *) const
  {
    return T();
  };

  template <class T>
  T &getRW(uint32_t at)
  {
    return *(T *)rwDataAt(at);
  }

  DataBlockOwned &getData();
  const DataBlockOwned &getData() const;

  static constexpr uint32_t IS_OWNED = ~uint32_t(0);
  int firstBlock() const { return firstBlockId; }
  // probably can be optimized, by assuming ofs is never 0 and firstBlockId is also never 0
  bool isOwned() const { return ofs == IS_OWNED; }
  bool isBlocksOwned() const { return firstBlockId == IS_OWNED; }
  __forceinline void toOwned()
  {
    if (!isOwned())
      convertToOwned();
  }

  uint32_t blocksOffset() const;
  uint32_t getUsedSize() const;
  void convertToOwned();
  void convertToBlocksOwned();
  uint32_t complexParamsSize() const;
  char *insertAt(uint32_t at, uint32_t n);
  char *insertAt(uint32_t at, uint32_t n, const char *v);
  void createData();
  void deleteShared();

  void issue_error_missing_param(const char *pname, int type) const;
  void issue_error_missing_file(const char *fname, const char *desc) const;
  void issue_error_load_failed(const char *fname, const char *desc) const;
  void issue_error_load_failed_ver(const char *fname, unsigned req_ver, unsigned file_ver) const;
  void issue_error_parsing(const char *fname, int curLine, const char *msg, const char *curLineP) const;
  void issue_error_bad_type(const char *pname, int type_new, int type_prev, const char *fname) const;
  void issue_error_bad_type(int pnid, int type_new, int type_prev) const;
  void issue_error_bad_value(const char *pname, const char *value, int type, const char *fname, int line) const;
  void issue_error_bad_type_get(int bnid, int pnid, int type_get, int type_data) const;
  void issue_warning_huge_string(const char *pname, const char *value, const char *fname, int line) const;
  void issue_deprecated_type_change(int pnid, int type_new, int type_prev) const;
};
DAG_DECLARE_RELOCATABLE(DataBlock);

#undef INLINE

namespace dblk
{
/// Load BLK from specified file
KRNLIMP bool load(DataBlock &blk, const char *fname, ReadFlags flg = ReadFlags(), DataBlock::IFileNotify *fnotify = nullptr);

/// Load BLK from text (fname is used only for error reporting)
KRNLIMP bool load_text(DataBlock &blk, dag::ConstSpan<char> text, ReadFlags flg = ReadFlags(), const char *fname = nullptr,
  DataBlock::IFileNotify *fnotify = nullptr);

/// Load BLK from stream (fname is used only for error reporting)
KRNLIMP bool load_from_stream(DataBlock &blk, IGenLoad &crd, ReadFlags flg = ReadFlags(), const char *fname = nullptr,
  DataBlock::IFileNotify *fnotify = nullptr, unsigned hint_size = 128);

/// Save BLK (and its sub-tree) to the specified file (text form)
KRNLIMP bool save_to_text_file(const DataBlock &blk, const char *filename);

/// Save BLK (and its sub-tree) to the specified file (text form) in compact form
KRNLIMP bool save_to_text_file_compact(const DataBlock &blk, const char *filename);

/// Save BLK (and its sub-tree) to the specified file (binary form)
KRNLIMP bool save_to_binary_file(const DataBlock &blk, const char *filename);

/// Print this DataBlock (and its sub-tree) to the arbitrary text stream with limitations; returns true when whole BLK is written
KRNLIMP bool print_to_text_stream_limited(const DataBlock &blk, IGenSave &cwr, int max_out_line_num = -1, int max_level_depth = -1,
  int init_indent = 0);

/// Save BLK (and its sub-tree) to the specified file (binary form, packed format)
KRNLIMP bool pack_to_binary_file(const DataBlock &blk, const char *filename, int approx_sz = 16 << 10);

/// Save BLK (and its sub-tree) to arbitrary stream (binary form, packed format)
KRNLIMP void pack_to_stream(const DataBlock &blk, IGenSave &cwr, int approx_sz = 16 << 10);

/// Export BLK as JSON to the arbitrary stream (text form)
KRNLIMP bool export_to_json_text_stream(const DataBlock &blk, IGenSave &cwr, bool allow_unquoted = false, int max_param_per_ln = 4,
  int max_block_per_ln = 1);


/// returns currently set flags of BLK object
KRNLIMP ReadFlags get_flags(const DataBlock &blk);
/// sets additional flags to BLK object
KRNLIMP void set_flag(DataBlock &blk, ReadFlags flg_to_add);
/// clear flags of BLK object
KRNLIMP void clr_flag(DataBlock &blk, ReadFlags flg_to_clr);


static inline const char *resolve_type(uint32_t type)
{
  static const char *types[DataBlock::TYPE_COUNT + 1] = {
    "none", "string", "int", "real", "point2", "point3", "point4", "ipoint2", "ipoint3", "bool", "e3dcolor", "tm", "int64", "unknown"};
  type = type < DataBlock::TYPE_COUNT ? type : DataBlock::TYPE_COUNT;
  return types[type];
}

static inline const char *resolve_short_type(uint32_t type)
{
  static const char *types[DataBlock::TYPE_COUNT + 1] = {
    "none", "t", "i", "r", "p2", "p3", "p4", "ip2", "ip3", "b", "c", "m", "i64", "err"};
  type = type < DataBlock::TYPE_COUNT ? type : DataBlock::TYPE_COUNT;
  return types[type];
}

static inline uint32_t get_type_size(uint32_t type)
{
  static const uint8_t sizes[DataBlock::TYPE_COUNT + 1] = {0, 8, 4, 4, 8, 12, 16, 8, 12, 1, 4, 12 * 4, 8, 0};
  type = type < DataBlock::TYPE_COUNT ? type : DataBlock::TYPE_COUNT;
  return sizes[type];
}

static inline bool is_ident_char(char c)
{
  return ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || (c >= 'A' && c <= 'Z') || c == '-' || c == '.' || c == '~');
}

extern KRNLIMP const char *SHARED_NAMEMAP_FNAME;

KRNLIMP DBNameMap *create_db_names();
KRNLIMP void destroy_db_names(DBNameMap *nm);
KRNLIMP bool write_names(IGenSave &cwr, const DBNameMap &names, uint64_t *names_hash);
KRNLIMP bool read_names(IGenLoad &cr, DBNameMap &names, uint64_t *names_hash);
KRNLIMP int db_names_count(DBNameMap *);

//! returns shared name map for specified vromfs (acquires reference)
KRNLIMP DBNameMap *get_vromfs_shared_name_map(const VirtualRomFsData *fs);
//! releases reference to shared name map; returns residual ref count
KRNLIMP int release_vromfs_shared_name_map(DBNameMap *nm);

//! returns ZSTD decoder dictionary for specified vromfs (acquires reference)
KRNLIMP ZSTD_DDict_s *get_vromfs_blk_ddict(const VirtualRomFsData *fs);
//! releases reference to ZSTD decoder dictionary; returns residual ref count
KRNLIMP int release_vromfs_blk_ddict(ZSTD_DDict_s *ddict);

//! returns the hash of ZSTD dictionary corresponding to given shared namemap
KRNLIMP dag::ConstSpan<char> get_vromfs_dict_hash(dag::ConstSpan<char> shared_nm_data);

//! discard unused shared name maps (that were allocated in get_vromfs_shared_name_map() calls); return residual used count
KRNLIMP int discard_unused_shared_name_maps();
//! discard unused ZSTD decoder dictionaries (that were allocated in get_vromfs_blk_ddict() calls); return residual used count
KRNLIMP int discard_unused_blk_ddict();

//! returns ZSTD encoder dictionary for specified vromfs created with zstd_create_cdict (to be released with zstd_destroy_cdict)
KRNLIMP ZSTD_CDict_s *create_vromfs_blk_cdict(const VirtualRomFsData *fs, int compr_level);

//! ZSTD-packs a binary stream as if it contained a datablock dump
KRNLIMP void pack_shared_nm_dump_to_stream(IGenSave &cwr, IGenLoad &crd, int sz, int compr_level, const ZSTD_CDict_s *cdict);
//! same but with the default (internally defined) compression level
KRNLIMP void pack_shared_nm_dump_to_stream(IGenSave &cwr, IGenLoad &crd, int sz, const ZSTD_CDict_s *cdict);

template <typename Cb>
static inline void iterate_child_blocks(const DataBlock &db, Cb cb);
template <typename Cb>
static inline void iterate_blocks(const DataBlock &db, Cb cb);
template <typename Cb>
static inline void iterate_child_blocks_by_name_id(const DataBlock &db, int name_id, Cb cb);
template <typename Cb>
static inline void iterate_child_blocks_by_name(const DataBlock &db, const char *nm, Cb cb);
template <typename Cb>
static inline void iterate_blocks_by_name_id(const DataBlock &db, int name_id, Cb cb);
template <typename Cb>
static inline void iterate_blocks_by_name(const DataBlock &db, const char *nm, Cb cb);
template <typename Cb>
static inline void iterate_blocks_by_name_id_list(const DataBlock &db, dag::ConstSpan<int> name_ids, Cb &&cb);
template <int N, typename Cb>
static inline void iterate_blocks_by_name_list(const DataBlock &db, const carray<const char *, N> &const_names, Cb &&cb);

template <typename Cb>
static inline void iterate_params(const DataBlock &db, Cb cb);
template <typename Cb>
static inline void iterate_params_by_type(const DataBlock &db, int type, Cb cb);
template <typename Cb>
static inline void iterate_params_by_name_id(const DataBlock &db, int name_id, Cb cb);
template <typename Cb>
static inline void iterate_params_by_name(const DataBlock &db, const char *name, Cb cb);
template <typename Cb>
static inline void iterate_params_by_name_id_and_type(const DataBlock &db, int name_id, int type, Cb cb);
template <typename Cb>
static inline void iterate_params_by_name_and_type(const DataBlock &db, const char *nm, int type, Cb cb);

} // namespace dblk

template <>
struct DataBlock::TypeOf<int>
{
  static constexpr int type = DataBlock::TYPE_INT;
};
template <>
struct DataBlock::TypeOf<float>
{
  static constexpr int type = DataBlock::TYPE_REAL;
};
template <>
struct DataBlock::TypeOf<bool>
{
  static constexpr int type = DataBlock::TYPE_BOOL;
};
template <>
struct DataBlock::TypeOf<E3DCOLOR>
{
  static constexpr int type = DataBlock::TYPE_E3DCOLOR;
};
template <>
struct DataBlock::TypeOf<int64_t>
{
  static constexpr int type = DataBlock::TYPE_INT64;
};
template <>
struct DataBlock::TypeOf<IPoint2>
{
  static constexpr int type = DataBlock::TYPE_IPOINT2;
};
template <>
struct DataBlock::TypeOf<IPoint3>
{
  static constexpr int type = DataBlock::TYPE_IPOINT3;
};
template <>
struct DataBlock::TypeOf<Point2>
{
  static constexpr int type = DataBlock::TYPE_POINT2;
};
template <>
struct DataBlock::TypeOf<Point3>
{
  static constexpr int type = DataBlock::TYPE_POINT3;
};
template <>
struct DataBlock::TypeOf<Point4>
{
  static constexpr int type = DataBlock::TYPE_POINT4;
};
template <>
struct DataBlock::TypeOf<TMatrix>
{
  static constexpr int type = DataBlock::TYPE_MATRIX;
};
template <>
struct DataBlock::TypeOf<const char *>
{
  static constexpr int type = DataBlock::TYPE_STRING;
};

template <>
bool DataBlock::set<DataBlock::string_t>(uint32_t param_idx, const DataBlock::string_t &v);
template <>
inline DataBlock::string_t DataBlock::return_string<DataBlock::string_t>(const char *s) const
{
  return s;
}

template <class T>
inline T DataBlock::getByName(const char *name) const
{
  return isOwned() ? getByName<T, true>(name) : getByName<T, false>(name);
}

template <class T>
inline T DataBlock::getByName(const char *name, const T &def) const
{
  const int id = getNameId(name);
  return id < 0 ? def : getByNameId(id, def);
}

template <class T>
inline T DataBlock::getByNameId(int paramNameId, const T &def) const
{
  return isOwned() ? getByNameId<T, true>(paramNameId, def) : getByNameId<T, false>(paramNameId, def);
}

inline bool DataBlock::saveToTextFile(const char *fn) const { return dblk::save_to_text_file(*this, fn); }

inline bool DataBlock::saveToTextFileCompact(const char *fn) const { return dblk::save_to_text_file_compact(*this, fn); }

inline bool DataBlock::printToTextStreamLimited(IGenSave &cwr, int max_out_line_num, int max_level_depth, int init_indent) const
{
  return dblk::print_to_text_stream_limited(*this, cwr, max_out_line_num, max_level_depth, init_indent);
}

template <typename Cb>
static inline void dblk::iterate_child_blocks(const DataBlock &db, Cb cb)
{
  if (const DataBlock *b = db.getBlockROPtr())
  {
    for (const DataBlock *be = b + db.blockCount(); b < be; b++)
      cb(*b);
  }
  else if (const DataBlock *const *b = db.getBlockRWPtr())
  {
    for (const DataBlock *const *be = b + db.blockCount(); b < be; b++)
      cb(**b);
  }
}
template <typename Cb>
static inline void dblk::iterate_blocks(const DataBlock &db, Cb cb)
{
  cb(db);
  iterate_child_blocks(db, [&](const DataBlock &b) { dblk::iterate_blocks(b, cb); });
}
template <typename Cb>
static inline void dblk::iterate_blocks_lev(const DataBlock &db, Cb cb, int lev)
{
  cb(db, lev);
  if (const DataBlock *b = db.getBlockROPtr())
  {
    for (const DataBlock *be = b + db.blockCount(); b < be; b++)
      iterate_blocks_lev(*b, cb, lev + 1);
  }
  else if (const DataBlock *const *b = db.getBlockRWPtr())
  {
    for (const DataBlock *const *be = b + db.blockCount(); b < be; b++)
      iterate_blocks_lev(**b, cb, lev + 1);
  }
}
template <typename Cb>
static inline void dblk::iterate_child_blocks_by_name_id(const DataBlock &db, int nid, Cb cb)
{
  iterate_child_blocks(db, [&](const DataBlock &b) {
    if (b.getBlockNameId() == nid)
      cb(b);
  });
}
template <typename Cb>
static inline void dblk::iterate_child_blocks_by_name(const DataBlock &db, const char *nm, Cb cb)
{
  int nid = db.getNameId(nm);
  if (nid >= 0)
    iterate_child_blocks_by_name_id(db, nid, cb);
}
template <typename Cb>
static inline void dblk::iterate_blocks_by_name_id(const DataBlock &db, int nid, Cb cb)
{
  if (db.getBlockNameId() == nid)
    cb(db);
  iterate_child_blocks_by_name_id(db, nid, [&](const DataBlock &b) { iterate_blocks_by_name_id(b, nid, cb); });
}
template <typename Cb>
static inline void dblk::iterate_blocks_by_name(const DataBlock &db, const char *nm, Cb cb)
{
  int nid = db.getNameId(nm);
  if (nid >= 0)
    dblk::iterate_blocks_by_name_id(db, nid, cb);
}

template <typename Cb>
static inline void dblk::iterate_blocks_by_name_id_list(const DataBlock &blk, dag::ConstSpan<int> name_ids, Cb &&cb)
{
  if (eastl::find(name_ids.begin(), name_ids.end(), blk.getBlockNameId()) != name_ids.end())
    cb(blk);
  dblk::iterate_child_blocks(blk, [&](const DataBlock &b) { iterate_blocks_by_name_id_list(b, name_ids, cb); });
}

template <int N, typename Cb>
static inline void dblk::iterate_blocks_by_name_list(const DataBlock &blk, const carray<const char *, N> &const_names, Cb &&cb)
{
  carray<int, N> nameIds;
  int cnt = 0;
  for (int i = 0; i < N; i++)
  {
    const int id = blk.getNameId(const_names[i]);
    if (id != -1)
      nameIds[cnt++] = id;
  }
  if (cnt == 0)
    return;
  iterate_blocks_by_name_id_list(blk, make_span_const(nameIds.data(), cnt), eastl::forward<Cb>(cb));
}

template <typename Cb>
static inline void dblk::iterate_params(const DataBlock &db, Cb cb)
{
  const DataBlock::Param *s = db.getParamsPtr(), *e = s + db.paramsCount;
  for (uint32_t i = 0; s < e; s++, i++)
    cb(i, s->nameId, s->type);
}

template <typename Cb>
static inline void dblk::iterate_params_by_type(const DataBlock &db, int type, Cb cb)
{
  iterate_params(db, [&](int param_idx, int param_name_id, int param_type) {
    if (param_type == type)
      cb(param_idx, param_name_id, param_type);
  });
}

template <typename Cb>
static inline void dblk::iterate_params_by_name_id(const DataBlock &db, int name_id, Cb cb)
{
  iterate_params(db, [&](int param_idx, int param_name_id, int param_type) {
    if (param_name_id == name_id)
      cb(param_idx, param_name_id, param_type);
  });
}
template <typename Cb>
static inline void dblk::iterate_params_by_name(const DataBlock &db, const char *nm, Cb cb)
{
  int name_id = db.getNameId(nm);
  if (name_id >= 0)
    iterate_params_by_name_id(db, name_id, cb);
}

template <typename Cb>
static inline void dblk::iterate_params_by_name_id_and_type(const DataBlock &db, int name_id, int type, Cb cb)
{
  iterate_params(db, [&](int param_idx, int param_name_id, int param_type) {
    if (param_name_id == name_id && param_type == type)
      cb(param_idx);
  });
}
template <typename Cb>
static inline void dblk::iterate_params_by_name_and_type(const DataBlock &db, const char *nm, int type, Cb cb)
{
  int name_id = db.getNameId(nm);
  if (name_id >= 0)
    iterate_params_by_name_id_and_type(db, name_id, type, cb);
}

#include <supp/dag_undef_KRNLIMP.h>

/// @}

/// @}
