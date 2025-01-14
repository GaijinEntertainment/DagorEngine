// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "stdRendObj.h"
#include <memory/dag_framemem.h>
#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_helpers.h>
#include <daRg/dag_picture.h>
#include <daRg/dag_sceneRender.h>
#include <generic/dag_carray.h>
#include <math/dag_mathUtils.h>
#include <math/dag_polyUtils.h>
#include <sqext.h>
#include "guiScene.h"
#include "textLayout.h"
#include "scriptUtil.h"
#include "textUtil.h"
#include "profiler.h"
#include <perfMon/dag_statDrv.h>
#include "dargDebugUtils.h"
#include "robjMask.h"
#include "robjDasCanvas.h"
#include "canvasDraw.h"

#include <videoPlayer/dag_videoPlayer.h>
#include "yuvRenderer.h"

#include <gui/dag_visualLog.h>

#include <utf8/utf8.h>
#include <osApiWrappers/dag_unicode.h>
#include <supp/dag_alloca.h>

#include "behaviors/bhvTextAreaEdit.h"
#include "editableText.h"


namespace darg
{

static const E3DCOLOR defFontFxColor(120, 120, 120, 120);
static const wchar_t ellipsisUtf16[] = {0x2026, 0};


static KeepAspectMode resolve_keep_aspect(const Sqrat::Object &val)
{
  SQObjectType tp = val.GetType();
  HSQOBJECT hobj = val.GetObject();
  if (tp == OT_BOOL)
    return sq_objtobool(&hobj) ? KEEP_ASPECT_FIT : KEEP_ASPECT_NONE;
  if (tp == OT_INTEGER)
  {
    SQInteger intVal = sq_objtointeger(&hobj);
    if (intVal == KEEP_ASPECT_FIT)
      return KEEP_ASPECT_FIT;
    if (intVal == KEEP_ASPECT_FILL)
      return KEEP_ASPECT_FILL;
  }
  return KEEP_ASPECT_NONE;
}

const E3DCOLOR RobjParamsColorOnly::defColor = E3DCOLOR(255, 255, 255, 255);

bool RobjParamsColorOnly::load(const Element *elem, E3DCOLOR def_color)
{
  color = elem->props.getColor(elem->csk->color, def_color);
  brightness = elem->props.getFloat(elem->csk->brightness, 1.0f);
  return true;
}

bool RobjParamsColorOnly::load(const Element *elem) { return load(elem, RobjParamsColorOnly::defColor); }


bool RobjParamsColorOnly::getAnimColor(AnimProp prop, E3DCOLOR **ptr)
{
  if (prop == AP_COLOR)
  {
    *ptr = &color;
    return true;
  }
  return false;
}


bool RobjParamsColorOnly::getAnimFloat(AnimProp prop, float **ptr)
{
  if (prop == AP_BRIGHTNESS)
  {
    *ptr = &brightness;
    return true;
  }
  return false;
}


void RenderObjectSolid::render(StdGuiRender::GuiContext &ctx, const Element *, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  RobjParamsColorOnly *params = static_cast<RobjParamsColorOnly *>(rdata->params);
  G_ASSERT(params);
  if (!params)
    return;

  E3DCOLOR color = color_apply_mods(params->color, render_state.opacity, params->brightness);

  ctx.set_color(color);
  ctx.reset_textures();
  ctx.render_rect(rdata->pos, rdata->pos + rdata->size);
}


void RenderObjectDebug::render(StdGuiRender::GuiContext &ctx, const Element *, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  RobjParamsColorOnly *params = static_cast<RobjParamsColorOnly *>(rdata->params);
  G_ASSERT(params);
  if (!params)
    return;

  E3DCOLOR color = color_apply_mods(params->color, render_state.opacity, params->brightness);

  ctx.set_color(color);
  ctx.reset_textures();

  Point2 lt = rdata->pos;
  Point2 rb = lt + rdata->size;
  ctx.render_frame(lt.x, lt.y, rb.x, rb.y, 1);
  ctx.draw_line(lt.x, lt.y, rb.x, rb.y);
  ctx.draw_line(lt.x, rb.y, rb.x, lt.y);
}


bool RobjParamsText::load(const Element *elem)
{
  const Properties &props = elem->props;

  color = props.getColor(elem->csk->color, elem->etree->guiScene->config.defTextColor);
  brightness = props.getFloat(elem->csk->brightness, 1.0f);

  passChar = 0;
  Sqrat::Object password = props.getObject(elem->csk->password);
  if (password.GetType() == OT_BOOL)
    passChar = L'*';
  else if (password.GetType() == OT_INTEGER)
    passChar = char(password.Cast<SQInteger>());
  else if (password.GetType() == OT_STRING)
  {
    const char *str = password.GetVar<const SQChar *>().value;
    Tab<wchar_t> buf(framemem_ptr());
    buf.resize(3);
    utf8_to_wcs_ex(str, get_next_char_size(str), buf.data(), buf.size());
    passChar = buf[0] > 32 ? buf[0] : L'*';
  }

  fontId = props.getFontId();
  fontHt = (int)floorf(props.getFontSize() + 0.5f);
  spacing = props.getInt(elem->csk->spacing, 0);
  monoWidth = font_mono_width_from_sq(fontId, fontHt, props.getObject(elem->csk->monoWidth));

  fontFx = (FontFxType)props.getInt(elem->csk->fontFx, FFT_NONE);
  fxColor = props.getColor(elem->csk->fontFxColor, defFontFxColor);
  fxFactor = props.getInt(elem->csk->fontFxFactor, 48);
  fxOffsX = props.getInt(elem->csk->fontFxOffsX, 0);
  fxOffsY = props.getInt(elem->csk->fontFxOffsY, 0);

  fontTex = props.getPicture(elem->csk->fontTex);
  fontTexSu = props.getInt(elem->csk->fontTexSu, 32);
  fontTexSv = props.getInt(elem->csk->fontTexSv, 32);
  fontTexBov = props.getInt(elem->csk->fontTexBov, 0);

  overflowX = (TextOverflow)props.getInt(elem->csk->textOverflowX, TOVERFLOW_CLIP);
  ellipsis = props.getBool(elem->csk->ellipsis, true);
  discard_text_cache(elem->robjParams);
  return true;
}


bool RobjParamsText::getAnimColor(AnimProp prop, E3DCOLOR **ptr)
{
  if (prop == AP_COLOR)
  {
    *ptr = &color;
    return true;
  }
  return false;
}


bool RobjParamsText::getAnimFloat(AnimProp prop, float **ptr)
{
  if (prop == AP_BRIGHTNESS)
  {
    *ptr = &brightness;
    return true;
  }
  return false;
}


bool RobjParamsTextArea::load(const Element *elem)
{
  RobjParamsColorOnly::load(elem, elem->etree->guiScene->config.defTextColor);

  const Properties &props = elem->props;

  overflowY = props.getInt<TextOverflow>(elem->csk->textOverflowY, TOVERFLOW_CLIP);
  fontFx = props.getInt<FontFxType>(elem->csk->fontFx, FFT_NONE);
  fxColor = props.getColor(elem->csk->fontFxColor, defFontFxColor);
  fxFactor = props.getInt(elem->csk->fontFxFactor, 48);
  fxOffsX = props.getInt(elem->csk->fontFxOffsX, 0);
  fxOffsY = props.getInt(elem->csk->fontFxOffsY, 0);

  lowLineCount = props.getInt(elem->csk->lowLineCount, 0);
  lowLineCountAlign = props.getInt<ElemAlign>(elem->csk->lowLineCountAlign, PLACE_DEFAULT);

  return true;
}


void RenderObjectText::render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  AutoProfileScope pfl(GuiScene::get_from_elem(elem)->getProfiler(), M_RENDER_TEXT);

  RobjParamsText *params = static_cast<RobjParamsText *>(rdata->params);
  G_ASSERT(params);
  if (!params)
    return;

  E3DCOLOR color = color_apply_mods(params->color, render_state.opacity, params->brightness);

  const ScreenCoord &sc = elem->screenCoord;
  scenerender::setTransformedViewPort(ctx, sc.screenPos, sc.screenPos + sc.size, render_state);

  ctx.set_color(color);
  ctx.reset_textures();

  const float SCALE = 1.0f;

  StdGuiFontContext fctx;
  StdGuiRender::get_font_context(fctx, params->fontId, params->spacing, params->monoWidth, params->fontHt);

  ctx.set_font(params->fontId, params->spacing, params->monoWidth);
  if (fctx.fontHt)
    ctx.set_font_ht(fctx.fontHt);
  int ascent = StdGuiRender::get_font_ascent(fctx);
  int descent = StdGuiRender::get_font_descent(fctx);
  int textLength = elem->props.text.length();

  if (params->fontFx != FFT_NONE)
    ctx.set_draw_str_attr(params->fontFx, params->fxOffsX, params->fxOffsY,
      color_apply_mods(params->fxColor, render_state.opacity, params->brightness), params->fxFactor);

  if (params->fontTex && params->fontTex->getPic().tex != BAD_TEXTUREID)
    ctx.set_draw_str_texture(params->fontTex->getPic().tex, params->fontTex->getPic().smp, params->fontTexSu, params->fontTexSv,
      params->fontTexBov);

