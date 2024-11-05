// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daEditorE/editorCommon/inGameEditorStub.inc.cpp>

EditableObject *EntityObjEditor::cloneObject(EditableObject *, bool) { G_ASSERT_RETURN(false, nullptr); }
void EntityObjEditor::addEntityEx(ecs::EntityId, bool) { G_ASSERT(0); }
void EntityObjEditor::saveComponent(ecs::EntityId, const char *) { G_ASSERT(0); }
void EntityObjEditor::saveAddTemplate(ecs::EntityId, const char *) { G_ASSERT(0); }
void EntityObjEditor::saveDelTemplate(ecs::EntityId, const char *, bool) { G_ASSERT(0); }
EditableObject *EntityObjEditor::getObjectByEid(ecs::EntityId) const { G_ASSERT_RETURN(false, nullptr); }
ecs::EntityId EntityObjEditor::createEntityDirect(EntCreateData *) { G_ASSERT_RETURN(false, ecs::INVALID_ENTITY_ID); }
void EntityObjEditor::saveEntityToBlk(ecs::EntityId, DataBlock &) const { G_ASSERT(0); }
void EntityObjEditor::destroyEntityDirect(ecs::EntityId) { G_ASSERT(0); }
void EntityObjEditor::selectEntity(ecs::EntityId, bool) { G_ASSERT(0); }
void ObjectEditor::removeInvalidObjects() { G_ASSERT(0); }
void EntityObjEditor::refreshEids() { G_ASSERT(0); }
void EntityObjEditor::zoomAndCenter() { G_ASSERT(0); }
ecs::EntityId EntityObjEditor::getFirstSelectedEntity() { G_ASSERT_RETURN(false, ecs::INVALID_ENTITY_ID); }

#include <daEditorE/editorCommon/entityObj.h>
EntCreateData::EntCreateData(const char *, const TMatrix *) { G_ASSERT(0); }
EntCreateData::EntCreateData(const char *, const TMatrix *, const char *) { G_ASSERT(0); }
