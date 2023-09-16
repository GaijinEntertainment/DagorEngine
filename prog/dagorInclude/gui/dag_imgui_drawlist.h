//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// Copyright (c) 2014-2018 Omar Cornut
// this is port of image draw list from dear imgui, v1.63 WIP
// however, it is heavily modified already, so we keep it as 1st party
// dear imgui is(was at time of copy) MIT license
// latest version at https://github.com/ocornut/imgui

// Configuration file begin
#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <generic/dag_tab.h>
#include <3d/dag_texMgr.h>
#include <string.h> // memset, memmove, memcpy, strlen, strchr, strcpy, strcmp

namespace dag_imgui_drawlist
{

typedef int ImDrawIdx;

//---- Define constructor and implicit cast operators to convert back<>forth between your math types and ImVec2/ImVec4.

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__) && __GNUC__ >= 8
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

// Forward declarations
struct ImDrawCmd;  // A single draw command within a parent ImDrawList (generally maps to 1 GPU draw call)
struct ImDrawList; // A single draw command list (generally one per window, conceptually you may see this as a dynamic "mesh" builder)
typedef TEXTUREID ImTextureID; // User data to identify a texture (this is whatever to you want it to be! read the FAQ about
                               // ImTextureID in imgui.cpp)
#define IM_BAD_TEXTUREID BAD_TEXTUREID

// Typedefs and Enums/Flags (declared as int for compatibility with old C++, to allow using as flags and to not pollute the top of this
// file) Use your programming IDE "Go to definition" facility on the names of the center columns to find the actual flags/enum lists.
typedef int ImDrawCornerFlags; // -> enum Corner_    // Flags: for ImDrawList::AddRect*() etc.

// Scalar data types
typedef unsigned int ImU32; // 32-bit unsigned integer (often used to store packed colors)

// 2D vector (often used to store positions, sizes, etc.)
typedef Point2 ImVec2;

// Helper: Lightweight std::vector<> like class to avoid dragging dependencies (also: Windows implementation of STL with debug enabled
// is absurdly slow, so let's bypass it so our code runs fast in debug). *Important* Our implementation does NOT call C++
// constructors/destructors. This is intentional, we do not require it but you have to be mindful of that. Do _not_ use this class as a
// std::vector replacement in your code!
template <typename T>
using ImVector = Tab<T>;

// Helpers macros to generate 32-bits encoded colors
enum
{
  DAG_IM_COL32_R_SHIFT = 16,
  DAG_IM_COL32_G_SHIFT = 8,
  DAG_IM_COL32_B_SHIFT = 0,
  DAG_IM_COL32_A_SHIFT = 24,
  DAG_IM_COL32_A_MASK = 0xFF000000
};

//-----------------------------------------------------------------------------
// Draw List
// Hold a series of drawing commands. The user provides a renderer for ImDrawData which essentially contains an array of ImDrawList.
//-----------------------------------------------------------------------------

// Draw callbacks for advanced uses.
// NB- You most likely do NOT need to use draw callbacks just to create your own widget or customized UI rendering (you can poke into
// the draw list for that) Draw callback may be useful for example, A) Change your GPU render state, B) render a complex 3D scene
// inside a UI element (without an intermediate texture/render target), etc. The expected behavior from your rendering function is 'if
// (cmd.UserCallback != NULL) cmd.UserCallback(parent_list, cmd); else RenderTriangles()'
typedef void (*ImDrawCallback)(const ImDrawList *parent_list, const ImDrawCmd *cmd);

