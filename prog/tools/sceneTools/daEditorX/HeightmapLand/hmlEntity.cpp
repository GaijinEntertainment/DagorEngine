// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlEntity.h"
#include "hmlPlugin.h"
#include "hmlObjectsEditor.h"
#include "hmlSplineObject.h"
#include "hmlSplinePoint.h"

#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_entityFilter.h>
#include <de3_randomSeed.h>
#include <de3_entityUserData.h>
#include <de3_genObjUtil.h>
#include <de3_entityCollision.h>
#include <de3_splineGenSrv.h>
#include <de3_genObjData.h>
#include <de3_staticMaterialEditor.h>
#include <EditorCore/ec_IEditorCore.h>
#include <math/dag_math3d.h>
#include <math/dag_rayIntersectBox.h>
#include <math/dag_math2d.h>
#include <math/random/dag_random.h>
#include <util/dag_globDef.h>
#include <EditorCore/ec_rect.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <debug/dag_debug.h>
#include <drv/3d/dag_info.h>

#include "objectParam.h"

using editorcore_extapi::dagRender;

using hdpi::_pxScaled;

static ObjectParam objParam;


LandscapeEntityObject::CollidersData LandscapeEntityObject::colliders;
static int default_place_type = LandscapeEntityObject::Props::PT_coll;

static const int decal_material_max_count = 20;
enum
{
  PID_PLACE_TYPE = 100,
  PID_PLACE_TYPE_BTN_RADIO,
  PID_PLACE_TYPE_BTN_DROPDOWN,
  PID_PLACE_TYPE_TABS,
  PID_PLACE_TYPE_RADIO,
  PID_PLACE_TYPE_DROPDOWN = PID_PLACE_TYPE_RADIO + LandscapeEntityObject::Props::PT_count + 1, /* +1 is "mixed" */
  PID_PLACE_TYPE_OVERRIDE,

  PID_ENTITY_LAYERS,

  PID_ENTITY_COLLISION,
  PID_ENTITY_NOTES,
  PID_ENTITY_CASTER_GRP,
  PID_ENTITY_CASTER_FIRST,
  PID_ENTITY_CASTER_LAST = PID_ENTITY_CASTER_FIRST + 200,

  PID_ENTITY_USE_FILTER,
  PID_ENTITY_FILTER_GRP,
  PID_ENTITY_FILTER_FIRST,
  PID_ENTITY_FILTER_LAST = PID_ENTITY_FILTER_FIRST + 200,
  PID_DEF_PLACE_TYPE,

  PID_TRACEOFFSET,

  PID_ENTITYNAME,
  PID_DECAL_MATERIAL_GROUP,
  PID_DECAL_MATERIAL_FIRST,
  PID_DECAL_MATERIAL_LAST = PID_DECAL_MATERIAL_FIRST + decal_material_max_count,
  PID_DECAL_MATERIAL_SAVE,

  PID_GENERATE_PERINST_SEED,
  PID_GENERATE_EQUAL_PERINST_SEED,
  PID_PERINST_SEED,

  PID_GENERATE_SEED,
  PID_GENERATE_EQUAL_SEED,
  PID_SEED,

  PID_SPLIT_RECURSIVE = PID_ENTITY_FILTER_FIRST + 300,
};

G_STATIC_ASSERT(LandscapeEntityObject::Props::PT_count == 7);
static const char *place_types_full[] = {
  "-- no --",
  "Place pivot",
  "Place pivot and use normal",
  "Place 3-point (bbox)",
  "Place foundation (bbox)",
  "Place on water (floatable)",
  "Place pivot with rendinst collision",
  "-- (mixed) --",
};

void on_object_entity_name_changed(RenderableEditableObject &obj)
{
  static_cast<HmapLandObjectEditor *>(obj.getObjEditor())->onObjectEntityNameChanged(obj);
}

static class DefHMLEntityParams
{
public:
  int placeTypeTab;
  bool placeTypeMixed;
  bool splitCompRecursive;
  DefHMLEntityParams() : placeTypeTab(PID_PLACE_TYPE_BTN_DROPDOWN), placeTypeMixed(false), splitCompRecursive(false) {}

} defaultHMLEntity;

LandscapeEntityObject::LandscapeEntityObject(const char *ent_name, int rnd_seed)
{
  props.entityName = ent_name;
  entity = NULL;
  rndSeed = rnd_seed;
  perInstSeed = 0;
  isCollidable = true;
}
LandscapeEntityObject::~LandscapeEntityObject() { destroy_it(entity); }

void LandscapeEntityObject::renderBox()
{
  if (EditLayerProps::layerProps[getEditLayerIdx()].hide)
    return;
  if (EditLayerProps::layerProps[getEditLayerIdx()].lock)
    return;

  if (!entity || isSelected())
  {
#define BOUND_BOX_LEN_DIV    4
#define BOUND_BOX_INDENT_MUL 0.03
    BBox3 box = entity ? entity->getBbox() : BBox3(Point3(-0.2, -0.2, -0.2), Point3(0.2, 0.2, 0.2));

    const real deltaX = box[1].x - box[0].x;
    const real deltaY = box[1].y - box[0].y;
    const real deltaZ = box[1].z - box[0].z;

    const real dx = deltaX / BOUND_BOX_LEN_DIV;
    const real dy = deltaY / BOUND_BOX_LEN_DIV;
    const real dz = deltaZ / BOUND_BOX_LEN_DIV;

    const E3DCOLOR c = isSelected() ? E3DCOLOR(0xff, 0, 0) : E3DCOLOR(0xff, 0xff, 0xff);

    if (entity)
    {
      TMatrix tm;
      entity->getTm(tm);
      dagRender->setLinesTm(tm);
    }
    else
      dagRender->setLinesTm(getWtm());

    dagRender->renderLine(box[0], box[0] + Point3(dx, 0, 0), c);
    dagRender->renderLine(box[0], box[0] + Point3(0, dy, 0), c);
    dagRender->renderLine(box[0], box[0] + Point3(0, 0, dz), c);

    dagRender->renderLine(box[0] + Point3(deltaX, 0, 0), box[0] + Point3(deltaX - dx, 0, 0), c);
    dagRender->renderLine(box[0] + Point3(deltaX, 0, 0), box[0] + Point3(deltaX, dy, 0), c);
    dagRender->renderLine(box[0] + Point3(deltaX, 0, 0), box[0] + Point3(deltaX, 0, dz), c);

    dagRender->renderLine(box[0] + Point3(deltaX, 0, deltaZ), box[0] + Point3(deltaX - dx, 0, deltaZ), c);
    dagRender->renderLine(box[0] + Point3(deltaX, 0, deltaZ), box[0] + Point3(deltaX, dy, deltaZ), c);
    dagRender->renderLine(box[0] + Point3(deltaX, 0, deltaZ), box[0] + Point3(deltaX, 0, deltaZ - dz), c);

    dagRender->renderLine(box[0] + Point3(0, 0, deltaZ), box[0] + Point3(dx, 0, deltaZ), c);
    dagRender->renderLine(box[0] + Point3(0, 0, deltaZ), box[0] + Point3(0, dy, deltaZ), c);
    dagRender->renderLine(box[0] + Point3(0, 0, deltaZ), box[0] + Point3(0, 0, deltaZ - dz), c);


    dagRender->renderLine(box[1], box[1] - Point3(dx, 0, 0), c);
    dagRender->renderLine(box[1], box[1] - Point3(0, dy, 0), c);
    dagRender->renderLine(box[1], box[1] - Point3(0, 0, dz), c);

    dagRender->renderLine(box[1] - Point3(deltaX, 0, 0), box[1] - Point3(deltaX - dx, 0, 0), c);
    dagRender->renderLine(box[1] - Point3(deltaX, 0, 0), box[1] - Point3(deltaX, dy, 0), c);
    dagRender->renderLine(box[1] - Point3(deltaX, 0, 0), box[1] - Point3(deltaX, 0, dz), c);

    dagRender->renderLine(box[1] - Point3(deltaX, 0, deltaZ), box[1] - Point3(deltaX - dx, 0, deltaZ), c);
    dagRender->renderLine(box[1] - Point3(deltaX, 0, deltaZ), box[1] - Point3(deltaX, dy, deltaZ), c);
    dagRender->renderLine(box[1] - Point3(deltaX, 0, deltaZ), box[1] - Point3(deltaX, 0, deltaZ - dz), c);

    dagRender->renderLine(box[1] - Point3(0, 0, deltaZ), box[1] - Point3(dx, 0, deltaZ), c);
    dagRender->renderLine(box[1] - Point3(0, 0, deltaZ), box[1] - Point3(0, dy, deltaZ), c);
    dagRender->renderLine(box[1] - Point3(0, 0, deltaZ), box[1] - Point3(0, 0, deltaZ - dz), c);

#undef BOUND_BOX_LEN_DIV
#undef BOUND_BOX_INDENT_MUL
  }
}

