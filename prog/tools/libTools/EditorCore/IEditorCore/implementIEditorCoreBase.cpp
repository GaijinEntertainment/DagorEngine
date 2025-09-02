// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "implementIEditorCore.h"
#include <EditorCore/captureCursor.h>
#include <EditorCore/ec_ObjectCreator.h>
#include <EditorCore/ec_wndGlobal.h>

#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>

#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_ioUtils.h>

#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_DynamicShadersBuffer.h>
#include <shaders/dag_DebugPrimitivesVbuffer.h>
#include <shaders/dag_renderScene.h>

#include <coolConsole/coolConsole.h>

#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/staticGeom/geomObject.h>

#include <libTools/util/hash.h>
#include <libTools/util/fileUtils.h>
#include <libTools/util/colorUtil.h>

#include <libTools/dagFileRW/dagUtil.h>
#include <libTools/renderUtil/dynRenderBuf.h>
#include <libTools/ObjCreator3d/objCreator3d.h>

#include <debug/dag_debug3d.h>
#include <debug/dag_fatal.h>

#include <memory/dag_memStat.h>

#include <math/dag_boundingSphere.h>

#include <scene/dag_loadLevel.h>
#include <scene/dag_renderSceneMgr.h>
#include <scene/dag_occlusionMap.h>

#include <streaming/dag_streamingMgr.h>
#include <streaming/dag_streamingCtrl.h>

#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_progGlobals.h>

#include <image/dag_tga.h>

#include <fx/dag_hdrRender.h>

#include <gui/dag_stdGuiRender.h>


#define CREATE_OBJECT(type, alloc)             \
  type *ret = NULL;                            \
  ret = (alloc) ? new (alloc) type : new type; \
  G_ASSERT(ret);                               \
  return ret;


#define CREATE_OBJECT_1PARAM(type, param, alloc)             \
  type *ret = NULL;                                          \
  ret = (alloc) ? new (alloc) type(param) : new type(param); \
  G_ASSERT(ret);                                             \
  return ret;


#define CREATE_OBJECT_2PARAM(type, param1, param2, alloc)                      \
  type *ret = NULL;                                                            \
  ret = (alloc) ? new (alloc) type(param1, param2) : new type(param1, param2); \
  G_ASSERT(ret);                                                               \
  return ret;


#define CREATE_OBJECT_3PARAM(type, param1, param2, param3, alloc)                              \
  type *ret = NULL;                                                                            \
  ret = (alloc) ? new (alloc) type(param1, param2, param3) : new type(param1, param2, param3); \
  G_ASSERT(ret);                                                                               \
  return ret;


//==================================================================================================
// EcRender
//==================================================================================================
void EcRender::fillD3dInterfaceTable(D3dInterfaceTable &d3dit) const { d3d::fill_interface_table(d3dit); }

//==================================================================================================
DagorCurView &EcRender::curView() const { return ::grs_cur_view; }


//==================================================================================================
void EcRender::startLinesRender(bool test_z, bool write_z, bool z_func_less) const
{
  ::begin_draw_cached_debug_lines(test_z, write_z, z_func_less);
}


//==================================================================================================
void EcRender::setLinesTm(const TMatrix &tm) const { ::set_cached_debug_lines_wtm(tm); }


//==================================================================================================
void EcRender::endLinesRender() const { ::end_draw_cached_debug_lines(); }


//==================================================================================================
void EcRender::renderTextFmt(real x, real y, E3DCOLOR color, const char *format, const DagorSafeArg *arg, int anum) const
{
  StdGuiRender::goto_xy(x, y);
  StdGuiRender::set_color(color);

  StdGuiRender::draw_strv(1.0f, format, arg, anum);
}


//==================================================================================================
BBox2 EcRender::getTextBBox(const char *str, int len) const { return StdGuiRender::get_str_bbox(str, len); }


//==================================================================================================
int EcRender::setTextFont(int font_id, int font_kern) const { return StdGuiRender::set_font(font_id, font_kern); }


//==================================================================================================
void EcRender::getFontAscentAndDescent(int &ascent, int &descent) const
{
  ascent = StdGuiRender::get_font_ascent();
  descent = StdGuiRender::get_font_descent();
}


