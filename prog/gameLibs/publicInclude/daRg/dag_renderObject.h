//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_e3dColor.h>

#include <sqrat.h>

#include <daRg/dag_animation.h>


struct GuiVertexTransform;

namespace StdGuiRender
{
class GuiContext;
}


namespace darg
{

class Element;
class RendObjParams;
struct RenderState;


struct ElemRenderData
{
  Point2 pos;
  Point2 size;
  RendObjParams *params;
};


class RenderObject
{
public:
  RenderObject() = default;
  virtual ~RenderObject() = default;

  virtual void render(StdGuiRender::GuiContext &ctx, const Element *, const ElemRenderData *, const RenderState &render_state) = 0;
  virtual void postRender(StdGuiRender::GuiContext &, const Element *) {}
};


class RendObjParams
{
public:
  virtual ~RendObjParams(){};
  virtual bool load(const Element *elem) = 0;

  virtual bool getAnimFloat(AnimProp, float **) { return false; }
  virtual bool getAnimColor(AnimProp, E3DCOLOR **) { return false; }
  virtual bool getAnimPoint2(AnimProp, Point2 **) { return false; }
  virtual void discardTextCache() {}
};


class RendObjEmptyParams : public RendObjParams
{
public:
  virtual bool load(const Element * /*elem*/) override { return true; }
};


class RobjParamsColorOnly : public RendObjParams
{
public:
  static const E3DCOLOR defColor;
  E3DCOLOR color = defColor;
  float brightness = 1.0f;

  bool load(const Element *elem, E3DCOLOR def_color);

  virtual bool load(const Element *elem) override;
  virtual bool getAnimColor(AnimProp prop, E3DCOLOR **ptr) override;
  virtual bool getAnimFloat(AnimProp, float **) override;
};


class RendObjFactory
{
public:
  virtual RenderObject *createRenderObject(void * /*stor*/) const = 0;
  virtual RendObjParams *createRendObjParams() const = 0;
};


static constexpr int ROBJ_NONE = -1;


#define ROBJ_FACTORY_PTR(ro_class) &robj_factory__##ro_class

#define ROBJ_FACTORY_IMPL(ro_class, params_class)                                                \
  static_assert(sizeof(ro_class) == sizeof(void *), "darg::RenderObject shouldn't have state!"); \
  static class RobjFactory_##ro_class final : public darg::RendObjFactory                        \
  {                                                                                              \
    virtual darg::RenderObject *createRenderObject(void *stor) const override                    \
    {                                                                                            \
      return new (stor, _NEW_INPLACE) ro_class();                                                \
    }                                                                                            \
    virtual darg::RendObjParams *createRendObjParams() const override                            \
    {                                                                                            \
      return new params_class();                                                                 \
    }                                                                                            \
  } robj_factory__##ro_class;


int add_rendobj_factory(const char *name, RendObjFactory *);
int find_rendobj_factory_id(const char *name);
RendObjFactory *get_rendobj_factory(int id);
RenderObject *create_rendobj(int id, void *stor);
RendObjParams *create_robj_params(int id);

extern int rendobj_text_id;
extern int rendobj_inscription_id;
extern int rendobj_textarea_id;
extern int rendobj_image_id;
extern int rendobj_world_blur_id;

void register_std_rendobj_factories();
void register_rendobj_script_ids(HSQUIRRELVM vm);
void register_blur_rendobj_factories(bool wt_compatibility_mode);
void register_blur_stubs();

static inline void discard_text_cache(RendObjParams *robjParams)
{
  if (robjParams)
    robjParams->discardTextCache();
}

} // namespace darg
