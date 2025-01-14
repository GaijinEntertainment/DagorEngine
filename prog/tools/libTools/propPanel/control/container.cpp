// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/control/container.h>
#include <propPanel/c_indirect.h>
#include <propPanel/focusHelper.h>
#include "button.h"
#include "checkBox.h"
#include "colorBox.h"
#include "comboBox.h"
#include "customControlHolder.h"
#include "curveEdit.h"
#include "editBox.h"
#include "embeddedContainer.h"
#include "extensible.h"
#include "extGroup.h"
#include "fileButton.h"
#include "fileEditBox.h"
#include "gradientBox.h"
#include "group.h"
#include "groupBox.h"
#include "indent.h"
#include "listBox.h"
#include "multiSelectListBox.h"
#include "point2.h"
#include "point3.h"
#include "point4.h"
#include "radioButton.h"
#include "radioGroup.h"
#include "separator.h"
#include "simpleColor.h"
#include "spinEditFloat.h"
#include "spinEditInt.h"
#include "static.h"
#include "tabPanel.h"
#include "targetButton.h"
#include "toolbar.h"
#include "trackBarFloat.h"
#include "trackBarInt.h"
#include "tree.h"
#include "../messageQueueInternal.h"
#include <imgui/imgui_internal.h>

namespace PropPanel
{

ContainerPropertyControl::ContainerPropertyControl(int id, ControlEventHandler *event_handler, ContainerPropertyControl *parent, int x,
  int y, hdpi::Px w, hdpi::Px h) :
  PropertyControlBase(id, event_handler, parent, x, y, w, h)
{
  setVerticalSpaceBetweenControls(hdpi::_pxScaled(Constants::DEFAULT_CONTROLS_INTERVAL));
}

ContainerPropertyControl::~ContainerPropertyControl() { message_queue.onContainerDelete(*this); }

ContainerPropertyControl *ContainerPropertyControl::createContainer(int id, bool new_line, hdpi::Px interval)
{
  EmbeddedContainerPropertyControl *newControl = new EmbeddedContainerPropertyControl(mEventHandler, this, id, getNextControlX(),
    getNextControlY(new_line), hdpi::Px(0), hdpi::Px(0), interval);
  addControl(newControl, new_line);
  return newControl;
}

ContainerPropertyControl *ContainerPropertyControl::createExtensible(int id, bool new_line)
{
  ExtensiblePropertyControl *newControl = new ExtensiblePropertyControl(mEventHandler, this, id, getNextControlX(),
    getNextControlY(new_line), getClientWidth(), hdpi::Px(0));

  addControl(newControl, new_line);
  return newControl;
}

ContainerPropertyControl *ContainerPropertyControl::createExtGroup(int id, const char caption[])
{
  ExtGroupPropertyControl *newControl =
    new ExtGroupPropertyControl(mEventHandler, this, id, getNextControlX(), getNextControlY(), getClientWidth(), hdpi::Px(0), caption);

  addControl(newControl);
  return newControl;
}

ContainerPropertyControl *ContainerPropertyControl::createGroup(int id, const char caption[])
{
  GroupPropertyControl *newControl = new GroupPropertyControl(mEventHandler, this, id, 0, 0, hdpi::Px(0), hdpi::Px(0), caption);
  addControl(newControl);
  return newControl;
}

ContainerPropertyControl *ContainerPropertyControl::createGroupHorzFlow(int id, const char caption[])
{
  // TODO: ImGui porting: AV: horizontal flow is used in AssetViewer/Entity/entity.cpp. We simply use vertical flow here.
  return createGroup(id, caption);
}

ContainerPropertyControl *ContainerPropertyControl::createGroupBox(int id, const char caption[])
{
  GroupBoxPropertyControl *newControl =
    new GroupBoxPropertyControl(mEventHandler, this, id, getNextControlX(), getNextControlY(), getClientWidth(), hdpi::Px(0), caption);

  addControl(newControl);
  return newControl;
}

ContainerPropertyControl *ContainerPropertyControl::createRadioGroup(int id, const char caption[], bool new_line)
{
  RadioGroupPropertyControl *newControl =
    new RadioGroupPropertyControl(mEventHandler, this, id, 0, 0, hdpi::Px(0), hdpi::Px(0), caption);
  addControl(newControl, new_line);
  return newControl;
}

ContainerPropertyControl *ContainerPropertyControl::createTabPanel(int id, const char caption[])
{
  G_UNUSED(caption);

  TabPanelPropertyControl *newControl =
    new TabPanelPropertyControl(mEventHandler, this, id, getNextControlX(), getNextControlY(), getClientWidth(), hdpi::Px(0));

  addControl(newControl);
  return newControl;
}

ContainerPropertyControl *ContainerPropertyControl::createTabPage(int id, const char caption[])
{
  G_ASSERT(false && "TabPage can be created only in TabPanel!");
  return nullptr;
}

ContainerPropertyControl *ContainerPropertyControl::createToolbarPanel(int id, const char caption[], bool new_line)
{
  ToolbarPropertyControl *newControl = new ToolbarPropertyControl(mEventHandler, this, id, caption);

  addControl(newControl, new_line);
  return newControl;
}

ContainerPropertyControl *ContainerPropertyControl::createTree(int id, const char caption[], hdpi::Px height, bool new_line)
{
  TreePropertyControl *newControl = new TreePropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), height, caption, /*has_checkboxes = */ false, /*multi_select = */ false);

  addControl(newControl, new_line);
  return newControl;
}

