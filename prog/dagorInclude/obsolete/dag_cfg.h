//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <math/dag_mathBase.h>
#include <math/dag_e3dColor.h>
#include <memory/dag_mem.h>


struct Color4;
struct Color3;

struct CfgVar
{
  char *id, *val;
  int ln;
};

struct CfgComm
{
  char *text;
  int ln;
};

#include <supp/dag_define_KRNLIMP.h>

struct CfgDiv
{
  DAG_DECLARE_NEW(tmpmem)

  char *id;
  Tab<CfgVar> var;
  Tab<CfgComm> comm;
  int ord, lines;

  KRNLIMP CfgDiv(char *idname = NULL);
#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
  CfgDiv(const CfgDiv &) = delete;
#endif
  KRNLIMP ~CfgDiv();
  KRNLIMP void clear();
  KRNLIMP const char *getstr(const char *var, const char *def = NULL) const;
  KRNLIMP bool getbool(const char *var, bool def) const;
  KRNLIMP int getint(const char *var, int def) const;
  KRNLIMP real getreal(const char *var, real def) const;
  KRNLIMP IPoint2 getipoint2(const char *var, const IPoint2 &def) const;
  KRNLIMP IPoint3 getipoint3(const char *var, const IPoint3 &def) const;
  KRNLIMP E3DCOLOR gete3dcolor(const char *var, E3DCOLOR def) const;
  KRNLIMP Point2 getpoint2(const char *var, const Point2 &def) const;
  KRNLIMP Point3 getpoint3(const char *var, const Point3 &def) const;
  KRNLIMP Point4 getpoint4(const char *var, const Point4 &def) const;
  KRNLIMP Color3 getcolor3(const char *var, const Color3 &def) const;
  KRNLIMP Color4 getcolor4(const char *var, const Color4 &def) const;
};
DAG_DECLARE_RELOCATABLE(CfgDiv);

class CfgReader
{
public:
  DAG_DECLARE_NEW(tmpmem)

  Tab<CfgDiv> div;
  int curdiv;
  String fname;
  int includes_num;

  KRNLIMP CfgReader();
  KRNLIMP CfgReader(const char *fn, const char *div = NULL);
  KRNLIMP ~CfgReader();
  KRNLIMP void resolve_includes();

  KRNLIMP void clear();
  KRNLIMP int readfile(const char *fn, bool clr = true);
  KRNLIMP int readtext(char *text, bool clr = true);
  KRNLIMP int getdiv(const char *fn, const char *div);
  KRNLIMP int getdiv(const char *div);
  KRNLIMP int getdiv_text(char *text, const char *div);

  KRNLIMP CfgDiv &getcurdiv();
  KRNLIMP const CfgDiv &getcurdiv() const { return const_cast<CfgReader *>(this)->getcurdiv(); }
  inline operator CfgDiv &() { return getcurdiv(); }
  inline operator const CfgDiv &() const { return getcurdiv(); }

  KRNLIMP const char *getstr(const char *var, const char *def = NULL) const;
  KRNLIMP bool getbool(const char *var, bool def) const;
  KRNLIMP int getint(const char *var, int def) const;
  KRNLIMP real getreal(const char *var, real def) const;
  KRNLIMP IPoint2 getipoint2(const char *var, const IPoint2 &def) const;
  KRNLIMP IPoint3 getipoint3(const char *var, const IPoint3 &def) const;
  KRNLIMP E3DCOLOR gete3dcolor(const char *var, E3DCOLOR def) const;
  KRNLIMP Point2 getpoint2(const char *var, const Point2 &def) const;
  KRNLIMP Point3 getpoint3(const char *var, const Point3 &def) const;
  KRNLIMP Point4 getpoint4(const char *var, const Point4 &def) const;
  KRNLIMP Color3 getcolor3(const char *var, const Color3 &def) const;
  KRNLIMP Color4 getcolor4(const char *var, const Color4 &def) const;
};

#include <supp/dag_undef_KRNLIMP.h>
