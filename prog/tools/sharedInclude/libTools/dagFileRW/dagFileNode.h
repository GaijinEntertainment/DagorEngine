// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_math3d.h>
#include <util/dag_simpleString.h>
#include <3d/dag_materialData.h>

// forward declarations for external classes
class MeshBones;
class Mesh;
class MaterialDataList;

// forward declarations for classes
class Node;
class NodeTMChangeCB;


#define OCID_OBJECT      1
#define OCID_MESHHOLDER  2
#define OCID_LIGHT       3
#define OCID_SHAPEHOLDER 4
#define nodemem          midmem


class Object
{
public:
  DAG_DECLARE_NEW(midmem)

  virtual ~Object();
  virtual int classid();
  virtual bool isSubOf(unsigned);
};


class MeshHolderObj : public Object
{
public:
  DAG_DECLARE_NEW(nodemem)

  Mesh *mesh;
  MeshBones *bones;

  struct MorphTarget
  {
    SimpleString name;
    Node *node;
  };

  Tab<MorphTarget> morphTargets;


  MeshHolderObj(Mesh *m = NULL);
  MeshHolderObj(const MeshHolderObj &) = delete;
  ~MeshHolderObj();
  void setmesh(Mesh *);
  void setbones(MeshBones *);
  int classid();
  bool isSubOf(unsigned);
};


#define NODEFLG_WTMOK      1
#define NODEFLG_RENDERABLE 2
#define NODEFLG_CASTSHADOW 4
#define NODEFLG_RCVSHADOW  8
#define NODEFLG_HIDDEN     16
#define NODEFLG_FADE       32
#define NODEFLG_FADENULL   64
#define NODEFLG_POINTCLOUD 128

class Node
{
public:
  DAG_DECLARE_NEW(nodemem)

  class NodeEnumCB
  {
  public:
    virtual int node(Node &c) = 0; /// 0 continue, 1 -remember as result & continue, -1 stop & return this node
  };

  Tab<Node *> child;
  Node *parent;
  TMatrix tm, wtm;
  Object *obj;
  MaterialDataList *mat;
  SimpleString name, script;
  int flags;
  int id;
  int tmpval;

  Node();
  Node(const Node &) = delete;
  ~Node();

  void setobj(Object *);
  Node *find_node(int id);

  void calc_wtm();
  void calc_wtm(NodeTMChangeCB &);

  void invalidate_anim();
  void invalidate_wtm();

  Node *find_node(const char *name);  /// Case sensitive
  Node *find_inode(const char *name); /// Case insensitive

  bool enum_nodes(NodeEnumCB &cb, Node **res = NULL); /// return true if stopped,false if finished
};


class NodeTMChangeCB
{
public:
  virtual void node_tm_changed(Node *) = 0;
};


class AScene
{
public:
  DAG_DECLARE_NEW(nodemem)

  Node *root;

  AScene();
  AScene(const AScene &) = delete;
  virtual ~AScene();
};

enum
{
  LASF_MATNAMES = 1 << 0,
  LASF_NOMATS = 1 << 1,
  LASF_NULLMATS = 1 << 2,
  LASF_NOMESHES = 1 << 3,
  LASF_NOSPLINES = 1 << 4,
  LASF_NOLIGHTS = 1 << 5
};


int load_ascene(const char *fn, AScene &sc, int flg = 0, bool fatal_on_error = true, PtrTab<MaterialData> *mat_list = nullptr);
