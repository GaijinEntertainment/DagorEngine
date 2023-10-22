// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <propPanel2/c_panel_base.h>
#include <propPanel2/c_indirect.h>

#include "panelControls/p_button.h"
#include "panelControls/p_check_box.h"
#include "panelControls/p_edit_box.h"
#include "panelControls/p_file_edit_box.h"
#include "panelControls/p_file_button.h"
#include "panelControls/p_target_button.h"
#include "panelControls/p_group_box.h"
#include "panelControls/p_indent.h"
#include "panelControls/p_placeholder.h"
#include "panelControls/p_radio_group.h"
#include "panelControls/p_separator.h"
#include "panelControls/p_spin_edit_int.h"
#include "panelControls/p_spin_edit_float.h"
#include "panelControls/p_static.h"
#include "panelControls/p_tab_panel.h"
#include "panelControls/p_tab_page.h"
#include "panelControls/p_track_bar_float.h"
#include "panelControls/p_track_bar_int.h"
#include "panelControls/p_toolbar.h"
#include "panelControls/p_radio_button.h"
#include "panelControls/p_point4.h"
#include "panelControls/p_point3.h"
#include "panelControls/p_point2.h"
#include "panelControls/p_list_box.h"
#include "panelControls/p_multi_select_list_box.h"
#include "panelControls/p_group.h"
#include "panelControls/p_gradient_box.h"
#include "panelControls/p_text_gradient.h"
#include "panelControls/p_gradient_plot.h"
#include "panelControls/p_curve_edit.h"
#include "panelControls/p_combo_box.h"
#include "panelControls/p_color_box.h"
#include "panelControls/p_simple_color.h"
#include "panelControls/p_container.h"
#include "panelControls/p_color_controls.h"
#include "panelControls/p_extensible_container.h"
#include "panelControls/p_extensible_group.h"
#include "panelControls/p_tree.h"
#include "panelControls/p_matrix.h"

#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug.h>

// ----------------------------------------------------------------------------

PropertyContainerControlBase::PropertyContainerControlBase(int id, ControlEventHandler *event_handler,
  PropertyContainerControlBase *parent, int x, int y, hdpi::Px w, hdpi::Px h) :
  PropertyControlBase(id, event_handler, parent, x, y, w, h), mControlArray(midmem), mControlsNewLine(midmem), mUpdate(false)
{
  this->clear();
}

PropertyContainerControlBase::~PropertyContainerControlBase() { this->clear(); }

//------------------------------ Creators -------------------------------------


PropertyContainerControlBase *PropertyContainerControlBase::createContainer(int id, bool new_line, hdpi::Px interval)
{
  CContainer *newContainer =
    new CContainer(this->mEventHandler, this, id, this->getNextControlX(), this->getNextControlY(new_line), this->getClientWidth(),
      _pxScaled(DEFAULT_GROUP_HEIGHT), interval == hdpi::Px::ZERO ? _pxScaled(DEFAULT_CONTROLS_INTERVAL) : interval);

  this->addControl(newContainer, new_line);
  return newContainer;
}


PropertyContainerControlBase *PropertyContainerControlBase::createExtensible(int id, bool new_line)
{
  CExtensible *newExtensible = new CExtensible(this->mEventHandler, this, id, this->getNextControlX(), this->getNextControlY(new_line),
    this->getClientWidth(), _pxScaled(DEFAULT_CONTROL_HEIGHT));

  this->addControl(newExtensible, new_line);
  return newExtensible;
}


PropertyContainerControlBase *PropertyContainerControlBase::createExtGroup(int id, const char text[])
{
  CGroup *newExtGroup = new CExtGroup(this->mEventHandler, this, id, this->getNextControlX(), this->getNextControlY(),
    this->getClientWidth(), _pxScaled(DEFAULT_GROUP_HEIGHT), text);

  this->addControl(newExtGroup);
  return newExtGroup;
}


PropertyContainerControlBase *PropertyContainerControlBase::createGroup(int id, const char text[])
{
  CGroup *newGroup = new CGroup(this->mEventHandler, this, id, this->getNextControlX(), this->getNextControlY(),
    this->getClientWidth(), _pxScaled(DEFAULT_GROUP_HEIGHT), text);

  this->addControl(newGroup);
  return newGroup;
}

