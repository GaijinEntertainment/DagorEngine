//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/control/propertyControlBase.h>

namespace PropPanel
{

class ICustomControl;
class ISetControlParams;
class ITreeControlEventHandler;
class PropPanelScheme;

class ContainerPropertyControl : public PropertyControlBase
{
public:
  ContainerPropertyControl(int id, ControlEventHandler *event_handler, ContainerPropertyControl *parent, int x = 0, int y = 0,
    hdpi::Px w = hdpi::Px(0), hdpi::Px h = hdpi::Px(0));
  ~ContainerPropertyControl();

  // Creates

  virtual ContainerPropertyControl *createContainer(int id, bool new_line = true, hdpi::Px interval = hdpi::Px::ZERO);
  virtual ContainerPropertyControl *createExtensible(int id, bool new_line = true);
  virtual ContainerPropertyControl *createExtGroup(int id, const char caption[]);
  virtual ContainerPropertyControl *createGroup(int id, const char caption[]);
  virtual ContainerPropertyControl *createGroupHorzFlow(int id, const char caption[]);
  virtual ContainerPropertyControl *createGroupBox(int id, const char caption[]);
  virtual ContainerPropertyControl *createRadioGroup(int id, const char caption[], bool new_line = true);
  virtual ContainerPropertyControl *createTabPanel(int id, const char caption[]);
  virtual ContainerPropertyControl *createTabPage(int id, const char caption[]);
  virtual ContainerPropertyControl *createToolbarPanel(int id, const char caption[], bool new_line = true);
  virtual ContainerPropertyControl *createTree(int id, const char caption[], hdpi::Px height, bool new_line = true);
  virtual ContainerPropertyControl *createTreeCheckbox(int id, const char caption[], hdpi::Px height, bool new_line = true,
    bool icons_show = true);
  virtual ContainerPropertyControl *createMultiSelectTree(int id, const char caption[], hdpi::Px height, bool new_line = true);
  virtual ContainerPropertyControl *createMultiSelectTreeCheckbox(int id, const char caption[], hdpi::Px height, bool new_line = true);

  virtual void createStatic(int id, const char caption[], bool new_line = true);
  virtual void createEditBox(int id, const char caption[], const char text[] = "", bool enabled = true, bool new_line = true,
    bool multiline = false, hdpi::Px multi_line_height = hdpi::_pxScaled(40));
  virtual void createFileEditBox(int id, const char caption[], const char file[] = "", bool enabled = true, bool new_line = true);
  virtual void createFileButton(int id, const char caption[], const char file[] = "", bool enabled = true, bool new_line = true);
  virtual void createTargetButton(int id, const char caption[], const char text[] = "", bool enabled = true, bool new_line = true);
  virtual void createEditInt(int id, const char caption[], int value = 0, bool enabled = true, bool new_line = true);
  virtual void createEditFloat(int id, const char caption[], float value = 0, int prec = 2, bool enabled = true, bool new_line = true);
  virtual void createEditFloatWidthEx(int id, const char caption[], float value = 0, int prec = 2, bool enabled = true,
    bool new_line = true, bool width_includes_label = true);
  virtual void createTrackInt(int id, const char caption[], int value, int min, int max, int step, bool enabled = true,
    bool new_line = true);
  virtual void createTrackFloat(int id, const char caption[], float value, float min, float max, float step, bool enabled = true,
    bool new_line = true);
  virtual void createTrackFloatLogarithmic(int id, const char caption[], float value, float min, float max, float step, float power,
    bool enabled = true, bool new_line = true);
  virtual void createCheckBox(int id, const char caption[], bool value = false, bool enabled = true, bool new_line = true);
  virtual void createButton(int id, const char caption[], bool enabled = true, bool new_line = true);
  virtual void createButtonLText(int id, const char caption[], bool enabled = true, bool new_line = true);
  virtual void createIndent(int id = 0, bool new_line = true);
  virtual void createSeparator(int id = 0, bool new_line = true);
  virtual void createCombo(int id, const char caption[], const Tab<String> &vals, int index, bool enabled = true,
    bool new_line = true);
  virtual void createCombo(int id, const char caption[], const Tab<String> &vals, const char *selection, bool enabled = true,
    bool new_line = true);
  virtual void createSortedCombo(int id, const char caption[], const Tab<String> &vals, int index, bool enabled = true,
    bool new_line = true);
  virtual void createList(int id, const char caption[], const Tab<String> &vals, int index, bool enabled = true, bool new_line = true);
  virtual void createList(int id, const char caption[], const Tab<String> &vals, const char *selection, bool enabled = true,
    bool new_line = true);

