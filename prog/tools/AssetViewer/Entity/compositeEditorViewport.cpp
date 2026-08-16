// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compositeEditorViewport.h"
#include "../av_appwnd.h"
#include "entity_cm.h"
#include <assets/asset.h>
#include <de3_baseInterfaces.h>
#include <de3_composit.h>
#include <de3_dataBlockIdHolder.h>
#include <de3_entityGetSceneLodsRes.h>
#include <de3_objEntity.h>
#include <debug/dag_debug3d.h>
#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_editorCommandSystem.h>
#include <EditorCore/ec_menu.h>
#include <math/dag_rayIntersectBox.h>
#include <rendInst/rendInstExtra.h>
#include <winGuiWrapper/wgw_input.h>

// #define DEBUG_VIEWPORT_DATABLOCK_IDS


CompositeEditorViewport::CompositeEditorViewport() { invalidateCache(); }

IObjEntity *CompositeEditorViewport::getHitSubEntity(IGenViewportWnd *wnd, int x, int y, IObjEntity &entity)
{
  if (!get_app().isCompositeEditorShown())
    return nullptr;

  IPixelPerfectSelectionService *pixelPerfectSelectionService = EDITORCORE->queryEditorInterface<IPixelPerfectSelectionService>();
  if (!pixelPerfectSelectionService)
    return nullptr;

  Point3 dir, world;
  wnd->clientToWorld(Point2(x, y), world, dir);

  pixelPerfectSelectionHitsCache.clear();
  fillPossiblePixelPerfectSelectionHits(*pixelPerfectSelectionService, entity, nullptr, world, dir, pixelPerfectSelectionHitsCache);

  pixelPerfectSelectionService->getHits(*wnd, x, y, pixelPerfectSelectionHitsCache);
  return pixelPerfectSelectionHitsCache.empty() ? nullptr : static_cast<IObjEntity *>(pixelPerfectSelectionHitsCache[0].userData);
}

static void getRendInstQuantizedTm(IObjEntity &entity, TMatrix &tm)
{
  const IRendInstEntity *rendInstEntity = entity.queryInterface<IRendInstEntity>();
  if (!rendInstEntity || !rendInstEntity->getRendInstQuantizedTm(tm))
    entity.getTm(tm);
}

bool CompositeEditorViewport::getSelectionBox(IObjEntity *entity, BBox3 &box) const
{
  IObjEntity *selectedSubEntity = getSelectedSubEntity(entity);
  if (!selectedSubEntity)
    return false;

  TMatrix tm;
  getRendInstQuantizedTm(*selectedSubEntity, tm);
  box = tm * selectedSubEntity->getBbox();
  return true;
}

void CompositeEditorViewport::registerEditorCommands(IEditorCommandSystem &command_system)
{
  command_system.addCommand(EditorCommandIds::VIEW_GRID_MOVE_SNAP, ImGuiKey_S);
  command_system.addCommand(EditorCommandIds::VIEW_GRID_ANGLE_SNAP, ImGuiKey_A);
  command_system.addCommand(EditorCommandIds::VIEW_GRID_SCALE_SNAP, ImGuiMod_Shift | ImGuiKey_5);
  command_system.addCommand(EditorCommandIds::VIEW_GRID_SETTINGS);

  command_system.addCommand(EditorCommandIds::ENTITY_CREATE_NODE, ImGuiMod_Ctrl | ImGuiKey_N);

  command_system.addCommand(EditorCommandIds::ENTITY_COPY_ASSET, ImGuiMod_Ctrl | ImGuiKey_C);
  command_system.addCommand(EditorCommandIds::ENTITY_PASTE_ASSET, ImGuiMod_Ctrl | ImGuiKey_V);
  command_system.addCommand(EditorCommandIds::ENTITY_DUPLICATE_ASSET, ImGuiMod_Ctrl | ImGuiKey_D);

  command_system.addCommand(EditorCommandIds::ENTITY_MAKE_PARENT, ImGuiMod_Ctrl | ImGuiKey_P);
  command_system.addCommand(EditorCommandIds::ENTITY_CLEAR_PARENT, ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P);
}