ContainerPropertyControl *ContainerPropertyControl::createTreeCheckbox(int id, const char caption[], hdpi::Px height, bool new_line,
  bool icons_show)
{
  TreePropertyControl *newControl = new TreePropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), height, caption, /*has_checkboxes = */ true, /*multi_select = */ false);

  addControl(newControl, new_line);
  return newControl;
}

ContainerPropertyControl *ContainerPropertyControl::createMultiSelectTree(int id, const char caption[], hdpi::Px height, bool new_line)
{
  TreePropertyControl *newControl = new TreePropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), height, caption, /*has_checkboxes = */ false, /*multi_select = */ true);

  addControl(newControl, new_line);
  return newControl;
}

ContainerPropertyControl *ContainerPropertyControl::createMultiSelectTreeCheckbox(int id, const char caption[], hdpi::Px height,
  bool new_line)
{
  TreePropertyControl *newControl = new TreePropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), height, caption, /*has_checkboxes = */ true, /*multi_select = */ true);

  addControl(newControl, new_line);
  return newControl;
}

void ContainerPropertyControl::createStatic(int id, const char caption[], bool new_line)
{
  StaticPropertyControl *newControl = new StaticPropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), caption, hdpi::Px(0));

  addControl(newControl, new_line);
}

void ContainerPropertyControl::createEditBox(int id, const char caption[], const char text[], bool enabled, bool new_line,
  bool multiline, hdpi::Px multi_line_height)
{
  EditBoxPropertyControl *newControl =
    new EditBoxPropertyControl(id, mEventHandler, this, 0, 0, hdpi::Px(0), hdpi::Px(0), caption, multiline);

  newControl->setTextValue(text);
  newControl->setEnabled(enabled);

  if (multiline)
    newControl->setHeight(multi_line_height);

  addControl(newControl, new_line);
}