bool LandscapeEntityObject::isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const
{
  if (EditLayerProps::layerProps[getEditLayerIdx()].hide)
    return false;
  if (!entity)
  {
    Point2 p;
    if (!vp->worldToClient(matrix.getcol(3), p, NULL))
      return false;
    return p.x >= rect.l && p.y >= rect.t && p.x <= rect.r && p.y <= rect.b;
  }

  BBox3 box = entity->getBbox();
  real z;

  TMatrix tm;
  entity->getTm(tm);

  HmapLandObjectEditor *editor = static_cast<HmapLandObjectEditor *>(getObjEditor());
  if (editor && editor->isSelectOnlyIfEntireObjectInRect())
  {
    for (int i = 0; i < 8; i++)
    {
      Point2 sp;
      if (!vp->worldToClient(tm * box.point(i), sp, &z))
        return false;
      if (z <= 0.0f || rect.l > sp.x || rect.t > sp.y || sp.x > rect.r || sp.y > rect.b)
        return false;
    }

    return true;
  }

  Point2 cp[8];
  BBox2 box2;
  bool in_frustum = false;

#define TEST_POINT(i, P)                                                                         \
  in_frustum |= vp->worldToClient(tm * P, cp[i], &z) && z > 0;                                   \
  if (z > 0 && rect.l <= cp[i].x && rect.t <= cp[i].y && cp[i].x <= rect.r && cp[i].y <= rect.b) \
    return true;                                                                                 \
  else                                                                                           \
    box2 += cp[i];

#define TEST_SEGMENT(i, j)                          \
  if (::isect_line_segment_box(cp[i], cp[j], rbox)) \
  return true

  for (int i = 0; i < 8; i++)
  {
    TEST_POINT(i, box.point(i));
  }

  if (!in_frustum)
    return false;
  BBox2 rbox(Point2(rect.l, rect.t), Point2(rect.r, rect.b));
  if (!(box2 & rbox))
    return false;

  TEST_SEGMENT(0, 4);
  TEST_SEGMENT(4, 5);
  TEST_SEGMENT(5, 1);
  TEST_SEGMENT(1, 0);
  TEST_SEGMENT(2, 6);
  TEST_SEGMENT(6, 7);
  TEST_SEGMENT(7, 3);
  TEST_SEGMENT(3, 2);
  TEST_SEGMENT(0, 2);
  TEST_SEGMENT(1, 3);
  TEST_SEGMENT(4, 6);
  TEST_SEGMENT(5, 7);

#undef TEST_POINT
#undef TEST_SEGMENT

  return isSelectedByPointClick(vp, rect.l, rect.t);
}
bool LandscapeEntityObject::isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const
{
  if (EditLayerProps::layerProps[getEditLayerIdx()].hide)
    return false;
  if (!entity)
  {
    Point2 p;
    if (!vp->worldToClient(matrix.getcol(3), p, NULL))
      return false;
    return p.x >= x - 3 && p.y >= y - 3 && p.x <= x + 3 && p.y <= y + 3;
  }

  Point3 dir, p0;
  float out_t;

  vp->clientToWorld(Point2(x, y), p0, dir);
  TMatrix tm;
  entity->getTm(tm);
  return ray_intersect_box(p0, dir, entity->getBbox(), tm, out_t);
}
bool LandscapeEntityObject::getWorldBox(BBox3 &box) const
{
  box = matrix * (entity ? entity->getBsph() : BSphere3(Point3(0, 0, 0), 0.5));
  return true;
}

bool LandscapeEntityObject::isColliderEnabled(const IDagorEdCustomCollider *collider)
{
  for (int i = colliders.col.size() - 1; i >= 0; i--)
    if (colliders.col[i] == collider)
      return true;

  return false;
}