PropertyContainerControlBase *PropertyContainerControlBase::createGroupHorzFlow(int id, const char caption[])
{
  CGroup *newGroup = new CGroup(this->mEventHandler, this, id, this->getNextControlX(), this->getNextControlY(),
    this->getClientWidth(), _pxScaled(DEFAULT_GROUP_HEIGHT), caption, HorzFlow::Enabled);

  this->addControl(newGroup);
  return newGroup;
}

PropertyContainerControlBase *PropertyContainerControlBase::createGroupBox(int id, const char text[])
{
  CGroupBox *newGroupBox = new CGroupBox(this->mEventHandler, this, id, this->getNextControlX(), this->getNextControlY(),
    this->getClientWidth(), _pxScaled(DEFAULT_GROUP_HEIGHT), text);

  this->addControl(newGroupBox);
  return newGroupBox;
}


PropertyContainerControlBase *PropertyContainerControlBase::createRadioGroup(int id, const char caption[], bool new_line)
{
  CRadioGroup *newRadioGroup = new CRadioGroup(this->mEventHandler, this, id, this->getNextControlX(), this->getNextControlY(new_line),
    this->getClientWidth(), _pxScaled(DEFAULT_GROUP_HEIGHT), caption);

  this->addControl(newRadioGroup, new_line);
  return newRadioGroup;
}


PropertyContainerControlBase *PropertyContainerControlBase::createTabPanel(int id, const char caption[])
{
  CTabPanel *newTabPanel = new CTabPanel(this->mEventHandler, this, id, this->getNextControlX(), this->getNextControlY(),
    this->getClientWidth(), _pxScaled(DEFAULT_TAB_HEIGHT), caption);

  this->addControl(newTabPanel);
  return newTabPanel;
}


PropertyContainerControlBase *PropertyContainerControlBase::createToolbarPanel(int id, const char caption[], bool new_line)
{
  CToolbar *newToolbarPanel = new CToolbar(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), caption);

  this->addControl(newToolbarPanel, new_line);
  return newToolbarPanel;
}


PropertyContainerControlBase *PropertyContainerControlBase::createTree(int id, const char caption[], hdpi::Px height, bool new_line)
{
  CTree *newTree = new CTree(this->mEventHandler, this, id, this->getNextControlX(new_line), this->getNextControlY(new_line),
    this->getClientWidth(), height, caption);

  this->addControl(newTree, new_line);
  return newTree;
}


PropertyContainerControlBase *PropertyContainerControlBase::createTabPage(int id, const char caption[])
{
  G_ASSERT(false && "TabPage can be created only in TabPanel!");
  return NULL;
}


void PropertyContainerControlBase::createStatic(int id, const char caption[], bool new_line)
{
  CStatic *newStatic = new CStatic(this->mEventHandler, this, id, this->getNextControlX(new_line), this->getNextControlY(new_line),
    this->getClientWidth(), caption);

  this->addControl(newStatic, new_line);
}


void PropertyContainerControlBase::createEditBox(int id, const char caption[], const char text[], bool enabled, bool new_line,
  bool multiline, hdpi::Px multi_line_height)
{
  CEditBox *newEditBox = new CEditBox(this->mEventHandler, this, id, this->getNextControlX(new_line), this->getNextControlY(new_line),
    this->getClientWidth(), caption, multiline);

  newEditBox->setTextValue(text);
  newEditBox->setEnabled(enabled);
  if (multiline)
    newEditBox->setHeight(multi_line_height);
  this->addControl(newEditBox, new_line);
}


void PropertyContainerControlBase::createFileEditBox(int id, const char caption[], const char file[], bool enabled, bool new_line)
{
  CFileEditBox *newFileEditBox = new CFileEditBox(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), caption);

  newFileEditBox->setTextValue(file);
  newFileEditBox->setEnabled(enabled);

  this->addControl(newFileEditBox, new_line);
}


void PropertyContainerControlBase::createFileButton(int id, const char caption[], const char file[], bool enabled, bool new_line)
{
  CFileButton *newFileButton = new CFileButton(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), caption);

  newFileButton->setTextValue(file);
  newFileButton->setEnabled(enabled);

  this->addControl(newFileButton, new_line);
}


void PropertyContainerControlBase::createTargetButton(int id, const char caption[], const char text[], bool enabled, bool new_line)
{
  CTargetButton *newTargetButton = new CTargetButton(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), caption);

  newTargetButton->setTextValue(text);
  newTargetButton->setEnabled(enabled);

  this->addControl(newTargetButton, new_line);
}


