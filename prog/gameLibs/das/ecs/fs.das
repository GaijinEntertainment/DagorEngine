module fs shared
require DagorFindFiles
require DagorSystem
require ecs
require strings


def load_folder(path : string) : bool
  var ok = true
  var n = 0
  find_files_in_folder(path, "*.das") <| $(files)
    for file in files
      n += 1
      ok = load_das(file) && ok
  if n == 0
    logerr("folder `{path}` is empty, maybe wrong path")
  return ok


def load_optional_folder(path : string) : bool
  var ok = true
  find_files_in_folder(path, "*.das") <| $(files)
    for file in files
      ok = load_das(file) && ok
  return ok


def load_sub_folder(base_path, sub_folder : string) : bool
  var path = "{base_path}/{sub_folder}"
  let res = load_folder(path)
  unsafe(delete_string(path))
  return res