void CompositeEditorViewport::registerMenuAccelerators()
{
  IWndManager &wndManager = *EDITORCORE->getWndManager();

  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_DELETE_SELECTED_NODES, EditorCommandIds::OBJED_DELETE);
  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_SET_GIZMO_MODE_NONE, EditorCommandIds::OBJED_MODE_SELECT);
  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_SET_GIZMO_MODE_MOVE, EditorCommandIds::OBJED_MODE_MOVE);
  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_SET_GIZMO_MODE_ROTATE, EditorCommandIds::OBJED_MODE_ROTATE);
  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_SET_GIZMO_MODE_SCALE, EditorCommandIds::OBJED_MODE_SCALE);
  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_TOGGLE_MOVE_SNAP, EditorCommandIds::VIEW_GRID_MOVE_SNAP);
  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_TOGGLE_ANGLE_SNAP, EditorCommandIds::VIEW_GRID_ANGLE_SNAP);
  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_TOGGLE_SCALE_SNAP, EditorCommandIds::VIEW_GRID_SCALE_SNAP);
  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_OPEN_GRID_SETTINGS, EditorCommandIds::VIEW_GRID_SETTINGS);
  wndManager.addViewportAccelerator(CM_COMPOSITE_EDITOR_CANCEL_GIZMO_TRANSFORM, EditorCommandIds::OBJED_CANCEL_GIZMO_TRANSFORM);
}

void CompositeEditorViewport::handleViewportAcceleratorCommand(unsigned id, IGenViewportWnd &wnd, IObjEntity *entity)
{
  if (id == CM_COMPOSITE_EDITOR_CREATE_NODE)
  {
    get_app().getCompositeEditor().createNode();
  }
  else if (id == CM_COMPOSITE_EDITOR_DELETE_SELECTED_NODES)
  {
    BBox3 bbox;
    if (getSelectionBox(entity, bbox)) // Delete rendered nodes only.
    {
      get_app().getCompositeEditor().deleteSelectedNodes();
      wnd.activate();
    }
  }
  else if (id == CM_COMPOSITE_EDITOR_SET_GIZMO_MODE_NONE)
  {
    get_app().getCompositeEditor().setGizmo(IEditorCoreEngine::ModeType::MODE_None);
  }
  else if (id == CM_COMPOSITE_EDITOR_SET_GIZMO_MODE_MOVE)
  {
    get_app().getCompositeEditor().setGizmo(IEditorCoreEngine::ModeType::MODE_Move);
  }
  else if (id == CM_COMPOSITE_EDITOR_SET_GIZMO_MODE_ROTATE)
  {
    get_app().getCompositeEditor().setGizmo(IEditorCoreEngine::ModeType::MODE_Rotate);
  }
  else if (id == CM_COMPOSITE_EDITOR_SET_GIZMO_MODE_SCALE)
  {
    get_app().getCompositeEditor().setGizmo(IEditorCoreEngine::ModeType::MODE_Scale);
  }
  else if (id == CM_COMPOSITE_EDITOR_TOGGLE_MOVE_SNAP)
  {
    get_app().getCompositeEditor().toggleSnapMode(CM_VIEW_GRID_MOVE_SNAP);
  }
  else if (id == CM_COMPOSITE_EDITOR_TOGGLE_ANGLE_SNAP)
  {
    get_app().getCompositeEditor().toggleSnapMode(CM_VIEW_GRID_ANGLE_SNAP);
  }
  else if (id == CM_COMPOSITE_EDITOR_TOGGLE_SCALE_SNAP)
  {
    get_app().getCompositeEditor().toggleSnapMode(CM_VIEW_GRID_SCALE_SNAP);
  }
  else if (id == CM_COMPOSITE_EDITOR_OPEN_GRID_SETTINGS)
  {
    get_app().getCompositeEditor().openGridSettings();
  }
  else if (id == CM_COMPOSITE_EDITOR_CANCEL_GIZMO_TRANSFORM)
  {
    if (EDITORCORE->isGizmoOperationStarted())
      EDITORCORE->endGizmo(/*apply = */ false);
    else if (get_app().getCompositeEditor().isEditingSubComposite())
      get_app().getCompositeEditor().exitSubCompositeEditing();
  }
  else if (id == CM_COMPOSITE_EDITOR_COPY_ASSET)
  {
    get_app().getCompositeEditor().copySelectedNodeParams();
  }
  else if (id == CM_COMPOSITE_EDITOR_PASTE_ASSET)
  {
    get_app().getCompositeEditor().pasteParamsToSelectedNode();
  }
  else if (id == CM_COMPOSITE_EDITOR_DUPLICATE_ASSET)
  {
    get_app().getCompositeEditor().duplicateSelectedNode();
  }
  else if (id == CM_COMPOSITE_EDITOR_MAKE_PARENT)
  {
    get_app().getCompositeEditor().makeSelectedParentRelation();
  }
  else if (id == CM_COMPOSITE_EDITOR_CLEAR_PARENT)
  {
    get_app().getCompositeEditor().clearSelectedParentRelation();
  }
}