// Typically, 1 command = 1 GPU draw call (unless command is a callback)
struct ImDrawCmd
{
  Point4 ClipRect = {0, 0, 0, 0}; // Clipping rectangle (x1, y1, x2, y2). Subtract ImDrawData->DisplayPos to get clipping rectangle in
                                  // "viewport" coordinates
  ImTextureID TextureId = IM_BAD_TEXTUREID; // User-provided texture ID. Set by user in ImfontAtlas::SetTexID() for fonts or passed to
                                            // Image*() functions. Ignore if never using images or multiple fonts atlas.
  ImDrawCallback UserCallback = 0; // If != NULL, call the function instead of rendering the vertices. clip_rect and texture_id will be
                                   // set normally.
  void *UserCallbackData = 0;      // The draw callback code can access this.
  unsigned int ElemCount = 0;      // Number of indices (multiple of 3) to be rendered as triangles. Vertices are stored in the callee
                                   // ImDrawList's vtx_buffer[] array, indices in idx_buffer[].
};

// Vertex layout
struct ImDrawVert
{
  ImVec2 pos;
  ImVec2 uv;
  ImU32 col;
};


// Draw command list
// This is the low-level list of polygons that ImGui functions are filling. At the end of the frame, all command lists are passed to
// your ImGuiIO::RenderDrawListFn function for rendering. Each ImGui window contains its own ImDrawList. You can use
// ImGui::GetWindowDrawList() to access the current window draw list and draw custom primitives. You can interleave normal ImGui::
// calls and adding primitives to the current draw list. All positions are generally in pixel coordinates (top-left at (0,0),
// bottom-right at io.DisplaySize), but you are totally free to apply whatever transformation matrix to want to the data (if you apply
// such transformation you'll want to apply it to ClipRect as well) Important: Primitives are always added to the list and not culled
// (culling is done at higher-level by ImGui:: functions), if you use this API a lot consider coarse culling your drawn objects.
struct ImDrawList
{
  // This is what you have to render
  ImVector<ImDrawCmd> CmdBuffer;  // Draw commands. Typically 1 command = 1 GPU draw call, unless the command is a callback.
  ImVector<ImDrawIdx> IdxBuffer;  // Index buffer. Each command consume ImDrawCmd::ElemCount of those
  ImVector<ImDrawVert> VtxBuffer; // Vertex buffer.

  // If you want to create ImDrawList instances, pass them ImGui::GetDrawListSharedData() or create and use your own
  // ImDrawListSharedData (so you can use ImDrawList without ImGui)
  ImDrawList()
  {
    Clear();
    AddDrawCmd();
  }
  ~ImDrawList() { ClearFreeMemory(); }
  //! Render-level scissoring. This is passed down to your render function but not used for CPU-side coarse clipping. Prefer using
  //! higher-level ImGui::PushClipRect() to affect logic (hit-testing and widget culling)
  void PushClipRect(ImVec2 clip_rect_min, ImVec2 clip_rect_max, bool intersect_with_current_clip_rect = false);
  void PushClipRectFullScreen();
  void PopClipRect();
  void PushTextureID(ImTextureID texture_id);
  void PopTextureID();

