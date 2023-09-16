#include <max.h>
#include <locale.h>
#include "dagor.h"
#include "enumnode.h"
#include <string>

std::string wideToStr(const TCHAR *sw);
M_STD_STRING strToWide(const char *sz);

#define MAKE4C(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))


#define CAPSULE_CLASS_ID Class_ID(0x6d3d77ac, 0x79c939a9)


#define COLL_ID_SPHERE  MAKE4C('C', 's', 'p', 'h')
#define COLL_ID_BOX     MAKE4C('C', 'b', 'o', 'x')
#define COLL_ID_CAPSULE MAKE4C('C', 'c', 'a', 'p')
// #define COLL_ID_CYLINDER  MAKE4C('C', 'c', 'y', 'l')


void explog(TCHAR *s, ...);
void explogWarning(TCHAR *s, ...);
void explogError(TCHAR *s, ...);


static const int CID_DagorPhysicsResource = 0xABEFB687; // DagorPhysicsResource


static double cubic_root(double x)
{
  if (x < 0)
    return -pow(-x, 1.0 / 3.0);
  return pow(x, 1.0 / 3.0);
}


class PhysFileSaver
{
public:
  FILE *file;


  PhysFileSaver(FILE *f) : file(f) {}


  void write(const void *ptr, int len) { ::fwrite(ptr, len, 1, file); }


  int tell() { return ::ftell(file); }


  void seekto(int ofs) { ::fseek(file, ofs, SEEK_SET); }


  template <class T>
  void write(const T &a)
  {
    write(&a, sizeof(a));
  }


  void alignOnDword(int len)
  {
    len &= 3;
    if (len)
    {
      int i = 0;
      write(&i, 4 - len);
    }
  }


  void writeString(const char *s)
  {
    if (!s)
      s = "";
    int len = (int)strlen(s);
    write(len);
    write(s, len);
    alignOnDword(len);
  }


  void beginBlock()
  {
    int ofs = 0;
    write(&ofs, sizeof(int));

    int o = tell();

    Block b;
    b.ofs = o;
    blocks.Append(1, &b);
  }


  void endBlock()
  {
    assert(blocks.Count() > 0);

    Block &b = blocks[blocks.Count() - 1];
    int o = tell();
    seekto(b.ofs - sizeof(int));
    int l = o - b.ofs;
    write(&l, sizeof(int));
    seekto(o);

    blocks.Delete(blocks.Count() - 1, 1);
  }


protected:
  struct Block
  {
    int ofs;
  };

  Tab<Block> blocks;
};


class PhysExportENodeCB : public ENodeCB
{
public:
  Tab<INode *> bodies;

  TimeValue curTime;


  static bool getNodeProp(INode *node, const char *name, TSTR &value)
  {
    if (!node || !name)
      return false;


    return node->GetUserPropString(strToWide(name).c_str(), value) ? true : false;
  }


  static bool getNodePropStr(INode *node, const char *name, TSTR &val)
  {
    std::string s(name);
    s += ":t";
    bool res = getNodeProp(node, s.c_str(), val);
    if (!res)
      return res;

    if (val.length() >= 2)
      if (val[0] == val[val.Length() - 1])
        if (val[0] == '"' || val[0] == '\'')
        {
          val.remove(val.Length() - 1, 1);
          val.remove(0, 1);
        }

    return res;
  }


  static bool getNodePropBool(INode *node, const char *name, bool def)
  {
    TSTR val;
    std::string s(name);
    s += ":b";
    if (!getNodeProp(node, s.c_str(), val))
      return def;

    return _tcsicmp(val, _T("yes")) == 0 || _tcsicmp(val, _T("on")) == 0 || _tcsicmp(val, _T("true")) == 0 ||
           _tcsicmp(val, _T("1")) == 0;
  }


  static float getNodePropReal(INode *node, const char *name, float def)
  {
    TSTR val;
    std::string s(name);
    s += ":r";
    if (!getNodeProp(node, s.c_str(), val))
      return def;


    std::string res = wideToStr(val);
    setlocale(LC_NUMERIC, "C");
    float r = strtod(res.c_str(), NULL);
    setlocale(LC_NUMERIC, "");

    return r;
  }