//==================================================================================================
void EcRender::drawSolidRectangle(real left, real top, real right, real bottom, E3DCOLOR color) const
{
  StdGuiRender::set_color(color);
  StdGuiRender::render_box(left, top, right, bottom);
}


//==================================================================================================
void EcRender::renderLine(const Point3 &p0, const Point3 &p1, E3DCOLOR color) const { ::draw_cached_debug_line(p0, p1, color); }


//==================================================================================================
void EcRender::renderLine(const Point3 *points, int count, E3DCOLOR color) const { ::draw_cached_debug_line(points, count, color); }


//==================================================================================================
void EcRender::renderBox(const BBox3 &box, E3DCOLOR color) const { ::draw_cached_debug_box(box, color); }


//==================================================================================================
void EcRender::renderBox(const Point3 &p, const Point3 &ax, const Point3 &ay, const Point3 &az, E3DCOLOR color) const
{
  ::draw_cached_debug_box(p, ax, ay, az, color);
}


//==================================================================================================
void EcRender::renderSphere(const Point3 &center, real rad, E3DCOLOR col, int segs) const
{
  ::draw_cached_debug_sphere(center, rad, col, segs);
}


//==================================================================================================
void EcRender::renderCircle(const Point3 &center, const Point3 &ax1, const Point3 &ax2, real radius, E3DCOLOR col, int segs) const
{
  ::draw_cached_debug_circle(center, ax1, ax2, radius, col, segs);
}


//==================================================================================================
void EcRender::renderXZCircle(const Point3 &center, real radius, E3DCOLOR col, int segs) const
{
  ::draw_cached_debug_xz_circle(center, radius, col, segs);
}


//==================================================================================================
void EcRender::renderCapsuleW(const Capsule &cap, E3DCOLOR c) const { ::draw_debug_capsule_w(cap, c); }


//==================================================================================================
void EcRender::renderCylinder(const TMatrix &tm, float rad, float height, E3DCOLOR c) const
{
  ::draw_cached_debug_cylinder(tm, rad, height, c);
}


DebugPrimitivesVbuffer *EcRender::newDebugPrimitivesVbuffer(const char *name, IMemAlloc *alloc) const
{
  DebugPrimitivesVbuffer *ret = NULL;
  ret = alloc ? new (alloc) DebugPrimitivesVbuffer(name) : new DebugPrimitivesVbuffer(name);

  G_ASSERT(ret);
  return ret;
}


void EcRender::deleteDebugPrimitivesVbuffer(DebugPrimitivesVbuffer *&vbuf) const { del_it(vbuf); }


void EcRender::beginDebugLinesCacheToVbuffer(DebugPrimitivesVbuffer &vbuf) const { vbuf.beginCache(); }


void EcRender::endDebugLinesCacheToVbuffer(DebugPrimitivesVbuffer &vbuf) const { vbuf.endCache(); }


void EcRender::invalidateDebugPrimitivesVbuffer(DebugPrimitivesVbuffer &vbuf) const { vbuf.invalidate(); }


void EcRender::renderLinesFromVbuffer(DebugPrimitivesVbuffer &vbuf) const { vbuf.render(); }

void EcRender::renderLinesFromVbuffer(DebugPrimitivesVbuffer &vbuf, E3DCOLOR c) const { vbuf.renderOverrideColor(c); }

void EcRender::renderLinesFromVbuffer(DebugPrimitivesVbuffer &vbuf, bool z_test, bool z_write, bool z_func_less,
  Color4 color_multiplier) const
{
  vbuf.renderEx(z_test, z_write, z_func_less, color_multiplier);
}

void EcRender::addLineToVbuffer(DebugPrimitivesVbuffer &vbuf, const Point3 &p0, const Point3 &p1, E3DCOLOR c) const
{
  vbuf.addLine(p0, p1, c);
}

