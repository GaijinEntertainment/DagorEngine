// Copyright (c) 2014-2018 Omar Cornut
//  dear imgui, v1.63 WIP
//  (drawing and font code)

// Contains implementation for
// - Default styles
// - ImDrawList
// - ImDrawData
// - Internal Render Helpers

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <math.h> // sqrtf, fabsf, fmodf, powf, floorf, ceilf, cosf, sinf
#include <gui/dag_imgui_drawlist.h>
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#define IM_ARRAYSIZE(_ARR) ((int)(sizeof(_ARR) / sizeof(*_ARR))) // Size of a static C-style array. Don't use on pointers!

#if !defined(alloca)
#if defined(__GLIBC__) || defined(__sun) || defined(__CYGWIN__)
#include <alloca.h>   // alloca (glibc uses <alloca.h>. Note that Cygwin may have _WIN32 defined, so the order matters here)
#define ALLOCA alloca // for clang with MS Codegen
#elif defined(_WIN32)
#include <malloc.h> // alloca
#if !defined(alloca)
#define ALLOCA _alloca // for clang with MS Codegen
#else
#define ALLOCA alloca // for clang with MS Codegen
#endif
#else
#include <stdlib.h>   // alloca
#define ALLOCA alloca // for clang with MS Codegen
#endif
#else
#define ALLOCA alloca // for clang with MS Codegen
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4505) // unreferenced local function has been removed (stb stuff)
#pragma warning(disable : 4996) // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wold-style-cast" // warning : use of old-style cast                              // yes, they are
                                                    // more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"    // warning : comparing floating point with == or != is unsafe   // storing and
                                                    // comparing against same constants ok.
#pragma clang diagnostic ignored "-Wglobal-constructors" // warning : declaration requires a global destructor           // similar to
                                                         // above, not sure what the exact difference it.
#pragma clang diagnostic ignored "-Wsign-conversion"     // warning : implicit conversion changes signedness             //
#if __has_warning("-Wcomma")
#pragma clang diagnostic ignored "-Wcomma" // warning : possible misuse of comma operator here             //
#endif
#if __has_warning("-Wreserved-id-macro")
#pragma clang diagnostic ignored "-Wreserved-id-macro" // warning : macro name is a reserved identifier                //
#endif
#if __has_warning("-Wdouble-promotion")
#pragma clang diagnostic ignored "-Wdouble-promotion" // warning: implicit conversion from 'float' to 'double' when passing argument to
                                                      // function
#endif
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"  // warning: 'xxxx' defined but not used
#pragma GCC diagnostic ignored "-Wdouble-promotion" // warning: implicit conversion from 'float' to 'double' when passing argument to
                                                    // function
#pragma GCC diagnostic ignored "-Wconversion"       // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#if __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wclass-memaccess" // warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no
                                                   // trivial copy-assignment; use assignment or value-initialization instead
#endif
#endif

//-----------------------------------------------------------------------------
// ImDrawListData
//-----------------------------------------------------------------------------

namespace dag_imgui_drawlist
{
#define IM_PI 3.14159265358979323846f

#define IM_COL32(R, G, B, A)                                                                                            \
  (((ImU32)(A) << DAG_IM_COL32_A_SHIFT) | ((ImU32)(B) << DAG_IM_COL32_B_SHIFT) | ((ImU32)(G) << DAG_IM_COL32_G_SHIFT) | \
    ((ImU32)(R) << DAG_IM_COL32_R_SHIFT))

// Helpers: Maths
// - Wrapper for standard libs functions. (Note that imgui_demo.cpp does _not_ use them to keep the code easy to copy)
inline float ImFabs(float x) { return fabsf(x); }
inline float ImSqrt(float x) { return sqrtf(x); }
inline float ImPow(float x, float y) { return powf(x, y); }
inline double ImPow(double x, double y) { return pow(x, y); }
inline float ImFmod(float x, float y) { return fmodf(x, y); }
inline double ImFmod(double x, double y) { return fmod(x, y); }
inline float ImCos(float x) { return cosf(x); }
inline float ImSin(float x) { return sinf(x); }
inline float ImAcos(float x) { return acosf(x); }
inline float ImAtan2(float y, float x) { return atan2f(y, x); }
inline double ImAtof(const char *s) { return atof(s); }
inline float ImFloorStd(float x)
{
  return floorf(x);
} // we already uses our own ImFloor() { return (float)(int)v } internally so the standard one wrapper is named differently (it's used
  // by stb_truetype)
inline float ImCeil(float x) { return ceilf(x); }
// - ImMin/ImMax/ImClamp/ImLerp/ImSwap are used by widgets which support for variety of types: signed/unsigned int/long long
// float/double, using templates here but we could also redefine them 6 times
template <typename T>
static inline T ImMin(T lhs, T rhs)
{
  return lhs < rhs ? lhs : rhs;
}
template <typename T>
static inline T ImMax(T lhs, T rhs)
{
  return lhs >= rhs ? lhs : rhs;
}
template <typename T>
static inline T ImClamp(T v, T mn, T mx)
{
  return (v < mn) ? mn : (v > mx) ? mx : v;
}
template <typename T>
static inline T ImLerp(T a, T b, float t)
{
  return (T)(a + (b - a) * t);
}
template <typename T>
static inline void ImSwap(T &a, T &b)
{
  T tmp = a;
  a = b;
  b = tmp;
}
static inline int ImLerp(int a, int b, float t) { return (int)(float(a) + float(b - a) * t + 0.5f); }
// - Misc maths helpers
static inline ImVec2 ImMin(const ImVec2 &lhs, const ImVec2 &rhs) { return min(lhs, rhs); }
static inline ImVec2 ImMax(const ImVec2 &lhs, const ImVec2 &rhs) { return max(lhs, rhs); }
static inline ImVec2 ImClamp(const ImVec2 &v, const ImVec2 &mn, ImVec2 mx)
{
  return ImVec2((v.x < mn.x) ? mn.x : (v.x > mx.x) ? mx.x : v.x, (v.y < mn.y) ? mn.y : (v.y > mx.y) ? mx.y : v.y);
}
static inline ImVec2 ImLerp(const ImVec2 &a, const ImVec2 &b, float t) { return ImVec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t); }
static inline ImVec2 ImLerp(const ImVec2 &a, const ImVec2 &b, const ImVec2 &t)
{
  return ImVec2(a.x + (b.x - a.x) * t.x, a.y + (b.y - a.y) * t.y);
}
static inline Point4 ImLerp(const Point4 &a, const Point4 &b, float t)
{
  return Point4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}
static inline float ImSaturate(float f) { return (f < 0.0f) ? 0.0f : (f > 1.0f) ? 1.0f : f; }
static inline float ImLengthSqr(const ImVec2 &lhs) { return lhs.x * lhs.x + lhs.y * lhs.y; }
static inline float ImLengthSqr(const Point4 &lhs) { return lhs.x * lhs.x + lhs.y * lhs.y + lhs.z * lhs.z + lhs.w * lhs.w; }
static inline float ImInvLength(const ImVec2 &lhs, float fail_value)
{
  float d = lhs.x * lhs.x + lhs.y * lhs.y;
  if (d > 0.0f)
    return 1.0f / ImSqrt(d);
  return fail_value;
}
static inline float ImFloor(float f) { return (float)floorf(f); }
static inline ImVec2 ImFloor(const ImVec2 &v) { return ImVec2((float)(int)v.x, (float)(int)v.y); }
static inline float ImDot(const ImVec2 &a, const ImVec2 &b) { return a.x * b.x + a.y * b.y; }
static inline ImVec2 ImRotate(const ImVec2 &v, float cos_a, float sin_a)
{
  return ImVec2(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a);
}
static inline ImVec2 ImMul(const ImVec2 &lhs, const ImVec2 &rhs) { return mul(lhs, rhs); }