int CompositeEditorViewport::onMenuItemClick(unsigned id)
{
  if (id == CM_COMPOSITE_EDITOR_COPY_ASSET)
  {
    get_app().getCompositeEditor().copySelectedNodeParams();
  }
  else if (id == CM_COMPOSITE_EDITOR_PASTE_ASSET)
  {
    get_app().getCompositeEditor().pasteParamsToSelectedNode();
  }
  else if (id == CM_COMPOSITE_EDITOR_DUPLICATE_ASSET)
  {
    get_app().getCompositeEditor().duplicateSelectedNode();
  }
  else if (id == CM_COMPOSITE_EDITOR_EDIT_SUB_COMPOSITE)
  {
    get_app().getCompositeEditor().enterSubCompositeEditing();
  }
  else if (id == CM_COMPOSITE_EDITOR_SUB_COMPOSITE_SAVE)
  {
    get_app().getCompositeEditor().saveSubCompositeEditing();
  }
  else if (id == CM_COMPOSITE_EDITOR_SUB_COMPOSITE_SAVE_UNIQUE)
  {
    get_app().getCompositeEditor().saveSubCompositeAsUnique();
  }
  else if (id == CM_COMPOSITE_EDITOR_SUB_COMPOSITE_REVERT)
  {
    get_app().getCompositeEditor().revertSubCompositeEditing();
  }
  else if (id == CM_COMPOSITE_EDITOR_EXIT_SUB_COMPOSITE)
  {
    get_app().getCompositeEditor().exitSubCompositeEditing();
  }
  else if (id == CM_COMPOSITE_EDITOR_MAKE_PARENT)
  {
    get_app().getCompositeEditor().makeSelectedParentRelation();

    return 1;
  }
  else if (id == CM_COMPOSITE_EDITOR_CLEAR_PARENT)
  {
    get_app().getCompositeEditor().clearSelectedParentRelation();

    return 1;
  }
  return 0;
}

void CompositeEditorViewport::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, IObjEntity *entity, int key_modif)
{
  if (!get_app().isCompositeEditorShown() || !entity)
    return;

  const bool multiSelect = (key_modif == wingw::M_CTRL);
  const bool selectAsParent = (key_modif == wingw::M_SHIFT);
  IObjEntity *hitEntity = getHitSubEntity(wnd, x, y, *entity);
  unsigned dataBlockId = hitEntity ? getEntityDataBlockId(*hitEntity) : IDataBlockIdHolder::invalid_id;
  get_app().getCompositeEditor().selectTreeNodeByDataBlockId(dataBlockId, multiSelect || selectAsParent, selectAsParent);
}

bool CompositeEditorViewport::handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, IObjEntity *entity, int key_modif)
{
  if (!get_app().isCompositeEditorShown() || !entity)
    return false;

  CompositeEditor &compositeEditor = get_app().getCompositeEditor();

  IObjEntity *hitEntity = getHitSubEntity(wnd, x, y, *entity);
  if (!hitEntity)
    return false;

  unsigned dataBlockId = getEntityDataBlockId(*hitEntity);
  compositeEditor.selectTreeNodeByDataBlockId(dataBlockId, false, false);

  const CompositeEditorTreeDataNode *selNode = compositeEditor.getSelectedTreeDataNode();
  if (!selNode || !selNode->isCompositeAsset())
    return false;

  compositeEditor.enterSubCompositeEditing();
  return true;
}