  float strHeight = (ascent + descent);
  bool all_glyphs_ready = true;
  bool has_focus = ((elem->getStateFlags() & Element::S_KB_FOCUS) && is_text_input(elem));
  float relCursorX = -1e6;
  if (!params->guiText.isReady() || has_focus)
  {
    if (const char *e = (const char *)memchr(elem->props.text.c_str(), 0, textLength))
      textLength = e - elem->props.text.c_str();

    wchar_t *textU16 = (wchar_t *)alloca((textLength + 2) * sizeof(wchar_t));
    int used = utf8_to_wcs_ex(elem->props.text.c_str(), textLength, textU16, textLength + 1);
    G_ASSERT(used <= textLength);
    textU16[used] = 0;
    textLength = used;
    StdGuiRender::replace_tabs_with_zwspaces(textU16);
    if (params->passChar)
      for (int i = 0; i < textLength; i++)
        textU16[i] = params->passChar;

    TextOverflow textOverflowMode = params->overflowX;
    if (elem->layout.hAlign == ALIGN_CENTER)
      textOverflowMode = TOVERFLOW_CLIP; // other modes are currently disabled for centered text (TODO)

    real ellipsisWidth = params->ellipsis ? StdGuiRender::get_str_bbox_u(ellipsisUtf16, 1, fctx).width().x : 0;
    int start_char_idx = 0, char_count = 0;
    float strWidth = StdGuiRender::get_str_bbox_u_ex(textU16, textLength, fctx,
      textOverflowMode == TOVERFLOW_CLIP ? 4096 : (sc.size.x - elem->layout.padding.left() - elem->layout.padding.right()),
      elem->layout.hAlign != ALIGN_RIGHT, start_char_idx, char_count, textOverflowMode == TOVERFLOW_WORD ? L" \t\r\n" : NULL,
      ellipsisWidth)
                       .width()
                       .x;

    if ((start_char_idx || char_count < textLength) && ellipsisWidth > 0)
    {
      if (elem->layout.hAlign != ALIGN_RIGHT)
      {
        // append ellipsis
        textU16[start_char_idx + char_count] = ellipsisUtf16[0];
        char_count++;
      }
      else
      {
        // prepend ellipsis
        if (start_char_idx > 0)
          start_char_idx--;
        else
          memmove(&textU16[1], &textU16[0], sizeof(*textU16) * char_count);
        textU16[start_char_idx] = ellipsisUtf16[0];
        char_count++;
      }
      strWidth += ellipsisWidth;
    }
    all_glyphs_ready = ctx.draw_str_scaled_u_buf(params->guiText.v, params->guiText.c, params->guiText.samplers,
      StdGuiRender::DSBFLAG_rel, SCALE, textU16 + start_char_idx, char_count);

    params->strWidth = strWidth;
    if (has_focus)
    {
      int cursorPos = elem->props.storage.RawGetSlotValue(elem->csk->cursorPos, textLength);

      int len = clamp(cursorPos, 0, textLength);
      if (len >= start_char_idx && len <= start_char_idx + char_count)
      {
        BBox2 bbox = StdGuiRender::get_str_bbox_u(textU16 + start_char_idx, len - start_char_idx, fctx);
        relCursorX = (bbox.isempty() ? 0 : bbox.right());
        if (len)
          relCursorX += params->spacing * 0.5f;
      }
    }
  }

  float startPosX = sc.screenPos.x - sc.scrollOffs.x;
  float startPosY = sc.screenPos.y;
  if (elem->layout.hAlign == ALIGN_RIGHT)
    startPosX += sc.size.x - elem->layout.padding.right() - params->strWidth;
  else if (elem->layout.hAlign == ALIGN_CENTER)
    startPosX += floorf(sc.size.x / 2 - params->strWidth / 2);
  else // ALIGN_LEFT
    startPosX += elem->layout.padding.left();

  if (elem->layout.vAlign == ALIGN_BOTTOM)
    startPosY += sc.size.y - descent - elem->layout.padding.bottom();
  else if (elem->layout.vAlign == ALIGN_CENTER)
    startPosY += floorf((sc.size.y - strHeight) / 2 + ascent);
  else // ALIGN_TOP
    startPosY += ascent + elem->layout.padding.top();

  if (all_glyphs_ready)
  {
    ctx.goto_xy(startPosX, startPosY);
    ctx.start_font_str(SCALE);
    ctx.render_str_buf(params->guiText.v, params->guiText.c, params->guiText.samplers,
      StdGuiRender::DSBFLAG_rel | StdGuiRender::DSBFLAG_curColor | StdGuiRender::DSBFLAG_checkVis);
  }
  else
    params->guiText.discard();

  ctx.reset_draw_str_attr();
  ctx.reset_draw_str_texture();

  if (has_focus && relCursorX > -1e6)
  {
    float x = floorf(startPosX + relCursorX + 1);
    ctx.draw_line(x, sc.screenPos.y, x, sc.screenPos.y + sc.size.y);
  }

  scenerender::restoreTransformedViewPort(ctx);
}


RobjParamsInscription::~RobjParamsInscription() { StdGuiRender::purge_inscription(inscription); }

// returns (fontSize, fullHeight)
static eastl::tuple<float, float> calc_inscription_dims(const Element *elem, const Properties &props, int fontId)
{
  float cellHeight = StdGuiRender::get_font_caps_ht(fontId);
  float fontSize = props.getFloat(elem->csk->fontSize, cellHeight);

  float fullHeight = (StdGuiRender::get_font_ascent(fontId) + StdGuiRender::get_font_descent(fontId)) * fontSize / cellHeight;

  return eastl::make_tuple(fontSize, fullHeight);
}


bool RobjParamsInscription::load(const Element *elem)
{
  const Properties &props = elem->props;

  color = props.getColor(elem->csk->color, elem->etree->guiScene->config.defTextColor);
  brightness = props.getFloat(elem->csk->brightness, 1.0f);
  fontId = props.getFontId();

  fontFx = props.getInt<FontFxType>(elem->csk->fontFx, FFT_NONE);
  fxColor = props.getColor(elem->csk->fontFxColor, defFontFxColor);
  fxFactor = props.getInt(elem->csk->fontFxFactor, 48);
  fxOffsX = props.getInt(elem->csk->fontFxOffsX, 0);
  fxOffsY = props.getInt(elem->csk->fontFxOffsY, 0);

  fontTex = elem->props.getPicture(elem->csk->fontTex);
  fontTexSu = props.getInt(elem->csk->fontTexSu, 32);
  fontTexSv = props.getInt(elem->csk->fontTexSv, 32);
  fontTexBov = props.getInt(elem->csk->fontTexBov, 0);

  const String &text = elem->props.text;
  eastl::tie(fontSize, fullHeight) = calc_inscription_dims(elem, props, fontId);

  inscription = StdGuiRender::create_inscription_ht(fontId, fontSize, text, text.length());

  return true;
}


void RobjParamsInscription::rebuildInscription(const Element *elem)
{
  const String &text = elem->props.text;
  eastl::tie(fontSize, fullHeight) = calc_inscription_dims(elem, elem->props, fontId);
  inscription = StdGuiRender::create_inscription_ht(fontId, fontSize, text, text.length());
}


bool RobjParamsInscription::getAnimColor(AnimProp prop, E3DCOLOR **ptr)
{
  if (prop == AP_COLOR)
  {
    *ptr = &color;
    return true;
  }
  return false;
}


bool RobjParamsInscription::getAnimFloat(AnimProp prop, float **ptr)
{
  if (prop == AP_BRIGHTNESS)
  {
    *ptr = &brightness;
    return true;
  }
  return false;
}


void RenderObjectInscription::render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  AutoProfileScope pfl(GuiScene::get_from_elem(elem)->getProfiler(), M_RENDER_TEXT);

  RobjParamsInscription *params = static_cast<RobjParamsInscription *>(rdata->params);
  G_ASSERT(params);
  if (!params || !params->inscription)
    return;

  if (!StdGuiRender::is_inscription_ready(params->inscription))
  {
    params->rebuildInscription(elem);
    if (!StdGuiRender::is_inscription_ready(params->inscription))
      return;
  }

  E3DCOLOR color = color_apply_mods(params->color, render_state.opacity, params->brightness);

  const ScreenCoord &sc = elem->screenCoord;
  scenerender::setTransformedViewPort(ctx, sc.screenPos, sc.screenPos + sc.size, render_state);

  BBox2 inscrBox = StdGuiRender::get_inscription_size(params->inscription);

  // get current ascent and descent after scaling to the needed font size
  float fontScale = params->fontSize / StdGuiRender::get_font_caps_ht(params->fontId);
  int descent = StdGuiRender::get_font_descent(params->fontId) * fontScale;
  int ascent = StdGuiRender::get_font_ascent(params->fontId) * fontScale;
  real startPosX, startPosY;

  if (elem->layout.hAlign == ALIGN_RIGHT)
    startPosX = sc.screenPos.x - sc.scrollOffs.x + sc.size.x - elem->layout.padding.right() - inscrBox.right();
  else if (elem->layout.hAlign == ALIGN_CENTER)
    startPosX = sc.screenPos.x - sc.scrollOffs.x + sc.size.x / 2 - inscrBox.size().x / 2;
  else // ALIGN_LEFT
    startPosX = sc.screenPos.x - sc.scrollOffs.x + elem->layout.padding.left() - inscrBox.left();

  if (elem->layout.vAlign == ALIGN_BOTTOM)
    startPosY = sc.screenPos.y + sc.size.y - descent - elem->layout.padding.bottom();
  else if (elem->layout.vAlign == ALIGN_CENTER)
    startPosY = sc.screenPos.y + sc.size.y / 2 + inscrBox.size().y / 2 - descent;
  else // ALIGN_TOP
    startPosY = sc.screenPos.y + ascent + elem->layout.padding.top();

  Point2 startPos(floorf(startPosX), floorf(startPosY));

  ctx.reset_textures();

#if 0
  ctx.set_color(200, 20, 20, 180);
  ctx.render_frame(startPos.x, startPos.y-ascent+descent,
    startPos.x + inscrBox.width().x, startPos.y-ascent+inscrBox.width().y+descent, 1);
#endif

  ctx.set_color(color);

  if (params->fontFx != FFT_NONE)
    ctx.set_draw_str_attr(params->fontFx, params->fxOffsX, params->fxOffsY,
      color_apply_mods(params->fxColor, render_state.opacity, params->brightness), params->fxFactor);

  if (params->fontTex && params->fontTex->getPic().tex != BAD_TEXTUREID)
    ctx.set_draw_str_texture(params->fontTex->getPic().tex, params->fontTex->getPic().smp, params->fontTexSu, params->fontTexSv,
      params->fontTexBov);

  ctx.goto_xy(startPos);
  ctx.draw_inscription(params->inscription);

  ctx.reset_draw_str_attr();
  ctx.reset_draw_str_texture();

  scenerender::restoreTransformedViewPort(ctx);
}


bool RobjParamsImage::load(const Element *elem)
{
  color = elem->props.getColor(elem->csk->color);
  image = elem->props.getPicture(elem->csk->image);
  fallbackImage = elem->props.getPicture(elem->csk->fallbackImage);
  keepAspect = resolve_keep_aspect(elem->props.scriptDesc.RawGetSlot(elem->csk->keepAspect));
  imageHalign = elem->props.getInt<ElemAlign>(elem->csk->imageHalign, ALIGN_CENTER);
  imageValign = elem->props.getInt<ElemAlign>(elem->csk->imageValign, ALIGN_CENTER);
  flipX = elem->props.getBool(elem->csk->flipX);
  flipY = elem->props.getBool(elem->csk->flipY);
  imageAffectsLayout = elem->props.getBool(elem->csk->imageAffectsLayout);

  saturateFactor = elem->props.getFloat(elem->csk->picSaturate, 1.0f);
  brightness = elem->props.getFloat(elem->csk->brightness, 1.0f);
  return true;
}


bool RobjParamsImage::getAnimColor(AnimProp prop, E3DCOLOR **ptr)
{
  if (prop == AP_COLOR)
  {
    *ptr = &color;
    return true;
  }
  return false;
}
bool RobjParamsImage::getAnimFloat(AnimProp prop, float **ptr)
{
  if (prop == darg::AP_PICSATURATE)
  {
    *ptr = &saturateFactor;
    return true;
  }
  if (prop == darg::AP_BRIGHTNESS)
  {
    *ptr = &brightness;
    return true;
  }
  return false;
}