ImDrawList::ImDrawListSharedData::ImDrawListSharedData()
{
  TexUvWhitePixel = ImVec2(0, 0);
  CurveTessellationTol = 0.0f;
  ClipRectFullscreen = Point4(-8192.0f, -8192.0f, +8192.0f, +8192.0f);

  // Const data
  for (int i = 0; i < IM_ARRAYSIZE(CircleVtx12); i++)
  {
    const float a = ((float)i * 2 * IM_PI) / (float)IM_ARRAYSIZE(CircleVtx12);
    CircleVtx12[i] = ImVec2(ImCos(a), ImSin(a));
  }
}

//-----------------------------------------------------------------------------
// ImDrawList
//-----------------------------------------------------------------------------

void ImDrawList::Clear()
{
  CmdBuffer.resize(0);
  IdxBuffer.resize(0);
  VtxBuffer.resize(0);
  _VtxCurrentIdx = 0;
  _VtxWritePtr = NULL;
  _IdxWritePtr = NULL;
  _ClipRectStack.resize(0);
  _TextureIdStack.resize(0);
  _Path.resize(0);
  AddDrawCmd();
}

void ImDrawList::ClearFreeMemory()
{
  clear_and_shrink(CmdBuffer);
  clear_and_shrink(IdxBuffer);
  clear_and_shrink(VtxBuffer);
  _VtxCurrentIdx = 0;
  _VtxWritePtr = NULL;
  _IdxWritePtr = NULL;
  clear_and_shrink(_ClipRectStack);
  clear_and_shrink(_TextureIdStack);
  clear_and_shrink(_Path);
}

// Using macros because C++ is a terrible language, we want guaranteed inline, no code in header, and no overhead in Debug builds
#define GET_CURRENT_CLIP_RECT() (_ClipRectStack.size() ? _ClipRectStack.data()[_ClipRectStack.size() - 1] : _Data.ClipRectFullscreen)
#define GET_CURRENT_TEX_ID()    (_TextureIdStack.size() ? _TextureIdStack.data()[_TextureIdStack.size() - 1] : IM_BAD_TEXTUREID)

void ImDrawList::AddDrawCmd()
{
  ImDrawCmd draw_cmd;
  draw_cmd.ClipRect = GET_CURRENT_CLIP_RECT();
  draw_cmd.TextureId = GET_CURRENT_TEX_ID();

  G_ASSERT(draw_cmd.ClipRect.x <= draw_cmd.ClipRect.z && draw_cmd.ClipRect.y <= draw_cmd.ClipRect.w);
  CmdBuffer.push_back(draw_cmd);
}

void ImDrawList::AddCallback(ImDrawCallback callback, void *callback_data)
{
  ImDrawCmd *current_cmd = CmdBuffer.size() ? &CmdBuffer.back() : NULL;
  if (!current_cmd || current_cmd->ElemCount != 0 || current_cmd->UserCallback != NULL)
  {
    AddDrawCmd();
    current_cmd = &CmdBuffer.back();
  }
  current_cmd->UserCallback = callback;
  current_cmd->UserCallbackData = callback_data;

  AddDrawCmd(); // Force a new command after us (see comment below)
}

// Our scheme may appears a bit unusual, basically we want the most-common calls AddLine AddRect etc. to not have to perform any check
// so we always have a command ready in the stack. The cost of figuring out if a new command has to be added or if we can merge is paid
// in those Update** functions only.
void ImDrawList::UpdateClipRect()
{
  // If current command is used with different settings we need to add a new command
  const Point4 curr_clip_rect = GET_CURRENT_CLIP_RECT();
  ImDrawCmd *curr_cmd = CmdBuffer.size() > 0 ? &CmdBuffer.data()[CmdBuffer.size() - 1] : NULL;
  if (!curr_cmd || (curr_cmd->ElemCount != 0 && memcmp(&curr_cmd->ClipRect, &curr_clip_rect, sizeof(Point4)) != 0) ||
      curr_cmd->UserCallback != NULL)
  {
    AddDrawCmd();
    return;
  }

  // Try to merge with previous command if it matches, else use current command
  ImDrawCmd *prev_cmd = CmdBuffer.size() > 1 ? curr_cmd - 1 : NULL;
  if (curr_cmd->ElemCount == 0 && prev_cmd && memcmp(&prev_cmd->ClipRect, &curr_clip_rect, sizeof(Point4)) == 0 &&
      prev_cmd->TextureId == GET_CURRENT_TEX_ID() && prev_cmd->UserCallback == NULL)
    CmdBuffer.pop_back();
  else
    curr_cmd->ClipRect = curr_clip_rect;
}

void ImDrawList::UpdateTextureID()
{
  // If current command is used with different settings we need to add a new command
  const ImTextureID curr_texture_id = GET_CURRENT_TEX_ID();
  ImDrawCmd *curr_cmd = CmdBuffer.size() ? &CmdBuffer.back() : NULL;
  if (!curr_cmd || (curr_cmd->ElemCount != 0 && curr_cmd->TextureId != curr_texture_id) || curr_cmd->UserCallback != NULL)
  {
    AddDrawCmd();
    return;
  }

  // Try to merge with previous command if it matches, else use current command
  ImDrawCmd *prev_cmd = CmdBuffer.size() > 1 ? curr_cmd - 1 : NULL;
  if (curr_cmd->ElemCount == 0 && prev_cmd && prev_cmd->TextureId == curr_texture_id &&
      memcmp(&prev_cmd->ClipRect, &GET_CURRENT_CLIP_RECT(), sizeof(Point4)) == 0 && prev_cmd->UserCallback == NULL)
    CmdBuffer.pop_back();
  else
    curr_cmd->TextureId = curr_texture_id;
}

#undef GET_CURRENT_CLIP_RECT
#undef GET_CURRENT_TEX_ID

// Render-level scissoring. This is passed down to your render function but not used for CPU-side coarse clipping. Prefer using
// higher-level ImGui::PushClipRect() to affect logic (hit-testing and widget culling)
void ImDrawList::PushClipRect(ImVec2 cr_min, ImVec2 cr_max, bool intersect_with_current_clip_rect)
{
  Point4 cr(cr_min.x, cr_min.y, cr_max.x, cr_max.y);
  if (intersect_with_current_clip_rect && _ClipRectStack.size())
  {
    Point4 current = _ClipRectStack.data()[_ClipRectStack.size() - 1];
    if (cr.x < current.x)
      cr.x = current.x;
    if (cr.y < current.y)
      cr.y = current.y;
    if (cr.z > current.z)
      cr.z = current.z;
    if (cr.w > current.w)
      cr.w = current.w;
  }
  cr.z = ImMax(cr.x, cr.z);
  cr.w = ImMax(cr.y, cr.w);

  _ClipRectStack.push_back(cr);
  UpdateClipRect();
}

