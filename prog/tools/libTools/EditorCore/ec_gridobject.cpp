// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_gridobject.h>
#include <EditorCore/ec_interface.h>

#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_quadIndexBuffer.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_debug.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_statDrv.h>
#include <render/viewVecs.h>
#include <shaders/dag_shaders.h>

#include <propPanel/control/container.h>

#define MAJOR_LINE_COLOR E3DCOLOR(0, 0, 0, 255)

static int infinite_grid_major_line_colorVarId = -1;
static int infinite_grid_minor_line_colorVarId = -1;
static int infinite_grid_x_axis_colorVarId = -1;
static int infinite_grid_z_axis_colorVarId = -1;
static int infinite_grid_major_line_widthVarId = -1;
static int infinite_grid_minor_line_widthVarId = -1;
static int infinite_grid_major_subdivisionsVarId = -1;
static int infinite_grid_view_projVarId = -1;
static int infinite_grid_y_positionVarId = -1;

GridObject::GridObject() : infiniteGridInitialized(false) { resetToDefault(); }

GridObject::~GridObject()
{
  if (infiniteGridInitialized)
    index_buffer::release_quads_16bit();
}

void GridObject::resetToDefault()
{
  step = 0.05;
  angleStep = 2.5;
  scaleStep = 10.0;
  gridHeight = 0;

  isMoveSnap = false;
  isRotateSnap = false;
  isScaleSnap = false;
  isDrawMajorLines = true;
  isDrawAxisLines = true;

  isUseInfiniteGrid = true;
  infiniteGridMajorLineColor = E3DCOLOR(0, 0, 0);
  infiniteGridMinorLineColor = E3DCOLOR(150, 150, 150);
  infiniteGridXAxisColor = e3dcolor_lerp(E3DCOLOR(228, 55, 80, 255), infiniteGridMinorLineColor, 0.5f);
  infiniteGridZAxisColor = e3dcolor_lerp(E3DCOLOR(46, 131, 228, 255), infiniteGridMinorLineColor, 0.5f);
  infiniteGridMajorLineWidth = 0.05f;
  infiniteGridMinorLineWidth = 0.01f;
  infiniteGridMajorSubdivisions = 5;
}

void GridObject::renderInfiniteGrid()
{
  if (!infiniteGridInitialized)
  {
    infiniteGridInitialized = true;

    infinite_grid_major_line_colorVarId = get_shader_variable_id("infinite_grid_major_line_color", true);
    infinite_grid_minor_line_colorVarId = get_shader_variable_id("infinite_grid_minor_line_color", true);
    infinite_grid_x_axis_colorVarId = get_shader_variable_id("infinite_grid_x_axis_color", true);
    infinite_grid_z_axis_colorVarId = get_shader_variable_id("infinite_grid_z_axis_color", true);
    infinite_grid_major_line_widthVarId = get_shader_variable_id("infinite_grid_major_line_width", true);
    infinite_grid_minor_line_widthVarId = get_shader_variable_id("infinite_grid_minor_line_width", true);
    infinite_grid_major_subdivisionsVarId = get_shader_variable_id("infinite_grid_major_subdivisions", true);
    infinite_grid_view_projVarId = get_shader_variable_id("infinite_grid_view_proj", true);
    infinite_grid_y_positionVarId = get_shader_variable_id("infinite_grid_y_position", true);

    index_buffer::init_quads_16bit();

    const char *shaderName = "infinite_grid";
    infiniteGridShaderMaterial = new_shader_material_by_name_optional(shaderName, nullptr);
    if (infiniteGridShaderMaterial.get())
      infiniteGridShaderElement = infiniteGridShaderMaterial->make_elem();

    if (!infiniteGridShaderElement)
      logwarn("Shader \"%s\" not found. The infinite grid will not render.", shaderName);
  }

  if (!infiniteGridShaderElement)
    return;

  TIME_D3D_PROFILE(infiniteGrid);

  TMatrix viewTm;
  TMatrix4 projTm;
  d3d::gettm(TM_VIEW, viewTm);
  d3d::gettm(TM_PROJ, &projTm);

  const TMatrix4 viewProj = TMatrix4(viewTm) * projTm;
  const bool drawMajorLines = isDrawMajorLines && infiniteGridMajorSubdivisions > 1;

  ShaderGlobal::set_color4(infinite_grid_major_line_colorVarId, drawMajorLines ? infiniteGridMajorLineColor : E3DCOLOR(0, 0, 0, 0));
  ShaderGlobal::set_color4(infinite_grid_minor_line_colorVarId, infiniteGridMinorLineColor);
  ShaderGlobal::set_color4(infinite_grid_x_axis_colorVarId, isDrawAxisLines ? infiniteGridXAxisColor : E3DCOLOR(0, 0, 0, 0));
  ShaderGlobal::set_color4(infinite_grid_z_axis_colorVarId, isDrawAxisLines ? infiniteGridZAxisColor : E3DCOLOR(0, 0, 0, 0));
  ShaderGlobal::set_real(infinite_grid_major_line_widthVarId, infiniteGridMajorLineWidth);
  ShaderGlobal::set_real(infinite_grid_minor_line_widthVarId, infiniteGridMinorLineWidth);
  ShaderGlobal::set_real(infinite_grid_major_subdivisionsVarId, drawMajorLines ? infiniteGridMajorSubdivisions : 1);
  ShaderGlobal::set_float4x4(infinite_grid_view_projVarId, viewProj);
  ShaderGlobal::set_real(infinite_grid_y_positionVarId, gridHeight);
  set_viewvecs_to_shader(viewTm, projTm);

  if (!infiniteGridShaderElement->setStates(0, true))
    return;

  index_buffer::use_quads_16bit();
  d3d::setvsrc(0, 0, 0);
  d3d::drawind_instanced(PRIM_TRILIST, 0, 2, 0, 1);
}