bool CompositeEditorViewport::handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, IObjEntity *entity, int key_modif)
{
  if (!get_app().isCompositeEditorShown() || !entity)
    return false;

  CompositeEditor &compositeEditor = get_app().getCompositeEditor();
  const bool isEditingSubComposite = compositeEditor.isEditingSubComposite();

  dag::Vector<IObjEntity *> selectedSubEntities;
  int parentIdx;
  getSelectedSubEntities(entity, selectedSubEntities, parentIdx);
  if (parentIdx < 0 && !isEditingSubComposite)
    return false;

  if (!popupMenu)
  {
    IEditorCommandSystem *commandSystem = EDITORCORE->queryEditorInterface<IEditorCommandSystem>();
    G_ASSERT(commandSystem);

    popupMenu.reset(ec_create_context_menu());
    popupMenu->setEventHandler(this);

    if (parentIdx >= 0)
    {
      if (selectedSubEntities.size() < 2)
      {
        commandSystem->addMenuItem(*popupMenu, PropPanel::ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_COPY_ASSET,
          EditorCommandIds::ENTITY_COPY_ASSET, "Copy asset");
        commandSystem->addMenuItem(*popupMenu, PropPanel::ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_PASTE_ASSET,
          EditorCommandIds::ENTITY_PASTE_ASSET, "Paste asset");
        commandSystem->addMenuItem(*popupMenu, PropPanel::ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_DUPLICATE_ASSET,
          EditorCommandIds::ENTITY_DUPLICATE_ASSET, "Duplicate asset");

        popupMenu->addSeparator(PropPanel::ROOT_MENU_ITEM);
        popupMenu->addItem(PropPanel::ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_EDIT_SUB_COMPOSITE, "Edit sub-composite in place");
        const CompositeEditorTreeDataNode *selNode = compositeEditor.getSelectedTreeDataNode();
        popupMenu->setEnabledById(CM_COMPOSITE_EDITOR_EDIT_SUB_COMPOSITE, selNode && selNode->isCompositeAsset());
      }
      else
      {
        commandSystem->addMenuItem(*popupMenu, PropPanel::ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_MAKE_PARENT,
          EditorCommandIds::ENTITY_MAKE_PARENT, "Make parent");
        popupMenu->setEnabledById(CM_COMPOSITE_EDITOR_MAKE_PARENT, compositeEditor.canParentSelectedTreeDataNodes());

        commandSystem->addMenuItem(*popupMenu, PropPanel::ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_CLEAR_PARENT,
          EditorCommandIds::ENTITY_CLEAR_PARENT, "Clear parent");
        popupMenu->setEnabledById(CM_COMPOSITE_EDITOR_CLEAR_PARENT, compositeEditor.hasSelectedParentRelation());
      }
    }

    if (isEditingSubComposite)
    {
      if (parentIdx >= 0)
        popupMenu->addSeparator(PropPanel::ROOT_MENU_ITEM);
      popupMenu->addItem(PropPanel::ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_SUB_COMPOSITE_SAVE, "Save");
      popupMenu->addItem(PropPanel::ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_SUB_COMPOSITE_SAVE_UNIQUE, "Save as a unique");
      popupMenu->addItem(PropPanel::ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_SUB_COMPOSITE_REVERT, "Revert changes");
      popupMenu->setEnabledById(CM_COMPOSITE_EDITOR_SUB_COMPOSITE_REVERT, compositeEditor.isModified());
      popupMenu->addSeparator(PropPanel::ROOT_MENU_ITEM);
      popupMenu->addItem(PropPanel::ROOT_MENU_ITEM, CM_COMPOSITE_EDITOR_EXIT_SUB_COMPOSITE, "Exit sub-composite editing");
    }
    return true;
  }
  return false;
}

