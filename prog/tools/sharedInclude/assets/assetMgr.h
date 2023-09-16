//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_simpleString.h>
#include <EASTL/vector.h>


// forward declarations for external classes
class ILogWriter;
class DataBlock;
class DagorAsset;
class DagorAssetPrivate;
class DagorVirtualAssetRule;
class DagorAssetFolder;
class IDagorAssetMsgPipe;
class IDagorAssetChangeNotify;
class IDagorAssetBaseChangeNotify;
class IDagorAssetExporter;
class IDagorAssetRefProvider;
class FastIntList;
class String;


class DagorAssetMgr
{
public:
  explicit DagorAssetMgr(bool use_shared_name_map_for_assets = false);
  ~DagorAssetMgr();
  DagorAssetMgr(const DagorAssetMgr &) = delete;
  DagorAssetMgr &operator=(const DagorAssetMgr &) = delete;


  //! sets up list of allowed types for asset base from data block
  void setupAllowedTypes(const DataBlock &blk, const DataBlock *exp_blk = NULL);

  //! sets message pipe used to report problems during manager operations; returns previous pipe
  IDagorAssetMsgPipe *setMsgPipe(IDagorAssetMsgPipe *pipe);

  //! returns current message pipe
  IDagorAssetMsgPipe &getMsgPipe() const { return *msgPipe; }

  //! scans assets folder for *.res.blk and creates list of assets
  bool loadAssetsBase(const char *assets_folder, const char *name_space);

  //! gathers built resources list and mounts to /<mount_folder_name> with resources-by-type tree
  //! returns false, if mount folder already exists
  bool mountBuiltGameRes(const char *mount_folder_name, const DataBlock &skip_types);

  //! gathers built textures list and mounts to /<mount_folder_name>
  //! returns false, if mount folder already exists
  bool mountBuiltDdsxTex(const char *mount_folder_name);

  //! registers built resources using pre-gathered list with folders
  bool mountBuiltGameResEx(const DataBlock &list, const DataBlock &skip_types);

  //! gathers FMOD events and mounts to /<mount_folder_name> with native project/group tree
  //! returns false, if mount folder already exists
  bool mountFmodEvents(const char *mount_folder_name);

  //! starts mounting direct assets via adding folder <mount_folder_name> to specified parent folder;
  //! should be used before calling makeAssetDirect() to set last folder
  //! returns false, if mount folder already exists or parent folder is invalid
  bool startMountAssetsDirect(const char *folder_name, int parent_folder_idx = 0);
  //! finishes mounting direct assets
  void stopMountAssetsDirect() { updateGlobUniqueFlags(); }

  //! writes *.<TYPE>.blk to assets folder, filtering out those assets for which virtual blk is not changed
  //! non-modified assets are not saved to preserve timestamps (asset treated non-modified if its BLK in
  //! text form coinsides *exactly* with text file on disk, or with virtual BLK generated for this asset)
  bool saveAssetsMetadata(int folder_idx = -1);

  //! writes *.<TYPE>.blk for specified assets (applies only for non-virtual assets)
  //! non-modified assets are not saved to preserve timestamps
  bool saveAssetMetadata(DagorAsset &a);

  //! checks for any changed metadata in asset base
  bool isAssetsMetadataChanged();

  //! checks whether metadata for given asset is changed
  bool isAssetMetadataChanged(const DagorAsset &a);

  //! checks assets base consistence; if errors found returns false and reports errors in text form to logger
  bool checkConsistence(ILogWriter &log);

  //! clears representation of asset base
  void clear();


  //! enables/disables changes tracking (disabled by default; when disabled, no additional resourses are allocated)
  void enableChangesTracker(bool en);

  //! rescans full asset base and sends notifations about changed assets; returns true if any asset changed
  //! NOTE: implemented using incremental update; unchanged assets remain the same in memory
  bool rescanBase();

  //! performs one step of continuos file tracking; return true if some of checked assets is changed
  bool trackChangesContinuous(int assets_to_check = 1);


  //! add client to on-base-changed-notify receivers list (reciviers called before any IDagorAssetChangeNotify)
  void subscribeBaseUpdateNotify(IDagorAssetBaseChangeNotify *notify);

  //! remove client from on-base-changed-notify receivers list
  void unsubscribeBaseUpdateNotify(IDagorAssetBaseChangeNotify *notify);

  //! add client to on-change-notify receivers list
  void subscribeUpdateNotify(IDagorAssetChangeNotify *notify, int asset_name_id, int asset_type);

