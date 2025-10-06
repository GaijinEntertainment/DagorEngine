// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <regExp/regExp.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <generic/dag_tab.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_localization.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#include <debug/dag_logSys.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>

static bool trailing_semicolon = true, write_utf8_mark = false;
static bool allow_dup_keys = false, dup_key_overwrite = true;
static int dup_key_err_count = 0;

static bool on_duplicate_loc_key(const char *key)
{
  printf("duplicate key <%s>, %s, %s\n", key, allow_dup_keys ? "allowed" : "forbidden", dup_key_overwrite ? "overwrite" : "skip");
  if (!allow_dup_keys)
    dup_key_err_count++;
  return dup_key_overwrite;
}
static bool is_bad_key_due_to_conflict(const char *name)
{
  return strncmp(name, "<<<<<<<", 7) == 0 || strncmp(name, "=======", 7) == 0 || strncmp(name, ">>>>>>>", 7) == 0;
}

static void scan_files(const char *root_path, const char *src_folder, dag::ConstSpan<SimpleString> wclist,
  dag::ConstSpan<RegExp *> reExclude, bool scan_files, bool scan_subfolders, FastNameMapEx &files);

static void addNames(FastNameMapEx &dest, const FastNameMapEx &src_add)
{
  for (int i = 0; i < src_add.nameCount(); i++)
    dest.addNameId(src_add.getName(i));
}

struct CsvFilesGroup
{
  FastNameMapEx files;
  SimpleString name;

  bool buildList(const DataBlock &blk, dag::ConstSpan<CsvFilesGroup> groups, const char *root)
  {
    Tab<SimpleString> wclist(tmpmem);
    Tab<RegExp *> reExclude(tmpmem);
    int exclude_nid = blk.getNameId("exclude");
    int wildcard_nid = blk.getNameId("wildcard");
    int find_nid = blk.getNameId("find");
    int csv_nid = blk.getNameId("csv");
    int incl_group_nid = blk.getNameId("group");

    String tmpName;

    for (int i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == find_nid)
      {
        const DataBlock &b = *blk.getBlock(i);
        const char *src_folder = b.getStr("path", ".");

        wclist.clear();
        clear_all_ptr_items(reExclude);
        for (int j = 0; j < b.paramCount(); j++)
          if (b.getParamType(j) == DataBlock::TYPE_STRING)
          {
            if (b.getParamNameId(j) == wildcard_nid)
              wclist.push_back() = b.getStr(j);
            else if (b.getParamNameId(j) == exclude_nid)
            {
              const char *pattern = b.getStr(j);
              RegExp *re = new RegExp;
              if (re->compile(pattern, "i"))
                reExclude.push_back(re);
              else
              {
                printf("ERR: bad regexp /%s/\n", pattern);
                delete re;
                return false;
              }
            }
          }

        if (!wclist.size())
          wclist.push_back() = "*.csv";
        if (!wclist.size())
        {
          printf("WARN: empty wildcard list in block #%d", i + 1);
          continue;
        }

        scan_files(root, src_folder, wclist, reExclude, b.getBool("scan_files", true), b.getBool("scan_subfolders", false), files);
      }

    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamNameId(i) == csv_nid && blk.getParamType(i) == blk.TYPE_STRING)
      {
        tmpName.printf(260, "%s/%s", root, blk.getStr(i));
        dd_simplify_fname_c(tmpName);
        files.addNameId(tmpName);
      }

    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamNameId(i) == incl_group_nid && blk.getParamType(i) == blk.TYPE_STRING)
      {
        const char *ign = blk.getStr(i);
        const CsvFilesGroup *g = NULL;

        for (int j = 0; j < groups.size(); j++)
          if (stricmp(groups[j].name, ign) == 0)
          {
            g = &groups[j];
            break;
          }

        if (g)
          addNames(files, g->files);
        else
        {
          printf("ERR: cannot find group <%s>\n", ign);
          return false;
        }
      }

    name = blk.getStr("name", NULL);
    return true;
  }
  void makeBlk(DataBlock &loc_blk) const
  {
    loc_blk.reset();
    DataBlock *lb = loc_blk.addNewBlock("locTable");
    for (int j = 0; j < files.nameCount(); j++)
      lb->addStr("file", files.getName(j));
    loc_blk.setStr("forceDefLang", "English");
  }
  void dumpFiles() const
  {
    printf("Group <%s>, %d file(s):\n", name.str(), files.nameCount());
    for (int i = 0; i < files.nameCount(); i++)
      printf(" %2d: %s\n", i, files.getName(i));
  }
};

struct CsvLangPack
{
  FastNameMapEx langs;
  SimpleString name;

  bool buildList(const DataBlock &blk, dag::ConstSpan<CsvLangPack> langp)
  {
    int lang_nid = blk.getNameId("lang");
    int langpack_nid = blk.getNameId("langpack");

    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamNameId(i) == lang_nid && blk.getParamType(i) == blk.TYPE_STRING)
        langs.addNameId(blk.getStr(i));

    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamNameId(i) == langpack_nid && blk.getParamType(i) == blk.TYPE_STRING)
      {
        const char *ign = blk.getStr(i);
        const CsvLangPack *g = NULL;

        for (int j = 0; j < langp.size(); j++)
          if (stricmp(langp[j].name, ign) == 0)
          {
            g = &langp[j];
            break;
          }

        if (g)
          addNames(langs, g->langs);
        else
        {
          printf("ERR: cannot find group <%s>\n", ign);
          return false;
        }
      }

    name = blk.getStr("name", NULL);
    return true;
  }

  void dumpLangs()
  {
    printf("LangPack <%s>, %d language(s):\n", name.str(), langs.nameCount());
    for (int i = 0; i < langs.nameCount(); i++)
      printf(" %2d: %s\n", i, langs.getName(i));
  }
};

struct CsvKeyPack
{
  FastNameMapEx inclKeys;
  FastNameMapEx exclKeys;
  SimpleString name;

  bool buildList(const DataBlock &blk, dag::ConstSpan<CsvKeyPack> keyp)
  {
    int includekey_nid = blk.getNameId("includeKey");
    int key_nid = blk.getNameId("key");
    int excludekey_nid = blk.getNameId("excludeKey");
    int keypack_nid = blk.getNameId("keypack");

    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamNameId(i) == includekey_nid && blk.getParamType(i) == blk.TYPE_STRING)
        inclKeys.addNameId(blk.getStr(i));

    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamNameId(i) == key_nid && blk.getParamType(i) == blk.TYPE_STRING)
        inclKeys.addNameId(String(64, "^%s$", blk.getStr(i)));

    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamNameId(i) == excludekey_nid && blk.getParamType(i) == blk.TYPE_STRING)
        exclKeys.addNameId(blk.getStr(i));

    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamNameId(i) == keypack_nid && blk.getParamType(i) == blk.TYPE_STRING)
      {
        const char *ign = blk.getStr(i);
        const CsvKeyPack *g = NULL;

        for (int j = 0; j < keyp.size(); j++)
          if (stricmp(keyp[j].name, ign) == 0)
          {
            g = &keyp[j];
            break;
          }

        if (g)
        {
          addNames(inclKeys, g->inclKeys);
          addNames(exclKeys, g->exclKeys);
        }
        else
        {
          printf("ERR: cannot find group <%s>\n", ign);
          return false;
        }
      }

    name = blk.getStr("name", NULL);
    return true;
  }
  void dumpKeys()
  {
    printf("KeyPack <%s>, %d include key pattern(s), %d exclude key pattern(s):\n", name.str(), inclKeys.nameCount(),
      exclKeys.nameCount());
    for (int i = 0; i < inclKeys.nameCount(); i++)
      printf(" %2d: include /%s/\n", i, inclKeys.getName(i));
    for (int i = 0; i < exclKeys.nameCount(); i++)
      printf(" %2d: exclude /%s/\n", inclKeys.nameCount() + i, exclKeys.getName(i));
  }
};