void ImDrawList::PushClipRectFullScreen()
{
  PushClipRect(ImVec2(_Data.ClipRectFullscreen.x, _Data.ClipRectFullscreen.y),
    ImVec2(_Data.ClipRectFullscreen.z, _Data.ClipRectFullscreen.w));
}

void ImDrawList::PopClipRect()
{
  G_ASSERT(_ClipRectStack.size() > 0);
  _ClipRectStack.pop_back();
  UpdateClipRect();
}

void ImDrawList::PushTextureID(ImTextureID texture_id)
{
  _TextureIdStack.push_back(texture_id);
  UpdateTextureID();
}

void ImDrawList::PopTextureID()
{
  G_ASSERT(_TextureIdStack.size() > 0);
  _TextureIdStack.pop_back();
  UpdateTextureID();
}

// NB: this can be called with negative count for removing primitives (as long as the result does not underflow)
void ImDrawList::PrimReserve(int idx_count, int vtx_count)
{
  ImDrawCmd &draw_cmd = CmdBuffer.data()[CmdBuffer.size() - 1];
  draw_cmd.ElemCount += idx_count;

  int vtx_buffer_old_size = VtxBuffer.size();
  VtxBuffer.resize(vtx_buffer_old_size + vtx_count);
  _VtxWritePtr = VtxBuffer.data() + vtx_buffer_old_size;

  int idx_buffer_old_size = IdxBuffer.size();
  IdxBuffer.resize(idx_buffer_old_size + idx_count);
  _IdxWritePtr = IdxBuffer.data() + idx_buffer_old_size;
}

// Fully unrolled with inline call to keep our debug builds decently fast.
void ImDrawList::PrimRect(const ImVec2 &a, const ImVec2 &c, ImU32 col)
{
  ImVec2 b(c.x, a.y), d(a.x, c.y), uv(_Data.TexUvWhitePixel);
  ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
  _IdxWritePtr[0] = idx;
  _IdxWritePtr[1] = (ImDrawIdx)(idx + 1);
  _IdxWritePtr[2] = (ImDrawIdx)(idx + 2);
  _IdxWritePtr[3] = idx;
  _IdxWritePtr[4] = (ImDrawIdx)(idx + 2);
  _IdxWritePtr[5] = (ImDrawIdx)(idx + 3);
  _VtxWritePtr[0].pos = a;
  _VtxWritePtr[0].uv = uv;
  _VtxWritePtr[0].col = col;
  _VtxWritePtr[1].pos = b;
  _VtxWritePtr[1].uv = uv;
  _VtxWritePtr[1].col = col;
  _VtxWritePtr[2].pos = c;
  _VtxWritePtr[2].uv = uv;
  _VtxWritePtr[2].col = col;
  _VtxWritePtr[3].pos = d;
  _VtxWritePtr[3].uv = uv;
  _VtxWritePtr[3].col = col;
  _VtxWritePtr += 4;
  _VtxCurrentIdx += 4;
  _IdxWritePtr += 6;
}

void ImDrawList::PrimRectUV(const ImVec2 &a, const ImVec2 &c, const ImVec2 &uv_a, const ImVec2 &uv_c, ImU32 col)
{
  ImVec2 b(c.x, a.y), d(a.x, c.y), uv_b(uv_c.x, uv_a.y), uv_d(uv_a.x, uv_c.y);
  ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
  _IdxWritePtr[0] = idx;
  _IdxWritePtr[1] = (ImDrawIdx)(idx + 1);
  _IdxWritePtr[2] = (ImDrawIdx)(idx + 2);
  _IdxWritePtr[3] = idx;
  _IdxWritePtr[4] = (ImDrawIdx)(idx + 2);
  _IdxWritePtr[5] = (ImDrawIdx)(idx + 3);
  _VtxWritePtr[0].pos = a;
  _VtxWritePtr[0].uv = uv_a;
  _VtxWritePtr[0].col = col;
  _VtxWritePtr[1].pos = b;
  _VtxWritePtr[1].uv = uv_b;
  _VtxWritePtr[1].col = col;
  _VtxWritePtr[2].pos = c;
  _VtxWritePtr[2].uv = uv_c;
  _VtxWritePtr[2].col = col;
  _VtxWritePtr[3].pos = d;
  _VtxWritePtr[3].uv = uv_d;
  _VtxWritePtr[3].col = col;
  _VtxWritePtr += 4;
  _VtxCurrentIdx += 4;
  _IdxWritePtr += 6;
}

void ImDrawList::PrimQuadUV(const ImVec2 &a, const ImVec2 &b, const ImVec2 &c, const ImVec2 &d, const ImVec2 &uv_a, const ImVec2 &uv_b,
  const ImVec2 &uv_c, const ImVec2 &uv_d, ImU32 col)
{
  ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
  _IdxWritePtr[0] = idx;
  _IdxWritePtr[1] = (ImDrawIdx)(idx + 1);
  _IdxWritePtr[2] = (ImDrawIdx)(idx + 2);
  _IdxWritePtr[3] = idx;
  _IdxWritePtr[4] = (ImDrawIdx)(idx + 2);
  _IdxWritePtr[5] = (ImDrawIdx)(idx + 3);
  _VtxWritePtr[0].pos = a;
  _VtxWritePtr[0].uv = uv_a;
  _VtxWritePtr[0].col = col;
  _VtxWritePtr[1].pos = b;
  _VtxWritePtr[1].uv = uv_b;
  _VtxWritePtr[1].col = col;
  _VtxWritePtr[2].pos = c;
  _VtxWritePtr[2].uv = uv_c;
  _VtxWritePtr[2].col = col;
  _VtxWritePtr[3].pos = d;
  _VtxWritePtr[3].uv = uv_d;
  _VtxWritePtr[3].col = col;
  _VtxWritePtr += 4;
  _VtxCurrentIdx += 4;
  _IdxWritePtr += 6;
}

