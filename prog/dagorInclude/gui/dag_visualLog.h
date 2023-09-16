//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_simpleString.h>
#include <math/dag_e3dColor.h>
#include <generic/dag_span.h>

class Point2;

namespace visuallog
{
struct LogItem;

/// @return  for EVT_REFRESH returns false, if message can be deleted from screen,
///          true to keep message active
typedef bool (*LogItemCBProc)(int event, struct visuallog::LogItem *item);

struct LogItem
{
  SimpleString text;
  E3DCOLOR color;

  LogItemCBProc cb;
  void *param;

  union
  {
    float alpha;
    int ialpha;
  };
  int level;
};

enum
{
  EVT_DELETE,
  EVT_REFRESH
};

void setMaxItems(int max_items_);

// add item with CB
void logmsg(const char *text, LogItemCBProc cb, void *param, E3DCOLOR c = E3DCOLOR(255, 255, 255), int level = 0);

// add item w/o CB
inline void logmsg(const char *text, E3DCOLOR c = E3DCOLOR(255, 255, 255), int level = 0) { logmsg(text, NULL, NULL, c, level); }

// reset font, offset and clear all log messages
void reset(bool reset_border = false);

// just clear all log messages
void clear();

void setOffset(const Point2 &offs);
void setFont(const char *name);
void setDrawDisabled(bool disabled);

// these functions are for internal use in core
void act(float dt);
void draw(int min_level = 0);

dag::ConstSpan<SimpleString> getHistory();
} // namespace visuallog


#if _TARGET_PC
#if DAGOR_DBGLEVEL > 0
#define DEFAULT_VISUALLOG_MAX_ITEMS 5
#else
#define DEFAULT_VISUALLOG_MAX_ITEMS 1
#endif
#else // console
#if DAGOR_DBGLEVEL > 0
#define DEFAULT_VISUALLOG_MAX_ITEMS 5
#else
#define DEFAULT_VISUALLOG_MAX_ITEMS 0 // just red box
#endif
#endif
