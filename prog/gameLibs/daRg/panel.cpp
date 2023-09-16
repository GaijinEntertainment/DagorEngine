#include "panel.h"
#include "guiScene.h"
#include "cursor.h"
#include "component.h"

#include <daRg/dag_stringKeys.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_panelRenderer.h>

#include <shaders/dag_shaderBlock.h>
#include <perfMon/dag_statDrv.h>


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


Panel::Panel(GuiScene *scene_) : scene(scene_), etree(scene_, nullptr), renderList() {}


Panel::~Panel() { clear(); }


void Panel::clear()
{
  renderList.clear();
  inputStack.clear();
  cursorStack.clear();
  etree.clear();
}


void Panel::init(const Sqrat::Object &markup)
{
  G_ASSERTF(!etree.root, "Repeated call to init()");

  Component rootComp;
  Component::build_component(rootComp, markup, scene->getStringKeys(), markup);

  int rebuildResult = 0;
  etree.root = etree.rebuild(scene->getScriptVM(), etree.root, rootComp, nullptr, rootComp.scriptBuilder, 0, rebuildResult);
  if (etree.root)
  {
    etree.root->recalcLayout();
    etree.root->callScriptAttach(scene);
  }

  rebuildStacks();
}


void Panel::rebuildStacks()
{
  renderList.clear();
  inputStack.clear();
  cursorStack.clear();
  if (etree.root)
  {
    ElemStacks stacks;
    ElemStackCounters counters;
    stacks.rlist = &renderList;
    stacks.input = &inputStack;
    stacks.cursors = &cursorStack;
    etree.root->putToSortedStacks(stacks, counters, 0, false);
  }
  renderList.afterRebuild();
}


void Panel::update(float dt)
{
  int updRes = etree.update(dt);
  if (updRes & ElementTree::RESULT_ELEMS_ADDED_OR_REMOVED)
    rebuildStacks();

  for (PanelPointer &ptr : pointers)
    if (ptr.cursor)
      ptr.cursor->update(dt);
}


void Panel::updateHover()
{
  SQInteger stickScrollFlags[MAX_POINTERS] = {0};
  Point2 pos[MAX_POINTERS];
  for (int hand = 0; hand < MAX_POINTERS; ++hand)
    pos[hand] = pointers[hand].isPresent ? pointers[hand].pos : Point2(-100, -100);
  etree.updateHover(inputStack, 2, pos, stickScrollFlags);
}