// TODO: Thickness anti-aliased lines cap are missing their AA fringe.
void ImDrawList::AddPolyline(const ImVec2 *points, const int points_count, ImU32 col, ImU32 fillcol, bool closed, float thickness,
  ImU32 aa_trans_mask)
{
  if (points_count < (closed ? 3 : 2))
    return;

  const ImVec2 uv = _Data.TexUvWhitePixel;

  int count = points_count;
  if (!closed)
    count = points_count - 1;

  const bool thick_line = thickness > 1.0f;
  const bool fillInternal = closed && fillcol != 0;
  if (aa_trans_mask)
  {
    // Anti-aliased stroke
    const float AA_SIZE = 1.0f;
    const ImU32 col_trans = col & ~aa_trans_mask;
    fillcol = fillInternal ? fillcol : col_trans;

    const int idx_count = (thick_line ? count * 18 : count * 12) + (fillInternal ? (points_count - 2) * 3 : 0);
    const int vtx_count = thick_line ? points_count * 4 : points_count * 3;
    PrimReserve(idx_count, vtx_count);

    // Temporary buffer
    ImVec2 *temp_normals = (ImVec2 *)ALLOCA(points_count * (thick_line ? 5 : 3) * sizeof(ImVec2));
    ImVec2 *temp_points = temp_normals + points_count;

    for (int i1 = 0; i1 < count; i1++)
    {
      const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1;
      ImVec2 diff = points[i2] - points[i1];
      diff *= ImInvLength(diff, 1.0f);
      temp_normals[i1].x = diff.y;
      temp_normals[i1].y = -diff.x;
    }
    if (!closed)
      temp_normals[points_count - 1] = temp_normals[points_count - 2];

    if (!thick_line)
    {
      if (!closed)
      {
        temp_points[0] = points[0] + temp_normals[0] * AA_SIZE;
        temp_points[1] = points[0] - temp_normals[0] * AA_SIZE;
        temp_points[(points_count - 1) * 2 + 0] = points[points_count - 1] + temp_normals[points_count - 1] * AA_SIZE;
        temp_points[(points_count - 1) * 2 + 1] = points[points_count - 1] - temp_normals[points_count - 1] * AA_SIZE;
      }

      // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
      unsigned int idx1 = _VtxCurrentIdx;
      for (int i1 = 0; i1 < count; i1++)
      {
        const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1;
        unsigned int idx2 = (i1 + 1) == points_count ? _VtxCurrentIdx : idx1 + 3;

        // Average normals
        ImVec2 dm = (temp_normals[i1] + temp_normals[i2]) * 0.5f;
        float dmr2 = dm.x * dm.x + dm.y * dm.y;
        if (dmr2 > 0.000001f)
        {
          float scale = 1.0f / dmr2;
          scale = min(100.f, scale);
          dm *= scale;
        }
        dm *= AA_SIZE;
        temp_points[i2 * 2 + 0] = points[i2] + dm;
        temp_points[i2 * 2 + 1] = points[i2] - dm;

        // Add indexes
        _IdxWritePtr[0] = (ImDrawIdx)(idx2 + 0);
        _IdxWritePtr[1] = (ImDrawIdx)(idx1 + 0);
        _IdxWritePtr[2] = (ImDrawIdx)(idx1 + 2);
        _IdxWritePtr[3] = (ImDrawIdx)(idx1 + 2);
        _IdxWritePtr[4] = (ImDrawIdx)(idx2 + 2);
        _IdxWritePtr[5] = (ImDrawIdx)(idx2 + 0);
        _IdxWritePtr[6] = (ImDrawIdx)(idx2 + 1);
        _IdxWritePtr[7] = (ImDrawIdx)(idx1 + 1);
        _IdxWritePtr[8] = (ImDrawIdx)(idx1 + 0);
        _IdxWritePtr[9] = (ImDrawIdx)(idx1 + 0);
        _IdxWritePtr[10] = (ImDrawIdx)(idx2 + 0);
        _IdxWritePtr[11] = (ImDrawIdx)(idx2 + 1);
        _IdxWritePtr += 12;

        idx1 = idx2;
      }

      if (fillInternal)
      {
        for (int i = 2; i < points_count; i++)
        {
          _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx + 2);
          _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + (i - 1) * 3 + 2);
          _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + i * 3 + 2);
          _IdxWritePtr += 3;
        }
      }

      // Add vertexes
      for (int i = 0; i < points_count; i++)
      {
        _VtxWritePtr[0].pos = points[i];
        _VtxWritePtr[0].uv = uv;
        _VtxWritePtr[0].col = col;
        _VtxWritePtr[1].pos = temp_points[i * 2 + 0];
        _VtxWritePtr[1].uv = uv;
        _VtxWritePtr[1].col = col_trans;
        _VtxWritePtr[2].pos = temp_points[i * 2 + 1];
        _VtxWritePtr[2].uv = uv;
        _VtxWritePtr[2].col = fillcol;
        _VtxWritePtr += 3;
      }
    }
    else
    {
      const float half_inner_thickness = (thickness - AA_SIZE) * 0.5f;
      if (!closed)
      {
        temp_points[0] = points[0] + temp_normals[0] * (half_inner_thickness + AA_SIZE);
        temp_points[1] = points[0] + temp_normals[0] * (half_inner_thickness);
        temp_points[2] = points[0] - temp_normals[0] * (half_inner_thickness);
        temp_points[3] = points[0] - temp_normals[0] * (half_inner_thickness + AA_SIZE);
        temp_points[(points_count - 1) * 4 + 0] =
          points[points_count - 1] + temp_normals[points_count - 1] * (half_inner_thickness + AA_SIZE);
        temp_points[(points_count - 1) * 4 + 1] = points[points_count - 1] + temp_normals[points_count - 1] * (half_inner_thickness);
        temp_points[(points_count - 1) * 4 + 2] = points[points_count - 1] - temp_normals[points_count - 1] * (half_inner_thickness);
        temp_points[(points_count - 1) * 4 + 3] =
          points[points_count - 1] - temp_normals[points_count - 1] * (half_inner_thickness + AA_SIZE);
      }

      // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
      unsigned int idx1 = _VtxCurrentIdx;
      for (int i1 = 0; i1 < count; i1++)
      {
        const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1;
        unsigned int idx2 = (i1 + 1) == points_count ? _VtxCurrentIdx : idx1 + 4;

        // Average normals
        ImVec2 dm = (temp_normals[i1] + temp_normals[i2]) * 0.5f;
        float dmr2 = dm.x * dm.x + dm.y * dm.y;
        if (dmr2 > 0.000001f)
        {
          float scale = 1.0f / dmr2;
          scale = min(100.f, scale);
          dm *= scale;
        }
        ImVec2 dm_out = dm * (half_inner_thickness + AA_SIZE);
        ImVec2 dm_in = dm * half_inner_thickness;
        temp_points[i2 * 4 + 0] = points[i2] + dm_out;
        temp_points[i2 * 4 + 1] = points[i2] + dm_in;
        temp_points[i2 * 4 + 2] = points[i2] - dm_in;
        temp_points[i2 * 4 + 3] = points[i2] - dm_out;

        // Add indexes
        _IdxWritePtr[0] = (ImDrawIdx)(idx2 + 1);
        _IdxWritePtr[1] = (ImDrawIdx)(idx1 + 1);
        _IdxWritePtr[2] = (ImDrawIdx)(idx1 + 2);
        _IdxWritePtr[3] = (ImDrawIdx)(idx1 + 2);
        _IdxWritePtr[4] = (ImDrawIdx)(idx2 + 2);
        _IdxWritePtr[5] = (ImDrawIdx)(idx2 + 1);
        _IdxWritePtr[6] = (ImDrawIdx)(idx2 + 1);
        _IdxWritePtr[7] = (ImDrawIdx)(idx1 + 1);
        _IdxWritePtr[8] = (ImDrawIdx)(idx1 + 0);
        _IdxWritePtr[9] = (ImDrawIdx)(idx1 + 0);
        _IdxWritePtr[10] = (ImDrawIdx)(idx2 + 0);
        _IdxWritePtr[11] = (ImDrawIdx)(idx2 + 1);
        _IdxWritePtr[12] = (ImDrawIdx)(idx2 + 2);
        _IdxWritePtr[13] = (ImDrawIdx)(idx1 + 2);
        _IdxWritePtr[14] = (ImDrawIdx)(idx1 + 3);
        _IdxWritePtr[15] = (ImDrawIdx)(idx1 + 3);
        _IdxWritePtr[16] = (ImDrawIdx)(idx2 + 3);
        _IdxWritePtr[17] = (ImDrawIdx)(idx2 + 2);
        _IdxWritePtr += 18;

        idx1 = idx2;
      }
      if (fillInternal)
      {
        for (int i = 2; i < points_count; i++)
        {
          _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx + 3);
          _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + (i - 1) * 4 + 3);
          _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + i * 4 + 3);
          _IdxWritePtr += 3;
        }
      }

      // Add vertexes
      for (int i = 0; i < points_count; i++)
      {
        _VtxWritePtr[0].pos = temp_points[i * 4 + 0];
        _VtxWritePtr[0].uv = uv;
        _VtxWritePtr[0].col = col_trans;
        _VtxWritePtr[1].pos = temp_points[i * 4 + 1];
        _VtxWritePtr[1].uv = uv;
        _VtxWritePtr[1].col = col;
        _VtxWritePtr[2].pos = temp_points[i * 4 + 2];
        _VtxWritePtr[2].uv = uv;
        _VtxWritePtr[2].col = col;
        _VtxWritePtr[3].pos = temp_points[i * 4 + 3];
        _VtxWritePtr[3].uv = uv;
        _VtxWritePtr[3].col = fillcol;
        _VtxWritePtr += 4;
      }
    }
    _VtxCurrentIdx += (ImDrawIdx)vtx_count;
  }
  else
  {
    // Non Anti-aliased Stroke
    fillcol = fillInternal ? fillcol : col;
    const int idx_count = count * 6 + (fillInternal ? (points_count - 2) * 3 : 0);
    const int vtx_count = count * 4 + (fillInternal ? count : 0); // FIXME-OPT: Not sharing edges
    PrimReserve(idx_count, vtx_count);

    if (fillInternal)
    {
      for (int i = 2; i < points_count; i++)
      {
        _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx);
        _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + (i - 1) * 5);
        _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + i * 5);
        _IdxWritePtr += 3;
      }
    }

    for (int i1 = 0; i1 < count; i1++)
    {
      const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1;
      const ImVec2 &p1 = points[i1];
      const ImVec2 &p2 = points[i2];
      ImVec2 diff = p2 - p1;
      diff *= ImInvLength(diff, 1.0f);

      const float dx = diff.x * (thickness * 0.5f);
      const float dy = diff.y * (thickness * 0.5f);
      if (fillInternal)
      {
        _VtxWritePtr[0].pos.x = p1.x - dy;
        _VtxWritePtr[0].pos.y = p1.y + dx;
        _VtxWritePtr[0].uv = uv;
        _VtxWritePtr[0].col = fillcol;
        _VtxWritePtr++;
        _VtxCurrentIdx++;
      }

      _VtxWritePtr[0].pos.x = p1.x + dy;
      _VtxWritePtr[0].pos.y = p1.y - dx;
      _VtxWritePtr[0].uv = uv;
      _VtxWritePtr[0].col = col;
      _VtxWritePtr[1].pos.x = p2.x + dy;
      _VtxWritePtr[1].pos.y = p2.y - dx;
      _VtxWritePtr[1].uv = uv;
      _VtxWritePtr[1].col = col;
      _VtxWritePtr[2].pos.x = p2.x - dy;
      _VtxWritePtr[2].pos.y = p2.y + dx;
      _VtxWritePtr[2].uv = uv;
      _VtxWritePtr[2].col = col;
      _VtxWritePtr[3].pos.x = p1.x - dy;
      _VtxWritePtr[3].pos.y = p1.y + dx;
      _VtxWritePtr[3].uv = uv;
      _VtxWritePtr[3].col = col;
      _VtxWritePtr += 4;

      _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx);
      _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + 1);
      _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + 2);
      _IdxWritePtr[3] = (ImDrawIdx)(_VtxCurrentIdx);
      _IdxWritePtr[4] = (ImDrawIdx)(_VtxCurrentIdx + 2);
      _IdxWritePtr[5] = (ImDrawIdx)(_VtxCurrentIdx + 3);
      _IdxWritePtr += 6;
      _VtxCurrentIdx += 4;
    }
  }
}

