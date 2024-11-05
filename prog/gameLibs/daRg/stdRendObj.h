// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_renderObject.h>
#include <daRg/dag_stdRendObj.h>
#include <daRg/dag_guiConstants.h>
#include <gui/dag_stdGuiRender.h>

namespace darg
{

class Picture;


class RenderObjectSolid : public RenderObject
{
  virtual void render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *, const RenderState &render_state);
};

class RenderObjectDebug : public RenderObject
{
  virtual void render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *, const RenderState &render_state);
};

struct GuiTextCache
{
  SmallTab<GuiVertex> v;
  SmallTab<uint16_t> c;

  void discard()
  {
    v.resize(0);
    c.resize(0);
  }
  bool isReady() const { return StdGuiRender::check_str_buf_ready(c); }
};

class RobjParamsText : public RendObjParams
{
public:
  E3DCOLOR color = 0;
  float brightness = 1.0f;
  int fontId = 0;
  int fontHt = 0;
  int spacing = 0;
  int monoWidth = 0;
  wchar_t passChar = 0;
  bool ellipsis = true;

  TextOverflow overflowX = TOVERFLOW_CLIP;
  FontFxType fontFx = FFT_NONE;
  E3DCOLOR fxColor = 0;
  int fxFactor = 48, fxOffsX = 0, fxOffsY = 0;
  Picture *fontTex = nullptr;
  int fontTexSu = 32, fontTexSv = 32, fontTexBov = 0;
  int strWidth = 0;
  GuiTextCache guiText;

  virtual bool load(const Element *elem) override;
  virtual bool getAnimColor(AnimProp prop, E3DCOLOR **ptr) override;
  virtual bool getAnimFloat(AnimProp prop, float **ptr) override;
  virtual void discardTextCache() { guiText.discard(); }
};


class RobjParamsTextArea : public RobjParamsColorOnly
{
public:
  TextOverflow overflowY = TOVERFLOW_CLIP;
  bool ellipsis = true;
  bool ellipsisSepLine = false;

  int lowLineCount = 0;
  ElemAlign lowLineCountAlign = PLACE_DEFAULT;

  FontFxType fontFx = FFT_NONE;
  E3DCOLOR fxColor = 0;
  int fxFactor = 48, fxOffsX = 0, fxOffsY = 0;

  virtual bool load(const Element *elem) override;
};


class RenderObjectText : public RenderObject
{
  virtual void render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *, const RenderState &render_state);
};


class RobjParamsInscription : public RendObjParams
{
public:
  E3DCOLOR color = 0;
  int fontId = 0;
  float fontSize = 0;
  float fullHeight = 0;
  float brightness = 1.0f;

  uint32_t inscription = 0;

  FontFxType fontFx = FFT_NONE;
  E3DCOLOR fxColor = E3DCOLOR(255, 255, 255, 255);
  int fxFactor = 48, fxOffsX = 0, fxOffsY = 0;
  Picture *fontTex = nullptr;
  int fontTexSu = 32, fontTexSv = 32, fontTexBov = 0;

  ~RobjParamsInscription();

  virtual bool load(const Element *elem) override;
  virtual bool getAnimColor(AnimProp prop, E3DCOLOR **ptr) override;
  virtual bool getAnimFloat(AnimProp prop, float **ptr) override;

  void rebuildInscription(const Element *elem);
};

class RenderObjectInscription : public RenderObject
{
  virtual void render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *, const RenderState &render_state);
};


class RobjParamsImage : public RendObjParams
{
public:
  E3DCOLOR color = E3DCOLOR(255, 255, 255);
  Picture *image = nullptr;
  Picture *fallbackImage = nullptr;
  KeepAspectMode keepAspect = KEEP_ASPECT_NONE;
  ElemAlign imageHalign = ALIGN_CENTER;
  ElemAlign imageValign = ALIGN_CENTER;
  bool flipX = false;
  bool flipY = false;
  bool imageAffectsLayout = false;

  float saturateFactor = 1.0f;
  float brightness = 1.0f;

  virtual bool load(const Element *elem) override;
  virtual bool getAnimColor(AnimProp prop, E3DCOLOR **ptr) override;
  virtual bool getAnimFloat(AnimProp prop, float **ptr) override;
};

class RenderObjectImage : public RenderObject
{
  virtual void render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *, const RenderState &render_state);
};


class RenderObject9rect : public RenderObject
{
  virtual void render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *, const RenderState &render_state);
};


class RenderObjectProgressLinear : public RenderObject
{
  virtual void render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *, const RenderState &render_state);
  void drawRect(StdGuiRender::GuiContext &ctx, const Element *elem, float val, E3DCOLOR color);
};


class RenderObjectProgressCircular : public RenderObject
{
  virtual void render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *, const RenderState &render_state);
  void renderProgress(StdGuiRender::GuiContext &ctx, const ElemRenderData *rdata, float val, float dir, Picture *image,
    E3DCOLOR color);
};


class RenderObjectTextArea : public RenderObject
{
  virtual void render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *, const RenderState &render_state);
};


class RenderObjectFrame : public RenderObject
{
  virtual void render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *, const RenderState &render_state);
};


class RobjParamsMovie : public RendObjParams
{
public:
  E3DCOLOR color = E3DCOLOR(255, 255, 255);
  float saturateFactor = 1.0f;
  float brightness = 1.0f;
  KeepAspectMode keepAspect = KEEP_ASPECT_NONE;
  ElemAlign imageHalign = ALIGN_CENTER;
  ElemAlign imageValign = ALIGN_CENTER;

  virtual bool load(const Element *elem) override;
  virtual bool getAnimColor(AnimProp prop, E3DCOLOR **ptr) override;
  virtual bool getAnimFloat(AnimProp prop, float **ptr) override;
};

class RenderObjectMovie : public RenderObject
{
public:
  virtual void render(StdGuiRender::GuiContext &ctx, const Element *elem, const ElemRenderData *rdata,
    const RenderState &render_state) override;
};


} // namespace darg
