// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <propPanel2/c_panel_placement.h>

#include "c_constants.h"
#include "c_window_base.h"

#include <debug/dag_debug.h>

//--------------------------- Vertical Placement ------------------------------


PropertyContainerVert::PropertyContainerVert(int id, ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int x,
  int y, unsigned w, unsigned h, HorzFlow horzFlow)

  :
  PropertyContainerControlBase(id, event_handler, parent, x, y, w, h),
  mVInterval(DEFAULT_CONTROLS_INTERVAL),
  m_BlockResizeCallback(false),
  m_BlockFillAutoResize(false),
  horzFlow(horzFlow)
{}


void PropertyContainerVert::setWidth(unsigned w)
{
  // PropertyControlBase::setWidth(w);

  int c = mControlArray.size();
  m_BlockResizeCallback = true;

  if (c)
  {
    setNewLine();

    int fi, li;
    unsigned all_width, cur_width, cur_x, cur_y;

    cur_width = all_width = this->getClientWidth();
    cur_x = cur_y = 0;
    fi = li = 0;

    for (fi = 0; fi < c; ++fi)
    {
      cur_y = mControlArray[fi]->getY();
      if (mControlsNewLine[fi])
      {
        for (li = fi + 1; (li < c) && (!mControlsNewLine[li]); ++li)
          ; // search on this line

        // width and pos calc
        cur_width = (li - fi > 1) ? (all_width - DEFAULT_CONTROLS_INTERVAL * (li - fi - 1)) / (li - fi) : all_width;
        cur_x = this->getNextControlX(true);

        if (horzFlow == HorzFlow::Enabled && fi > 0)
        {
          int maxHeight = 0;
          for (int i = fi - 1; i >= 0; --i)
          {
            if (mControlArray[i]->getHeight() > maxHeight)
              maxHeight = mControlArray[i]->getHeight();
            if (mControlsNewLine[i])
              break;
          }
          cur_y = mControlArray[fi - 1]->getY() + maxHeight + getVerticalInterval(fi, mControlsNewLine[fi]);
        }
      }
      else
      {
        cur_x = DEFAULT_CONTROLS_INTERVAL + ((fi) ? this->mControlArray[fi - 1]->getX() + this->mControlArray[fi - 1]->getWidth() : 0);
        if (horzFlow == HorzFlow::Enabled)
          cur_y = mControlArray[fi - 1]->getY();
      }

      mControlArray[fi]->setWidth(cur_width);
      mControlArray[fi]->moveTo(cur_x, cur_y);
    }
  }

  m_BlockResizeCallback = false;
}


void PropertyContainerVert::onChildResize(int id)
{
  // move all conrols on resize

  if (m_BlockResizeCallback || m_BlockFillAutoResize)
    return;

  int index = 0;
  int x, y;
  bool changed = false;

  for (int i = 0; i < mControlArray.size(); i++)
  {
    if (mControlArray[i]->getID() == id)
    {
      index = i;
      changed = true;
      break;
    }
  }

  for (int i = index + 1; i < mControlArray.size(); i++)
  {
    x = mControlArray[i]->getX();
    y = mControlArray[i - 1]->getY();

    unsigned maxheight = mControlArray[i - 1]->getHeight();
    if (!mControlsNewLine[i - 1])
      for (int j = i - 2; j >= 0; --j)
      {
        if (maxheight < mControlArray[j]->getHeight())
          maxheight = mControlArray[j]->getHeight();
        if (mControlsNewLine[j])
          break;
      }

    if (mControlsNewLine[i])
      y += maxheight + getVerticalInterval(i, mControlsNewLine[i]);

    mControlArray[i]->moveTo(x, y);
  }

  if (changed)
  {
    unsigned int w = this->getWidth();
    int h = this->getNextControlY();
    // workaround for panel is not invalidated when dimensions aren't changed
    if (horzFlow == HorzFlow::Enabled && w == mW && h == mH)
      this->resizeControl(w, h + 1);
    this->resizeControl(w, h);
  }
}


void PropertyContainerVert::clear()
{
  __super::clear();
  this->resizeControl(this->getWidth(), this->getNextControlY());
}


void PropertyContainerVert::addControl(PropertyControlBase *pcontrol, bool new_line)
{
  mControlsNewLine.push_back(new_line);
  mControlArray.push_back(pcontrol);
  pcontrol->enableChangeHandlers();


  setNewLine();
  new_line = mControlsNewLine.back();

  // --- for old line ---

  if ((!new_line) && (mControlsNewLine.size() > 1))
  {
    int fi, li;
    fi = li = mControlsNewLine.size() - 1;

    for (fi = li - 1; ((fi >= 0) && (!mControlsNewLine[fi])); --fi)
      ;

    unsigned nw = (this->getClientWidth() - DEFAULT_CONTROLS_INTERVAL * (li - fi)) / (li - fi + 1);
    for (int i = fi; i < li + 1; ++i)
    {
      mControlArray[i]->setWidth(nw);
      mControlArray[i]->moveTo(DEFAULT_CONTROLS_INTERVAL + (DEFAULT_CONTROLS_INTERVAL + nw) * (i - fi), mControlArray[i]->getY());
    }
  }

  //---------------------

  if (m_BlockFillAutoResize)
  {
    PropPanel2 *_pc = pcontrol->getContainer();
    if (_pc)
      _pc->disableFillAutoResize();
    return;
  }

  this->onControlAdd(pcontrol);
}


