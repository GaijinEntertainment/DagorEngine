// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "gradientControlStandalone.h"
#include <propPanel/commonWindow/colorDialog.h>
#include "../c_constants.h"
#include <math/dag_rectInt.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

using namespace hdpi;

namespace PropPanel
{

// -----------------Track Gradient Button --------------------------

TrackGradientButton::TrackGradientButton(WindowControlEventHandler *event_handler, E3DCOLOR color, bool moveable,
  GradientControlStandalone *owner, float _value) :
  mEventHandler(event_handler), value(0)
{
  mOwner = owner;
  value = _value > 1.0f ? 1.0f : _value;
  value = value < 0.0f ? 0.0f : value;
  mColor = color;
  canMove = moveable;
  useful = true;
  firstDragMsg = true;
  moving = false;

  updateTooltipText();
}

void TrackGradientButton::showColorPicker()
{
  if (!moving)
  {
    E3DCOLOR _color = mColor;
    String name;
    static int i = 1;
    name.printf(32, "Select color %d", i++);
    if (ColorDialog::select_color(nullptr, name, _color))
    {
      if (mEventHandler)
        mEventHandler->onWcChanging(windowBaseForEventHandler);

      mColor = _color;
      mOwner->updateCycled(this);

      if (mEventHandler)
        mEventHandler->onWcChange(windowBaseForEventHandler);
    }
  }

  moving = false;
  firstDragMsg = true;
  updateTooltipText();
}

intptr_t TrackGradientButton::onDrag(int new_x, int new_y)
{
  G_UNUSED(new_y);

  if (firstDragMsg)
  {
    firstDragMsg = false;
    return 0;
  }

  int leftLimit, rightLimit;
  mOwner->getLimits(this, leftLimit, rightLimit);

  if (new_x < leftLimit + _pxS(TRACK_GRADIENT_BUTTON_WIDTH))
    new_x = leftLimit + _pxS(TRACK_GRADIENT_BUTTON_WIDTH);
  if (new_x > rightLimit - _pxS(TRACK_GRADIENT_BUTTON_WIDTH))
    new_x = rightLimit - _pxS(TRACK_GRADIENT_BUTTON_WIDTH);

  if (canMove)
  {
    float valueNew = (float)(new_x) / (float)(mOwner->getGradientDisplayWidth());
    valueNew = (valueNew > 1.0f) ? 1.0f : (valueNew < 0.0f) ? 0.0f : valueNew;

    if (valueNew != value)
    {
      if (!moving && mEventHandler)
        mEventHandler->onWcChanging(windowBaseForEventHandler);

      value = valueNew;

      updateTooltipText();

      if (mEventHandler)
        mEventHandler->onWcChange(windowBaseForEventHandler);
    }
  }

  moving = true;

  return 0;
}

void TrackGradientButton::cancelMoving()
{
  moving = false;
  firstDragMsg = true;
  updateTooltipText();
}

void TrackGradientButton::markForDeletion()
{
  if (!canMove || !mOwner->canRemove())
    return;

  useful = false;
}

int TrackGradientButton::getPosInGradientDisplaySpace() const { return (getValue() * mOwner->getGradientDisplayWidth()); }

void TrackGradientButton::setValue(float new_value)
{
  value = new_value;
  value = value > 1.0f ? 1.0f : value;
  value = value < 0.0f ? 0.0f : value;
}

void TrackGradientButton::updateTooltipText()
{
  if (moving)
    tooltipText.clear();
  else
    tooltipText.printf(128, "Position: %1.3f, RGB: (%d, %d, %d)", getValue(), mColor.r, mColor.g, mColor.b);
}

bool TrackGradientButton::updateImgui(const Point2 &view_offset, const Point2 &mouse_pos, bool gradient_control_hovered)
{
  const Point2 cur_pos(getPosInGradientDisplaySpace() + (hdpi::_pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2),
    hdpi::_pxS(TRACK_GRADIENT_BUTTON_HEIGHT));
  const int mx = hdpi::_pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2;
  const int my = hdpi::_pxS(TRACK_GRADIENT_BUTTON_HEIGHT) - 1;
  const ImVec2 point1(cur_pos.x, cur_pos.y - 1);
  const ImVec2 point2(cur_pos.x - mx, cur_pos.y - my);
  const ImVec2 point3(cur_pos.x + mx - 1, cur_pos.y - my);
  const bool hovered = gradient_control_hovered && ImRect(point2.x, point2.y, point3.x, point1.y).Contains(mouse_pos);
  const ImU32 color = IM_COL32(mColor.r, mColor.g, mColor.b, 255);
  const ImU32 borderColor = (hovered || moving) ? IM_COL32(255, 255, 255, 255) : IM_COL32(0, 0, 0, 255);
  ImDrawList *drawList = ImGui::GetWindowDrawList();

  for (int i = 0; i < my; ++i)
  {
    int j = lerp(0, mx, i / (my - 1.0));

    drawList->AddLine(ImVec2(cur_pos.x - j, cur_pos.y - i) + view_offset, ImVec2(cur_pos.x + j, cur_pos.y - i) + view_offset, color);
  }

  drawList->AddLine(point1 + view_offset, point2 + view_offset, borderColor);
  drawList->AddLine(point2 + view_offset, point3 + view_offset, borderColor);
  drawList->AddLine(point3 + view_offset, point1 + view_offset, borderColor);

  if (hovered && !moving && !tooltipText.empty())
    ImGui::SetTooltip(tooltipText);

  return hovered;
}

// -------------- Gradient ---------------------------

GradientControlStandalone::GradientControlStandalone(WindowControlEventHandler *event_handler, int w) :

