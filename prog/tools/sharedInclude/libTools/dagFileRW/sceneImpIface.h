
#ifndef __DAGOR_LOAD3D_H
#define __DAGOR_LOAD3D_H
#pragma once

#include <3d/dag_materialData.h>
#include <math/dag_Point3.h>
#include <util/dag_stdint.h>

class Mesh;
class SplineShape;

#define IMP_MF_2SIDED 1

#define IMPTEXNUM 8

struct D3dLight
{
  enum
  {
    TYPE_POINT = 1,
    TYPE_SPOT,
    TYPE_DIR
  };

  int type;
  D3dColor diff, spec, amb;
  Point3 pos, dir;
  float range, falloff, atten0, atten1, atten2, inang, outang;
};

struct ImpColor
{
  uint8_t b, g, r;

  int isblack() { return r == 0 && g == 0 && b == 0; }
  operator D3dColor() { return D3dColor(r / 255.0f, g / 255.0f, b / 255.0f); }
};

struct ImpMat
{
  ImpColor amb, diff, spec, emis;
  float power;
  uint32_t flags;
  uint16_t texid[IMPTEXNUM];
  float param[IMPTEXNUM];
  char *name;
  char *classname;
  char *script;
};

#define IMP_NF_RENDERABLE 1
#define IMP_NF_CASTSHADOW 2
#define IMP_NF_RCVSHADOW  4

class ImpSceneCB
{
public:
  DAG_DECLARE_NEW(tmpmem)

  virtual void nomem_error(const char *fn) = 0;
  virtual void open_error(const char *fn) = 0;
  virtual void format_error(const char *fn) = 0;
  virtual void read_error(const char *fn) = 0;

  // following functions return 0 to skip respective blocks
  virtual int load_textures(int num) = 0;
  virtual int load_material(const char *name) = 0;
  virtual int load_node() = 0;          // return 0 to skip node and its children
  virtual int load_node_children() = 0; // return 0 to skip children only
  virtual int load_node_data() = 0;
  virtual int load_node_tm() = 0;
  virtual int load_node_mat(int numsubs) = 0;
  virtual int load_node_script() = 0;
  virtual int load_node_obj() = 0;
  virtual int load_node_bones(int num) = 0;
  virtual int load_node_bonesinf(int numv) = 0;
  virtual bool load_node_morph(int num_targets) = 0;

  // following functions return 0 to cancel loading
  virtual int load_texture(int, const char *texfn) = 0;
  virtual int load_material(ImpMat &) = 0; // free script if you don't need it
  virtual int load_node_data(char *name, uint16_t id, int numchild, int flg) = 0;
  virtual int load_node_tm(TMatrix &) = 0;
  virtual int load_node_submat(int, uint16_t id) = 0;
  virtual int load_node_script(const char *) = 0; // free script if you don't need it
  virtual int load_node_mesh(Mesh *) = 0;         // delete mesh if you don't need it
  virtual int load_node_light(D3dLight &) = 0;
  // NOTE: you must call recalc() on splines before using them
  virtual int load_node_splines(SplineShape *) = 0; // delete shape if you don't need it
  virtual int load_node_bone(int, uint16_t nodeid, TMatrix &) = 0;
  virtual int load_node_bonesinf(real *inf) = 0;                                         // inf[numb][numv]; free it
  virtual bool load_node_morph_target(int index, uint16_t nodeid, const char *name) = 0; // name is temporary
};


// one-pass loading
// returns 0 if loading cancelled (usually on error)
int load_scene(const char *fn, ImpSceneCB &);

// controls how relative paths are handled during DAG loading
void disable_relative_textures_path_expansion();
void enable_relative_textures_path_expansion();


#endif