template <typename T>
static void swap_vals(T &a, T &b)
{
  T t = a;
  a = b;
  b = t;
}

static void adjust_coord_for_image_aspect(Point2 &posLt, Point2 &posRb, Point2 &tc0, Point2 &tc1, const Point2 &elemPos,
  const Point2 &elemSz, const Point2 &picSz, KeepAspectMode keep_aspect, ElemAlign image_halign, ElemAlign image_valign)
{
  if (picSz.x <= 0 || picSz.y <= 0 || elemSz.x <= 0 || elemSz.y <= 0)
    return;

  float picRatio = picSz.x / picSz.y;
  float elemRatio = elemSz.x / elemSz.y;

  if (keep_aspect == KEEP_ASPECT_FIT)
  {
    float w, h;
    if (picRatio <= elemRatio)
    {
      w = floorf(elemSz.y * picRatio);
      h = elemSz.y;
    }
    else
    {
      w = elemSz.x;
      h = floorf(elemSz.x / picRatio);
    }

    if (image_halign == ALIGN_CENTER)
    {
      float center = floor(elemPos.x + elemSz.x * 0.5f);
      posLt.x = center - floorf(w * 0.5f);
      posRb.x = posLt.x + w;
    }
    else if (image_halign == ALIGN_RIGHT)
      posLt.x = elemPos.x + elemSz.x - w;
    else
      posRb.x = elemPos.x + w;

    if (image_valign == ALIGN_CENTER)
    {
      float center = floor(elemPos.y + elemSz.y * 0.5f);
      posLt.y = center - floorf(h * 0.5f);
      posRb.y = posLt.y + h;
    }
    else if (image_valign == ALIGN_BOTTOM)
      posLt.y = elemPos.y + elemSz.y - h;
    else
      posRb.y = elemPos.y + h;
  }
  else if (keep_aspect == KEEP_ASPECT_FILL)
  {
    if (picRatio > elemRatio)
    {
      float w = (tc1.x - tc0.x) * elemRatio / picRatio;
      if (image_halign == ALIGN_CENTER)
      {
        float border = 0.5f * (tc1.x - tc0.x - w);
        tc0.x = tc0.x + border;
        tc1.x = tc1.x - border;
      }
      else if (image_valign == ALIGN_RIGHT)
        tc0.x = tc1.x - w;
      else
        tc1.x = tc0.x + w;
    }
    else if (picRatio < elemRatio)
    {
      float h = (tc1.y - tc0.y) * picRatio / elemRatio;
      if (image_valign == ALIGN_CENTER)
      {
        float border = 0.5f * (tc1.y - tc0.y - h);
        tc0.y = tc0.y + border;
        tc1.y = tc1.y - border;
      }
      else if (image_valign == ALIGN_RIGHT)
        tc0.y = tc1.y - h;
      else
        tc1.y = tc0.y + h;
    }
  }
}

void RenderObjectImage::render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  G_UNUSED(elem);
  RobjParamsImage *params = static_cast<RobjParamsImage *>(rdata->params);
  G_ASSERT_RETURN(params, );

  if (!params->image)
    return;

  Picture *image = params->image;
  if (!image->isLoading() && params->fallbackImage)
  {
    if (image->getPic().pic == BAD_PICTUREID)
      image = params->fallbackImage;
    else
    {
      // Handle non-existing items in dynamic atlas
      // They have valid ids, but their size is empty
      Point2 sz = PictureManager::get_picture_pix_size(image->getPic().pic);
      if (sz.x <= 0 || sz.y <= 0)
        image = params->fallbackImage;
    }
  }

  const PictureManager::PicDesc &pic = image->getPic();

  E3DCOLOR color = color_apply_mods(params->color, render_state.opacity, params->brightness);
  ctx.set_color(color);

  if (params->saturateFactor != 1.0f)
    ctx.set_picquad_color_matrix_saturate(params->saturateFactor);

  ctx.set_texture(pic.tex, pic.smp);
  ctx.set_alpha_blend(image->getBlendMode());

  pic.updateTc();

  Point2 tc0 = pic.tcLt, tc1 = pic.tcRb;
  if (params->flipX)
    swap_vals(tc0.x, tc1.x);
  if (params->flipY)
    swap_vals(tc0.y, tc1.y);

  Point2 picSz = PictureManager::get_picture_pix_size(pic.pic);

  Point2 elemPos = rdata->pos, elemSz = rdata->size;
  Point2 posLt = elemPos, posRb = elemPos + elemSz;

  if (picSz.x > 0 && picSz.y > 0 && elemSz.x > 0 && elemSz.y > 0)
  {
    adjust_coord_for_image_aspect(posLt, posRb, tc0, tc1, elemPos, elemSz, picSz, params->keepAspect, params->imageHalign,
      params->imageValign);
    ctx.render_rect_t(posLt, posRb, tc0, tc1);
  }
  if (params->saturateFactor != 1.0f)
    ctx.reset_picquad_color_matrix();
}


void RobjParamsVectorCanvas::setupDrawApi(GuiScene *scene)
{
  HSQUIRRELVM vm = draw.GetVM();
  G_ASSERT(vm);
  G_ASSERT(scene);

  SqStackChecker chk(vm);

  sq_newuserdata(vm, sizeof(RenderCanvasContext *));
  sq_settypetag(vm, -1, RenderCanvasContext::get_type_tag());
  sq_pushobject(vm, scene->canvasApi);
  G_VERIFY(SQ_SUCCEEDED(sq_setdelegate(vm, -2)));
  HSQOBJECT handle;
  sq_getstackobj(vm, -1, &handle);
  canvasScriptRef = Sqrat::Object(handle, vm);
  sq_pop(vm, 1);

  rectCache = Sqrat::Table(vm);
}


bool RobjParamsVectorCanvas::load(const Element *elem)
{
  color = elem->props.getColor(elem->csk->color);
  fillColor = elem->props.getColor(elem->csk->fillColor);
  lineWidth = elem->props.getFloat(elem->csk->lineWidth, 2.f);
  commands = elem->props.getObject(elem->csk->commands);
  draw = elem->props.scriptDesc.RawGetFunction(elem->csk->draw);
  brightness = elem->props.getFloat(elem->csk->brightness, 1.f);
  if (!draw.IsNull())
    setupDrawApi(GuiScene::get_from_elem(elem));
  return true;
}


bool RobjParamsVectorCanvas::getAnimColor(AnimProp prop, E3DCOLOR **ptr)
{
  if (prop == AP_COLOR)
  {
    *ptr = &color;
    return true;
  }
  if (prop == AP_FILL_COLOR)
  {
    *ptr = &fillColor;
    return true;
  }
  return false;
}


void RenderObjectVectorCanvas::render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  RobjParamsVectorCanvas *params = static_cast<RobjParamsVectorCanvas *>(rdata->params);
  G_ASSERT(params);
  if (!params || (params->commands.IsNull() && params->draw.IsNull()))
    return;

  if (!params->commands.IsNull() && params->commands.GetType() != OT_ARRAY)
  {
    darg_assert_trace_var("VectorCanvas: 'commands' is not array", elem->props.scriptDesc, elem->csk->commands);
    return;
  }

  ctx.reset_textures();
  ctx.set_alpha_blend(PREMULTIPLIED);

  RenderCanvasContext rctx;
  rctx.ctx = &ctx;
  rctx.renderStateOpacity = render_state.opacity;
  rctx.color = color_apply_mods(params->color, render_state.opacity, params->brightness);
  rctx.fillColor = color_apply_mods(params->fillColor, render_state.opacity, params->brightness);
  rctx.currentColor = rctx.color;
  rctx.currentFillColor = rctx.fillColor;

  rctx.offset = elem->screenCoord.screenPos;
  rctx.scale = elem->screenCoord.size * 0.01f;

  rctx.lineWidth = params->lineWidth;
  rctx.lineAlign = 0.f; // 0 = center, 0.5 = outer, -0.5 = inner

  Point2 lineIndentPx = Point2(0, 0);
  Point2 lineIndentPct = Point2(0, 0);

  if (!params->commands.IsNull())
  {
    Sqrat::Array commands(params->commands);

    int len = commands.Length();
    for (int idx = 0; idx < len; idx++)
    {
      if (commands[idx].GetType() != OT_ARRAY)
      {
        darg_assert_trace_var(String(0, "VectorCanvas: 'commands[%d]' is not array", idx), params->commands, idx);
        break;
      }

      Sqrat::Array cmd(commands[idx]);

      if (cmd.Length() == 0)
        continue;

      switch (sq_ext_get_array_int(cmd.GetObject(), 0, -1))
      {
        case VECTOR_WIDTH:
          if (cmd.Length() == 1)
            rctx.lineWidth = params->lineWidth;
          else
            rctx.lineWidth = cmd[1].Cast<float>();
          break;

        case VECTOR_LINE_INDENT_PX:
          if (cmd.Length() == 1)
            lineIndentPx = Point2(0, 0);
          if (cmd.Length() == 2)
            lineIndentPx.x = lineIndentPx.y = cmd[1].Cast<float>();
          else if (cmd.Length() == 3)
            lineIndentPx = Point2(cmd[1].Cast<float>(), cmd[2].Cast<float>());
          break;

        case VECTOR_LINE_INDENT_PCT:
          if (cmd.Length() == 1)
            lineIndentPct = Point2(0, 0);
          if (cmd.Length() == 2)
            lineIndentPct.x = lineIndentPct.y = rctx.scale.x * cmd[1].Cast<float>();
          else if (cmd.Length() == 3)
            lineIndentPct = rctx.scale.x * Point2(cmd[1].Cast<float>(), cmd[2].Cast<float>());
          break;

        case VECTOR_COLOR:
          if (cmd.Length() == 1)
            rctx.currentColor = params->color;
          else
            rctx.currentColor = script_decode_e3dcolor(cmd[1].Cast<SQInteger>());
          rctx.color = color_apply_mods(rctx.currentColor, render_state.opacity, params->brightness);
          break;

        case VECTOR_FILL_COLOR:
          if (cmd.Length() == 1)
            rctx.currentFillColor = params->fillColor;
          else
            rctx.currentFillColor = script_decode_e3dcolor(cmd[1].Cast<SQInteger>());
          rctx.fillColor = color_apply_mods(rctx.currentFillColor, render_state.opacity, params->brightness);
          break;

        case VECTOR_MID_COLOR:
          if (cmd.Length() == 1)
          {
            rctx.midColorUsed = false;
          }
          else
          {
            rctx.midColorUsed = true;
            rctx.currentMidColor = script_decode_e3dcolor(cmd[1].Cast<SQInteger>());
            rctx.midColor = color_apply_mods(rctx.currentMidColor, render_state.opacity, params->brightness);
          }
          break;

        case VECTOR_OPACITY:
        {
          const float op =
            (cmd.Length() == 1) ? render_state.opacity : render_state.opacity * ::clamp(cmd[1].Cast<SQFloat>(), 0.f, 1.f);
          rctx.color = color_apply_opacity(rctx.currentColor, op);
          rctx.fillColor = color_apply_opacity(rctx.currentFillColor, op);
          rctx.midColor = color_apply_opacity(rctx.currentMidColor, op);
        }
        break;

        case VECTOR_OUTER_LINE: rctx.lineAlign = 0.5f; break;

        case VECTOR_CENTER_LINE: rctx.lineAlign = 0.0f; break;

        case VECTOR_INNER_LINE: rctx.lineAlign = -0.5f; break;

        case VECTOR_TM_OFFSET:
          if (cmd.Length() == 1)
            rctx.offset = elem->screenCoord.screenPos;
          else
            rctx.offset = Point2(cmd[1].Cast<SQFloat>(), cmd[2].Cast<SQFloat>()) + elem->screenCoord.screenPos;
          break;

        case VECTOR_TM_SCALE:
          if (cmd.Length() == 1)
            rctx.scale = elem->screenCoord.size * 0.01f;
          else
            rctx.scale = elem->screenCoord.size * 0.01f * cmd[1].Cast<SQFloat>();
          break;

        case VECTOR_LINE: rctx.renderLine(cmd, lineIndentPx + lineIndentPct); break;

        case VECTOR_LINE_DASHED: rctx.renderLineDashed(cmd); break;

        case VECTOR_ELLIPSE: rctx.renderEllipse(cmd); break;

        case VECTOR_SECTOR: rctx.renderSector(cmd); break;

        case VECTOR_RECTANGLE: rctx.renderRectangle(cmd); break;

        case VECTOR_POLY: rctx.renderFillPoly(cmd); break;

        case VECTOR_INVERSE_POLY: rctx.renderFillInversePoly(cmd); break;

        case VECTOR_QUADS: rctx.renderQuads(cmd); break;

        case VECTOR_NOP: break;

        default: darg_assert_trace_var(String(0, "'VectorCanvas: invalid command in commands[%d]", idx), params->commands, idx);
      };
    }
  }

  if (!params->draw.IsNull())
  {
    const StringKeys *csk = elem->csk;

    HSQOBJECT hUd = params->canvasScriptRef.GetObject();
    SQUserPointer ptr = nullptr, typeTag = nullptr;
    G_VERIFY(SQ_SUCCEEDED(sq_direct_getuserdata(&hUd, &ptr, &typeTag)));
    G_ASSERT(typeTag == RenderCanvasContext::get_type_tag());

    RenderCanvasContext **udPctx = (RenderCanvasContext **)ptr;
    *udPctx = &rctx;
    params->rectCache.SetValue(csk->w, rdata->size.x);
    params->rectCache.SetValue(csk->h, rdata->size.y);

    GuiVertexTransform prevGvtm;
    ctx.getViewTm(prevGvtm.vtm);
    ctx.setViewTm(Point2(1, 0), Point2(0, 1), rdata->pos, true);

    params->draw.Execute(params->canvasScriptRef, params->rectCache);

    ctx.setViewTm(prevGvtm.vtm);

    *udPctx = nullptr;
  }
}

