//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_e3dColor.h>
#include <math/dag_TMatrix.h>
#include <math/dag_bounds3.h>
#include <vecmath/dag_vecMathDecl.h>

// froward declarations for external classes
class GeomNodeTree;
class BaseTexture;
typedef BaseTexture Texture;
typedef BaseTexture CubeTexture;
typedef BaseTexture VolTexture;
typedef BaseTexture ArrayTexture;
struct Capsule;


//
// cached debug rendering (generally much faster)
//
bool init_draw_cached_debug_twocolored_shader();
void close_draw_cached_debug(); // shaders + render state overrides

void begin_draw_cached_debug_lines(bool test_z = true, bool write_z = true, bool z_func_less = false);
bool begin_draw_cached_debug_lines_ex();

void set_cached_debug_lines_wtm(const TMatrix &wtm);

void end_draw_cached_debug_lines();
void end_draw_cached_debug_lines_ex();


void draw_cached_debug_line(const Point3 &p0, const Point3 &p1, E3DCOLOR color);
void draw_cached_debug_line_twocolored(const Point3 &p0, const Point3 &p1, E3DCOLOR color_front, E3DCOLOR color_behind);
void draw_cached_debug_line(const Point3 *p0, int nm, E3DCOLOR color);

void draw_cached_debug_box(const BBox3 &box, E3DCOLOR color);

void draw_cached_debug_box(const BBox3 &box, E3DCOLOR color, TMatrix tm);

void draw_cached_debug_box(const Point3 &p0, const Point3 &ax, const Point3 &ay, const Point3 &az, E3DCOLOR color);

void draw_cached_debug_sphere(const Point3 &c, real rad, E3DCOLOR col, int segs = 24);

void draw_cached_debug_circle(const Point3 &c, const Point3 &a1, const Point3 &a2, real rad, E3DCOLOR col, int segs = 24);

inline void draw_cached_debug_xz_circle(const Point3 &c, real rad, E3DCOLOR col, int segs = 24)
{
  draw_cached_debug_circle(c, Point3(1, 0, 0), Point3(0, 0, 1), rad, col, segs);
}

void draw_cached_debug_solid_box(const mat43f *__restrict tm, const bbox3f *__restrict box, int count, E3DCOLOR c);

void draw_cached_debug_capsule_w(const Point3 &wp0, const Point3 &wp1, float rad, E3DCOLOR c);
void draw_cached_debug_capsule_w(const Capsule &cap, E3DCOLOR c);
void draw_cached_debug_capsule(const Capsule &cap, E3DCOLOR c, const TMatrix &wtm);

void draw_cached_debug_cylinder(const TMatrix &wtm, float rad, float height, E3DCOLOR c);
void draw_cached_debug_cylinder(const TMatrix &tm, Point3 &a, Point3 &b, float rad, E3DCOLOR c);
void draw_cached_debug_cylinder_w(Point3 &a, Point3 &b, float rad, E3DCOLOR c);

void draw_cached_debug_cone(const Point3 &p0, const Point3 &p1, real angleRad, E3DCOLOR col, int segs = 24);

// Draw bone link using draw_cached_debug_line().
void draw_skeleton_link(const Point3 &local_target, real radius = 0.02f, E3DCOLOR link_color = E3DCOLOR(200, 180, 20));

// Draw skeleton bones using draw_cached_debug_line().
void draw_skeleton_tree(const GeomNodeTree &tree, real radius = 0.02f, E3DCOLOR point_color = E3DCOLOR(200, 180, 20),
  E3DCOLOR link_color = E3DCOLOR(200, 180, 20));

