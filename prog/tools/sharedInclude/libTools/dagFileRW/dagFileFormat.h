// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <math/dag_Point3.h>
#include <math/dag_Quat.h>
#include <math/dag_color.h>
#include <math/dag_e3dColor.h>

#pragma pack(push, 1)

#define DAG_ID 0x1A474144

enum
{
  DAG_END = 0,
  DAG_NODE,
  DAG_MATER,
  DAG_TEXTURES,
  DAG_KEYLABELS,
  DAG_NOTETRACK,
};

// inside DAG_NODE:
enum
{
  DAG_NODE_TM = 1,
  DAG_NODE_DATA,
  DAG_NODE_SCRIPT,
  DAG_NODE_OBJ,
  DAG_NODE_MATER,
  DAG_NODE_ANIM,
  DAG_NODE_CHILDREN,
  DAG_NODE_NOTETRACK,
};

// inside DAG_NODE_OBJ:
enum
{
  DAG_OBJ_MESH = 1,
  DAG_OBJ_BIGMESH,
  DAG_OBJ_LIGHT,
  DAG_OBJ_BONES,
  DAG_OBJ_SPLINES,
  DAG_OBJ_FACEFLG,
  DAG_OBJ_NORMALS,

  DAG_OBJ_MORPH = 10,
};

#define DAG_FACEFLG_EDGE0  1
#define DAG_FACEFLG_EDGE1  2
#define DAG_FACEFLG_EDGE2  4
#define DAG_FACEFLG_HIDDEN 8

#define DAG_MF_2SIDED 1
#define DAG_MF_16TEX  0x80000000

struct DagColor
{
  unsigned char b, g, r;
  DagColor(unsigned char r1, unsigned char g1, unsigned char b1) : r(r1), g(g1), b(b1) {}
  DagColor() : r(0), g(0), b(0) {}
  DagColor(const Color3 &c)
  {
    if (c.r < 0)
      r = 0;
    else if (c.r > 1)
      r = 255;
    else
      r = (unsigned char)(c.r * 255);
    if (c.g < 0)
      g = 0;
    else if (c.g > 1)
      g = 255;
    else
      g = (unsigned char)(c.g * 255);
    if (c.b < 0)
      b = 0;
    else if (c.b > 1)
      b = 255;
    else
      b = (unsigned char)(c.b * 255);
  }
  DagColor(E3DCOLOR const &c) : r(c.r), g(c.g), b(c.b) {}
};

#define DAGTEXNUM 16

#define DAGBADMATID 0xFFFF

struct DagMater
{
  // uint8_t namelen;
  // char matname[namelen];
  DagColor amb, diff, spec, emis;
  float power;
  unsigned flags;
  unsigned short texid[DAGTEXNUM];
  unsigned short _resv[8];
  // uint8_t namelen;
  // char classname[namelen];
  // char script[];
};

#define DAG_NF_RENDERABLE 1
#define DAG_NF_CASTSHADOW 2
#define DAG_NF_RCVSHADOW  4

struct DagNodeData
{
  unsigned short id, cnum;
  unsigned flg;
  // node name follows
};

struct DagLight
{
  float r, g, b;
  float range, drad;
  unsigned char decay;
};

enum
{
  DAG_LIGHT_OMNI,
  DAG_LIGHT_SPOT,
  DAG_LIGHT_DIR
};

struct DagLight2
{
  unsigned char type;
  float hotspot, falloff; // in degrees, if angle
  float mult;
};

struct DagBone
{
  unsigned short id;
  float tm[4][3];
};

struct DagPosKey
{
  int t;
  Point3 p, i, o;
};

struct DagRotKey
{
  int t;
  Quat p, i, o;
};

struct DagNoteKey
{
  unsigned short id;
  int t;
};

struct DagFace
{
  unsigned short v[3];
  unsigned smgr;
  unsigned short mat;
};

struct DagBigFace
{
  unsigned int v[3];
  unsigned smgr;
  unsigned short mat;
};

struct DagTFace
{
  unsigned short t[3];
};

struct DagBigTFace
{
  unsigned int t[3];
};

#define DAG_SPLINE_CLOSED 1

// bezier or auto
#define DAG_SKNOT_BEZIER 1
// corner or smooth
#define DAG_SKNOT_CORNER 2

/*
SPLINES format:
        int numsplines;
        for(each spline) {
                char flg; // DAG_SPLINE_CLOSED
                int numknots;
                for(each knot) {
                        char ktype;
                        struct{float x,y,z;} in,pt,out; // absolute positions of control points
                }
        }
*/

/*
MESH format:
        uint16_t numv;
        struct{float x,y,z;} vert[numv];
        uint16_t numf;
        DagFace face[numf];
        uint8_t number_of_mapping_channels;
        for(each channel) {
                uint16_t numtv;
                uint8_t num_tv_coords; // currently 2 or 3
                uint8_t channel_id;
                struct{float c[num_tv_coords];} tvert[numtv];
                DagTFace tface[numf];
        }
BIGMESH format:
        same as above, change shorts to ints and Faces to BigFaces
*/

/*
BONES format:
        uint16_t numb;
        DagBone bone[numb];
        uint32_t numv; // should be equal to number of mesh vertexes
        float weight[numb][numv];
*/

/*
ANIM format:
        uint16_t ntrack_id;
        pos_track; // T=Point3
        rot_track; // T=Quat
        scl_track; // T=Point3

        Track format:
                uint16_t numkeys;
                if(numkeys==1) {
                        T val;
                }else for(each key) {
                        DagPosKey/DagRotKey;
                }
*/

#pragma pack(pop)