void GridObject::render(Point3 *pt, Point3 *dirs, real zoom, int index, bool test_z, bool write_z)
{
  struct LineDesc
  {
    float minZoom;
    float step;
    E3DCOLOR col;
  };
  static LineDesc lnd[] = {
    {200, 0.1, E3DCOLOR(150, 150, 150, 255)},
    {50, 1, E3DCOLOR(80, 80, 80, 255)},
    {10, 10, E3DCOLOR(120, 130, 130, 255)},
    {1, 100, E3DCOLOR(60, 60, 60, 255)},
    {0, 1000, E3DCOLOR(90, 90, 90, 255)},
  };

  if (!visible[index])
    return;

  if (isUseInfiniteGrid)
  {
    renderInfiniteGrid();
    return;
  }

  float move_snap = step;
  E3DCOLOR lc = MAJOR_LINE_COLOR, lc2 = MAJOR_LINE_COLOR;
  for (int i = 0; i < sizeof(lnd) / sizeof(lnd[0]); i++)
    if (zoom > lnd[i].minZoom)
    {
      step = lnd[i].step;
      lc = lnd[i].col;
      if (i + 1 < sizeof(lnd) / sizeof(lnd[0]))
        lc2 = lnd[i + 1].col;
      else
        lc2 = lc;
      break;
    }

  step *= 10;

  pt[0] = snapToGrid(pt[0]);
  pt[1] = snapToGrid(pt[1]);
  pt[2] = snapToGrid(pt[2]);
  pt[3] = snapToGrid(pt[3]);

  Point3 ptc = (pt[2] + pt[0]) * 0.5f;
  Point3 size = pt[2] - pt[0];
  float hsm = 4 * step;

  if (size.x > 2 * hsm)
  {
    pt[0].x = ptc.x - hsm;
    pt[1].x = ptc.x - hsm;
    pt[2].x = ptc.x + hsm;
    pt[3].x = ptc.x + hsm;
    size.x = 2 * hsm;
  }
  if (size.z > 2 * hsm)
  {
    pt[0].z = ptc.z - hsm;
    pt[1].z = ptc.z + hsm;
    pt[2].z = ptc.z + hsm;
    pt[3].z = ptc.z - hsm;
    size.z = 2 * hsm;
  }

  step /= 10;
  int counter = 0;

  ::begin_draw_cached_debug_lines(test_z, write_z);
  ::set_cached_debug_lines_wtm(TMatrix::IDENT);
  for (int i = 0; step * i <= max(size.x, size.z); ++i)
  {
    draw_cached_debug_line(pt[0] + Point3(0, gridHeight, step * i), pt[3] + Point3(0, gridHeight, step * i), counter ? lc : lc2);
    draw_cached_debug_line(pt[0] + Point3(step * i, gridHeight, 0), pt[1] + Point3(step * i, gridHeight, 0), counter ? lc : lc2);
    if (++counter > 9)
      counter = 0;
  }

  if (isDrawMajorLines)
  {
    draw_cached_debug_line(Point3(min(pt[0].x, 0.0f), gridHeight, 0), Point3(max(pt[3].x, 0.0f), gridHeight, 0), MAJOR_LINE_COLOR);
    draw_cached_debug_line(Point3(0, gridHeight, min(pt[0].z, 0.0f)), Point3(0, gridHeight, max(pt[1].z, 0.0f)), MAJOR_LINE_COLOR);
  }
  ::end_draw_cached_debug_lines();
  step = move_snap;
}


