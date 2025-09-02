//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabFwd.h>

class DagorAsset;
class ILogWriter;
class SimpleString;
class DataBlock;
namespace mkbindump
{
class BinDumpSaveCB;
}
namespace ddsx
{
struct Header;
}


// asset exporter plug-in component interface
class IDagorAssetExporter
{
public:
  virtual const char *__stdcall getExporterIdStr() const = 0;

  virtual const char *__stdcall getAssetType() const = 0;
  virtual unsigned __stdcall getGameResClassId() const = 0;
  virtual unsigned __stdcall getGameResVersion() const = 0;

  virtual void __stdcall onRegister() = 0;
  virtual void __stdcall onUnregister() = 0;

  //! sets destination BLK that can be used to output build results as <asset_name> blocks
  virtual void __stdcall setBuildResultsBlk(DataBlock *) {}

  //! fills list of files that data export depends on (if none of files changed since last export, re-export is not needed)
  virtual void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) = 0;

  //! checks whether asset may be exported (stub exporters will generally return false)
  virtual bool __stdcall isExportableAsset(DagorAsset &a) = 0;

  //! builds resource data and writes to output stream
  virtual bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log) = 0;

  //! fills ddsx::Header for texture with TQ/BQ/HQ/UHQ levels (expected to be superfast, avoids building tex)
  virtual bool __stdcall makeTexDDSxHeader(DagorAsset &, ddsx::Header & /*dest_hdr*/, unsigned & /*dest_lev_desc*/) { return false; }

  //! computes (using cache shared data) and returns hash of source data (to be used for pack naming)
  virtual bool getAssetSourceHash(SimpleString & /*dest_hash*/, DagorAsset &, void * /*cache_shared_data_ptr*/, unsigned /*tc*/)
  {
    return false;
  }

  //! tries to update asset desc (or returns false when full asset rebuild required)
  virtual bool updateBuildResultsBlk(DagorAsset &, void * /*cache_shared_data_ptr*/, unsigned /*tc*/) { return false; }
};