void ContainerPropertyControl::createFileEditBox(int id, const char caption[], const char file[], bool enabled, bool new_line)
{
  FileEditBoxPropertyControl *newControl = new FileEditBoxPropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), caption);

  newControl->setTextValue(file);
  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createFileButton(int id, const char caption[], const char file[], bool enabled, bool new_line)
{
  FileButtonPropertyControl *newControl = new FileButtonPropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), caption);

  newControl->setTextValue(file);
  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createTargetButton(int id, const char caption[], const char text[], bool enabled, bool new_line)
{
  TargetButtonPropertyControl *newControl = new TargetButtonPropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), caption);

  newControl->setTextValue(text);
  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createEditInt(int id, const char caption[], int value, bool enabled, bool new_line)
{
  SpinEditIntPropertyControl *newControl = new SpinEditIntPropertyControl(mEventHandler, this, id, caption);

  newControl->setIntValue(value);
  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createEditFloat(int id, const char caption[], float value, int prec, bool enabled, bool new_line)
{
  createEditFloatWidthEx(id, caption, value, prec, enabled, new_line);
}

void ContainerPropertyControl::createEditFloatWidthEx(int id, const char caption[], float value, int prec, bool enabled, bool new_line,
  bool width_includes_label)
{
  SpinEditFloatPropertyControl *newControl =
    new SpinEditFloatPropertyControl(mEventHandler, this, id, caption, prec, width_includes_label);
  newControl->setFloatValue(value);
  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createTrackInt(int id, const char caption[], int value, int min, int max, int step, bool enabled,
  bool new_line)
{
  TrackBarIntPropertyControl *newControl = new TrackBarIntPropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), caption, min, max, step);

  newControl->setIntValue(value);
  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createTrackFloat(int id, const char caption[], float value, float min, float max, float step,
  bool enabled, bool new_line)
{
  TrackBarFloatPropertyControl *newControl = new TrackBarFloatPropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), caption, min, max, step, TRACKBAR_DEFAULT_POWER);

  newControl->setFloatValue(value);
  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createTrackFloatLogarithmic(int id, const char caption[], float value, float min, float max, float step,
  float power, bool enabled, bool new_line)
{
  TrackBarFloatPropertyControl *newControl = new TrackBarFloatPropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), caption, min, max, step, power);

  newControl->setFloatValue(value);
  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createCheckBox(int id, const char caption[], bool value, bool enabled, bool new_line)
{
  CheckBoxPropertyControl *newControl = new CheckBoxPropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), caption);

  newControl->setBoolValue(value);
  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createButton(int id, const char caption[], bool enabled, bool new_line)
{
  ButtonPropertyControl *newControl = new ButtonPropertyControl(id, mEventHandler, this, 0, 0, hdpi::Px(0), hdpi::Px(0), caption);
  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createButtonLText(int id, const char caption[], bool enabled, bool new_line)
{
  ButtonPropertyControl *newControl =
    new ButtonPropertyControl(id, mEventHandler, this, 0, 0, hdpi::Px(0), hdpi::Px(0), caption, true);
  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createIndent(int id, bool new_line)
{
  IndentPropertyControl *newControl =
    new IndentPropertyControl(mEventHandler, this, id, getNextControlX(new_line), getNextControlY(new_line), getClientWidth());

  addControl(newControl, new_line);
}

void ContainerPropertyControl::createSeparator(int id, bool new_line)
{
  SeparatorPropertyControl *newControl =
    new SeparatorPropertyControl(mEventHandler, this, id, getNextControlX(new_line), getNextControlY(new_line), getClientWidth());

  addControl(newControl, new_line);
}

void ContainerPropertyControl::createCombo(int id, const char caption[], const Tab<String> &vals, int index, bool enabled,
  bool new_line)
{
  ComboBoxPropertyControl *newControl = new ComboBoxPropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), caption, vals, index, /*sorted = */ false);

  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createCombo(int id, const char caption[], const Tab<String> &vals, const char *selection, bool enabled,
  bool new_line)
{
  const int index = ImguiHelper::getStringIndexInTab(vals, selection);
  createCombo(id, caption, vals, index, enabled, new_line);
}

void ContainerPropertyControl::createSortedCombo(int id, const char caption[], const Tab<String> &vals, int index, bool enabled,
  bool new_line)
{
  ComboBoxPropertyControl *newControl = new ComboBoxPropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), caption, vals, index, /*sorted = */ true);

  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createList(int id, const char caption[], const Tab<String> &vals, int index, bool enabled,
  bool new_line)
{
  ListBoxPropertyControl *newControl = new ListBoxPropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), caption, vals, index);

  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createList(int id, const char caption[], const Tab<String> &vals, const char *selection, bool enabled,
  bool new_line)
{
  const int index = ImguiHelper::getStringIndexInTab(vals, selection);
  createList(id, caption, vals, index, enabled, new_line);
}