struct CsvKeyChecker
{
  Tab<RegExp *> incl;
  Tab<RegExp *> excl;

  CsvKeyChecker(const CsvKeyPack &p) : incl(tmpmem), excl(tmpmem)
  {
    incl.reserve(p.inclKeys.nameCount());
    excl.reserve(p.exclKeys.nameCount());
    for (int i = 0; i < p.inclKeys.nameCount(); i++)
    {
      RegExp *re = new RegExp;
      if (re->compile(p.inclKeys.getName(i), "i"))
        incl.push_back(re);
      else
      {
        printf("ERR: bad regexp include /%s/\n", p.inclKeys.getName(i));
        delete re;
      }
    }
    for (int i = 0; i < p.exclKeys.nameCount(); i++)
    {
      RegExp *re = new RegExp;
      if (re->compile(p.exclKeys.getName(i), "i"))
        excl.push_back(re);
      else
      {
        printf("ERR: bad regexp exclude /%s/\n", p.exclKeys.getName(i));
        delete re;
      }
    }
  }
  ~CsvKeyChecker()
  {
    clear_all_ptr_items(incl);
    clear_all_ptr_items(excl);
  }

  bool keyHolds(const char *k) const
  {
    if (incl.size())
    {
      for (int i = 0; i < incl.size(); i++)
        if (incl[i]->test(k))
          goto check_excl;
      return false;
    }

  check_excl:
    if (excl.size())
    {
      for (int i = 0; i < excl.size(); i++)
        if (excl[i]->test(k))
          return false;
    }
    return true;
  }
};

struct CsvRec
{
  Tab<Tab<String> *> csv;
  FastNameMapEx keys;
  SimpleString fname;

public:
  CsvRec() : csv(tmpmem) {}
  ~CsvRec() { clear_all_ptr_items(csv); }

  bool load(const DataBlock &loc_blk, const CsvLangPack &lang, const char *task_name)
  {
    Tab<const char *> loc_keys(tmpmem), loc_vals(tmpmem);
    keys.reset();
    clear_all_ptr_items(csv);
    for (int j = 0; j < lang.langs.nameCount(); j++)
    {
      shutdown_localization();
      dup_key_err_count = 0;
      if (!::startup_localization_V2(loc_blk, lang.langs.getName(j)) || dup_key_err_count)
      {
        printf("ERR: cannot load localization for lang=<%s>, task=<%s>\n", lang.langs.getName(j), task_name);
        return false;
      }

      ::get_all_localization(loc_keys, loc_vals);
      bool bad_keys = false;
      for (int k = 0; k < loc_keys.size(); k++)
        if (strchr(loc_keys[k], '\"'))
        {
          printf("ERR: bad key=<%s> with <\">, task=<%s>\n", loc_keys[k], task_name);
          bad_keys = true;
        }
        else if (is_bad_key_due_to_conflict(loc_keys[k]))
        {
          printf("ERR: bad key=\"%s\" (merge conflict?), task=<%s>\n", loc_keys[k], task_name);
          bad_keys = true;
        }
      if (bad_keys)
        return false;
      Tab<String> *v = new Tab<String>(tmpmem);

      if (keys.nameCount() == 0)
        for (int k = 0; k < loc_keys.size(); k++)
          if (keys.addNameId(loc_keys[k]) != k)
          {
            printf("ERR: duplicate key=<%s>, task=<%s>\n", loc_keys[k], task_name);
            return false;
          }

      if (keys.nameCount() == loc_vals.size())
      {
        v->resize(loc_vals.size());
        for (int k = 0; k < loc_vals.size(); k++)
          (*v)[k] = loc_vals[k];
      }
      csv.push_back(v);
    }
    return true;
  }
};

static dag::Vector<CsvFilesGroup> groups;
static dag::Vector<CsvLangPack> langpacks;
static dag::Vector<CsvKeyPack> keypacks;
static const char *rootPath = NULL;

static void scan_files(const char *root_path, const char *src_folder, dag::ConstSpan<SimpleString> wclist,
  dag::ConstSpan<RegExp *> reExclude, bool scan_files, bool scan_subfolders, FastNameMapEx &files)
{
  String tmpPath;

  // scan for files
  if (scan_files)
    for (int i = 0; i < wclist.size(); i++)
    {
      tmpPath.printf(260, "%s/%s/%s", root_path, src_folder, wclist[i].str());
      for (const alefind_t &ff : dd_find_iterator(tmpPath, DA_FILE))
      {
        bool excl = false;
        for (int j = 0; j < reExclude.size(); j++)
          if (reExclude[j]->test(ff.name))
          {
            excl = true;
            break;
          }

        if (!excl)
        {
          tmpPath.printf(260, "%s/%s/%s", root_path, src_folder, ff.name);
          dd_simplify_fname_c(tmpPath);
          files.addNameId(tmpPath);
        }
      }
    }

  // scan for sub-folders
  if (scan_subfolders)
  {
    tmpPath.printf(260, "%s/%s/*", root_path, src_folder);
    for (const alefind_t &ff : dd_find_iterator(tmpPath, DA_SUBDIR))
    {
      if (ff.attr & DA_SUBDIR)
      {
        if (dd_stricmp(ff.name, "cvs") == 0 || dd_stricmp(ff.name, ".svn") == 0)
          continue;

        ::scan_files(root_path, String(260, "%s/%s", src_folder, ff.name).str(), wclist, reExclude, scan_files, scan_subfolders,
          files);
      }
    }
  }
}

static const char *findSym(const char *str, const char *chars)
{
  while (*str)
    if (strchr(chars, *str))
      return str;
    else
      str++;
  return NULL;
}
static const char *convertCsvStr(const char *csv, String &buf)
{
  const char *p = findSym(csv, "\t\"");
  if (!p)
    return csv;

  buf = "";
  while (p)
  {
    if (p - csv)
      buf.append(csv, p - csv);
    switch (*p)
    {
      case '\t': buf += "\\t"; break;
      case '\"': buf += "\"\""; break;
      default: G_ASSERT(0 && "internall error");
    }
    csv = p + 1;
    p = findSym(csv, "\t\"");
  }
  buf += csv;
  return buf;
}
static bool check_digits_only(const char *str)
{
  while (*str)
  {
    if (!isdigit(*str))
      return false;
    str++;
  }
  return true;
}

