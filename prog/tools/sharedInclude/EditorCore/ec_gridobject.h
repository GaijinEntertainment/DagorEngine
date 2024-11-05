// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_DObject.h>
#include <generic/dag_tab.h>
#include <math/dag_e3dColor.h>
#include <util/dag_globDef.h>
#include <propPanel/commonWindow/dialogWindow.h>

class DataBlock;
class BBox3;
class Point3;
class ShaderElement;
class ShaderMaterial;

/// Grid object.
/// Used for representing Editor grid.
/// @ingroup EditorCore
/// @ingroup Grid
class GridObject
{
public:
  /// Constructor.
  GridObject();
  ~GridObject();

  // ****************************************************************
  /// @name Get grid properties.
  //@{
  /// Test whether the grid is visible in specified viewport.
  /// @param[in] idx - viewport index
  /// @return @b true if the grid is visible, @b false in other case
  inline bool isVisible(int idx) const { return (idx >= 0 && idx < VIS_CNT) ? visible[idx] : false; }

  /// Get object move step.
  /// @return objects move step (in metres)
  inline real getStep() const { return step; }
  /// Get object rotation step.
  /// @return objects rotation step (in degrees)
  inline real getAngleStep() const { return angleStep; }
  /// Get object scaling step.
  /// @return objects scaling step (in percent from original size)
  inline real getScaleStep() const { return scaleStep; }
  //@}

  // ****************************************************************
  /// @name Set grid properties.
  //@{
  /// Set grid visibility in specified viewport.
  /// @param[in] v - <b>true / false</b>: set visibility <b>on / off</b>
  /// @param[in] index - viewport index
  inline void setVisible(bool v, int index) { visible[index] = v; }
  /// Set object move step.
  /// @param[in] st - object move step (in meters)
  inline void setStep(real st) { step = st; }
  /// Set object rotation step.
  /// @param[in] st - object rotation step (in degrees)
  inline void setAngleStep(real st) { angleStep = st; }
  /// Set object scaling step.
  /// @param[in] st - objects scaling step (in percent from original size)
  inline void setScaleStep(real st) { scaleStep = st; }
  //@}

  /// Save grid properties to blk file.
  /// @param[in] blk - Data Block that contains data to save (see #DataBlock)
  void save(DataBlock &blk);
  /// Load grid properties from blk file.
  /// @param[in] blk - Data Block that contains data to load (see #DataBlock)
  void load(const DataBlock &blk);

  /// Render the grid in specified viewport.
  /// @param[in] pt - pointer to array of four Point3, specifying vertex
  ///                 coordinates of grid's rectangle
  /// @param[in] dirs - pointer to array of two Point3, specifyind grid`s
  ///                 lines directions
  /// @param[in] zoom - viewport zoom
  /// @param[in] index - viewport index
  void render(Point3 *pt, Point3 *dirs, real zoom, int index, bool test_z = true, bool write_z = true);

  /// Round value to the nearest (snapped) value.
  /// @param[in] f - the value to snap
  /// @param[in] st - snap step
  /// @return the snapped value
  real snap(real f, real st) const;

  /// Snap to grid - round coordinates to the nearest grid's point.
  /// @param[in] p - point coordinates to snap
  /// @return coordinates of snapped point
  Point3 snapToGrid(const Point3 &p) const;
  /// Snap to angle - round rotation angle to the nearest (snapped) angle
  /// @param[in] p - angle to snap
  /// @return snapped angle
  Point3 snapToAngle(const Point3 &p) const;
  /// Snap to scale - round scale value to the nearest (snapped) value
  /// @param[in] p - scale value to snap
  /// @return snapped scale value
  Point3 snapToScale(const Point3 &p) const;