void CompositeEditorViewport::renderObjects(IGenViewportWnd &wnd, IObjEntity *entity)
{
  if (!get_app().isCompositeEditorShown())
    return;

  const bool isSubCompositeEditing = get_app().getCompositeEditor().isEditingSubComposite();

  int parentIdx;
  subEntitySelectionLookup.clear();
  getSelectedSubEntities(entity, subEntitySelectionLookup, parentIdx);
  if (subEntitySelectionLookup.empty() && !isSubCompositeEditing)
    return;

  if (subEntitySelectionLookup != cachedSelectedSubEntities || cachedParentIdx != parentIdx ||
      (isSubCompositeEditing && outlineElementsCache.empty()))
  {
    cachedSelectedSubEntities.clear();
    cachedSelectedSubEntities.insert(cachedSelectedSubEntities.end(), subEntitySelectionLookup.begin(),
      subEntitySelectionLookup.end());
    cachedParentIdx = parentIdx;
    outlineElementsCache.clear();
    for (int i = 0; i < subEntitySelectionLookup.size(); ++i)
    {
      OutlineElementsCache &cache = outlineElementsCache.push_back();
      fillRenderElements(*subEntitySelectionLookup[i], cache.riElements, cache.dynmodelElements);
      cache.color = (parentIdx == i) ? OutlineRenderer::default_outline_color : E3DCOLOR(255, 128, 0);
    }
  }
  if (!outlineElementsCache.empty())
  {
    for (int i = 0; i < outlineElementsCache.size(); ++i)
    {
      OutlineElementsCache &cache = outlineElementsCache[i];
      if (!cache.riElements.empty() || !cache.dynmodelElements.empty())
        outlineRenderer.render(wnd, cache.riElements, cache.dynmodelElements, cache.color);
    }
  }
  if (isSubCompositeEditing)
  {
    dag::ConstSpan<CompositeEditorSubContext> stack = get_app().getCompositeEditor().getSubCompositeStack();
    bool cacheValid = (cachedTintGhostEntities.size() == stack.size());
    for (int i = 0; cacheValid && i < (int)stack.size(); ++i)
      cacheValid = (stack[i].parentGhostEntity == cachedTintGhostEntities[i]);
    if (!cacheValid)
    {
      cachedTintGhostEntities.clear();
      tintCache.riElements.clear();
      tintCache.dynmodelElements.clear();
      tintCache.occluderRiElements.clear();
      tintCache.occluderDynmodelElements.clear();
      for (const CompositeEditorSubContext &ctx : stack)
      {
        cachedTintGhostEntities.push_back(ctx.parentGhostEntity);
        if (!ctx.parentGhostEntity)
          continue;
        fillRenderElements(*ctx.parentGhostEntity, tintCache.riElements, tintCache.dynmodelElements, ctx.subCompositeDataBlockId);
        if (IObjEntity *slotEntity = getSubEntityByDataBlockId(*ctx.parentGhostEntity, ctx.subCompositeDataBlockId))
          fillRenderElements(*slotEntity, tintCache.occluderRiElements, tintCache.occluderDynmodelElements, 0);
      }
    }
    if (!tintCache.riElements.empty() || !tintCache.dynmodelElements.empty())
      editModeRenderer.render(wnd, tintCache.riElements, tintCache.dynmodelElements, tintCache.occluderRiElements,
        tintCache.occluderDynmodelElements, EditModeRenderer::default_tint_color);
  }
}

/*static*/
void CompositeEditorViewport::cacheSubEntitySpatials(SubEntityNode &s)
{
  if (!s.spatialsCached)
  {
    s.bbox = s.entity->getBbox();
    getRendInstQuantizedTm(*s.entity, s.tm);
    s.spatialsCached = true;
  }
}

/*static*/
void CompositeEditorViewport::cacheSubEntityScreenPos(SubEntityNode &s, IGenViewportWnd &wnd)
{
  cacheSubEntitySpatials(s);
  if (!s.screenPosCached)
  {
    Point3 center = s.bbox.center();
    Point2 screenPos;
    real z;
    if (wnd.worldToClient(s.tm * center, screenPos, &z) || z > 0.0f)
      s.sPos = screenPos;
    s.screenPosCached = true;
  }
}

