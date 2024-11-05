//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_dataBlock.h>


class DagorAssetMgr;


class DagorAsset
{
public:
  int getType() const { return assetType; }
  int getFolderIndex() const { return folderIdx; }
  int getNameId() const { return nameId; }
  int getFileNameId() const { return fileNameId; }
  int getNameSpaceId() const { return nspaceId; }
  bool isVirtual() const { return virtualBlk; }
  bool isGloballyUnique() const { return globUnique; }

  DagorAssetMgr &getMgr() const { return mgr; }

  const char *getName() const;
  const char *getNameSpace() const;
  const char *getSrcFileName() const;
  const char *getTypeStr() const;
  const char *getFolderPath() const;

  //! returns getFolderPath()+"/"+getSrcFileName()
  String getSrcFilePath() const;

  //! returns getFolderPath()+"/"+props.getStr("name", getSrcFileName())
  String getTargetFilePath() const;

  //! returns getName()+":"+getTypeStr()
  String getNameTypified() const;

  //! checks whether metadata of asset changed from source file on disk
  bool isMetadataChanged() const;

  //! bit operations with user flags
  void resetUserFlags() { userFlags = 0; }
  void setUserFlags(int mask) { userFlags |= mask; }
  void clrUserFlags(int mask) { userFlags &= ~mask; }
  int testUserFlags(int mask) { return userFlags & mask; }

  //! returns profile/target related block (does ordered search and returns first match);
  //! 1. if profile != NULL, "profile~target" block is searched first
  //! 2. "target" block is searched next
  //! 3. if neither block present, returns pointer to props
  const DataBlock &getProfileTargetProps(unsigned target, const char *profile = NULL) const;

  //! returns effective profile/target block name
  //! 1. if profile != NULL AND "profile~target" block exists, name of this block is returned
  //! 2. otherwise, target_str is returned
  const char *resolveEffProfileTargetStr(const char *target_str, const char *profile) const;

  //! returns custom package name for this asset
  const char *getCustomPackageName(const char *target, const char *profile, bool full = true);

  //! returns destination pack name
  const char *getDestPackName(bool dabuild_collapse_packs = false, bool pure_name = false, bool allow_grouping = true);

  //! returns string where assetName is constructed from filepath reference
  //! (folder path and 3-letter extension are removed, e.g. "c:\my\tex.dds" - > "tex")
  static String fpath2asset(const char *fpath);

public:
  DataBlock props;

protected:
  int nameId, fileNameId;
  short folderIdx;
  short assetType;
  short nspaceId;
  unsigned short globUnique : 1, virtualBlk : 1, userFlags : 14;
  DagorAssetMgr &mgr;

  DagorAsset(DagorAssetMgr &m) :
    mgr(m), folderIdx(-1), assetType(-1), nameId(-1), nspaceId(-1), fileNameId(-1), globUnique(0), virtualBlk(0), userFlags(0)
  {}
};