  static Point3 getNodePropPoint3(INode *node, const char *name, const Point3 &def)
  {
    TSTR val;
    std::string s(name);
    s += ":p3";
    if (!getNodeProp(node, s.c_str(), val))
      return def;

    std::string res = wideToStr(val);

    Point3 p;
    setlocale(LC_NUMERIC, "C");
    if (sscanf(res.c_str(), " %f , %f , %f", &p.x, &p.y, &p.z) != 3)
      return def;
    setlocale(LC_NUMERIC, "");

    return p;
  }


  static void setNodeProp(INode *node, const char *name, const char *val)
  {
    if (!node || !name)
      return;

    M_STD_STRING wname = strToWide(name);
    M_STD_STRING wval = strToWide(val);
    node->SetUserPropString(wname.c_str(), wval.c_str());
  }


  void setNodePropReal(INode *node, const char *name, float val)
  {
    setlocale(LC_NUMERIC, "C");
    char s[256];
    sprintf(s, "%g", val);
    setlocale(LC_NUMERIC, "");

    std::string sn(name);
    sn += ":r";
    setNodeProp(node, sn.c_str(), s);
  }


  void setNodePropPoint3(INode *node, const char *name, const Point3 &val)
  {
    setlocale(LC_NUMERIC, "C");
    char s[256];
    sprintf(s, "%g, %g, %g", val.x, val.y, val.z);
    setlocale(LC_NUMERIC, "");

    std::string sn(name);
    sn += ":p3";
    setNodeProp(node, sn.c_str(), s);
  }


  static inline void adjWtm(Matrix3 &tm)
  {
    MRow *m = tm.GetAddr();
    for (int i = 0; i < 4; ++i)
    {
      float a = m[i][1];
      m[i][1] = m[i][2];
      m[i][2] = a;
    }
    tm.ClearIdentFlag(ROT_IDENT | SCL_IDENT);
  }


  static inline Matrix3 getScaledNodeTm(INode *node, TimeValue time)
  {
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
    float masterScale = (float)GetSystemUnitScale(UNITS_METERS);
#else
    float masterScale = (float)GetMasterScale(UNITS_METERS);
#endif
    Matrix3 tm = node->GetStretchTM(time) * node->GetNodeTM(time);
    // tm.SetTrans(tm.GetTrans() * masterScale);
    tm *= masterScale;
    adjWtm(tm);
    return tm;
  }


  static inline Matrix3 getScaledObjectTm(INode *node, TimeValue time)
  {
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
    float masterScale = (float)GetSystemUnitScale(UNITS_METERS);
#else
    float masterScale = (float)GetMasterScale(UNITS_METERS);
#endif
    Matrix3 tm = node->GetStretchTM(time) * node->GetObjectTM(time);
    // tm.SetTrans(tm.GetTrans() * masterScale);
    tm *= masterScale;
    adjWtm(tm);
    return tm;
  }


  Matrix3 getScaledNodeTm(INode *node) { return getScaledNodeTm(node, curTime); }


  Matrix3 getScaledObjectTm(INode *node) { return getScaledObjectTm(node, curTime); }


  static void makeMatrixRightOrthonormal(Matrix3 &wtm)
  {
    //== check for orthogonality

    wtm.Orthogonalize();

    wtm.SetRow(0, Normalize(wtm.GetRow(0)));
    wtm.SetRow(1, Normalize(wtm.GetRow(1)));
    wtm.SetRow(2, Normalize(wtm.GetRow(2)));

    if (wtm.Parity())
      wtm.SetRow(2, -wtm.GetRow(2));
  }


  static float getAverageScale(Matrix3 &tm)
  {
    //== check for non-uniform scale?

    Point3 s;
    s.x = Length(tm.GetRow(0));
    s.y = Length(tm.GetRow(1));
    s.z = Length(tm.GetRow(2));
    return (s.x + s.y + s.z) / 3;
  }