void ImDrawList::AddConvexPolyFilled(const ImVec2 *points, const int points_count, ImU32 col, ImU32 aa_trans_mask)
{
  const ImVec2 uv = _Data.TexUvWhitePixel;

  if (aa_trans_mask)
  {
    // Anti-aliased Fill
    const float AA_SIZE = 1.0f;
    const ImU32 col_trans = col & ~aa_trans_mask;
    const int idx_count = (points_count - 2) * 3 + points_count * 6;
    const int vtx_count = (points_count * 2);
    PrimReserve(idx_count, vtx_count);

    // Add indexes for fill
    unsigned int vtx_inner_idx = _VtxCurrentIdx;
    unsigned int vtx_outer_idx = _VtxCurrentIdx + 1;
    for (int i = 2; i < points_count; i++)
    {
      _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx);
      _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + ((i - 1) << 1));
      _IdxWritePtr[2] = (ImDrawIdx)(vtx_inner_idx + (i << 1));
      _IdxWritePtr += 3;
    }

    // Compute normals
    ImVec2 *temp_normals = (ImVec2 *)ALLOCA(points_count * sizeof(ImVec2));
    for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
    {
      const ImVec2 &p0 = points[i0];
      const ImVec2 &p1 = points[i1];
      ImVec2 diff = p1 - p0;
      diff *= ImInvLength(diff, 1.0f);
      temp_normals[i0].x = diff.y;
      temp_normals[i0].y = -diff.x;
    }

    for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
    {
      // Average normals
      const ImVec2 &n0 = temp_normals[i0];
      const ImVec2 &n1 = temp_normals[i1];
      ImVec2 dm = (n0 + n1) * 0.5f;
      float dmr2 = dm.x * dm.x + dm.y * dm.y;
      if (dmr2 > 0.000001f)
      {
        float scale = 1.0f / dmr2;
        scale = min(100.f, scale);
        dm *= scale;
      }
      dm *= AA_SIZE * 0.5f;

      // Add vertices
      _VtxWritePtr[0].pos = (points[i1] - dm);
      _VtxWritePtr[0].uv = uv;
      _VtxWritePtr[0].col = col; // Inner
      _VtxWritePtr[1].pos = (points[i1] + dm);
      _VtxWritePtr[1].uv = uv;
      _VtxWritePtr[1].col = col_trans; // Outer
      _VtxWritePtr += 2;

      // Add indexes for fringes
      _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1));
      _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + (i0 << 1));
      _IdxWritePtr[2] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1));
      _IdxWritePtr[3] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1));
      _IdxWritePtr[4] = (ImDrawIdx)(vtx_outer_idx + (i1 << 1));
      _IdxWritePtr[5] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1));
      _IdxWritePtr += 6;
    }
    _VtxCurrentIdx += (ImDrawIdx)vtx_count;
  }
  else
  {
    // Non Anti-aliased Fill
    const int idx_count = (points_count - 2) * 3;
    const int vtx_count = points_count;
    PrimReserve(idx_count, vtx_count);
    for (int i = 0; i < vtx_count; i++)
    {
      _VtxWritePtr[0].pos = points[i];
      _VtxWritePtr[0].uv = uv;
      _VtxWritePtr[0].col = col;
      _VtxWritePtr++;
    }
    for (int i = 2; i < points_count; i++)
    {
      _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx);
      _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + i - 1);
      _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + i);
      _IdxWritePtr += 3;
    }
    _VtxCurrentIdx += (ImDrawIdx)vtx_count;
  }
}

