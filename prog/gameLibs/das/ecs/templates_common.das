module templates_common shared
require ecs
require EcsUtils
require strings


def get_first_template_names(count : int)
  var templateNames : array<string>
  getTemplateDB() |> find_templateDB() <| $ [unused_argument(tmpl)] (name; tmpl)
    templateNames |> push(clone_string(name))
    return length(templateNames) >= count
  return <- templateNames


def find_template_names(str : string)
  var templateNames : array<string>
  getTemplateDB() |> find_templateDB() <| $ [unused_argument(tmpl)] (name; tmpl)
    if find(name, str) != -1
      templateNames |> push(clone_string(name))
    return false
  return <- templateNames


def find_template_names_sorted(str : string)
  var templateNames <- find_template_names(str)

  templateNames |> sort() <| $(left : string; right : string)
    let leftStartWith = starts_with(left, str)
    let rightStartsWith = starts_with(right, str)
    return leftStartWith != rightStartsWith ? leftStartWith : left < right
  return <- templateNames