void ContainerPropertyControl::createMultiSelectList(int id, const Tab<String> &vals, hdpi::Px height, bool enabled, bool new_line)
{
  MultiSelectListBoxPropertyControl *newControl = new MultiSelectListBoxPropertyControl(mEventHandler, this, id,
    getNextControlX(new_line), getNextControlY(new_line), getClientWidth(), height, vals);

  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createRadio(int id, const char caption[], bool enabled, bool new_line)
{
  RadioButtonPropertyControl *newControl =
    new RadioButtonPropertyControl(mEventHandler, this, getID(), 0, 0, hdpi::Px(0), caption, id);
  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createColorBox(int id, const char caption[], E3DCOLOR value, bool enabled, bool new_line)
{
  ColorBoxPropertyControl *newControl = new ColorBoxPropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), caption);

  newControl->setColorValue(value);
  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createSimpleColor(int id, const char caption[], E3DCOLOR value, bool enabled, bool new_line)
{
  SimpleColorPropertyControl *newControl = new SimpleColorPropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), caption);

  newControl->setColorValue(value);
  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createPoint2(int id, const char caption[], Point2 value, int prec, bool enabled, bool new_line)
{
  Point2PropertyControl *newControl = new Point2PropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), caption, prec);

  newControl->setPoint2Value(value);
  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createPoint3(int id, const char caption[], Point3 value, int prec, bool enabled, bool new_line)
{
  Point3PropertyControl *newControl = new Point3PropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), caption, prec);

  newControl->setPoint3Value(value);
  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createPoint4(int id, const char caption[], Point4 value, int prec, bool enabled, bool new_line)
{
  Point4PropertyControl *newControl = new Point4PropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), caption, prec);

  newControl->setPoint4Value(value);
  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createMatrix(int id, const char caption[], const TMatrix &value, int prec, bool enabled, bool new_line)
{
  // TODO: ImGui porting: missing control: used in scriptPanelWrapper.
  logdbg("ImGui porting: missing control: %s", __FUNCTION__);
  G_ASSERT(false);
}

void ContainerPropertyControl::createGradientBox(int id, const char caption[], bool enabled, bool new_line)
{
  GradientBoxPropertyControl *newControl = new GradientBoxPropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), caption);

  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createTextGradient(int id, const char caption[], bool enabled, bool new_line)
{
  // TODO: ImGui porting: missing control: used in scriptPanelWrapper.
  logdbg("ImGui porting: missing control: %s", __FUNCTION__);
  G_ASSERT(false);
}

void ContainerPropertyControl::createGradientPlot(int id, const char caption[], hdpi::Px height, bool enabled, bool new_line)
{
  // Only used by ColorDialog, not really usable elsewhere.
  G_ASSERT_LOG(false, "GradientPlot is an internal control in propPanel.");
}

void ContainerPropertyControl::createCurveEdit(int id, const char caption[], hdpi::Px height, bool enabled, bool new_line)
{
  CurveEditPropertyControl *newControl = new CurveEditPropertyControl(mEventHandler, this, id, getNextControlX(new_line),
    getNextControlY(new_line), getClientWidth(), height == hdpi::Px::ZERO ? hdpi::_pxScaled(DEFAULT_CURVE_HEIGHT) : height, caption);

  newControl->setEnabled(enabled);
  addControl(newControl, new_line);
}

void ContainerPropertyControl::createTwoColorIndicator(int id, const char caption[], hdpi::Px height, bool enabled, bool new_line)
{
  // Only used by ColorDialog, not really usable elsewhere.
  G_ASSERT_LOG(false, "TwoColorIndicator is an internal control in propPanel.");
}

void ContainerPropertyControl::createPaletteCell(int id, const char caption[], bool enabled, bool new_line)
{
  // Only used by ColorDialog, not really usable elsewhere.
  G_ASSERT_LOG(false, "PaletteCell is an internal control in propPanel.");
}

PropertyControlBase *ContainerPropertyControl::createPlaceholder(int id, hdpi::Px height, bool new_line)
{
  // NOTE: ImGui porting: unused.
  G_ASSERT_LOG(false, "createPlaceholder was unused, and has not been implemented.");
  return nullptr;
}

void ContainerPropertyControl::createCustomControlHolder(int id, ICustomControl *control, bool new_line)
{
  CustomControlHolderPropertyControl *newControl = new CustomControlHolderPropertyControl(mEventHandler, this, id,
    getNextControlX(new_line), getNextControlY(new_line), getClientWidth(), control);
  addControl(newControl, new_line);
}