// draws filled triangle list
void draw_cached_debug_trilist(const Point3 *p, int tn, E3DCOLOR c = E3DCOLOR_MAKE(255, 255, 64, 255));
// draws filled hexagon
void draw_cached_debug_hex(const TMatrix &view_itm, const Point3 &pos, real rad, E3DCOLOR c = E3DCOLOR_MAKE(255, 255, 64, 255));
// draws filled quad
void draw_cached_debug_quad(const Point3 p[4], E3DCOLOR c = E3DCOLOR_MAKE(255, 255, 64, 255));
void draw_cached_debug_solid_triangle(const Point3 p[3], E3DCOLOR c);

struct Frustum;
void draw_cached_debug_quad(vec4f &p0, vec4f &p1, vec4f &p2, vec4f &p3, E3DCOLOR col);
void draw_debug_frustum(const Frustum &fr, E3DCOLOR col);

void draw_cached_debug_proj_matrix(const TMatrix4 &tm, E3DCOLOR nearplaneColor = E3DCOLOR_MAKE(0, 255, 0, 255),
  E3DCOLOR farplaneColor = E3DCOLOR_MAKE(255, 0, 0, 255), E3DCOLOR sideColor = E3DCOLOR_MAKE(255, 255, 255, 255));

//
// store RT as tga
//
void save_rt_image_as_tga(Texture *tex, const char *filename);
void save_cube_rt_image_as_tga(CubeTexture *tex, int id, const char *filename, int level = 0);

// store RT as DDSx
bool save_tex_as_ddsx(Texture *tex, const char *filename, bool srgb = false);
bool save_cubetex_as_ddsx(CubeTexture *tex, const char *filename, bool srgb = false);
bool save_voltex_as_ddsx(VolTexture *tex, const char *filename, bool srgb = false);

//
// non-cached debug rendering (uses currect WTM in d3d, or reused current begin_draw_cached...)
//
#if DAGOR_DBGLEVEL > 0
#define D3D_PROLOGUE() bool need_end = begin_draw_cached_debug_lines_ex()
#define D3D_EPILOGUE()                \
  if (need_end)                       \
    end_draw_cached_debug_lines_ex(); \
  else

inline void draw_debug_hex(const TMatrix &view_itm, const Point3 &pos, real rad, E3DCOLOR c = E3DCOLOR_MAKE(255, 255, 64, 255))
{
  D3D_PROLOGUE();
  draw_cached_debug_hex(view_itm, pos, rad, c);
  D3D_EPILOGUE();
}

inline void draw_debug_quad(const Point3 p[4], E3DCOLOR c = E3DCOLOR_MAKE(255, 64, 255, 255))
{
  D3D_PROLOGUE();
  draw_cached_debug_quad(p, c);
  D3D_EPILOGUE();
}

inline void draw_debug_line(const Point3 *pt, int numpt, E3DCOLOR c = E3DCOLOR_MAKE(255, 32, 32, 255))
{
  D3D_PROLOGUE();
  draw_cached_debug_line(pt, numpt, c);
  D3D_EPILOGUE();
}

inline void draw_debug_line(const Point3 &p0, const Point3 &p1, E3DCOLOR c = E3DCOLOR_MAKE(255, 32, 32, 255))
{
  D3D_PROLOGUE();
  draw_cached_debug_line(p0, p1, c);
  D3D_EPILOGUE();
}


inline void draw_debug_box(const BBox3 &b, E3DCOLOR c = E3DCOLOR_MAKE(255, 64, 255, 255))
{
  D3D_PROLOGUE();
  draw_cached_debug_box(b, c);
  D3D_EPILOGUE();
}

inline void draw_debug_box(const BBox3 &b, const TMatrix &tm, E3DCOLOR c = E3DCOLOR_MAKE(64, 255, 64, 255))
{
  D3D_PROLOGUE();
  draw_cached_debug_box(tm * b[0], tm.getcol(0) * b.width()[0], tm.getcol(1) * b.width()[1], tm.getcol(2) * b.width()[2], c);
  D3D_EPILOGUE();
}