void EcRender::addHatchedBoxToVbuffer(DebugPrimitivesVbuffer &vbuf, const TMatrix &box_tm, float hatching_step, E3DCOLOR color) const
{
  vbuf.addBox(box_tm.getcol(3), box_tm.getcol(0), box_tm.getcol(1), box_tm.getcol(2), color);
  // for each direction (0-3)
  const Point3 center = box_tm * Point3(0.5f, 0.5f, 0.5f);
  for (int i = 0; i < 3; ++i)
  {
    // Choose 2 other axes as a basis
    const Point3 &leftBasis = box_tm.getcol((i + 1) % 3);
    const Point3 &upBasis = box_tm.getcol((i + 2) % 3);
    const float leftLen = length(leftBasis);
    const float upLen = length(upBasis);
    // for each face
    for (int dir = -1; dir <= 1; dir += 2)
    {
      const Point3 origin = center + dir * (box_tm.getcol(i) + dir * leftBasis + upBasis) * 0.5f;
      float y = hatching_step; // vert intercept
      while (true)
      {
        // right side
        float rx = leftLen;
        float ry = y - leftLen; // horz intercept for vertical line
        // constrain by the rectangle
        rx += min(ry, 0.f);
        ry -= min(ry, 0.f);

        // left side
        float lx = 0.f;
        float ly = y;
        // constrain by the rectangle
        lx += max(ly - upLen, 0.f);
        ly -= max(ly - upLen, 0.f);

        if (lx > rx)
          break;

        vbuf.addLine(origin - (dir * leftBasis * lx / leftLen + upBasis * ly / upLen) * dir,
          origin - (dir * leftBasis * rx / leftLen + upBasis * ry / upLen) * dir, color);

        y += hatching_step;
      }
    }
  }
}

void EcRender::addBoxToVbuffer(DebugPrimitivesVbuffer &vbuf, const Point3 &p, const Point3 &ax, const Point3 &ay, const Point3 &az,
  E3DCOLOR color) const
{
  vbuf.addBox(p, ax, ay, az, color);
}

void EcRender::addSolidBoxToVbuffer(DebugPrimitivesVbuffer &vbuf, const Point3 &p, const Point3 &ax, const Point3 &ay,
  const Point3 &az, E3DCOLOR color) const
{
  vbuf.addSolidBox(p, ax, ay, az, color);
}

void EcRender::addTriangleToVbuffer(DebugPrimitivesVbuffer &vbuf, const Point3 p[3], E3DCOLOR color) const
{
  vbuf.addTriangle(p, color);
}


void EcRender::addBoxToVbuffer(DebugPrimitivesVbuffer &vbuf, const BBox3 &box, E3DCOLOR color) const { vbuf.addBox(box, color); }


bool EcRender::isLinesVbufferValid(DebugPrimitivesVbuffer &vbuf) const { return vbuf.isValid(); }


void EcRender::setVbufferTm(DebugPrimitivesVbuffer &vbuf, const TMatrix &tm) const { vbuf.setTm(tm); }


//==================================================================================================
DynamicShadersBuffer *EcRender::newDynamicShadersBuffer(IMemAlloc *alloc) const { CREATE_OBJECT(DynamicShadersBuffer, alloc); }


//==================================================================================================
DynamicShadersBuffer *EcRender::newDynamicShadersBuffer(CompiledShaderChannelId *channels, int channel_count, int max_verts,
  int max_faces, IMemAlloc *alloc) const
{
  DynamicShadersBuffer *ret = NULL;

  ret = alloc ? new (alloc) DynamicShadersBuffer(channels, channel_count, max_verts, max_faces)
              : new DynamicShadersBuffer(channels, channel_count, max_verts, max_faces);

  G_ASSERT(ret);

  return ret;
}


//==================================================================================================
void EcRender::deleteDynamicShadersBuffer(DynamicShadersBuffer *&buf) const { del_it(buf); }


//==================================================================================================
void EcRender::dynShaderBufferSetCurrentShader(DynamicShadersBuffer &buf, ShaderElement *shader) const
{
  buf.setCurrentShader(shader);
}


//==================================================================================================
void EcRender::dynShaderBufferAddFaces(DynamicShadersBuffer &buf, const void *vertex_data, int num_verts, const int *indices,
  int num_faces) const
{
  buf.addFaces(vertex_data, num_verts, indices, num_faces);
}


//==================================================================================================
void EcRender::dynShaderBufferFlush(DynamicShadersBuffer &buf) const { buf.flush(); }