TLeafHandle ContainerPropertyControl::createTreeLeaf(TLeafHandle parent, const char caption[], const char image[])
{
  G_ASSERT(false && "Tree leaf can be created only in Tree control!");
  return 0;
}

void ContainerPropertyControl::createIndirect(DataBlock &dataBlock, ISetControlParams &controlParams)
{
  // NOTE: ImGui porting: unused.
  G_ASSERT_LOG(false, "createIndirect was unused, and has not been implemented.");
}

PropPanelScheme *ContainerPropertyControl::createSceme()
{
  PropPanelScheme *scheme = new PropPanelScheme();
  return scheme;
}

void ContainerPropertyControl::setEnabledById(int id, bool enabled)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    ptr->setEnabled(enabled);
}

void ContainerPropertyControl::resetById(int id)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    ptr->reset();
}

void ContainerPropertyControl::setWidthById(int id, hdpi::Px w)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    ptr->setWidth(w);
}

void ContainerPropertyControl::setFocusById(int id)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    ptr->setFocus();
}

void ContainerPropertyControl::setUserData(int id, const void *value)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getUserDataValue() != value))
      ptr->reset();
    else
      ptr->setUserDataValue(value);
  }
}

void ContainerPropertyControl::setText(int id, const char value[])
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    char buf[256];
    ptr->getTextValue(buf, 256);
    if ((mUpdate) && (strcmp(buf, value)))
      ptr->reset();
    else
      ptr->setTextValue(value);
  }
}

void ContainerPropertyControl::setFloat(int id, float value)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getFloatValue() != value))
      ptr->reset();
    else
      ptr->setFloatValue(value);
  }
}

void ContainerPropertyControl::setInt(int id, int value)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getIntValue() != value))
      ptr->reset();
    else
      ptr->setIntValue(value);
  }
}

void ContainerPropertyControl::setBool(int id, bool value)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getBoolValue() != value))
      ptr->reset();
    else
      ptr->setBoolValue(value);
  }
}

void ContainerPropertyControl::setColor(int id, E3DCOLOR value)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getColorValue() != value))
      ptr->reset();
    else
      ptr->setColorValue(value);
  }
}

void ContainerPropertyControl::setPoint2(int id, Point2 value)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getPoint2Value() != value))
      ptr->reset();
    else
      ptr->setPoint2Value(value);
  }
}

void ContainerPropertyControl::setPoint3(int id, Point3 value)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getPoint3Value() != value))
      ptr->reset();
    else
      ptr->setPoint3Value(value);
  }
}

void ContainerPropertyControl::setPoint4(int id, Point4 value)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getPoint4Value() != value))
      ptr->reset();
    else
      ptr->setPoint4Value(value);
  }
}

void ContainerPropertyControl::setMatrix(int id, const TMatrix &value)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getMatrixValue() != value))
      ptr->reset();
    else
      ptr->setMatrixValue(value);
  }
}

void ContainerPropertyControl::setGradient(int id, PGradient value)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    ptr->setGradientValue(value);
  }
}

void ContainerPropertyControl::setTextGradient(int id, const TextGradient &source)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    ptr->setTextGradientValue(source);
  }
}

void ContainerPropertyControl::setControlPoints(int id, Tab<Point2> &points)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    ptr->setControlPointsValue(points);
  }
}

void ContainerPropertyControl::setTooltipId(int id, const char text[])
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    ptr->setTooltip(text);
  }
}

void ContainerPropertyControl::setMinMaxStep(int id, float min, float max, float step)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    ptr->setMinMaxStepValue(min, max, step);
  }
}

void ContainerPropertyControl::setPrec(int id, int prec)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    ptr->setPrecValue(prec);
  }
}

void ContainerPropertyControl::setStrings(int id, const Tab<String> &vals)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    ptr->setStringsValue(vals);
  }
}

void ContainerPropertyControl::setSelection(int id, const Tab<int> &sels)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    ptr->setSelectionValue(sels);
  }
}

void ContainerPropertyControl::setCaption(int id, const char value[])
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    ptr->setCaptionValue(value);
  }
}