void ImDrawList::PathArcToFast(const ImVec2 &centre, float radius, int a_min_of_12, int a_max_of_12)
{
  if (radius == 0.0f || a_min_of_12 > a_max_of_12)
  {
    _Path.push_back(centre);
    return;
  }
  _Path.reserve(_Path.size() + (a_max_of_12 - a_min_of_12 + 1));
  for (int a = a_min_of_12; a <= a_max_of_12; a++)
  {
    const ImVec2 &c = _Data.CircleVtx12[a % IM_ARRAYSIZE(_Data.CircleVtx12)];
    _Path.push_back(ImVec2(centre.x + c.x * radius, centre.y + c.y * radius));
  }
}

void ImDrawList::PathArcTo(const ImVec2 &centre, float radius, float a_min, float a_max, int num_segments)
{
  if (radius == 0.0f)
  {
    _Path.push_back(centre);
    return;
  }
  _Path.reserve(_Path.size() + (num_segments + 1));
  for (int i = 0; i <= num_segments; i++)
  {
    const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
    _Path.push_back(ImVec2(centre.x + ImCos(a) * radius, centre.y + ImSin(a) * radius));
  }
}

static void PathBezierToCasteljau(ImVector<ImVec2> *path, float x1, float y1, float x2, float y2, float x3, float y3, float x4,
  float y4, float tess_tol, int level)
{
  float dx = x4 - x1;
  float dy = y4 - y1;
  float d2 = ((x2 - x4) * dy - (y2 - y4) * dx);
  float d3 = ((x3 - x4) * dy - (y3 - y4) * dx);
  d2 = (d2 >= 0) ? d2 : -d2;
  d3 = (d3 >= 0) ? d3 : -d3;
  if ((d2 + d3) * (d2 + d3) < tess_tol * (dx * dx + dy * dy))
  {
    path->push_back(ImVec2(x4, y4));
  }
  else if (level < 10)
  {
    float x12 = (x1 + x2) * 0.5f, y12 = (y1 + y2) * 0.5f;
    float x23 = (x2 + x3) * 0.5f, y23 = (y2 + y3) * 0.5f;
    float x34 = (x3 + x4) * 0.5f, y34 = (y3 + y4) * 0.5f;
    float x123 = (x12 + x23) * 0.5f, y123 = (y12 + y23) * 0.5f;
    float x234 = (x23 + x34) * 0.5f, y234 = (y23 + y34) * 0.5f;
    float x1234 = (x123 + x234) * 0.5f, y1234 = (y123 + y234) * 0.5f;

    PathBezierToCasteljau(path, x1, y1, x12, y12, x123, y123, x1234, y1234, tess_tol, level + 1);
    PathBezierToCasteljau(path, x1234, y1234, x234, y234, x34, y34, x4, y4, tess_tol, level + 1);
  }
}

void ImDrawList::PathBezierCurveTo(const ImVec2 &p2, const ImVec2 &p3, const ImVec2 &p4, int num_segments)
{
  ImVec2 p1 = _Path.back();
  if (num_segments == 0)
  {
    // Auto-tessellated
    PathBezierToCasteljau(&_Path, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, _Data.CurveTessellationTol, 0);
  }
  else
  {
    float t_step = 1.0f / (float)num_segments;
    for (int i_step = 1; i_step <= num_segments; i_step++)
    {
      float t = t_step * i_step;
      float u = 1.0f - t;
      float w1 = u * u * u;
      float w2 = 3 * u * u * t;
      float w3 = 3 * u * t * t;
      float w4 = t * t * t;
      _Path.push_back(ImVec2(w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x, w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y));
    }
  }
}

void ImDrawList::PathRect(const ImVec2 &a, const ImVec2 &b, const Point4 &rounding_)
{
  const float boxWidth = min(fabsf(b.x - a.x), fabsf(b.y - a.y)) * 0.5f - 1.f;
  const Point4 rounding = max(Point4(0, 0, 0, 0), min(rounding_, Point4(boxWidth, boxWidth, boxWidth, boxWidth)));
  const float maxRounding = max(max(rounding.x, rounding.y), max(rounding.z, rounding.w));
  if (maxRounding <= 0.0f)
  {
    PathLineTo(a);
    PathLineTo(ImVec2(b.x, a.y));
    PathLineTo(b);
    PathLineTo(ImVec2(a.x, b.y));
  }
  else
  {
    const float rounding_tl = rounding.x;
    const float rounding_tr = rounding.y;
    const float rounding_br = rounding.z;
    const float rounding_bl = rounding.w;
    if (maxRounding >= 3.f)
    {
      const int num_segments = 12 + ceilf((maxRounding - 3.f));
      PathArcTo(ImVec2(a.x + rounding_tl, a.y + rounding_tl), rounding_tl, IM_PI, 1.5f * IM_PI, num_segments);
      PathArcTo(ImVec2(b.x - rounding_tr, a.y + rounding_tr), rounding_tr, 1.5f * IM_PI, 2.f * IM_PI, num_segments);
      PathArcTo(ImVec2(b.x - rounding_br, b.y - rounding_br), rounding_br, 0, 0.5f * IM_PI, num_segments);
      PathArcTo(ImVec2(a.x + rounding_bl, b.y - rounding_bl), rounding_bl, 0.5f * IM_PI, IM_PI, num_segments);
    }
    else
    {
      PathArcToFast(ImVec2(a.x + rounding_tl, a.y + rounding_tl), rounding_tl, 6, 9);
      PathArcToFast(ImVec2(b.x - rounding_tr, a.y + rounding_tr), rounding_tr, 9, 12);
      PathArcToFast(ImVec2(b.x - rounding_br, b.y - rounding_br), rounding_br, 0, 3);
      PathArcToFast(ImVec2(a.x + rounding_bl, b.y - rounding_bl), rounding_bl, 3, 6);
    }
  }
}

void ImDrawList::AddLine(const ImVec2 &a, const ImVec2 &b, ImU32 col, float thickness, ImU32 aa_trans_mask)
{
  PathLineTo(a + ImVec2(0.5f, 0.5f));
  PathLineTo(b + ImVec2(0.5f, 0.5f));
  PathStroke(col, false, thickness, aa_trans_mask);
}