static FILE *start_csv_file(const char *fn, const FastNameMapEx &lang, String &tmpStr)
{
  dd_mkpath(fn);
  FILE *fp = fopen(fn, "wb");
  G_ASSERT(fp);
  if (write_utf8_mark)
    fprintf(fp, "\xEF\xBB\xBF");
  fprintf(fp, "\"<ID|readonly|noverify>\"");
  for (int l = 0; l < lang.nameCount(); l++)
    fprintf(fp, ";\"<%s>\"", convertCsvStr(lang.getName(l), tmpStr));
  fprintf(fp, "%s\r\n", trailing_semicolon ? "; " : "");
  return fp;
}

static bool should_detect_format_errors = false;
static bool format_err_detected = false, format_err_detected_first = true;
static const char *out_fn_for_check_fmt_str = NULL;
static void get_fmt_digest(const char *str, Tab<char> &digest)
{
  digest.clear();
  if (!str || !*str)
    return;
  const char *p = strchr(str, '%');
  while (p && p[1])
  {
    if (strchr("%!,? ", p[1]))
      p++;
    else
      for (int i = 1; p[i] && i < 31; i++)
        if (strchr("diuoxXfFeEgGaAcsp", p[i]))
        {
          append_items(digest, i + 1, p);
          p += i;
          break;
        }
        else if (strchr(" \n\r", p[i]) || !(isdigit(p[i]) || strchr(".*+-", p[i])))
        {
          p += i;
          break;
        }
    p = strchr(p + 1, '%');
  }
}

static void write_csv_line(FILE *fp, const char *key, dag::ConstSpan<Tab<String> *> csv, int id, String &tmpStr, int check_fmt_id = -1,
  const char *lang = NULL)
{
  G_ASSERT(fp);
  if (check_digits_only(key))
    fprintf(fp, "%s", key);
  else
    fprintf(fp, "\"%s\"", key);

  if (check_fmt_id >= 0)
  {
    int l = check_fmt_id;
    Tab<char> fmt1, fmt2;
    get_fmt_digest(csv.size() && csv[0] ? (*csv[0])[id].str() : "", fmt1);
    get_fmt_digest(csv[l] ? (*csv[l])[id].str() : "", fmt2);

    bool eq = fmt1.size() == fmt2.size() && mem_eq(fmt1, fmt2.data());
    if (!fmt2.size()) // treat format-less strings as compatible
      eq = true;
    if (!eq)
    {
      format_err_detected = true;
      if (out_fn_for_check_fmt_str)
      {
        printf("\nFMT error(s) in %s, %s:\n", lang, out_fn_for_check_fmt_str);
        out_fn_for_check_fmt_str = NULL;
      }

      printf("  BAD %s ('%.*s'!='%.*s'):\n    -> '%s'\n    -> '%s'\n", key, (int)fmt1.size(), fmt1.data(), (int)fmt2.size(),
        fmt2.data(), csv.size() && csv[0] ? (*csv[0])[id].str() : "", csv[l] ? (*csv[l])[id].str() : "");
    }
  }

  for (int l = 0; l < csv.size(); l++)
    if (!csv[l])
      fprintf(fp, ";");
    else if (check_digits_only((*csv[l])[id]))
      fprintf(fp, ";%s", (*csv[l])[id].str());
    else
      fprintf(fp, ";\"%s\"", convertCsvStr((*csv[l])[id], tmpStr));
  fprintf(fp, "%s\r\n", trailing_semicolon ? "; " : "");
}