void PropertyContainerControlBase::createEditInt(int id, const char caption[], int value, bool enabled, bool new_line)
{
  CSpinEditInt *newSpinInt = new CSpinEditInt(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), caption);

  newSpinInt->setIntValue(value);
  newSpinInt->setEnabled(enabled);

  this->addControl(newSpinInt, new_line);
}


void PropertyContainerControlBase::createEditFloat(int id, const char caption[], float value, int prec, bool enabled, bool new_line)
{
  CSpinEditFloat *newSpinFloat = new CSpinEditFloat(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), caption, prec);

  newSpinFloat->setFloatValue(value);
  newSpinFloat->setEnabled(enabled);

  this->addControl(newSpinFloat, new_line);
}


void PropertyContainerControlBase::createCheckBox(int id, const char caption[], bool value, bool enabled, bool new_line)
{
  CCheckBox *newCheckBox = new CCheckBox(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), caption);

  newCheckBox->setBoolValue(value);
  newCheckBox->setEnabled(enabled);

  this->addControl(newCheckBox, new_line);
}


void PropertyContainerControlBase::createButton(int id, const char caption[], bool enabled, bool new_line)
{
  CButton *newButton = new CButton(this->mEventHandler, this, id, this->getNextControlX(new_line), this->getNextControlY(new_line),
    this->getClientWidth(), caption);

  newButton->setEnabled(enabled);

  this->addControl(newButton, new_line);
}


void PropertyContainerControlBase::createIndent(int id, bool new_line)
{
  CIndent *newIdent = new CIndent(this->mEventHandler, this, id, this->getNextControlX(new_line), this->getNextControlY(new_line),
    this->getClientWidth());

  this->addControl(newIdent, new_line);
}


void PropertyContainerControlBase::createSeparator(int id, bool new_line)
{
  CSeparator *newSeparator = new CSeparator(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth());

  this->addControl(newSeparator, new_line);
}


void PropertyContainerControlBase::createTrackInt(int id, const char caption[], int value, int min, int max, int step, bool enabled,
  bool new_line)
{
  CTrackBarInt *newTrackBarInt = new CTrackBarInt(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), caption, min, max, step);

  newTrackBarInt->setIntValue(value);
  newTrackBarInt->setEnabled(enabled);

  this->addControl(newTrackBarInt, new_line);
}


void PropertyContainerControlBase::createTrackFloat(int id, const char caption[], float value, float min, float max, float step,
  bool enabled, bool new_line)
{
  CTrackBarFloat *newTrackBarFloat = new CTrackBarFloat(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), caption, min, max, step);

  newTrackBarFloat->setFloatValue(value);
  newTrackBarFloat->setEnabled(enabled);

  this->addControl(newTrackBarFloat, new_line);
}


void PropertyContainerControlBase::createTrackFloatLogarithmic(int id, const char caption[], float value, float min, float max,
  float step, float power, bool enabled, bool new_line)
{
  CTrackBarFloat *newTrackBarFloat = new CTrackBarFloat(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), caption, min, max, step, power);

  newTrackBarFloat->setFloatValue(value);
  newTrackBarFloat->setEnabled(enabled);

  this->addControl(newTrackBarFloat, new_line);
}


void PropertyContainerControlBase::createCombo(int id, const char caption[], const Tab<String> &vals, int index, bool enabled,
  bool new_line)
{
  CComboBox *newComboBox = new CComboBox(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), caption, vals, index);

  newComboBox->setEnabled(enabled);

  this->addControl(newComboBox, new_line);
}


void PropertyContainerControlBase::createList(int id, const char caption[], const Tab<String> &vals, int index, bool enabled,
  bool new_line)
{
  CListBox *newListBox = new CListBox(this->mEventHandler, this, id, this->getNextControlX(new_line), this->getNextControlY(new_line),
    this->getClientWidth(), caption, vals, index);

  newListBox->setEnabled(enabled);

  this->addControl(newListBox, new_line);
}


static int get_index_in_tab(dag::ConstSpan<String> vals, const char *selection)
{
  for (int i = 0; i < vals.size(); ++i)
    if (vals[i] == selection)
      return i;

  return -1;
}