void GridObject::save(DataBlock &blk)
{
  DataBlock &gridBlk = *blk.addBlock("grid");
  gridBlk.setBool("grid_visible0", visible[0]);
  gridBlk.setBool("grid_visible1", visible[1]);
  gridBlk.setBool("grid_visible2", visible[2]);
  gridBlk.setBool("grid_visible3", visible[3]);

  gridBlk.setReal("step", step);
  gridBlk.setReal("angle_step", angleStep);
  gridBlk.setReal("scale_step", scaleStep);
  gridBlk.setReal("grid_height", gridHeight);

  gridBlk.setBool("move_snap_on", isMoveSnap);
  gridBlk.setBool("rotate_snap_on", isRotateSnap);
  gridBlk.setBool("scale_snap_on", isScaleSnap);
  gridBlk.setBool("draw_major_lines", isDrawMajorLines);
  gridBlk.setBool("draw_axis_lines", isDrawAxisLines);

  DataBlock *infiniteGridBlk = gridBlk.addBlock("infinite_grid");
  infiniteGridBlk->clearData();
  infiniteGridBlk->setBool("use_infinite_grid", isUseInfiniteGrid);
  infiniteGridBlk->setE3dcolor("major_line_color", infiniteGridMajorLineColor);
  infiniteGridBlk->setE3dcolor("minor_line_color", infiniteGridMinorLineColor);
  infiniteGridBlk->setReal("major_line_width", infiniteGridMajorLineWidth);
  infiniteGridBlk->setReal("minor_line_width", infiniteGridMinorLineWidth);
  infiniteGridBlk->setInt("major_subdivisions", infiniteGridMajorSubdivisions);
}


void GridObject::load(const DataBlock &blk)
{
  const DataBlock &gridBlk = *blk.getBlockByNameEx("grid");
  if (&gridBlk == &DataBlock::emptyBlock)
    return;
  visible[0] = gridBlk.getBool("grid_visible0", true);
  visible[1] = gridBlk.getBool("grid_visible1", true);
  visible[2] = gridBlk.getBool("grid_visible2", true);
  visible[3] = gridBlk.getBool("grid_visible3", true);

  step = gridBlk.getReal("step", 0.05);
  angleStep = gridBlk.getReal("angle_step", 2.5);
  scaleStep = gridBlk.getReal("scale_step", 5);
  gridHeight = gridBlk.getReal("grid_height", 0);

  isMoveSnap = gridBlk.getBool("move_snap_on", true);
  isRotateSnap = gridBlk.getBool("rotate_snap_on", true);
  isScaleSnap = gridBlk.getBool("scale_snap_on", true);
  isDrawMajorLines = gridBlk.getBool("draw_major_lines", true);
  isDrawAxisLines = gridBlk.getBool("draw_axis_lines", true);

  const DataBlock *infiniteGridBlk = gridBlk.getBlockByName("infinite_grid");
  if (infiniteGridBlk)
  {
    isUseInfiniteGrid = infiniteGridBlk->getBool("use_infinite_grid", isUseInfiniteGrid);
    infiniteGridMajorLineColor = infiniteGridBlk->getE3dcolor("major_line_color", infiniteGridMajorLineColor);
    infiniteGridMinorLineColor = infiniteGridBlk->getE3dcolor("minor_line_color", infiniteGridMinorLineColor);
    infiniteGridMajorLineWidth = infiniteGridBlk->getReal("major_line_width", infiniteGridMajorLineWidth);
    infiniteGridMinorLineWidth = infiniteGridBlk->getReal("minor_line_width", infiniteGridMinorLineWidth);
    infiniteGridMajorSubdivisions = infiniteGridBlk->getInt("major_subdivisions", infiniteGridMajorSubdivisions);
  }
}