  virtual int proc(INode *node)
  {
    if (!node)
      return ECB_CONT;
    if (node->IsNodeHidden())
      return ECB_CONT;

    Object *obj = node->GetObjectRef();
    if (!obj)
      return ECB_CONT;

    if (obj->ClassID() == RBDummy_CID)
    {
      bodies.Append(1, &node);
    }

    //== enum more physics-related stuff

    return ECB_CONT;
  }


  void saveHelper(PhysFileSaver &sv, INode *node, const Matrix3 &body_wtm)
  {
    if (!node)
      return;

    TSTR name;
    if (getNodePropStr(node, "link", name))
    {
      if (name.Length() == 0)
        name = node->GetName();

      sv.beginBlock();
      sv.write(MAKE4C('H', 't', 'm', ' '));

      std::string s = wideToStr(name);
      sv.writeString(s.c_str());

      Matrix3 tm = getScaledNodeTm(node) * Inverse(body_wtm);
      sv.write(tm.GetAddr(), 4 * 3 * 4);

      sv.endBlock();
    }
  }


  void saveCollision(PhysFileSaver &sv, INode *node, const Matrix3 &body_wtm)
  {
    if (!node)
      return;

    if (!getNodePropBool(node, "isCollision", false))
      return;

    Object *obj = node->GetObjectRef();
    if (!obj || obj->SuperClassID() != GEOMOBJECT_CLASS_ID)
    {
      explogError(_T("Invalid collision object '%s'\r\n"), node->GetName());
      return;
    }

    Class_ID cid = obj->ClassID();

    IParamBlock *pblock = (IParamBlock *)obj->GetReference(0);

    Point3 pt0(0, 0, 0), pt1(0, 0, 0);
    Point3 size(0, 0, 0);
    float radius = 0;

    int type = 0;

    if (cid == Class_ID(SPHERE_CLASS_ID, 0))
    {
      // sphere
      type = COLL_ID_SPHERE;

      pt0 = Point3(0, 0, 0);
      radius = 0;

      pblock->GetValue(0, curTime, radius, FOREVER);

      if (radius < 0)
        radius = 0;

      int recenter = 0;
      pblock->GetValue(5, curTime, recenter, FOREVER);

      if (recenter)
        pt0.z += radius;
    }
    else if (cid == Class_ID(BOXOBJ_CLASS_ID, 0))
    {
      // box
      type = COLL_ID_BOX;

      pblock->GetValue(0, curTime, size.y, FOREVER);
      pblock->GetValue(1, curTime, size.x, FOREVER);
      pblock->GetValue(2, curTime, size.z, FOREVER);

      pt0.z = size.z / 2;
    }
    else if (cid == CAPSULE_CLASS_ID)
    {
      // capsule
      type = COLL_ID_CAPSULE;

      pt0 = Point3(0, 0, 0);
      pt1 = Point3(0, 0, 0);
      radius = 0;

      float ht = 0;
      int centers = 0;

      pblock->GetValue(0, curTime, radius, FOREVER);

      if (radius < 0)
        radius = 0;

      pblock->GetValue(1, curTime, ht, FOREVER);
      pblock->GetValue(2, curTime, centers, FOREVER);

      if (centers)
        ht += (ht < 0 ? -radius : +radius) * 2;

      if (fabsf(ht) < radius * 2)
        ht = (ht < 0 ? -radius : +radius) * 2;

      pt1.z = ht;
    }
    /*
    else if (cid==Class_ID(CYLINDER_CLASS_ID, 0))
    {
      // cylinder
      type=COLL_ID_CYLINDER;

      pt0=Point3(0, 0, 0);
      pt1=Point3(0, 0, 0);
      radius=0;
      pblock->GetValue(0, curTime, radius, FOREVER);
      pblock->GetValue(1, curTime, pt1.z, FOREVER);

      if (radius<0) radius=0;
    }
    */
    else
    {
      explog(_T("Unknown collision geometry type '%s'\r\n"), node->GetName());
      return;
    }


    Matrix3 objWtm = getScaledObjectTm(node);
    Matrix3 tm = objWtm * Inverse(body_wtm);

    // save collision object
    sv.beginBlock();
    sv.write(type);

    if (type == COLL_ID_SPHERE)
    {
      pt0 = pt0 * tm;
      radius *= getAverageScale(tm);

      sv.write(pt0);
      sv.write(radius);
    }
    else if (type == COLL_ID_BOX)
    {
      pt0 = pt0 * tm;

      Matrix3 boxTm;
      Point3 v;
      float l;

      boxTm.SetRow(3, pt0);

      v = tm.GetRow(0);
      l = Length(v);
      if (size.x < 0)
        l = -l;
      boxTm.SetRow(0, v / l);
      size.x *= l;

      v = tm.GetRow(1);
      l = Length(v);
      if (size.y < 0)
        l = -l;
      boxTm.SetRow(1, v / l);
      size.y *= l;

      v = tm.GetRow(2);
      l = Length(v);
      if (size.z < 0)
        l = -l;
      boxTm.SetRow(2, v / l);
      size.z *= l;

      //== check for orthogonality

      if (boxTm.Parity())
        boxTm.SetRow(2, -boxTm.GetRow(2));

      sv.write(boxTm.GetAddr(), 4 * 3 * 4);
      sv.write(size);
    }
    else if (type == COLL_ID_CAPSULE)
    {
      pt0 = pt0 * tm;
      pt1 = pt1 * tm;
      radius *= getAverageScale(tm);

      sv.write((pt0 + pt1) / 2);
      sv.write((pt1 - pt0) / 2);
      sv.write(radius);
    }
    else
      assert(false);

    sv.endBlock();
  }


