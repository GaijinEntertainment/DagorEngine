// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

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
#ifdef VERSION_3DSMAX
  DagColor() { memset(this, 0, sizeof(*this)); }
  DagColor(const Color &c)
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
  operator Color() { return Color(r / 255.0f, g / 255.0f, b / 255.0f); }
#else
  DagColor() { memset(this, 0, sizeof(*this)); }
#endif
};

#define DAGTEXNUM 16

#define DAGBADMATID 0xFFFF

struct DagMater
{
  // uchar namelen;
  // char matname[namelen];
  DagColor amb, diff, spec, emis;
  float power;
  unsigned flags;
  unsigned short texid[DAGTEXNUM];
  unsigned short _resv[8];
  // uchar namelen;
  // char classname[namelen];
  // char script[];

  DagMater() { memset(this, 0, sizeof(*this)); }
};
static bool operator==(const DagMater &a, const DagMater &b) { return !memcmp(&a, &b, sizeof(DagMater)); }

#define DAG_NF_RENDERABLE 1
#define DAG_NF_CASTSHADOW 2
#define DAG_NF_RCVSHADOW  4

struct DagNodeData
{
  unsigned short id, cnum;
  unsigned flg;
  // node name follows

  DagNodeData() { memset(this, 0, sizeof(*this)); }
};

struct DagLight
{
  float r, g, b;
  float range, drad;
  unsigned char decay;
  DagLight() { memset(this, 0, sizeof(*this)); }
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
  DagLight2() { memset(this, 0, sizeof(*this)); }
};

struct DagBone
{
  unsigned short id;
  float tm[4][3];
  DagBone() { memset(this, 0, sizeof(*this)); }
};

struct DagPosKey
{
  int t;
  Point3 p, i, o;
  DagPosKey() { memset(this, 0, sizeof(*this)); }
};

struct DagRotKey
{
  int t;
  Quat p, i, o;
  DagRotKey() { memset(this, 0, sizeof(*this)); }
};

struct DagNoteKey
{
  unsigned short id;
  int t;
  DagNoteKey() { memset(this, 0, sizeof(*this)); }
};

struct DagFace
{
  unsigned short v[3];
  unsigned smgr;
  unsigned short mat;
  DagFace() { memset(this, 0, sizeof(*this)); }
};

struct DagBigFace
{
  unsigned int v[3];
  unsigned smgr;
  unsigned short mat;
  DagBigFace() { memset(this, 0, sizeof(*this)); }
};

struct DagTFace
{
  unsigned short t[3];
  DagTFace() { memset(this, 0, sizeof(*this)); }
};

struct DagBigTFace
{
  unsigned int t[3];
  DagBigTFace() { memset(this, 0, sizeof(*this)); }
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
        ushort numv;
        struct{float x,y,z;} vert[numv];
        ushort numf;
        DagFace face[numf];
        uchar number_of_mapping_channels;
        for(each channel) {
                ushort numtv;
                uchar num_tv_coords; // currently 2 or 3
                uchar channel_id;
                struct{float c[num_tv_coords];} tvert[numtv];
                DagTFace tface[numf];
        }
BIGMESH format:
        same as above, change shorts to ints and Faces to BigFaces
*/

/*
BONES format:
        ushort numb;
        DagBone bone[numb];
        uint numv; // should be equal to number of mesh vertexes
        float weight[numb][numv];
*/

/*
ANIM format:
        ushort ntrack_id;
        pos_track; // T=Point3
        rot_track; // T=Quat
        scl_track; // T=Point3

        Track format:
                ushort numkeys;
                if(numkeys==1) {
                        T val;
                }else for(each key) {
                        DagPosKey/DagRotKey;
                }
*/

#pragma pack(pop)