  void ShadeVertsLinearColorGradientKeepAlpha(int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImU32 col0,
    ImU32 col1, bool multiply_by_alpha);
  void ShadeVertsLinearUV(int vert_start_idx, int vert_end_idx, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_a,
    const ImVec2 &uv_b, bool clamp);
  void ShadeVertsLinearUV(int vert_start_idx, int vert_end_idx, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_0,
    const ImVec2 &uv_dx, const ImVec2 &uv_dy);
  // Primitives
  void AddLine(const ImVec2 &a, const ImVec2 &b, ImU32 col, float thickness, ImU32 aa_trans_mask);
  //! a: upper-left, b: lower-right
  void AddRect(const ImVec2 &a, const ImVec2 &b, ImU32 col, const Point4 &rounding, float thickness, ImU32 aa_trans_mask);
  //! a: upper-left, b: lower-right
  void AddRectFilledBordered(const ImVec2 &a, const ImVec2 &b, ImU32 col, ImU32 fillcol, const Point4 &rounding, float thickness,
    ImU32 aa_trans_mask);
  //! a: upper-left, b: lower-right
  void AddRectFilled(const ImVec2 &a, const ImVec2 &b, ImU32 col, const Point4 &rounding, ImU32 aa_trans_mask);
  void AddRectFilledMultiColor(const ImVec2 &a, const ImVec2 &b, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right,
    ImU32 col_bot_left);
  void AddQuad(const ImVec2 &a, const ImVec2 &b, const ImVec2 &c, const ImVec2 &d, ImU32 col, float thickness, ImU32 aa_trans_mask);
  void AddQuadFilled(const ImVec2 &a, const ImVec2 &b, const ImVec2 &c, const ImVec2 &d, ImU32 col, ImU32 aa_trans_mask);
  void AddTriangle(const ImVec2 &a, const ImVec2 &b, const ImVec2 &c, ImU32 col, float thickness, ImU32 aa_trans_mask);
  void AddTriangleFilled(const ImVec2 &a, const ImVec2 &b, const ImVec2 &c, ImU32 col, ImU32 aa_trans_mask);
  void AddCircle(const ImVec2 &centre, float radius, ImU32 col, int num_segments, float thickness, ImU32 aa_trans_mask);
  void AddCircleFilled(const ImVec2 &centre, float radius, ImU32 col, int num_segments, ImU32 aa_trans_mask);
  void AddImage(ImTextureID user_texture_id, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_a, const ImVec2 &uv_b, ImU32 col);
  void AddImageQuad(ImTextureID user_texture_id, const ImVec2 &a, const ImVec2 &b, const ImVec2 &c, const ImVec2 &d,
    const ImVec2 &uv_a, const ImVec2 &uv_b, const ImVec2 &uv_c, const ImVec2 &uv_d, ImU32 col);
  void AddImageRounded(ImTextureID user_texture_id, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_a, const ImVec2 &uv_b,
    ImU32 col, const Point4 &rounding, ImU32 aa_trans_mask);
  void AddImageRounded(ImTextureID user_texture_id, const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_0, const ImVec2 &uv_dx,
    const ImVec2 &uv_dy, ImU32 col, const Point4 &rounding, ImU32 aa_trans_mask);
  void AddPolyline(const ImVec2 *points, const int num_pts, ImU32 col, ImU32 fillcol, bool closed, float thickness,
    ImU32 aa_trans_mask);
  //! Note: Anti-aliased filling requires points to be in clockwise order.
  void AddConvexPolyFilled(const ImVec2 *points, const int num_points, ImU32 col, ImU32 aa_trans_mask);
  void AddBezierCurve(const ImVec2 &pos0, const ImVec2 &cp0, const ImVec2 &cp1, const ImVec2 &pos1, ImU32 col, float thickness,
    ImU32 aa_trans_mask, int num_segments = 0);

  // Stateful path API, add points then finish with PathFillConvex() or PathStroke()
  inline void PathClear() { _Path.resize(0); }
  inline void PathLineTo(const ImVec2 &pos) { _Path.push_back(pos); }
  inline void PathLineToMergeDuplicate(const ImVec2 &pos)
  {
    if (_Path.size() == 0 || memcmp(&_Path.back(), &pos, 8) != 0)
      _Path.push_back(pos);
  }
  //! Note: Anti-aliased filling requires points to be in clockwise order.
  inline void PathFillConvex(ImU32 col, ImU32 aa_trans_mask)
  {
    AddConvexPolyFilled(_Path.data(), _Path.size(), col, aa_trans_mask);
    PathClear();
  }
  inline void PathStroke(ImU32 col, bool closed, float thickness, ImU32 aa_trans_mask)
  {
    AddPolyline(_Path.data(), _Path.size(), col, 0, closed, thickness, aa_trans_mask);
    PathClear();
  }