  mEventHandler(event_handler), mKeys(midmem), mCycled(false), mMinPtCount(2), mMaxPtCount(100), mSelected(false)
{
  calculateSizes(w, _pxS(GRADIENT_HEIGHT));
  reset();
}


GradientControlStandalone::~GradientControlStandalone()
{
  cancelColorPickerShowRequest();
  clear_all_ptr_items(mKeys);
}


void GradientControlStandalone::draw()
{
  for (int i = 0; i < mKeys.size(); i++)
  {
    if (mKeys[i]->useless())
    {
      if (mEventHandler)
        mEventHandler->onWcChanging(windowBaseForEventHandler);

      delete mKeys[i];
      erase_items(mKeys, i, 1);
      cancelColorPickerShowRequest();
      mouseClickKeyIndex = -1;

      if (mEventHandler)
        mEventHandler->onWcChange(windowBaseForEventHandler);
    }
  }

  ImDrawList *drawList = ImGui::GetWindowDrawList();

  RectInt curRect;
  curRect.left = 1;
  curRect.top = 1;
  curRect.right = 2;
  curRect.bottom = gradientDisplaySize.y - 1;

  for (int i = 0; i < (int)mKeys.size() - 1; i++)
  {
    E3DCOLOR startColor = mKeys[i]->getColorValue();
    E3DCOLOR endColor = mKeys[i + 1]->getColorValue();
    E3DCOLOR color = startColor;

    float step[3];
    int w = mKeys[i + 1]->getPosInGradientDisplaySpace() - mKeys[i]->getPosInGradientDisplaySpace();

    step[0] = (float)(endColor.r - startColor.r) / w;
    step[1] = (float)(endColor.g - startColor.g) / w;
    step[2] = (float)(endColor.b - startColor.b) / w;

    int n = 0;
    while (curRect.left < mKeys[i + 1]->getPosInGradientDisplaySpace())
    {
      int r = startColor.r + step[0] * n;
      int g = startColor.g + step[1] * n;
      int b = startColor.b + step[2] * n;

      const ImVec2 leftTop(gradientDisplayStart.x + curRect.left, gradientDisplayStart.y + curRect.top);
      const ImVec2 rightBottom(gradientDisplayStart.x + curRect.right, gradientDisplayStart.y + curRect.bottom);
      drawList->AddRectFilled(leftTop + viewOffset, rightBottom + viewOffset, IM_COL32(r, g, b, 255));

      curRect.left += 1;
      curRect.right += 1;
      n++;
    }
  }

  // draw current X value
  if (mCurValue)
  {
    const ImVec2 leftTop(gradientDisplayStart.x + (gradientDisplaySize.x * mCurValue), gradientDisplayStart.y);
    const ImVec2 rightBottom(leftTop.x, leftTop.y + gradientDisplaySize.y - 1);
    const E3DCOLOR curColor = getColor(mCurValue);
    const int mid_value = (curColor.r + curColor.g + curColor.b) / 3;
    const ImU32 cur_pen = (mid_value < 128) ? IM_COL32(255, 255, 255, 255) : IM_COL32(0, 0, 0, 255); // NOTE: ImGui porting: it was
                                                                                                     // using PS_DOT.
    drawList->AddLine(leftTop + viewOffset, rightBottom + viewOffset, cur_pen);
  }

  // draw rect
  const ImVec2 leftTop(gradientDisplayStart.x, gradientDisplayStart.y);
  const ImVec2 rightBottom(leftTop.x + gradientDisplaySize.x, leftTop.y + gradientDisplaySize.y);
  const ImU32 rect_pen = mSelected ? IM_COL32(255, 0, 0, 255) : IM_COL32(0, 0, 0, 255);
  drawList->AddRect(leftTop + viewOffset, rightBottom + viewOffset, rect_pen);
}

void GradientControlStandalone::onLButtonDClick(float x_pos_in_canvas_space)
{
  if (!mEventHandler)
    return;

  mEventHandler->onWcChanging(windowBaseForEventHandler);

  const float position = (x_pos_in_canvas_space - gradientDisplayStart.x) / ((float)gradientDisplaySize.x);
  addKey(position, nullptr, /*check_distance = */ true);

  mEventHandler->onWcChange(windowBaseForEventHandler);
}


bool GradientControlStandalone::canRemove() { return (mKeys.size() > mMinPtCount); }

void GradientControlStandalone::addKey(float position, E3DCOLOR *color, bool check_distance)
{
  if ((position < 0) || (position > 1))
    return;

  if (mKeys.size() >= mMaxPtCount)
    return;

  E3DCOLOR curColor;
  if (color == NULL)
    curColor = getColor(position);
  else
    curColor = *color;

  // NOTE: ImGui porting: the behavior has been changed here because at the first setValue call we do not have width
  // yet. Also what if there are a lot of keys and the property panel is narrow?
  if (check_distance)
  {
    int posTest = position * gradientDisplaySize.x;
    for (int i = 0; i < mKeys.size(); i++)
    {
      if (abs(posTest - mKeys[i]->getPosInGradientDisplaySpace()) < _pxS(TRACK_GRADIENT_BUTTON_WIDTH))
        return;
    }
  }

  for (int i = 0; i < (int)mKeys.size() - 1; i++)
  {
    if ((position > mKeys[i]->getValue()) && (position < mKeys[i + 1]->getValue()))
    {
      TrackGradientButton *key = new TrackGradientButton(mEventHandler, curColor, true, this, position);
      insert_item_at(mKeys, i + 1, key);
      cancelColorPickerShowRequest();
      mouseClickKeyIndex = -1;
      return;
    }
  }
}

E3DCOLOR GradientControlStandalone::getColor(float position)
{
  int x = position * gradientDisplaySize.x;
  int curX = 0;

  for (int i = 0; i < (int)mKeys.size() - 1; i++)
  {
    E3DCOLOR startColor = mKeys[i]->getColorValue();
    E3DCOLOR endColor = mKeys[i + 1]->getColorValue();
    E3DCOLOR color = startColor;

    float step[3];
    int w = mKeys[i + 1]->getPosInGradientDisplaySpace() - mKeys[i]->getPosInGradientDisplaySpace();

    step[0] = (float)(endColor.r - startColor.r) / w;
    step[1] = (float)(endColor.g - startColor.g) / w;
    step[2] = (float)(endColor.b - startColor.b) / w;

    int n = 0;
    while (curX < mKeys[i + 1]->getPosInGradientDisplaySpace())
    {
      if (curX == x)
      {
        int r = startColor.r + step[0] * n;
        int g = startColor.g + step[1] * n;
        int b = startColor.b + step[2] * n;
        return E3DCOLOR(r, g, b);
      }
      curX++;
      n++;
    }
  }
  return E3DCOLOR(0, 0, 0);
}


void GradientControlStandalone::getLimits(TrackGradientButton *key, int &left_limit, int &right_limit)
{
  left_limit = right_limit = key->getPosInGradientDisplaySpace();

  for (int i = 0; i < mKeys.size(); i++)
  {
    if (key == mKeys[i])
    {
      if (i > 0)
        left_limit = mKeys[i - 1]->getPosInGradientDisplaySpace();

      if (i + 1 < mKeys.size())
        right_limit = mKeys[i + 1]->getPosInGradientDisplaySpace();

      return;
    }
  }
}

void GradientControlStandalone::setValue(PGradient source)
{
  if (!source)
    return;

  Gradient &value = *source;

  if (value.size() < 2)
    return;

  for (int i = 0; i < value.size(); i++)
  {
    if ((value[i].position < 0) || (value[i].position > 1.0f))
      return;
  }

  clear_all_ptr_items(mKeys);
  cancelColorPickerShowRequest();
  mouseClickKeyIndex = -1;

  TrackGradientButton *left = new TrackGradientButton(mEventHandler, value[0].color, false, this, 0);
  TrackGradientButton *right = new TrackGradientButton(mEventHandler, (mCycled) ? value[0].color : value.back().color, false, this, 1);

  mKeys.push_back(left);
  mKeys.push_back(right);

  for (int i = 1; i < (int)value.size() - 1; i++)
    addKey(value[i].position, &value[i].color, /*check_distance = */ false);
}

void GradientControlStandalone::setCurValue(float value) { mCurValue = value; }


void GradientControlStandalone::setMinMax(int min, int max)
{
  mMinPtCount = min;
  mMaxPtCount = max;

  if (mMinPtCount < 2)
    mMinPtCount = 2;

  if (mMaxPtCount < mMinPtCount)
    mMaxPtCount = mMinPtCount;
}


void GradientControlStandalone::setSelected(bool value) { mSelected = value; }


void GradientControlStandalone::getValue(PGradient destGradient) const
{
  if (!destGradient)
    return;

  clear_and_shrink(*destGradient);

  GradientKey current;

  for (int i = 0; i < mKeys.size(); i++)
  {
    current.position = mKeys[i]->getValue();
    current.color = mKeys[i]->getColorValue();
    destGradient->push_back(current);
  }
}

void GradientControlStandalone::reset()
{
  clear_all_ptr_items(mKeys);
  cancelColorPickerShowRequest();
  mouseClickKeyIndex = -1;

  TrackGradientButton *left = new TrackGradientButton(mEventHandler, E3DCOLOR(255, 255, 255), false, this, 0);
  TrackGradientButton *right = new TrackGradientButton(mEventHandler, E3DCOLOR(0, 0, 0), false, this, 1);

  mKeys.push_back(left);
  mKeys.push_back(right);
}

void GradientControlStandalone::updateCycled(TrackGradientButton *button)
{
  if (!mCycled || !(button == mKeys[0] || button == mKeys.back()))
    return;

  int i = (button == mKeys[0]) ? 0 : mKeys.size() - 1;
  int j = (i == 0) ? mKeys.size() - 1 : 0;

  mKeys[j]->setColorValue(mKeys[i]->getColorValue());
}

void GradientControlStandalone::calculateSizes(int total_width, int total_height)
{
  if (totalControlWidth != total_width || totalControlHeight != total_height)
  {
    totalControlWidth = total_width;
    totalControlHeight = total_height;
    gradientDisplayStart.x = _pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2;
    gradientDisplayStart.y = _pxS(TRACK_GRADIENT_BUTTON_HEIGHT);
    gradientDisplaySize.x = total_width - _pxS(TRACK_GRADIENT_BUTTON_WIDTH); // half before it, half after it
    gradientDisplaySize.y = total_height - gradientDisplayStart.y;
  }
}

void GradientControlStandalone::cancelColorPickerShowRequest()
{
  remove_delayed_callback(*this);
  showColorPickerForKeyIndex = -1;
}

void GradientControlStandalone::onImguiDelayedCallback(void *user_data)
{
  if (showColorPickerForKeyIndex < 0 || showColorPickerForKeyIndex >= mKeys.size())
    return;

  mKeys[showColorPickerForKeyIndex]->showColorPicker();
  showColorPickerForKeyIndex = -1;
}

void GradientControlStandalone::updateImgui(int width, int height)
{
  ImGui::PushID(this);

  calculateSizes(width, height);
  viewOffset = ImGui::GetCursorScreenPos();

  // This will catch the interactions.
  const ImGuiID canvasId = ImGui::GetCurrentWindow()->GetID("canvas");
  ImGui::PushID(canvasId);
  ImGui::InvisibleButton("", ImVec2(totalControlWidth, totalControlHeight));
  ImGui::PopID();

  const bool isHovered = ImGui::IsItemHovered();
  const ImVec2 mousePosInCanvas = ImGui::GetIO().MousePos - viewOffset;

  if (isHovered)
  {
    ImGui::SetKeyOwner(ImGuiKey_MouseLeft, canvasId);
    ImGui::SetKeyOwner(ImGuiKey_MouseRight, canvasId);
  }

  draw();

  int hoveredKeyIndex = mouseClickKeyIndex;
  for (int i = 0; i < mKeys.size(); ++i)
    if (mKeys[i]->updateImgui(viewOffset, mousePosInCanvas, isHovered && mouseClickKeyIndex < 0) && hoveredKeyIndex < 0)
      hoveredKeyIndex = i;

  if (hoveredKeyIndex >= 0 && mouseClickKeyIndex < 0 &&
      (ImGui::IsMouseClicked(ImGuiMouseButton_Left, ImGuiInputFlags_None, canvasId) ||
        ImGui::IsMouseClicked(ImGuiMouseButton_Right, ImGuiInputFlags_None, canvasId)))
  {
    mouseClickKeyIndex = hoveredKeyIndex;
    lastMouseDragPosInCanvas = Point2(-1.0f, -1.0f);
  }

  if (mouseClickKeyIndex >= 0)
  {
    G_ASSERT(mouseClickKeyIndex < mKeys.size());

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left, canvasId))
    {
      if (mKeys[mouseClickKeyIndex]->isMoving())
      {
        mKeys[mouseClickKeyIndex]->cancelMoving();
      }
      else
      {
        cancelColorPickerShowRequest();
        showColorPickerForKeyIndex = mouseClickKeyIndex;
        request_delayed_callback(*this);
      }

      mouseClickKeyIndex = -1;
    }
    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right, canvasId))
    {
      mKeys[mouseClickKeyIndex]->cancelMoving();
      mKeys[mouseClickKeyIndex]->markForDeletion();
      mouseClickKeyIndex = -1;
    }
    else if (mousePosInCanvas != lastMouseDragPosInCanvas && ImGui::IsMouseDown(ImGuiMouseButton_Left, canvasId))
    {
      lastMouseDragPosInCanvas = mousePosInCanvas;
      mKeys[mouseClickKeyIndex]->onDrag(mousePosInCanvas.x, mousePosInCanvas.y);
    }
  }
  else if (isHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left, canvasId))
  {
    onLButtonDClick(mousePosInCanvas.x);
  }

  ImGui::PopID();
}

} // namespace PropPanel