void CompositeEditorViewport::renderTransObjects(IGenViewportWnd &wnd, IObjEntity *entity)
{
  if (!get_app().isCompositeEditorShown())
    return;

  CompositeEditor &compositeEditor = get_app().getCompositeEditor();
  if (compositeEditor.isEditingSubComposite())
  {
    int viewportWidth, viewportHeight;
    wnd.getViewportSize(viewportWidth, viewportHeight);
    // When active, the yellow border (3px) covers the outer edge, so inset to stay visible.
    const int inset = wnd.isActive() ? 3 : 0;
    StdGuiRender::start_render();
    StdGuiRender::set_color(E3DCOLOR(255, 128, 0));
    StdGuiRender::render_frame(inset, inset, viewportWidth - inset, viewportHeight - inset, 3);
    StdGuiRender::end_render();
  }

  unsigned int parentDataBlockId;
  dag::Vector<unsigned int> selectedDataBlockIds;
  compositeEditor.getSelectedTreeNodeDataBlockIds(selectedDataBlockIds, parentDataBlockId);
  if (selectedDataBlockIds.empty() && parentDataBlockId == IDataBlockIdHolder::invalid_id)
    return;

  if (!entity)
    return;

  // collect relevant sub-entity data
  subEntityNodeLookup.clear();
  ICompositObj *compositObj = entity->queryInterface<ICompositObj>();
  if (compositObj)
  {
    const int subEntityCount = compositObj->getCompositSubEntityCount();
    for (int subEntityIndex = 0; subEntityIndex < subEntityCount; ++subEntityIndex)
    {
      IObjEntity *subEntity = compositObj->getCompositSubEntity(subEntityIndex);
      if (!subEntity)
        continue;

      IDataBlockIdHolder *dbih = subEntity->queryInterface<IDataBlockIdHolder>();
      if (dbih && dbih->getDataBlockId() != IDataBlockIdHolder::invalid_id)
      {
        SubEntityNode n;
        n.entity = subEntity;
        unsigned int dataBlockId = dbih->getDataBlockId();
        n.isSelected =
          eastl::find(selectedDataBlockIds.begin(), selectedDataBlockIds.end(), dataBlockId) != selectedDataBlockIds.end();
        n.isSelectedParent = (dataBlockId == parentDataBlockId);
        n.sPos = eastl::optional<Point2>();

        subEntityNodeLookup[dataBlockId] = n;
      }
    }
  }

  // draw selection bboxes
  begin_draw_cached_debug_lines();
  for (auto subEntityNode : subEntityNodeLookup)
  {
    SubEntityNode &entityNode = subEntityNode.second;
    if (entityNode.isSelected)
    {
      cacheSubEntitySpatials(entityNode);
      set_cached_debug_lines_wtm(entityNode.tm);
      draw_cached_debug_box(entityNode.bbox, E3DCOLOR(255, 0, 0));
    }
  }
  end_draw_cached_debug_lines();

  StdGuiRender::start_render();
  dag::Vector<const CompositeEditorTreeDataNode *> nodes;
  nodes.push_back(compositeEditor.getRootTreeDataNode());
  G_ASSERT(compositeEditor.getRootTreeDataNode() != nullptr);
  for (int i = 0; i < nodes.size(); ++i)
  {
    const CompositeEditorTreeDataNode *parent = nodes[i];
    auto it = subEntityNodeLookup.find(parent->dataBlockId);
    if (it == subEntityNodeLookup.end())
    {
      // just look through child sub-tree(s)
      for (int j = 0; j < parent->nodeCount(); ++j)
      {
        if (parent->nodes[j].get() != nullptr)
          nodes.push_back(parent->nodes[j].get());
      }
      continue;
    }

    SubEntityNode *p = &it->second;
    int selectedChildNodes = 0;
    for (int j = 0; j < parent->nodeCount(); ++j)
    {
      const CompositeEditorTreeDataNode *child = parent->nodes[j].get();
      if (!child)
        continue;

      nodes.push_back(child); // may have child sub-tree(s)!

      auto it = subEntityNodeLookup.find(child->dataBlockId);
      if (it == subEntityNodeLookup.end())
        continue;

      SubEntityNode *c = &it->second;
      if (c->isSelected)
        selectedChildNodes++;

      // render parent-child relation lines
      c->isAncestorSelectedParent = p->isSelectedParent || p->isAncestorSelectedParent;
      cacheSubEntityScreenPos(*c, wnd);
      cacheSubEntityScreenPos(*p, wnd);
      if (c->sPos.has_value() && p->sPos.has_value())
      {
        if (c->isAncestorSelectedParent)
          StdGuiRender::get_stdgui_context()->render_dashed_line(c->sPos.value(), p->sPos.value(), 8, 16, 2, COLOR_BLACK);
        else if (c->isSelected)
          StdGuiRender::get_stdgui_context()->render_dashed_line(c->sPos.value(), p->sPos.value(), 4, 24, 2, E3DCOLOR(64, 64, 64));
      }
    }

    // render active selections of the parent-child relation
    if (p->isSelected && ((p->isSelectedParent && selectedChildNodes > 0) || p->isAncestorSelectedParent))
    {
      cacheSubEntityScreenPos(*p, wnd);
      if (p->sPos.has_value())
      {
        E3DCOLOR fill_color = p->isSelectedParent ? OutlineRenderer::default_outline_color : E3DCOLOR(255, 128, 0);
        StdGuiRender::get_stdgui_context()->render_ellipse_aa(p->sPos.value(), Point2(5, 5), 2.0f, COLOR_BLACK, COLOR_BLACK,
          fill_color);
      }
    }
  }
  StdGuiRender::end_render();

#ifdef DEBUG_VIEWPORT_DATABLOCK_IDS
  StdGuiRender::start_render();
  for (auto subEntityNode : subEntities)
  {
    SubEntityNode &entityNode = subEntityNode.second;
    if (!entityNode.isSelected)
      continue;

    unsigned int dataBlockId = subEntityNode.first;
    if (dataBlockId == IDataBlockIdHolder::invalid_id)
      continue;

    if (entityNode.sPos.has_value())
    {
      Point2 sPos = entityNode.sPos.value();
      auto font = StdGuiRender::get_stdgui_context()->curRenderFont;
      String id(0, "%d", dataBlockId);
      StdGuiRender::draw_strf_to(sPos.x - font.monoW, sPos.y - (font.fontHt * 3), id);
    }
  }
  StdGuiRender::end_render();
#endif
}

