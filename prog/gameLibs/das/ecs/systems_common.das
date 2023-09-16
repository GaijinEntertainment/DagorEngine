module systems_common shared
require ecs
require EcsUtils
require strings


def get_first_system_names(count : int)
  var systemNames : array<string>
  find_systemDB() <| $ [unused_argument(_)] (name; _)
    systemNames |> push(clone_string(name))
    return length(systemNames) >= count
  return <- systemNames

def find_system_names(str : string)
  var templateNames : array<string>
  find_systemDB() <| $ [unused_argument(_)] (name; _)
    if find(name, str) != -1
      templateNames |> push(clone_string(name))
    return false
  return <- templateNames


[unused_argument(str)]
def find_system_names_sorted(str : string)
  var systemNames <- find_system_names(str)

  systemNames |> sort() <| $(left : string; right : string)
    let leftStartWith = starts_with(left, str)
    let rightStartsWith = starts_with(right, str)
    return leftStartWith != rightStartsWith ? leftStartWith : left < right
  return <- systemNames