#undef MAKEDATA


#define T 0
#define R 1
#define B 2
#define L 3

class RobjParams9rect : public RendObjParams
{
public:
  E3DCOLOR color = E3DCOLOR(0, 0, 0, 0);
  float brightness = 1.0f;
  Picture *image = nullptr;
  mutable float texOffs[4] = {0, 0, 0, 0};
  mutable float screenOffs[4] = {0, 0, 0, 0};
  mutable bool offsetsReady = false;

  void initOffsets(const Element *elem) const
  {
    Point2 fullTexSize, picSize;
    if (image && image->getPic().tex != BAD_TEXTUREID && !image->isLoading() &&
        PictureManager::get_picture_size(image->getPic().pic, fullTexSize, picSize))
    {
      const Sqrat::Table &scriptDesc = elem->props.scriptDesc;
      Sqrat::Object texOffsObj = scriptDesc.RawGetSlot(elem->csk->texOffs);
      Sqrat::Object screenOffsObj = scriptDesc.RawGetSlot(elem->csk->screenOffs);

      const char *errMsg = nullptr;

      if (!script_parse_offsets(elem, texOffsObj, texOffs, &errMsg))
        darg_assert_trace_var(errMsg, scriptDesc, elem->csk->texOffs);

      if (!script_parse_offsets(elem, screenOffsObj, screenOffs, &errMsg))
        darg_assert_trace_var(errMsg, scriptDesc, elem->csk->screenOffs);

      for (int i = 0; i < 4; ++i)
        if (texOffs[i] <= 0)
          texOffs[i] = screenOffs[i] = 0;

      texOffs[L] /= fullTexSize.x;
      texOffs[R] /= fullTexSize.x;
      texOffs[T] /= fullTexSize.y;
      texOffs[B] /= fullTexSize.y;

      offsetsReady = true;
    }
  }

  virtual bool load(const Element *elem) override
  {
    color = elem->props.getColor(elem->csk->color);
    image = elem->props.getPicture(elem->csk->image);
    brightness = elem->props.getFloat(elem->csk->brightness, 1.0f);

    initOffsets(elem);

    return true;
  }

  virtual bool getAnimColor(AnimProp prop, E3DCOLOR **ptr) override
  {
    if (prop == AP_COLOR)
    {
      *ptr = &color;
      return true;
    }
    return false;
  }

  virtual bool getAnimFloat(AnimProp prop, float **ptr) override
  {
    if (prop == AP_BRIGHTNESS)
    {
      *ptr = &brightness;
      return true;
    }
    return false;
  }
};


void RenderObject9rect::render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  const ScreenCoord &sc = elem->screenCoord;
  if (sc.size.x < 1 || sc.size.y < 1)
    return;

  RobjParams9rect *params = static_cast<RobjParams9rect *>(rdata->params);
  G_ASSERT(params);
  if (!params)
    return;

  AutoProfileScope pfl(GuiScene::get_from_elem(elem)->getProfiler(), M_RENDER_9RECT);

  E3DCOLOR color = color_apply_mods(params->color, render_state.opacity, params->brightness);
  if (!color.u)
    return;

  if (params->image && params->image->isLoading())
    return;

  ctx.set_color(color);

  if (!params->image || params->image->getPic().pic == BAD_PICTUREID)
  {
    ctx.reset_textures();
    ctx.render_box(sc.screenPos, sc.screenPos + sc.size);
    return;
  }

  if (!params->offsetsReady)
    params->initOffsets(elem);

  const PictureManager::PicDesc &pic = params->image->getPic();

  ctx.set_texture(pic.tex, pic.smp);
  ctx.set_alpha_blend(params->image->getBlendMode());

  const float *texOffs = params->texOffs;
  float screenOffs[4];
  memcpy(screenOffs, params->screenOffs, sizeof(screenOffs));

  float sumScreenOffsX = screenOffs[L] + screenOffs[R];
  if (sumScreenOffsX > sc.size.x)
  {
    screenOffs[L] *= sc.size.x / sumScreenOffsX;
    screenOffs[R] *= sc.size.x / sumScreenOffsX;
  }

  float sumScreenOffsY = screenOffs[T] + screenOffs[B];
  if (sumScreenOffsY > sc.size.y)
  {
    screenOffs[T] *= sc.size.y / sumScreenOffsY;
    screenOffs[B] *= sc.size.y / sumScreenOffsY;
  }

  Point2 soffsLt(screenOffs[L], screenOffs[T]);
  Point2 soffsRb(screenOffs[R], screenOffs[B]);
  Point2 toffsLt(texOffs[L], texOffs[T]);
  Point2 toffsRb(texOffs[R], texOffs[B]);

  pic.updateTc();

  // top row
  if (screenOffs[T] > 0)
  {
    // left top
    if (screenOffs[L] > 0)
    {
      ctx.render_rect_t(sc.screenPos, sc.screenPos + soffsLt, pic.tcLt, pic.tcLt + toffsLt);
    }
    // top center
    if (sc.size.x > sumScreenOffsX)
    {
      ctx.render_rect_t(sc.screenPos.x + soffsLt.x, sc.screenPos.y, sc.screenPos.x + sc.size.x - soffsRb.x, sc.screenPos.y + soffsLt.y,
        Point2(pic.tcLt.x + toffsLt.x, pic.tcLt.y), Point2(pic.tcRb.x - toffsRb.x, pic.tcLt.y + toffsLt.y));
    }
    // right top
    if (screenOffs[R] > 0)
    {
      ctx.render_rect_t(sc.screenPos.x + sc.size.x - soffsRb.x, sc.screenPos.y, sc.screenPos.x + sc.size.x, sc.screenPos.y + soffsLt.y,
        Point2(pic.tcRb.x - toffsRb.x, pic.tcLt.y), Point2(pic.tcRb.x, pic.tcLt.y + toffsLt.y));
    }
  }

  // middle row
  if (sc.size.y > sumScreenOffsY)
  {
    // left middle
    if (screenOffs[L] > 0)
    {
      ctx.render_rect_t(sc.screenPos.x, sc.screenPos.y + soffsLt.y, sc.screenPos.x + soffsLt.x, sc.screenPos.y + sc.size.y - soffsRb.y,
        Point2(pic.tcLt.x, pic.tcLt.y + toffsLt.y), Point2(pic.tcLt.x + toffsLt.x, pic.tcRb.y - toffsRb.y));
    }
    // center
    if (sc.size.x > sumScreenOffsX)
    {
      ctx.render_rect_t(sc.screenPos + soffsLt, sc.screenPos + sc.size - soffsRb, pic.tcLt + toffsLt, pic.tcRb - toffsRb);
    }
    // right middle
    if (screenOffs[R] > 0)
    {
      ctx.render_rect_t(sc.screenPos.x + sc.size.x - soffsRb.x, sc.screenPos.y + soffsLt.y, sc.screenPos.x + sc.size.x,
        sc.screenPos.y + sc.size.y - soffsRb.y, Point2(pic.tcRb.x - toffsRb.x, pic.tcLt.y + toffsLt.y),
        Point2(pic.tcRb.x, pic.tcRb.y - toffsRb.y));
    }
  }

  // bottom row
  if (screenOffs[B] > 0)
  {
    // left bottom
    if (screenOffs[L] > 0)
    {
      ctx.render_rect_t(sc.screenPos.x, sc.screenPos.y + sc.size.y - soffsRb.y, sc.screenPos.x + soffsLt.x, sc.screenPos.y + sc.size.y,
        Point2(pic.tcLt.x, pic.tcRb.y - toffsRb.y), Point2(pic.tcLt.x + toffsLt.x, pic.tcRb.y));
    }
    // center
    if (sc.size.x > sumScreenOffsX)
    {
      ctx.render_rect_t(sc.screenPos.x + soffsLt.x, sc.screenPos.y + sc.size.y - soffsRb.y, sc.screenPos.x + sc.size.x - soffsRb.x,
        sc.screenPos.y + sc.size.y, Point2(pic.tcLt.x + toffsLt.x, pic.tcRb.y - toffsRb.y),
        Point2(pic.tcRb.x - toffsRb.x, pic.tcRb.y));
    }
    // right bottom
    if (screenOffs[R] > 0)
    {
      ctx.render_rect_t(sc.screenPos + sc.size - soffsRb, sc.screenPos + sc.size, pic.tcRb - toffsRb, pic.tcRb);
    }
  }
}