void LandscapeEntityObject::fillProps(PropPanel::ContainerPropertyControl &panel, DClassID for_class_id,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
  PropPanel::ContainerPropertyControl *commonGrp = panel.getContainerById(PID_COMMON_GROUP);

  bool one_type = true;
  int one_layer = -1;

  bool targetLayerEnabled = true;
  for (int i = 0; i < objects.size(); ++i)
    if (LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(objects[i]))
    {
      if (one_layer == -1)
        one_layer = o->getEditLayerIdx();
      else if (one_layer != o->getEditLayerIdx())
        one_layer = -2;
    }
    else
    {
      one_layer = -2;
      one_type = false;
      targetLayerEnabled = false;
      break;
    }

  int targetLayerValue = 0;
  Tab<String> targetLayers(tmpmem);
  if (one_type)
  {
    targetLayerValue = EditLayerProps::findTypeIdx(EditLayerProps::ENT, one_layer);
    getObjEditor()->getLayerNames(EditLayerProps::ENT, targetLayers);
    targetLayerEnabled = targetLayers.size() > 1;
  }
  if (one_layer < 0 || !one_type)
  {
    targetLayerValue = targetLayers.size();
    targetLayers.push_back(String(place_types_full[Props::PT_count]));
  }

  commonGrp->createStatic(-1, "Edit layer:");
  commonGrp->createCombo(PID_ENTITY_LAYERS, "", targetLayers, targetLayerValue, targetLayerEnabled);

  if (one_type)
  {
    int plColl = props.placeType;
    String entName(props.entityName);
    String entNotes(props.notes);

    ::defaultHMLEntity.placeTypeMixed = false;
    for (int i = 0; i < objects.size(); ++i)
    {
      LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(objects[i]);
      if (!o)
        continue;

      if (o->props.placeType != props.placeType)
      {
        plColl = -1;
        ::defaultHMLEntity.placeTypeMixed = true;
      }
      if (strcmp(o->props.entityName, props.entityName) != 0)
        entName = "";
      if (o->props.notes != entNotes)
        entNotes = "";
    }

    commonGrp->createEditBox(PID_ENTITY_NOTES, "Notes", entNotes);
    commonGrp->createIndent();
    commonGrp->createButton(PID_ENTITYNAME, entName);

    PropPanel::ContainerPropertyControl *transformGrp =
      getObjEditor()->createPanelGroup(RenderableEditableObject::PID_TRANSFORM_GROUP);

    getObjEditor()->createPanelTransform(CM_OBJED_MODE_MOVE);
    getObjEditor()->createPanelTransform(CM_OBJED_MODE_ROTATE);
    getObjEditor()->createPanelTransform(CM_OBJED_MODE_SCALE);

    PropPanel::ContainerPropertyControl &placeContainer = *transformGrp->createContainer(PID_PLACE_TYPE);
    placeContainer.setHorizontalSpaceBetweenControls(hdpi::Px(0));
    placeContainer.createStatic(0, "Place on collision:");
    const bool placeTypeRadio = ::defaultHMLEntity.placeTypeTab == PID_PLACE_TYPE_BTN_RADIO;
    placeContainer.createButton(PID_PLACE_TYPE_BTN_RADIO, "Radiobuttons", !placeTypeRadio);
    placeContainer.createButton(PID_PLACE_TYPE_BTN_DROPDOWN, "Dropdown", placeTypeRadio, false);

    PropPanel::ContainerPropertyControl &placeTabs = *transformGrp->createContainer(PID_PLACE_TYPE_TABS);
    int placeTypeValue = plColl < 0 ? Props::PT_count : plColl;
    if (placeTypeRadio)
    {
      PropPanel::ContainerPropertyControl &placeGrp = *placeTabs.createRadioGroup(PID_PLACE_TYPE_RADIO, "");
      for (int i = props.PT_none; i < props.PT_count; ++i)
        placeGrp.createRadio(i, place_types_full[i]);
      if (plColl < 0)
        placeGrp.createRadio(Props::PT_count, place_types_full[Props::PT_count]);
      placeGrp.setIntValue(placeTypeValue);
    }
    else
    {
      Tab<String> placeTypes(tmpmem);
      placeTypes.insert(placeTypes.end(), eastl::begin(place_types_full), eastl::begin(place_types_full) + Props::PT_count);
      if (plColl < 0)
        placeTypes.push_back(String(place_types_full[Props::PT_count]));
      placeTabs.createCombo(PID_PLACE_TYPE_DROPDOWN, "", placeTypes, placeTypeValue, true);
    }

    transformGrp->createCheckBox(PID_PLACE_TYPE_OVERRIDE, "Override placement type for composit", props.overridePlaceTypeForComposit);

    if (entity)
    {
      IEntityCollisionState *ecs = entity->queryInterface<IEntityCollisionState>();
      if (ecs)
        panel.createCheckBox(PID_ENTITY_COLLISION, "Has collision", isCollidable);
    }

    fillMaterialProps(panel);

    PropPanel::ContainerPropertyControl *seedGrp = getObjEditor()->createPanelGroup(RenderableEditableObject::PID_SEED_GROUP);

    seedGrp->createStatic(0, "Generate per-inst seed:");
    seedGrp->createButton(PID_GENERATE_PERINST_SEED, "Individual");
    seedGrp->createButton(PID_GENERATE_EQUAL_PERINST_SEED, "Equal", true, false);
    if (entity && objects.size() == 1)
      if (IRandomSeedHolder *irsh = entity->queryInterface<IRandomSeedHolder>())
        seedGrp->createTrackInt(PID_PERINST_SEED, "Per-instance seed", irsh->getPerInstanceSeed() & 0x7FFF, 0, 32767, 1);

    seedGrp->createIndent();

    seedGrp->createStatic(0, "Generate seed:");
    seedGrp->createButton(PID_GENERATE_SEED, "Individual");
    seedGrp->createButton(PID_GENERATE_EQUAL_SEED, "Equal", true, false);
    if (entity && objects.size() == 1)
      if (IRandomSeedHolder *irsh = entity->queryInterface<IRandomSeedHolder>())
        if (irsh->isSeedSetSupported())
          seedGrp->createTrackInt(PID_SEED, "Random seed", irsh->getSeed() & 0x7FFF, 0, 32767, 1);

    objParam.fillParams(panel, objects);

    PropPanel::ContainerPropertyControl *casterGrp = panel.createGroup(PID_ENTITY_CASTER_GRP, "Entity casters");

    Tab<String> def_place_type_nm(tmpmem);
    def_place_type_nm.insert(def_place_type_nm.end(), eastl::begin(place_types_full),
      eastl::begin(place_types_full) + Props::PT_count);
    casterGrp->createCombo(PID_DEF_PLACE_TYPE, "Def. place type:", def_place_type_nm, default_place_type, true);

    casterGrp->createEditFloat(PID_TRACEOFFSET, "Tracert up offset", colliders.tracertUpOffset);
    casterGrp->createIndent();

    const int colCnt = DAGORED2->getCustomCollidersCount();
    G_ASSERT(colCnt < PID_ENTITY_CASTER_LAST - PID_ENTITY_CASTER_FIRST);

    for (int i = 0; i < colCnt; ++i)
    {
      const IDagorEdCustomCollider *collider = DAGORED2->getCustomCollider(i);

      if (collider)
      {
        casterGrp->createCheckBox(PID_ENTITY_CASTER_FIRST + i, collider->getColliderName(), isColliderEnabled(collider));
      }
    }

    panel.createCheckBox(PID_ENTITY_USE_FILTER, "Apply Filters", colliders.useFilters);

    if (colliders.useFilters)
    {
      PropPanel::ContainerPropertyControl *filterGrp = panel.createGroup(PID_ENTITY_FILTER_GRP, "Entity filters");

      unsigned old_mask = 0;

      old_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
      DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION, colliders.filter);

      const int plgCnt = DAGORED2->getPluginCount();
      for (int i = 0; i < plgCnt; i++)
      {
        IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);
        IObjEntityFilter *filter = plugin->queryInterface<IObjEntityFilter>();

        if (filter && filter->allowFiltering(IObjEntityFilter::STMASK_TYPE_COLLISION))
        {
          const bool val = filter->isFilteringActive(IObjEntityFilter::STMASK_TYPE_COLLISION);
          filterGrp->createCheckBox(PID_ENTITY_FILTER_FIRST + i, plugin->getMenuCommandName(), val);
        }
      }

      DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION, old_mask);
    }
  }
}

void LandscapeEntityObject::rePlaceAllEntities()
{
  HmapLandObjectEditor *ed = (HmapLandObjectEditor *)getObjEditor();
  if (!ed)
    return;

  DAGORED2->setColliders(colliders.col, colliders.getFilter());
  for (int i = ed->objectCount() - 1; i >= 0; i--)
  {
    LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(ed->getObject(i));
    if (o)
      o->updateEntityPosition();
  }

  DAGORED2->restoreEditorColliders();
  DAGORED2->invalidateViewportCache();
}

