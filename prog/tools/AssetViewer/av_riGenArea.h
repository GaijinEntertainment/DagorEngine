// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <de3_rendInstGen.h>
#include <EditorCore/ec_interface.h>

// The RIGen area the previews generate into, centered at the origin: 32 m grid, cells of 8 grid steps
// (256 m, which sets the instance quantization), 32x32 cells, so -4096..+4096 m (to preview trees on landClasses)
static constexpr float AV_RIGEN_GRID2WORLD = 32.f;
static constexpr int AV_RIGEN_CELL_SZ = 8;
static constexpr int AV_RIGEN_CELL_NUM = 32;
static constexpr int AV_RIGEN_GRID_SIZE = AV_RIGEN_CELL_SZ * AV_RIGEN_CELL_NUM;
static constexpr float AV_RIGEN_ORIGIN = -0.5f * AV_RIGEN_GRID_SIZE * AV_RIGEN_GRID2WORLD;

inline void av_create_rt_rigen_data()
{
  if (IRendInstGenService *rigenSrv = EDITORCORE->queryEditorInterface<IRendInstGenService>())
    rigenSrv->createRtRIGenData(AV_RIGEN_ORIGIN, AV_RIGEN_ORIGIN, AV_RIGEN_GRID2WORLD, //
      AV_RIGEN_CELL_SZ, AV_RIGEN_CELL_NUM, AV_RIGEN_CELL_NUM, {}, AV_RIGEN_ORIGIN, AV_RIGEN_ORIGIN);
}

inline void av_discard_rigen_area()
{
  if (IRendInstGenService *rigenSrv = EDITORCORE->queryEditorInterface<IRendInstGenService>())
    rigenSrv->discardRIGenRect(0, 0, AV_RIGEN_GRID_SIZE, AV_RIGEN_GRID_SIZE);
}
