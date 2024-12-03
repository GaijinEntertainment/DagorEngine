require ecs


[es(tag=server, on_appear)]
def entity_prefab_create_entities_on_appear(evt : Event;
                                            eid : EntityId;
                                            [[shared_comp]] entity_prefab__templatesToCreate : Object;
                                            var entity_prefab__createdEntities : EidList)
  for maybeObject in entity_prefab__templatesToCreate
    let object : ecs::Object const? = maybeObject.value |> get_ecs_object()
    if object == null
      continue

    let createdEid = createEntity("{maybeObject.key}+entity_prefab_child") <| $(init)
      init |> set("entity_prefab__parentEid", eid)
      for comp in *object
        set(init, comp.key, comp.value)

    entity_prefab__createdEntities |> push(createdEid)

[es(on_appear)]
def entity_prefab_set_transform_on_appear(evt : Event;
                                          entity_prefab__parentEid : EntityId;
                                          relativeTransform : float3x4;
                                          var transform aka child_transform : float3x4)
  query(entity_prefab__parentEid) <| $ [es] (transform aka parent_transform : float3x4)
    child_transform = parent_transform * relativeTransform

[es(tag=server, on_disappear)]
def entity_prefab_remove_children_on_disappear(evt : Event;
                                               entity_prefab__createdEntities : EidList)
  for eid in entity_prefab__createdEntities
    destroyEntity(eid)