void LandscapeEntityObject::fillMaterialProps(PropPanel::ContainerPropertyControl &panel)
{
  decalMaterialIndices.clear();
  if (!entity)
    return;

  Tab<IStaticGeometryMaterialEditor::MaterialEntry> materialEntries;
  for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
  {
    IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);
    IStaticGeometryMaterialEditor *decalEditor = plugin->queryInterface<IStaticGeometryMaterialEditor>();
    if (!decalEditor)
      continue;
    decalEditor->getMaterialsForDecalEntity(entity, materialEntries);
  }

  if (materialEntries.size())
  {
    PropPanel::ContainerPropertyControl &materialGroup = *panel.createGroup(PID_DECAL_MATERIAL_GROUP, "Material Colors");
    for (int iMat = 0; iMat < materialEntries.size(); iMat++)
    {
      if (iMat > decal_material_max_count)
        break;

      IStaticGeometryMaterialEditor::MaterialEntry &materialEntry = materialEntries[iMat];
      decalMaterialIndices.push_back({materialEntry.node_idx, materialEntry.mat_idx});

      String colorString(128, "Color for Material %d-%d (%s)", materialEntry.node_idx, materialEntry.mat_idx,
        materialEntry.material->name);
      materialGroup.createColorBox(PID_DECAL_MATERIAL_FIRST + iMat, colorString.c_str(), E3DCOLOR(materialEntry.color));
    }

    materialGroup.createButton(PID_DECAL_MATERIAL_SAVE, "Save material changes");
  }
}

void LandscapeEntityObject::rePlaceAllEntitiesOverRI(HmapLandObjectEditor &objEd)
{
  DAGORED2->setColliders(colliders.col, colliders.getFilter());
  for (int i = objEd.objectCount() - 1; i >= 0; i--)
    if (LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(objEd.getObject(i)))
      if (o->props.placeType == o->props.PT_riColl)
        o->updateEntityPosition();
  DAGORED2->restoreEditorColliders();
  DAGORED2->invalidateViewportCache();
}

void LandscapeEntityObject::rePlaceAllEntitiesOnCollision(HmapLandObjectEditor &objEd, bool loft_changed, bool polygeom_changed,
  bool roads_chanded, BBox3 changed_region)
{
  bool need_work = false;
  for (int i = 0; i < colliders.col.size(); i++)
    if ((loft_changed && colliders.col[i] == &objEd.loftGeomCollider) ||
        (polygeom_changed && colliders.col[i] == &objEd.polyGeomCollider) || (roads_chanded && colliders.col[i] == &objEd))
    {
      need_work = true;
      break;
    }
  if (!need_work)
    return;

  DAGORED2->setColliders(colliders.col, colliders.getFilter());
  for (int i = objEd.objectCount() - 1; i >= 0; i--)
  {
    LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(objEd.getObject(i));
    if (o && o->props.placeType && o->entity && (o->matrix * o->entity->getBsph() & changed_region))
      o->updateEntityPosition();
  }

  DAGORED2->restoreEditorColliders();
  DAGORED2->invalidateViewportCache();
}

void LandscapeEntityObject::changeAssset(ObjectEditor &object_editor, dag::ConstSpan<RenderableEditableObject *> objects,
  const char *initially_selected_asset_name)
{
  class UndoEntityNamePropsChange : public UndoPropsChange
  {
  public:
    explicit UndoEntityNamePropsChange(LandscapeEntityObject *o) : UndoPropsChange(o) {}

    virtual void restore(bool save_redo)
    {
      UndoPropsChange::restore(save_redo);
      on_object_entity_name_changed(*getObj());
    }

    virtual void redo()
    {
      UndoPropsChange::redo();
      on_object_entity_name_changed(*getObj());
    }
  };

  const char *asset = DAEDITOR3.selectAsset(initially_selected_asset_name, "Select entity", DAEDITOR3.getGenObjAssetTypes());
  if (!asset)
    return;

  object_editor.getUndoSystem()->begin();
  for (int i = 0; i < objects.size(); i++)
  {
    LandscapeEntityObject *p = RTTI_cast<LandscapeEntityObject>(objects[i]);
    if (p)
    {
      object_editor.getUndoSystem()->put(new UndoEntityNamePropsChange(p));
      p->props.entityName = asset;
      p->propsChanged();
      on_object_entity_name_changed(*p);
    }
  }
  object_editor.getUndoSystem()->accept("Change entity");
  DAGORED2->repaint();
}

void LandscapeEntityObject::updateEntityPosition(bool apply_collision)
{
  if (!entity)
    return;

  if (props.placeType == props.PT_riColl)
  {
    apply_collision = false;
    DAGORED2->restoreEditorColliders();
  }

  if (apply_collision)
    DAGORED2->setColliders(colliders.col, colliders.getFilter());
  if (!props.placeType)
    entity->setTm(matrix);
  else
    setPosOnCollision(matrix.getcol(3));
  if (apply_collision)
    DAGORED2->restoreEditorColliders();
}

void LandscapeEntityObject::onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
  if (objParam.onPPChange(panel, pid, objects))
    return;