void PropertyContainerControlBase::createCombo(int id, const char caption[], const Tab<String> &vals, const char *selection,
  bool enabled, bool new_line)
{
  int index = ::get_index_in_tab(vals, selection);
  PropertyContainerControlBase::createCombo(id, caption, vals, index, enabled, new_line);
}


void PropertyContainerControlBase::createSortedCombo(int id, const char caption[], const Tab<String> &vals, int index, bool enabled,
  bool new_line)
{
  CComboBox *newComboBox = new CComboBox(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), caption, vals, index, true);

  newComboBox->setEnabled(enabled);

  this->addControl(newComboBox, new_line);
}


void PropertyContainerControlBase::createList(int id, const char caption[], const Tab<String> &vals, const char *selection,
  bool enabled, bool new_line)
{
  int index = ::get_index_in_tab(vals, selection);
  PropertyContainerControlBase::createList(id, caption, vals, index, enabled, new_line);
}


void PropertyContainerControlBase::createMultiSelectList(int id, const Tab<String> &vals, hdpi::Px height, bool enabled, bool new_line)
{
  CMultiSelectListBox *newListBox = new CMultiSelectListBox(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), height, vals);

  newListBox->setEnabled(enabled);

  this->addControl(newListBox, new_line);
}


void PropertyContainerControlBase::createRadio(int ind, const char caption[], bool enabled, bool new_line)
{
  CRadioButton *newRadioButton = new CRadioButton(this->mEventHandler, this, this->getID(), this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), caption, ind);

  newRadioButton->setEnabled(enabled);

  this->addControl(newRadioButton, new_line);
}


void PropertyContainerControlBase::createColorBox(int id, const char caption[], E3DCOLOR value, bool enabled, bool new_line)
{
  CColorBox *newColorBox = new CColorBox(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), caption);

  newColorBox->setColorValue(value);
  newColorBox->setEnabled(enabled);

  this->addControl(newColorBox, new_line);
}


void PropertyContainerControlBase::createSimpleColor(int id, const char caption[], E3DCOLOR value, bool enabled, bool new_line)
{
  CSimpleColor *newSimpleColor = new CSimpleColor(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), caption);

  newSimpleColor->setColorValue(value);
  newSimpleColor->setEnabled(enabled);

  this->addControl(newSimpleColor, new_line);
}


void PropertyContainerControlBase::createPoint2(int id, const char caption[], Point2 value, int prec, bool enabled, bool new_line)
{
  CPoint2 *newPoint2 = new CPoint2(this->mEventHandler, this, id, this->getNextControlX(new_line), this->getNextControlY(new_line),
    this->getClientWidth(), caption, prec);

  newPoint2->setPoint2Value(value);
  newPoint2->setEnabled(enabled);

  this->addControl(newPoint2, new_line);
}


void PropertyContainerControlBase::createPoint3(int id, const char caption[], Point3 value, int prec, bool enabled, bool new_line)
{
  CPoint3 *newPoint3 = new CPoint3(this->mEventHandler, this, id, this->getNextControlX(new_line), this->getNextControlY(new_line),
    this->getClientWidth(), caption, prec);

  newPoint3->setPoint3Value(value);
  newPoint3->setEnabled(enabled);

  this->addControl(newPoint3, new_line);
}


void PropertyContainerControlBase::createPoint4(int id, const char caption[], Point4 value, int prec, bool enabled, bool new_line)
{
  CPoint4 *newPoint4 = new CPoint4(this->mEventHandler, this, id, this->getNextControlX(new_line), this->getNextControlY(new_line),
    this->getClientWidth(), caption, prec);

  newPoint4->setPoint4Value(value);
  newPoint4->setEnabled(enabled);

  this->addControl(newPoint4, new_line);
}


void PropertyContainerControlBase::createMatrix(int id, const char caption[], const TMatrix &value, int prec, bool enabled,
  bool new_line)
{
  CMatrix *newMatrix = new CMatrix(this->mEventHandler, this, id, this->getNextControlX(new_line), this->getNextControlY(new_line),
    this->getClientWidth(), caption, prec);

  newMatrix->setMatrixValue(value);
  newMatrix->setEnabled(enabled);

  this->addControl(newMatrix, new_line);
}


