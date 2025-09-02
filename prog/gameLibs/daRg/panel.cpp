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

void PanelRenderInfo::resetPointer()
{
  pointerTexture[0] = BAD_TEXTUREID;
  pointerTexture[1] = BAD_TEXTUREID;
  pointerUVLeftTop[0].set(-1, -1);
  pointerUVLeftTop[1].set(-1, -1);
  pointerUVInvSize[0].set(0, 0);
  pointerUVInvSize[1].set(0, 0);
  pointerColor[0] = 0;
  pointerColor[1] = 0;
}


Panel::Panel(GuiScene *scene_) : scene(scene_) {}


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


void Panel::onRemoveCursor(Cursor *cursor)
{
  for (PanelPointer &ptr : pointers)
    if (ptr.cursor == cursor)
      ptr.cursor = nullptr;
}


void Panel::updateSpatialInfoFromScript()
{
  G_ASSERT_RETURN(screen && screen->etree.root, );

  const Sqrat::Table &scriptDesc = screen->etree.root->props.scriptDesc;

  spatialInfo.anchor = PanelAnchor(scriptDesc.RawGetSlotValue("worldAnchor", PanelAnchor::None));
  spatialInfo.constraint =
    PanelRotationConstraint(scriptDesc.RawGetSlotValue("worldRotationConstraint", PanelRotationConstraint::None));
  spatialInfo.geometry = PanelGeometry(scriptDesc.RawGetSlotValue("worldGeometry", PanelGeometry::None));
  spatialInfo.canBePointedAt = scriptDesc.RawGetSlotValue("worldCanBePointedAt", false);
  spatialInfo.canBeTouched = scriptDesc.RawGetSlotValue("worldCanBeTouched", false);

  spatialInfo.position = scriptDesc.RawGetSlotValue("worldOffset", Point3(0, 0, 0));
  spatialInfo.angles = scriptDesc.RawGetSlotValue("worldAngles", Point3(0, 0, 0));
  spatialInfo.headDirOffset = scriptDesc.RawGetSlotValue("headDirOffset", 0.f);

  Point2 size2 = scriptDesc.RawGetSlotValue("worldSize", Point2(0, 0));
  spatialInfo.size.set(size2.x, size2.y, 1);

  spatialInfo.anchorEntityId = scriptDesc.RawGetSlotValue("worldAnchorEntity", 0u);
  spatialInfo.facingEntityId = scriptDesc.RawGetSlotValue("worldFacingEntity", 0u);

  spatialInfo.anchorNodeName = scriptDesc.RawGetSlotValue("worldAnchorEntityNode", eastl::string());
}

void Panel::updateRenderInfoParamsFromScript()
{
  G_ASSERT_RETURN(screen && screen->etree.root, );
  Sqrat::Table &desc = screen->etree.root->props.scriptDesc;
  renderInfo.brightness = desc.RawGetSlotValue("worldBrightness", PanelRenderInfo::def_brightness);
  renderInfo.smoothness = desc.RawGetSlotValue("worldSmoothness", PanelRenderInfo::def_smoothness);
  renderInfo.reflectance = desc.RawGetSlotValue("worldReflectance", PanelRenderInfo::def_reflectance);
  renderInfo.metalness = desc.RawGetSlotValue("worldMetalness", PanelRenderInfo::def_metalness);
  renderInfo.worldRenderFeatures = desc.RawGetSlotValue("worldRenderFeatures", 0);
}


/* *******************************************************/


PanelData::PanelData(GuiScene &scene, const Sqrat::Object &object, int panelIndex)
{
  panel.reset(new Panel(&scene));
  panel->init(panelIndex, object);

  index = panelIndex;

  panel->updateSpatialInfoFromScript();
  if (panel->spatialInfo.anchor != PanelAnchor::None)
    syncCanvas();
}


bool PanelData::getCanvasSize(IPoint2 &size) const
{
  if (!panel->screen || !panel->screen->etree.root)
    return false;
  size = panel->screen->etree.root->props.scriptDesc.RawGetSlotValue("canvasSize", IPoint2(-1, -1));
  return size.x > 0 && size.y > 0;
}


eastl::string PanelData::getPointerPath() const
{
  if (!panel->screen || !panel->screen->etree.root)
    return eastl::string();
  return panel->screen->etree.root->props.scriptDesc.GetSlotValue("worldPointerTexture", eastl::string());
}