  inline void PathStrokeFilled(ImU32 col, ImU32 fillcol, float thickness, ImU32 aa_trans_mask)
  {
    AddPolyline(_Path.data(), _Path.size(), col, fillcol, true, thickness, aa_trans_mask);
    PathClear();
  }
  void PathArcTo(const ImVec2 &centre, float radius, float a_min, float a_max, int num_segments = 10);
  //! Use precomputed angles for a 12 steps circle
  void PathArcToFast(const ImVec2 &centre, float radius, int a_min_of_12, int a_max_of_12);
  void PathBezierCurveTo(const ImVec2 &p1, const ImVec2 &p2, const ImVec2 &p3, int num_segments = 0);
  void PathRect(const ImVec2 &rect_min, const ImVec2 &rect_max, const Point4 &rounding);

  // Advanced
  //! Your rendering function must check for 'UserCallback' in ImDrawCmd and call the function instead of rendering triangles.
  void AddCallback(ImDrawCallback callback, void *callback_data);
  //! This is useful if you need to forcefully create a new draw call (to allow for dependent rendering / blending). Otherwise
  //! primitives are merged into the same draw-call as much as possible
  void AddDrawCmd();

  // Internal helpers
  // NB: all primitives needs to be reserved via PrimReserve() beforehand!
  void Clear();

protected:
  void ClearFreeMemory();
  void PrimReserve(int idx_count, int vtx_count);
  void PrimRect(const ImVec2 &a, const ImVec2 &b, ImU32 col); // Axis aligned rectangle (composed of two triangles)
  void PrimRectUV(const ImVec2 &a, const ImVec2 &b, const ImVec2 &uv_a, const ImVec2 &uv_b, ImU32 col);
  void PrimQuadUV(const ImVec2 &a, const ImVec2 &b, const ImVec2 &c, const ImVec2 &d, const ImVec2 &uv_a, const ImVec2 &uv_b,
    const ImVec2 &uv_c, const ImVec2 &uv_d, ImU32 col);
  inline void PrimWriteVtx(const ImVec2 &pos, const ImVec2 &uv, ImU32 col)
  {
    _VtxWritePtr->pos = pos;
    _VtxWritePtr->uv = uv;
    _VtxWritePtr->col = col;
    _VtxWritePtr++;
    _VtxCurrentIdx++;
  }
  inline void PrimWriteIdx(ImDrawIdx idx)
  {
    *_IdxWritePtr = idx;
    _IdxWritePtr++;
  }
  inline void PrimVtx(const ImVec2 &pos, const ImVec2 &uv, ImU32 col)
  {
    PrimWriteIdx((ImDrawIdx)_VtxCurrentIdx);
    PrimWriteVtx(pos, uv, col);
  }
  void UpdateClipRect();
  void UpdateTextureID();

  // [Internal, used while building lists]
  struct ImDrawListSharedData
  {
    ImVec2 TexUvWhitePixel; // UV of white pixel in the atlas
    float CurveTessellationTol;
    Point4 ClipRectFullscreen; // Value for PushClipRectFullscreen()

    // Const data
    // FIXME: Bake rounded corners fill/borders in atlas
    ImVec2 CircleVtx12[12];

    ImDrawListSharedData();
  } _Data;
  // const ImDrawListSharedData* _Data;          // Pointer to shared draw data (you can use ImGui::GetDrawListSharedData() to get the
  // one from current ImGui context)
  unsigned int _VtxCurrentIdx; // [Internal] == VtxBuffer.size()
  ImDrawVert *_VtxWritePtr; // [Internal] point within VtxBuffer.data() after each add command (to avoid using the ImVector<> operators
                            // too much)
  ImDrawIdx *_IdxWritePtr;  // [Internal] point within IdxBuffer.data() after each add command (to avoid using the ImVector<> operators
                            // too much)
  ImVector<Point4> _ClipRectStack;       // [Internal]
  ImVector<ImTextureID> _TextureIdStack; // [Internal]
  ImVector<ImVec2> _Path;                // [Internal] current path building
};

}; // namespace dag_imgui_drawlist
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__) && __GNUC__ >= 8
#pragma GCC diagnostic pop
#endif