void PropertyContainerControlBase::createGradientBox(int id, const char caption[], bool enabled, bool new_line)
{
  CGradientBox *newGradientBox = new CGradientBox(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), caption);

  newGradientBox->setEnabled(enabled);

  this->addControl(newGradientBox, new_line);
}


void PropertyContainerControlBase::createTextGradient(int id, const char caption[], bool enabled, bool new_line)
{
  CTextGradient *newTextGradient = new CTextGradient(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), caption);

  newTextGradient->setEnabled(enabled);

  this->addControl(newTextGradient, new_line);
}


void PropertyContainerControlBase::createGradientPlot(int id, const char caption[], hdpi::Px height, bool enabled, bool new_line)
{
  CGradientPlot *newGradientPlot = new CGradientPlot(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), height != hdpi::Px::ZERO ? height : _pxScaled(GRADIENT_HEIGHT), caption);

  newGradientPlot->setEnabled(enabled);

  this->addControl(newGradientPlot, new_line);
}


void PropertyContainerControlBase::createCurveEdit(int id, const char caption[], hdpi::Px height, bool enabled, bool new_line)
{
  CCurveEdit *newCurveEdit =
    new CCurveEdit(this->mEventHandler, this, id, this->getNextControlX(new_line), this->getNextControlY(new_line),
      this->getClientWidth(), height != hdpi::Px::ZERO ? height : _pxScaled(DEFAULT_CURVE_HEIGHT), caption);

  newCurveEdit->setEnabled(enabled);

  this->addControl(newCurveEdit, new_line);
}


void PropertyContainerControlBase::createTwoColorIndicator(int id, const char caption[], hdpi::Px height, bool enabled, bool new_line)
{
  CTwoColorIndicator *newTwoColorIndicator = new CTwoColorIndicator(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), height != hdpi::Px::ZERO ? height : _pxScaled(DEFAULT_CONTROL_HEIGHT));

  newTwoColorIndicator->setEnabled(enabled);

  this->addControl(newTwoColorIndicator, new_line);
}


void PropertyContainerControlBase::createPaletteCell(int id, const char caption[], bool enabled, bool new_line)
{
  CPaletteCell *newPaletteCell =
    new CPaletteCell(this->mEventHandler, this, id, this->getNextControlX(new_line), this->getNextControlY(new_line));

  newPaletteCell->setEnabled(enabled);

  this->addControl(newPaletteCell, new_line);
}


PropertyControlBase *PropertyContainerControlBase::createPlaceholder(int id, hdpi::Px height, bool new_line)
{
  CPlaceholder *newPlaceholder = new CPlaceholder(this->mEventHandler, this, id, this->getNextControlX(new_line),
    this->getNextControlY(new_line), this->getClientWidth(), height);

  this->addControl(newPlaceholder, new_line);

  return newPlaceholder;
}


TLeafHandle PropertyContainerControlBase::createTreeLeaf(TLeafHandle parent, const char caption[], const char image[])
{
  G_ASSERT(false && "Tree leaf can be created only in Tree control!");
  return 0;
}


//-----------------------------------------------------------------------------

PropPanelScheme *PropertyContainerControlBase::createSceme()
{
  PropPanelScheme *_scheme = new PropPanelScheme();

  return _scheme;
}

//-----------------------------------------------------------------------------