  // You can also use Constants::LISTBOX_DEFAULT_HEIGHT and Constants::LISTBOX_FULL_HEIGHT for height.
  virtual void createMultiSelectList(int id, const Tab<String> &vals, hdpi::Px height, bool enabled = true, bool new_line = true);

  virtual void createRadio(int id, const char caption[], bool enabled = true, bool new_line = true);
  virtual void createColorBox(int id, const char caption[], E3DCOLOR value = E3DCOLOR(0, 0, 0, 255), bool enabled = true,
    bool new_line = true);
  virtual void createSimpleColor(int id, const char caption[], E3DCOLOR value = E3DCOLOR(0, 0, 0, 255), bool enabled = true,
    bool new_line = true);
  virtual void createPoint2(int id, const char caption[], Point2 value = Point2(0, 0), int prec = 2, bool enabled = true,
    bool new_line = true);
  virtual void createPoint3(int id, const char caption[], Point3 value = Point3(0, 0, 0), int prec = 2, bool enabled = true,
    bool new_line = true);
  virtual void createPoint4(int id, const char caption[], Point4 value = Point4(0, 0, 0, 0), int prec = 2, bool enabled = true,
    bool new_line = true);
  virtual void createMatrix(int id, const char caption[], const TMatrix &value = TMatrix::IDENT, int prec = 2, bool enabled = true,
    bool new_line = true);
  virtual void createGradientBox(int id, const char caption[], bool enabled = true, bool new_line = true);
  virtual void createTextGradient(int id, const char caption[], bool enabled = true, bool new_line = true);
  virtual void createGradientPlot(int id, const char caption[], hdpi::Px height = hdpi::Px::ZERO, bool enabled = true,
    bool new_line = true);
  virtual void createCurveEdit(int id, const char caption[], hdpi::Px height = hdpi::Px::ZERO, bool enabled = true,
    bool new_line = true);
  virtual void createTwoColorIndicator(int id, const char caption[], hdpi::Px height = hdpi::Px::ZERO, bool enabled = true,
    bool new_line = true);
  virtual void createPaletteCell(int id, const char caption[], bool enabled = true, bool new_line = true);
  virtual PropertyControlBase *createPlaceholder(int id, hdpi::Px height, bool new_line = true);
  virtual void createCustomControlHolder(int id, ICustomControl *control, bool new_line = true);
  virtual TLeafHandle createTreeLeaf(TLeafHandle parent, const char caption[], const char image[]);

  virtual void createIndirect(DataBlock &dataBlock, ISetControlParams &controlParams);

  virtual PropPanelScheme *createSceme();

  // Sets
  virtual void setEnabledById(int id, bool enabled);
  virtual void resetById(int id);
  virtual void setWidthById(int id, hdpi::Px w);
  virtual void setFocusById(int id);

  virtual void setUserData(int id, const void *value);
  virtual void setText(int id, const char value[]);
  virtual void setFloat(int id, float value);
  virtual void setInt(int id, int value);
  virtual void setBool(int id, bool value);
  virtual void setColor(int id, E3DCOLOR value);
  virtual void setPoint2(int id, Point2 value);
  virtual void setPoint3(int id, Point3 value);
  virtual void setPoint4(int id, Point4 value);
  virtual void setMatrix(int id, const TMatrix &value);
  virtual void setGradient(int id, PGradient value);
  virtual void setTextGradient(int id, const TextGradient &source);
  virtual void setControlPoints(int id, Tab<Point2> &points);

  virtual void setTooltipId(int id, const char text[]);

  // for indirect fill

  virtual void setMinMaxStep(int id, float min, float max, float step);
  virtual void setPrec(int id, int prec);
  virtual void setStrings(int id, const Tab<String> &vals);
  virtual void setSelection(int id, const Tab<int> &sels); // for multiselect list
  virtual void setCaption(int id, const char value[]);
  virtual void setButtonPictures(int id, const char *fname = NULL);

  // change strings in list
  virtual int addString(int id, const char *value);
  virtual void removeString(int id, int idx);