#undef T
#undef R
#undef B
#undef L


class RobjParamsProgressLinear : public RendObjParams
{
public:
  E3DCOLOR bgColor;
  E3DCOLOR fgColor;
  float brightness = 1.0f;

  virtual bool load(const Element *elem) override
  {
    bgColor = elem->props.getColor(elem->csk->bgColor, E3DCOLOR(100, 100, 100, 100));
    fgColor = elem->props.getColor(elem->csk->fgColor, E3DCOLOR(255, 255, 255, 255));
    brightness = elem->props.getFloat(elem->csk->brightness, 1.0f);
    return true;
  }

  virtual bool getAnimColor(AnimProp prop, E3DCOLOR **ptr) override
  {
    if (prop == AP_BG_COLOR)
    {
      *ptr = &bgColor;
      return true;
    }
    if (prop == AP_FG_COLOR)
    {
      *ptr = &fgColor;
      return true;
    }
    return false;
  }

  virtual bool getAnimFloat(AnimProp prop, float **ptr) override
  {
    if (prop == AP_BRIGHTNESS)
    {
      *ptr = &brightness;
      return true;
    }
    return false;
  }
};


void RenderObjectProgressLinear::render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  RobjParamsProgressLinear *params = static_cast<RobjParamsProgressLinear *>(rdata->params);
  G_ASSERT(params);
  if (!params)
    return;

  const float eps = 1e-5f;

  const float fValue = elem->props.getCurrentFloat(elem->csk->fValue, 0.f);
  float val = ::clamp(fValue, -1.0f, 1.0f);

  if (fabsf(val) < 1.0f - eps)
  {
    E3DCOLOR bgColor = color_apply_mods(params->bgColor, render_state.opacity, params->brightness);
    if (bgColor.u)
    {
      if (val > 0)
        drawRect(ctx, elem, val - 1.0f, bgColor);
      else
        drawRect(ctx, elem, val + 1.0f, bgColor);
    }
  }

  if (fabsf(val) > 1e-5f)
  {
    E3DCOLOR fgColor = color_apply_mods(params->fgColor, render_state.opacity, params->brightness);
    drawRect(ctx, elem, val, fgColor);
  }
}


void RenderObjectProgressLinear::drawRect(StdGuiRender::GuiContext &ctx, const Element *elem, float val, E3DCOLOR color)
{
  Point2 pos = elem->screenCoord.screenPos;
  Point2 size = elem->screenCoord.size;
  size.x *= fabsf(val);
  if (val < 0)
    pos.x += elem->screenCoord.size.x - size.x;

  ctx.set_color(color);
  ctx.reset_textures();
  ctx.render_rect(pos, pos + size);
}


class RobjParamsProgressCircular : public RendObjParams
{
public:
  E3DCOLOR bgColor;
  E3DCOLOR fgColor;
  Picture *image = nullptr;
  Picture *fallbackImage = nullptr;
  float brightness = 1.0f;

  virtual bool load(const Element *elem) override
  {
    bgColor = elem->props.getColor(elem->csk->bgColor, E3DCOLOR(100, 100, 100, 100));
    fgColor = elem->props.getColor(elem->csk->fgColor, E3DCOLOR(255, 255, 255, 255));
    image = elem->props.getPicture(elem->csk->image);
    fallbackImage = elem->props.getPicture(elem->csk->fallbackImage);
    brightness = elem->props.getFloat(elem->csk->brightness, 1.0f);
    return true;
  }

  virtual bool getAnimColor(AnimProp prop, E3DCOLOR **ptr) override
  {
    if (prop == AP_BG_COLOR)
    {
      *ptr = &bgColor;
      return true;
    }
    if (prop == AP_FG_COLOR)
    {
      *ptr = &fgColor;
      return true;
    }
    return false;
  }

  virtual bool getAnimFloat(AnimProp prop, float **ptr) override
  {
    if (prop == AP_BRIGHTNESS)
    {
      *ptr = &brightness;
      return true;
    }
    return false;
  }
};


void RenderObjectProgressCircular::render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  RobjParamsProgressCircular *params = static_cast<RobjParamsProgressCircular *>(rdata->params);
  G_ASSERT(params);
  if (!params)
    return;

  Picture *image = params->image;
  if (image && image->getPic().pic == BAD_PICTUREID && !image->isLoading() && params->fallbackImage)
    image = params->fallbackImage;

  const float fValue = elem->props.getCurrentFloat(elem->csk->fValue, 0.f);
  float val = clamp(fabsf(fValue), 0.0f, 1.0f);
  float dir = (fValue >= 0) ? 1.0f : -1.0f;

  if (val >= 1e-5f)
  {
    E3DCOLOR color = color_apply_mods(params->fgColor, render_state.opacity, params->brightness);
    renderProgress(ctx, rdata, val, dir, image, color);
  }

  if (val < 0.999f)
  {
    E3DCOLOR color = color_apply_mods(params->bgColor, render_state.opacity, params->brightness);
    renderProgress(ctx, rdata, 1.0f - val, -dir, image, color);
  }
}


static void clamp_unit_square(float &x, float &y)
{
  float vx = fabsf(x);
  if (vx > 1.0)
  {
    x /= vx;
    y /= vx;
  }

  float vy = fabsf(y);
  if (vy > 1.0)
  {
    x /= vy;
    y /= vy;
  }
}

void RenderObjectProgressCircular::renderProgress(StdGuiRender::GuiContext &ctx, const ElemRenderData *rdata, float val, float dir,
  Picture *image, E3DCOLOR color)
{
  const int maxSectors = 8;
  const int maxVertices = 1 + 2 * maxSectors;
  const int maxIndices = 3 * maxSectors;
  const float d = sqrtf(2.0);

  TEXTUREID texId = BAD_TEXTUREID;
  d3d::SamplerHandle smpId = d3d::INVALID_SAMPLER_HANDLE;
  Point2 tcLt(0, 0), tcRb(0, 0);
  BlendMode blendMode = PREMULTIPLIED;

  if (image)
  {
    const PictureManager::PicDesc &pic = image->getPic();
    pic.updateTc();
    texId = pic.tex;
    smpId = pic.smp;
    tcLt = pic.tcLt;
    tcRb = pic.tcRb;
    blendMode = image->getBlendMode();
  }

  ctx.set_color(255, 255, 255);
  ctx.set_texture(texId, smpId);
  ctx.set_alpha_blend(blendMode);

  ctx.start_raw_layer();

  GuiVertex v[maxVertices];
  GuiVertexTransform xf;
  ctx.getViewTm(xf.vtm);

  StdGuiRender::IndexType indices[maxIndices];

  Point2 c = rdata->pos + rdata->size * 0.5f;
  float rx = rdata->size.x * 0.5f;
  float ry = rdata->size.y * 0.5f;

  int sectorNo = floorf(::min(val, 0.999f) * maxSectors);
  float vStep = 1.0f / maxSectors;

  v[0].setTc0((tcLt + tcRb) * 0.5f);
  v[0].zeroTc1();
  v[0].color = color;
  v[0].setPos(xf, c);

  for (int iSec = 0; iSec <= sectorNo; ++iSec)
  {
    float angle0 = iSec * vStep * TWOPI * dir;
    float angle1 = ::min(val, (iSec + 1) * vStep) * TWOPI * dir;

    float x0 = sinf(angle0) * d;
    float y0 = -cosf(angle0) * d;

    float x1 = sinf(angle1) * d;
    float y1 = -cosf(angle1) * d;

    clamp_unit_square(x0, y0);
    clamp_unit_square(x1, y1);

    int iv = 1 + iSec * 2;
    G_ASSERTF(iv + 1 < maxVertices, "iv = %d, mv = %d, val = %f, dir = %f", iv, maxVertices, val, dir);

    v[iv].setTc0(lerp(tcLt.x, tcRb.x, 0.5f + 0.5f * x0), lerp(tcLt.y, tcRb.y, 0.5f + 0.5f * y0));
    v[iv].zeroTc1();
    v[iv].color = color;

    v[iv + 1].setTc0(lerp(tcLt.x, tcRb.x, 0.5f + 0.5f * x1), lerp(tcLt.y, tcRb.y, 0.5f + 0.5f * y1));
    v[iv + 1].zeroTc1();
    v[iv + 1].color = color;

    v[iv].setPos(xf, c + Point2(x0 * rx, y0 * ry));
    v[iv + 1].setPos(xf, c + Point2(x1 * rx, y1 * ry));

    G_ASSERT(iSec * 3 + 2 < maxIndices);

    indices[iSec * 3] = 0;
    indices[iSec * 3 + 1] = iv;
    indices[iSec * 3 + 2] = iv + 1;
  }

  int nVertices = 1 + 2 * (sectorNo + 1);
  int nFaces = sectorNo + 1;
  G_ASSERT(nVertices <= maxVertices);
  ctx.draw_faces(v, nVertices, indices, nFaces);

  ctx.end_raw_layer();
}


