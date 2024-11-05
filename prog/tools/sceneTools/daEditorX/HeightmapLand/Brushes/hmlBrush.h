// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "brushMask.h"

#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <EditorCore/ec_brush.h>


class DataBlock;


class IHmapBrushImage
{
public:
  enum Channel
  {
    CHANNEL_RGB = 0,
    CHANNEL_R,
    CHANNEL_G,
    CHANNEL_B,
    CHANNEL_A,

    CHANNELS_COUNT,
  };

  virtual real getBrushImageData(int x, int y, Channel channel) = 0;
  virtual void setBrushImageData(int x, int y, real v, Channel channel) = 0;
};


class HmapLandBrush : public Brush
{
public:
  HmapLandBrush(IBrushClient *client, IHmapBrushImage &height_map);
  virtual ~HmapLandBrush() {}

  virtual void saveToBlk(DataBlock &blk) const;
  virtual void loadFromBlk(const DataBlock &blk);

  void brushStartEnd();

  virtual void onBrushPaintStart(int buttons, int key_modif)
  {
    dirtyBrushBox.setEmpty();
    brushStartEnd();
  }
  virtual void onBrushPaintEnd(int buttons, int key_modif) { brushStartEnd(); }
  virtual void onBrushPaint(const Point3 &center, const Point3 &prev_center, const Point3 &normal, int buttons, int key_modif,
    real cell_size)
  {
    paint(center, prev_center, cell_size, false);
  }
  virtual void brushPaintApplyStart(const IBBox2 &where);
  virtual bool brushPaintApply(int x, int y, float inc, bool rb) = 0; // true if applied
  virtual void brushPaintApplyEnd() {}
  virtual void onRBBrushPaintStart(int buttons, int key_modif)
  {
    dirtyBrushBox.setEmpty();
    brushStartEnd();
  }
  virtual void onRBBrushPaintEnd(int buttons, int key_modif) { brushStartEnd(); }
  virtual void onRBBrushPaint(const Point3 &center, const Point3 &prev_center, const Point3 &normal, int buttons, int key_modif,
    real cell_size)
  {
    paint(center, prev_center, cell_size, true);
  }

  virtual void paint(const Point3 &center, const Point3 &prev_center, real cell_size, bool rb);


  virtual void fillParams(PropPanel::ContainerPropertyControl &panel);
  virtual void updateToPanel(PropPanel::ContainerPropertyControl &panel);

  virtual bool updateFromPanelRef(PropPanel::ContainerPropertyControl &panel, int pid);
  virtual void dynamicItemChange(PropPanel::ContainerPropertyControl &panel);

  virtual void draw();

  virtual IBBox2 getDirtyBox() const { return dirtyBrushBox; }
  void resetDirtyBox() { dirtyBrushBox.setEmpty(); }
  bool updateMask(const char *);
  void setHeightmapOffset(const Point2 &p) { heightMapOffset = p; }

protected:
  Point2 heightMapOffset;
  IBrushMask mask;
  IHmapBrushImage &heightMap;
  String maskName;
  int maskInd;
  IBBox2 dirtyBrushBox;
  struct Applied
  {
    SmallTab<float, TmpmemAlloc> weight;
    IBBox2 at;
    float *operator[](IPoint2 p)
    {
      if (!(at & p))
        return NULL;
      p.x -= at[0].x;
      p.y -= at[0].y;
      return &weight[p.x + p.y * (at[1].x - at[0].x + 1)];
    }
    const float *operator[](IPoint2 p) const { return (*const_cast<Applied *>(this))[p]; }
    float getWeight(int x, int y) const
    {
      const float *p = (*this)[IPoint2(x, y)];
      if (!p)
        return 1.0;
      return *p;
    }
    void init(const IBBox2 &a)
    {
      at = a;
      IPoint2 width = at[1] - at[0] + IPoint2(1, 1);
      clear_and_resize(weight, width.x * width.y);
      mem_set_0(weight);
    }
  };
  Tab<Applied> applied;
  bool applyOnlyOnce, useAutoAngle;
  real currentAngle;
  Point2 currentVector;
  float gridCellSize;


  virtual bool calcCenter(IGenViewportWnd *wnd);
  virtual bool traceDown(const Point3 &pos, Point3 &clip_pos, IGenViewportWnd *wnd);
};


namespace heightmap_land
{
extern HmapLandBrush *getUpHillBrush(IBrushClient *, IHmapBrushImage &);
extern HmapLandBrush *getDownHillBrush(IBrushClient *, IHmapBrushImage &);
extern HmapLandBrush *getSmoothBrush(IBrushClient *, IHmapBrushImage &);
extern HmapLandBrush *getAlignBrush(IBrushClient *, IHmapBrushImage &);
extern HmapLandBrush *getShadowsBrush(IBrushClient *, IHmapBrushImage &);
extern HmapLandBrush *getScriptBrush(IBrushClient *, IHmapBrushImage &);
} // namespace heightmap_land
