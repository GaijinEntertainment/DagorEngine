#include "de_ExportToDagDlg.h"
#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>
#include <sepGui/wndPublic.h>
#include <de3_entityFilter.h>

#include <debug/dag_debug.h>


//==============================================================================
ExportToDagDlg::ExportToDagDlg(Tab<String> &sel_names, bool visual, const char *name) :
  CDialogWindow(DAGORED2->getWndManager()->getMainWindow(), hdpi::_pxScaled(280), hdpi::_pxScaled(400), name), plugs(midmem)
{
  mDPanel = getPanel();
  G_ASSERT(mDPanel && "ExportToDagDlg::ExportToDagDlg: NO PANEL FOUND!");

  int mask = visual ? IObjEntityFilter::STMASK_TYPE_RENDER : IObjEntityFilter::STMASK_TYPE_COLLISION;
  for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
  {
    IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);

    if (!plugin)
      continue;

    IGatherStaticGeometry *geom = plugin->queryInterface<IGatherStaticGeometry>();
    IObjEntityFilter *filter = plugin->queryInterface<IObjEntityFilter>();
    if (filter && !filter->allowFiltering(mask))
      filter = NULL;
    if (visual && filter && !filter->allowFiltering(IObjEntityFilter::STMASK_TYPE_EXPORT))
      filter = NULL;

    if (geom || filter)
    {
      plugs.push_back(i);
      mDPanel->createCheckBox(FIRST_CHECK_ID + i, plugin->getMenuCommandName());

      for (int i = 0; i < sel_names.size(); ++i)
        if (!strcmp(sel_names[i], plugin->getMenuCommandName()))
          mDPanel->setBool(FIRST_CHECK_ID + i, true);
    }
  }
}

//==============================================================================
void ExportToDagDlg::getSelPlugins(Tab<int> &sel)
{
  clear_and_shrink(sel);

  for (int i = 0; i < plugs.size(); ++i)
    if (mDPanel->getBool(FIRST_CHECK_ID + plugs[i]))
      sel.push_back(plugs[i]);
}
