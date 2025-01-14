// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaders.h>
#include <gui/dag_stdGuiRender.h>
void GuiVertex::resetViewTm(void) { G_ASSERT(0); }
void GuiVertex::setRotViewTm(float, float, float, float, bool) { G_ASSERT(0); }
class StdGuiRender::GuiContext *StdGuiRender::get_stdgui_context(void) { G_ASSERT_RETURN(false, nullptr); }
void StdGuiRender::GuiContext::render_line_aa(Point2 const *, int, bool, float, Point2, E3DCOLOR) { G_ASSERT(0); }
void StdGuiRender::GuiContext::render_dashed_line(const Point2, const Point2, const float, const float, const float, E3DCOLOR)
{
  G_ASSERT(0);
}
class BBox2 StdGuiRender::get_str_bbox(char const *, int, struct StdGuiFontContext const &) { G_ASSERT_RETURN(false, {}); }
void StdGuiRender::set_color(E3DCOLOR) { G_ASSERT(0); }
void StdGuiRender::set_textures(TEXTUREID, d3d::SamplerHandle, TEXTUREID, d3d::SamplerHandle) { G_ASSERT(0); }
void StdGuiRender::start_render(void) { G_ASSERT(0); }
void StdGuiRender::end_render(void) { G_ASSERT(0); }
void StdGuiRender::flush_data(void) { G_ASSERT(0); }
void StdGuiRender::render_frame(float, float, float, float, float) { G_ASSERT(0); }
void StdGuiRender::render_box(float, float, float, float) { G_ASSERT(0); }
void StdGuiRender::draw_line(real, real, real, real, real) { G_ASSERT(0); }
Point2 StdGuiRender::screen_size() { G_ASSERT_RETURN(false, {}); }
void StdGuiRender::render_quad(Point2, Point2, Point2, Point2, Point2, Point2, Point2, Point2) { G_ASSERT(0); }
void StdGuiRender::render_rect(float, float, float, float, Point2, Point2, Point2) { G_ASSERT(0); }
void StdGuiRender::set_alpha_blend(BlendMode) { G_ASSERT(0); }
BlendMode StdGuiRender::get_alpha_blend() { G_ASSERT_RETURN(false, BlendMode::NO_BLEND); }
void StdGuiRender::goto_xy(Point2) { G_ASSERT(0); }
int StdGuiRender::set_font(int, int, int) { G_ASSERT_RETURN(false, -1); }
void StdGuiRender::set_draw_str_attr(enum FontFxType, int, int, E3DCOLOR, int) { G_ASSERT(0); }
void StdGuiRender::draw_str_scaled(float, char const *, int) { G_ASSERT(0); }
void StdGuiRender::GuiContext::draw_char_u(wchar_t) { G_ASSERT(0); }
void StdGuiRender::set_shader(char const *) { G_ASSERT(0); }
void StdGuiRender::set_shader(class ShaderMaterial *) { G_ASSERT(0); }
StdGuiRender::GuiShader::GuiShader(void) { G_ASSERT(0); }
bool StdGuiRender::GuiShader::init(char const *, bool) { G_ASSERT_RETURN(false, false); }
void StdGuiRender::GuiShader::close(void) { G_ASSERT(0); }
void StdGuiRender::StdGuiShader::channels(struct CompiledShaderChannelId const *&, int &) { G_ASSERT(0); }
void StdGuiRender::StdGuiShader::link(void) { G_ASSERT(0); }
void StdGuiRender::StdGuiShader::cleanup(void) { G_ASSERT(0); }
void StdGuiRender::StdGuiShader::setStates(float const *const, struct StdGuiRender::GuiState const &,
  struct StdGuiRender::ExtState const *, bool, bool, bool, int, int)
{
  G_ASSERT(0);
}
void StdGuiRender::set_shader(struct StdGuiRender::GuiShader *) { G_ASSERT(0); }
int StdGuiRender::get_font_id(const char *) { G_ASSERT_RETURN(false, -1); }
DagorFontBinDump *StdGuiRender::get_font(int) { G_ASSERT_RETURN(false, NULL); }
bool StdGuiRender::get_font_context(StdGuiFontContext &, int, int, int, int) { G_ASSERT_RETURN(false, false); }
Point2 StdGuiRender::get_font_cell_size(const StdGuiFontContext &) { G_ASSERT_RETURN(false, Point2()); }
int StdGuiRender::get_font_caps_ht(const StdGuiFontContext &) { G_ASSERT_RETURN(false, 0); }
float StdGuiRender::get_font_line_spacing(const StdGuiFontContext &) { G_ASSERT_RETURN(false, 0.f); }
int StdGuiRender::get_font_ascent(const StdGuiFontContext &) { G_ASSERT_RETURN(false, 0); }
int StdGuiRender::get_font_descent(const StdGuiFontContext &) { G_ASSERT_RETURN(false, 0); }
void StdGuiRender::reset_shader(void) { G_ASSERT(0); }
class GuiViewPort const &StdGuiRender::get_viewport(void)
{
  static GuiViewPort p;
  G_ASSERT_RETURN(false, p);
}
bool StdGuiRender::is_render_started(void) { G_ASSERT_RETURN(false, false); }
real StdGuiRender::screen_width() { G_ASSERT_RETURN(false, -1); }
real StdGuiRender::screen_height() { G_ASSERT_RETURN(false, -1); }
class Point2 const StdGuiRender::P2::ZERO = {};
class Point2 const StdGuiRender::P2::AXIS_X = {};
class Point2 const StdGuiRender::P2::AXIS_Y = {};
void StdGuiRender::GuiContext::render_rect_t(float, float, float, float, Point2, Point2) { G_ASSERT(0); }
void StdGuiRender::GuiContext::set_textures(TEXTUREID, d3d::SamplerHandle, TEXTUREID, d3d::SamplerHandle, bool, bool) { G_ASSERT(0); }
void StdGuiRender::GuiContext::set_color(E3DCOLOR) { G_ASSERT(0); }
float StdGuiRender::GuiContext::screen_width() { G_ASSERT_RETURN(false, 0.f); }
float StdGuiRender::GuiContext::screen_height() { G_ASSERT_RETURN(false, 0.f); }
void StdGuiRender::GuiContext::render_frame(float, float, float, float, float) { G_ASSERT(0); }
void StdGuiRender::GuiContext::render_box(float, float, float, float) { G_ASSERT(0); }
void StdGuiRender::GuiContext::render_rounded_box(Point2, Point2, E3DCOLOR, E3DCOLOR, Point4, float) { G_ASSERT(0); }
void StdGuiRender::GuiContext::render_rounded_frame(Point2, Point2, E3DCOLOR, Point4, float) { G_ASSERT(0); }
void StdGuiRender::GuiContext::draw_line(float, float, float, float, float) { G_ASSERT(0); }
StdGuiRender::GuiContext::GuiContext() { G_ASSERT(0); }
StdGuiRender::GuiContext::~GuiContext() { G_ASSERT(0); }
void StdGuiRender::GuiContext::render_poly(dag::ConstSpan<Point2>, E3DCOLOR) { G_ASSERT(0); }
void StdGuiRender::GuiContext::render_inverse_poly(dag::ConstSpan<Point2>, E3DCOLOR, Point2, Point2) { G_ASSERT(0); }
void StdGuiRender::GuiContext::render_ellipse_aa(Point2, Point2, float, E3DCOLOR, E3DCOLOR) { G_ASSERT(0); }
void StdGuiRender::GuiContext::render_ellipse_aa(Point2, Point2, float, E3DCOLOR, E3DCOLOR, E3DCOLOR) { G_ASSERT(0); }
void StdGuiRender::GuiContext::render_sector_aa(Point2, Point2, Point2, float, E3DCOLOR, E3DCOLOR) { G_ASSERT(0); }
void StdGuiRender::GuiContext::render_sector_aa(Point2, Point2, Point2, float, E3DCOLOR, E3DCOLOR, E3DCOLOR) { G_ASSERT(0); }
void StdGuiRender::GuiContext::render_rectangle_aa(Point2, Point2, float, E3DCOLOR, E3DCOLOR) { G_ASSERT(0); }
void StdGuiRender::GuiContext::render_rectangle_aa(Point2, Point2, float, E3DCOLOR, E3DCOLOR, E3DCOLOR) { G_ASSERT(0); }
void StdGuiRender::GuiContext::render_smooth_round_rect(Point2, Point2, float, float, E3DCOLOR) { G_ASSERT(0); }
void StdGuiRender::GuiContext::render_rect(real, real, real, real, Point2, Point2, Point2) { G_ASSERT(0); }
void StdGuiRender::GuiContext::goto_xy(Point2) { G_ASSERT(0); }
Point2 StdGuiRender::GuiContext::get_text_pos() { G_ASSERT_RETURN(false, {}); }
void StdGuiRender::GuiContext::set_spacing(int) { G_ASSERT(0); }
void StdGuiRender::GuiContext::set_mono_width(int) { G_ASSERT(0); }
int StdGuiRender::GuiContext::get_spacing() { G_ASSERT_RETURN(false, 0); }
int StdGuiRender::GuiContext::set_font(int, int, int) { G_ASSERT_RETURN(false, 0); }
int StdGuiRender::GuiContext::set_font_ht(int) { G_ASSERT_RETURN(false, 0); }
void StdGuiRender::GuiContext::set_draw_str_attr(FontFxType, int, int, E3DCOLOR, int) { G_ASSERT(0); }
void StdGuiRender::GuiContext::draw_str_scaled(float, const char *, int) { G_ASSERT(0); }
void StdGuiRender::GuiContext::resetViewTm() { G_ASSERT(0); }
void StdGuiRender::GuiContext::setViewTm(const float[2][3]) { G_ASSERT(0); }
void StdGuiRender::GuiContext::setViewTm(Point2, Point2, Point2, bool) { G_ASSERT(0); }
void StdGuiRender::GuiContext::setRotViewTm(float, float, float, float, bool) { G_ASSERT(0); }
void StdGuiRender::GuiContext::getViewTm(float[2][3], bool) const { G_ASSERT(0); }
void GuiVertexTransform::resetViewTm() { G_ASSERT(0); }
void GuiVertexTransform::setViewTm(Point2, Point2, Point2) { G_ASSERT(0); }
void GuiVertexTransform::addViewTm(Point2, Point2, Point2) { G_ASSERT(0); }
bool StdGuiRender::GuiContext::restore_viewport() { G_ASSERT_RETURN(false, false); }
void StdGuiRender::GuiContext::set_viewport(const GuiViewPort &) { G_ASSERT(0); }
void GuiViewPort::normalize() { G_ASSERT(0); }
namespace dag_imgui_drawlist
{
ImDrawList::ImDrawListSharedData::ImDrawListSharedData() { G_ASSERT(0); }
void ImDrawList::Clear() { G_ASSERT(0); }
void ImDrawList::AddDrawCmd() { G_ASSERT(0); }
void ImDrawList::ClearFreeMemory() { G_ASSERT(0); }
} // namespace dag_imgui_drawlist
