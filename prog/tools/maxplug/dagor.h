// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#define FontUtil_CID       Class_ID(0x3409613e, 0x754e4800)
#define DagorMat_CID       Class_ID(0x70a066e2, 0x18a04e07)
#define DagorMat2_CID      Class_ID(0x72fcfac4, 0xddfb36f0)
#define ExpUtil_CID        Class_ID(0x53784107, 0x8a06f33)
#define DAGIMP_CID         Class_ID(0x4306338e, 0x73f0114b)
#define DagUtil_CID        Class_ID(0x6eae3224, 0x38a2327b)
#define Dummy_CID          Class_ID(0x1abf52ba, 0x256f0991)
#define RBDummy_CID        Class_ID(0x319b7f66, 0x4e157c84)
#define Texmaps_CID        Class_ID(0x6c79794a, 0x2c9c5949)
#define VPNORM_CID         Class_ID(0x38bb0602, 0x636949ac)
#define VPZBUF_CID         Class_ID(0x3ca32dff, 0x5207050d)
#define DagUtilFreeCam_CID Class_ID(0xc765357, 0x25063e74)
#define TexAnimIO_CID      Class_ID(0x3ce771ac, 0x3663135a)
#define PolyBumpUtil_CID   Class_ID(0x7d651a5f, 0x79bd1cf0)
#define ImpUtil_CID        Class_ID(0x5575310b, 0x5b5840b1)

#define MatConvUtil_CID              Class_ID(0x13354bd4, 0x29c32883)
#define OBJECT_PROPERTIES_EDITOR_CID Class_ID(0x73c5685c, 0x71a47013)

#define DAGEXP_CID Class_ID(0x76BEE712, 0x7AB041AA)

ClassDesc *GetFontUtilCD();
ClassDesc *GetMatConvUtilCD();
ClassDesc *GetMaterCD();
ClassDesc *GetMaterCD2();
ClassDesc *GetExpUtilCD();
ClassDesc *GetDAGIMPCD();
ClassDesc *GetDagUtilCD();
ClassDesc *GetDummyCD();
ClassDesc *GetRBDummyCD();
ClassDesc *GetTexmapsCD();
ClassDesc *GetVPnormCD();
ClassDesc *GetVPzbufCD();
ClassDesc *GetDagFreeCamUtilCD();
ClassDesc *GetTexAnimIOCD();
ClassDesc *GetPolyBumpCD();
ClassDesc *GetObjectPropertiesEditorCD();
ClassDesc *GetDAGEXPCD();
ClassDesc *GetImpUtilCD();

TCHAR *GetString(int id);

const TCHAR *make_path_rel(const TCHAR *);

void set_dagor_path(const char *p);

extern char dagor_path[];

extern HINSTANCE hInstance;

enum
{
  EXP_HID = 0x0001,
  EXP_SEL = 0x0002,
  EXP_MESH = 0x0004,
  EXP_LT = 0x0008,
  EXP_CAM = 0x0010,
  EXP_HLP = 0x0020,
  EXP_MAT = 0x0040,
  // EXP_ANI = 0x0080,   // unused
  EXP_ARNG = 0x0100,
  EXP_LTARG = 0x0200,
  EXP_CTARG = 0x0400,
  EXP_UKEYS = 0x0800,
  EXP_UNTKEYS = 0x1000,
  EXP_SPLINE = 0x2000,
  EXP_DONTCHKKEYS = 0x4000,
  EXP_NO_VNORM = 0x8000,

  EXP_DONT_REDUCE_POS = 0x00010000,
  EXP_DONT_REDUCE_ROT = 0x00020000,
  EXP_DONT_REDUCE_SCL = 0x00040000,
  EXP_LOOPED_ANIM = 0x00080000,
  EXP_MATOPT = 0x00100000,

  EXP_DONT_CALC_MOMJ = 0x01000000,
  // EXP_BY_GROUP = 0x02000000, // This is no longer used. The value cannot be reused because expflg is saved to the Max files.

  EXP_OBJECTS = EXP_MESH | EXP_LT | EXP_CAM | EXP_HLP | EXP_SPLINE,
  EXP_DEFAULT = EXP_MESH | EXP_LT | EXP_CAM | EXP_HLP | EXP_SPLINE | EXP_MAT | EXP_MATOPT
};


#define DEGENERATE_VERTEX_DELTA 1e-5f // 0.01mm

inline bool is_equal_float(float left, float right, float delta = 1e-4f)
{
  float diff = left - right;
  (*(DWORD *)&diff) &= 0x7FFFFFFF; // fabsf()
  return diff < delta;
}


inline bool is_equal_point(Point3 &left, Point3 &right, float delta = 1e-4f)
{
  return is_equal_float(left.x, right.x, delta) && is_equal_float(left.y, right.y, delta) && is_equal_float(left.z, right.z, delta);
}


Matrix3 get_scaled_stretch_node_tm(INode *node, TimeValue time);

Matrix3 get_scaled_object_tm(INode *node, TimeValue time);

template <typename ValueType>
BOOL pb_get_value(IParamBlock &pb, int i, TimeValue t, ValueType &v)
{
  Interval valid = FOREVER;
  return pb.GetValue(i, t, v, valid);
}