static bool doDiff(const DataBlock &blk, const CsvFilesGroup &fg, const CsvLangPack &lang)
{
  const DataBlock &diff_blk = *blk.getBlockByNameEx("diff");
  const char *dest = diff_blk.getStr("dest", ".");

  CsvFilesGroup fg2;
  CsvLangPack diff_lang;
  CsvRec src, src2;
  DataBlock loc_blk;
  String tmpStr;
  const char *task_name = blk.getStr("name", "?");
  Tab<FILE *> fp_lang_diff(tmpmem);
  Tab<char> lang_mask(tmpmem);
  FILE *fp_add = NULL, *fp_del = NULL, *fp_mod = NULL;


  if (!lang.langs.nameCount())
  {
    printf("ERR: lang list is empty for task <%s>\n", task_name);
    return false;
  }
  if (!diff_lang.buildList(diff_blk, make_span(langpacks)))
  {
    printf("ERR: cannot build <diff> lang list for task <%s>\n", task_name);
    return false;
  }
  if (!diff_lang.langs.nameCount())
    diff_lang = lang;
  if (!diff_lang.langs.nameCount())
  {
    printf("ERR: <diff> lang list is empty for task <%s>\n", task_name);
    return false;
  }

  fp_lang_diff.resize(lang.langs.nameCount());
  mem_set_0(fp_lang_diff);
  lang_mask.resize(lang.langs.nameCount());
  mem_set_0(lang_mask);
  for (int l = 0; l < diff_lang.langs.nameCount(); l++)
  {
    int id = lang.langs.getNameId(diff_lang.langs.getName(l));
    if (id < 0)
    {
      printf("ERR: lang <%s> (specified in <diff>) not found in common lang list for task <%s>\n", diff_lang.langs.getName(l),
        task_name);
      return false;
    }
    lang_mask[id] = 0xFF;
  }

  if (!fg.files.nameCount())
    printf("WARN: <group> CSV list is empty for task <%s>\n", task_name);
  fg.makeBlk(loc_blk);
  if (!src.load(loc_blk, lang, task_name))
    return false;

  if (!fg2.buildList(diff_blk, make_span(groups), rootPath))
  {
    printf("ERR: cannot build <diff> CSV list for task <%s>\n", task_name);
    return false;
  }

  if (!fg2.files.nameCount())
    printf("WARN: <diffWith> CSV list is empty for task <%s>\n", task_name);
  fg2.makeBlk(loc_blk);
  if (!src2.load(loc_blk, lang, task_name))
    return false;
  dd_mkdir(String(260, "%s/%s", rootPath, dest));

  // do compare
  for (int id = 0; id < src.keys.nameCount(); id++)
  {
    const char *key = src.keys.getName(id);
    int id2 = src2.keys.getNameId(key);
    if (id2 < 0)
    {
      if (!fp_del)
        fp_del = start_csv_file(String(260, "%s/%s/removed_lines.csv", rootPath, dest), lang.langs, tmpStr);
      write_csv_line(fp_del, key, src.csv, id, tmpStr);
      continue;
    }

    bool changed = false;
    for (int l = 0; l < lang_mask.size(); l++)
      if (lang_mask[l] && ((!src.csv[l] || !src2.csv[l] || strcmp((*src.csv[l])[id], (*src2.csv[l])[id2]) != 0)))
      {
        changed = true;
        if (!fp_lang_diff[l])
        {
          fp_lang_diff[l] = fopen(String(260, "%s/%s/%s_lines.csv", rootPath, dest, lang.langs.getName(l)), "wb");
          if (write_utf8_mark)
            fprintf(fp_lang_diff[l], "\xEF\xBB\xBF");
          fprintf(fp_lang_diff[l], "\"<ID|readonly|noverify>\";\"<original>\";\"<changed>\"%s\r\n", trailing_semicolon ? "; " : "");
        }

        fprintf(fp_lang_diff[l], "\"%s\";", key);
        fprintf(fp_lang_diff[l], "\"%s\";", src.csv[l] ? convertCsvStr((*src.csv[l])[id], tmpStr) : "");
        fprintf(fp_lang_diff[l], "\"%s\"%s\r\n", src2.csv[l] ? convertCsvStr((*src2.csv[l])[id2], tmpStr) : "",
          trailing_semicolon ? "; " : "");

        (*src.csv[l])[id] = (*src2.csv[l])[id2];
      }
    if (changed)
    {
      if (!fp_mod)
        fp_mod = start_csv_file(String(260, "%s/%s/modified_lines.csv", rootPath, dest), lang.langs, tmpStr);
      write_csv_line(fp_mod, key, src.csv, id, tmpStr);
    }
  }
  for (int id2 = 0; id2 < src2.keys.nameCount(); id2++)
  {
    const char *key = src2.keys.getName(id2);
    int id = src.keys.getNameId(key);
    if (id < 0)
    {
      if (!fp_add)
        fp_add = start_csv_file(String(260, "%s/%s/added_lines.csv", rootPath, dest), lang.langs, tmpStr);
      write_csv_line(fp_add, key, src2.csv, id2, tmpStr);
    }
  }

  if (fp_add)
    fclose(fp_add);
  if (fp_del)
    fclose(fp_del);
  if (fp_mod)
    fclose(fp_mod);
  for (int i = 0; i < fp_lang_diff.size(); i++)
    if (fp_lang_diff[i])
      fclose(fp_lang_diff[i]);
  return true;
}
static bool doUpdate(const DataBlock &blk, const CsvFilesGroup &fg, const CsvLangPack &lang, const CsvKeyChecker &kc)
{
  const DataBlock &upd_blk = *blk.getBlockByNameEx("update");
  const char *dest = upd_blk.getStr("dest", ".");
  bool addKeyToMissedLoc = upd_blk.getBool("addKeyToMissedLoc", false);

  CsvFilesGroup fg2;
  CsvLangPack diff_lang;
  CsvRec src, src2;
  FastNameMap existing_keys;
  DataBlock loc_blk;
  String tmpStr;
  const char *task_name = blk.getStr("name", "?");
  Tab<char> lang_mask(tmpmem);

  if (!lang.langs.nameCount())
  {
    printf("ERR: lang list is empty for task <%s>\n", task_name);
    return false;
  }
  if (!diff_lang.buildList(upd_blk, make_span(langpacks)))
  {
    printf("ERR: cannot build <diff> lang list for task <%s>\n", task_name);
    return false;
  }
  if (!diff_lang.langs.nameCount())
    diff_lang = lang;
  if (!diff_lang.langs.nameCount())
  {
    printf("ERR: <diff> lang list is empty for task <%s>\n", task_name);
    return false;
  }

  lang_mask.resize(lang.langs.nameCount());
  mem_set_0(lang_mask);
  for (int l = 0; l < diff_lang.langs.nameCount(); l++)
  {
    int id = lang.langs.getNameId(diff_lang.langs.getName(l));
    if (id < 0)
    {
      printf("ERR: lang <%s> (specified in <diff>) not found in common lang list for task <%s>\n", diff_lang.langs.getName(l),
        task_name);
      return false;
    }
    lang_mask[id] = 0xFF;
  }

  if (!fg.files.nameCount())
    printf("WARN: <group> CSV list is empty for task <%s>\n", task_name);

  if (!fg2.buildList(upd_blk, make_span(groups), rootPath))
  {
    printf("ERR: cannot build <diff> CSV list for task <%s>\n", task_name);
    return false;
  }

  if (!fg2.files.nameCount())
    printf("WARN: <diffWith> CSV list is empty for task <%s>\n", task_name);
  fg2.makeBlk(loc_blk);
  if (!src2.load(loc_blk, lang, task_name))
    return false;

  // do update
  for (int f = 0; f < fg.files.nameCount(); f++)
  {
    loc_blk.reset();
    loc_blk.addNewBlock("locTable")->addStr("file", fg.files.getName(f));
    loc_blk.setStr("forceDefLang", "English");
    if (!src.load(loc_blk, lang, task_name))
      return false;

    String out_fn(260, "%s/%s/%s", rootPath, dest, fg.files.getName(f));
    FILE *fp = start_csv_file(out_fn, lang.langs, tmpStr);
    out_fn_for_check_fmt_str = out_fn;
    for (int id = 0; id < src.keys.nameCount(); id++)
    {
      const char *key = src.keys.getName(id);
      int id2 = src2.keys.getNameId(key);
      int cur_lang_idx = -1;
      existing_keys.addNameId(key);

      if (!kc.keyHolds(key))
        ;
      else if (id2 >= 0)
      {
        for (int l = 0; l < lang_mask.size(); l++)
          if (lang_mask[l] && ((!src.csv[l] || !src2.csv[l] || strcmp((*src.csv[l])[id], (*src2.csv[l])[id2]) != 0)))
          {
            (*src.csv[l])[id] = (*src2.csv[l])[id2];
            cur_lang_idx = l;
          }
      }
      else if (addKeyToMissedLoc)
      {
        for (int l = 0; l < lang_mask.size(); l++)
          if (lang_mask[l] && src.csv[l])
          {
            const char *val = (*src.csv[l])[id];
            if (!val)
              val = "";
            (*src.csv[l])[id] = String(0, "[%s] %s", key, val);
          }
      }

      const char *langName = NULL;
      if (out_fn_for_check_fmt_str && cur_lang_idx >= 0 && cur_lang_idx < lang.langs.nameCount())
        langName = lang.langs.getName(cur_lang_idx);
      write_csv_line(fp, key, src.csv, id, tmpStr, should_detect_format_errors ? cur_lang_idx : -1, langName);
    }
    out_fn_for_check_fmt_str = NULL;
    fclose(fp);
  }

  FILE *fp = NULL;
  for (int id2 = 0; id2 < src2.keys.nameCount(); id2++)
  {
    const char *key = src2.keys.getName(id2);
    if (existing_keys.getNameId(key) < 0)
    {
      if (!fp)
      {
        const char *add_fn = upd_blk.getStr("addedCsv", NULL);
        if (!add_fn)
        {
          printf("ERR: update need to add new keys, but addedCsv is not set for task <%s>\n", task_name);
          return false;
        }
        fp = start_csv_file(String(260, "%s/%s/%s", rootPath, dest, add_fn), lang.langs, tmpStr);
      }
      write_csv_line(fp, key, src2.csv, id2, tmpStr);
    }
  }
  if (fp)
    fclose(fp);
  return !format_err_detected;
}

extern const char *process_japanese_string(const char *str, wchar_t sep_char = '\t');
extern const char *process_chinese_string(const char *str, wchar_t sep_char = '\t');