// a: upper-left, b: lower-right. we don't render 1 px sized rectangles properly.
void ImDrawList::AddRect(const ImVec2 &a, const ImVec2 &b, ImU32 col, const Point4 &rounding, float thickness, ImU32 aa_trans_mask)
{
  if (thickness < 1.f)
  {
    if (aa_trans_mask)
    {
      const ImU32 colT = col & ~aa_trans_mask;
      Point4 col0f((col >> DAG_IM_COL32_R_SHIFT) & 0xFF, (col >> DAG_IM_COL32_G_SHIFT) & 0xFF, (col >> DAG_IM_COL32_B_SHIFT) & 0xFF,
        col >> DAG_IM_COL32_A_SHIFT);
      Point4 col1f((colT >> DAG_IM_COL32_R_SHIFT) & 0xFF, (colT >> DAG_IM_COL32_G_SHIFT) & 0xFF, (colT >> DAG_IM_COL32_B_SHIFT) & 0xFF,
        colT >> DAG_IM_COL32_A_SHIFT);
      Point4 colf = floor(ImLerp(col1f, col0f, thickness) + Point4(0.5, 0.5, 0.5, 0.5));
      col = IM_COL32(colf.x, colf.y, colf.z, colf.w);
    }
    thickness = 1.f;
  }
  PathRect(a + 0.5f * ImVec2(thickness, thickness), b - 0.5f * ImVec2(thickness, thickness), rounding);
  PathStroke(col, true, thickness, aa_trans_mask);
}

void ImDrawList::AddRectFilledBordered(const ImVec2 &a, const ImVec2 &b, ImU32 col, ImU32 fillcoll, const Point4 &rounding,
  float thickness, ImU32 aa_trans_mask)
{
  if (thickness < 1.f)
  {
    if (aa_trans_mask)
    {
      const ImU32 colT = col & ~aa_trans_mask;
      Point4 col0f((col >> DAG_IM_COL32_R_SHIFT) & 0xFF, (col >> DAG_IM_COL32_G_SHIFT) & 0xFF, (col >> DAG_IM_COL32_B_SHIFT) & 0xFF,
        col >> DAG_IM_COL32_A_SHIFT);
      Point4 col1f((colT >> DAG_IM_COL32_R_SHIFT) & 0xFF, (colT >> DAG_IM_COL32_G_SHIFT) & 0xFF, (colT >> DAG_IM_COL32_B_SHIFT) & 0xFF,
        colT >> DAG_IM_COL32_A_SHIFT);
      Point4 colf = floor(ImLerp(col1f, col0f, thickness) + Point4(0.5, 0.5, 0.5, 0.5));
      col = IM_COL32(colf.x, colf.y, colf.z, colf.w);
    }
    thickness = 1.f;
  }
  PathRect(a + 0.5f * ImVec2(thickness, thickness), b - 0.5f * ImVec2(thickness, thickness), rounding);
  PathStrokeFilled(col, fillcoll, thickness, aa_trans_mask);
}

void ImDrawList::AddRectFilled(const ImVec2 &a, const ImVec2 &b, ImU32 col, const Point4 &rounding, ImU32 aa_trans_mask)
{
  const float maxRounding = max(max(rounding.x, rounding.y), max(rounding.z, rounding.w));
  if (maxRounding > 0.0f)
  {
    PathRect(a, b, rounding);
    PathFillConvex(col, aa_trans_mask);
  }
  else
  {
    PrimReserve(6, 4);
    PrimRect(a, b, col);
  }
}

void ImDrawList::AddRectFilledMultiColor(const ImVec2 &a, const ImVec2 &c, ImU32 col_upr_left, ImU32 col_upr_right,
  ImU32 col_bot_right, ImU32 col_bot_left)
{
  if (((col_upr_left | col_upr_right | col_bot_right | col_bot_left) & DAG_IM_COL32_A_MASK) == 0)
    return;

  const ImVec2 uv = _Data.TexUvWhitePixel;
  PrimReserve(6, 4);
  PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx));
  PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 1));
  PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 2));
  PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx));
  PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 2));
  PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 3));
  PrimWriteVtx(a, uv, col_upr_left);
  PrimWriteVtx(ImVec2(c.x, a.y), uv, col_upr_right);
  PrimWriteVtx(c, uv, col_bot_right);
  PrimWriteVtx(ImVec2(a.x, c.y), uv, col_bot_left);
}

void ImDrawList::AddQuad(const ImVec2 &a, const ImVec2 &b, const ImVec2 &c, const ImVec2 &d, ImU32 col, float thickness,
  ImU32 aa_trans_mask)
{
  PathLineTo(a);
  PathLineTo(b);
  PathLineTo(c);
  PathLineTo(d);
  PathStroke(col, true, thickness, aa_trans_mask);
}

void ImDrawList::AddQuadFilled(const ImVec2 &a, const ImVec2 &b, const ImVec2 &c, const ImVec2 &d, ImU32 col, ImU32 aa_trans_mask)
{
  PathLineTo(a);
  PathLineTo(b);
  PathLineTo(c);
  PathLineTo(d);
  PathFillConvex(col, aa_trans_mask);
}

void ImDrawList::AddTriangle(const ImVec2 &a, const ImVec2 &b, const ImVec2 &c, ImU32 col, float thickness, ImU32 aa_trans_mask)
{
  PathLineTo(a);
  PathLineTo(b);
  PathLineTo(c);
  PathStroke(col, true, thickness, aa_trans_mask);
}

void ImDrawList::AddTriangleFilled(const ImVec2 &a, const ImVec2 &b, const ImVec2 &c, ImU32 col, ImU32 aa_trans_mask)
{
  PathLineTo(a);
  PathLineTo(b);
  PathLineTo(c);
  PathFillConvex(col, aa_trans_mask);
}

void ImDrawList::AddCircle(const ImVec2 &centre, float radius, ImU32 col, int num_segments, float thickness, ImU32 aa_trans_mask)
{
  const float a_max = IM_PI * 2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
  PathArcTo(centre, radius - 0.5f, 0.0f, a_max, num_segments);
  PathStroke(col, true, thickness, aa_trans_mask);
}

void ImDrawList::AddCircleFilled(const ImVec2 &centre, float radius, ImU32 col, int num_segments, ImU32 aa_trans_mask)
{
  const float a_max = IM_PI * 2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
  PathArcTo(centre, radius, 0.0f, a_max, num_segments);
  PathFillConvex(col, aa_trans_mask);
}

void ImDrawList::AddBezierCurve(const ImVec2 &pos0, const ImVec2 &cp0, const ImVec2 &cp1, const ImVec2 &pos1, ImU32 col,
  float thickness, ImU32 aa_trans_mask, int num_segments)
{
  PathLineTo(pos0);
  PathBezierCurveTo(cp0, cp1, pos1, num_segments);
  PathStroke(col, false, thickness, aa_trans_mask);
}

void ImDrawList::AddImage(ImTextureID user_texture_id, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_a, const ImVec2 &uv_b,
  ImU32 col)
{
  const bool push_texture_id = _TextureIdStack.empty() || user_texture_id != _TextureIdStack.back();
  if (push_texture_id)
    PushTextureID(user_texture_id);

  PrimReserve(6, 4);
  PrimRectUV(a, b, uv_a, uv_b, col);

  if (push_texture_id)
    PopTextureID();
}