int PropertyContainerVert::getNextControlX(bool new_line)
{
  int c = mControlArray.size();
  if ((c) && (!new_line))
  {
    return mControlArray[c - 1]->getX() + mControlArray[c - 1]->getWidth() + DEFAULT_CONTROLS_INTERVAL;
  }
  return DEFAULT_CONTROLS_INTERVAL;
}


int PropertyContainerVert::getNextControlY(bool new_line)
{
  int c = mControlArray.size();

  if (c)
  {
    if (new_line)
    {
      unsigned maxheight = mControlArray[c - 1]->getHeight();
      if (!mControlsNewLine[c - 1])
        for (int i = c - 2; i >= 0; --i)
        {
          if (maxheight < mControlArray[i]->getHeight())
            maxheight = mControlArray[i]->getHeight();
          if (mControlsNewLine[i])
            break;
        }

      return mControlArray[c - 1]->getY() + getVerticalInterval(c - 1, new_line) + maxheight;
    }
    else
      return mControlArray[c - 1]->getY();
  }
  return getVerticalInterval(-1, new_line);
}


// --- fill without resizes

void PropertyContainerVert::disableFillAutoResize() { m_BlockFillAutoResize = true; }


void PropertyContainerVert::restoreFillAutoResize()
{
  int cnt = mControlArray.size();

  for (int i = 0; i < cnt; i++)
  {
    PropPanel2 *_pc = mControlArray[i]->getContainer();
    if (_pc)
      _pc->restoreFillAutoResize();
  }

  m_BlockFillAutoResize = false;
  if (cnt > 0)
    this->onChildResize(mControlArray[0]->getID());

  // this->resizeControl(this->getWidth(), this->getNextControlY());
}

void PropertyContainerVert::enableResizeCallback()
{
  m_BlockResizeCallback = false;
  if (PropPanel2 *pc = getParent())
    pc->enableResizeCallback();
}

void PropertyContainerVert::disableResizeCallback()
{
  m_BlockResizeCallback = true;
  if (PropPanel2 *pc = getParent())
    pc->disableResizeCallback();
}

void PropertyContainerVert::setNewLine()
{
  if (horzFlow == HorzFlow::Disabled || mControlArray.empty())
    return;
  unsigned int width = getClientWidth();
  int startX = DEFAULT_CONTROLS_INTERVAL + mControlArray[0]->getX();
  for (int i = 1; i < mControlArray.size(); ++i)
  {
    int curX = startX + defaultControlWidth;
    mControlsNewLine[i] = int(width - curX) < defaultControlWidth;
    if (mControlsNewLine[i])
      startX = DEFAULT_CONTROLS_INTERVAL;
    else
      startX += DEFAULT_CONTROLS_INTERVAL + defaultControlWidth;
  }
}

//--------------------------- Horizontal Placement ----------------------------


void PropertyContainerHorz::onChildResize(int id)
{
  int fi, li, new_x;
  int c = mControlArray.size();

  for (fi = 0; (fi < c) && (mControlArray[fi]->getID() != id); ++fi)
    ;

  if (fi < c)
  {
    for (li = fi + 1; li < c; ++li)
    {
      new_x = mControlArray[li - 1]->getX() + mControlArray[li - 1]->getWidth() + DEFAULT_CONTROLS_INTERVAL;

      mControlArray[li]->moveTo(new_x, mControlArray[li]->getY());
    }
  }

  resizeControl(this->getNextControlX(), this->getNextControlY(false));
}


int PropertyContainerHorz::getNextControlX(bool new_line)
{
  int c = mControlArray.size();
  if (c)
  {
    return mControlArray[c - 1]->getX() + mControlArray[c - 1]->getWidth() + DEFAULT_CONTROLS_INTERVAL;
  }

  return DEFAULT_TOOLWINDOW_IDENT;
}


int PropertyContainerHorz::getNextControlY(bool new_line)
{
  int c = mControlArray.size();
  unsigned maxheight = getHeight();

  if (new_line)
  {
    return 0;
  }

  for (int i = 0; i < c; ++i)
  {
    if (maxheight < mControlArray[i]->getHeight())
      maxheight = mControlArray[i]->getHeight();
  }

  return maxheight;
}


unsigned PropertyContainerHorz::getClientWidth()
{
  WindowBase *win = this->getWindow();
  if (win)
  {
    int result = win->getWidth() - 2 * DEFAULT_TOOLWINDOW_IDENT;
    return (result > 0) ? result : 0;
  }

  debug("PropertyContainerHorz::getClientWidth(): getWindow() == NULL!");
  return 0;
}


void PropertyContainerHorz::addControl(PropertyControlBase *pcontrol, bool new_line)
{
  mControlsNewLine.push_back(new_line);
  mControlArray.push_back(pcontrol);
  pcontrol->enableChangeHandlers();

  // --- for toolbar controls standard width ---

  int c = mControlsNewLine.size();
  if (c > 0)
  {
    mControlArray[c - 1]->setWidth(DEFAULT_TOOLBAR_CONTROL_WIDTH);

    // vertical auto-align

    int ay = (getHeight() - mControlArray[c - 1]->getHeight()) / 2;
    mControlArray[c - 1]->moveTo(mControlArray[c - 1]->getX(), ay);
  }

  //---------------------

  this->onControlAdd(pcontrol);
}


void PropertyContainerHorz::onControlAdd(PropertyControlBase *control)
{
  if (control)
  {
    resizeControl(this->getNextControlX(), this->getNextControlY(false));
  }
}