#define CHANGE_VAL(type, pname, getfunc)                                       \
  {                                                                            \
    type val = panel.getfunc(pid);                                             \
    for (int i = 0; i < objects.size(); ++i)                                   \
    {                                                                          \
      LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(objects[i]); \
      if (!o || o->pname == val)                                               \
        continue;                                                              \
      getObjEditor()->getUndoSystem()->put(new UndoPropsChange(o));            \
      o->pname = val;                                                          \
      o->propsChanged();                                                       \
    }                                                                          \
  }

  if (pid == PID_ENTITY_NOTES)
  {
    for (int i = 0; i < objects.size(); ++i)
    {
      LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(objects[i]);
      if (o)
        o->props.notes = panel.getText(PID_ENTITY_NOTES);
    }
    DAGORED2->invalidateViewportCache();
  }
  else if (pid == PID_ENTITY_LAYERS)
  {
    int selectedLayer = panel.getInt(PID_ENTITY_LAYERS);
    int layerIdx = EditLayerProps::findLayerByTypeIdx(EditLayerProps::ENT, selectedLayer);
    if (layerIdx != -1)
    {
      dag::Vector<RenderableEditableObject *> objectsToMove;
      objectsToMove.set_capacity(objects.size());
      objectsToMove.insert(objectsToMove.end(), objects.begin(), objects.end());
      HmapLandPlugin::self->moveObjectsToLayer(layerIdx, make_span(objectsToMove));

      DAGORED2->invalidateViewportCache();
      getObjEditor()->invalidateObjectProps();
    }
  }
  else if (((pid == PID_PLACE_TYPE_RADIO || pid == PID_PLACE_TYPE_DROPDOWN) &&
             (panel.getInt(pid) >= 0 && panel.getInt(pid) < Props::PT_count)) ||
           pid == PID_PLACE_TYPE_OVERRIDE)
  {
    bool invalidate = false;
    getObjEditor()->getUndoSystem()->begin();
    if (pid == PID_PLACE_TYPE_RADIO || pid == PID_PLACE_TYPE_DROPDOWN)
    {
      CHANGE_VAL(int, props.placeType, getInt)
      invalidate = ::defaultHMLEntity.placeTypeMixed;
    }
    else // if (pid == PID_PLACE_TYPE_OVERRIDE)
      CHANGE_VAL(bool, props.overridePlaceTypeForComposit, getBool)
    getObjEditor()->getUndoSystem()->accept("Change props");

    DAGORED2->setColliders(colliders.col, colliders.getFilter());
    for (int i = objects.size() - 1; i >= 0; i--)
    {
      LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(objects[i]);
      if (o)
        o->updateEntityPosition();
    }

    DAGORED2->restoreEditorColliders();
    DAGORED2->invalidateViewportCache();

    if (invalidate)
      getObjEditor()->invalidateObjectProps();
  }
  else if (pid == PID_DEF_PLACE_TYPE)
    default_place_type = panel.getInt(pid);
  else if (pid == PID_ENTITY_COLLISION)
  {
    getObjEditor()->getUndoSystem()->begin();
    CHANGE_VAL(bool, isCollidable, getBool)
    getObjEditor()->getUndoSystem()->accept("Change props");
  }
  else if (pid == PID_TRACEOFFSET)
  {
    float ofs = panel.getFloat(PID_TRACEOFFSET);

    if (float_nonzero(colliders.tracertUpOffset - ofs))
    {
      colliders.tracertUpOffset = ofs;
      rePlaceAllEntities();
    }
  }
  else if (pid >= PID_ENTITY_CASTER_FIRST && pid < PID_ENTITY_CASTER_LAST)
  {
    getObjEditor()->getUndoSystem()->begin();
    getObjEditor()->getUndoSystem()->put(new UndoStaticPropsChange());
    getObjEditor()->getUndoSystem()->accept("Change entity colliders");

    colliders.col.clear();
    for (int i = DAGORED2->getCustomCollidersCount() - 1; i >= 0; i--)
    {
      IDagorEdCustomCollider *collider = DAGORED2->getCustomCollider(i);
      if (panel.getBool(PID_ENTITY_CASTER_FIRST + i))
        colliders.col.push_back(collider);
    }

    rePlaceAllEntities();
  }
  else if (pid == PID_ENTITY_USE_FILTER)
  {
    getObjEditor()->getUndoSystem()->begin();
    getObjEditor()->getUndoSystem()->put(new UndoStaticPropsChange());
    getObjEditor()->getUndoSystem()->accept("Change entity filtering");

    colliders.useFilters = panel.getBool(PID_ENTITY_USE_FILTER);

    getObjEditor()->invalidateObjectProps();
    rePlaceAllEntities();
  }
  else if (pid >= PID_ENTITY_FILTER_FIRST && pid < PID_ENTITY_FILTER_LAST)
  {
    if (panel.getBool(PID_ENTITY_USE_FILTER))
    {
      const int id = pid - PID_ENTITY_FILTER_FIRST;
      if (id >= DAGORED2->getPluginCount())
        return;

      getObjEditor()->getUndoSystem()->begin();
      getObjEditor()->getUndoSystem()->put(new UndoStaticPropsChange());
      getObjEditor()->getUndoSystem()->accept("Change entity filtering");

      IGenEditorPlugin *plugin = DAGORED2->getPlugin(pid - PID_ENTITY_FILTER_FIRST);
      IObjEntityFilter *filter = plugin->queryInterface<IObjEntityFilter>();
      G_ASSERT(filter && filter->allowFiltering(IObjEntityFilter::STMASK_TYPE_COLLISION));

      unsigned old_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);

      DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION, colliders.filter);
      filter->applyFiltering(IObjEntityFilter::STMASK_TYPE_COLLISION, panel.getBool(pid));
      colliders.filter = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);

      DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION, old_mask);

      rePlaceAllEntities();
    }
  }
  else if (pid == PID_SEED && objects.size() == 1)
  {
    if (LandscapeEntityObject *p = RTTI_cast<LandscapeEntityObject>(objects[0]))
      p->setRndSeed(panel.getInt(pid));
  }
  else if (pid == PID_PERINST_SEED && objects.size() == 1)
  {
    if (LandscapeEntityObject *p = RTTI_cast<LandscapeEntityObject>(objects[0]))
      p->setPerInstSeed(panel.getInt(pid));
  }
  else if (pid >= PID_DECAL_MATERIAL_FIRST && pid <= PID_DECAL_MATERIAL_LAST)
  {
    int materialEntryIndex = pid - PID_DECAL_MATERIAL_FIRST;
    for (RenderableEditableObject *obj : objects)
    {
      LandscapeEntityObject *p = RTTI_cast<LandscapeEntityObject>(obj);
      const DecalMaterialIndex &decalMaterialIndex = decalMaterialIndices[materialEntryIndex];
      E3DCOLOR color = panel.getColor(pid);

      for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
      {
        IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);
        IStaticGeometryMaterialEditor *decalEditor = plugin->queryInterface<IStaticGeometryMaterialEditor>();
        if (!decalEditor)
          continue;
        decalEditor->setMaterialColorForEntity(p->entity, decalMaterialIndex.node_idx, decalMaterialIndex.material_idx, color);
      }
      p->propsChanged();
    }
  }

#undef CHANGE_VAL
}

void LandscapeEntityObject::onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
  if (pid == PID_PLACE_TYPE_BTN_RADIO || pid == PID_PLACE_TYPE_BTN_DROPDOWN)
  {
    ::defaultHMLEntity.placeTypeTab = pid;
  }
  else if (pid == PID_ENTITYNAME)
  {
    changeAssset(*objEditor, objects, props.entityName);
  }
  else if (pid == PID_GENERATE_SEED || pid == PID_GENERATE_PERINST_SEED)
  {
    bool gen_rnd_seed = (pid == PID_GENERATE_SEED);
    for (int i = objects.size() - 1; i >= 0; i--)
    {
      LandscapeEntityObject *p = RTTI_cast<LandscapeEntityObject>(objects[i]);

      gen_rnd_seed ? p->setRndSeed(grnd()) : p->setPerInstSeed(grnd());
      if (objects.size() == 1)
        panel.setInt(gen_rnd_seed ? PID_SEED : PID_PERINST_SEED, gen_rnd_seed ? p->rndSeed : p->perInstSeed);
    }

    DAGORED2->invalidateViewportCache();
  }
  else if (pid == PID_GENERATE_EQUAL_SEED || pid == PID_GENERATE_EQUAL_PERINST_SEED)
  {
    bool gen_rnd_seed = (pid == PID_GENERATE_EQUAL_SEED);
    const int seed = grnd();

    for (int i = objects.size() - 1; i >= 0; i--)
    {
      LandscapeEntityObject *p = RTTI_cast<LandscapeEntityObject>(objects[i]);

      gen_rnd_seed ? p->setRndSeed(seed) : p->setPerInstSeed(seed);
    }
    if (objects.size() == 1)
      panel.setInt(gen_rnd_seed ? PID_SEED : PID_PERINST_SEED, seed);

    DAGORED2->invalidateViewportCache();
  }
  else if (pid == PID_DECAL_MATERIAL_SAVE)
  {
    for (int i = 0; i < DAGORED2->getPluginCount(); ++i)
    {
      IGenEditorPlugin *plugin = DAGORED2->getPlugin(i);
      IStaticGeometryMaterialEditor *decalEditor = plugin->queryInterface<IStaticGeometryMaterialEditor>();
      if (!decalEditor)
        continue;
      decalEditor->saveStaticGeometryAssetByEntity(entity);
    }
  }

  getObjEditor()->invalidateObjectProps();
}

void LandscapeEntityObject::saveColliders(DataBlock &blk)
{
  DataBlock *colBlk = blk.addBlock("entity_colliders");

  colBlk->addReal("tracertUpOffset", colliders.tracertUpOffset);

  DAGORED2->saveColliders(*colBlk, colliders.col, colliders.filter, colliders.useFilters);
}

void LandscapeEntityObject::loadColliders(const DataBlock &blk)
{
  const DataBlock *colBlk = blk.getBlockByName("entity_colliders");
  if (colBlk)
  {
    colliders.col = DAGORED2->loadColliders(blk, colliders.filter, "entity_colliders");
    colliders.useFilters = colBlk->getBool("applyFilters", false);
    colliders.tracertUpOffset = colBlk->getReal("tracertUpOffset", 1.0);
  }
  else
  {
    colliders.useFilters = false;
    colliders.tracertUpOffset = 1.0;

    for (int i = DAGORED2->getCustomCollidersCount() - 1; i >= 0; i--)
    {
      IDagorEdCustomCollider *collider = DAGORED2->getCustomCollider(i);
      if (collider)
        colliders.col.push_back(collider);
    }
  }
}

