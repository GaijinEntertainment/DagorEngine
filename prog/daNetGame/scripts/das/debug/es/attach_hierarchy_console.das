options no_aot
require ecs
require app
require DagorConsole

[console_cmd(name="hierarchy.attach")]
def hierarchy_attach_cmd(child, parent : uint)
  addSubTemplate(EntityId(child), "hierarchial_entity") <| $(var init)
    init |> set("hierarchy_parent", EntityId(parent))


[console_cmd(name="hierarchy.make_free_transform")]
def hierarchy_free_transform_cmd(eid : uint)
  addSubTemplate(EntityId(eid), "hierarchial_free_transform")

[console_cmd(name="hierarchy.make_fixed_transform")]
def hierarchy_fixed_transform_cmd(eid : uint)
  removeSubTemplate(EntityId(eid), "hierarchial_free_transform")

[console_cmd(name="hierarchy.detach")]
def hierarchy_detach_cmd(eid : uint)
  removeSubTemplate(EntityId(eid), "hierarchial_entity")