void RenderObjectTextArea::render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  AutoProfileScope profile(GuiScene::get_from_elem(elem)->getProfiler(), M_RENDER_TEXTAREA);
  TIME_PROFILE(bhv_render_textarea);

  RobjParamsTextArea *params = static_cast<RobjParamsTextArea *>(rdata->params);
  G_ASSERT(params);
  if (!params)
    return;

  textlayout::FormattedText *fmtText = nullptr;

  EditableText *etext = elem->props.storage.RawGetSlotValue<EditableText *>(elem->csk->editableText, nullptr);
  if (etext)
    fmtText = &etext->fmtText;
  else
    fmtText = elem->props.storage.RawGetSlotValue<textlayout::FormattedText *>(elem->csk->formattedText, nullptr);

  if (!fmtText)
    return;

  if (fmtText->hasFormatError())
  {
    ctx.set_color(40, 20, 20, 40);
    ctx.render_box(rdata->pos, rdata->pos + rdata->size);
    ctx.set_color(255, 255, 255, 255);
    ctx.set_font(0);
    ctx.goto_xy(rdata->pos.x, rdata->pos.y + rdata->size.y);
    ctx.draw_str(fmtText->formatErrorMsg.c_str(), fmtText->formatErrorMsg.length());
    return;
  }

  bool ellipsis = (fmtText->preformattedFlags & textlayout::FMT_HIDE_ELLIPSIS) == 0;

  AutoProfileScope pfl(GuiScene::get_from_elem(elem)->getProfiler(), M_RENDER_TEXT);

  E3DCOLOR color = color_apply_mods(params->color, render_state.opacity, params->brightness);
  int spacing = elem->props.getInt(elem->csk->spacing, 0);
  Sqrat::Object objMonoWidth = elem->props.getObject(elem->csk->monoWidth);
  int lastFontId = -2, lastFontHt = 0;
  int monoWidth = 0;

  const ScreenCoord &sc = elem->screenCoord;
  scenerender::setTransformedViewPort(ctx, sc.screenPos, sc.screenPos + sc.size, render_state);

  ctx.set_color(color);
  ctx.reset_textures();
  if (params->fontFx != FFT_NONE)
    ctx.set_draw_str_attr(params->fontFx, params->fxOffsX, params->fxOffsY,
      color_apply_mods(params->fxColor, render_state.opacity, params->brightness), params->fxFactor);

  bool useEllipsis = ellipsis && params->overflowY == TOVERFLOW_LINE;
  bool shouldBreakWithEllipsis = false;

  // FIXME: ellipsis is rendered using block font, but its size is calculated with default
  StdGuiFontContext fctxDefault;
  StdGuiRender::get_font_context(fctxDefault, 0, spacing /*, monoWidth*/);

  Tab<wchar_t> tmpU16(framemem_ptr());
  BBox2 ellipsisBox = StdGuiRender::get_str_bbox_u(ellipsisUtf16, 1, fctxDefault);
  real ellipsisHeight = useEllipsis ? ellipsisBox.width().y : 0;
  real ellipsisWidth = ellipsisBox.width().x;

  bool ellipsisOnSeparateLine = useEllipsis;
  ElemAlign halign = elem->layout.hAlign;
  if (fmtText->lines.size() <= params->lowLineCount && params->lowLineCountAlign != PLACE_DEFAULT)
    halign = params->lowLineCountAlign;

  for (int iLine = 0, nLines = fmtText->lines.size(); iLine < nLines; ++iLine)
  {
    if (shouldBreakWithEllipsis)
      break;

    textlayout::TextLine &line = fmtText->lines[iLine];

    if (line.yPos + fmtText->yOffset + line.contentHeight < sc.scrollOffs.y)
      continue;

    if (iLine == fmtText->lines.size() - 1)
      ellipsisHeight = 0;

    if (line.yPos + fmtText->yOffset + (params->overflowY == TOVERFLOW_LINE ? line.contentHeight : 0) >
        sc.size.y + sc.scrollOffs.y - ellipsisHeight)
    {
      if (useEllipsis)
        shouldBreakWithEllipsis = true;
      else
        break;
    }

    if (!ctx.vpBoxIsVisible(rdata->pos.x, rdata->pos.y + line.yPos - sc.scrollOffs.y, rdata->pos.x + rdata->size.x,
          rdata->pos.y + line.yPos + line.contentHeight - sc.scrollOffs.y))
      continue;

    float xLineOffs = 0;
    float lineWidth = (shouldBreakWithEllipsis && ellipsisOnSeparateLine) ? ellipsisWidth : line.contentWidth;
    int numBlocks = line.blocks.size();

    int lastBlockIndexToRender = numBlocks - 1;

    if (shouldBreakWithEllipsis && !ellipsisOnSeparateLine)
    {
      float tailBlocksWidth = 0;

      while (lastBlockIndexToRender >= 0 && tailBlocksWidth < ellipsisWidth)
      {
        tailBlocksWidth += line.blocks[lastBlockIndexToRender]->size.x;
        --lastBlockIndexToRender;
      }
    }

    int numBlocksToRender = numBlocks;
    if (shouldBreakWithEllipsis && !ellipsisOnSeparateLine)
      numBlocksToRender = lastBlockIndexToRender + 1;
    else if (shouldBreakWithEllipsis && ellipsisOnSeparateLine)
      numBlocksToRender = 0;

    bool hadAllSymbols = true;
    bool nowHaveAllSymbols = true;
    for (int i = 0; i < numBlocksToRender && hadAllSymbols; ++i)
    {
      textlayout::TextBlock *block = line.blocks[i];
      darg::GuiTextCache &guiText = block->guiText;
      if (block->type == textlayout::TextBlock::TBT_SPACE)
        continue;
      hadAllSymbols = guiText.isReady();
    }

    if (halign == ALIGN_RIGHT)
      xLineOffs += sc.size.x - lineWidth;
    else if (halign == ALIGN_CENTER)
      xLineOffs += floorf((sc.size.x - lineWidth) * 0.5f);

    auto iterateBlocks = [&]() {
      for (int i = 0; i < numBlocks; ++i)
      {
        textlayout::TextBlock *block = line.blocks[i];
        darg::GuiTextCache &guiText = block->guiText;

        float x = block->position.x + xLineOffs;
        float y = block->position.y + fmtText->yOffset + line.baseLineY;

#if 0 // debug blocks
        ctx.set_color(color_apply_mods(color, 0.5, 1.0));

        int ascent = StdGuiRender::get_font_ascent(block->fontId, block->fontHt);
        ctx.render_frame(sc.screenPos.x - sc.scrollOffs.x + x, sc.screenPos.y - sc.scrollOffs.y + y - ascent,
          sc.screenPos.x - sc.scrollOffs.x + x + block->size.x, sc.screenPos.y - sc.scrollOffs.y + y - ascent + block->size.y, 1);

        ctx.set_color(color);
#endif

        if (block->type != textlayout::TextBlock::TBT_TEXT) // otherwise don't neeed to draw anything
          continue;

        if (block->useCustomColor)
          ctx.set_color(color_apply_mods(block->customColor, render_state.opacity, params->brightness));

        if (block->fontId != lastFontId || block->fontHt != lastFontHt)
        {
          lastFontId = block->fontId;
          lastFontHt = block->fontHt;
          monoWidth = font_mono_width_from_sq(block->fontId, block->fontHt, objMonoWidth);
        }

        ctx.set_font(block->fontId, spacing, monoWidth);
        if (block->fontHt)
          ctx.set_font_ht(block->fontHt);

        ctx.goto_xy(sc.screenPos.x - sc.scrollOffs.x + x, sc.screenPos.y - sc.scrollOffs.y + y);

        if (shouldBreakWithEllipsis && (ellipsisOnSeparateLine || i > lastBlockIndexToRender))
        {
          ctx.draw_str_u(ellipsisUtf16, 1);
          break;
        }

        bool all_glyphs_ready = true;
        if (!guiText.isReady())
        {
          tmpU16.resize(block->text.length() + 1);
          tmpU16.resize(utf8_to_wcs_ex(block->text.c_str(), tmpU16.size() - 1, tmpU16.data(), tmpU16.size()));
          all_glyphs_ready = ctx.draw_str_scaled_u_buf(guiText.v, guiText.c, guiText.samplers, StdGuiRender::DSBFLAG_rel, 1.0f,
            tmpU16.data(), tmpU16.size());
        }
        nowHaveAllSymbols &= all_glyphs_ready;
        if (all_glyphs_ready && hadAllSymbols)
        {
          ctx.goto_xy(sc.screenPos.x - sc.scrollOffs.x + x, sc.screenPos.y - sc.scrollOffs.y + y);
          ctx.start_font_str(1.0f);
          ctx.render_str_buf(guiText.v, guiText.c, guiText.samplers,
            StdGuiRender::DSBFLAG_rel | StdGuiRender::DSBFLAG_curColor | StdGuiRender::DSBFLAG_checkVis);
        }
        if (!all_glyphs_ready)
          guiText.discard();

        if (block->useCustomColor)
          ctx.set_color(color);
      }
    };

    iterateBlocks();
    if (!hadAllSymbols && nowHaveAllSymbols)
    {
      hadAllSymbols = true;
      iterateBlocks();
    }
  }

  ctx.reset_draw_str_attr();

  // draw cursor
  bool hasFocus = ((elem->getStateFlags() & Element::S_KB_FOCUS) != 0);
  if (hasFocus && etext)
  {
    textlayout::FormatParams fmtParams = {};
    BhvTextAreaEdit::fill_format_params(elem, elem->screenCoord.size, fmtParams);

    StdGuiFontContext fctx;
    StdGuiRender::get_font_context(fctx, fmtParams.defFontId, fmtParams.spacing, fmtParams.monoWidth, fmtParams.defFontHt);
    int ascent = StdGuiRender::get_font_ascent(fctx);
    int descent = StdGuiRender::get_font_descent(fctx);

    int relPos = -1;
    int curBlockIdx = BhvTextAreaEdit::find_block_left(fmtText, etext->cursorPos, relPos);

    float screenX = 0, screenY = 0;
    if (curBlockIdx >= 0)
    {
      float inBlockPixelPos = 0;
      textlayout::TextBlock *curBlock = fmtText->blocks[curBlockIdx];
      if (curBlock->type == textlayout::TextBlock::TBT_TEXT)
      {
        int relPosBytes = utf_calc_bytes_for_symbols(curBlock->text.c_str(), curBlock->text.length(), relPos);
        BBox2 bbox = StdGuiRender::get_str_bbox(curBlock->text.c_str(), relPosBytes, fctx);
        inBlockPixelPos = (bbox.isempty() ? 0 : bbox.right()) + fmtParams.spacing * 0.5f;
      }
      else
      {
        inBlockPixelPos = curBlock->size.x;
      }

      // TODO: padding? per-line baseline?
      screenX = floorf(sc.screenPos.x + curBlock->position.x + inBlockPixelPos - sc.scrollOffs.x);
      screenY = floorf(sc.screenPos.y + curBlock->position.y + fmtText->yOffset - sc.scrollOffs.y);
    }
    else
    {
      screenX = floorf(sc.screenPos.x + 1 - sc.scrollOffs.x);
      screenY = floorf(sc.screenPos.y + fmtText->yOffset - sc.scrollOffs.y);
    }

    ctx.set_color(params->color);
    ctx.draw_line(screenX, screenY, screenX, screenY + ascent + descent, 2);
  }

  scenerender::restoreTransformedViewPort(ctx);