void LandscapeEntityObject::save(DataBlock &blk)
{
  blk.setStr("name", getName());
  blk.setStr("notes", props.notes);
  blk.setStr("entName", props.entityName);
  blk.setInt("place_type", props.placeType);
  if (props.overridePlaceTypeForComposit)
    blk.setBool("force_cmp_place_type", props.overridePlaceTypeForComposit);

  blk.setTm("tm", matrix);

  // fx
  DataBlock *sblk = NULL;
  if (fxProps.maxRadius != 10.f)
  {
    sblk = blk.addBlock("fx");
    sblk->setReal("maxRadius", fxProps.maxRadius);
  }
  if (fxProps.updateWhenInvisible)
  {
    if (!sblk)
      sblk = blk.addBlock("fx");
    sblk->setBool("updateWhenInvisible", fxProps.updateWhenInvisible);
  }

  // physObj
  sblk = NULL;
  if (physObjProps.active)
  {
    sblk = blk.addBlock("physObj");
    sblk->setBool("physActive", physObjProps.active);
  }
  if (physObjProps.scriptClass.length())
  {
    if (!sblk)
      sblk = blk.addBlock("physObj");
    sblk->setStr("scriptClass", physObjProps.scriptClass);
  }

  if (entity)
  {
    IRandomSeedHolder *irsh = entity->queryInterface<IRandomSeedHolder>();
    if (irsh && (rndSeed != -1))
      blk.addInt("entSeed", rndSeed);
    if (irsh && perInstSeed)
      blk.addInt("entPerInstSeed", perInstSeed);

    if (!isCollidable)
      blk.addBool("isCollidable", isCollidable);
  }
}
void LandscapeEntityObject::load(const DataBlock &blk)
{
  getObjEditor()->setUniqName(this, blk.getStr("name", ""));
  props.notes = blk.getStr("notes", "");
  props.entityName = blk.getStr("entName", NULL);
  if (!blk.getBool("place_on_collision", true))
    props.placeType = props.PT_none;
  else if (blk.getBool("use_collision_norm", false))
    props.placeType = props.PT_collNorm;
  else
    props.placeType = props.PT_coll;
  props.placeType = blk.getInt("place_type", props.placeType);
  props.overridePlaceTypeForComposit = blk.getBool("force_cmp_place_type", false);
  TMatrix _tm = blk.getTm("tm", TMatrix::IDENT);
  if (check_nan(_tm))
  {
    DAEDITOR3.conError("entity <%s> (%s) has invalid TM=%@", getName(), props.entityName, _tm);
    if (d3d::is_stub_driver())
    {
      DAEDITOR3.conError("entity <%s> with invalid TM removed!", getName());
      getObjEditor()->removeObject(this, false);
      return;
    }
    else
    {
      if (check_nan(_tm.getcol(3)))
        _tm = TMatrix::IDENT;
      else
      {
        _tm.setcol(0, TMatrix::IDENT.getcol(0));
        _tm.setcol(1, TMatrix::IDENT.getcol(1));
        _tm.setcol(2, TMatrix::IDENT.getcol(2));
      }
      DAEDITOR3.conWarning("entity <%s>: replaced invalid TM with %@", getName(), _tm);
    }
  }
  setWtm(_tm);

  // fx
  const DataBlock *sblk = blk.getBlockByName("fx");
  fxProps.maxRadius = sblk ? sblk->getReal("maxRadius", fxProps.maxRadius) : fxProps.maxRadius;
  fxProps.updateWhenInvisible = sblk ? sblk->getBool("updateWhenInvisible", false) : false;

  // physObj
  sblk = blk.getBlockByName("physObj");
  physObjProps.active = sblk ? sblk->getBool("physActive", false) : false;
  physObjProps.scriptClass = sblk ? sblk->getStr("scriptClass", "") : "";

  rndSeed = blk.getInt("entSeed", -1);
  perInstSeed = blk.getInt("entPerInstSeed", 0);
  isCollidable = blk.getBool("isCollidable", true);
  propsChanged(true); // gizmoTranformMode will be reset later in HmapLandPlugin::beforeMainLoop()
  if (entity && props.placeType != props.PT_none && props.placeType != props.PT_riColl)
    setWtm(_tm);
}

void LandscapeEntityObject::setRndSeed(int seed)
{
  rndSeed = seed;
  if (!entity)
    return;

  IRandomSeedHolder *irsh = entity->queryInterface<IRandomSeedHolder>();
  if (irsh)
    irsh->setSeed(rndSeed);
}

void LandscapeEntityObject::setPerInstSeed(int seed)
{
  perInstSeed = seed;
  if (!entity)
    return;

  IRandomSeedHolder *irsh = entity->queryInterface<IRandomSeedHolder>();
  if (irsh)
    irsh->setPerInstanceSeed(perInstSeed);
}

bool LandscapeEntityObject::setName(const char *nm)
{
  const bool result = RenderableEditableObject::setName(nm);

  HmapLandObjectEditor *editor = static_cast<HmapLandObjectEditor *>(getObjEditor());
  if (editor)
    editor->onRegisteredObjectNameChanged(*this);

  return result;
}

void LandscapeEntityObject::setWtm(const TMatrix &wtm)
{
  RenderableEditableObject::setWtm(wtm);
  if (entity)
    updateEntityPosition(true);
}

void LandscapeEntityObject::setGizmoTranformMode(bool enable)
{
  if (gizmoEnabled == enable)
    return;
  if (entity)
  {
    entity->setGizmoTranformMode(enable);
    if (gizmoEnabled && !enable)
      updateEntityPosition(true);
  }
  gizmoEnabled = enable;
}

void LandscapeEntityObject::onRemove(ObjectEditor *) { destroy_it(entity); }
void LandscapeEntityObject::onAdd(ObjectEditor *objEditor)
{
  propsChanged();

  if (name.empty())
  {
    String fn(dd_get_fname(props.entityName));
    objEditor->setUniqName(this, fn);
  }
}