  //! remove client from on-change-notify receivers list (removes all client's entries, for any asset/type)
  void unsubscribeUpdateNotify(IDagorAssetChangeNotify *notify);

  void callAssetChangeNotifications(const DagorAsset &a, int aname, int atype) const;
  void callAssetBaseChangeNotifications(dag::ConstSpan<DagorAsset *> changed_assets, dag::ConstSpan<DagorAsset *> added_assets,
    dag::ConstSpan<DagorAsset *> removed_assets) const;


  //! returns array of assets (array itself cannot be modified, assets can be)
  dag::ConstSpan<DagorAsset *> getAssets() const { return assets; }

  //! returns number of assets in loaded base
  int getAssetCount() const { return assets.size(); }

  //! returns reference to asset by index
  DagorAsset &getAsset(int idx) const { return *assets[idx]; }


  //! finds asset by name and returns pointer to it or NULL if asset does not exsit
  DagorAsset *findAsset(const char *name) const;

  //! finds asset by name restricted by type and returns pointer to it or NULL if does not exist
  DagorAsset *findAsset(const char *name, int type_id) const;

  //! finds asset by name restricted by types and returns pointer to it or NULL if does not exist
  DagorAsset *findAsset(const char *name, dag::ConstSpan<int> types) const;


  //! returns pointer to root folder of assets base
  const DagorAssetFolder *getRootFolder() const { return folders.size() > 0 ? folders[0] : NULL; }

  //! returns pointer to any folder of assets base by index or NULL if does not exist
  const DagorAssetFolder *getFolderPtr(int folder_idx) const
  {
    return unsigned(folder_idx) < unsigned(folders.size()) ? folders[folder_idx] : NULL;
  }

  //! returns reference to any folder of assets base by index
  DagorAssetFolder &getFolder(int i)
  {
    G_ASSERT((unsigned)i < folders.size());
    return *folders[i];
  }

  //! gather folder indices which cointain assets for specified types (returns pointer to temporary buffer)
  dag::ConstSpan<int> getFilteredFolders(dag::ConstSpan<int> types);

  //! gather asset indices for specified types, optionally restricted by folder (returns pointer to temporary buffer)
  dag::ConstSpan<int> getFilteredAssets(dag::ConstSpan<int> types, int folder_idx = -1) const;

  //! returns range of asset indices for specified folder
  void getFolderAssetIdxRange(int folder_idx, int &out_start_idx, int &out_end_idx) const;

  //! returns true if asset unique (that is no other asset with the same name exists in base for given list of types)
  bool isAssetNameUnique(const DagorAsset &asset, dag::ConstSpan<int> types);


  //! returns custom package name assigned for folder (for pName "gameRes" or "ddsxTex")
  const char *getPackageName(int folder_idx, const char *pName, const char *target, const char *profile);

  //! returns custom pack name for type
  const char *getPackNameForType(int a_type, const char *def = NULL)
  {
    return (a_type >= 0 && a_type < perTypePack.size() && !perTypePack[a_type].empty()) ? perTypePack[a_type].str() : def;
  }

  //! returns custom package name for type
  const char *getPkgNameForType(int a_type)
  {
    return (a_type >= 0 && a_type < perTypePkg.size() && !perTypePkg[a_type].empty()) ? perTypePkg[a_type].str() : NULL;
  }

  //! returns pack name for folder (.dxp.bin is for_tex=true and .grp if for_tex=false)
  const char *getFolderPackName(int folder_idx, bool for_tex, const char *asset_pack_override, bool dabuild_collapse_packs = false,
    bool pure_name = false, int asset_type = -1);

  //! registers asset exporter
  bool registerAssetExporter(IDagorAssetExporter *exp);
  //! unregisters asset exporter
  bool unregisterAssetExporter(IDagorAssetExporter *exp);

  //! returns asset exporter registered for given asset type
  IDagorAssetExporter *getAssetExporter(int asset_type_id) const
  {
    if (asset_type_id < 0 || asset_type_id >= perTypeExp.size())
      return NULL;
    return perTypeExp[asset_type_id];
  }


  //! registers asset reference provider
  bool registerAssetRefProvider(IDagorAssetRefProvider *exp);
  //! unregisters asset reference provider
  bool unregisterAssetRefProvider(IDagorAssetRefProvider *exp);

  //! returns asset reference provider registered for given asset type
  IDagorAssetRefProvider *getAssetRefProvider(int asset_type_id) const
  {
    if (asset_type_id < 0 || asset_type_id >= perTypeRefProv.size())
      return NULL;
    return perTypeRefProv[asset_type_id];
  }

public:
  //! creates asset directly (without full database loading) using folder (for special virtual asset rules) and
  //! file_name (for .res.blk or to apply virtual asset rules)
  DagorAsset *loadAssetDirect(const char *folder, const char *file_name);

