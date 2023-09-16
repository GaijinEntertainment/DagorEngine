#include <stdio.h>
#include <generic/dag_sort.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_localConv.h>
#include <generic/dag_tab.h>
#include <obsolete/dag_cfg.h>
#include <math/dag_color.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <util/dag_tabHlp.h>
#include <debug/dag_debug.h>
#include <stdlib.h>


static int var_cmp(CfgVar const *a, CfgVar const *b) { return dd_stricmp(a->id, b->id); }

static int var_key(const void *k, const void *b) { return dd_stricmp((char *)k, ((CfgVar *)b)->id); }

/*
static int div_key(const void *k,const void *b) {
  return dd_stricmp((char*)k,(*(CfgDiv**)b)->id);
}
*/

static int read_line(char *&text, char *s, int n)
{
  *s = 0;
  int id = 0;
  *s = text[id];
  if (!*s)
    return 0;
  ++id;
  if (*s == '\r')
  {
    *s = 0;
    text += id;
    return 1;
  }
  if (*s != '\n')
  {
    ++s;
    --n;
  }
  for (--n; n; ++s, --n)
  {
    *s = text[id];
    if (!*s)
      break;
    ++id;
    if (*s == '\r' || *s == '\n')
      break;
  }
  *s = 0;
  if (!n)
  {
    for (;;)
    {
      char c = text[id];
      if (!c)
        break;
      ++id;
      if (c == '\r' || c == '\n')
        break;
    }
  }
  text += id;
  return 1;
}

static int iswhite(char c) { return c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == 0 || c == '\x1A'; }

/*
static int isempty(char *s) {
  for(;;++s) {
    if(!*s) return 1;
    if(!iswhite(*s)) return 0;
  }
}
*/

// CfgDiv

CfgDiv::CfgDiv(char *a) : var(tmpmem), comm(tmpmem)
{
  id = a;
  ord = -1;
  lines = 2;
}

CfgDiv::~CfgDiv()
{
  if (id)
    memfree(id, strmem);
  int i;
  for (i = 0; i < var.size(); ++i)
  {
    if (var[i].id)
      memfree(var[i].id, strmem);
    if (var[i].val)
      memfree(var[i].val, strmem);
  }
  for (i = 0; i < comm.size(); ++i)
  {
    if (comm[i].text)
      memfree(comm[i].text, strmem);
  }
}

void CfgDiv::clear()
{
  int i;
  for (i = 0; i < var.size(); ++i)
  {
    if (var[i].id)
      memfree(var[i].id, strmem);
    if (var[i].val)
      memfree(var[i].val, strmem);
  }
  clear_and_shrink(var);
  for (i = 0; i < comm.size(); ++i)
  {
    if (comm[i].text)
      memfree(comm[i].text, strmem);
  }
  clear_and_shrink(comm);
}

#define LUP_VAR                                                                                                           \
  CfgVar *a = var.size() ? (CfgVar *)dag_bin_search(v, var.data(), var.size(), sizeof(CfgVar), var_key) : (CfgVar *)NULL; \
  if (!a)                                                                                                                 \
  return def

const char *CfgDiv::getstr(const char *v, const char *def) const
{
  LUP_VAR;
  return a->val;
}

bool CfgDiv::getbool(const char *v, bool def) const
{
  LUP_VAR;
  if (dd_stricmp(a->val, "on") == 0)
    return true;
  if (dd_stricmp(a->val, "off") == 0)
    return false;
  if (dd_stricmp(a->val, "yes") == 0)
    return true;
  if (dd_stricmp(a->val, "no") == 0)
    return false;
  if (dd_stricmp(a->val, "true") == 0)
    return true;
  if (dd_stricmp(a->val, "false") == 0)
    return false;
  return strtol(a->val, NULL, 0) ? true : false;
}

int CfgDiv::getint(const char *v, int def) const
{
  LUP_VAR;
  return strtol(a->val, NULL, 0);
}

real CfgDiv::getreal(const char *v, real def) const
{
  LUP_VAR;
  return strtod(a->val, NULL);
}