static void doProcess(const DataBlock &blk, const CsvLangPack &lang, CsvRec &rec, const CsvKeyChecker &kc)
{
  // TODO: maybe more processing options in the future
  //(in which case we should read settings from blk)
  // OR maybe, we'll need to process several columns at once
  const char *subType = blk.getStr("subtype", "jap_line_break");
  bool useU200B = blk.getBool("useU200B", false);
  wchar_t sepChar = useU200B ? L'\u200B' : L'\t';
  if (!strcmp(subType, "jap_line_break"))
  {
    const char *col = blk.getStr("lang", "Japanese");
    for (int l = 0; l < lang.langs.nameCount(); l++)
      if (!strcmp(lang.langs.getName(l), col))
      {
        Tab<String> *vals = rec.csv[l];
        for (int i = 0; i < (*vals).size(); i++)
        {
          if (kc.keyHolds(rec.keys.getName(i)))
            (*vals)[i] = process_japanese_string((*vals)[i], sepChar);
          else
            debug("skip <%s>", rec.keys.getName(i));
        }
      }
  }
  else if (!strcmp(subType, "chn_line_break"))
  {
    const char *col = blk.getStr("lang", "Chinese");
    for (int l = 0; l < lang.langs.nameCount(); l++)
      if (!strcmp(lang.langs.getName(l), col))
      {
        Tab<String> *vals = rec.csv[l];
        for (int i = 0; i < (*vals).size(); i++)
        {
          if (kc.keyHolds(rec.keys.getName(i)))
            (*vals)[i] = process_chinese_string((*vals)[i], sepChar);
          else
            debug("skip <%s>", rec.keys.getName(i));
        }
      }
  }
}

static bool doProcess(const DataBlock &blk, const CsvFilesGroup &fg, const CsvLangPack &lang, const CsvKeyChecker &kc)
{
  const DataBlock &proc_blk = *blk.getBlockByNameEx("process");
  const char *dest = proc_blk.getStr("dest", ".");

  CsvRec src;
  DataBlock loc_blk;
  String tmpStr;
  const char *task_name = blk.getStr("name", "?");

  if (!lang.langs.nameCount())
  {
    printf("ERR: lang list is empty for task <%s>\n", task_name);
    return false;
  }

  if (!fg.files.nameCount())
    printf("WARN: <group> CSV list is empty for task <%s>\n", task_name);

  // do process
  for (int f = 0; f < fg.files.nameCount(); f++)
  {
    loc_blk.reset();
    loc_blk.addNewBlock("locTable")->addStr("file", fg.files.getName(f));
    loc_blk.setStr("forceDefLang", "English");
    if (!src.load(loc_blk, lang, task_name))
      return false;

    doProcess(proc_blk, lang, src, kc);

    FILE *fp = start_csv_file(String(260, "%s/%s/%s", rootPath, dest, fg.files.getName(f)), lang.langs, tmpStr);
    for (int id = 0; id < src.keys.nameCount(); id++)
    {
      const char *key = src.keys.getName(id);
      write_csv_line(fp, key, src.csv, id, tmpStr);
    }
    fclose(fp);
  }
  return true;
}
static bool doExtract(const DataBlock &blk, const CsvFilesGroup &fg, const CsvLangPack &lang)
{
  const DataBlock &extr_blk = *blk.getBlockByNameEx("extract");
  const char *dest = extr_blk.getStr("dest", ".");

  CsvFilesGroup fg2;
  CsvRec src, src2;
  DataBlock loc_blk;
  String tmpStr;
  const char *task_name = blk.getStr("name", "?");
  FILE *fp = NULL;


  if (!lang.langs.nameCount())
  {
    printf("ERR: lang list is empty for task <%s>\n", task_name);
    return false;
  }

  if (!fg.files.nameCount())
    printf("WARN: <group> CSV list is empty for task <%s>\n", task_name);
  fg.makeBlk(loc_blk);
  if (!src.load(loc_blk, lang, task_name))
    return false;

  if (extr_blk.getBlockByName("find") || extr_blk.paramExists("csv") || extr_blk.paramExists("group"))
  {
    if (!fg2.buildList(extr_blk, make_span(groups), rootPath))
    {
      printf("ERR: cannot build <diff> CSV list for task <%s>\n", task_name);
      return false;
    }

    if (!fg2.files.nameCount())
      printf("WARN: <diffWith> CSV list is empty for task <%s>\n", task_name);
    fg2.makeBlk(loc_blk);
    if (!src2.load(loc_blk, lang, task_name))
      return false;
  }
  else
    src2.keys = src.keys;
  dd_mkpath(String(260, "%s/%s", rootPath, dest));

  // do extract
  bool result = true;
  fp = start_csv_file(String(260, "%s/%s", rootPath, dest), lang.langs, tmpStr);
  for (int id2 = 0; id2 < src2.keys.nameCount(); id2++)
  {
    const char *key = src2.keys.getName(id2);
    int id = src.keys.getNameId(key);
    if (id < 0)
    {
      printf("ERR: cannot build <diff> CSV list for task <%s>\n", task_name);
      result = false;
      continue;
    }

    write_csv_line(fp, key, src.csv, id, tmpStr);
  }
  fclose(fp);
  return result;
}