  void saveBody(PhysFileSaver &sv, int body_id)
  {
    INode *node = bodies[body_id];

    sv.beginBlock();
    sv.write(MAKE4C('b', 'o', 'd', 'y'));

    std::string s = wideToStr(node->GetName());
    sv.writeString(s.c_str());

    Matrix3 wtm = getScaledNodeTm(node);
    makeMatrixRightOrthonormal(wtm);
    sv.write(wtm.GetAddr(), 4 * 3 * 4);

    sv.write(getNodePropReal(node, "mass", 0));
    sv.write(getNodePropPoint3(node, "momj", Point3(0, 0, 0)));

    // save collision primitives
    int i;

    int numChildren = node->NumberOfChildren();
    for (i = 0; i < numChildren; ++i)
      saveCollision(sv, node->GetChildNode(i), wtm);

    // save tm helpers
    saveHelper(sv, node, wtm);
    for (i = 0; i < numChildren; ++i)
      saveHelper(sv, node->GetChildNode(i), wtm);

    sv.endBlock();
  }


  void save(FILE *file, Interface *ip)
  {
    curTime = ip->GetTime();

    PhysFileSaver sv(file);

    sv.write(CID_DagorPhysicsResource);
    sv.beginBlock();

    int i;

    // save rigid bodies
    for (i = 0; i < bodies.Count(); ++i)
      saveBody(sv, i);

    //== save more physics stuff

    sv.endBlock();
  }


  struct MomjNode
  {
    Point3 cmPos;
    float mass;
    Matrix3 momj;
  };


  static inline Matrix3 diagonalMatrix3(const Point3 &d)
  {
    Matrix3 tm(TRUE);
    tm.Scale(d, FALSE);
    return tm;
  }


