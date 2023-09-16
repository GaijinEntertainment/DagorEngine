#include "compositeAssetCreator.h"

#include "assetBuildCache.h"
#include "av_appwnd.h"

#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_files.h>
#include <propPanel2/comWnd/dialog_window.h>

namespace
{

class NewCompositeNamePickerDialog : public CDialogWindow
{
public:
  NewCompositeNamePickerDialog(void *phandle, unsigned w, unsigned h) : CDialogWindow(phandle, w, h, "Create composit")
  {
    mPropertiesPanel->createEditBox(NAME_CONTROL_ID, "Name");
    mPropertiesPanel->createStatic(ERROR_LINE1_CONTROL_ID, "");
    mPropertiesPanel->createStatic(ERROR_LINE2_CONTROL_ID, "");
    mButtonsPanel->setText(DIALOG_ID_OK, "Create");
  }

  const String &getPickedName() const { return pickedName; }

private:
  virtual void show() override
  {
    CDialogWindow::show();

    getPanel()->setFocusById(NAME_CONTROL_ID);
  }

  virtual bool onOk() override
  {
    String name(getPanel()->getText(NAME_CONTROL_ID).c_str());
    trim(name);
    if (name.length() <= 0)
    {
      getPanel()->setText(ERROR_LINE1_CONTROL_ID, "The name cannot be empty!");
      getPanel()->setText(ERROR_LINE2_CONTROL_ID, "");
      return false;
    }

    String identName = name;
    make_ident_like_name(identName);
    if (identName != name)
    {
      getPanel()->setText(ERROR_LINE1_CONTROL_ID, "Allowed characters: a-z, A-Z, 0-9 and _!");
      getPanel()->setText(ERROR_LINE2_CONTROL_ID, "");
      return false;
    }

    if (get_app().getAssetMgr().findAsset(name))
    {
      String nameWithCmpSuffix(name);
      nameWithCmpSuffix += "_cmp";

      if (name.suffix("_cmp") || get_app().getAssetMgr().findAsset(nameWithCmpSuffix))
      {
        getPanel()->setText(ERROR_LINE1_CONTROL_ID, "An asset already exists with this name!");
        getPanel()->setText(ERROR_LINE2_CONTROL_ID, "");
      }
      else
      {
        getPanel()->setText(ERROR_LINE1_CONTROL_ID, "An asset already exists with this name. Try the following:");
        getPanel()->setText(ERROR_LINE2_CONTROL_ID, nameWithCmpSuffix);
      }

      return false;
    }

    pickedName = name;
    return true;
  }

  enum
  {
    NAME_CONTROL_ID = 1,
    ERROR_LINE1_CONTROL_ID,
    ERROR_LINE2_CONTROL_ID,
  };

  String pickedName;
};

} // namespace

String CompositeAssetCreator::pickName()
{
  eastl::unique_ptr<NewCompositeNamePickerDialog> dialog = eastl::make_unique<NewCompositeNamePickerDialog>(nullptr, 400, 180);
  if (dialog->showDialog() != DIALOG_ID_OK)
    return String();

  return dialog->getPickedName();
}

DagorAsset *CompositeAssetCreator::create(const String &asset_name, const String &asset_full_path)
{
  file_ptr_t file = df_open(asset_full_path.c_str(), DF_CREATE | DF_WRITE);
  if (!file)
    return nullptr;

  df_printf(file, "className:t=\"composit\"\n");
  df_close(file);

  DagorAssetMgr &assetMgr = const_cast<DagorAssetMgr &>(get_app().getAssetMgr());

  // Let asset manager detect the creation of the new composite asset file.
  assetMgr.trackChangesContinuous(-1);
  post_base_update_notify_dabuild();

  return assetMgr.findAsset(asset_name);
}