static bool doCombine(const DataBlock &blk, const CsvFilesGroup &fg, const CsvLangPack &lang)
{
  const DataBlock &comb_blk = *blk.getBlockByNameEx("combine");
  const char *dest = comb_blk.getStr("dest", ".");
  const char *task_name = blk.getStr("name", "?");
  Tab<int> fallback_lang;
  CsvRec src;
  bool reportMissingKeys = blk.getBool("reportMissingKeys", false);
  bool reportExtraKeys = blk.getBool("reportExtraKeys", false);

  if (!lang.langs.nameCount())
  {
    printf("ERR: lang list is empty for task <%s>\n", task_name);
    return false;
  }

  src.csv.reserve(lang.langs.nameCount());
  for (int l = 0; l < lang.langs.nameCount(); l++)
    src.csv.push_back(new Tab<String>);
  fallback_lang.resize(lang.langs.nameCount());
  mem_set_ff(fallback_lang);
  if (const char *def_loc_lang = comb_blk.getStr("defLocLang", nullptr))
  {
    int lang_id = lang.langs.getNameId(def_loc_lang);
    for (int l = 0; l < fallback_lang.size(); l++)
      fallback_lang[l] = lang_id;
  }

  for (int i = 0; i < comb_blk.blockCount(); i++)
  {
    CsvFilesGroup fg2;
    CsvLangPack lang2;
    CsvRec src2;
    DataBlock tmpBlk;
    bool addKeys = comb_blk.getBlock(i)->getBool("addKeys", i == 0);

    tmpBlk.setStr("langpack", comb_blk.getBlock(i)->getBlockName());
    if (!lang2.buildList(tmpBlk, make_span(langpacks)))
    {
      printf("ERR: cannot build <combine:%s> LANG for task <%s>\n", comb_blk.getBlock(i)->getBlockName(), task_name);
      return false;
    }
    if (lang2.langs.nameCount() < 1 || lang.langs.getNameId(lang2.langs.getName(0)) < 0)
    {
      printf("WARN: skip <combine:%s> LANG for task <%s>\n", comb_blk.getBlock(i)->getBlockName(), task_name);
      continue;
    }

    if (!fg2.buildList(*comb_blk.getBlock(i), make_span(groups), rootPath))
    {
      printf("ERR: cannot build <combine:%s> CSV list for task <%s>\n", comb_blk.getBlock(i)->getBlockName(), task_name);
      return false;
    }

    if (!fg2.files.nameCount())
      printf("WARN: <combine:%s> CSV list is empty for task <%s>\n", comb_blk.getBlock(i)->getBlockName(), task_name);
    fg2.makeBlk(tmpBlk);
    if (!src2.load(tmpBlk, lang2, task_name))
    {
      printf("ERR: failed to load <combine:%s> localizations for task <%s>\n", comb_blk.getBlock(i)->getBlockName(), task_name);
      return false;
    }

    int lang_id = lang.langs.getNameId(lang2.langs.getName(0));
    if (const char *def_loc_lang = comb_blk.getBlock(i)->getStr("defLocLang", nullptr))
      fallback_lang[lang_id] = lang.langs.getNameId(def_loc_lang);
    if (fallback_lang[lang_id] == lang_id)
      fallback_lang[lang_id] = -1;
    for (int id2 = 0; id2 < src2.keys.nameCount(); id2++)
    {
      int id = addKeys ? src.keys.addNameId(src2.keys.getName(id2)) : src.keys.getNameId(src2.keys.getName(id2));
      if (id < 0)
      {
        if (reportExtraKeys)
          fprintf(stderr, "WARN: [%s] extra key <%s>\n", lang.langs.getName(lang_id), src2.keys.getName(id2));
        continue;
      }
      if (id == src.csv[0]->size())
        for (int l = 0; l < lang.langs.nameCount(); l++)
          src.csv[l]->push_back() = "\1";
      if (!(*src2.csv[0])[id2].empty())
        (*src.csv[lang_id])[id] = (*src2.csv[0])[id2];
    }
  }

  // substitute missing localizations with default ones
  Tab<int> lang_order;
  for (int pass = 0; lang_order.size() < lang.langs.nameCount() && pass < lang.langs.nameCount(); pass++)
    for (int l = 0; l < fallback_lang.size(); l++)
      if (find_value_idx(lang_order, l) < 0 && (fallback_lang[l] == -1 || find_value_idx(lang_order, fallback_lang[l]) >= 0))
        lang_order.push_back(l);
  if (lang_order.size() < lang.langs.nameCount())
  {
    printf("ERR: circular deps on fallback lang?\n");
    for (int l = 0; l < fallback_lang.size(); l++)
      printf("  lang[%d]=%s  fallback=%d (%s)\n", l, lang.langs.getName(l), fallback_lang[l], lang.langs.getName(fallback_lang[l]));
    return false;
  }

  for (int i = 0; i < lang_order.size(); i++)
  {
    int l = lang_order[i];
    int subst_missing_loc_from = fallback_lang[l];
    for (int id = 0; id < src.keys.nameCount(); id++)
      if (strcmp((*src.csv[l])[id], "\1") == 0)
      {
        (*src.csv[l])[id] = subst_missing_loc_from != -1 ? (*src.csv[subst_missing_loc_from])[id].str() : "";
        if (reportMissingKeys)
          fprintf(stderr, "ERR: [%s] missing <%s>\n", lang.langs.getName(l), src.keys.getName(id));
      }
  }

  bool result = true;
  String tmpStr;
  dd_mkpath(String(260, "%s/%s", rootPath, dest));
  FILE *fp = start_csv_file(String(260, "%s/%s", rootPath, dest), lang.langs, tmpStr);
  for (int id = 0; id < src.keys.nameCount(); id++)
    write_csv_line(fp, src.keys.getName(id), src.csv, id, tmpStr);
  fclose(fp);
  return result;
}