void LandscapeEntityObject::setPosOnCollision(Point3 pos)
{
  int stype = entity->getSubtype();
  entity->setSubtype(entity->ST_NOT_COLLIDABLE);

  TMatrix etm = matrix;
  if (props.placeType == props.PT_collNorm)
  {
    Point3 norm(0, 1, 0);
    objgenerator::place_on_ground(pos, norm, colliders.tracertUpOffset);
    if (fabs(matrix.getcol(0) * norm) < 0.999)
    {
      etm.setcol(1, norm);
      etm.setcol(2, normalize(etm.getcol(0) % norm));
      etm.setcol(0, normalize(norm % etm.getcol(2)));
    }
    else
    {
      etm.setcol(1, norm);
      etm.setcol(0, normalize(norm % etm.getcol(2)));
      etm.setcol(2, normalize(etm.getcol(0) % norm));
    }
  }
  else if (props.placeType == props.PT_coll)
    objgenerator::place_on_ground(pos, colliders.tracertUpOffset);
  else if (props.placeType == props.PT_3pod && entity)
  {
    MpPlacementRec mppRec;
    mppRec.mpOrientType = mppRec.MP_ORIENT_3POINT;
    mppRec.makePointsFromBox(entity->getBbox());
    mppRec.computePivotBc();

    etm.setcol(3, ZERO<Point3>());
    objgenerator::place_multipoint(mppRec, pos, etm, colliders.tracertUpOffset);
    objgenerator::rotate_multipoint(etm, mppRec);
  }
  else if (props.placeType == props.PT_fnd && entity)
  {
    BBox3 box = entity->getBbox();
    float dh;
    box[0].y = 0;
    dh = objgenerator::dist_to_ground(matrix * box.point(0), colliders.tracertUpOffset);
    inplace_max(dh, objgenerator::dist_to_ground(matrix * box.point(1), colliders.tracertUpOffset));
    inplace_max(dh, objgenerator::dist_to_ground(matrix * box.point(4), colliders.tracertUpOffset));
    inplace_max(dh, objgenerator::dist_to_ground(matrix * box.point(5), colliders.tracertUpOffset));
    pos.y -= dh;
  }
  else if (props.placeType == props.PT_flt && entity)
  {
    if (IWaterService *waterService = EDITORCORE->queryEditorInterface<IWaterService>())
      pos.y = waterService->get_level();
  }
  else if (props.placeType == props.PT_riColl && entity)
  {
    setCollisionIgnored();
    EDITORCORE->setupColliderParams(1, BBox3());
    objgenerator::place_on_plane(pos, savedPlacementNormal, colliders.tracertUpOffset);
    EDITORCORE->setupColliderParams(0, BBox3());
    resetCollisionIgnored();
  }

  etm.setcol(3, pos);
  entity->setSubtype(stype);
  entity->setTm(etm);
}

bool LandscapeEntityObject::setPos(const Point3 &p)
{
  if (!RenderableEditableObject::setPos(p))
    return false;

  if (entity)
    updateEntityPosition(true);

  return true;
}

void LandscapeEntityObject::setPlaceOnCollision(bool place_on_rendinst)
{
  if (!props.overridePlaceTypeForComposit)
    props.placeType = place_on_rendinst ? props.PT_riColl : default_place_type;
  setWtm(matrix);
}
void LandscapeEntityObject::objectPropsChanged()
{
  if (!entity)
    return;

  IObjEntityUserDataHolder *oeud = entity->queryInterface<IObjEntityUserDataHolder>();
  if (!oeud)
    return;

  static int fxId = IDaEditor3Engine::get().getAssetTypeId("fx");
  static int physObjId = IDaEditor3Engine::get().getAssetTypeId("physObj");

  int id = entity->getAssetTypeId();

  DataBlock *blk = oeud->getUserDataBlock(true);
  G_ASSERT(blk);

  if (id == fxId)
  {
    blk->setReal("maxRadius", fxProps.maxRadius);
    blk->setBool("updateWhenInvisible", fxProps.updateWhenInvisible);
  }
  else if (id == physObjId)
  {
    blk->setStr("name", getName());
    blk->setBool("physActive", physObjProps.active);
    blk->setStr("scriptClass", physObjProps.scriptClass);
  }
}
void LandscapeEntityObject::setEditLayerIdx(int idx)
{
  editLayerIdx = idx;
  if (entity)
    entity->setEditLayerIdx(editLayerIdx);
}
void LandscapeEntityObject::propsChanged(bool prevent_gen)
{
  destroy_it(entity);
  DagorAsset *a = DAEDITOR3.getGenObjAssetByName(props.entityName);
  if (!a && !props.entityName.empty())
    DAEDITOR3.conError("cannot find entity asset: <%s>", props.entityName.str());
  entity = a ? DAEDITOR3.createEntity(*a) : NULL;
  if (entity)
  {
    if (prevent_gen)
      setGizmoTranformMode(true); // will be reset later in HmapLandPlugin::beforeMainLoop()

    entity->setSubtype(IDaEditor3Engine::get().registerEntitySubTypeId("single_ent"));
    entity->setEditLayerIdx(editLayerIdx);

    IRandomSeedHolder *irsh = entity->queryInterface<IRandomSeedHolder>();
    if (irsh)
    {
      irsh->setSeed(rndSeed);
      irsh->setPerInstanceSeed(perInstSeed);
    }
    if (ICompositObj *ico = entity->queryInterface<ICompositObj>())
      ico->setCompositPlaceTypeOverride(props.overridePlaceTypeForComposit ? props.placeType : -1);

    IEntityCollisionState *ecs = entity->queryInterface<IEntityCollisionState>();
    if (ecs)
      ecs->setCollisionFlag(isCollidable);

    DAGORED2->setColliders(colliders.col, colliders.getFilter());
    entity->setTm(matrix);
    DAGORED2->restoreEditorColliders();
    objectPropsChanged();
  }
}

LandscapeEntityObject *LandscapeEntityObject::clone()
{
  LandscapeEntityObject *obj = new LandscapeEntityObject(props.entityName);
  obj->setEditLayerIdx(EditLayerProps::activeLayerIdx[lpIndex()]);

  getObjEditor()->setUniqName(obj, getName());

  Props pr = obj->getProps();
  pr.placeType = props.placeType;
  obj->setProps(pr);

  TMatrix tm = getWtm();
  obj->setWtm(tm);

  return obj;
}

void LandscapeEntityObject::putMoveUndo()
{
  HmapLandObjectEditor *ed = (HmapLandObjectEditor *)getObjEditor();
  if (!ed->isCloneMode())
    RenderableEditableObject::putMoveUndo();
}

bool HmapLandObjectEditor::splitComposits(const PtrTab<RenderableEditableObject> &sel, PtrTab<LandscapeEntityObject> &compObj,
  PtrTab<LandscapeEntityObject> &decompObj, DataBlock &splitSplinesBlk, PtrTab<RenderableEditableObject> &otherObj)
{
  for (int i = 0; i < sel.size(); ++i)
  {
    LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(sel[i]);
    if (o && o->getEntity() && o->getEntity()->queryInterface<ICompositObj>())
      compObj.push_back(o);
    else
      otherObj.push_back(sel[i]);
  }
  if (!compObj.size())
    return false;

  for (int i = 0; i < compObj.size(); i++)
  {
    ICompositObj *co = compObj[i]->getEntity()->queryInterface<ICompositObj>();
    for (int j = 0, je = co->getCompositSubEntityCount(); j < je; j++)
      if (IObjEntity *e = co->getCompositSubEntity(j))
      {
        if (ISplineEntity *se = e->queryInterface<ISplineEntity>())
        {
          DataBlock splineBlk;
          if (se->saveSplineTo(splineBlk) && splineBlk.blockCount() == 1)
          {
            splineBlk.getBlock(0)->setStr("name",
              String(0, "%s_%s", compObj[i]->getName(), splineBlk.getBlock(0)->getStr("blkGenName", "")));
            splitSplinesBlk.addNewBlock(splineBlk.getBlock(0));
          }
          continue;
        }

        if (!e->getObjAssetName())
          continue;
        String nm(e->getObjAssetName());
        int seed = 0;
        if (IRandomSeedHolder *irsh = e->queryInterface<IRandomSeedHolder>())
          seed = irsh->getSeed();
        else if (IRandomSeedHolder *irsh = compObj[i]->getEntity()->queryInterface<IRandomSeedHolder>())
          seed = irsh->getSeed();

        LandscapeEntityObject *obj = new LandscapeEntityObject(nm, seed);
        obj->setEditLayerIdx(EditLayerProps::activeLayerIdx[obj->lpIndex()]);
        const ICompositObj::Props &p = co->getCompositSubEntityProps(j);

        if (char *p = strrchr(nm, ':'))
          *p = 0;
        setUniqName(obj, String(0, "%s_%s", compObj[i]->getName(), nm));

        LandscapeEntityObject::Props pr = obj->getProps();
        pr.placeType = p.placeType;
        obj->setProps(pr);
        TMatrix tm(TMatrix::IDENT);
        e->getTm(tm);
        obj->setWtm(tm);
        decompObj.push_back(obj);
      }
  }
  return true;
}