  //! creates asset directly using name and properties (added to the last folder; use startMountAssetsDirect() beforehand)
  DagorAsset *makeAssetDirect(const char *asset_name, const DataBlock &props, int atype);

  //! returns type name is string form by type ID
  const char *getAssetTypeName(int type_id) const { return typeNames.getName(type_id); }

  //! returns namespace from id
  const char *getAssetNameSpace(int nspace_id) const { return nspaceNames.getName(nspace_id); }

  //! returns type id by typename
  int getAssetTypeId(const char *type_name) const { return typeNames.getNameId(type_name); }

  //! returns type id for "tex" (cached!)
  int getTexAssetTypeId() const { return texAssetType; }

  //! returns count of allowed types
  int getAssetTypesCount() const { return typeNames.nameCount(); }

  //! returns namespace id by namespace name
  int getAssetNameSpaceId(const char *nspace) const { return nspaceNames.getNameId(nspace); }

  const char *getAssetName(int name_id) const { return assetNames.getName(name_id); }
  const char *getAssetFile(int file_id) const { return assetFileNames.getName(file_id); }

  //! returns nameId for given asset name; -1 means that asset with such name is surely not present in base
  int getAssetNameId(const char *name) const { return assetNames.getNameId(name); }

  const char *getDefDxpPath() const { return defDxpPath.empty() ? NULL : defDxpPath.str(); }
  const char *getDefGrpPath() const { return defGrpPath.empty() ? NULL : defGrpPath.str(); }
  const char *getLocDefDxp() const { return locDefDxp.empty() ? NULL : locDefDxp.str(); }
  const char *getLocDefGrp() const { return locDefGrp.empty() ? NULL : locDefGrp.str(); }
  const char *getBasePkg() const { return basePkg.empty() ? nullptr : basePkg.str(); }

  DataBlock *getSharedNm() { return sharedAssetNameMapOwner; }

protected:
  struct RootEntryRec;
  struct PerAssetIdNotifyTab;
  struct ChangesTracker;
  struct ClassidToAssetMap;
  struct AssetMap
  {
    unsigned name;
    unsigned type : 12, idx : 20;
    bool operator<(const AssetMap &b) const { return name < b.name; }
  };

  Tab<DagorAsset *> assets;
  Tab<DagorVirtualAssetRule *> vaRule;
  Tab<DagorAssetFolder *> folders;
  OAHashNameMap<true> assetNames, assetFileNames, typeNames, nspaceNames;
  mutable eastl::vector<AssetMap> amap;
  Tab<FastIntList> perTypeNameIds;
  Tab<int> perFolderStartAssetIdx;
  Tab<IDagorAssetExporter *> perTypeExp;
  Tab<IDagorAssetRefProvider *> perTypeRefProv;
  IDagorAssetMsgPipe *msgPipe;

  Tab<IDagorAssetBaseChangeNotify *> updBaseNotify;
  Tab<IDagorAssetChangeNotify *> updNotify;
  Tab<PerAssetIdNotifyTab> perTypeNotify;
  Tab<SimpleString> perTypePkg, perTypePack;

  Tab<RootEntryRec> baseRoots;
  ChangesTracker *tracker;
  int texAssetType;
  bool texRtMipGenAllowed;

  SimpleString defDxpPath, defGrpPath, locDefDxp, locDefGrp;
  SimpleString basePkg;
  DataBlock *sharedAssetNameMapOwner;

  void loadAssets(int parent_folder_idx, const char *folder_name, int nspace_id);
  void readFoldersBlk(DagorAssetFolder &f, const DataBlock &blk);

  bool addAsset(const char *folder_path, const char *fname, int nspace_id, DagorAssetPrivate *&ca, DagorAssetFolder *f, int fidx,
    int &start_rule_idx, bool reg);

  void fillGameResFolder(const char *type_name, int root_fidx, int nsid, dag::ConstSpan<String> names, unsigned res_classid);
  void fillGameResFolder(int pfidx, int nsid, const DataBlock &folder, dag::ConstSpan<ClassidToAssetMap> map);
  void addFmodAssets(int parent_fidx, const char *base_path, void *fmod_group);

  void updateGlobUniqueFlags();
  void syncTracker();
  bool saveAssetsMetadataImpl(int start_idx, int end_idx);

  inline eastl::vector<AssetMap>::const_iterator findAssetByName(unsigned name) const;
  void rebuildAssetMap() const;
};