void CompositeEditorViewport::updateImgui()
{
  if (popupMenu)
  {
    const bool open = PropPanel::render_context_menu(*popupMenu);
    if (!open)
      popupMenu.reset();
  }
}

void CompositeEditorViewport::invalidateCache()
{
  cachedSelectedSubEntities.clear();
  outlineElementsCache.clear();
  cachedTintGhostEntities.clear();
}

void CompositeEditorViewport::getAllHits(IObjEntity &entity, const Point3 &from, const Point3 &dir,
  dag::Vector<CompositeEntityHit> &hits)
{
  ICompositObj *compositObj = entity.queryInterface<ICompositObj>();
  if (!compositObj)
    return;

  const int subEntityCount = compositObj->getCompositSubEntityCount();
  for (int subEntityIndex = 0; subEntityIndex < subEntityCount; ++subEntityIndex)
  {
    IObjEntity *subEntity = compositObj->getCompositSubEntity(subEntityIndex);
    if (!subEntity)
      continue;

    TMatrix tm;
    getRendInstQuantizedTm(*subEntity, tm);

    float out_t;
    if (ray_intersect_box(from, dir, subEntity->getBbox(), tm, out_t))
    {
      CompositeEntityHit hit;
      hit.entity = subEntity;
      hit.distance = out_t;
      hits.push_back(std::move(hit));
    }
  }
}

IObjEntity *CompositeEditorViewport::getNearestHit(IObjEntity &entity, const Point3 &from, const Point3 &dir)
{
  dag::Vector<CompositeEntityHit> hits;
  getAllHits(entity, from, dir, hits);
  if (hits.empty())
    return nullptr;

  CompositeEntityHit *nearestHit = nullptr;
  for (int i = 0; i < hits.size(); ++i)
  {
    if (hits[i].entity == &entity)
      hits[i].distance = FLT_MAX;

    if (!nearestHit || hits[i].distance < nearestHit->distance)
      nearestHit = &hits[i];
  }

  return nearestHit->entity;
}

unsigned CompositeEditorViewport::getEntityDataBlockId(IObjEntity &entity)
{
  IDataBlockIdHolder *dbih = entity.queryInterface<IDataBlockIdHolder>();
  return dbih ? dbih->getDataBlockId() : IDataBlockIdHolder::invalid_id;
}

IObjEntity *CompositeEditorViewport::getSubEntityByDataBlockId(IObjEntity &entity, unsigned dataBlockId)
{
  ICompositObj *compositObj = entity.queryInterface<ICompositObj>();
  if (!compositObj)
    return nullptr;

  const int subEntityCount = compositObj->getCompositSubEntityCount();
  for (int subEntityIndex = 0; subEntityIndex < subEntityCount; ++subEntityIndex)
  {
    IObjEntity *subEntity = compositObj->getCompositSubEntity(subEntityIndex);
    if (!subEntity)
      continue;

    IDataBlockIdHolder *dbih = subEntity->queryInterface<IDataBlockIdHolder>();
    if (dbih && dbih->getDataBlockId() == dataBlockId)
      return subEntity;
  }

  return nullptr;
}

IObjEntity *CompositeEditorViewport::getSelectedSubEntity(IObjEntity *entity)
{
  const unsigned dataBlockId = get_app().getCompositeEditor().getSelectedTreeNodeDataBlockId();
  if (!entity || dataBlockId == IDataBlockIdHolder::invalid_id)
    return nullptr;

  return getSubEntityByDataBlockId(*entity, dataBlockId);
}

