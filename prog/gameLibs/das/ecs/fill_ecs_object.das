module fill_ecs_object shared
require ecs
require daslib/apply


def add_to_ecs_object(var obj : Object; name : string; value)
  static_if typeinfo(is_function value)
    pass // skip functions
  static_if typeinfo(is_pointer value)
    if value != null
      add_to_ecs_object(obj, name, *value)
  static_elif typeinfo(is_struct value)
    using() <| $(var child : Object)
      fill_ecs_object(child, value)
      set(obj, name, child)
  static_elif typeinfo(is_table value)
    using() <| $(var child : Object)
      fill_ecs_object(child, value)
      set(obj, name, child)
  else
    set(obj, name, value)


def fill_ecs_object(var obj : Object; value)
  static_if typeinfo(is_struct value)
    apply(value) <| $(name : string; field)
      add_to_ecs_object(obj, name, field)
  static_elif typeinfo(is_table value)
    for k, v in keys(value), values(value)
      add_to_ecs_object(obj, string(k), v)
  else
    concept_assert(false, "EcsObject support only structs and tables")