void PropertyContainerControlBase::createIndirect(DataBlock &dataBlock, ISetControlParams &controlParams)
{
  struct ControlRec
  {
    int id;
    const char *control_name;
    PropertyContainerControlBase *(*create_method)(int, PropertyContainerControlBase *, const char *, bool);
  };

  ControlRec recs[] = {{0, "Group", &CGroup::createDefault}, {0, "GroupBox", &CGroupBox::createDefault},
    {0, "RadioGroup", &CRadioGroup::createDefault}, {0, "TabPanel", &CTabPanel::createDefault},
    {0, "Toolbar", &CToolbar::createDefault}, {0, "TabPage", &CTabPage::createDefault}, {0, "EditBox", &CEditBox::createDefault},
    {0, "FileEditBox", &CFileEditBox::createDefault}, {0, "FileButton", &CFileButton::createDefault},
    {0, "Static", &CStatic::createDefault}, {0, "SpinEditInt", &CSpinEditInt::createDefault},
    {0, "SpinEditFloat", &CSpinEditFloat::createDefault}, {0, "CheckBox", &CCheckBox::createDefault},
    {0, "Button", &CButton::createDefault}, {0, "Indent", &CIndent::createDefault}, {0, "Separator", &CSeparator::createDefault},
    {0, "TrackBarInt", &CTrackBarInt::createDefault}, {0, "TrackBarFloat", &CTrackBarFloat::createDefault},
    {0, "ComboBox", &CComboBox::createDefault}, {0, "ListBox", &CListBox::createDefault},
    {0, "MultiSelectListBox", &CMultiSelectListBox::createDefault}, {0, "RadioButton", &CRadioButton::createDefault},
    {0, "ColorBox", &CColorBox::createDefault}, {0, "Point2", &CPoint2::createDefault}, {0, "Point3", &CPoint3::createDefault},
    {0, "Point4", &CPoint4::createDefault}, {0, "Gradient", &CGradientBox::createDefault},
    {0, "GradientPlot", &CGradientPlot::createDefault}, {0, "CurveEdit", &CCurveEdit::createDefault},
    {0, "Matrix", &CMatrix::createDefault}};

  int recs_count = sizeof(recs) / sizeof(recs[0]);
  for (int i = 0; i < recs_count; i++)
    recs[i].id = dataBlock.getNameId(recs[i].control_name);

  for (int i = 0; i < dataBlock.blockCount(); ++i)
  {
    DataBlock &controlBlock = *dataBlock.getBlock(i);
    int control_name_id = controlBlock.getNameId(controlBlock.getBlockName());
    int block_id = controlBlock.getInt("id", -1);
    const char *name = controlBlock.getStr("name", "noname");
    bool on_prev_line = controlBlock.getBool("on_prev_line", false);

    // find component, that will be create
    for (int j = 0; j < recs_count; j++)
    {
      if (recs[j].id == control_name_id)
      {
        // if component is group - create it's childrens
        if (PropertyContainerControlBase *may_be_group = recs[j].create_method(block_id, this, name, !on_prev_line))
          may_be_group->createIndirect(controlBlock, controlParams);
        break;
      }
    }
    controlParams.setParams(block_id);
  }
}

//-----------------------------------------------------------------------------

void PropertyContainerControlBase::setEnabledById(int id, bool enabled)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
    ptr->setEnabled(enabled);
}


void PropertyContainerControlBase::resetById(int id)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
    ptr->reset();
}


void PropertyContainerControlBase::setWidthById(int id, hdpi::Px w)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
    ptr->setWidth(w);
}


void PropertyContainerControlBase::setFocusById(int id)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
    ptr->setFocus();
}


void PropertyContainerControlBase::setUserData(int id, const void *value)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getUserDataValue() != value))
      ptr->reset();
    else
      ptr->setUserDataValue(value);
  }
}


void PropertyContainerControlBase::setText(int id, const char value[])
{
  PropertyControlBase *ptr = this->getById(id);
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


void PropertyContainerControlBase::setFloat(int id, float value)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getFloatValue() != value))
      ptr->reset();
    else
      ptr->setFloatValue(value);
  }
}

void PropertyContainerControlBase::setInt(int id, int value)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getIntValue() != value))
      ptr->reset();
    else
      ptr->setIntValue(value);
  }
}


void PropertyContainerControlBase::setBool(int id, bool value)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getBoolValue() != value))
      ptr->reset();
    else
      ptr->setBoolValue(value);
  }
}


void PropertyContainerControlBase::setColor(int id, E3DCOLOR value)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getColorValue() != value))
      ptr->reset();
    else
      ptr->setColorValue(value);
  }
}


void PropertyContainerControlBase::setPoint2(int id, Point2 value)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getPoint2Value() != value))
      ptr->reset();
    else
      ptr->setPoint2Value(value);
  }
}


void PropertyContainerControlBase::setPoint3(int id, Point3 value)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getPoint3Value() != value))
      ptr->reset();
    else
      ptr->setPoint3Value(value);
  }
}


void PropertyContainerControlBase::setPoint4(int id, Point4 value)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getPoint4Value() != value))
      ptr->reset();
    else
      ptr->setPoint4Value(value);
  }
}


void PropertyContainerControlBase::setMatrix(int id, const TMatrix &value)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    if ((mUpdate) && (ptr->getMatrixValue() != value))
      ptr->reset();
    else
      ptr->setMatrixValue(value);
  }
}


void PropertyContainerControlBase::setGradient(int id, PGradient value)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    ptr->setGradientValue(value);
  }
}


