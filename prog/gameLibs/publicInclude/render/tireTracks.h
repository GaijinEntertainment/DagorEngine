//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_globDef.h>
#include <math/dag_bounds3.h>
#include <generic/dag_tab.h>

class Point3;
struct Color3;
struct Frustum;
class DataBlock;

namespace tire_tracks
{

void emit(int id, const Point3 &norm, const Point3 &pos, const Point3 &movedir, real opacity, int tex_id, float additional_width,
  float omnidirectional_tex_blend);

void finalize_track(int id);

// init system. load settings from blk-file
void init(const char *blk_file, bool has_normalmap, bool has_vertex_normal, bool is_low_quality, bool stub_render_mode);

// release system
void release(bool close_emitters = true);

// remove all tire tracks from screen
void clear(bool completeClear = false);

// update vbuffer
void beforeRender(float dt, const Point3 &origin);

// render tires
void render(const Frustum &frustum, bool for_displacement);

// create new track emitter
// prio_scale_factor - the smaller number is (>=0) the bigger is priority. Visible distance is affected.
// so, if prio_scale_factor is 0 - it is very important (always visible)  track
// so, if prio_scale_factor is 10 - it will be suppose to be sqrt(10) times more distant, and so, less important
// ownership will be obtained for minTimeToOwn
int create_emitter(float width, float texture_length_factor, float minTimeToOwn, float prio_scale_factor, int track_type_no);

// delete track emitter.
void delete_emitter(int id);

// level specific params
void set_current_params(const DataBlock *data);

// removes tire tracks in given bounding box
void invalidate_region(const BBox3 &bbox);

// track updated regions holder
void add_updated_region(const BBox3 &bbox);
const Tab<BBox3> &get_updated_regions();
void clear_updated_regions();
bool is_initialized();
} // namespace tire_tracks