static bool processTasks(const DataBlock &blk, dag::ConstSpan<char *> targets)
{
  bool result = true;
  int task_nid = blk.getNameId("task");
  int find_nid = blk.getNameId("find");
  Tab<const char *> loc_keys(tmpmem), loc_vals(tmpmem);
  rootPath = blk.getStr("rootFolder", ".");

  for (int i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == task_nid)
    {
      const DataBlock &b = *blk.getBlock(i);
      const char *name = b.getStr("name", NULL);

      if (targets.size())
      {
        bool found = false;
        if (!name)
          continue;

        for (int i = 0; i < targets.size(); i++)
          if (stricmp(targets[i], name) == 0)
          {
            found = true;
            break;
          }

        if (!found)
          continue;
      }


      CsvFilesGroup fg;
      CsvLangPack lang;
      CsvKeyPack keys;
      if (!fg.buildList(b, make_span(groups), rootPath))
      {
        result = false;
        continue;
      }
      if (!lang.buildList(b, make_span(langpacks)))
      {
        result = false;
        continue;
      }
      if (!keys.buildList(b, make_span(keypacks)))
      {
        result = false;
        continue;
      }

      /*
      printf("task <%s>:\n", name);
      fg.dumpFiles();
      lang.dumpLangs();
      keys.dumpKeys();
      */
      bool printList = b.getBool("printList", false);
      const char *writeCsv = b.getStr("writeCsv", NULL);
      const char *writeValDump = b.getStr("writeValDump", NULL);
      const char *taskType = NULL;
      for (int j = 0; j < b.blockCount(); j++)
        if (b.getBlock(j)->getBlockNameId() != find_nid)
        {
          taskType = b.getBlock(j)->getBlockName();
          break;
        }
      Tab<Tab<String> *> csv(tmpmem);
      Tab<String> csv_keys(tmpmem);

      if (printList)
        fg.dumpFiles();

      if (taskType)
      {
        if (stricmp(taskType, "diff") == 0)
          result &= doDiff(b, fg, lang);
        else if (stricmp(taskType, "extract") == 0)
          result &= doExtract(b, fg, lang);
        else if (stricmp(taskType, "combine") == 0)
          result &= doCombine(b, fg, lang);
        else if (stricmp(taskType, "update") == 0)
        {
          CsvKeyChecker kc(keys);
          result &= doUpdate(b, fg, lang, kc);
        }
        else if (stricmp(taskType, "process") == 0)
        {
          CsvKeyChecker kc(keys);
          result &= doProcess(b, fg, lang, kc);
        }
        else
        {
          printf("ERR: bad task type <%s> in task <%s>\n", taskType, name);
          continue;
        }
      }
      else if (writeCsv || writeValDump)
      {
        DataBlock loc_blk;
        fg.makeBlk(loc_blk);
        if (writeCsv)
          dd_mkpath(writeCsv);
        if (writeValDump)
          dd_mkpath(writeValDump);
        file_ptr_t fpCsv = writeCsv ? df_open(writeCsv, DF_WRITE) : NULL;
        file_ptr_t fpDump = writeValDump ? df_open(writeValDump, DF_WRITE) : NULL;

        if (!fpCsv && writeCsv)
          printf("ERR: cannot write CSV <%s>\n", writeCsv);
        if (!fpDump && writeValDump)
          printf("ERR: cannot write values dump <%s>\n", writeValDump);
        CsvKeyChecker kc(keys);

        for (int j = 0; j < lang.langs.nameCount(); j++)
        {
          shutdown_localization();
          dup_key_err_count = 0;
          if (!::startup_localization_V2(loc_blk, lang.langs.getName(j)) || dup_key_err_count)
          {
            result = false;
            printf("ERR: cannot load localization for lang=<%s>, task=<%s>\n", lang.langs.getName(j), name);
          }

          ::get_all_localization(loc_keys, loc_vals);
          for (int k = loc_keys.size() - 1; k >= 0; k--)
            if (!kc.keyHolds(loc_keys[k]))
            {
              erase_items(loc_keys, k, 1);
              erase_items(loc_vals, k, 1);
            }
            else if (is_bad_key_due_to_conflict(loc_keys[k]))
            {
              result = false;
              printf("ERR: bad key=\"%s\" (merge conflict?), task=<%s>\n", loc_keys[k], name);
            }

          if (!loc_keys.size())
            printf("WARN: no keys found for task=<%s> lang=<%s>\n", name, lang.langs.getName(j));
          else
          {
            if (fpDump)
            {
              for (int k = 0; k < loc_vals.size(); k++)
              {
                df_write(fpDump, loc_vals[k], (int)strlen(loc_vals[k]));
                df_write(fpDump, "\r\n", 2);
              }
              df_write(fpDump, "\r\n", 2);
            }
          }

          if (fpCsv)
          {
            Tab<String> *v = new Tab<String>(tmpmem);
            if (csv_keys.size() == 0)
            {
              csv_keys.resize(loc_keys.size());
              for (int k = 0; k < loc_keys.size(); k++)
                csv_keys[k] = loc_keys[k];
            }

            if (csv_keys.size() == loc_vals.size())
            {
              v->resize(loc_vals.size());
              for (int k = 0; k < loc_vals.size(); k++)
                (*v)[k] = loc_vals[k];
            }
            csv.push_back(v);
          }
        }

        if (fpCsv)
        {
          String tmpStr;
          df_printf(fpCsv, "\"<ID|readonly|noverify>\";");
          for (int j = 0; j < lang.langs.nameCount(); j++)
            df_printf(fpCsv, "\"%s\";", lang.langs.getName(j));
          df_printf(fpCsv, "\"comment\"\r\n");

          for (int j = 0; j < csv_keys.size(); j++)
          {
            if (csv_keys[j].empty())
              continue;
            df_printf(fpCsv, "\"%s\";", csv_keys[j].str());
            for (int k = 0; k < csv.size(); k++)
              df_printf(fpCsv, "\"%s\";", convertCsvStr(csv[k]->at(j).str(), tmpStr));
            df_printf(fpCsv, " \r\n");
          }
          df_close(fpCsv);
        }
        if (fpDump)
          df_close(fpDump);

        clear_all_ptr_items(csv);
      }
    }
  rootPath = NULL;
  return result;
}

static void gatherGroups(const DataBlock &blk)
{
  int group_nid = blk.getNameId("group");
  const char *root_path = blk.getStr("rootFolder", ".");

  for (int i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == group_nid)
    {
      CsvFilesGroup &g = groups.push_back();
      if (!g.buildList(*blk.getBlock(i), make_span(groups), root_path))
        groups.pop_back();
    }
}

static void gatherLangPacks(const DataBlock &blk)
{
  int langpack_nid = blk.getNameId("langpack");

  for (int i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == langpack_nid)
    {
      CsvLangPack &g = langpacks.push_back();
      if (!g.buildList(*blk.getBlock(i), make_span(langpacks)))
        langpacks.pop_back();
    }
}

static void gatherKeyPacks(const DataBlock &blk)
{
  int keypack_nid = blk.getNameId("keypack");

  for (int i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == keypack_nid)
    {
      CsvKeyPack &g = keypacks.push_back();
      if (!g.buildList(*blk.getBlock(i), make_span(keypacks)))
        keypacks.pop_back();
    }
}

static void __cdecl ctrl_break_handler(int) { quit_game(0); }

static void print_title() { printf("CSV utility v2.06\nCopyright (C) Gaijin Games KFT, 2023\n"); }
static void print_usage(bool verbose)
{
  const char *exe_name = dd_get_fname(dgs_argv[0]);
  printf("usage:  %s [-q] <task.blk> [task1 task2...]\n"
         "        %s [-q] <input.txt> -word_breaks:{jp|zh} <output.txt> [-use:U+200B]\n"
         "        %s [-q] <task.blk> -task [-check_fmt] <task-BLK-written-in-one-line>\n"
         "        %s -help   (for BLK and cmdline example)\n\n",
    exe_name, exe_name, exe_name, exe_name);
  if (!verbose)
    return;
  printf("task.blk format:\n"
         "  rootFolder:t=\"d:/dagor2/deMon/develop\"\n"
         "\n"
         "  group {\n"
         "    name:t=\"common\"\n"
         "    find {\n"
         "      path:t=\"./gameBase/lang\"\n"
         "\n"
         "      wildcard:t=\"*.csv\"\n"
         "      exclude:t=\"^.*_PC\\.csv$\"    // regexp to reject found files\n"
         "      //exclude:t=\"^.*_PS3\\.csv$\" // regexp to reject found files\n"
         "    }\n"
         "  }\n"
         "  group {\n"
         "    name:t=\"pc\"\n"
         "    csv:t=\"add01_PC.csv\"\n"
         "  }\n"
         "  group {\n"
         "    name:t=\"pc-full\"\n"
         "    group:t=\"common\"\n"
         "    group:t=\"pc\"\n"
         "  }\n"
         "  langpack {\n"
         "    name:t=\"euro\"\n"
         "    lang:t=\"English\"\n"
         "    lang:t=\"Russian\"\n"
         "    lang:t=\"German\"\n"
         "  }\n"
         "  keypack {\n"
         "    name:t=\"souls\"\n"
         "    includeKey:t=\"combo\\/mult_format\" // regexp to filter CSV keys\n"
         "    //key:t=\"combo\" // exact CSV key name\n"
         "    //excludeKey:t=\"combo.*\" // regexp to reject CSV keys\n"
         "  }\n"
         "  task {\n"
         "    group:t=\"pc-full\"\n"
         "    langpack:t=\"euro\"\n"
         "\n"
         "    //printList:b=yes\n"
         "    writeCsv:t=\"pc-euro.csv\"\n"
         "    writeValDump:t=\"pc-euro.utf8.txt\"\n"
         "  }\n"
         "  task {\n"
         "    name:t=\"make_fr\"; langpack:t=\"rel\"; find {}\n"
         "    update {\n"
         "      find { path:t=\"../_fr_src\"; }\n"
         "      lang:t=\"French\";\n"
         "      dest:t=\"../lang_compiled\"; addedCsv:t=\"../added_fr_common.csv\"\n"
         "    }\n"
         "  }\n"
         "  task {\n"
         "    name:t=\"make_fr\"; find {}\n"
         "    langpack:t=\"_fr\";\n"
         "    extract {\n"
         "      dest:t=\"./split/_fr_src.csv\";\n"
         "      //lang:t=\"French\";\n"
         "      //group:t=\"common\";\n"
         "    }\n"
         "  }\n"
         "  task {\n"
         "    name:t=\"make_fr\"; langpack:t=\"rel\";\n"
         "    reportMissingKeys:b=no\n"
         "    reportExtraKeys:b=no\n"
         "    combine {\n"
         "      dest:t=\"./combined/final.csv\";\n"
         "      //defLocLang:t=\"English\";\n"
         "      _en { addKeys:b=yes  find { path:t=\"./split\"; wildcard:t=\"_en_src.csv\" } } // name of block is langpack name\n"
         "      _ru { find { path:t=\"./split\"; wildcard:t=\"_ru_src.csv\" } }\n"
         "      _zh { find { path:t=\"./split\"; wildcard:t=\"_zh_src.csv\" } defLocLang:t=Russian }\n"
         "    }\n"
         "  }\n"
         "\nor\n"
         "  %s task.blk -task langpack:t=rel find {} "
         "update { find { path:t=../_fr_src } lang:t=French dest:t=../lang_compiled addedCsv:t=../added_fr_common.csv }\n",
    exe_name);
}

