//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <squirrel.h>


namespace darg
{

enum LayoutFlow
{
  FLOW_PARENT_RELATIVE = 0,
  FLOW_HORIZONTAL,
  FLOW_VERTICAL,
};


enum ElemAlign
{
  PLACE_DEFAULT = 0,

  ALIGN_LEFT_OR_TOP = 1,
  ALIGN_CENTER = 2,
  ALIGN_RIGHT_OR_BOTTOM = 3,

  ALIGN_LEFT = ALIGN_LEFT_OR_TOP,
  ALIGN_RIGHT = ALIGN_RIGHT_OR_BOTTOM,

  ALIGN_TOP = ALIGN_LEFT_OR_TOP,
  ALIGN_BOTTOM = ALIGN_RIGHT_OR_BOTTOM,
};

enum Orientation
{
  O_HORIZONTAL = 0x1,
  O_VERTICAL = 0x2
};

enum TextOverflow
{
  TOVERFLOW_CLIP = 0,
  TOVERFLOW_CHAR,
  TOVERFLOW_WORD,
  TOVERFLOW_LINE
};


enum VectorCanvasCommand
{
  VECTOR_WIDTH = 0,
  VECTOR_COLOR,
  VECTOR_FILL_COLOR,
  VECTOR_MID_COLOR,
  VECTOR_OUTER_LINE,
  VECTOR_CENTER_LINE,
  VECTOR_INNER_LINE,
  VECTOR_TM_OFFSET,
  VECTOR_TM_SCALE,
  VECTOR_LINE,
  VECTOR_LINE_INDENT_PX,
  VECTOR_LINE_INDENT_PCT,
  VECTOR_ELLIPSE,
  VECTOR_SECTOR,
  VECTOR_RECTANGLE,
  VECTOR_POLY,
  VECTOR_QUADS,
  VECTOR_INVERSE_POLY,
  VECTOR_OPACITY,
  VECTOR_LINE_DASHED,
  VECTOR_NOP,
};

enum SetupMode
{
  SM_INITIAL = 0,
  SM_REBUILD_UPDATE,
  SM_REALTIME_UPDATE
};

enum KeepAspectMode
{
  KEEP_ASPECT_NONE = SQFalse,
  KEEP_ASPECT_FIT = SQTrue,
  KEEP_ASPECT_FILL = 2
};

// Directly maps to xbox gamepad dirpad
enum Direction
{
  DIR_UP = 0,
  DIR_DOWN = 1,
  DIR_LEFT = 2,
  DIR_RIGHT = 3,
};

enum XmbCode
{
  XMB_STOP,
  XMB_CONTINUE,
};


enum BehaviorResult
{
  R_PROCESSED = 0x01,
  R_STOPPED = 0x02,
  R_REBUILD_RENDER_AND_INPUT_LISTS = 0x04,

  R_USER_FLAG = 0x100, // User defined flags start here
};


} // namespace darg