inline void draw_debug_sph(const Point3 &p0, real rad, E3DCOLOR c = E3DCOLOR_MAKE(255, 64, 255, 255))
{
  D3D_PROLOGUE();
  draw_cached_debug_sphere(p0, rad, c);
  D3D_EPILOGUE();
}


inline void draw_debug_sph(const BSphere3 &s, E3DCOLOR c = E3DCOLOR_MAKE(255, 64, 255, 255))
{
  D3D_PROLOGUE();
  draw_cached_debug_sphere(s.c, s.r, c);
  D3D_EPILOGUE();
}

inline void draw_debug_circle(const Point3 &c, const Point3 &a1, const Point3 &a2, real rad,
  E3DCOLOR col = E3DCOLOR_MAKE(255, 64, 255, 255), int segs = 24)
{
  D3D_PROLOGUE();
  draw_cached_debug_circle(c, a1, a2, rad, col, segs);
  D3D_EPILOGUE();
}

inline void draw_debug_xz_circle(const Point3 &c, real rad, E3DCOLOR col)
{
  D3D_PROLOGUE();
  draw_cached_debug_xz_circle(c, rad, col);
  D3D_EPILOGUE();
}

inline void draw_debug_capsule_w(const Capsule &cap, E3DCOLOR c)
{
  D3D_PROLOGUE();
  draw_cached_debug_capsule_w(cap, c);
  D3D_EPILOGUE();
}

inline void draw_debug_capsule_w(const Point3 &w0, const Point3 &w1, float rad, E3DCOLOR c)
{
  D3D_PROLOGUE();
  draw_cached_debug_capsule_w(w0, w1, rad, c);
  D3D_EPILOGUE();
}

#else
inline void draw_debug_hex(const TMatrix &, const Point3 &, real, E3DCOLOR = E3DCOLOR_MAKE(255, 255, 64, 255)) {}
inline void draw_debug_quad(const Point3[4], E3DCOLOR = E3DCOLOR_MAKE(255, 64, 255, 255)) {}
inline void draw_debug_line(const Point3 *, int, E3DCOLOR = E3DCOLOR_MAKE(255, 32, 32, 255)) {}
inline void draw_debug_line(const Point3 &, const Point3 &, E3DCOLOR = E3DCOLOR_MAKE(255, 32, 32, 255)) {}
inline void draw_debug_box(const BBox3 &, E3DCOLOR = E3DCOLOR_MAKE(255, 64, 255, 255)) {}
inline void draw_debug_box(const BBox3 &, const TMatrix &, E3DCOLOR = E3DCOLOR_MAKE(64, 255, 64, 255)) {}
inline void draw_debug_sph(const Point3 &, real, E3DCOLOR = E3DCOLOR_MAKE(255, 64, 255, 255)) {}
inline void draw_debug_sph(const BSphere3 &, E3DCOLOR = E3DCOLOR_MAKE(255, 64, 255, 255)) {}
inline void draw_debug_circle(const Point3 &, const Point3 &, const Point3 &, real, E3DCOLOR = E3DCOLOR_MAKE(255, 64, 255, 255),
  int = 24)
{}
inline void draw_debug_xz_circle(const Point3 &, real, E3DCOLOR) {}
inline void draw_debug_capsule_w(const Point3 &, const Point3 &, float, E3DCOLOR) {}
inline void draw_debug_capsule_w(const Capsule &, E3DCOLOR) {}

#endif

#define DEBUG3D_DEFAULT_FRAMES_TO_BUFFER 10

void draw_debug_line_buffered(const Point3 &p0, const Point3 &p1, E3DCOLOR c = E3DCOLOR_MAKE(255, 32, 32, 255),
  size_t frames = DEBUG3D_DEFAULT_FRAMES_TO_BUFFER);
void draw_debug_arrow_buffered(const Point3 &from, const Point3 &to, E3DCOLOR c = E3DCOLOR_MAKE(255, 32, 32, 255),
  size_t frames = DEBUG3D_DEFAULT_FRAMES_TO_BUFFER);
