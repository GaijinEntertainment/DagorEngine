require ecs
require DngNet
options indenting = 2

[generic]
def remote_recreate_entity_from(eid : EntityId const; templ : string) : EntityId
  return _builtin_remote_recreate_entity_from(eid, templ)
[generic]
def remote_recreate_entity_from(eid : EntityId const; templ : string;
                                subexpr : block<(var compInit : ComponentsInitializer) : void>) : EntityId
  return _builtin_remote_recreate_entity_from_block(eid, templ, subexpr)
[generic]
def remote_recreate_entity_from(eid : EntityId const; templ : string;
                                callback : lambda<(id : EntityId; var cInit : ComponentsInitializer) : void>;
                                subexpr : block<(var compInit : ComponentsInitializer) : void>) : EntityId
  return _builtin_remote_recreate_entity_from_block_lambda(eid, templ, callback, subexpr)

[generic]
def remote_add_sub_template(eid : EntityId const; templ : string; force : bool = false) : EntityId
  return remote_change_sub_template(eid, "", templ, force)
[generic]
def remote_add_sub_template(eid : EntityId const; templ : string;
                            subexpr : block<(var compInit : ComponentsInitializer) : void>; force : bool = false) : EntityId
  return remote_change_sub_template(eid, "", templ, subexpr, force)
[generic]
def remote_add_sub_template(eid : EntityId const; templ : string;
                            callback : lambda<(id : EntityId; var cInit : ComponentsInitializer) : void>;
                            subexpr : block<(var compInit : ComponentsInitializer) : void>) : EntityId
  return remote_change_sub_template(eid, "", templ, callback, subexpr)

[generic]
def remote_remove_sub_template(eid : EntityId const; templ : string; force : bool = false) : EntityId
  return remote_change_sub_template(eid, templ, "", force)
[generic]
def remote_remove_sub_template(eid : EntityId const; templ : string;
                               subexpr : block<(var compInit : ComponentsInitializer) : void>; force : bool = false) : EntityId
  return remote_change_sub_template(eid, templ, "", subexpr, force)
[generic]
def remote_remove_sub_template(eid : EntityId const; templ : string;
                               callback : lambda<(id : EntityId; var cInit : ComponentsInitializer) : void>;
                               subexpr : block<(var compInit : ComponentsInitializer) : void>) : EntityId
  return remote_change_sub_template(eid, templ, "", callback, subexpr)
