// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// public interface implemented on editor side
namespace ddsx
{
/// conversion parameters and tex description to be written to DDSx header
struct ConvertParams
{
  ConvertParams() { defaults(); }

  void defaults()
  {
    allowNonPow2 = true;
    optimize = true;
    canPack = true;
    packSzThres = (256 << 10);
    addrU = ADDR_WRAP;
    addrV = ADDR_WRAP;
    hQMip = 0;
    mQMip = 0;
    lQMip = 0;
    imgGamma = 2.2;
    rtGenMipsBox = rtGenMipsKaizer = false;
    kaizerAlpha = kaizerStretch = 0;
    needSysMemCopy = false;
    needBaseTex = false;
    mipOrdRev = true;
    splitHigh = false;
    splitAt = 0;
  }

  //! enables usage of non-pow-2 textures as-is, without rescaling to neares pow-of-2 sizes
  bool allowNonPow2;
  //! target-dependent option (performs texture tiling on xbox and xbox360)
  bool optimize;
  //! enables packing of texture
  bool canPack;
  //! marks that copy of source texture data should be stored by driver to be retrieved with d3d::get_original_ddsx_copy()
  bool needSysMemCopy;
  //! marks that baseTex data is needed to decode texture
  bool needBaseTex;
  //! force load-time mipmap generation
  bool rtGenMipsBox, rtGenMipsKaizer;
  //! build DDSx with mips reversed (optimal for partial reading)
  bool mipOrdRev;
  //! gamma for rtGenMipsBox and rtGenMipsKaizer
  float imgGamma;
  //! parameters for rtGenMipsKaizer
  float kaizerAlpha, kaizerStretch;

  //! default addressing mode to be set after texture loading
  unsigned addrU, addrV;

  //! 3 quality levels (PC-specific)
  unsigned hQMip, mQMip, lQMip;

  //! size threshold (in bytes) to use packing
  unsigned packSzThres;

  //! normal/HQ mips split settings
  bool splitHigh;
  int splitAt;

  // identical to TEXADDR_WRAP for PC (for easy migration)
  enum
  {
    ADDR_WRAP = 1,
    ADDR_MIRROR = 2,
    ADDR_CLAMP = 3,
    ADDR_BORDER = 4,
    ADDR_MIRRORONCE = 5,
  };

public:
  static bool forceZlibPacking;     // =false by default
  static bool preferZstdPacking;    // =false by default
  static bool allowOodlePacking;    // =false by default
  static unsigned zstdMaxWindowLog; // =0 by default (to use zstd defaults for compression level)
  static int zstdCompressionLevel;  // =18 by default
};

struct Buffer
{
  void *ptr;
  int len;

  Buffer() : ptr(0), len(0) {}

  void zero()
  {
    ptr = 0;
    len = 0;
  }
  void free();
};

/// loads compatible plugins from given location; returns number of plugins
int load_plugins(const char *dirpath);

/// unloads plugins
void unload_plugins();

/// shutdowns all loaded plugins
void shutdown_plugins();

/// converts dds to target-specific DDSx format
bool convert_dds(unsigned target_code, Buffer &dest, void *dds_data, int dds_len, const ConvertParams &params);

/// returns text for last error, or NULL when no error occured
const char *get_last_error_text();
} // namespace ddsx


// plugin interface
class IDdsxCreatorPlugin
{
  static const int VERSION = 0x106;
  int version;

public:
  class IAlloc
  {
  public:
    virtual void *__stdcall alloc(int sz) = 0;
    virtual void __stdcall free(void *p) = 0;
  };

public:
  IDdsxCreatorPlugin() : version(VERSION) {}

  /// inline check for plugin/editor version match
  inline bool checkVersion() { return version == VERSION; }

  /// prepares plugin for work; no other methods can be called before startup()
  virtual bool __stdcall startup() = 0;
  /// frees resources use by plugin; no other methods can be called after shutdown() except for startup()
  virtual void __stdcall shutdown() = 0;

  /// returns fourcc code for target (e.g., 'PC' or 'X360')
  virtual unsigned __stdcall targetCode() = 0;

  /// converts dds to target-specific DDSx format
  virtual bool __stdcall convertDds(ddsx::Buffer &dest, void *dds_data, int dds_len, const ddsx::ConvertParams &params) = 0;

  /// returns constant size of data header (to be excluded from packing)
  virtual int __stdcall getTexDataHeaderSize(const void *data) const = 0;

  /// checks whether specific compression flags supported (ddsx::Header::FLG_ZLIB, ddsx::Header::FLG_7ZIP, etc.)
  virtual bool __stdcall checkCompressionSupport(int compr_flag) const = 0;

  /// sets compression flag in texture data header
  virtual void __stdcall setTexDataHeaderCompression(void *data, int compr_sz, int compr_flag) const = 0;

  /// returns text for last error, or NULL when no error occured
  virtual const char *__stdcall getLastErrorText() = 0;
};

#if _TARGET_64BIT
#define DDSX_GET_PLUGIN_PROC "get_plugin"
#else
#define DDSX_GET_PLUGIN_PROC "_get_plugin@4"
#endif
