//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <stdio.h>
#include <libTools/pageAsg/asg_decl.h>

class Point2;
class Point3;
class Point4;
class IPoint2;
class IPoint3;
struct E3DCOLOR;
class DataBlock;


struct AsgStateGenParameter
{
  enum
  {
    TYPE_INDENT,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_INT,
    TYPE_REAL,
    TYPE_POINT2,
    TYPE_POINT3,
    TYPE_E3DCOLOR,
  };

  String name, caption;
  int type;
  union Value
  {
    int indentSize;
    const char *s;
    int i;
    float r;
    float p2[2];
    float p3[3];
    float p4[4];
    int ip2[2];
    int ip3[3];
    bool b;
    int c;

    Point2 &point2() { return reinterpret_cast<Point2 &>(p2); }
    Point3 &point3() { return reinterpret_cast<Point3 &>(p3); }
    Point4 &point4() { return reinterpret_cast<Point4 &>(p4); }
    IPoint2 &ipoint2() { return reinterpret_cast<IPoint2 &>(ip2); }
    IPoint3 &ipoint3() { return reinterpret_cast<IPoint3 &>(ip3); }
    E3DCOLOR &color() { return reinterpret_cast<E3DCOLOR &>(c); }
  } defVal;

public:
  AsgStateGenParameter(const DataBlock &blk);
  AsgStateGenParameter(int indent_size);

  ~AsgStateGenParameter();


  void generateDeclaration(FILE *fp);
  void generateValue(FILE *fp, const DataBlock &data);
};


struct AsgStateGenParamGroup
{
  struct Item
  {
    union
    {
      AsgStateGenParameter *p;
      AsgStateGenParamGroup *g;
    };
    bool group;
  };
  Tab<Item> items;
  String name, caption;

public:
  AsgStateGenParamGroup(const char *_name, const char *_caption, const DataBlock &blk);
  ~AsgStateGenParamGroup();

  void generateClassCode(FILE *fp, AsgStateGenParamGroup *g, int indent, bool skip_use);
  static bool generateClassCode(const char *fname, AsgStateGenParamGroup *pg);

  void generateStructDecl(FILE *fp);
  void generateStaticVar(FILE *fp, AsgStateGenParamGroup *g, const DataBlock &d, bool skip_use);
  void generateStaticVar(FILE *fp, int state_id, const DataBlock &data);
  void generateFuncCode(FILE *fp, int state_id, const DataBlock &data);
};
