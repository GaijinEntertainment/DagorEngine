//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_globDef.h>
#include <math/dag_frustum.h>

class Point3;
struct Color3;

namespace tires0
{
class ITrackEmitter
{
public:
  // emit new track (start new or continue existing)
  virtual void emit(const Point3 &norm, const Point3 &pos, const Point3 &movedir, const Point3 &wdir, const Color3 &lt,
    real side_speed, int tex_id) = 0;

  // finalize track
  virtual void finalize() = 0;

protected:
  ITrackEmitter(){};
  virtual ~ITrackEmitter(){};
};

// init system. load settings from blk-file
void init(const char *blk_file, bool pack_normal_into_color);

// release system
void release();

// remove all tire tracks from screen
void clear(bool completeClear = false);

// should be called once per frame to reset dynbuf pos to provide lockless rendering during frame
void reset_per_frame_dynbuf_pos();

// act tires
void act(real dt);

// before render tires
void before_render(const Frustum &frustum, const Point3 &pos);

// render tires
void render();

// create new track emitter
ITrackEmitter *create_emitter();

// delete track emitter.
void delete_emitter(ITrackEmitter *e);
} // namespace tires0