void ContainerPropertyControl::setButtonPictures(int id, const char *fname)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    ptr->setButtonPictureValues(fname);
  }
}

int ContainerPropertyControl::addString(int id, const char *value)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    return ptr->addStringValue(value);

  return 0;
}

void ContainerPropertyControl::removeString(int id, int idx)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    ptr->removeStringValue(idx);
}

void *ContainerPropertyControl::getUserData(int id) const
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    return ptr->getUserDataValue();

  return NULL;
}


SimpleString ContainerPropertyControl::getText(int id) const
{
  SimpleString buffer;
  buffer.allocBuffer(STRING_SIZE);

  PropertyControlBase *ptr = getById(id);
  if (ptr)
    ptr->getTextValue(buffer.str(), STRING_SIZE);

  return buffer;
}

float ContainerPropertyControl::getFloat(int id) const
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    return ptr->getFloatValue();

  return 0;
}

int ContainerPropertyControl::getInt(int id) const
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    return ptr->getIntValue();

  return 0;
}

bool ContainerPropertyControl::getBool(int id) const
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    return ptr->getBoolValue();

  return false;
}

E3DCOLOR ContainerPropertyControl::getColor(int id) const
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    return ptr->getColorValue();

  return 0;
}

Point2 ContainerPropertyControl::getPoint2(int id) const
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    return ptr->getPoint2Value();

  return Point2(0, 0);
}


Point3 ContainerPropertyControl::getPoint3(int id) const
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    return ptr->getPoint3Value();

  return Point3(0, 0, 0);
}

Point4 ContainerPropertyControl::getPoint4(int id) const
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    return ptr->getPoint4Value();

  return Point4(0, 0, 0, 0);
}

TMatrix ContainerPropertyControl::getMatrix(int id) const
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    return ptr->getMatrixValue();

  return TMatrix::IDENT;
}


void ContainerPropertyControl::getGradient(int id, PGradient destGradient) const
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    ptr->getGradientValue(destGradient);
  else
  {
    if (destGradient)
    {
      clear_and_shrink(*destGradient);
    }
  }
}

void ContainerPropertyControl::getTextGradient(int id, TextGradient &destGradient) const
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    ptr->getTextGradientValue(destGradient);
  else
    clear_and_shrink(destGradient);
}

void ContainerPropertyControl::getCurveCoefs(int id, Tab<Point2> &points) const
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    ptr->getCurveCoefsValue(points);
  else
    clear_and_shrink(points);
}

bool ContainerPropertyControl::getCurveCubicCoefs(int id, Tab<Point2> &xy_4c_per_seg) const
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    return ptr->getCurveCubicCoefsValue(xy_4c_per_seg);
  clear_and_shrink(xy_4c_per_seg);
  return false;
}

int ContainerPropertyControl::getStrings(int id, Tab<String> &vals)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    return ptr->getStringsValue(vals);
  }

  return 0;
}

int ContainerPropertyControl::getSelection(int id, Tab<int> &sels)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
  {
    ptr->getSelectionValue(sels);
  }

  return 0;
}

unsigned ContainerPropertyControl::getTypeMaskForSet() const
{
  // NOTE: ImGui porting: unused.
  return 0;
}

unsigned ContainerPropertyControl::getTypeMaskForGet() const
{
  // NOTE: ImGui porting: unused.
  return 0;
}

SimpleString ContainerPropertyControl::getCaption() const
{
  SimpleString buffer;
  buffer.allocBuffer(256);
  getCaptionValue(buffer.str(), 256);
  return buffer;
}


void ContainerPropertyControl::clear()
{
  for (int i = 0; i < mControlArray.size(); ++i)
  {
    if (mControlArray[i])
    {
      delete (mControlArray[i]); // destructor for container calls clear()
    }
  }

  clear_and_shrink(mControlArray);
  clear_and_shrink(mControlsNewLine);
}


unsigned ContainerPropertyControl::getChildCount() { return mControlArray.size(); }

hdpi::Px ContainerPropertyControl::getClientWidth()
{
  // NOTE: ImGui porting: unused.
  return hdpi::Px(0);
}

hdpi::Px ContainerPropertyControl::getClientHeight()
{
  // NOTE: ImGui porting: unused.
  return hdpi::Px(0);
}