real GridObject::snap(real f, real st) const
{
  real x1 = (int)(f / st) * st;
  real x2 = (f > 0) ? (x1 + st) : (x1 - st);

  return fabs(f - x1) < fabs(f - x2) ? x1 : x2;
}


Point3 GridObject::snapToGrid(const Point3 &p) const { return Point3(snap(p.x, step), snap(p.y, step), snap(p.z, step)); }


Point3 GridObject::snapToAngle(const Point3 &p) const
{
  return Point3(p.x ? snap(p.x, DegToRad(angleStep)) : 0, p.y ? snap(p.y, DegToRad(angleStep)) : 0,
    p.z ? snap(p.z, DegToRad(angleStep)) : 0);
}


Point3 GridObject::snapToScale(const Point3 &p) const
{
  return Point3(snap(p.x, scaleStep / 100), snap(p.y, scaleStep / 100), snap(p.z, scaleStep / 100));
}


enum
{
  ID_SHOW_VIEWPORT_CHECKBOX = 500,
  ID_MOVE_SNAP_EDIT,
  ID_ROTATE_SNAP_EDIT,
  ID_SCALE_SNAP_EDIT,
  ID_MOVE_SNAP_CHK,
  ID_ROTATE_SNAP_CHK,
  ID_SCALE_SNAP_CHK,

  ID_DRAW_MAJOR_LINES,
  ID_DRAW_AXIS_LINES,
  ID_GRID_HEIGHT,

  ID_USE_INFINITE_GRID,
  ID_INFINITE_GRID_MAJOR_LINE_COLOR,
  ID_INFINITE_GRID_MINOR_LINE_COLOR,
  ID_INFINITE_GRID_MAJOR_LINE_WIDTH,
  ID_INFINITE_GRID_MINOR_LINE_WIDTH,
  ID_INFINITE_GRID_MAJOR_SUBDIVISIONS,
};


GridEditDialog::GridEditDialog(void *phandle, GridObject &grid, const char *caption) :
  DialogWindow(phandle, hdpi::_pxScaled(300), hdpi::_pxScaled(300), caption), mGrid(grid)
{
  G_ASSERT(&mGrid);
  fillPanel();

  PropPanel::ContainerPropertyControl *buttonsContainer = buttonsPanel->getContainer();
  buttonsContainer->removeById(PropPanel::DIALOG_ID_CANCEL);
}


void GridEditDialog::showGridEditDialog(int viewport_index)
{
  G_ASSERT(viewport_index >= 0);
  index = viewport_index;

  updateShowGridDialogControl();
  autoSize(!hasEverBeenShown());
  show();
}