  // Gets
  virtual void *getUserData(int id) const;
  virtual SimpleString getText(int id) const;
  virtual float getFloat(int id) const;
  virtual int getInt(int id) const;
  virtual bool getBool(int id) const;
  virtual E3DCOLOR getColor(int id) const;
  virtual Point2 getPoint2(int id) const;
  virtual Point3 getPoint3(int id) const;
  virtual Point4 getPoint4(int id) const;
  virtual TMatrix getMatrix(int id) const;
  virtual void getGradient(int id, PGradient destGradient) const;
  virtual void getTextGradient(int id, TextGradient &destGradient) const;
  virtual void getCurveCoefs(int id, Tab<Point2> &points) const;
  virtual bool getCurveCubicCoefs(int id, Tab<Point2> &xy_4c_per_seg) const;

  virtual int getStrings(int id, Tab<String> &vals);
  virtual int getSelection(int id, Tab<int> &sels); // for multiselect list

  virtual unsigned getTypeMaskForSet() const override;
  virtual unsigned getTypeMaskForGet() const override;

  virtual SimpleString getCaption() const;

  virtual void clear();
  virtual unsigned getChildCount();

  virtual hdpi::Px getClientWidth();
  virtual hdpi::Px getClientHeight();

  virtual WindowBase *getWindow(); // for CPanelWindow it returns static with controls, for operating window use handles below
  virtual void *getWindowHandle();
  virtual void *getParentWindowHandle();

  virtual void onChildResize(int id) { G_UNUSED(id); }

  virtual const ContainerPropertyControl *getContainer() const override { return this; }
  virtual ContainerPropertyControl *getContainer() override { return this; }
  virtual bool isRealContainer() { return true; }
  virtual bool isImguiContainer() const { return true; }

  virtual PropertyControlBase *getById(int id, bool stopOnDifferentEventHandler = true) const; // for set, get, show, hide,
                                                                                               // disable, enable, resize
  virtual ContainerPropertyControl *getContainerById(int id) const; // for set, get, show, hide, disable, enable, resize
  virtual PropertyControlBase *getByIndex(int index) const;         // only for radio groups

  virtual void setPostEvent(int pcb_id);
  virtual void setVisible(bool show = true);
  // for use need call from panel with root window handle
  // if need update panel with removing and adding controls see flicker_free_change_panel()
  virtual void setFlickerFreeDraw(bool enable) { G_UNUSED(enable); }

  // saving and loading state with datablock

  virtual int saveState(DataBlock &datablk) override;
  virtual int loadState(DataBlock &datablk) override;

  // fast fill for large pannels

  virtual void disableFillAutoResize() {}
  virtual void restoreFillAutoResize() {}

  // panel content changing routines

  virtual bool removeById(int id);
  virtual bool moveById(int id, int ba_id = -1, bool after = false);

  // tree leaf handling
  virtual void setCaption(TLeafHandle leaf, const char value[])
  {
    G_UNUSED(leaf);
    G_UNUSED(value);
  }

  virtual String getCaption(TLeafHandle leaf) const
  {
    G_UNUSED(leaf);
    return String("");
  }

  virtual int getChildCount(TLeafHandle leaf) const
  {
    G_UNUSED(leaf);
    return 0;
  }

  virtual const char *getImageName(TLeafHandle leaf) const
  {
    G_UNUSED(leaf);
    return 0;
  }

  virtual TLeafHandle getChildLeaf(TLeafHandle leaf, unsigned idx)
  {
    G_UNUSED(leaf);
    G_UNUSED(idx);
    return nullptr;
  }

  virtual TLeafHandle getParentLeaf(TLeafHandle leaf)
  {
    G_UNUSED(leaf);
    return nullptr;
  }

  virtual TLeafHandle getNextLeaf(TLeafHandle leaf)
  {
    G_UNUSED(leaf);
    return nullptr;
  }

  virtual TLeafHandle getPrevLeaf(TLeafHandle leaf)
  {
    G_UNUSED(leaf);
    return nullptr;
  }

  virtual TLeafHandle getRootLeaf() { return nullptr; }

  virtual void setButtonPictures(TLeafHandle leaf, const char *fname = NULL)
  {
    G_UNUSED(leaf);
    G_UNUSED(fname);
  }

  virtual void setColor(TLeafHandle leaf, E3DCOLOR value)
  {
    G_UNUSED(leaf);
    G_UNUSED(value);
  }

  virtual void setBool(TLeafHandle leaf, bool open)
  {
    G_UNUSED(leaf);
    G_UNUSED(open);
  }

  virtual bool getBool(TLeafHandle leaf) const
  {
    G_UNUSED(leaf);
    return false;
  }

  virtual void setUserData(TLeafHandle leaf, const void *value)
  {
    G_UNUSED(leaf);
    G_UNUSED(value);
  }