#if 0 // debug internal state
  {
    if (etext)
    {
      String msg(0, "cursor pos: %d", etext->cursorPos);
      ctx.set_font(0);
      BBox2 box = StdGuiRender::get_str_bbox(msg.c_str(), msg.length(), ctx.curRenderFont);
      Point2 screenSize = StdGuiRender::screen_size();
      ctx.set_color(200, 200, 0, 200);
      ctx.goto_xy(screenSize.x - box.width().x - 10, 30 + 2 * StdGuiRender::get_font_ascent(0));
      ctx.draw_str(msg.c_str(), msg.length());
    }
  }
#endif
}


class RobjParamsFrame : public RendObjParams
{
public:
  E3DCOLOR color;
  Point4 borderWidth;
  E3DCOLOR fillColor;
  float brightness = 1.0f;

  virtual bool load(const Element *elem) override
  {
    color = elem->props.getColor(elem->csk->color);
    fillColor = elem->props.getColor(elem->csk->fillColor, E3DCOLOR(0, 0, 0, 0));
    brightness = elem->props.getFloat(elem->csk->brightness, 1.0f);

    const Sqrat::Table &scriptDesc = elem->props.scriptDesc;
    Sqrat::Object bWidthObj = scriptDesc.RawGetSlot(elem->csk->borderWidth);
    const char *errMsg = nullptr;
    if (bWidthObj.IsNull())
    {
      borderWidth[0] = borderWidth[1] = borderWidth[2] = borderWidth[3] = 1;
    }
    else if (!script_parse_offsets(elem, bWidthObj, &borderWidth.x, &errMsg))
    {
      borderWidth[0] = borderWidth[1] = borderWidth[2] = borderWidth[3] = 1;
      darg_assert_trace_var(errMsg, scriptDesc, elem->csk->borderWidth);
    }

    for (int i = 0; i < 4; ++i)
      if (borderWidth[i] < 0)
        borderWidth[i] = 0;

    return true;
  }

  virtual bool getAnimColor(AnimProp prop, E3DCOLOR **ptr) override
  {
    if (prop == AP_COLOR)
    {
      *ptr = &color;
      return true;
    }
    if (prop == AP_FILL_COLOR)
    {
      *ptr = &fillColor;
      return true;
    }
    return false;
  }

  virtual bool getAnimFloat(AnimProp prop, float **ptr) override
  {
    if (prop == AP_BRIGHTNESS)
    {
      *ptr = &brightness;
      return true;
    }
    return false;
  }
};


void RenderObjectFrame::render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  G_UNUSED(elem);
  RobjParamsFrame *params = static_cast<RobjParamsFrame *>(rdata->params);
  G_ASSERT(params);
  if (!params)
    return;

  E3DCOLOR color = color_apply_mods(params->color, render_state.opacity, params->brightness);

  ctx.set_color(color);
  ctx.reset_textures();

  Point2 lt = rdata->pos;
  Point2 rb = lt + rdata->size;

#define T 0
#define R 1
#define B 2
#define L 3

  float *borderWidth = &params->borderWidth.x;

  if (borderWidth[T] + borderWidth[B] > rdata->size.y || borderWidth[L] + borderWidth[R] > rdata->size.x)
  {
    ctx.render_box(lt, rb);
  }
  else
  {
    if (borderWidth[T] > 0)
      ctx.render_box(lt.x, lt.y, rb.x, lt.y + borderWidth[T]);
    if (borderWidth[B] > 0)
      ctx.render_box(lt.x, rb.y - borderWidth[B], rb.x, rb.y);
    if (borderWidth[L] > 0)
      ctx.render_box(lt.x, lt.y + borderWidth[T], lt.x + borderWidth[L], rb.y - borderWidth[B]);
    if (borderWidth[R] > 0)
      ctx.render_box(rb.x - borderWidth[R], lt.y + borderWidth[T], rb.x, rb.y - borderWidth[B]);


    if (params->fillColor.u)
    {
      E3DCOLOR fillColor = color_apply_mods(params->fillColor, render_state.opacity, params->brightness);

      ctx.set_color(fillColor);
      ctx.reset_textures();
      ctx.render_box(lt.x + borderWidth[L], lt.y + borderWidth[T], rb.x - borderWidth[R], rb.y - borderWidth[B]);
    }
  }


#undef T
#undef R
#undef B
#undef L
}


bool RobjParamsBox::load(const Element *elem)
{
  borderColor = elem->props.getColor(elem->csk->borderColor);
  fillColor = elem->props.getColor(elem->csk->fillColor, E3DCOLOR(0, 0, 0, 0));
  image = elem->props.getPicture(elem->csk->image);
  keepAspect = resolve_keep_aspect(elem->props.scriptDesc.RawGetSlot(elem->csk->keepAspect));
  imageHalign = elem->props.getInt<ElemAlign>(elem->csk->imageHalign, ALIGN_CENTER);
  imageValign = elem->props.getInt<ElemAlign>(elem->csk->imageValign, ALIGN_CENTER);
  fallbackImage = elem->props.getPicture(elem->csk->fallbackImage);
  flipX = elem->props.getBool(elem->csk->flipX);
  flipY = elem->props.getBool(elem->csk->flipY);
  saturateFactor = elem->props.getFloat(elem->csk->picSaturate, 1.0f);
  brightness = elem->props.getFloat(elem->csk->brightness, 1.0f);

  const char *errMsg = nullptr;

  const Sqrat::Table &scriptDesc = elem->props.scriptDesc;

  Sqrat::Object bWidthObj = scriptDesc.RawGetSlot(elem->csk->borderWidth);
  if (!script_parse_offsets(elem, bWidthObj, &borderWidth.x, &errMsg))
  {
    borderWidth[0] = borderWidth[1] = borderWidth[2] = borderWidth[3] = 1;
    darg_assert_trace_var(errMsg, scriptDesc, elem->csk->borderWidth);
  }

  for (int i = 0; i < 4; ++i)
    if (borderWidth[i] < 0)
      borderWidth[i] = 0;

  Sqrat::Object bRoundingObj = scriptDesc.RawGetSlot(elem->csk->borderRadius);
  if (!script_parse_offsets(elem, bRoundingObj, &borderRadius.x, &errMsg))
  {
    borderRadius[0] = borderRadius[1] = borderRadius[2] = borderRadius[3] = 0;
    darg_assert_trace_var(errMsg, scriptDesc, elem->csk->borderRadius);
  }

  for (int i = 0; i < 4; ++i)
    if (borderRadius[i] < 0)
      borderRadius[i] = 0;

  return true;
}


bool RobjParamsBox::getAnimColor(AnimProp prop, E3DCOLOR **ptr)
{
  if (prop == AP_BORDER_COLOR)
  {
    *ptr = &borderColor;
    return true;
  }
  if (prop == AP_FILL_COLOR)
  {
    *ptr = &fillColor;
    return true;
  }
  return false;
}


bool RobjParamsBox::getAnimFloat(AnimProp prop, float **ptr)
{
  if (prop == darg::AP_PICSATURATE)
  {
    *ptr = &saturateFactor;
    return true;
  }
  if (prop == darg::AP_BRIGHTNESS)
  {
    *ptr = &brightness;
    return true;
  }
  return false;
}


// consider removing this
void RenderObjectBox::renderNoRadius(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  G_UNUSED(elem);

  RobjParamsBox *params = static_cast<RobjParamsBox *>(rdata->params);
  G_ASSERT(params);
  if (!params)
    return;

  E3DCOLOR borderColor = color_apply_mods(params->borderColor, render_state.opacity, params->brightness);

  Point2 lt = rdata->pos;
  Point2 rb = lt + rdata->size;

#define T 0
#define R 1
#define B 2
#define L 3

  const Point4 &borderWidth = params->borderWidth;

  if (borderWidth[T] + borderWidth[B] > rdata->size.y || borderWidth[L] + borderWidth[R] > rdata->size.x)
  {
    if (params->borderColor.u || ctx.get_alpha_blend() == NO_BLEND)
    {
      ctx.set_color(borderColor);
      ctx.reset_textures();
      ctx.render_box(lt, rb);
    }
  }
  else
  {
    if (params->borderColor.u || ctx.get_alpha_blend() == NO_BLEND)
    {
      ctx.set_color(borderColor);
      ctx.reset_textures();
      if (borderWidth[T] > 0)
        ctx.render_box(lt.x, lt.y, rb.x, lt.y + borderWidth[T]);
      if (borderWidth[B] > 0)
        ctx.render_box(lt.x, rb.y - borderWidth[B], rb.x, rb.y);
      if (borderWidth[L] > 0)
        ctx.render_box(lt.x, lt.y + borderWidth[T], lt.x + borderWidth[L], rb.y - borderWidth[B]);
      if (borderWidth[R] > 0)
        ctx.render_box(rb.x - borderWidth[R], lt.y + borderWidth[T], rb.x, rb.y - borderWidth[B]);
    }

    if (params->fillColor.u)
    {
      E3DCOLOR fillColor = color_apply_mods(params->fillColor, render_state.opacity, params->brightness);
      ctx.set_color(fillColor);

      Picture *image = params->image;
      if (image)
      {
        if (image->getPic().pic == BAD_PICTUREID && !image->isLoading() && params->fallbackImage)
          image = params->fallbackImage;
        const PictureManager::PicDesc &pic = image->getPic();
        pic.updateTc();
        ctx.set_texture(pic.tex, pic.smp);
        ctx.set_alpha_blend(image->getBlendMode());
        if (params->saturateFactor != 1.0f)
          ctx.set_picquad_color_matrix_saturate(params->saturateFactor);

        Point2 tc0 = pic.tcLt, tc1 = pic.tcRb;
        if (params->flipX)
          swap_vals(tc0.x, tc1.x);
        if (params->flipY)
          swap_vals(tc0.y, tc1.y);

        ctx.render_rect_t(lt.x + borderWidth[L], lt.y + borderWidth[T], rb.x - borderWidth[R], rb.y - borderWidth[B], tc0, tc1);
        if (params->saturateFactor != 1.0f)
          ctx.reset_picquad_color_matrix();
      }
      else
      {
        ctx.reset_textures();
        ctx.render_box(lt.x + borderWidth[L], lt.y + borderWidth[T], rb.x - borderWidth[R], rb.y - borderWidth[B]);
      }
    }
  }


#undef T
#undef R
#undef B
#undef L
}


static bool has_radius(const Point4 &border_radius)
{
  for (int i = 0; i < 4; ++i)
    if (border_radius[i] > 0)
      return true;
  return false;
}


