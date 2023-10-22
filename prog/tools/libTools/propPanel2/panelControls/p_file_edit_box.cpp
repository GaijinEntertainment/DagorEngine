// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_file_edit_box.h"
#include "../c_constants.h"
#include <winGuiWrapper/wgw_dialogs.h>
#include <osApiWrappers/dag_direct.h>
#include <libTools/util/strUtil.h>

CFileEditBox::CFileEditBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  const char caption[]) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, _pxScaled(DEFAULT_CONTROL_HEIGHT) * 2),

  mCaption(this, parent->getWindow(), x, y, _px(w), _pxS(DEFAULT_CONTROL_HEIGHT)),

  mEdit(this, parent->getWindow(), x, y + _pxS(DEFAULT_CONTROL_HEIGHT),
    _px(w) - 2 * _pxS(DEFAULT_CONTROL_HEIGHT) - _pxS(DEFAULT_CONTROLS_INTERVAL), _pxS(DEFAULT_CONTROL_HEIGHT)),

  mButton(this, parent->getWindow(), x + mEdit.getWidth() + _pxS(DEFAULT_CONTROLS_INTERVAL), y + _pxS(DEFAULT_CONTROL_HEIGHT),
    2 * _pxS(DEFAULT_CONTROL_HEIGHT), _pxS(DEFAULT_CONTROL_HEIGHT)),

  m_DialogMode(FS_DIALOG_OPEN_FILE),
  manualChange(false),
  basePath("")
{
  hasCaption = strlen(caption) > 0;
  mCaption.setTextValue(caption);
  mButton.setTextValue("...");
  strcpy(mMasks, "All|*.*||");

  if (!hasCaption)
  {
    mCaption.hide();
    mH = mEdit.getHeight();
    this->moveTo(x, y);
  }
  initTooltip(&mEdit);
}


PropertyContainerControlBase *CFileEditBox::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  parent->createFileEditBox(id, caption, "", true, new_line);
  return NULL;
}


void CFileEditBox::setEnabled(bool enabled)
{
  mCaption.setEnabled(enabled);
  mEdit.setEnabled(enabled);
  mButton.setEnabled(enabled);
}


void CFileEditBox::reset()
{
  this->setTextValue("");
  PropertyControlBase::reset();
}


void CFileEditBox::setWidth(hdpi::Px w)
{
  int minw = 2 * _pxS(DEFAULT_CONTROL_HEIGHT) + _pxS(DEFAULT_CONTROLS_INTERVAL);
  w = (_px(w) < minw) ? _pxActual(minw) : w;

  PropertyControlBase::setWidth(w);

  mCaption.resizeWindow(_px(w), mCaption.getHeight());
  mEdit.resizeWindow(_px(w) - 2 * _pxS(DEFAULT_CONTROL_HEIGHT) - _pxS(DEFAULT_CONTROLS_INTERVAL), mEdit.getHeight());
  mButton.resizeWindow(2 * _pxS(DEFAULT_CONTROL_HEIGHT), mButton.getHeight());

  this->moveTo(this->getX(), this->getY());
}


void CFileEditBox::setTextValue(const char value[])
{
  manualChange = true;
  mEdit.setTextValue(value);
  manualChange = false;
}


void CFileEditBox::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }

void CFileEditBox::setBoolValue(bool value)
{
  this->setIntValue(value ? FS_DIALOG_DIRECTORY : FS_DIALOG_OPEN_FILE);
  // G_ASSERT(false && "Don't use setBoolValue with CFileEditBo. See setIntValue");
}


void CFileEditBox::setIntValue(int value) { m_DialogMode = value; }


void CFileEditBox::setStringsValue(const Tab<String> &vals)
{
  int c = vals.size();
  strcpy(mMasks, "\0");

  for (int i = 0; (i < c) && (strlen(mMasks) + vals[i].size() < FILTER_STRING_SIZE - 2); ++i)
  {
    strcat(mMasks, vals[i]);
    strcat(mMasks, "|");
  }

  strcat(mMasks, "|");
}


void CFileEditBox::setUserDataValue(const void *value)
{
  if (value)
    basePath = (const char *)value;
}


int CFileEditBox::getTextValue(char *buffer, int buflen) const { return mEdit.getTextValue(buffer, buflen); }


void CFileEditBox::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mCaption.moveWindow(x, y);
  mEdit.moveWindow(x, (hasCaption) ? y + mCaption.getHeight() : y);
  mButton.moveWindow(x + mEdit.getWidth() + _pxS(DEFAULT_CONTROLS_INTERVAL), (hasCaption) ? y + mCaption.getHeight() : y);
}


void CFileEditBox::onWcChange(WindowBase *source)
{
  if (!manualChange)
    PropertyControlBase::onWcChange(source);
}


void CFileEditBox::onWcClick(WindowBase *source)
{
  if (source == &mButton)
  {
    char fn[512] = "\0";

    if (m_DialogMode == FS_DIALOG_NONE)
    {
      PropertyControlBase::onWcClick(source);
      onWcChange(&mEdit);
      return;
    }
    else
      PropertyControlBase::onWcChanging(source); // for change pathes if it needs

    mEdit.getTextValue(fn, sizeof(fn));

    SimpleString fname, full_fn, result;
    char floc[260];

    if (!basePath.empty())
    {
      full_fn = ::make_full_path(basePath.str(), fn);
      fname = dd_get_fname(full_fn.str());
      dd_get_fname_location(floc, full_fn.str());
    }
    else
    {
      full_fn = fn;
      fname = dd_get_fname(fn);
      dd_get_fname_location(floc, fn);
    }

    switch (m_DialogMode)
    {
      case FS_DIALOG_SAVE_FILE:
        result = wingw::file_save_dlg(this->getRootParent()->getWindowHandle(), "Save file as...", mMasks, "", floc, fname);
        break;

      case FS_DIALOG_DIRECTORY:
        result = wingw::dir_select_dlg(this->getRootParent()->getWindowHandle(), "Open folder...", full_fn.str());
        break;

      default: m_DialogMode = FS_DIALOG_OPEN_FILE;

      case FS_DIALOG_OPEN_FILE:
        result = wingw::file_open_dlg(this->getRootParent()->getWindowHandle(), "Open file...", mMasks, "", floc, fname);
        break;
    }

    if (!result.empty())
    {
      if (!basePath.empty())
      {
        String rel_fn = ::make_path_relative(result.str(), basePath.str());
        result = rel_fn;
      }

      this->setTextValue(result.str());
      onWcChange(&mEdit);
    }
  }
}
