// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "panel.h"
#include "guiScene.h"
#include "cursor.h"
#include "component.h"

#include <daRg/dag_stringKeys.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_panelRenderer.h>

#include <shaders/dag_shaderBlock.h>
#include <perfMon/dag_statDrv.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>


namespace darg
{


Panel::Panel(GuiScene &scene_, const Sqrat::Object &object, int panelIndex) : scene(&scene_), index(panelIndex)
{
  init(panelIndex, object);

  if (spatialInfo.anchor != PanelAnchor::None)
    syncCanvas();
}


Panel::~Panel() { clear(); }


void Panel::clear()
{
  scene->destroyGuiScreen(screen);
  screen = nullptr;
  spatialInfo.lastTransform.forEach([](auto &opt) { opt.reset(); });
}


void Panel::init(int screen_id, const Sqrat::Object &markup)
{
  G_ASSERTF(!screen, "Repeated call to init()");
  screen = scene->createGuiScreen(screen_id);
  G_ASSERTF_RETURN(screen, , "Failed to create gui screen #%d", screen_id);
  screen->isMainScreen = false;
  screen->panel = this;

  Component rootComp;
  Component::build_component(rootComp, markup, scene->getStringKeys(), markup);

  int rebuildResult = 0;
  screen->etree.root =
    screen->etree.rebuild(scene->getScriptVM(), screen->etree.root, rootComp, nullptr, rootComp.scriptBuilder, 0, rebuildResult);
  if (screen->etree.root)
  {
    screen->etree.root->recalcLayout();
    screen->etree.root->callScriptAttach(scene);
  }

  rebuildStacks();
}


void Panel::rebuildStacks()
{
  if (screen)
    screen->rebuildStacks();
}


void Panel::update(float dt)
{
  for (PanelPointer &ptr : pointers)
    if (ptr.cursor)
      ptr.cursor->update(dt);
}


void Panel::updateHover()
{
  SQInteger stickScrollFlags[MAX_POINTERS] = {0};
  Point2 pos[MAX_POINTERS];
  for (int i = 0; i < MAX_POINTERS; ++i)
    pos[i] = pointers[i].isPresent ? pointers[i].pos : Point2(-100, -100);
  if (screen)
    screen->etree.updateHover(screen->inputStack, MAX_POINTERS, pos, stickScrollFlags);
}


void Panel::updateActiveCursors()
{
  if (!screen)
    return;
  for (PanelPointer &ptr : pointers)
  {
    Cursor *prevCursor = ptr.cursor;
    ptr.cursor = nullptr;

    if (ptr.isPresent)
    {
      for (const InputEntry &ie : screen->cursorStack.stack)
      {
        if (ie.elem->hitTest(ptr.pos))
        {
          if (ie.elem->props.getCurrentCursor(scene->getStringKeys()->cursor, &ptr.cursor))
            break;
        }
      }
    }

    if (prevCursor && prevCursor != ptr.cursor)
      prevCursor->etree.skipAllOneShotAnims();
  }
}

bool Panel::hasActiveCursors() const
{
  for (const PanelPointer &ptr : pointers)
    if (ptr.cursor && ptr.isPresent)
      return true;
  return false;
}


void Panel::onRemoveCursor(Cursor *cursor)
{
  for (PanelPointer &ptr : pointers)
    if (ptr.cursor == cursor)
      ptr.cursor = nullptr;
}


void Panel::updatePanelParamsFromScript(const Sqrat::Table &desc)
{
  spatialInfo.anchor = PanelAnchor(desc.RawGetSlotValue("worldAnchor", PanelAnchor::None));
  spatialInfo.constraint = PanelRotationConstraint(desc.RawGetSlotValue("worldRotationConstraint", PanelRotationConstraint::None));
  spatialInfo.geometry = PanelGeometry(desc.RawGetSlotValue("worldGeometry", PanelGeometry::None));
  spatialInfo.canBePointedAt = desc.RawGetSlotValue("worldCanBePointedAt", false);
  spatialInfo.canBeTouched = desc.RawGetSlotValue("worldCanBeTouched", false);
  spatialInfo.shouldBlockVrHandInteractions = desc.RawGetSlotValue("shouldBlockVrHandInteractions", false);
  spatialInfo.allowDisplayCursorProjection = desc.RawGetSlotValue("allowDisplayCursorProjection", true);

  spatialInfo.position = desc.RawGetSlotValue("worldOffset", Point3(0, 0, 0));
  spatialInfo.angles = desc.RawGetSlotValue("worldAngles", Point3(0, 0, 0));
  spatialInfo.headDirOffset = desc.RawGetSlotValue("headDirOffset", 0.f);

  Point2 size2 = desc.RawGetSlotValue("worldSize", Point2(0, 0));
  spatialInfo.size.set(size2.x, size2.y, 1);

  spatialInfo.anchorEntityId = desc.RawGetSlotValue("worldAnchorEntity", 0u);
  spatialInfo.facingEntityId = desc.RawGetSlotValue("worldFacingEntity", 0u);

  spatialInfo.anchorNodeName = desc.RawGetSlotValue("worldAnchorEntityNode", eastl::string());

  renderInfo.brightness = desc.RawGetSlotValue("worldBrightness", PanelRenderInfo::def_brightness);
  renderInfo.smoothness = desc.RawGetSlotValue("worldSmoothness", PanelRenderInfo::def_smoothness);
  renderInfo.reflectance = desc.RawGetSlotValue("worldReflectance", PanelRenderInfo::def_reflectance);
  renderInfo.metalness = desc.RawGetSlotValue("worldMetalness", PanelRenderInfo::def_metalness);
  renderInfo.worldRenderFeatures = desc.RawGetSlotValue("worldRenderFeatures", 0);
}


/* *******************************************************/


bool Panel::getCanvasSize(IPoint2 &size) const
{
  if (!screen || !screen->etree.root)
    return false;
  size = screen->etree.root->props.scriptDesc.RawGetSlotValue("canvasSize", IPoint2(-1, -1));
  return size.x > 0 && size.y > 0;
}


bool Panel::isAutoUpdated() const
{
  if (!screen || !screen->etree.root)
    return true;
  return screen->etree.root->props.scriptDesc.RawGetSlotValue("isAutoUpdated", true);
}


bool Panel::isInThisPass(darg_panel_renderer::RenderPass render_pass) const
{
  int features = renderInfo.worldRenderFeatures;

  switch (render_pass)
  {
    case darg_panel_renderer::RenderPass::Translucent: return (features & darg_panel_renderer::RenderFeatures::Opaque) == 0;

    case darg_panel_renderer::RenderPass::TranslucentWithoutDepth:
      return (features & darg_panel_renderer::RenderFeatures::AlwaysOnTop) != 0;

    case darg_panel_renderer::RenderPass::Shadow: return (features & darg_panel_renderer::RenderFeatures::CastShadow) != 0;

    case darg_panel_renderer::RenderPass::GBuffer: return (features & darg_panel_renderer::RenderFeatures::Opaque) != 0;

    case darg_panel_renderer::RenderPass::ReactiveMask: return true;

    default: G_ASSERTF(false, "Implement handling this pass!");
  }

  return true;
}


void Panel::setCursorState(int hand, bool enabled, Point2 pos)
{
  if (hand < 0 || hand >= MAX_POINTERS)
    G_ASSERTF(0, "Invalid hand %d", hand);
  else
  {
    PanelPointer &ptr = pointers[hand];
    bool needHoverUpdate = (ptr.isPresent != enabled) || (enabled && pos != ptr.pos);
    ptr.isPresent = enabled;
    ptr.pos = pos;
    if (needHoverUpdate)
      updateHover();
  }
}


bool Panel::needRenderInWorld() const
{
  if (spatialInfo.anchor == PanelAnchor::None)
    return false;

  if (spatialInfo.geometry == PanelGeometry::None)
    return false;

  if (spatialInfo.renderRedirection)
    return false;

  IPoint2 size;
  return getCanvasSize(size);
}


void Panel::syncCanvas()
{
  IPoint2 targetSize;
  if (getCanvasSize(targetSize))
  {
    if (canvas)
    {
      TextureInfo texInfo;
      canvas->getTex2D()->getinfo(texInfo);
      if (texInfo.w == targetSize.x && texInfo.h == targetSize.y)
        return;

      canvas->close();
    }
    else
      canvas = eastl::make_unique<TextureIDHolder>();

    eastl::string canvasName;
    canvasName.eastl::string::sprintf("panelCanvas_%d", index);
    const int flags = TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_RTARGET;
    canvas->set(d3d::create_tex(nullptr, targetSize.x, targetSize.y, flags, 1, canvasName.data(), RESTAG_GUI));
  }
  else if (canvas)
  {
    canvas.release();
  }
}


void Panel::updateTexture(GuiScene &gui_scene, BaseTexture *target)
{
  if (!isAutoUpdated() && !isDirty)
    return;

  TIME_D3D_PROFILE(Panel__updateTexture);

  {
    SCOPE_RENDER_TARGET;
    SCOPE_VIEWPORT;

    if (!target)
    {
      syncCanvas();
      target = canvas->getTex2D();
    }

    TextureInfo texInfo;
    target->getinfo(texInfo);

    d3d::set_render_target({}, DepthAccess::RW, {{target, 0, 0}});
    d3d::setview(0, 0, texInfo.w, texInfo.h, 0, 1);
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);

    int savedBlockId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

    gui_scene.refreshGuiContextState();
    gui_scene.buildPanelRender(index);

    // basically, the same as GuiScene::flushRenderImpl() / GuiScene::flushPanelRender()
    gui_scene.getGuiContext()->setTarget();
    StdGuiRender::acquire();
    gui_scene.getGuiContext()->flushData();
    gui_scene.getGuiContext()->end_render();

    ShaderGlobal::setBlock(savedBlockId, ShaderGlobal::LAYER_FRAME);

    isDirty = false;
  }
}


} // namespace darg
