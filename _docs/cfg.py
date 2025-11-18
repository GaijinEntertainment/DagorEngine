import re
JSON5_LOADED = False
try:
  import pyjson5 as json
  JSON5_LOADED = true
  print("Using Json5")
except:
  try:
    import thirdparty.pyjson5 as json
    JSON5_LOADED = true
    print("Using Json5")
  except:
    import json
    print("No Json5")

cmt = re.compile(r"(.*)\/\/.*")

def read_commented_json(fpath):
  with open(fpath, "rt", encoding="utf-8") as f:
    if JSON5_LOADED:
      return json.loads(f)
    s = f.read()
    # Remove JSON5 comments (// and /* */)
    s = re.sub(r'//.*?\n', '\n', s)  # Remove single-line comments
    s = re.sub(r'/\*.*?\*/', '', s, flags=re.DOTALL)  # Remove multi-line comments

    # Handle unquoted keys (e.g., {key: value} -> {"key": value})
    s = re.sub(r'([{,\s])(\w+)(:)', r'\1"\2"\3', s)

    # Remove trailing commas in objects and arrays
    s = re.sub(r',(\s*[}\]])', r'\1', s)

    # Replace single quotes with double quotes for strings
    s = re.sub(r"'([^']*)'", r'"\1"', s)

    # Validate as standard JSON
    return json.loads(s)

def validated_qdox_cfg(config):
  all_cfg = []
  for e in config:
    paths = e.get("paths")
    doc_chapter = e.get("doc_chapter")
    recursive = e.get("recursive", False)
    exclude_dirs_re = e.get("exclude_dirs_re", [""])
    chapter_desc = e.get("chapter_desc", "")
    chapter_desc_files = e.get("chapter_desc_files", [])
    exclude_files_re = e.get("exclude_files_re", [""])
    chapter_title = e.get("chapter_title", doc_chapter.replace("_", " "))
    extensions = e.get("extensions", [""])
    start_path = e.get("start_path", "")
    if not isinstance(doc_chapter, str):
      log.error("no 'doc_chapter' in given config or it is not string")
      log.error(config)
      continue
    if not isinstance(extensions, list):
      log.error("'extensions:[]' in given config is not list")
      extensions = [""]
    if not isinstance(paths, list):
      log.error("no 'paths:[]' in given config or it is not list")
      log.error(config)
      continue
    if not isinstance(exclude_dirs_re, list):
      log.warning("no 'exclude_dirs_re:[]' in given config is not list, skipping it")
      exclude_dirs_re = [""]
    if not isinstance(exclude_files_re, list):
      log.warning("no 'exclude_files_re:[]' in given config is not list, skipping it")
      exclude_files_re = [""]
    if not isinstance(chapter_title, str):
      log.warning("'chapter_title' in given config is not string, skipping it")
      chapter_title = doc_chapter.replace("_"," ")
    if not isinstance(chapter_desc, str):
      log.warning("'chapter_desc' in given config is not string, skipping it")
      chapter_desc = ""
    if not isinstance(chapter_desc_files, list):
      log.warning("'chapter_desc_files' in given config is not list, skipping it")
      chapter_desc_files = []
    if not isinstance(recursive, bool):
      log.warning("'recursive' in given config is not boolean, skipping it")
      recursive = True
    for i, desc_file in enumerate(chapter_desc_files):
      if not isinstance(desc_file, str):
        log.warning(f"'chapter_desc_file' {desc_file} is not string, skipping it")
        chapter_desc_files[i] = ""

    all_cfg.append({
      "start_path":start_path,
      "paths":paths,
      "extensions":extensions,
      "doc_chapter":doc_chapter,
      "recursive": recursive,
      "exclude_dirs_re":exclude_dirs_re,
      "exclude_files_re":exclude_files_re,
      "chapter_desc":chapter_desc,
      "chapter_desc_files":chapter_desc_files,
      "chapter_title":chapter_title
    })
  return all_cfg

