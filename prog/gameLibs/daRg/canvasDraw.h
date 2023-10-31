#pragma once

#include <gui/dag_stdGuiRender.h>
#include <sqrat.h>

namespace darg
{

struct RenderCanvasContext
{
  StdGuiRender::GuiContext *ctx = nullptr;
  float lineWidth = 0;
  float lineAlign = 0;
  Point2 offset = Point2(0, 0);
  Point2 scale = Point2(1, 1);
  bool midColorUsed = false;
  E3DCOLOR color = 0xFFFFFFFF;
  E3DCOLOR midColor = 0xFFFFFFFF;
  E3DCOLOR fillColor = 0xFFFFFFFF;
  E3DCOLOR currentColor = 0xFFFFFFFF;
  E3DCOLOR currentFillColor = 0xFFFFFFFF;
  E3DCOLOR currentMidColor = 0xFFFFFFFF;
  float renderStateOpacity = 1.0f;

  void renderLine(const Sqrat::Array &cmd, const Point2 &line_indent) const;
  void renderEllipse(const Sqrat::Array &cmd) const;
  void renderSector(const Sqrat::Array &cmd) const;
  void renderRectangle(const Sqrat::Array &cmd) const;
  void renderFillPoly(const Sqrat::Array &cmd) const;
  void renderFillInversePoly(const Sqrat::Array &cmd) const;
  void renderLineDashed(const Sqrat::Array &cmd) const;
  void renderQuads(const Sqrat::Array &cmd) const;

  void bind_delegate_to_handle(HSQUIRRELVM vm);

  static void bind_script_api(Sqrat::Table &api);
  static SQUserPointer get_type_tag();
  static int type_tag_dummy;
};


} // namespace darg