Point2 PanelData::getPointerSize() const
{
  const auto defaultValue = Point2(64, 64);
  if (!panel->screen || !panel->screen->etree.root)
    return defaultValue;
  return panel->screen->etree.root->props.scriptDesc.RawGetSlotValue("worldPointerSize", defaultValue);
}


bool PanelData::isAutoUpdated() const
{
  if (!panel->screen || !panel->screen->etree.root)
    return true;
  return panel->screen->etree.root->props.scriptDesc.RawGetSlotValue("isAutoUpdated", true);
}


E3DCOLOR PanelData::getPointerColor() const
{
  auto defaultValue = E3DCOLOR(255, 0, 0);
  if (!panel->screen || !panel->screen->etree.root)
    return defaultValue;
  return panel->screen->etree.root->props.scriptDesc.RawGetSlotValue("worldPointerColor", defaultValue);
}


bool PanelData::isInThisPass(darg_panel_renderer::RenderPass render_pass) const
{
  int features = panel->renderInfo.worldRenderFeatures;

  switch (render_pass)
  {
    case darg_panel_renderer::RenderPass::Translucent: return (features & darg_panel_renderer::RenderFeatures::Opaque) == 0;

    case darg_panel_renderer::RenderPass::TranslucentWithoutDepth:
      return (features & darg_panel_renderer::RenderFeatures::AlwaysOnTop) != 0;

    case darg_panel_renderer::RenderPass::Shadow: return (features & darg_panel_renderer::RenderFeatures::CastShadow) != 0;

    case darg_panel_renderer::RenderPass::GBuffer: return (features & darg_panel_renderer::RenderFeatures::Opaque) != 0;

    default: G_ASSERTF(false, "Implement handling this pass!");
  }

  return true;
}


void PanelData::setCursorStatus(int hand, bool enabled)
{
  if (panel)
  {
    if (hand < 0 || hand >= Panel::MAX_POINTERS)
      G_ASSERTF(0, "Invalid hand %d", hand);
    else
    {
      debug("[DARG][CTRL] %s(hand=%d, en=%d)", __FUNCTION__, hand, enabled);
      PanelPointer &ptr = panel->pointers[hand];
      if (ptr.isPresent != enabled)
      {
        ptr.isPresent = enabled;
        ptr.pos.set(-100, -100);
        panel->updateHover();
      }
    }
  }
}


bool PanelData::needRenderInWorld() const
{
  if (!panel)
    return false;

  if (panel->spatialInfo.anchor == PanelAnchor::None)
    return false;

  if (panel->spatialInfo.geometry == PanelGeometry::None)
    return false;

  if (panel->spatialInfo.renderRedirection)
    return false;

  IPoint2 size;
  return getCanvasSize(size);
}


void PanelData::syncCanvas()
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
    canvas->set(d3d::create_tex(nullptr, targetSize.x, targetSize.y, flags, 1, canvasName.data()));

    if (panel->spatialInfo.canBePointedAt)
    {
      eastl::string currentPointerPath = getPointerPath();
      if (pointerPath != currentPointerPath && pointerTex != BAD_TEXTUREID)
        ::release_managed_tex(pointerTex);
      if (!currentPointerPath.empty())
      {
        pointerTex = ::get_managed_texture_id(currentPointerPath.data());
        pointerPath = currentPointerPath;

        G_ASSERTF(pointerTex != BAD_TEXTUREID, "Pointer specified for panel can't be found: %s", currentPointerPath.data());
      }
    }
  }
  else if (canvas)
  {
    canvas.release();
  }
}


void PanelData::updateTexture(GuiScene &scene, BaseTexture *target)
{
  if (!isAutoUpdated() && !isDirty)
    return;

  TIME_D3D_PROFILE(PanelData__updateTexture);

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

    d3d::set_render_target(target, 0);
    d3d::setview(0, 0, texInfo.w, texInfo.h, 0, 1);
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);

    int savedBlockId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

    scene.refreshGuiContextState();
    scene.buildPanelRender(index);

    // basically, the same as GuiScene::flushRenderImpl() / GuiScene::flushPanelRender()
    scene.getGuiContext()->setTarget();
    StdGuiRender::acquire();
    scene.getGuiContext()->flushData();
    scene.getGuiContext()->end_render();

    ShaderGlobal::setBlock(savedBlockId, ShaderGlobal::LAYER_FRAME);

    isDirty = false;
  }
}


} // namespace darg
