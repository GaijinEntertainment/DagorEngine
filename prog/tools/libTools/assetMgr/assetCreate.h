// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <assets/asset.h>

// add-nonvirtual-functions-only class derived from DagorAsset to facilitate asset creation
class DagorAssetPrivate : public DagorAsset
{
public:
  DagorAssetPrivate(DagorAssetMgr &m) : DagorAsset(m) { props.setSharedNameMapAndClearData(m.getSharedNm()); }
  DagorAssetPrivate(DagorAsset &a, int name_id, int nsid) : DagorAsset(a.getMgr())
  {
    props.setSharedNameMapAndClearData(a.getMgr().getSharedNm());
    props = a.props;
    folderIdx = a.getFolderIndex();
    assetType = a.getType();
    nameId = name_id;
    nspaceId = nsid;
    fileNameId = a.getFileNameId();
    globUnique = a.isGloballyUnique();
    virtualBlk = a.isVirtual();
  }

  void setNames(int asset_nid, int nspace_id, bool va)
  {
    nspaceId = nspace_id;
    nameId = asset_nid;
    virtualBlk = va;
  }

  bool loadResBlk(int asset_nid, const char *fname, int nspace_id)
  {
    nspaceId = nspace_id;
    nameId = asset_nid;
    virtualBlk = false;
    return props.load(fname);
  }

  void setAssetData(int folder_idx, int file_nid, int asset_type)
  {
    folderIdx = folder_idx;
    fileNameId = file_nid;
    assetType = asset_type;
  }

  void setGlobUnique(bool unique) { globUnique = unique ? 1 : 0; }
};