  virtual void *getUserData(TLeafHandle leaf) const
  {
    G_UNUSED(leaf);
    return NULL;
  }

  virtual void setSelLeaf(TLeafHandle leaf, bool keep_selected = false)
  {
    G_UNUSED(leaf);
    G_UNUSED(keep_selected);
  }
  virtual TLeafHandle getSelLeaf() const { return NULL; }

  virtual bool removeLeaf(TLeafHandle id)
  {
    G_UNUSED(id);
    return false;
  }

  virtual bool swapLeaf(TLeafHandle id, bool after)
  {
    G_UNUSED(id);
    G_UNUSED(after);
    return false;
  }

  // also it can disable for state icon in tree
  virtual void setCheckboxEnable(TLeafHandle leaf, bool is_enable)
  {
    G_UNUSED(leaf);
    G_UNUSED(is_enable);
  }

  virtual bool isCheckboxEnable(TLeafHandle leaf) const
  {
    G_UNUSED(leaf);
    return false;
  }

  virtual void setCheckboxValue(TLeafHandle leaf, bool is_checked)
  {
    G_UNUSED(leaf);
    G_UNUSED(is_checked);
  }

  // also can return false if checkbox disabled
  virtual bool getCheckboxValue(TLeafHandle leaf) const
  {
    G_UNUSED(leaf);
    return false;
  }

  // for disable icon you can put "" or see setCheckboxEnable()
  virtual void setImageState(TLeafHandle leaf, const char *fname)
  {
    G_UNUSED(leaf);
    G_UNUSED(fname);
  }

  // for multi select tree
  virtual void getSelectedLeafs(dag::Vector<TLeafHandle> &leafs) const { G_UNUSED(leafs); }

  virtual void enableResizeCallback() {}
  virtual void disableResizeCallback() {}

  // When there are more than one control on a line then the available space is distributed evenly among them by default.
  // After calling setUseFixedWidthColumns the width of the controls is used instead.
  virtual void setUseFixedWidthColumns() { useFixedWidthColumns = true; }

  virtual void setHorizontalSpaceBetweenControls(hdpi::Px space);

  virtual void setTreeEventHandler(ITreeControlEventHandler *event_handler) {}
  virtual void setTreeCheckboxIcons(const char *checked, const char *unchecked) {}

  virtual void updateImgui() override;

protected:
  virtual void resizeControl(unsigned w, unsigned h)
  {
    G_UNUSED(w);
    G_UNUSED(h);
  }

  virtual void addControl(PropertyControlBase *pcontrol, bool new_line = true);
  virtual void onControlAdd(PropertyControlBase *control) { G_UNUSED(control); } // for container resize or add scroll

  virtual int getNextControlX(bool new_line = true);
  virtual int getNextControlY(bool new_line = true);

  int getControlsInSameLine(int start_index) const;
  void setVerticalSpaceBetweenControls(hdpi::Px space);

  // This is in lieu of simply using ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x,
  // verticalSpaceBetweenControls)); at the start of ContainerPropertyControl::updateImgui(). Unfortunately that interferes with
  // children (for example the spacing between pop-up menu items).
  void addVerticalSpaceAfterControl();

  Tab<PropertyControlBase *> mControlArray;
  Tab<bool> mControlsNewLine;

private:
  const bool mUpdate = false; // NOTE: ImGui porting: unused.

  int verticalSpaceBetweenControls;
  bool useFixedWidthColumns = false;

  bool useCustomHorizontalSpacing = false;
  float horizontalSpaceBetweenControls = 0.0f;
};

// iterate all tree leaves recursively (using children), root is usually nullptr and not iterated
template <typename C>
inline bool iterate_children_leafs(ContainerPropertyControl &control, C &cb, TLeafHandle root = nullptr)
{
  for (unsigned i = 0, n = control.getChildCount(root); i < n; i++)
    if (!cb(root, n, control.getChildLeaf(root, i)))
      return false;
  for (unsigned i = 0, n = control.getChildCount(root); i < n; i++)
    if (!iterate_children_leafs(control, cb, control.getChildLeaf(root, i)))
      return false;
  return true;
}

// iterate all tree leaves linearly starting from some leaf
template <typename C>
inline bool iterate_all_leafs_linear(ContainerPropertyControl &control, TLeafHandle start_leaf, const C &cb)
{
  for (TLeafHandle leaf = start_leaf; leaf;)
    if (!cb(leaf))
      return false;
    else if ((leaf = control.getNextLeaf(leaf)) == start_leaf)
      break;
  return true;
}

} // namespace PropPanel