  bool calcNodeMomj(INode *node, MomjNode &mj)
  {
    if (!node)
      return false;

    if (!getNodePropBool(node, "isMassObject", false))
      return false;

    mj.mass = getNodePropReal(node, "mass", 0);

    if (mj.mass == 0)
      return false;

    Object *obj = node->GetObjectRef();
    if (!obj || obj->SuperClassID() != GEOMOBJECT_CLASS_ID)
    {
      explogError(_T("Invalid mass object '%s'\r\n"), node->GetName());
      return false;
    }

    Class_ID cid = obj->ClassID();

    IParamBlock *pblock = (IParamBlock *)obj->GetReference(0);

    Point3 pt0(0, 0, 0), pt1(0, 0, 0);
    Point3 size(0, 0, 0);
    float radius = 0;

    int type = 0;

    Matrix3 objWtm = getScaledObjectTm(node);

    if (cid == Class_ID(SPHERE_CLASS_ID, 0))
    {
      // sphere
      pt0 = Point3(0, 0, 0);
      radius = 0;

      pblock->GetValue(0, curTime, radius, FOREVER);

      if (radius < 0)
        radius = 0;

      int recenter = 0;
      pblock->GetValue(5, curTime, recenter, FOREVER);

      if (recenter)
        pt0.z += radius;

      pt0 = pt0 * objWtm;
      radius *= getAverageScale(objWtm);

      mj.cmPos = pt0;

      float momj = 0.4f * mj.mass * radius * radius;

      mj.momj = diagonalMatrix3(Point3(1, 1, 1) * momj);
    }
    else if (cid == Class_ID(BOXOBJ_CLASS_ID, 0))
    {
      // box
      pblock->GetValue(0, curTime, size.y, FOREVER);
      pblock->GetValue(1, curTime, size.x, FOREVER);
      pblock->GetValue(2, curTime, size.z, FOREVER);

      pt0.z = size.z / 2;

      pt0 = pt0 * objWtm;

      Matrix3 boxTm(TRUE);
      Point3 v;
      float l;

      mj.cmPos = pt0;

      v = objWtm.GetRow(0);
      l = Length(v);
      if (size.x < 0)
        l = -l;
      boxTm.SetRow(0, v / l);
      size.x *= l;

      v = objWtm.GetRow(1);
      l = Length(v);
      if (size.y < 0)
        l = -l;
      boxTm.SetRow(1, v / l);
      size.y *= l;

      v = objWtm.GetRow(2);
      l = Length(v);
      if (size.z < 0)
        l = -l;
      boxTm.SetRow(2, v / l);
      size.z *= l;

      //== check for orthogonality

      if (boxTm.Parity())
        boxTm.SetRow(2, -boxTm.GetRow(2));

      mj.momj = Inverse(boxTm) *
                diagonalMatrix3(
                  Point3(size.y * size.y + size.z * size.z, size.x * size.x + size.z * size.z, size.y * size.y + size.x * size.x) *
                  (mj.mass * (1.0f / 12))) *
                boxTm;
    }
    else if (cid == CAPSULE_CLASS_ID)
    {
      // capsule
      pt0 = Point3(0, 0, 0);
      pt1 = Point3(0, 0, 0);
      radius = 0;

      float ht = 0;
      int centers = 0;

      pblock->GetValue(0, curTime, radius, FOREVER);

      if (radius < 0)
        radius = 0;

      pblock->GetValue(1, curTime, ht, FOREVER);
      pblock->GetValue(2, curTime, centers, FOREVER);

      if (centers)
        ht += (ht < 0 ? -radius : +radius) * 2;

      if (fabsf(ht) < radius * 2)
        ht = (ht < 0 ? -radius : +radius) * 2;

      pt1.z = ht;

      pt0 = pt0 * objWtm;
      pt1 = pt1 * objWtm;
      radius *= getAverageScale(objWtm);

      Matrix3 tm = objWtm;
      tm *= 1 / getAverageScale(objWtm);
      tm.SetTrans(Point3(0, 0, 0));

      mj.cmPos = (pt0 + pt1) / 2;

      float r = radius;
      float h = Length(pt1 - pt0);

      float H = h - 2 * r;
      if (H < 0)
        H = 0;

      float sideJ = (32 * r * r * r + 45 * H * r * r + 20 * r * H * H + 5 * H * H * H) / (80 * r + 60 * H);

      // momj=blk.getPoint3("momj",
      //   Point3(r*r*(16*r+15*H)/(40*r+30*H), sideJ, sideJ)*mass);
      mj.momj = Inverse(tm) * diagonalMatrix3(Point3(sideJ, sideJ, r * r * (16 * r + 15 * H) / (40 * r + 30 * H)) * mj.mass) * tm;
    }
    /*
    else if (cid==Class_ID(CYLINDER_CLASS_ID, 0))
    {
      // cylinder
      type=COLL_ID_CYLINDER;

      pt0=Point3(0, 0, 0);
      pt1=Point3(0, 0, 0);
      radius=0;
      pblock->GetValue(0, curTime, radius, FOREVER);
      pblock->GetValue(1, curTime, pt1.z, FOREVER);

      if (radius<0) radius=0;
    }
    */
    else
    {
      explog(_T("Unknown mass geometry type for '%s'\r\n"), node->GetName());
      return false;
    }

    return true;
  }