WindowBase *ContainerPropertyControl::getWindow()
{
  // NOTE: ImGui porting: unused.
  return nullptr;
}

void *ContainerPropertyControl::getWindowHandle()
{
  // NOTE: ImGui porting: unused.
  return nullptr;
}

void *ContainerPropertyControl::getParentWindowHandle()
{
  // NOTE: ImGui porting: unused.
  return nullptr;
}

PropertyControlBase *ContainerPropertyControl::getById(int id, bool stopOnDifferentEventHandler) const
{
  for (int i = 0; i < mControlArray.size(); i++)
  {
    if (mControlArray[i]->getID() == id)
      return mControlArray[i];

    const ContainerPropertyControl *container = mControlArray[i]->getContainer();

    if (container && (!stopOnDifferentEventHandler || (container->getEventHandler() == getEventHandler())))
    {
      PropertyControlBase *result = container->getById(id);
      if (result)
        return result;
    }
  }

  return nullptr;
}

ContainerPropertyControl *ContainerPropertyControl::getContainerById(int id) const
{
  PropertyControlBase *control = getById(id);
  if (!control)
    return NULL;

  return control->getContainer();
}

PropertyControlBase *ContainerPropertyControl::getByIndex(int index) const
{
  if ((index >= 0) && (index < mControlArray.size()))
    return mControlArray[index];

  return NULL;
}

void ContainerPropertyControl::setPostEvent(int pcb_id) { message_queue.sendDelayedPostEvent(*this, pcb_id); }

void ContainerPropertyControl::setVisible(bool show)
{
  // NOTE: ImGui porting: unused.
}

int ContainerPropertyControl::saveState(DataBlock &datablk)
{
  PropertyControlBase::saveState(datablk);

  for (int i = 0; i < mControlArray.size(); i++)
  {
    mControlArray[i]->saveState(datablk);
  }

  return 0;
}

int ContainerPropertyControl::loadState(DataBlock &datablk)
{
  PropertyControlBase::loadState(datablk);

  for (int i = 0; i < mControlArray.size(); i++)
  {
    mControlArray[i]->loadState(datablk);
  }

  return 0;
}

bool ContainerPropertyControl::removeById(int id)
{
  for (int i = 0; i < mControlArray.size(); ++i)
  {
    if (mControlArray[i]->getID() == id)
    {
      if (i == 0 && mControlArray.size() > 1)
      {
        int y = mControlArray[0]->getY();
        mControlArray[1]->moveTo(mControlArray[1]->getX(), y);
      }

      delete mControlArray[i];
      safe_erase_items(mControlArray, i, 1);
      safe_erase_items(mControlsNewLine, i, 1);
      restoreFillAutoResize();
      return true;
    }

    ContainerPropertyControl *container = mControlArray[i]->getContainer();

    if (container && (container->getEventHandler() == getEventHandler()))
    {
      bool result = container->removeById(id);
      if (result)
        return result;
    }
  }

  return false;
}

bool ContainerPropertyControl::moveById(int id, int ba_id, bool after)
{
  if (id == ba_id || mControlArray.size() < 2)
    return false;

  int target_ind = -1;
  int ba_ind = -1;

  for (int i = 0; i < mControlArray.size(); ++i)
  {
    if (mControlArray[i]->getID() == id)
      target_ind = i;
    if (mControlArray[i]->getID() == ba_id)
      ba_ind = i;
  }

  if (after)
    ba_ind = (ba_ind != -1 && ba_ind + 1 < mControlArray.size()) ? ba_ind + 1 : -1;

  if (target_ind != -1 && ba_id == -1)
  {
    PropertyControlBase *ctrl = mControlArray[target_ind];
    bool new_line = mControlsNewLine[target_ind];

    mControlArray.push_back(ctrl);
    mControlsNewLine.push_back(new_line);
  }
  else if (target_ind != -1 && ba_ind != -1)
  {
    PropertyControlBase *ctrl = mControlArray[target_ind];
    bool new_line = mControlsNewLine[target_ind];

    // PropertyControlBase *ctrl2 = mControlArray[ba_ind];
    // int y = ctrl2->getY();
    // ctrl2->moveTo(ctrl2->getX(), ctrl->getY());
    // ctrl->moveTo(ctrl->getX(), y);

    int y = mControlArray[ba_ind]->getY();
    ctrl->moveTo(ctrl->getX(), y);

    insert_items(mControlArray, ba_ind, 1, &ctrl);
    insert_items(mControlsNewLine, ba_ind, 1, &new_line);

    if (target_ind > ba_ind)
      ++target_ind;
  }
  else
    return false;

  safe_erase_items(mControlArray, target_ind, 1);
  safe_erase_items(mControlsNewLine, target_ind, 1);

  restoreFillAutoResize();

  return true;
}