void PropertyContainerControlBase::setTextGradient(int id, const TextGradient &source)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    ptr->setTextGradientValue(source);
  }
}


void PropertyContainerControlBase::setControlPoints(int id, Tab<Point2> &points)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    ptr->setControlPointsValue(points);
  }
}


//------------------------------- for indirect fill ---------------------------


void PropertyContainerControlBase::setMinMaxStep(int id, float min, float max, float step)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    ptr->setMinMaxStepValue(min, max, step);
  }
}


void PropertyContainerControlBase::setPrec(int id, int prec)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    ptr->setPrecValue(prec);
  }
}


void PropertyContainerControlBase::setStrings(int id, const Tab<String> &vals)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    ptr->setStringsValue(vals);
  }
}


void PropertyContainerControlBase::setSelection(int id, const Tab<int> &sels)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    ptr->setSelectionValue(sels);
  }
}


void PropertyContainerControlBase::setCaption(int id, const char value[])
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    ptr->setCaptionValue(value);
  }
}


void PropertyContainerControlBase::setButtonPictures(int id, const char *fname)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    ptr->setButtonPictureValues(fname);
  }
}

int PropertyContainerControlBase::addString(int id, const char *value)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    return ptr->addStringValue(value);

  return 0;
}

void PropertyContainerControlBase::removeString(int id, int idx)
{
  PropertyControlBase *ptr = getById(id);
  if (ptr)
    ptr->removeStringValue(idx);
}


//------------------------------- gets ----------------------------------------


void *PropertyContainerControlBase::getUserData(int id) const
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
    return ptr->getUserDataValue();

  return NULL;
}


SimpleString PropertyContainerControlBase::getText(int id) const
{
  SimpleString buffer;
  buffer.allocBuffer(STRING_SIZE);

  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
    ptr->getTextValue(buffer.str(), STRING_SIZE);

  return buffer;
}

SimpleString PropertyContainerControlBase::getCaption() const
{
  SimpleString buffer;
  buffer.allocBuffer(256);
  getCaptionValue(buffer.str(), 256);
  return buffer;
}


float PropertyContainerControlBase::getFloat(int id) const
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
    return ptr->getFloatValue();

  return 0;
}


int PropertyContainerControlBase::getInt(int id) const
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
    return ptr->getIntValue();

  return 0;
}


bool PropertyContainerControlBase::getBool(int id) const
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
    return ptr->getBoolValue();

  return false;
}

E3DCOLOR PropertyContainerControlBase::getColor(int id) const
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
    return ptr->getColorValue();

  return 0;
}


Point2 PropertyContainerControlBase::getPoint2(int id) const
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
    return ptr->getPoint2Value();

  return Point2(0, 0);
}


Point3 PropertyContainerControlBase::getPoint3(int id) const
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
    return ptr->getPoint3Value();

  return Point3(0, 0, 0);
}


Point4 PropertyContainerControlBase::getPoint4(int id) const
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
    return ptr->getPoint4Value();

  return Point4(0, 0, 0, 0);
}


TMatrix PropertyContainerControlBase::getMatrix(int id) const
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
    return ptr->getMatrixValue();

  return TMatrix::IDENT;
}


void PropertyContainerControlBase::getGradient(int id, PGradient destGradient) const
{
  PropertyControlBase *ptr = this->getById(id);
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


void PropertyContainerControlBase::getTextGradient(int id, TextGradient &destGradient) const
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
    ptr->getTextGradientValue(destGradient);
  else
    clear_and_shrink(destGradient);
}


void PropertyContainerControlBase::getCurveCoefs(int id, Tab<Point2> &points) const
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
    ptr->getCurveCoefsValue(points);
  else
    clear_and_shrink(points);
}

bool PropertyContainerControlBase::getCurveCubicCoefs(int id, Tab<Point2> &xy_4c_per_seg) const
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
    return ptr->getCurveCubicCoefsValue(xy_4c_per_seg);
  clear_and_shrink(xy_4c_per_seg);
  return false;
}

int PropertyContainerControlBase::getStrings(int id, Tab<String> &vals)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    return ptr->getStringsValue(vals);
  }

  return 0;
}


int PropertyContainerControlBase::getSelection(int id, Tab<int> &sels)
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    ptr->getSelectionValue(sels);
  }

  return 0;
}