void Panel::updateActiveCursors()
{
  for (PanelPointer &ptr : pointers)
  {
    Cursor *prevCursor = ptr.cursor;
    ptr.cursor = nullptr;

    if (ptr.isPresent)
    {
      for (const InputEntry &ie : cursorStack.stack)
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
  G_ASSERT_RETURN(etree.root, );

  const Sqrat::Table &scriptDesc = etree.root->props.scriptDesc;

  spatialInfo.anchor = PanelAnchor(scriptDesc.RawGetSlotValue("worldAnchor", PanelAnchor::None));
  spatialInfo.constraint =
    PanelRotationConstraint(scriptDesc.RawGetSlotValue("worldRotationConstraint", PanelRotationConstraint::None));
  spatialInfo.geometry = PanelGeometry(scriptDesc.RawGetSlotValue("worldGeometry", PanelGeometry::None));
  spatialInfo.canBePointedAt = scriptDesc.RawGetSlotValue("worldCanBePointedAt", false);
  spatialInfo.canBeTouched = scriptDesc.RawGetSlotValue("worldCanBeTouched", false);

  spatialInfo.position = scriptDesc.RawGetSlotValue("worldOffset", Point3(0, 0, 0));
  spatialInfo.angles = scriptDesc.RawGetSlotValue("worldAngles", Point3(0, 0, 0));

  Point2 size2 = scriptDesc.RawGetSlotValue("worldSize", Point2(0, 0));
  spatialInfo.size.set(size2.x, size2.y, 1);

  spatialInfo.anchorEntityId = scriptDesc.RawGetSlotValue("worldAnchorEntity", 0u);
  spatialInfo.facingEntityId = scriptDesc.RawGetSlotValue("worldFacingEntity", 0u);

  spatialInfo.anchorNodeName = scriptDesc.RawGetSlotValue("worldAnchorEntityNode", eastl::string());
}

void Panel::updateRenderInfoParamsFromScript()
{
  Sqrat::Table &desc = etree.root->props.scriptDesc;
  renderInfo.brightness = desc.RawGetSlotValue("worldBrightness", PanelRenderInfo::def_brightness);
  renderInfo.smoothness = desc.RawGetSlotValue("worldSmoothness", PanelRenderInfo::def_smoothness);
  renderInfo.reflectance = desc.RawGetSlotValue("worldReflectance", PanelRenderInfo::def_reflectance);
  renderInfo.metalness = desc.RawGetSlotValue("worldMetalness", PanelRenderInfo::def_metalness);
  renderInfo.worldRenderFeatures = desc.RawGetSlotValue("worldRenderFeatures", 0);
}


/* *******************************************************/


void PanelData::init(GuiScene &scene, const Sqrat::Object &object, int panelIndex)
{
  close();

  panel.reset(new Panel(&scene));
  panel->init(object);

  index = panelIndex;

  panel->updateSpatialInfoFromScript();
  if (panel->spatialInfo.anchor != PanelAnchor::None)
    syncCanvas();
}


void PanelData::close()
{
  panel.reset();
  canvas.reset();
}


bool PanelData::getCanvasSize(IPoint2 &size) const
{
  if (!panel->etree.root)
    return false;
  size = panel->etree.root->props.scriptDesc.RawGetSlotValue("canvasSize", IPoint2(-1, -1));
  return size.x > 0 && size.y > 0;
}


eastl::string PanelData::getPointerPath() const
{
  return panel->etree.root->props.scriptDesc.GetSlotValue("worldPointerTexture", eastl::string());
}


Point2 PanelData::getPointerSize() const
{
  return panel->etree.root->props.scriptDesc.RawGetSlotValue("worldPointerSize", Point2(64, 64));
}


bool PanelData::isAutoUpdated() const { return panel->etree.root->props.scriptDesc.RawGetSlotValue("isAutoUpdated", true); }


E3DCOLOR PanelData::getPointerColor() const
{
  return panel->etree.root->props.scriptDesc.RawGetSlotValue("worldPointerColor", E3DCOLOR(255, 0, 0));
}


bool PanelData::isInThisPass(darg_panel_renderer::RenderPass render_pass) const
{
  int features = isPanelInited() ? panel->renderInfo.worldRenderFeatures : 0;

  switch (render_pass)
  {
    case darg_panel_renderer::RenderPass::Translucent: return (features & darg_panel_renderer::RenderFeatures::Opaque) == 0;

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


void PanelData::updateTexture(GuiScene &scene)
{
  if (!isAutoUpdated() && !isDirty)
    return;

  TIME_D3D_PROFILE(PanelData__updateTexture);

  {
    SCOPE_RENDER_TARGET;
    SCOPE_VIEWPORT;

    syncCanvas();

    TextureInfo texInfo;
    canvas->getTex()->getinfo(texInfo);

    d3d::set_render_target(canvas->getTex2D(), 0);
    d3d::setview(0, 0, texInfo.w, texInfo.h, 0, 1);
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);

    int savedBlockId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

    scene.refreshGuiContextState();
    scene.buildPanelRender(index);

    scene.getGuiContext()->setTarget();
    StdGuiRender::acquire();
    scene.getGuiContext()->flushData();
    scene.getGuiContext()->end_render();

    ShaderGlobal::setBlock(savedBlockId, ShaderGlobal::LAYER_FRAME);

    isDirty = false;
  }
}


} // namespace darg