void GridEditDialog::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  G_ASSERT(index >= 0);

  mGrid.setVisible(panel->getBool(ID_SHOW_VIEWPORT_CHECKBOX), index);
  mGrid.setStep(panel->getFloat(ID_MOVE_SNAP_EDIT));
  mGrid.setAngleStep(panel->getFloat(ID_ROTATE_SNAP_EDIT));
  mGrid.setScaleStep(panel->getFloat(ID_SCALE_SNAP_EDIT));
  mGrid.setDrawMajorLines(panel->getBool(ID_DRAW_MAJOR_LINES));
  mGrid.setDrawAxisLines(panel->getBool(ID_DRAW_AXIS_LINES));
  mGrid.setGridHeight(panel->getFloat(ID_GRID_HEIGHT));
  mGrid.setMoveSnap(panel->getBool(ID_MOVE_SNAP_CHK));
  mGrid.setRotateSnap(panel->getBool(ID_ROTATE_SNAP_CHK));
  mGrid.setScaleSnap(panel->getBool(ID_SCALE_SNAP_CHK));

  mGrid.setUseInfiniteGrid(panel->getBool(ID_USE_INFINITE_GRID));
  mGrid.setInfiniteGridMajorLineColor(panel->getColor(ID_INFINITE_GRID_MAJOR_LINE_COLOR));
  mGrid.setInfiniteGridMinorLineColor(panel->getColor(ID_INFINITE_GRID_MINOR_LINE_COLOR));
  mGrid.setInfiniteGridMajorLineWidth(panel->getFloat(ID_INFINITE_GRID_MAJOR_LINE_WIDTH));
  mGrid.setInfiniteGridMinorLineWidth(panel->getFloat(ID_INFINITE_GRID_MINOR_LINE_WIDTH));
  mGrid.setInfiniteGridMajorSubdivisions(panel->getInt(ID_INFINITE_GRID_MAJOR_SUBDIVISIONS));
}


void GridEditDialog::fillPanel()
{
  PropPanel::ContainerPropertyControl *panel = getPanel();

  panel->createCheckBox(ID_SHOW_VIEWPORT_CHECKBOX, "Show grid in viewport");

  panel->createCheckBox(ID_MOVE_SNAP_CHK, "Move snap, m", mGrid.getMoveSnap());
  panel->createEditFloat(ID_MOVE_SNAP_EDIT, "", mGrid.getStep(), 3, true, false);

  panel->createCheckBox(ID_ROTATE_SNAP_CHK, "Rotate snap, deg", mGrid.getRotateSnap());
  panel->createEditFloat(ID_ROTATE_SNAP_EDIT, "", mGrid.getAngleStep(), 3, true, false);

  panel->createCheckBox(ID_SCALE_SNAP_CHK, "Scale snap, %", mGrid.getScaleSnap());
  panel->createEditFloat(ID_SCALE_SNAP_EDIT, "", mGrid.getScaleStep(), 3, true, false);

  panel->createCheckBox(ID_DRAW_MAJOR_LINES, "Draw major lines", mGrid.getDrawMajorLines());
  panel->createCheckBox(ID_DRAW_AXIS_LINES, "Draw axis lines", mGrid.getDrawAxisLines());
  panel->createEditFloat(ID_GRID_HEIGHT, "Grid height", mGrid.getGridHeight());

  panel->createSeparator();
  panel->createCheckBox(ID_USE_INFINITE_GRID, "Use infinite grid", mGrid.getUseInfiniteGrid());
  panel->createSimpleColor(ID_INFINITE_GRID_MAJOR_LINE_COLOR, "Major line color", mGrid.getInfiniteGridMajorLineColor());
  panel->createSimpleColor(ID_INFINITE_GRID_MINOR_LINE_COLOR, "Minor line color", mGrid.getInfiniteGridMinorLineColor());
  panel->createEditFloat(ID_INFINITE_GRID_MAJOR_LINE_WIDTH, "Major line width", mGrid.getInfiniteGridMajorLineWidth());
  panel->createEditFloat(ID_INFINITE_GRID_MINOR_LINE_WIDTH, "Minor line width", mGrid.getInfiniteGridMinorLineWidth());
  panel->createEditInt(ID_INFINITE_GRID_MAJOR_SUBDIVISIONS, "Major subdivisions", mGrid.getInfiniteGridMajorSubdivisions());
}


void GridEditDialog::updateShowGridDialogControl()
{
  G_ASSERT(index >= 0);

  PropPanel::ContainerPropertyControl *panel = getPanel();
  G_ASSERT(panel->getById(ID_SHOW_VIEWPORT_CHECKBOX));
  panel->setCaption(ID_SHOW_VIEWPORT_CHECKBOX, String(64, "Show grid in viewport #%d", index));
  panel->setBool(ID_SHOW_VIEWPORT_CHECKBOX, mGrid.isVisible(index));
}


void GridEditDialog::onGridVisibilityChanged(int viewport_index)
{
  if (viewport_index != index)
    return;

  updateShowGridDialogControl();
}