/*static*/
void CompositeEditorViewport::getSelectedSubEntities(IObjEntity *entity, dag::Vector<IObjEntity *> &entities, int &parent_idx)
{
  if (!entity)
    return;

  dag::Vector<unsigned> dataBlockIds;
  unsigned parentDataBlockId;
  get_app().getCompositeEditor().getSelectedTreeNodeDataBlockIds(dataBlockIds, parentDataBlockId);

  parent_idx = -1;
  for (int i = 0; i < dataBlockIds.size(); ++i)
  {
    unsigned dataBlockId = dataBlockIds[i];
    if (dataBlockId != IDataBlockIdHolder::invalid_id)
    {
      IObjEntity *subEntity = getSubEntityByDataBlockId(*entity, dataBlockId);
      if (subEntity != nullptr)
      {
        entities.push_back(subEntity);
        if (parentDataBlockId == dataBlockId)
          parent_idx = entities.size() - 1;
      }
    }
  }
}

int CompositeEditorViewport::getEntityRendInstExtraResourceIndex(IObjEntity &entity)
{
  IRendInstEntity *rendInstEntity = entity.queryInterface<IRendInstEntity>();
  if (!rendInstEntity)
    return -1;

  DagorAsset *asset = rendInstEntity->getAsset();
  if (!asset)
    return -1;

  return rendinst::addRIGenExtraResIdx(asset->getName(), -1, -1, {});
}

void CompositeEditorViewport::fillRenderElements(IObjEntity &entity, OutlineRenderer::RIElementsCache &renderElements,
  dag::Vector<DynamicRenderableSceneInstance *> &dynmodelElements, unsigned excludeDataBlockId)
{
  ICompositObj *compositObj = entity.queryInterface<ICompositObj>();
  if (compositObj)
  {
    const int subEntityCount = compositObj->getCompositSubEntityCount();
    for (int subEntityIndex = 0; subEntityIndex < subEntityCount; ++subEntityIndex)
    {
      IObjEntity *subEntity = compositObj->getCompositSubEntity(subEntityIndex);
      if (!subEntity)
        continue;

      if (excludeDataBlockId != 0 && getEntityDataBlockId(*subEntity) == excludeDataBlockId)
        continue;

      // Do not propagate excludeDataBlockId into nested composites: each composite asset has
      // its own ID counter starting at 1, so the parent-level exclude ID would wrongly match
      // children of sibling composites.
      fillRenderElements(*subEntity, renderElements, dynmodelElements, 0);
    }
  }

  if (auto *rendInstEntity = entity.queryInterface<IRendInstEntity>())
  {
    const int ri_idx = rendInstEntity->getPregenId();
    if (DAGOR_LIKELY(ri_idx >= 0))
    {
      TMatrix tm;
      getRendInstQuantizedTm(entity, tm);
      renderElements.emplace(ri_idx, tm);
    }
  }
  else if (auto *dynSceneRes = entity.queryInterface<IEntityGetDynSceneLodsRes>())
  {
    if (auto *sceneInstance = dynSceneRes->getSceneInstance(); sceneInstance)
      dynmodelElements.emplace_back(sceneInstance);
  }
}

void CompositeEditorViewport::fillPossiblePixelPerfectSelectionHits(IPixelPerfectSelectionService &pixelPerfectSelectionService,
  IObjEntity &entity, IObjEntity *entityForSelection, const Point3 &rayOrigin, const Point3 &rayDirection,
  dag::Vector<IPixelPerfectSelectionService::Hit> &hits)
{
  TMatrix tm;
  getRendInstQuantizedTm(entity, tm);

  float out_t;
  if (!ray_intersect_box(rayOrigin, rayDirection, entity.getBbox(), tm, out_t))
    return;

  ICompositObj *compositObj = entity.queryInterface<ICompositObj>();
  if (compositObj)
  {
    const int subEntityCount = compositObj->getCompositSubEntityCount();
    for (int subEntityIndex = 0; subEntityIndex < subEntityCount; ++subEntityIndex)
    {
      IObjEntity *subEntity = compositObj->getCompositSubEntity(subEntityIndex);
      if (!subEntity)
        continue;

      fillPossiblePixelPerfectSelectionHits(pixelPerfectSelectionService, *subEntity,
        entityForSelection ? entityForSelection : subEntity, rayOrigin, rayDirection, hits);
    }
  }

  IPixelPerfectSelectionService::Hit hit;
  if (!pixelPerfectSelectionService.initializeHit(hit, entity))
    return;

  hit.transform = tm;
  hit.userData = entityForSelection ? entityForSelection : &entity;
  hits.emplace_back(hit);
}