void draw_debug_rect_buffered(const Point3 &p0, const Point3 &p1, const Point3 &p2, E3DCOLOR color = E3DCOLOR_MAKE(255, 48, 255, 255),
  size_t frames = DEBUG3D_DEFAULT_FRAMES_TO_BUFFER);
void draw_debug_box_buffered(const BBox3 &box, E3DCOLOR color = E3DCOLOR_MAKE(255, 64, 255, 255),
  size_t frames = DEBUG3D_DEFAULT_FRAMES_TO_BUFFER);
void draw_debug_box_buffered(const Point3 &p0, const Point3 &ax, const Point3 &ay, const Point3 &az,
  E3DCOLOR color = E3DCOLOR_MAKE(255, 64, 255, 255), size_t frames = DEBUG3D_DEFAULT_FRAMES_TO_BUFFER);
void draw_debug_box_buffered(const BBox3 &b, const TMatrix &tm, E3DCOLOR color = E3DCOLOR_MAKE(255, 64, 255, 255),
  size_t frames = DEBUG3D_DEFAULT_FRAMES_TO_BUFFER);
/// half_diag1 and half_diag2 are assumed to be orthogonal and lying in the same plane
void draw_debug_elipse_buffered(const Point3 &pos, const Point3 &half_diag1, const Point3 &half_diag2,
  E3DCOLOR col = E3DCOLOR_MAKE(255, 255, 64, 255), int segs = 24, size_t frames = DEBUG3D_DEFAULT_FRAMES_TO_BUFFER);
void draw_debug_circle_buffered(const Point3 &pos, Point3 norm, real rad, E3DCOLOR col = E3DCOLOR_MAKE(255, 255, 64, 255),
  int segs = 24, size_t frames = DEBUG3D_DEFAULT_FRAMES_TO_BUFFER);
void draw_debug_tube_buffered(const Point3 &p0, const Point3 &p1, float radius, E3DCOLOR col = E3DCOLOR_MAKE(255, 255, 0, 255),
  int segs = 24, float circle_density = 0.1, size_t frames = DEBUG3D_DEFAULT_FRAMES_TO_BUFFER);
void draw_debug_sphere_buffered(const Point3 &c, real rad, E3DCOLOR col, int segs = 24,
  size_t frames = DEBUG3D_DEFAULT_FRAMES_TO_BUFFER);
void draw_debug_tetrapod_buffered(const Point3 &c, real radius, E3DCOLOR col = E3DCOLOR_MAKE(255, 255, 64, 255),
  size_t frames = DEBUG3D_DEFAULT_FRAMES_TO_BUFFER);
void draw_debug_tehedron_buffered(const Point3 &c, real radius, E3DCOLOR col = E3DCOLOR_MAKE(255, 255, 64, 255),
  size_t frames = DEBUG3D_DEFAULT_FRAMES_TO_BUFFER);
void flush_buffered_debug_lines(bool game_is_paused = false);
void clear_buffered_debug_lines();
void draw_debug_cylinder_buffered(const Point3 &p0, const Point3 &p1, float radius, E3DCOLOR col, int segs, size_t frames);
void draw_debug_cylinder_buffered(const Point3 &p0, const Point3 &p1, float radius1, float radius2, E3DCOLOR col, int segs,
  size_t frames);

void draw_debug_capsule_buffered(const Point3 &p0, const Point3 &p1, real rad, E3DCOLOR col, int segs = 24,
  size_t frames = DEBUG3D_DEFAULT_FRAMES_TO_BUFFER);

void draw_debug_cone_buffered(const Point3 &p0, const Point3 &p1, real angleRad, E3DCOLOR col, int segs = 24,
  size_t frames = DEBUG3D_DEFAULT_FRAMES_TO_BUFFER);

#undef D3D_PROLOGUE
#undef D3D_EPILOGUE