void RenderObjectBox::render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  RobjParamsBox *params = static_cast<RobjParamsBox *>(rdata->params);
  G_ASSERT(params);
  if (!params)
    return;

  if (!has_radius(params->borderRadius))
  {
    renderNoRadius(ctx, elem, rdata, render_state);
    return;
  }

  E3DCOLOR borderColor = color_apply_mods(params->borderColor, render_state.opacity, params->brightness);

  Point2 lt = rdata->pos;
  Point2 rb = lt + rdata->size;

  E3DCOLOR fillColor = color_apply_mods(params->fillColor, render_state.opacity, params->brightness);
  Picture *image = params->image;
  const Point4 &radius = params->borderRadius;
  const float thickness = params->borderWidth[0];

#if DAGOR_DBGLEVEL > 0
  const Point4 &border = params->borderWidth;
  if (border.x != border.y || border.z != border.w || border.x != border.z)
  {
    String err;
    err.printf(128, "rounded box can only be rendered with fixed frame thickness (cur borderWidth = %@)", border);
    darg_assert_trace_var(err, //"rounded box can only be rendered with fixed frame thickness(borderWidth)",
      elem->props.scriptDesc, elem->csk->borderWidth);
  }
#endif

  if (image)
  {
    if (image->getPic().pic == BAD_PICTUREID && !image->isLoading() && params->fallbackImage)
      image = params->fallbackImage;
    const PictureManager::PicDesc &pic = image->getPic();

    pic.updateTc();
    ctx.set_texture(pic.tex, pic.smp);
    ctx.set_alpha_blend(image->getBlendMode() == NO_BLEND ? PREMULTIPLIED : image->getBlendMode());
    Point2 picLt = lt, picRb = rb, picLtTc = pic.tcLt, picRbTc = pic.tcRb;

    if (borderColor.u) // otherwise image can be a bit over it's frame, due to AA
    {
      const float adjThickness = max(1.f, thickness / 2) - 0.5f;
      const Point2 th(adjThickness, adjThickness);
      picLt += th;
      picRb -= th;
      // we also have to adjust it's tc now
      const Point2 origTcW = picRbTc - picLtTc, origW = rb - lt;
      const Point2 halfTcW = 0.5f * origTcW - mul(Point2(safediv(adjThickness, origW.x), safediv(adjThickness, origW.y)), origTcW);
      const Point2 tcC = 0.5f * (picRbTc + picLtTc);
      picLtTc = tcC - halfTcW;
      picRbTc = tcC + halfTcW;

      if (params->flipX)
        swap_vals(picLtTc.x, picRbTc.x);
      if (params->flipY)
        swap_vals(picLtTc.y, picRbTc.y);
    }

    const Point2 picSz = PictureManager::get_picture_pix_size(pic.pic);
    adjust_coord_for_image_aspect(picLt, picRb, picLtTc, picRbTc, rdata->pos, rdata->size, picSz, params->keepAspect,
      params->imageHalign, params->imageValign);

    ctx.render_rounded_image(picLt, picRb, picLtTc, picRbTc, fillColor, radius);

    ctx.reset_textures();
    ctx.set_alpha_blend(PREMULTIPLIED);
    ctx.render_rounded_frame(lt, rb, borderColor, radius, thickness);
  }
  else
  {
    ctx.reset_textures();
    ctx.set_alpha_blend(PREMULTIPLIED);
    ctx.render_rounded_box(lt, rb, fillColor, borderColor, radius, thickness);
  }
}


bool RobjParamsMovie::load(const Element *elem)
{
  color = elem->props.getColor(elem->csk->color);
  saturateFactor = elem->props.getFloat(elem->csk->picSaturate, 1.0f);
  brightness = elem->props.getFloat(elem->csk->brightness, 1.0f);
  keepAspect = resolve_keep_aspect(elem->props.scriptDesc.RawGetSlot(elem->csk->keepAspect));
  imageHalign = elem->props.getInt<ElemAlign>(elem->csk->imageHalign, ALIGN_CENTER);
  imageValign = elem->props.getInt<ElemAlign>(elem->csk->imageValign, ALIGN_CENTER);
  return true;
}

bool RobjParamsMovie::getAnimColor(AnimProp prop, E3DCOLOR **ptr)
{
  if (prop == AP_COLOR)
  {
    *ptr = &color;
    return true;
  }
  return false;
}

bool RobjParamsMovie::getAnimFloat(AnimProp prop, float **ptr)
{
  if (prop == darg::AP_PICSATURATE)
  {
    *ptr = &saturateFactor;
    return true;
  }
  if (prop == darg::AP_BRIGHTNESS)
  {
    *ptr = &brightness;
    return true;
  }
  return false;
}


void RenderObjectMovie::render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
  const RenderState &render_state)
{
  G_UNUSED(ctx);
  IGenVideoPlayer *player = elem->props.storage.RawGetSlotValue<IGenVideoPlayer *>(elem->csk->moviePlayer, nullptr);
  if (!player)
    return;
  RobjParamsMovie *params = static_cast<RobjParamsMovie *>(rdata->params);
  G_ASSERT_RETURN(params, );

  GuiScene *scene = GuiScene::get_from_elem(elem);
  YuvRenderer &yuvRenderer = scene->yuvRenderer;

  TEXTUREID texIdY, texIdU, texIdV;
  d3d::SamplerHandle smp;
  if (player->getFrame(texIdY, texIdU, texIdV, smp))
  {
    const Offsets &padding = elem->layout.padding;

    Point2 lt = rdata->pos + padding.lt();
    Point2 rb = rdata->pos + rdata->size - padding.rb();

    IPoint2 frameSize = player->getFrameSize();

    if (lt.x < rb.x && lt.y < rb.y && frameSize.x > 0 && frameSize.y > 0)
    {
      E3DCOLOR color(params->color.r, params->color.g, params->color.b, ::clamp(int(params->color.a * render_state.opacity), 0, 255));
      Point2 tcLt(0, 0);
      Point2 tcRb(1, 1);

      adjust_coord_for_image_aspect(lt, rb, tcLt, tcRb, lt, rb - lt, frameSize, params->keepAspect, params->imageHalign,
        params->imageValign);

      bool ownRender;
      yuvRenderer.startRender(ctx, &ownRender);
      yuvRenderer.render(ctx, texIdY, texIdU, texIdV, smp, lt.x, lt.y, rb.x, rb.y, tcLt, tcRb, color,
        color.a == 255 ? NO_BLEND : PREMULTIPLIED, params->saturateFactor);
      ctx.execCommand(
        5309, ZERO<Point2>(), ZERO<Point2>(),
        +[](StdGuiRender::CallBackState &cbs) {
          G_FAST_ASSERT(cbs.command == 5309);
          ((IGenVideoPlayer *)cbs.data)->onCurrentFrameDispatched();
        },
        uintptr_t(player));
      yuvRenderer.endRender(ctx, ownRender);
    }
  }
  else if (!player->isFinished())
    LOGWARN_CTX("getFrame()=false, ids=%d,%d,%d", texIdY, texIdU, texIdV);
}


ROBJ_FACTORY_IMPL(RenderObjectSolid, RobjParamsColorOnly)
ROBJ_FACTORY_IMPL(RenderObjectDebug, RobjParamsColorOnly)
ROBJ_FACTORY_IMPL(RenderObjectText, RobjParamsText)
ROBJ_FACTORY_IMPL(RenderObjectInscription, RobjParamsInscription)
ROBJ_FACTORY_IMPL(RenderObjectImage, RobjParamsImage)
ROBJ_FACTORY_IMPL(RenderObjectVectorCanvas, RobjParamsVectorCanvas)
ROBJ_FACTORY_IMPL(RenderObject9rect, RobjParams9rect)
ROBJ_FACTORY_IMPL(RenderObjectProgressLinear, RobjParamsProgressLinear)
ROBJ_FACTORY_IMPL(RenderObjectProgressCircular, RobjParamsProgressCircular)
ROBJ_FACTORY_IMPL(RenderObjectTextArea, RobjParamsTextArea)
ROBJ_FACTORY_IMPL(RenderObjectFrame, RobjParamsFrame)
ROBJ_FACTORY_IMPL(RenderObjectBox, RobjParamsBox)
ROBJ_FACTORY_IMPL(RobjMask, RobjMaskParams)
ROBJ_FACTORY_IMPL(RobjDasCanvas, RobjDasCanvasParams)

static class RobjFactory_RenderObjectMovie final : public darg::RendObjFactory
{
  virtual darg::RenderObject *createRenderObject(void *stor) const override
  {
    if (Ptr<ShaderMaterial> shMat = new_shader_material_by_name_optional(YuvRenderer::shaderName))
      return new (stor, _NEW_INPLACE) RenderObjectMovie();
    logerr("Failed to init YUV renderer: shader <%s> not found", YuvRenderer::shaderName);
    return NULL;
  }
  virtual darg::RendObjParams *createRendObjParams() const override { return new RobjParamsMovie(); }
} robj_factory__RenderObjectMovie;

bool rendobj_inited = false;

int rendobj_text_id = -1;
int rendobj_inscription_id = -1;
int rendobj_textarea_id = -1;
int rendobj_image_id = -1;
int rendobj_world_blur_id = -1;

void register_std_rendobj_factories()
{
  if (rendobj_inited)
    return;

  rendobj_inited = true;

  G_ASSERT(rendobj_text_id < 0);
  G_ASSERT(rendobj_inscription_id < 0);
  G_ASSERT(rendobj_textarea_id < 0);
  G_ASSERT(rendobj_image_id < 0);

#define RF(name, cls) add_rendobj_factory(#name, ROBJ_FACTORY_PTR(cls))

  RF(ROBJ_SOLID, RenderObjectSolid);
  RF(ROBJ_DEBUG, RenderObjectDebug);
  rendobj_text_id = RF(ROBJ_TEXT, RenderObjectText);
  rendobj_inscription_id = RF(ROBJ_INSCRIPTION, RenderObjectInscription);
  rendobj_image_id = RF(ROBJ_IMAGE, RenderObjectImage);
  RF(ROBJ_VECTOR_CANVAS, RenderObjectVectorCanvas);
  RF(ROBJ_9RECT, RenderObject9rect);
  RF(ROBJ_PROGRESS_LINEAR, RenderObjectProgressLinear);
  RF(ROBJ_PROGRESS_CIRCULAR, RenderObjectProgressCircular);
  rendobj_textarea_id = RF(ROBJ_TEXTAREA, RenderObjectTextArea);
  RF(ROBJ_FRAME, RenderObjectFrame);
  RF(ROBJ_BOX, RenderObjectBox);
  RF(ROBJ_MASK, RobjMask);
  RF(ROBJ_MOVIE, RenderObjectMovie);
  RF(ROBJ_DAS_CANVAS, RobjDasCanvas);

#undef RF
}


} // namespace darg