  void calcBodyMomj(int body_id)
  {
    INode *node = bodies[body_id];

    Tab<MomjNode> objs;

    int i;

    int numChildren = node->NumberOfChildren();
    for (i = 0; i < numChildren; ++i)
    {
      MomjNode mn;
      if (calcNodeMomj(node->GetChildNode(i), mn))
        objs.Append(1, &mn);
    }

    if (!objs.Count())
      return;

    // find center of mass
    DPoint3 centerOfMass(0, 0, 0);
    double mass = 0;

    for (i = 0; i < objs.Count(); ++i)
    {
      MomjNode &o = objs[i];

      mass += o.mass;
      centerOfMass += o.cmPos * o.mass;
    }

    centerOfMass /= mass;
    Point3 cm(centerOfMass.x, centerOfMass.y, centerOfMass.z);

    if (mass < 0)
      explogWarning(_T("body '%s' has negative mass!\r\n"), node->GetName());

    // calc total momj
    Matrix3 momj;
    momj.Zero();

    for (i = 0; i < objs.Count(); ++i)
    {
      MomjNode &o = objs[i];

      momj += o.momj;

      DPoint3 r = DPoint3(o.cmPos) - cm;

      momj.GetAddr()[0][0] += o.mass * (r.y * r.y + r.z * r.z);
      momj.GetAddr()[1][1] += o.mass * (r.z * r.z + r.x * r.x);
      momj.GetAddr()[2][2] += o.mass * (r.x * r.x + r.y * r.y);

      momj.GetAddr()[0][1] -= o.mass * r.x * r.y;
      momj.GetAddr()[1][0] -= o.mass * r.x * r.y;
      momj.GetAddr()[0][2] -= o.mass * r.x * r.z;
      momj.GetAddr()[2][0] -= o.mass * r.x * r.z;
      momj.GetAddr()[1][2] -= o.mass * r.y * r.z;
      momj.GetAddr()[2][1] -= o.mass * r.y * r.z;
    }

    momj.ValidateFlags();

    /*
    MRow *m=momj.GetAddr();
    explog("'%s' momj:\r\n%f\t%f\t%f\r\n%f\t%f\t%f\r\n%f\t%f\t%f\r\n",
      (const char*)node->GetName(),
      m[0][0], m[0][1], m[0][2],
      m[1][0], m[1][1], m[1][2],
      m[2][0], m[2][1], m[2][2]
    );
    explog("mass=%f\r\n", mass);
    //*/

    // find inertia ellipsoid axes

    // find eigenvalues
    Point3 dmomj;

    {
      MRow *m = momj.GetAddr();

      double Mxx = m[0][0];
      double Myy = m[1][1];
      double Mzz = m[2][2];
      double Mxy = m[0][1];
      double Myz = m[1][2];
      double Mzx = m[2][0];

      double a = -1;
      double b = Mxx + Myy + Mzz;
      double c = Mxy * Mxy + Myz * Myz + Mzx * Mzx - Mxx * Myy - Myy * Mzz - Mzz * Mxx;
      double d = Mxx * Myy * Mzz + 2 * Mxy * Myz * Mzx - Mxx * Myz * Myz - Myy * Mzx * Mzx - Mzz * Mxy * Mxy;

      double f = c / a - b * b / (a * a * 3);
      double g = 2 * b * b * b / (a * a * a * 27) - b * c / (a * a) / 3 + d / a;
      double h = g * g / 4 + f * f * f / 27;

      // explog("  h=%g  f=%g  g=%g\r\n", h, f, g);

      /*
      if (h>0)
      {
        // single real root
        double R=-g/2+sqrt(h);
        double S=cubic_root(R);
        //explog("  R=%g S=%g\r\n", R, S);
        double T=-g/2-sqrt(h);
        double U=cubic_root(T);
        //explog("  T=%g U=%g\r\n", T, U);

        double x1=S+U-b/(3*a);

        dmomj=Point3(x1, x1, x1);
      }
      //else if (f==0 && g==0)
      else
      */
      if (f >= 0)
      {
        // 3 equal real roots
        double x1 = cubic_root(-d / a);

        dmomj = Point3(x1, x1, x1);
      }
      else
      {
        // 3 real roots
        double i = sqrt(-f * f * f / 27);
        double j = cubic_root(i);

        double k = -g / (2 * i);
        if (k >= 1)
          k = 0;
        else if (k <= -1)
          k = PI;
        else
          k = acos(k);

        double L = -j;
        double M = cos(k / 3);
        double N = sqrt(3.0f) * sin(k / 3);
        double P = -b / (3 * a);

        double x1 = 2 * j * M + P;
        double x2 = L * (M + N) + P;
        double x3 = L * (M - N) + P;

        dmomj = Point3(x1, x2, x3);
      }
    }

    // explog("dmomj=(%f  %f  %f)\r\n", dmomj.x, dmomj.y, dmomj.z);

    // find eigenvectors
    Matrix3 dmj(TRUE);

    for (i = 0; i < 3; ++i)
    {
      Point3 sm[3];
      sm[0] = momj.GetRow(0);
      sm[1] = momj.GetRow(1);
      sm[2] = momj.GetRow(2);

      sm[0][0] -= dmomj[i];
      sm[1][1] -= dmomj[i];
      sm[2][2] -= dmomj[i];

      sm[0].Normalize();
      sm[1].Normalize();
      sm[2].Normalize();

      const float EPS = 0.001f;

      if (LengthSquared(sm[0]) <= EPS)
      {
        if (LengthSquared(sm[2]) <= EPS)
        {
          Point3 p = sm[0];
          sm[0] = sm[1];
          sm[1] = p;
        }
        else
        {
          Point3 p = sm[0];
          sm[0] = sm[2];
          sm[2] = p;
        }
      }
      else if (LengthSquared(sm[1]) <= EPS)
      {
        Point3 p = sm[1];
        sm[1] = sm[2];
        sm[2] = p;
      }

      Point3 v;

      /*
      explog("  sm[0]=(%f  %f  %f)\r\n", sm[0].x, sm[0].y, sm[0].z);
      explog("  sm[1]=(%f  %f  %f)\r\n", sm[1].x, sm[1].y, sm[1].z);
      explog("  sm[2]=(%f  %f  %f)\r\n", sm[2].x, sm[2].y, sm[2].z);
      //*/

      if (LengthSquared(sm[0]) <= EPS)
      {
        // matrix is zero - any vector is ok
        v = Point3(0, 0, 0);
        v[i] = 1;
      }
      else if (LengthSquared(sm[1]) <= EPS)
      {
        // two zero rows - any vector orthogonal to non-zero row
        v = sm[0] ^ Point3(1, 0, 0);
        if (Length(v) < 0.1f)
          v = Normalize(sm[0] ^ Point3(0, 1, 0));
        else
          v = Normalize(v);
      }
      else
      {
        v = sm[0] ^ sm[1];
        if (LengthSquared(v) <= EPS)
        {
          // sm[0] and sm[1] are collinear

          if (LengthSquared(sm[2]) <= EPS)
          {
            // any vector orthogonal to non-zero row
            v = sm[0] ^ Point3(1, 0, 0);
            if (Length(v) < 0.1f)
              v = Normalize(sm[0] ^ Point3(0, 1, 0));
            else
              v = Normalize(v);
          }
          else
          {
            v = sm[0] ^ sm[2];

            if (LengthSquared(v) <= EPS)
            {
              // all rows are collinear - any vector orthogonal to non-zero row
              v = sm[0] ^ Point3(1, 0, 0);
              if (Length(v) < 0.1f)
                v = Normalize(sm[0] ^ Point3(0, 1, 0));
              else
                v = Normalize(v);
            }
            else
              v = Normalize(v);
          }
        }
        else
          v = Normalize(v);
      }

      dmj.SetRow(i, v);
    }

    /*
    m=dmj.GetAddr();
    explog("'%s' dmj:\r\n%f\t%f\t%f\r\n%f\t%f\t%f\r\n%f\t%f\t%f\r\n",
      (const char*)node->GetName(),
      m[0][0], m[0][1], m[0][2],
      m[1][0], m[1][1], m[1][2],
      m[2][0], m[2][1], m[2][2]
    );
    //*/

    // orthogonalize dmj
    int mi = 0;
    float mv = LengthSquared(dmj.GetRow(1) ^ dmj.GetRow(2));

    for (i = 1; i < 3; ++i)
    {
      float v = LengthSquared(dmj.GetRow((i + 1) % 3) ^ dmj.GetRow((i + 2) % 3));
      if (v < mv)
      {
        mv = v;
        mi = i;
      }
    }

    if (mv < 0.9f)
    {
      Point3 a = dmj.GetRow(mi);
      Point3 v = a ^ Point3(1, 0, 0);
      if (LengthSquared(v) < 0.1f)
        v = Normalize(a ^ Point3(0, 1, 0));
      else
        v = Normalize(v);
      dmj.SetRow((mi + 1) % 3, v);
      dmj.SetRow((mi + 2) % 3, a ^ v);
    }

    /*
    m=dmj.GetAddr();
    explog("'%s' dmj:\r\n%f\t%f\t%f\r\n%f\t%f\t%f\r\n%f\t%f\t%f\r\n",
      (const char*)node->GetName(),
      m[0][0], m[0][1], m[0][2],
      m[1][0], m[1][1], m[1][2],
      m[2][0], m[2][1], m[2][2]
    );

    momj=dmj*momj*Inverse(dmj);

    m=momj.GetAddr();
    explog("'%s' new momj:\r\n%f\t%f\t%f\r\n%f\t%f\t%f\r\n%f\t%f\t%f\r\n",
      (const char*)node->GetName(),
      m[0][0], m[0][1], m[0][2],
      m[1][0], m[1][1], m[1][2],
      m[2][0], m[2][1], m[2][2]
    );
    //*/

    // set node tm and props
    setNodePropReal(node, "mass", mass);
    setNodePropPoint3(node, "momj", dmomj);

    Matrix3 newNodeWtm = dmj;
#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
    newNodeWtm.SetTrans(cm / (float)GetSystemUnitScale(UNITS_METERS));
#else
    newNodeWtm.SetTrans(cm / (float)GetMasterScale(UNITS_METERS));
#endif
    adjWtm(newNodeWtm);

    Matrix3 deltaTm = Inverse(newNodeWtm) * node->GetNodeTM(curTime);

    node->SetNodeTM(curTime, newNodeWtm);
    node->InvalidateWS();

    for (i = 0; i < numChildren; ++i)
    {
      INode *n = node->GetChildNode(i);
      if (!n)
        continue;
      n->SetNodeTM(curTime, n->GetNodeTM(curTime) * deltaTm);
      n->InvalidateWS();
    }
  }


  void calcMomjs(Interface *ip)
  {
    curTime = ip->GetTime();

    int i;

    for (i = 0; i < bodies.Count(); ++i)
      calcBodyMomj(i);
  }
};


void export_physics(FILE *file, Interface *ip)
{
  PhysExportENodeCB cb;

  enum_nodes(ip->GetRootNode(), &cb);

  cb.save(file, ip);
}


void calc_momjs(Interface *ip)
{
  PhysExportENodeCB cb;

  enum_nodes(ip->GetRootNode(), &cb);

  int holding = theHold.Holding();
  if (!holding)
    theHold.Begin();

  cb.calcMomjs(ip);

  if (!holding)
    theHold.Accept(_T("calc momjs"));

  ip->RedrawViews(ip->GetTime());
}
