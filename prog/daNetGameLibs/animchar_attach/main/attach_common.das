module attach_common shared
require ecs

def get_node_no(node : uint16)
  return int(node) & int(0xFF)

def find_base_attach_parent(parent_eid : EntityId)
  var parent = parent_eid
  query(parent_eid) <| $ [es] (animchar_attach__attachedTo : EntityId)
    parent = find_base_attach_parent(animchar_attach__attachedTo)
  return parent

def get_remapped_node_no(node : uint16)
  return (int(node) >> 8) & int(0xFF)
