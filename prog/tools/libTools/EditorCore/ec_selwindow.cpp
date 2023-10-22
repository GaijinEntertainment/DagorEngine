#include <EditorCore/ec_selwindow.h>
#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_interface_ex.h>

#include <generic/dag_sort.h>
#include <util/dag_globDef.h>

#include <libTools/util/strUtil.h>
#include <ioSys/dag_fileIo.h>

#include <sepGui/wndGlobal.h>
#include <3d/dag_drv3d.h>

using hdpi::_pxActual;
using hdpi::_pxScaled;

static int scr_width()
{
  int w = 0, h = 0;
  d3d::get_screen_size(w, h);
  return w;
}

// SelWindow buttons IDs
enum
{
  ID_NAME_MASK = 1,

  ID_NAMES_LIST,

  ID_SELECT_ALL,
  ID_SELECT_NONE,
  ID_SELECT_INVERT,

  ID_OBJ_TYPE
};


//==============================================================================
int str_compare(const String *s1, const String *s2) { return _stricmp(s1->str(), s2->str()); }


//==============================================================================
// SelWindow
//==============================================================================


SelWindow::SelWindow(void *phandle, IObjectsList *obj, const char *obj_list_owner_name) :

  typesBlk(NULL),
  objects(obj),
  objListName(obj_list_owner_name),
  typeNames(tmpmem),
  CDialogWindow(phandle, _pxActual(max(scr_width() / 3, hdpi::_pxS(600))), _pxScaled(426), "Select objects")
{
  ctorInit();
}


//==============================================================================
SelWindow::SelWindow(void *phandle, const EcRect &rect, IObjectsList *obj, const char *obj_list_owner_name) :

  typesBlk(NULL),
  objects(obj),
  objListName(obj_list_owner_name),
  typeNames(tmpmem),
  CDialogWindow(phandle, _pxActual(rect.r - rect.l), _pxActual(rect.b - rect.t), "Select objects")
{
  ctorInit();
}


//==============================================================================
void SelWindow::ctorInit()
{
  if (!typesBlk)
  {
    typesBlk = new DataBlock;
    FullFileLoadCB *crd = new FullFileLoadCB(::make_full_path(sgg::get_exe_path(), "../.local/selwindow.bin"));
    if (crd->fileHandle)
      typesBlk->loadFromStream(*crd);
    delete crd;
  }

  PropertyContainerControlBase *_panel = getPanel();
  G_ASSERT(_panel && "SelWindow::ctorInit(): NO PANEL FOUND!!!");

  Tab<String> vals(tmpmem);

  _panel->createEditBox(ID_NAME_MASK, "");
  _panel->createMultiSelectList(ID_NAMES_LIST, vals, _pxScaled(230));

  _panel->createButton(ID_SELECT_ALL, "All", true, true);
  _panel->createButton(ID_SELECT_NONE, "None", true, false);
  _panel->createButton(ID_SELECT_INVERT, "Invert", true, false);

  if (objects)
    objects->getTypeNames(typeNames);

  for (int i = 0; i < typeNames.size(); ++i)
    _panel->createCheckBox(ID_OBJ_TYPE + i, (const char *)typeNames[i], true, true, (i % 2) == 0);

  if (objListName)
  {
    DataBlock *objed_blk = typesBlk->getBlockByName(objListName);
    if (objed_blk && objed_blk->paramCount() > 1)
    {
      for (int i = 0; i < objed_blk->paramCount(); i += 2)
      {
        if (objed_blk->getParamType(i) != DataBlock::TYPE_STRING || objed_blk->getParamType(i + 1) != DataBlock::TYPE_BOOL)
          continue;

        const char *name = objed_blk->getStr(i);

        if (!name)
          continue;

        for (int j = 0; j < typeNames.size(); j++)
        {
          if (strcmp(typeNames[j], name) == 0)
          {
            _panel->setBool(ID_OBJ_TYPE + j, objed_blk->getBool(i + 1));
            break;
          }
        }
      }
    }
  }

  fillNames();
}