void ContainerPropertyControl::addControl(PropertyControlBase *pcontrol, bool new_line)
{
  mControlsNewLine.push_back(new_line);
  mControlArray.push_back(pcontrol);
  pcontrol->enableChangeHandlers();

  onControlAdd(pcontrol);
}

int ContainerPropertyControl::getNextControlX(bool new_line)
{
  // NOTE: ImGui porting: unused.
  return 0;
}

int ContainerPropertyControl::getNextControlY(bool new_line)
{
  // NOTE: ImGui porting: unused.
  return 0;
}

int ContainerPropertyControl::getControlsInSameLine(int start_index) const
{
  int count = 1;
  for (int i = start_index + 1; i < mControlsNewLine.size() && !mControlsNewLine[i]; ++i)
    ++count;

  return count;
}

void ContainerPropertyControl::setHorizontalSpaceBetweenControls(hdpi::Px space)
{
  useCustomHorizontalSpacing = true;
  horizontalSpaceBetweenControls = (float)hdpi::_px(space);
}

void ContainerPropertyControl::setVerticalSpaceBetweenControls(hdpi::Px space) { verticalSpaceBetweenControls = hdpi::_px(space); }

void ContainerPropertyControl::addVerticalSpaceAfterControl()
{
  const ImVec2 cursor = ImGui::GetCursorScreenPos();
  const int itemSpacingY = verticalSpaceBetweenControls - ((int)floorf(ImGui::GetStyle().ItemSpacing.y));
  ImGui::SetCursorScreenPos(ImVec2(cursor.x, cursor.y + itemSpacingY));
}

void ContainerPropertyControl::updateImgui()
{
  G_ASSERT(mControlArray.size() == mControlsNewLine.size());

  for (int i = 0; i < mControlArray.size();)
  {
    if (i != 0)
      addVerticalSpaceAfterControl();

    const int controlsInSameLine = getControlsInSameLine(i);
    if (controlsInSameLine > 1)
    {
      // Modifying the CellPadding is needed otherwise there would more space before and after rows that has a table.
      const float paddingX = useCustomHorizontalSpacing ? horizontalSpaceBetweenControls : ImGui::GetStyle().CellPadding.x;
      ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(paddingX, 0));

      // "t" stands for table. It could be anything.
      ImGui::PushID(i);
      if (ImGui::BeginTable("##t", controlsInSameLine, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchSame))
      {
        if (useFixedWidthColumns)
        {
          for (int column = 0; column < controlsInSameLine; ++column)
          {
            PropertyControlBase *control = mControlArray[i + column];
            ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, control->getWidth());
          }
        }

        for (int column = 0; column < controlsInSameLine; ++column)
        {
          PropertyControlBase *control = mControlArray[i + column];

          ImGui::TableNextColumn();

          ImGui::PushID(control);
          control->updateImgui();
          ImGui::PopID();
        }

        ImGui::EndTable();
      }
      ImGui::PopID();

      ImGui::PopStyleVar();

      i += controlsInSameLine;
    }
    else
    {
      PropertyControlBase *control = mControlArray[i];

      ImGui::PushID(control);
      if (control->getImguiControlType() == (int)ControlType::GroupBox)
      {
        const bool lastItem = (i + 1) == mControlArray.size();
        static_cast<GroupBoxPropertyControl *>(control)->groupBoxUpdateImgui(!lastItem);
      }
      else
      {
        control->updateImgui();
      }
      ImGui::PopID();

      ++i;
    }
  }
}

} // namespace PropPanel