  //*******************************************************
  ///@name Test 'snap toggle' buttons
  //@{
  /// Test whether 'Move snap toggle' button pressed.
  /// @return @b true if 'Move snap toggle' button is in pressed state,
  ///         @b false in other case
  bool getMoveSnap() const { return isMoveSnap; }
  /// Set 'Move' snap flag.
  /// @param[in] is_snap - <b>true / false: set / clear </b>  snap property
  void setMoveSnap(bool is_snap) { isMoveSnap = is_snap; }

  /// Test whether 'Rotate snap toggle' button pressed.
  /// @return @b true if 'Rotate snap toggle' button is in pressed state,
  ///         @b false in other case
  bool getRotateSnap() const { return isRotateSnap; }
  /// Set 'Rotate' snap flag.
  /// @param[in] is_snap - <b>true / false: set / clear </b>  snap property
  void setRotateSnap(bool is_snap) { isRotateSnap = is_snap; }

  /// Test whether 'Scale snap toggle' button pressed.
  /// @return @b true if 'Scale snap toggle' button is in pressed state,
  ///         @b false in other case
  bool getScaleSnap() const { return isScaleSnap; }
  /// Set 'Scale' snap flag.
  /// @param[in] is_snap - <b>true / false: set / clear </b>  snap property
  void setScaleSnap(bool is_snap) { isScaleSnap = is_snap; }
  //@}

  bool getDrawMajorLines() { return isDrawMajorLines; }
  void setDrawMajorLines(bool draw) { isDrawMajorLines = draw; }

  real getGridHeight() { return gridHeight; }
  void setGridHeight(real new_height) { gridHeight = new_height; }

  bool getUseInfiniteGrid() const { return isUseInfiniteGrid; }
  void setUseInfiniteGrid(bool infinite) { isUseInfiniteGrid = infinite; }

  E3DCOLOR getInfiniteGridMajorLineColor() const { return infiniteGridMajorLineColor; }
  void setInfiniteGridMajorLineColor(E3DCOLOR color) { infiniteGridMajorLineColor = color; }

  E3DCOLOR getInfiniteGridMinorLineColor() const { return infiniteGridMinorLineColor; }
  void setInfiniteGridMinorLineColor(E3DCOLOR color) { infiniteGridMinorLineColor = color; }

  real getInfiniteGridMajorLineWidth() const { return infiniteGridMajorLineWidth; }
  void setInfiniteGridMajorLineWidth(real width) { infiniteGridMajorLineWidth = width; }

  real getInfiniteGridMinorLineWidth() const { return infiniteGridMinorLineWidth; }
  void setInfiniteGridMinorLineWidth(real width) { infiniteGridMinorLineWidth = width; }

  int getInfiniteGridMajorSubdivisions() const { return infiniteGridMajorSubdivisions; }
  void setInfiniteGridMajorSubdivisions(int subdivisions) { infiniteGridMajorSubdivisions = subdivisions; }

  void resetToDefault();


protected:
  static const int VIS_CNT = 4;

  bool visible[VIS_CNT];
  real step;
  real angleStep;
  real scaleStep;
  real gridHeight;
  bool isMoveSnap;
  bool isRotateSnap;
  bool isScaleSnap;
  bool isDrawMajorLines;

  bool infiniteGridInitialized;
  Ptr<ShaderMaterial> infiniteGridShaderMaterial;
  Ptr<ShaderElement> infiniteGridShaderElement;
  bool isUseInfiniteGrid;
  E3DCOLOR infiniteGridMajorLineColor;
  E3DCOLOR infiniteGridMinorLineColor;
  real infiniteGridMajorLineWidth;
  real infiniteGridMinorLineWidth;
  int infiniteGridMajorSubdivisions;

  void renderInfiniteGrid();
};


class GridEditDialog : public PropPanel::DialogWindow
{
public:
  GridEditDialog(void *phandle, GridObject &grid, const char *caption);

  void showGridEditDialog(int viewport_index);
  void onGridVisibilityChanged(int viewport_index);

private:
  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  void fillPanel();
  void updateShowGridDialogControl();

  GridObject &mGrid;
  int index = -1;
  bool autoCenter = true;
};