WindowBase *PropertyContainerControlBase::getWindow() { return NULL; }


void *PropertyContainerControlBase::getWindowHandle()
{
  if (!this->getWindow())
    return 0;
  else
    return this->getWindow()->getHandle();
}


void *PropertyContainerControlBase::getParentWindowHandle()
{
  if (!this->getWindow())
    return 0;
  else
    return this->getWindow()->getParentHandle();
}


PropertyControlBase *PropertyContainerControlBase::getById(int id, bool stopOnDifferentEventHandler) const
{
  for (int i = 0; i < mControlArray.size(); i++)
  {
    if (mControlArray[i]->getID() == id)
      return mControlArray[i];

    const PropertyContainerControlBase *container = mControlArray[i]->getContainer();

    if (container && (!stopOnDifferentEventHandler || (container->getEventHandler() == this->getEventHandler())))
    {
      PropertyControlBase *result = container->getById(id);
      if (result)
        return result;
    }
  }

  return NULL;
}


PropertyContainerControlBase *PropertyContainerControlBase::getContainerById(int id) const
{
  PropertyControlBase *control = getById(id);
  if (!control)
    return NULL;

  return control->getContainer();
}


PropertyControlBase *PropertyContainerControlBase::getByIndex(int index) const
{
  if ((index >= 0) && (index < mControlArray.size()))
    return mControlArray[index];

  return NULL;
}


// ----------------------------------------------------------------------------


hdpi::Px PropertyContainerControlBase::getClientWidth()
{
  WindowBase *win = this->getWindow();
  if (win)
  {
    int result = win->getWidth() - 2 * _pxS(DEFAULT_CONTROLS_INTERVAL);
    return _pxActual((result > 0) ? result : 0);
  }

  debug("PropertyContainerControlBase::getClientWidth(): getWindow() == NULL!");
  return hdpi::Px::ZERO;
}


hdpi::Px PropertyContainerControlBase::getClientHeight() { return _pxActual(this->getNextControlY()); }


void PropertyContainerControlBase::clear()
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


unsigned PropertyContainerControlBase::getChildCount() { return mControlArray.size(); }


void PropertyContainerControlBase::setVisible(bool show)
{
  WindowBase *win = this->getWindow();
  if (win)
  {
    if (show)
      win->show();
    else
      win->hide();
  }
}


// ----------------------------------------------------------------------------
// panel content changing routines


bool PropertyContainerControlBase::removeById(int id)
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

    PropertyContainerControlBase *container = mControlArray[i]->getContainer();

    if (container && (container->getEventHandler() == this->getEventHandler()))
    {
      bool result = container->removeById(id);
      if (result)
        return result;
    }
  }

  return false;
}


bool PropertyContainerControlBase::moveById(int id, int ba_id, bool after)
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
  getWindow()->hide();
  restoreFillAutoResize();
  getWindow()->show();
  // getWindow()->refresh();

  return true;
}


// ----------------------------------------------------------------------------


void PropertyContainerControlBase::addControl(PropertyControlBase *pcontrol, bool new_line)
{
  mControlsNewLine.push_back(new_line);
  mControlArray.push_back(pcontrol);
  pcontrol->enableChangeHandlers();

  this->onControlAdd(pcontrol);
}

int PropertyContainerControlBase::getNextControlX(bool new_line) { return _pxS(DEFAULT_CONTROLS_INTERVAL); }

int PropertyContainerControlBase::getNextControlY(bool new_line) { return _pxS(DEFAULT_CONTROLS_INTERVAL); }

// ----------------------------------------------------------------------------

int PropertyContainerControlBase::saveState(DataBlock &datablk)
{
  __super::saveState(datablk);

  for (int i = 0; i < mControlArray.size(); i++)
  {
    mControlArray[i]->saveState(datablk);
  }

  return 0;
}

// ----------------------------------------------------------------------------

int PropertyContainerControlBase::loadState(DataBlock &datablk)
{
  __super::loadState(datablk);

  for (int i = 0; i < mControlArray.size(); i++)
  {
    mControlArray[i]->loadState(datablk);
  }

  return 0;
}

void PropertyContainerControlBase::setTooltipId(int id, const char text[])
{
  PropertyControlBase *ptr = this->getById(id);
  if (ptr)
  {
    ptr->setTooltip(text);
  }
}