void HmapLandObjectEditor::splitComposits()
{
  PtrTab<RenderableEditableObject> otherObj;
  PtrTab<LandscapeEntityObject> compObj, decompObj;
  DataBlock splitSplinesBlk;
  if (!HmapLandObjectEditor::splitComposits(selection, compObj, decompObj, splitSplinesBlk, otherObj))
  {
    wingw::message_box(wingw::MBS_INFO, "Cannot split composits", "Selected %d objects do not contain any composits",
      selection.size());
    return;
  }
  otherObj.clear();

  eastl::unique_ptr<PropPanel::DialogWindow> dialog(
    DAGORED2->createDialog(hdpi::_pxScaled(280), hdpi::_pxScaled(100), "Split Composites"));
  PropPanel::ContainerPropertyControl *panel = dialog->getPanel();
  panel->createCheckBox(PID_SPLIT_RECURSIVE, "Recursive (split sub-components as well)", ::defaultHMLEntity.splitCompRecursive);
  dialog->setDialogButtonText(PropPanel::DIALOG_ID_OK, "Split");

  if (dialog->showDialog() == PropPanel::DIALOG_ID_CANCEL)
    return;

  const bool recursive = panel->getBool(PID_SPLIT_RECURSIVE);
  ::defaultHMLEntity.splitCompRecursive = recursive;

  if (recursive)
  {
    PtrTab<RenderableEditableObject> new_sel, new_objs;
    PtrTab<LandscapeEntityObject> compObjItermediate;
    do
    {
      new_sel.clear();
      new_sel.reserve(decompObj.size());
      for (auto &o : decompObj)
        new_sel.push_back(o);
      decompObj.clear();
      compObjItermediate.clear();
    } while (HmapLandObjectEditor::splitComposits(new_sel, compObjItermediate, decompObj, splitSplinesBlk, new_objs));

    decompObj.reserve(new_objs.size());
    for (auto &new_obj : new_objs)
      if (LandscapeEntityObject *o = RTTI_cast<LandscapeEntityObject>(new_obj))
        decompObj.push_back(o);
  }

  getUndoSystem()->begin();
  removeObjects((RenderableEditableObject **)compObj.data(), compObj.size(), true);
  addObjects((RenderableEditableObject **)decompObj.data(), decompObj.size(), true);

  String tmpName;
  for (int i = 0; i < decompObj.size(); i++)
  {
    tmpName = decompObj[i]->getName();
    decompObj[i]->setName("");
    setUniqName(decompObj[i], tmpName);
    decompObj[i]->selectObject();
    decompObj[i]->propsChanged();
  }
  for (int i = 0; i < splitSplinesBlk.blockCount(); i++)
  {
    const DataBlock &b = *splitSplinesBlk.getBlock(i);
    SplineObject *s = new SplineObject(strcmp(b.getBlockName(), "polygon") == 0);
    s->setEditLayerIdx(EditLayerProps::activeLayerIdx[s->lpIndex()]);
    addObject(s, true);
    s->load(b, true);
    setUniqName(s, b.getStr("name"));
    s->onCreated(false);
    s->selectObject();
  }
  updateSelection();

  getUndoSystem()->accept(String(0, "Decomposit %d objects", compObj.size()));

  wingw::message_box(wingw::MBS_INFO, "Composits are splitted", "%d composit objects are splitted into %d subobjects and %d splines",
    compObj.size(), decompObj.size(), splitSplinesBlk.blockCount());
}

void HmapLandObjectEditor::instantiateGenToEntities()
{
  Tab<SplineObject *> genObj;
  PtrTab<LandscapeEntityObject> decompObj;

  for (int i = 0; i < selection.size(); ++i)
    if (SplineObject *o = RTTI_cast<SplineObject>(selection[i]))
      genObj.push_back(o);
  if (!genObj.size())
  {
    wingw::message_box(wingw::MBS_INFO, "Cannot split generated", "Selected %d objects do not contain any splines", selection.size());
    return;
  }

  auto make_entities = [&decompObj, this](const char *parent_obj_nm, dag::ConstSpan<IObjEntity *> entities) {
    for (auto *e : entities)
    {
      if (!e->getObjAssetName())
        continue;
      String nm(e->getObjAssetName());
      int seed = 0;
      if (IRandomSeedHolder *irsh = e->queryInterface<IRandomSeedHolder>())
        seed = irsh->getSeed();

      LandscapeEntityObject *obj = new LandscapeEntityObject(nm, seed);
      obj->setEditLayerIdx(EditLayerProps::activeLayerIdx[obj->lpIndex()]);

      if (char *p = strrchr(nm, ':'))
        *p = 0;
      setUniqName(obj, String(0, "%s_%s", parent_obj_nm, nm));
      TMatrix tm(TMatrix::IDENT);
      e->getTm(tm);
      obj->setWtm(tm);
      decompObj.push_back(obj);
    }
  };

  for (auto *spl : genObj)
  {
    if (spl->points.size() < 2)
      continue;
    if (spl->isPoly() && spl->getLandClass())
    {
      for (auto &p : spl->getLandClass()->poolTiled)
        make_entities(spl->getName(), make_span_const(p.entUsed ? p.entPool.data() : nullptr, p.entUsed));
      for (auto &p : spl->getLandClass()->poolPlanted)
        make_entities(spl->getName(), make_span_const(p.entUsed ? p.entPool.data() : nullptr, p.entUsed));
    }
    for (auto sp : spl->points)
      if (ISplineGenObj *gen = sp->getSplineGen())
        for (auto &p : gen->entPools)
          make_entities(spl->getName(), make_span_const(p.entUsed ? p.entPool.data() : nullptr, p.entUsed));
  }
  if (wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNO, "Instantiate generated objects",
        String(0, "Split generated objects of %d splines into %d distinct entities and reset spline class?", genObj.size(),
          decompObj.size())) != wingw::MB_ID_YES)
    return;

  getUndoSystem()->begin();
  for (auto *spl : genObj)
  {
    if (spl->points.size() < 2)
      continue;
    for (auto sp : spl->points)
      sp->setEffectiveAsset("", true, 0);
    spl->points[0]->setBlkGenName("");
    spl->selectObject(false);
  }
  addObjects((RenderableEditableObject **)decompObj.data(), decompObj.size(), true);
  String tmpName;
  for (int i = 0; i < decompObj.size(); i++)
  {
    tmpName = decompObj[i]->getName();
    decompObj[i]->setName("");
    setUniqName(decompObj[i], tmpName);
    decompObj[i]->selectObject();
    decompObj[i]->propsChanged();
  }
  updateSelection();
  getUndoSystem()->accept(String(0, "Split %d splines into %d entities", genObj.size(), decompObj.size()));

  wingw::message_box(wingw::MBS_INFO, "Generated objects instantiated",
    "Generated objects of %d splines are splitted into %d entities", genObj.size(), decompObj.size());
}