Point4 CfgDiv::getpoint4(const char *v, const Point4 &def) const
{
  LUP_VAR;
  String cs(a->val + (a->val[0] == '(' ? 1 : 0));
  Point4 ret;
#define ptsize 4
  int cr, i;
  for (cr = 0, i = 0; i < (int)cs.size() - 1; ++i)
  {
    if (cs[i] == ',')
    {
      cs[i] = 0;
      ret[cr] = strtod((char *)cs, NULL);
      if (++cr >= ptsize)
        return ret;
      erase_items(cs, 0, i + 1);
      i = -1;
    }
  }
  ret[cr] = strtod((char *)cs, NULL);
  if (++cr >= ptsize)
    return ret;
  return def;
#undef ptsize
}

Point3 CfgDiv::getpoint3(const char *v, const Point3 &def) const
{
  LUP_VAR;
  String cs(a->val + (a->val[0] == '(' ? 1 : 0));
  Point3 ret;
#define ptsize 3
  int cr, i;
  for (cr = 0, i = 0; i < (int)cs.size() - 1; ++i)
  {
    if (cs[i] == ',')
    {
      cs[i] = 0;
      ret[cr] = strtod((char *)cs, NULL);
      if (++cr >= ptsize)
        return ret;
      erase_items(cs, 0, i + 1);
      i = -1;
    }
  }
  ret[cr] = strtod((char *)cs, NULL);
  if (++cr >= ptsize)
    return ret;
  return def;
#undef ptsize
}

Point2 CfgDiv::getpoint2(const char *v, const Point2 &def) const
{
  LUP_VAR;
  String cs(a->val + (a->val[0] == '(' ? 1 : 0));
  Point2 ret;
#define ptsize 2
  int cr, i;
  for (cr = 0, i = 0; i < (int)cs.size() - 1; ++i)
  {
    if (cs[i] == ',')
    {
      cs[i] = 0;
      ret[cr] = strtod((char *)cs, NULL);
      if (++cr >= ptsize)
        return ret;
      erase_items(cs, 0, i + 1);
      i = -1;
    }
  }
  ret[cr] = strtod((char *)cs, NULL);
  if (++cr >= ptsize)
    return ret;
  return def;
#undef ptsize
}

IPoint3 CfgDiv::getipoint3(const char *v, const IPoint3 &def) const
{
  LUP_VAR;
  String cs(a->val);
  IPoint3 ret;
#define ptsize 3
  int cr, i;
  for (cr = 0, i = 0; i < (int)cs.size() - 1; ++i)
  {
    if (cs[i] == ',')
    {
      cs[i] = 0;
      ret[cr] = strtol((char *)cs, NULL, 0);
      if (++cr >= ptsize)
        return ret;
      erase_items(cs, 0, i + 1);
      i = -1;
    }
  }
  ret[cr] = strtol((char *)cs, NULL, 0);
  if (++cr >= ptsize)
    return ret;
  return def;
#undef ptsize
}

E3DCOLOR CfgDiv::gete3dcolor(const char *v, E3DCOLOR def) const
{
  LUP_VAR;
  int ret[4];
  ret[3] = 255;
  int n = sscanf(a->val, " %i , %i , %i , %i", ret, ret + 1, ret + 2, ret + 3);
  if (n == 3 || n == 4)
  {
    for (int i = 0; i < 4; ++i)
      if (ret[i] < 0)
        ret[i] = 0;
      else if (ret[i] > 255)
        ret[i] = 255;
    return E3DCOLOR_MAKE(ret[0], ret[1], ret[2], ret[3]);
  }
  return def;
}

IPoint2 CfgDiv::getipoint2(const char *v, const IPoint2 &def) const
{
  LUP_VAR;
  String cs(a->val);
  IPoint2 ret;
#define ptsize 2
  int cr, i;
  for (cr = 0, i = 0; i < (int)cs.size() - 1; ++i)
  {
    if (cs[i] == ',')
    {
      cs[i] = 0;
      ret[cr] = strtol((char *)cs, NULL, 0);
      if (++cr >= ptsize)
        return ret;
      erase_items(cs, 0, i + 1);
      i = -1;
    }
  }
  ret[cr] = strtol((char *)cs, NULL, 0);
  if (++cr >= ptsize)
    return ret;
  return def;
#undef ptsize
}