//==============================================================================
SelWindow::~SelWindow()
{
  if (objListName && typesBlk)
  {
    // typesBlk->setInt("xpos", getLeft());
    // typesBlk->setInt("ypos", getTop());
    // typesBlk->setInt("width", getW());
    // typesBlk->setInt("height", getH());

    DataBlock *objed_blk = typesBlk->getBlockByName(objListName);

    if (objed_blk)
      objed_blk->clearData();
    else
      objed_blk = typesBlk->addNewBlock(objListName);

    PropertyContainerControlBase *_panel = getPanel();
    G_ASSERT(_panel && "SelWindow::~SelWindow: NO PANEL FOUND!!!");

    for (int i = 0; i < typeNames.size(); ++i)
    {
      objed_blk->addStr("name", typeNames[i]);
      objed_blk->addBool("checked", _panel->getBool(ID_OBJ_TYPE + i));
    }

    FullFileSaveCB cwr(::make_full_path(sgg::get_exe_path(), "../.local/selwindow.bin"));
    if (cwr.fileHandle)
      typesBlk->saveToStream(cwr);
    delete typesBlk;
    typesBlk = NULL;
  }
}

//==============================================================================

void SelWindow::show()
{
  autoSize();
  CDialogWindow::show();

  getPanel()->setFocusById(ID_NAME_MASK);
}

//==============================================================================

void SelWindow::fillNames()
{
  int i;
  Tab<int> types(tmpmem);
  Tab<String> names(tmpmem);
  Tab<String> selNames(tmpmem);
  Tab<int> selection(tmpmem);

  PropertyContainerControlBase *_panel = getPanel();
  G_ASSERT(_panel && "SelWindow::fillNames: NO PANEL FOUND!!!");

  for (i = 0; i < typeNames.size(); ++i)
    if (_panel->getBool(ID_OBJ_TYPE + i))
      types.push_back(i);

  if (objects)
    objects->getObjNames(names, selNames, types);

  sort(names, &str_compare);

  _panel->setStrings(ID_NAMES_LIST, names);

  for (int i = 0; i < names.size(); ++i)
    for (int j = 0; j < selNames.size(); ++j)
      if (names[i] == selNames[j])
        selection.push_back(i);

  _panel->setSelection(ID_NAMES_LIST, selection);
}


//==============================================================================
void SelWindow::getSelectedNames(Tab<String> &names)
{
  PropertyContainerControlBase *_panel = getPanel();
  G_ASSERT(_panel && "SelWindow::getSelectedNames: NO PANEL FOUND!!!");

  clear_and_shrink(names);

  Tab<int> sel(tmpmem);
  Tab<String> list(tmpmem);

  _panel->getSelection(ID_NAMES_LIST, sel);
  _panel->getStrings(ID_NAMES_LIST, list);

  for (int i = 0; i < sel.size(); ++i)
    names.push_back(list[sel[i]]);
}


//==============================================================================

void SelWindow::onClick(int pcb_id, PropertyContainerControlBase *panel)
{
  if (!panel)
    return;

  Tab<int> sels(tmpmem);
  panel->getSelection(ID_NAMES_LIST, sels);
  int _count = panel->getInt(ID_NAMES_LIST);

  switch (pcb_id)
  {
    case ID_SELECT_NONE:
      clear_and_shrink(sels);
      panel->setSelection(ID_NAMES_LIST, sels);
      break;

    case ID_SELECT_ALL:
      clear_and_shrink(sels);
      for (int i = 0; i < _count; ++i)
        sels.push_back(i);
      panel->setSelection(ID_NAMES_LIST, sels);
      break;

    case ID_SELECT_INVERT:
    {
      bool *flags = new bool[_count];

      for (int i = 0; i < _count; ++i)
        flags[i] = true;

      for (int i = 0; i < sels.size(); ++i)
        flags[sels[i]] = false;

      clear_and_shrink(sels);
      for (int i = 0; i < _count; ++i)
        if (flags[i])
          sels.push_back(i);

      delete[] flags;
      panel->setSelection(ID_NAMES_LIST, sels);
    }
    break;

    default:
    {
      const int index = pcb_id - ID_OBJ_TYPE;
      if (index >= 0 && index < typeNames.size())
        fillNames();
    }
    break;
  }
}


//==============================================================================

void SelWindow::onDoubleClick(int pcb_id, PropertyContainerControlBase *panel)
{
  switch (pcb_id)
  {
    case ID_NAMES_LIST: click(DIALOG_ID_OK); break;
  }
}

//==============================================================================

void SelWindow::onChange(int pcb_id, PropertyContainerControlBase *panel)
{
  switch (pcb_id)
  {
    case ID_NAME_MASK:
    {
      panel->setText(ID_NAMES_LIST, panel->getText(ID_NAME_MASK).str());
    }
    break;
  }
}
