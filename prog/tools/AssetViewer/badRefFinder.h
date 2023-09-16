#pragma once

class DagorAssetMgr;
class DagorAsset;
class IDagorAssetRefProvider;

class BadRefFinder
{
public:
  BadRefFinder(DagorAssetMgr *assetMgr);

private:
  void findBadReferences();
  void checkAssetReferences(DagorAsset *asset);

  DagorAssetMgr *mAssetMgr;
  int mBadRefCount;
};