Color3 CfgDiv::getcolor3(const char *var_name, const Color3 &def) const
{
  return Color3(&getpoint3(var_name, Point3(def.r, def.g, def.b))[0]);
}

Color4 CfgDiv::getcolor4(const char *var_name, const Color4 &def) const
{
  return Color4(&getpoint4(var_name, Point4(def.r, def.g, def.b, def.a))[0]);
}

// CfgReader

CfgReader::CfgReader() : div(tmpmem)
{
  curdiv = -1;
  includes_num = 0;
}

CfgReader::CfgReader(const char *fn, const char *div) : div(tmpmem)
{
  curdiv = -1;
  includes_num = 0;
  getdiv(fn, div);
}

CfgReader::~CfgReader() {}

void CfgReader::clear()
{
  curdiv = -1;
  includes_num = 0;
  clear_and_shrink(div);
  fname = "";
}

void CfgReader::resolve_includes()
{
  // debug ( "start resolve includes: %d", includes_num );
  for (int i = 0; i < includes_num; i++)
  {
    for (int j = 0; j < div.size(); ++j)
    {
      const char *inc = div[j].getstr(String(32, "INCLUDE%d", i), NULL);
      if (inc)
      {
        // debug ( "  resolve include: %s", inc );
        readfile(inc, false);
      }
    }
  }
  // debug ( "-- includes resolved: %d", includes_num );
}
// static bool _debug=false;

int CfgReader::readfile(const char *fn, bool clr)
{
  if (dd_stricmp(fname, fn) == 0)
  {
    curdiv = -1;
    return 1;
  }
  if (clr)
    clear();
  file_ptr_t fh = df_open(fn, DF_READ);
  if (!fh)
    return 0;
  int flg = df_length(fh);
  String text;
  text.resize(flg + 1);
  if (df_read(fh, &text[0], flg) != flg)
  {
    df_close(fh);
    return 0;
  }
  text[flg] = 0;
  df_close(fh);
  // debug("!!! <%s>",fn);
  // if(dd_stricmp(fn,"deathdrive.cfg")==0) _debug=true;
  int res = readtext(&text[0], clr);
  //_debug=false;
  fname = fn;
  return res;
}

int CfgReader::getdiv(const char *fn, const char *div_name)
{
  if (!readfile(fn))
    return 0;
  return getdiv(div_name);
}

int CfgReader::getdiv_text(char *text, const char *div_name)
{
  if (!readtext(text))
    return 0;
  return getdiv(div_name);
}

int CfgReader::getdiv(const char *dn)
{
  curdiv = -1;
  if (!dn)
    return 0;
  for (int i = 0; i < div.size(); ++i)
    if (dd_stricmp(div[i].id, dn) == 0)
    {
      curdiv = i;
      return 1;
    }
  return 0;
}