//==================================================================================================
DynRenderBuffer *EcRender::newDynRenderBuffer(const char *class_name, IMemAlloc *alloc) const
{
  CREATE_OBJECT_1PARAM(DynRenderBuffer, class_name, alloc);
}


//==================================================================================================
void EcRender::deleteDynRenderBuffer(DynRenderBuffer *&buf) const { del_it(buf); }


//==================================================================================================
void EcRender::dynRenderBufferClearBuf(DynRenderBuffer &buf) const { buf.clearBuf(); }


//==================================================================================================
void EcRender::dynRenderBufferFlush(DynRenderBuffer &buf) const { buf.flush(); }


//==================================================================================================
void EcRender::dynRenderBufferFlushToBuffer(DynRenderBuffer &buf, TEXTUREID tid) const { buf.flushToBuffer(tid); }


//==================================================================================================
void EcRender::dynRenderBufferDrawQuad(DynRenderBuffer &buf, const Point3 &p0, const Point3 &p1, const Point3 &p2, const Point3 &p3,
  E3DCOLOR color, float u, float v) const
{
  buf.drawQuad(p0, p1, p2, p3, color, u, v);
}

//==================================================================================================
void *EcRender::dynRenderBufferDrawNetSurface(DynRenderBuffer &buf, int w, int h) const { return buf.drawNetSurface(w, h); }

//==================================================================================================
void EcRender::dynRenderBufferDrawLine(DynRenderBuffer &buf, const Point3 &from, const Point3 &to, real width, E3DCOLOR color) const
{
  buf.drawLine(from, to, width, color);
}


//==================================================================================================
void EcRender::dynRenderBufferDrawSquare(DynRenderBuffer &buf, const Point3 &p, real radius, E3DCOLOR color) const
{
  buf.drawSquare(p, radius, color);
}


//==================================================================================================
void EcRender::dynRenderBufferDrawBox(DynRenderBuffer &buf, const TMatrix &tm, E3DCOLOR color) const { buf.drawBox(tm, color); }


//==================================================================================================
TEXTUREID EcRender::addManagedTexture(const char *name) const { return ::add_managed_texture(name); }

//==================================================================================================
TEXTUREID EcRender::registerManagedTex(const char *name, BaseTexture *basetex) const { return ::register_managed_tex(name, basetex); }


//==================================================================================================
BaseTexture *EcRender::acquireManagedTex(TEXTUREID id) const { return ::acquire_managed_tex(id); }


//==================================================================================================
void EcRender::releaseManagedResVerified(TEXTUREID &id, D3dResource *cmp) const { ::release_managed_tex_verified(id, cmp); }


//==================================================================================================
void EcRender::releaseManagedTex(TEXTUREID id) const { ::release_managed_tex(id); }


//==================================================================================================
E3DCOLOR EcRender::normalizeColor4(const Color4 &color, real &bright) const { return ::normalize_color4(color, bright); }


//==================================================================================================
int EcRender::getHdrMode() const { return ::hdr_render_mode; }


//==================================================================================================
// EcConsole
//==================================================================================================
void EcConsole::startProgress(CoolConsole &con) const { con.startProgress(); }


//==================================================================================================
void EcConsole::endProgress(CoolConsole &con) const { con.endProgress(); }


//==================================================================================================
void EcConsole::addMessageFmt(CoolConsole &con, ILogWriter::MessageType type, const char *fmt, const DagorSafeArg *arg, int anum) const
{
  con.addMessageFmt(type, fmt, arg, anum);
}


//==================================================================================================
void EcConsole::showConsole(CoolConsole &con, bool activate) const { con.showConsole(activate); }


//==================================================================================================
void EcConsole::hideConsole(const CoolConsole &con) const { const_cast<CoolConsole &>(con).hideConsole(); }


//==================================================================================================
bool EcConsole::registerCommand(CoolConsole &con, const char *cmd, IConsoleCmd *handler) const
{
  return con.registerCommand(cmd, handler);
}


//==================================================================================================
bool EcConsole::unregisterCommand(CoolConsole &con, const char *cmd, IConsoleCmd *handler) const
{
  return con.unregisterCommand(cmd, handler);
}