static int err_to_stderr(int lev_tag, const char *fmt, const void *arg, int anum, const char *ctx_file, int ctx_line)
{
  if (lev_tag != LOGLEVEL_ERR)
    return 1;
  if (ctx_file)
    fprintf(stderr, "\n[E] %s,%d: ", ctx_file, ctx_line);
  else
    fprintf(stderr, "\n[E] ");
  DagorSafeArg::mixed_fprint_fmt(stderr, fmt, arg, anum);
  fprintf(stderr, "\n");
  return 1;
}

int DagorWinMain(bool debugmode)
{
  if (dgs_argc < 2)
  {
    print_title();
    print_usage(false);
    return -1;
  }
  if (strcmp(dgs_argv[1], "-quiet") == 0 || strcmp(dgs_argv[1], "-q") == 0)
  {
    ::dgs_execute_quiet = true;
    memmove(&dgs_argv[1], &dgs_argv[2], (dgs_argc - 2) * sizeof(*dgs_argv));
    dgs_argc--;
  }
  if (!::dgs_execute_quiet)
    print_title();
  if (strcmp(dgs_argv[1], "-help") == 0 || strcmp(dgs_argv[1], "-h") == 0 || strcmp(dgs_argv[1], "/?") == 0)
  {
    print_usage(true);
    return -1;
  }
  signal(SIGINT, ctrl_break_handler);
  debug_set_log_callback(&err_to_stderr);

  if (strnicmp(dgs_argv[2], "-word_breaks:", 13) == 0)
  {
    if (stricmp(dgs_argv[2] + 13, "zh") != 0 && stricmp(dgs_argv[2] + 13, "jp") != 0)
    {
      printf("bad target lang %s for -word_breaks: option\n", dgs_argv[2] + 13);
      print_usage(false);
      return -1;
    }
    bool jap = stricmp(dgs_argv[2] + 13, "jp") == 0;
    if (dgs_argc < 4)
    {
      print_usage(false);
      return -1;
    }
    wchar_t sep_char = '\t';
    for (int i = 4; i < dgs_argc; i++)
      if (stricmp(dgs_argv[i], "-use:U+200B") == 0)
        sep_char = L'\u200B';
      else
      {
        printf("bad option %s\n", dgs_argv[i]);
        print_usage(false);
        return -1;
      }

    file_ptr_t fpIn = df_open(dgs_argv[1], DF_READ | DF_IGNORE_MISSING);
    if (!fpIn)
    {
      printf("cannot read input: %s\n", dgs_argv[1]);
      return -1;
    }

    Tab<char> s;
    s.resize(df_length(fpIn) + 1);
    df_read(fpIn, s.data(), s.size() - 1);
    s.back() = '\0';
    df_close(fpIn);

    file_ptr_t fpOut = df_open(dgs_argv[3], DF_WRITE);
    if (!fpOut)
    {
      printf("cannot write output: %s\n", dgs_argv[3]);
      return -1;
    }

    const char *s2 = jap ? process_japanese_string(s.data(), sep_char) : process_chinese_string(s.data(), sep_char);
    df_write(fpOut, s2, (int)strlen(s2));
    df_close(fpOut);
    printf("%s word breaks to %s text\n  %s -> %s\n%d bytes -> %d bytes\n", (s.size() - 1 != strlen(s2)) ? "added" : "no new",
      jap ? "japanese" : "chinese", dgs_argv[1], dgs_argv[3], int(s.size() - 1), int(strlen(s2)));
    return 0;
  }

  DataBlock inp;
  if (!inp.load(dgs_argv[1]))
  {
    printf("cannot read: %s\n", dgs_argv[1]);
    return -1;
  }
  trailing_semicolon = inp.getBool("trailing_semicolon", false);
  write_utf8_mark = inp.getBool("write_utf8_mark", false);
  allow_dup_keys = inp.getBool("allow_dup_keys", false);
  dup_key_overwrite = inp.getBool("dup_key_overwrite", true);
  dag_on_duplicate_loc_key = &on_duplicate_loc_key;

  gatherGroups(inp);
  gatherLangPacks(inp);
  gatherKeyPacks(inp);
  /*
  for (int i = 0; i < groups.size(); i ++)
    groups[i].dumpFiles();
  for (int i = 0; i < langpacks.size(); i ++)
    langpacks[i].dumpLangs();
  for (int i = 0; i < keypacks.size(); i ++)
    keypacks[i].dumpKeys();
  */
  if (stricmp(dgs_argv[2], "-task") == 0)
  {
    const char *tname = "__cmdLineTask";
    DataBlock cmdBlk;
    String cmdBlkText;

    for (int i = 3; i < dgs_argc; i++)
      cmdBlkText.aprintf(0, "%s\n", dgs_argv[i]);
    if (!dblk::load_text(cmdBlk, cmdBlkText))
    {
      printf("cannot parse: %s\n", cmdBlkText.str());
      return -1;
    }
    cmdBlk.setStr("name", tname);
    inp.addNewBlock("task")->setFrom(&cmdBlk);
    return processTasks(inp, dag::ConstSpan<char *>((char *const *)&tname, 1)) ? 0 : -1;
  }
  if (stricmp(dgs_argv[2], "-check_fmt") == 0)
  {
    should_detect_format_errors = true;
    if (!processTasks(inp, dag::ConstSpan<char *>(dgs_argv + 3, dgs_argc - 3)))
    {
      printf("processed with ERRORS!\n");
      return -1;
    }
  }
  if (!processTasks(inp, dag::ConstSpan<char *>(dgs_argv + 2, dgs_argc - 2)))
    return -1;
  return 0;
}