int CfgReader::readtext(char *text, bool clr)
{
  if (clr)
    clear();
  if (!text)
    return 0;
  // if(_debug) debug("--------------------------------------------");
  char s[512];
  int cd = -1;
  while (read_line(text, s, 512))
  {
    // if(_debug) debug("$ <%s>",s);
    char *p = s;
    for (; *p; ++p)
      if (!iswhite(*p))
        break;
    if (!*p)
      continue;
    if (*p == '#' || *p == '\'' || *p == ';' || *p == '/')
      continue;
    if (*p == '[')
    {
      cd = -1;
      for (++p; *p; ++p)
        if (!iswhite(*p))
          break;
      if (!*p)
        continue;
      char *id = p;
      p = strchr(p, ']');
      if (!p)
        p = id + strlen(id);
      for (--p; p >= id; --p)
        if (!iswhite(*p))
          break;
      p[1] = 0;
      int i;
      for (i = 0; i < div.size(); ++i)
        if (dd_stricmp(div[i].id, id) == 0)
          break;
      if (i >= div.size())
      {
        i = append_items(div, 1);
        if (i < 0)
          continue;
        div[i].id = str_dup(id, strmem);
      }
      cd = i;
    }
    else
    {
      if (cd < 0)
        continue;
      char *id = p;
      for (++p; *p; ++p)
        if (iswhite(*p) || *p == '=')
          break;
      if (!*p)
        continue;
      *p = 0;
      for (++p; *p; ++p)
        if (!iswhite(*p) && *p != '=')
          break;
      // if(!*p) continue;
      char *val = p;
      char qc = 0;
      if (*p == '\'' || *p == '\"')
      {
        qc = *p;
        ++val;
        ++p;
      }
      //      debug("<%s>",p);
      int i;
      for (i = i_strlen(p) - 1; i >= 0; --i)
      {
        //        debug("i=%d '%c' %d %d",i,p[i],(p[i]==qc),(iswhite(p[i])));
        if (p[i] == qc || !(iswhite(p[i])))
          break;
      }
      //      debug("i=%d",i);
      if (i >= 0)
        if (p[i] == qc)
          --i;
      ++i;
      //      debug("i=%d",i);
      if (i < 0)
        i = 0;
      p[i] = 0;

      // if(_debug) debug("@ <%s>=<%s>",id,val);

      CfgDiv &d = div[cd];
      char *include = NULL;
      if (strcmp(id, "INCLUDE") == 0)
      {
        include = str_dup((char *)String(16, "INCLUDE%d", includes_num), strmem);
        includes_num++;
        i = d.var.size();
      }
      else
        for (i = 0; i < d.var.size(); ++i)
          if (dd_stricmp(d.var[i].id, id) == 0)
            break;

      if (i < d.var.size())
      {
        char *v = str_dup(val, strmem);
        if (!v)
          continue;
        memfree(d.var[i].val, strmem);
        d.var[i].val = v;
      }
      else
      {
        CfgVar z;
        z.id = include ? include : str_dup(id, strmem);
        if (!z.id)
          continue;
        z.val = str_dup(val, strmem);
        if (!z.val)
        {
          memfree(z.id, strmem);
          continue;
        }
        d.var.push_back(z);
      }
      //      debug("%s=<%s>",id,val);
    }
  }
  // if(_debug) debug("--------------------------------------------");
  for (int i = 0; i < div.size(); ++i)
    sort(div[i].var, &var_cmp);
  return 1;
}

static CfgDiv *_empty_div = NULL;
CfgDiv &CfgReader::getcurdiv()
{
  if (curdiv >= 0 && curdiv < div.size())
    return div[curdiv];
  if (!_empty_div)
    _empty_div = new (inimem) CfgDiv;
  _empty_div->clear();
  return *_empty_div;
}

const char *CfgReader::getstr(const char *var, const char *def) const { return getcurdiv().getstr(var, def); }

bool CfgReader::getbool(const char *var, bool def) const { return getcurdiv().getbool(var, def); }

int CfgReader::getint(const char *var, int def) const { return getcurdiv().getint(var, def); }

real CfgReader::getreal(const char *var, real def) const { return getcurdiv().getreal(var, def); }

IPoint2 CfgReader::getipoint2(const char *var, const IPoint2 &def) const { return getcurdiv().getipoint2(var, def); }

IPoint3 CfgReader::getipoint3(const char *var, const IPoint3 &def) const { return getcurdiv().getipoint3(var, def); }

E3DCOLOR CfgReader::gete3dcolor(const char *var, E3DCOLOR def) const { return getcurdiv().gete3dcolor(var, def); }

Point2 CfgReader::getpoint2(const char *var, const Point2 &def) const { return getcurdiv().getpoint2(var, def); }

Point3 CfgReader::getpoint3(const char *var, const Point3 &def) const { return getcurdiv().getpoint3(var, def); }

Point4 CfgReader::getpoint4(const char *var, const Point4 &def) const { return getcurdiv().getpoint4(var, def); }

Color3 CfgReader::getcolor3(const char *var, const Color3 &def) const
{
  return Color3(&getpoint3(var, Point3(def.r, def.g, def.b))[0]);
}

Color4 CfgReader::getcolor4(const char *var, const Color4 &def) const
{
  return Color4(&getpoint4(var, Point4(def.r, def.g, def.b, def.a))[0]);
}

#define EXPORT_PULL dll_pull_iosys_obsolete_cfg
#include <supp/exportPull.h>
