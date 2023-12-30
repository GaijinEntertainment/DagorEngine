import re
import json
cmt = re.compile(r"(.*)\/\/.*")

def read_commented_json(fpath):
  with open(fpath, "rt", encoding="utf-8") as f:
    data = f.read()
    data = data.splitlines()
    res = []
    for line in data:
      c = cmt.search(line)
      if c:
        res.append(line.split("//")[0])
      else:
        res.append(line)
    return json.loads("\n".join(res))

def validated_qdox_cfg(config):
  all_cfg = []
  for e in config:
    paths = e.get("paths")
    doc_chapter = e.get("doc_chapter")
    recursive = e.get("recursive", False)
    exclude_dirs_re = e.get("exclude_dirs_re", [""])
    chapter_desc = e.get("chapter_desc", "")
    exclude_files_re = e.get("exclude_files_re", [""])
    chapter_title = e.get("chapter_title", doc_chapter.replace("_", " "))
    extensions = e.get("extensions", [""])
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
    if not isinstance(recursive, bool):
      log.warning("'recursive' in given config is not boolean, skipping it")
      recursive = True

    all_cfg.append({
      "paths":paths,
      "extensions":extensions,
      "doc_chapter":doc_chapter,
      "recursive": recursive,
      "exclude_dirs_re":exclude_dirs_re,
      "exclude_files_re":exclude_files_re,
      "chapter_desc":chapter_desc,
      "chapter_title":chapter_title
    })
  return all_cfg

