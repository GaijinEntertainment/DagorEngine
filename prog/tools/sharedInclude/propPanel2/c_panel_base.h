//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <propPanel2/c_panel_control.h>
#include <propPanel2/c_util.h>
#include <util/dag_globDef.h>


class DataBlock;
class ISetControlParams;
class PropPanelScheme;
typedef class PropertyContainerControlBase PropPanel2;


class PropertyContainerControlBase : public PropertyControlBase
{
public:
  PropertyContainerControlBase(int id, ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int x, int y,
    unsigned w, unsigned h);
  virtual ~PropertyContainerControlBase();

  // Creates

  virtual PropertyContainerControlBase *createContainer(int id, bool new_line = true, int interval = -1);
  virtual PropertyContainerControlBase *createExtensible(int id, bool new_line = true);
  virtual PropertyContainerControlBase *createExtGroup(int id, const char caption[]);
  virtual PropertyContainerControlBase *createGroup(int id, const char caption[]);
  virtual PropertyContainerControlBase *createGroupHorzFlow(int id, const char caption[]);
  virtual PropertyContainerControlBase *createGroupBox(int id, const char caption[]);
  virtual PropertyContainerControlBase *createRadioGroup(int id, const char caption[], bool new_line = true);
  virtual PropertyContainerControlBase *createTabPanel(int id, const char caption[]);
  virtual PropertyContainerControlBase *createTabPage(int id, const char caption[]);
  virtual PropertyContainerControlBase *createToolbarPanel(int id, const char caption[], bool new_line = true);
  virtual PropertyContainerControlBase *createTree(int id, const char caption[], int height, bool new_line = true);

  virtual void createStatic(int id, const char caption[], bool new_line = true);
  virtual void createEditBox(int id, const char caption[], const char text[] = "", bool enabled = true, bool new_line = true,
    bool multiline = false, int multi_line_height = 40);
  virtual void createFileEditBox(int id, const char caption[], const char file[] = "", bool enabled = true, bool new_line = true);
  virtual void createFileButton(int id, const char caption[], const char file[] = "", bool enabled = true, bool new_line = true);
  virtual void createTargetButton(int id, const char caption[], const char text[] = "", bool enabled = true, bool new_line = true);
  virtual void createEditInt(int id, const char caption[], int value = 0, bool enabled = true, bool new_line = true);
  virtual void createEditFloat(int id, const char caption[], float value = 0, int prec = 2, bool enabled = true, bool new_line = true);
  virtual void createTrackInt(int id, const char caption[], int value, int min, int max, int step, bool enabled = true,
    bool new_line = true);
  virtual void createTrackFloat(int id, const char caption[], float value, float min, float max, float step, bool enabled = true,
    bool new_line = true);
  virtual void createTrackFloatLogarithmic(int id, const char caption[], float value, float min, float max, float step, float power,
    bool enabled = true, bool new_line = true);
  virtual void createCheckBox(int id, const char caption[], bool value = false, bool enabled = true, bool new_line = true);
  virtual void createButton(int id, const char caption[], bool enabled = true, bool new_line = true);
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
  virtual void createMultiSelectList(int id, const Tab<String> &vals, int height, bool enabled = true, bool new_line = true);
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
  virtual void createGradientPlot(int id, const char caption[], int height = 0, bool enabled = true, bool new_line = true);
  virtual void createCurveEdit(int id, const char caption[], int height = 0, bool enabled = true, bool new_line = true);
  virtual void createTwoColorIndicator(int id, const char caption[], int height = 0, bool enabled = true, bool new_line = true);
  virtual void createPaletteCell(int id, const char caption[], bool enabled = true, bool new_line = true);
  virtual PropertyControlBase *createPlaceholder(int id, int height, bool new_line = true);
  virtual TLeafHandle createTreeLeaf(TLeafHandle parent, const char caption[], const char image[]);

  virtual void createIndirect(DataBlock &dataBlock, ISetControlParams &controlParams);

  virtual PropPanelScheme *createSceme();

  // Sets
  virtual void setEnabledById(int id, bool enabled);
  virtual void resetById(int id);
  virtual void setWidthById(int id, unsigned w);
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

  virtual SimpleString getCaption() const;

  virtual void beginUpdate() { mUpdate = true; };
  virtual void endUpdate() { mUpdate = false; };

  virtual void clear();
  virtual unsigned getChildCount();

  virtual unsigned getClientWidth();
  virtual unsigned getClientHeight();

  virtual WindowBase *getWindow(); // for CPanelWindow it returns static with controls, for operating window use handles below
  virtual void *getWindowHandle();
  virtual void *getParentWindowHandle();

  virtual void onChildResize(int id) { G_UNUSED(id); };

  virtual const PropertyContainerControlBase *getContainer() const { return this; };
  virtual PropertyContainerControlBase *getContainer() { return this; };
  virtual bool isRealContainer() { return true; }

  virtual PropertyControlBase *getById(int id, bool stopOnDifferentEventHandler = true) const; // for set, get, show, hide, disable,
                                                                                               // enable, resize
  virtual PropertyContainerControlBase *getContainerById(int id) const; // for set, get, show, hide, disable, enable, resize
  virtual PropertyControlBase *getByIndex(int index) const;             // only for radio groups

  virtual void setPostEvent(int pcb_id);
  virtual void setVisible(bool show = true);

  // saving and loading state with datablock

  virtual int saveState(DataBlock &datablk);
  virtual int loadState(DataBlock &datablk);

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
  virtual void setSelLeaf(TLeafHandle leaf) { G_UNUSED(leaf); }
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

  virtual void enableResizeCallback() {}
  virtual void disableResizeCallback() {}

protected:
  virtual void resizeControl(unsigned w, unsigned h)
  {
    G_UNUSED(w);
    G_UNUSED(h);
  };
  virtual void addControl(PropertyControlBase *pcontrol, bool new_line = true);
  virtual void onControlAdd(PropertyControlBase *control) { G_UNUSED(control); }; // for container resize or add scroll

  virtual int getNextControlX(bool new_line = true);
  virtual int getNextControlY(bool new_line = true);

  Tab<PropertyControlBase *> mControlArray;
  Tab<bool> mControlsNewLine;

private:
  bool mUpdate;
};

// iterate all tree leaves recursively (using children), root is usually nullptr and not iterated
template <typename C>
inline bool iterate_children_leafs(PropertyContainerControlBase &control, C &cb, TLeafHandle root = nullptr)
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
inline bool iterate_all_leafs_linear(PropertyContainerControlBase &control, TLeafHandle start_leaf, const C &cb)
{
  for (TLeafHandle leaf = start_leaf; leaf;)
    if (!cb(leaf))
      return false;
    else if ((leaf = control.getNextLeaf(leaf)) == start_leaf)
      break;
  return true;
}
