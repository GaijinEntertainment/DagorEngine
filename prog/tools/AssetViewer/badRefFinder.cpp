#include "badRefFinder.h"
#include "av_appwnd.h"

#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <assets/assetRefs.h>

#include <de3_interface.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_busy.h>


BadRefFinder::BadRefFinder(DagorAssetMgr *assetMgr) : mAssetMgr(assetMgr), mBadRefCount(0)
{
  wingw::set_busy(true);
  findBadReferences();
  wingw::set_busy(false);

  if (mBadRefCount)
  {
    wingw::message_box(wingw::MBS_EXCL | wingw::MBS_OK, "Asset base", "Found %d invalid references! See console for details.",
      mBadRefCount);
    DAEDITOR3.conShow(true);
  }
  else
    wingw::message_box(wingw::MBS_OK, "Asset base", "Invalid references not found.");
}


void BadRefFinder::findBadReferences()
{
  dag::ConstSpan<DagorAsset *> assets = mAssetMgr->getAssets();

  for (int i = 0; i < assets.size(); i++)
  {
    DagorAsset *a = assets[i];
    if (a->getType() != mAssetMgr->getTexAssetTypeId()) // subtle optimization: textures don't reference anything now
      checkAssetReferences(a);
  }
}


void BadRefFinder::checkAssetReferences(DagorAsset *asset)
{
  IDagorAssetRefProvider *refProvider = mAssetMgr->getAssetRefProvider(asset->getType());
  if (!refProvider)
    return;

  dag::ConstSpan<IDagorAssetRefProvider::Ref> refs = refProvider->getAssetRefs(*asset);

  for (int i = 0; i < refs.size(); ++i)
  {
    DagorAsset *cur_asset = refs[i].getAsset();
    if (cur_asset)
      continue;

    if ((refs[i].flags & IDagorAssetRefProvider::RFLG_BROKEN) || !(refs[i].flags & IDagorAssetRefProvider::RFLG_OPTIONAL))
    {
      ++mBadRefCount;
      get_app().getConsole().addMessage(ILogWriter::ERROR, "asset '%s:%s' has invalid reference '%s'", asset->getName(),
        asset->getTypeStr(), refs[i].getBrokenRef());
    }
  }
}