void ImDrawList::AddImageQuad(ImTextureID user_texture_id, const ImVec2 &a, const ImVec2 &b, const ImVec2 &c, const ImVec2 &d,
  const ImVec2 &uv_a, const ImVec2 &uv_b, const ImVec2 &uv_c, const ImVec2 &uv_d, ImU32 col)
{
  const bool push_texture_id = _TextureIdStack.empty() || user_texture_id != _TextureIdStack.back();
  if (push_texture_id)
    PushTextureID(user_texture_id);

  PrimReserve(6, 4);
  PrimQuadUV(a, b, c, d, uv_a, uv_b, uv_c, uv_d, col);

  if (push_texture_id)
    PopTextureID();
}

void ImDrawList::AddImageRounded(ImTextureID user_texture_id, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_a, const ImVec2 &uv_b,
  ImU32 col, const Point4 &rounding, ImU32 aa_trans_mask)
{
  const float maxRounding = max(max(rounding.x, rounding.y), max(rounding.z, rounding.w));
  if (maxRounding <= 0.0f)
  {
    AddImage(user_texture_id, a, b, uv_a, uv_b, col);
    return;
  }

  const bool push_texture_id = _TextureIdStack.empty() || user_texture_id != _TextureIdStack.back();
  if (push_texture_id)
    PushTextureID(user_texture_id);

  int vert_start_idx = VtxBuffer.size();
  PathRect(a, b, rounding);
  PathFillConvex(col, aa_trans_mask);
  int vert_end_idx = VtxBuffer.size();
  ShadeVertsLinearUV(vert_start_idx, vert_end_idx, a, b, uv_a, uv_b, true);

  if (push_texture_id)
    PopTextureID();
}

void ImDrawList::AddImageRounded(ImTextureID user_texture_id, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_0,
  const ImVec2 &uv_dx, const ImVec2 &uv_dy, ImU32 col, const Point4 &rounding, ImU32 aa_trans_mask)
{
  const float maxRounding = max(max(rounding.x, rounding.y), max(rounding.z, rounding.w));
  if (maxRounding <= 0.0f)
  {
    AddImageQuad(user_texture_id, a, ImVec2(b.x, a.y), b, ImVec2(a.x, b.y), uv_0, uv_0 + uv_dx, uv_0 + uv_dx + uv_dy, uv_0 + uv_dy,
      col);
    return;
  }

  const bool push_texture_id = _TextureIdStack.empty() || user_texture_id != _TextureIdStack.back();
  if (push_texture_id)
    PushTextureID(user_texture_id);

  int vert_start_idx = VtxBuffer.size();
  PathRect(a, b, rounding);
  PathFillConvex(col, aa_trans_mask);
  int vert_end_idx = VtxBuffer.size();
  ShadeVertsLinearUV(vert_start_idx, vert_end_idx, a, b, uv_0, uv_dx, uv_dy);

  if (push_texture_id)
    PopTextureID();
}

//-----------------------------------------------------------------------------
// Shade functions
//-----------------------------------------------------------------------------

// Generic linear color gradient, write to RGB fields, leave A untouched.
void ImDrawList::ShadeVertsLinearColorGradientKeepAlpha(int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1,
  ImU32 col0, ImU32 col1, bool alpha_mul)
{
  ImVec2 gradient_extent = gradient_p1 - gradient_p0;
  float gradient_inv_length2 = 1.0f / ImLengthSqr(gradient_extent);
  Point4 col0f((col0 >> DAG_IM_COL32_R_SHIFT) & 0xFF, (col0 >> DAG_IM_COL32_G_SHIFT) & 0xFF, (col0 >> DAG_IM_COL32_B_SHIFT) & 0xFF,
    col0 >> DAG_IM_COL32_A_SHIFT);
  Point4 col1f((col1 >> DAG_IM_COL32_R_SHIFT) & 0xFF, (col1 >> DAG_IM_COL32_G_SHIFT) & 0xFF, (col1 >> DAG_IM_COL32_B_SHIFT) & 0xFF,
    col1 >> DAG_IM_COL32_A_SHIFT);
  ImDrawVert *vert_start = VtxBuffer.data() + vert_start_idx;
  ImDrawVert *vert_end = VtxBuffer.data() + vert_end_idx;
  for (ImDrawVert *vert = vert_start; vert < vert_end; vert++)
  {
    float d = ImDot(vert->pos - gradient_p0, gradient_extent);
    float t = ImClamp(d * gradient_inv_length2, 0.0f, 1.0f);
    Point4 col = lerp(col0f, col1f, t);
    if (alpha_mul)
      col = col * (float(vert->col >> DAG_IM_COL32_A_SHIFT) / 255.f);
    int r = clamp((int)(col.x + 0.5f), 0, 255);
    int g = clamp((int)(col.y + 0.5f), 0, 255);
    int b = clamp((int)(col.z + 0.5f), 0, 255);
    vert->col =
      (r << DAG_IM_COL32_R_SHIFT) | (g << DAG_IM_COL32_G_SHIFT) | (b << DAG_IM_COL32_B_SHIFT) | (vert->col & DAG_IM_COL32_A_MASK);
  }
}

// Distribute UV over (a, b) rectangle
void ImDrawList::ShadeVertsLinearUV(int vert_start_idx, int vert_end_idx, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_a,
  const ImVec2 &uv_b, bool clamp)
{
  const ImVec2 size = b - a;
  const ImVec2 uv_size = uv_b - uv_a;
  const ImVec2 scale = ImVec2(size.x != 0.0f ? (uv_size.x / size.x) : 0.0f, size.y != 0.0f ? (uv_size.y / size.y) : 0.0f);

  ImDrawVert *vert_start = VtxBuffer.data() + vert_start_idx;
  ImDrawVert *vert_end = VtxBuffer.data() + vert_end_idx;
  if (clamp)
  {
    const ImVec2 min = ImMin(uv_a, uv_b);
    const ImVec2 max = ImMax(uv_a, uv_b);
    for (ImDrawVert *vertex = vert_start; vertex < vert_end; ++vertex)
      vertex->uv = ImClamp(uv_a + ImMul(ImVec2(vertex->pos.x, vertex->pos.y) - a, scale), min, max);
  }
  else
  {
    for (ImDrawVert *vertex = vert_start; vertex < vert_end; ++vertex)
      vertex->uv = uv_a + ImMul(ImVec2(vertex->pos.x, vertex->pos.y) - a, scale);
  }
}

void ImDrawList::ShadeVertsLinearUV(int vert_start_idx, int vert_end_idx, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_0,
  const ImVec2 &uv_dx, const ImVec2 &uv_dy)
{
  const ImVec2 size = b - a;
  const ImVec2 step_dx = size.x != 0.0f ? uv_dx / size.x : ImVec2(0.0f, 0.0f);
  const ImVec2 step_dy = size.y != 0.0f ? uv_dy / size.y : ImVec2(0.0f, 0.0f);

  ImDrawVert *vert_start = VtxBuffer.data() + vert_start_idx;
  ImDrawVert *vert_end = VtxBuffer.data() + vert_end_idx;

  for (ImDrawVert *vertex = vert_start; vertex < vert_end; ++vertex)
    vertex->uv = uv_0 + step_dx * (vertex->pos.x - a.x) + step_dy * (vertex->pos.y - a.y);
}

}; // namespace dag_imgui_drawlist