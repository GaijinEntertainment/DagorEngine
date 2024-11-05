// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include <plugapi.h>
#include <ilayermanager.h>
#include <ilayer.h>
#include <modstack.h>
#include <iparamb2.h>
#include <iskin.h>
#include <bmmlib.h>
#include <stdmat.h>
#include <splshape.h>
#include <notetrck.h>
#include <meshnormalspec.h>
#include "dagor.h"
#include "dagfmt.h"
#include "mater.h"
#include "resource.h"
#include "debug.h"
#include <io.h>

#define ERRMSG_DELAY 5000

struct SkinData
{
  int numb, numvert;
  Tab<DagBone> bones;
  Tab<float> wt;
  INode *skinNode;
  SkinData() : numb(0), numvert(0), skinNode(NULL) {}
};
struct NodeId
{
  int id;
  INode *node;
  NodeId(int _id, INode *n) : id(_id), node(n) {}
};

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

extern void convertnew(Interface *ip, bool on_import);
extern void collapse_materials(Interface *ip);

TCHAR *ReadCharString(int len, FILE *h)
{
  char *s = (char *)malloc(len + 1);
  assert(s);
  if (len)
  {
    if (fread(s, len, 1, h) != 1)
    {
      free(s);
      return NULL;
    }
  }
  s[len] = 0;
#ifdef _UNICODE
  TCHAR *sw = (TCHAR *)malloc(sizeof(TCHAR) * (len + 1));
  mbstowcs(sw, s, len + 1);
  free(s);
  return sw;
#else
  return s;
#endif
}


struct Block
{
  int ofs, len;
  int t;
};

static Tab<Block> blk;
static FILE *fileh = NULL;

static void init_blk(FILE *h)
{
  blk.SetCount(0);
  fileh = h;
}

static int begin_blk()
{
  Block b;
  if (fread(&b.len, 4, 1, fileh) != 1)
    return 0;
  if (fread(&b.t, 4, 1, fileh) != 1)
    return 0;
  b.len -= 4;
  b.ofs = ftell(fileh);
  int n = blk.Count();
  blk.Append(1, &b);
  if (blk.Count() != n + 1)
    return 0;
  return 1;
}

static int end_blk()
{
  int i = blk.Count() - 1;
  if (i < 0)
    return 0;
  fseek(fileh, blk[i].ofs + blk[i].len, SEEK_SET);
  blk.Delete(i, 1);
  return 1;
}

static int blk_type()
{
  int i = blk.Count() - 1;
  if (i < 0)
  {
    assert(0);
    return 0;
  }
  return blk[i].t;
}

static int blk_len()
{
  int i = blk.Count() - 1;
  if (i < 0)
  {
    assert(0);
    return 0;
  }
  return blk[i].len;
}

static int blk_rest()
{
  int i = blk.Count() - 1;
  if (i < 0)
  {
    assert(0);
    return 0;
  }
  return blk[i].ofs + blk[i].len - ftell(fileh);
}

class DagImp : public SceneImport
{
public:
  DagImp() {}
  ~DagImp() {}

  int ExtCount() { return 1; }
  const TCHAR *Ext(int n) { return _T("dag"); }

  const TCHAR *LongDesc() { return GetString(IDS_DAGIMP_LONG); }
  const TCHAR *ShortDesc() { return GetString(IDS_DAGIMP_SHORT); }
  const TCHAR *AuthorName() { return GetString(IDS_AUTHOR); }
  const TCHAR *CopyrightMessage() { return GetString(IDS_COPYRIGHT); }
  const TCHAR *OtherMessage1() { return _T(""); }
  const TCHAR *OtherMessage2() { return _T(""); }

  unsigned int Version() { return 1; }

  void ShowAbout(HWND hWnd) {}

  int DoImport(const TCHAR *name, ImpInterface *ii, Interface *i, BOOL suppressPrompts = FALSE);
  int doImportOne(const TCHAR *name, ImpInterface *ii, Interface *i, BOOL suppressPrompts);

  static bool separateLayers; // Only used when suppressPrompts is set.
};

class DagImpCD : public ClassDesc
{
public:
  int IsPublic() { return TRUE; }
  void *Create(BOOL loading) { return new DagImp; }
  const TCHAR *ClassName() { return GetString(IDS_DAGIMP); }
  const MCHAR *NonLocalizedClassName() { return ClassName(); }
  SClass_ID SuperClassID() { return SCENE_IMPORT_CLASS_ID; }
  Class_ID ClassID() { return DAGIMP_CID; }
  const TCHAR *Category() { return _T(""); }

  const TCHAR *InternalName() { return _T("DagImporter"); }
  HINSTANCE HInstance() { return hInstance; }
};
static DagImpCD dagimpcd;

ClassDesc *GetDAGIMPCD() { return &dagimpcd; }

//===============================================================================//

struct ImpMat
{
  DagMater m;
  TCHAR *name;
  TCHAR *clsname;
  TCHAR *script;
  Mtl *mtl;
  ImpMat()
  {
    name = clsname = script = NULL;
    mtl = NULL;
  }
};

struct Impnode
{
  ImpNode *in;
  INode *n;
  //      ushort pid;
  Mtl *mtl;
};

static Tab<TCHAR *> tex;
static Tab<ImpMat> mat;
// static Tab<Impnode> node;
static Tab<TCHAR *> keylabel;
static Tab<DefNoteTrack *> ntrack;
// static Tab<DagNoteKey*> ntrack;
// static Tab<int> ntrack_knum;

static TSTR scene_name = _T("");

static void cleanup(int err = 0)
{
  int i;
  for (i = 0; i < tex.Count(); ++i)
    if (tex[i])
      free(tex[i]);
  tex.ZeroCount();
  for (i = 0; i < mat.Count(); ++i)
  {
    if (mat[i].name)
      free(mat[i].name);
    if (mat[i].clsname)
      free(mat[i].clsname);
    if (mat[i].script)
      free(mat[i].script);
  }
  if (err)
  {
    for (i = 0; i < mat.Count(); ++i)
      if (mat[i].mtl)
        mat[i].mtl->DeleteThis();
    /*              for(i=0;i<node.Count();++i) if(node[i].mtl) {
                            for(int j=0;j<mat.Count();++j) if(mat[j].mtl==node[i].mtl) break;
                            if(j>=mat.Count()) node[i].mtl->DeleteThis();
                    }
    */
  }
  mat.ZeroCount();
  //      node.ZeroCount();
  for (i = 0; i < keylabel.Count(); ++i)
    if (keylabel[i])
      free(keylabel[i]);
  keylabel.ZeroCount();
  // for(i=0;i<ntrack.Count();++i) if(ntrack[i]) free(ntrack[i]);
  for (i = 0; i < ntrack.Count(); ++i)
    if (ntrack[i])
      ntrack[i]->DeleteThis();
  ntrack.ZeroCount();
  // ntrack_knum.ZeroCount();
}

static void make_mtl(ImpMat &m, int ind)
{
  m.mtl = (Mtl *)CreateInstance(MATERIAL_CLASS_ID, DagorMat_CID);
  assert(m.mtl);
  IDagorMat *d = (IDagorMat *)m.mtl->GetInterface(I_DAGORMAT);
  assert(d);
  d->set_amb(m.m.amb);
  d->set_diff(m.m.diff);
  d->set_emis(m.m.emis);
  d->set_spec((m.m.power == 0.0f) ? Color(0, 0, 0) : Color(m.m.spec));
  d->set_power(m.m.power);
  d->set_classname(m.clsname);
  d->set_script(m.script);
  d->set_2sided((m.m.flags & DAG_MF_2SIDED) ? TRUE : FALSE);
  char noname = 0;
  if (!m.name)
    noname = 1;
  else if (m.name[0] == 0)
    noname = 1;
  if (!noname)
    m.mtl->SetName(m.name);
  for (int i = 0; i < DAGTEXNUM; ++i)
  {
    TCHAR *s = NULL;
    if (m.m.texid[i] < tex.Count() && (i < 8 || (m.m.flags & DAG_MF_16TEX)))
      s = tex[m.m.texid[i]];
    d->set_texname(i, s);
    if (i == 0 && noname)
    {
      if (s)
      {
        TSTR nm;
        SplitPathFile(TSTR(s), NULL, &nm);
        m.mtl->SetName(nm);
      }
      else
      {
        TSTR nm;
        nm.printf(_T("%s #%02d"), scene_name, ind);
        m.mtl->SetName(nm);
      }
    }
    d->set_param(i, 0);
  }
  m.mtl->ReleaseInterface(I_DAGORMAT, d);
}

static void make_mtl2(ImpMat &m, int ind)
{
  m.mtl = (Mtl *)CreateInstance(MATERIAL_CLASS_ID, DagorMat2_CID);
  assert(m.mtl);
  IDagorMat *d = (IDagorMat *)m.mtl->GetInterface(I_DAGORMAT);
  assert(d);
  d->set_amb(m.m.amb);
  d->set_diff(m.m.diff);
  d->set_emis(m.m.emis);
  d->set_spec((m.m.power == 0.0f) ? Color(0, 0, 0) : Color(m.m.spec));
  d->set_power(m.m.power);
  d->set_classname(m.clsname);
  d->set_script(m.script);
  d->set_2sided((m.m.flags & DAG_MF_2SIDED) ? TRUE : FALSE);
  char noname = 0;
  if (!m.name)
    noname = 1;
  else if (m.name[0] == 0)
    noname = 1;
  if (!noname)
    m.mtl->SetName(m.name);
  for (int i = 0; i < DAGTEXNUM; ++i)
  {
    TCHAR *s = NULL;
    if (m.m.texid[i] < tex.Count() && (i < 8 || (m.m.flags & DAG_MF_16TEX)))
      s = tex[m.m.texid[i]];
    d->set_texname(i, s);
    if (i == 0 && noname)
    {
      if (s)
      {
        TSTR nm;
        SplitPathFile(TSTR(s), NULL, &nm);
        m.mtl->SetName(nm);
      }
      else
      {
        TSTR nm;
        nm.printf(_T("%s #%02d"), scene_name, ind);
        m.mtl->SetName(nm);
      }
    }
    d->set_param(i, 0);
  }
  m.mtl->ReleaseInterface(I_DAGORMAT, d);
}

static void adj_wtm(Matrix3 &tm)
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

static void adj_pos(Point3 &p)
{
  float a = p.y;
  p.y = p.z;
  p.z = a;
}

static void adj_rot(Quat &q)
{
  float a = q.y;
  q.y = -q.z;
  q.z = -a;
  q.x = -q.x;
  q = Quat(-0.7071067811865476, 0., 0., 0.7071067811865476) * q;
}

static void adj_scl(Point3 &p) { p.z = -p.z; }

#define rd(p, l)                \
  {                             \
    if (fread(p, l, 1, h) != 1) \
      goto read_err;            \
  }
#define bblk          \
  {                   \
    if (!begin_blk()) \
      goto read_err;  \
  }
#define eblk         \
  {                  \
    if (!end_blk())  \
      goto read_err; \
  }

static void flip_normals(Mesh &m)
{
  for (int i = 0; i < m.numFaces; ++i)
    m.FlipNormal(i);
}

static int load_node(INode *pnode, FILE *h, ImpInterface *ii, Interface *ip, Tab<SkinData *> &skin_data, Tab<NodeId> &node_id)
{
  ImpNode *in = NULL;
  INode *n = NULL;
  if (pnode)
  {
    in = ii->CreateNode();
    assert(in);
    ii->AddNodeToScene(in);
    n = in->GetINode();
    assert(n);
    // pnode->AttachChild(n,0);
  }
  else
  {
    n = ip->GetRootNode();
    assert(n);
  }
  ushort nid = ~0;
  Mtl *mtl = NULL;
  Object *obj = NULL;
  int nflg = 0;
  while (blk_rest() > 0)
  {
    bblk;

    DebugPrint(_T("load_node %d\n"), blk_type());

    if (blk_type() == DAG_NODE_DATA)
    {
      DagNodeData d;
      rd(&d, sizeof(d));
      nid = d.id;
      n->SetRenderable(d.flg & DAG_NF_RENDERABLE ? 1 : 0);
      n->SetCastShadows(d.flg & DAG_NF_CASTSHADOW ? 1 : 0);
      n->SetRcvShadows(d.flg & DAG_NF_RCVSHADOW ? 1 : 0);
      NodeId id(nid, n);
      node_id.Append(1, &id);
      nflg = d.flg;
      int l = blk_rest();
      if (l > 0 && in)
      {

        TCHAR *nm = ReadCharString(l, h);
        in->SetName(nm);
        DebugPrint(_T("node <%s>\n"), nm);
        free(nm);
      }
    }
    else if (blk_type() == DAG_NODE_TM && in)
    {
      Matrix3 tm;
      rd(tm.GetAddr(), 4 * 3 * 4);
      tm.SetNotIdent();
      if (pnode->IsRootNode())
        adj_wtm(tm);
      in->SetTransform(0, tm);
      // n->SetNodeTM(0,tm);
    }
    else if (blk_type() == DAG_NODE_ANIM && in)
    {
      int adj = pnode->IsRootNode();
      ushort ntrkid;
      rd(&ntrkid, 2);
      // note track
      if (ntrkid < ntrack.Count())
      {
        DefNoteTrack *nt = ntrack[ntrkid];
        if (nt)
        {
          DefNoteTrack *nnt = (DefNoteTrack *)NewDefaultNoteTrack();
          assert(nnt);
          nnt->keys = nt->keys;
          n->AddNoteTrack(nnt);
        }
      }
      /*
      if(ntrkid<ntrack.Count()) {
          int knum=ntrack_knum[ntrkid];
          DagNoteKey *nk=ntrack[ntrkid];
              DefNoteTrack *nt=(DefNoteTrack*)NewDefaultNoteTrack();
              if(nt) {
              for(int i=0;i<knum;++i) {
                  char *nm="";
                  if(nk[i].id<keylabel.Count()) nm=keylabel[nk[i].id];
                  NoteKey *k=new NoteKey(nk[i].t,nm);
                  nt->keys.Append(1,&k);
              }
              n->AddNoteTrack(nt);
              }
      }
      */
      // create PRS controller
      Control *prs = CreatePRSControl();
      assert(prs);
      n->SetTMController(prs);
      Control *pc = CreateInterpPosition();
      assert(pc);
      verify(prs->SetPositionController(pc));
      Control *rc = CreateInterpRotation();
      assert(rc);
      verify(prs->SetRotationController(rc));
      Control *sc = CreateInterpScale();
      assert(sc);
      verify(prs->SetScaleController(sc));
      // pos track
      IKeyControl *ik = GetKeyControlInterface(pc);
      assert(ik);
      ushort numk;
      rd(&numk, 2);
      if (numk == 1)
      {
        IBezPoint3Key k;
        rd(&k.val, 12);
        if (adj)
          adj_pos(k.val);
        k.intan = Point3(0, 0, 0);
        k.outtan = Point3(0, 0, 0);
        ik->SetNumKeys(1);
        ik->SetKey(0, &k);
      }
      else
      {
        ik->SetNumKeys(numk);
        IBezPoint3Key k;
        k.flags = BEZKEY_XBROKEN | BEZKEY_YBROKEN | BEZKEY_ZBROKEN;
        DagPosKey *dk = new DagPosKey[numk];
        assert(dk);
        rd(dk, sizeof(*dk) * numk);
        for (int i = 0; i < numk; ++i)
        {
          k.time = dk[i].t;
          k.val = dk[i].p;
          float dt;
          if (i > 0)
            dt = (dk[i].t - dk[i - 1].t) / 3.f;
          else
            dt = 1;
          k.intan = dk[i].i / dt;
          if (i + 1 < numk)
            dt = (dk[i + 1].t - dk[i].t) / 3.f;
          else
            dt = 1;
          k.outtan = dk[i].o / dt;
          if (adj)
            adj_pos(k.val);
          if (adj)
            adj_pos(k.intan);
          if (adj)
            adj_pos(k.outtan);
          ik->SetKey(i, &k);
        }
        delete[] dk;
        ik->SortKeys();
      }
      // rot track
      SuspendAnimate();
      AnimateOn();
      ik = GetKeyControlInterface(rc);
      assert(ik);
      rd(&numk, 2);
      if (numk == 1)
      {
        Quat q;
        rd(&q, 16);
        q = Conjugate(q);
        if (adj)
          adj_rot(q);
        rc->SetValue(0, &q);
      }
      else
      {
        DagRotKey *dk = new DagRotKey[numk];
        assert(dk);
        rd(dk, sizeof(*dk) * numk);
        for (int i = 0; i < numk; ++i)
        {
          dk[i].p = Conjugate(dk[i].p);
          if (adj)
            adj_rot(dk[i].p);
          rc->SetValue(dk[i].t, &dk[i].p);
        }
        delete[] dk;
      }
      ResumeAnimate();
      // scl track
      ik = GetKeyControlInterface(sc);
      assert(ik);
      rd(&numk, 2);
      if (numk == 1)
      {
        IBezScaleKey k;
        Point3 p;
        rd(&p, 12);
        if (adj)
          adj_scl(p);
        k.val = p;
        k.intan = Point3(0, 0, 0);
        k.outtan = Point3(0, 0, 0);
        ik->SetNumKeys(1);
        ik->SetKey(0, &k);
      }
      else
      {
        ik->SetNumKeys(numk);
        IBezScaleKey k;
        k.flags = BEZKEY_XBROKEN | BEZKEY_YBROKEN | BEZKEY_ZBROKEN;
        DagPosKey *dk = new DagPosKey[numk];
        assert(dk);
        rd(dk, sizeof(*dk) * numk);
        for (int i = 0; i < numk; ++i)
        {
          k.time = dk[i].t;
          k.val = dk[i].p;
          float dt;
          if (i > 0)
            dt = (dk[i].t - dk[i - 1].t) / 3.f;
          else
            dt = 1;
          k.intan = dk[i].i / dt;
          if (i + 1 < numk)
            dt = (dk[i + 1].t - dk[i].t) / 3.f;
          else
            dt = 1;
          k.outtan = dk[i].o / dt;
          if (adj)
            adj_scl(k.val.s);
          if (adj)
            adj_scl(k.intan);
          if (adj)
            adj_scl(k.outtan);
          ik->SetKey(i, &k);
        }
        delete[] dk;
        ik->SortKeys();
      }
    }
    else if (blk_type() == DAG_NODE_SCRIPT && in)
    {
      DebugPrint(_T("   node script...\n"));
      int l = blk_rest();
      if (l > 0)
      {
        TCHAR *s = ReadCharString(l, h);
        n->SetUserPropBuffer(s);
        free(s);
      }
      DebugPrint(_T("   node script ok\n"));
    }
    else if (blk_type() == DAG_NODE_MATER && in)
    {
      DebugPrint(_T("   node material...\n"));
      int num = blk_len() / 2;
      if (num > 1)
      {
        MultiMtl *mm = NewDefaultMultiMtl();
        assert(mm);
        mm->SetNumSubMtls(num);
        for (int i = 0; i < num; ++i)
        {
          ushort id;
          rd(&id, 2);
          if (id < mat.Count())
          {
            if (!mat[id].mtl)
              make_mtl(mat[id], id);
            if (mat[id].mtl)
              mm->SetSubMtl(i, mat[id].mtl);
          }
        }
        mm->SetName(n->GetName());
        mtl = mm;
      }
      else if (num == 1)
      {
        ushort id;
        rd(&id, 2);
        if (id < mat.Count())
        {
          if (!mat[id].mtl)
            make_mtl(mat[id], id);
          if (mat[id].mtl)
            mtl = mat[id].mtl;
        }
      }
      DebugPrint(_T("   node materials ok\n"));
    }
    else if (blk_type() == DAG_NODE_OBJ && in)
    {
      DebugPrint(_T("   obj mesh...\n"));
      while (blk_rest() > 0)
      {
        bblk;
        if (blk_type() == DAG_OBJ_BIGMESH)
        {
          TriObject *tri = CreateNewTriObject();
          assert(tri);
          Mesh &m = tri->mesh;
          uint nv = 0;
          rd(&nv, 4);

          verify(m.setNumVerts(nv));
          rd(m.verts, nv * 12);
          uint nf = 0;
          rd(&nf, 4);
          verify(m.setNumFaces(nf));

          DebugPrint(_T("   big faces: %d\n"), nf);

          int i;
          for (i = 0; i < nf; ++i)
          {
            DagBigFace f;
            rd(&f, sizeof(f));
            m.faces[i].v[0] = f.v[0];
            m.faces[i].v[2] = f.v[1];
            m.faces[i].v[1] = f.v[2];
            m.faces[i].setMatID(f.mat);
            m.faces[i].smGroup = f.smgr;
            m.faces[i].flags |= EDGE_ALL;
          }
          uchar numch;
          rd(&numch, 1);
#if MAX_RELEASE >= 3000
          for (int ch = 0; ch < numch; ++ch)
          {
            uint ntv = 0;
            rd(&ntv, 4);
            uchar tcsz, chid;
            rd(&tcsz, 1);
            rd(&chid, 1);
            m.setMapSupport(chid);
            m.setNumMapVerts(chid, ntv);
            Point3 *tv = m.mapVerts(chid);
            for (; ntv; --ntv, ++tv)
            {
              for (i = 0; i < tcsz && i < 3; ++i)
                rd(&tv[0][i], 4);
              for (; i < 3; ++i)
                tv[0][i] = 0;
            }
            TVFace *tf = m.mapFaces(chid);
            for (i = 0; i < nf; ++i)
            {
              DagBigTFace f;
              rd(&f, sizeof(f));
              tf[i].t[0] = f.t[0];
              tf[i].t[2] = f.t[1];
              tf[i].t[1] = f.t[2];
            }
          }
#else
          if (numch >= 1)
          {
            uint ntv = 0;
            rd(&ntv, 4);
            uchar tcsz, chid;
            rd(&tcsz, 1);
            rd(&chid, 1);
            verify(m.setNumTVFaces(nf));
            verify(m.setNumTVerts(ntv));
            Point3 *tv = m.tVerts;
            for (; ntv; --ntv, ++tv)
            {
              for (i = 0; i < tcsz && i < 3; ++i)
                rd((float *)*tv + i, 4);
              for (; i < 3; ++i)
                ((float *)*tv)[i] = 0;
            }
            TVFace *tf = m.tvFace;
            for (i = 0; i < nf; ++i)
            {
              DagBigTFace f;
              rd(&f, sizeof(f));
              tf[i].t[0] = f.t[0];
              tf[i].t[2] = f.t[1];
              tf[i].t[1] = f.t[2];
            }
          }
          if (numch >= 2)
          {
            uint ntv = 0;
            rd(&ntv, 4);
            uchar tcsz, chid;
            rd(&tcsz, 1);
            rd(&chid, 1);
            verify(m.setNumVCFaces(nf));
            verify(m.setNumVertCol(ntv));
            Point3 *tv = m.vertCol;
            for (; ntv; --ntv, ++tv)
            {
              for (i = 0; i < tcsz && i < 3; ++i)
                rd((float *)*tv + i, 4);
              for (; i < 3; ++i)
                ((float *)*tv)[i] = 0;
            }
            TVFace *tf = m.vcFace;
            for (i = 0; i < nf; ++i)
            {
              DagBigTFace f;
              rd(&f, sizeof(f));
              tf[i].t[0] = f.t[0];
              tf[i].t[2] = f.t[1];
              tf[i].t[1] = f.t[2];
            }
          }
#endif
          if (n->GetNodeTM(0).Parity())
            flip_normals(m);
          obj = tri;
        }
        else if (blk_type() == DAG_OBJ_MESH)
        {
          TriObject *tri = CreateNewTriObject();
          assert(tri);
          Mesh &m = tri->mesh;
          uint nv = 0;
          rd(&nv, 2);
          verify(m.setNumVerts(nv));
          if (nv)
            rd(m.verts, nv * 12);
          uint nf = 0;
          rd(&nf, 2);
          verify(m.setNumFaces(nf));
          DebugPrint(_T("   faces: %d\n"), nf);
          int i;
          for (i = 0; i < nf; ++i)
          {
            DagFace f;
            rd(&f, sizeof(f));
            m.faces[i].v[0] = f.v[0];
            m.faces[i].v[2] = f.v[1];
            m.faces[i].v[1] = f.v[2];
            m.faces[i].setMatID(f.mat);
            m.faces[i].smGroup = f.smgr;
            m.faces[i].flags |= EDGE_ALL;
          }
          uchar numch;
          rd(&numch, 1);
#if MAX_RELEASE >= 3000
          for (int ch = 0; ch < numch; ++ch)
          {
            uint ntv = 0;
            rd(&ntv, 2);
            uchar tcsz, chid;
            rd(&tcsz, 1);
            rd(&chid, 1);
            m.setMapSupport(chid);
            m.setNumMapVerts(chid, ntv);
            Point3 *tv = m.mapVerts(chid);
            for (; ntv; --ntv, ++tv)
            {
              for (i = 0; i < tcsz && i < 3; ++i)
                rd(&tv[0][i], 4);
              for (; i < 3; ++i)
                tv[0][i] = 0;
            }
            TVFace *tf = m.mapFaces(chid);
            for (i = 0; i < nf; ++i)
            {
              DagTFace f;
              rd(&f, sizeof(f));
              tf[i].t[0] = f.t[0];
              tf[i].t[2] = f.t[1];
              tf[i].t[1] = f.t[2];
            }
          }
#else
          if (numch >= 1)
          {
            uint ntv = 0;
            rd(&ntv, 2);
            uchar tcsz, chid;
            rd(&tcsz, 1);
            rd(&chid, 1);
            verify(m.setNumTVFaces(nf));
            verify(m.setNumTVerts(ntv));
            Point3 *tv = m.tVerts;
            for (; ntv; --ntv, ++tv)
            {
              for (i = 0; i < tcsz && i < 3; ++i)
                rd((float *)*tv + i, 4);
              for (; i < 3; ++i)
                ((float *)*tv)[i] = 0;
            }
            TVFace *tf = m.tvFace;
            for (i = 0; i < nf; ++i)
            {
              DagTFace f;
              rd(&f, sizeof(f));
              tf[i].t[0] = f.t[0];
              tf[i].t[2] = f.t[1];
              tf[i].t[1] = f.t[2];
            }
          }
          if (numch >= 2)
          {
            uint ntv = 0;
            rd(&ntv, 2);
            uchar tcsz, chid;
            rd(&tcsz, 1);
            rd(&chid, 1);
            verify(m.setNumVCFaces(nf));
            verify(m.setNumVertCol(ntv));
            Point3 *tv = m.vertCol;
            for (; ntv; --ntv, ++tv)
            {
              for (i = 0; i < tcsz && i < 3; ++i)
                rd((float *)*tv + i, 4);
              for (; i < 3; ++i)
                ((float *)*tv)[i] = 0;
            }
            TVFace *tf = m.vcFace;
            for (i = 0; i < nf; ++i)
            {
              DagTFace f;
              rd(&f, sizeof(f));
              tf[i].t[0] = f.t[0];
              tf[i].t[2] = f.t[1];
              tf[i].t[1] = f.t[2];
            }
          }
#endif
          if (n->GetNodeTM(0).Parity())
            flip_normals(m);
          obj = tri;
#if MAX_RELEASE >= 12000
        }
        else if (blk_type() == DAG_OBJ_BONES)
        {
          SkinData *sd = new SkinData;
          sd->skinNode = n;
          rd(&sd->numb, 2);
          sd->bones.SetCount(sd->numb);
          rd(&sd->bones[0], sd->bones.Count() * sizeof(DagBone));
          rd(&sd->numvert, 4);
          sd->wt.SetCount(sd->numvert * sd->numb);
          rd(&sd->wt[0], sd->wt.Count() * sizeof(float));

          skin_data.Append(1, &sd);
#endif
        }
        else if (blk_type() == DAG_OBJ_SPLINES)
        {
          SplineShape *shape = new SplineShape;
          assert(shape);
          BezierShape &shp = shape->shape;
          int numspl;
          rd(&numspl, 4);
          for (; numspl > 0; --numspl)
          {
            Spline3D &s = *shp.NewSpline();
            assert(&s);
            char flg;
            rd(&flg, 1);
            int numk;
            rd(&numk, 4);
            for (; numk > 0; --numk)
            {
              char ktype;
              rd(&ktype, 1);
              Point3 i, p, o;
              rd(&i, 12);
              rd(&p, 12);
              rd(&o, 12);
              SplineKnot k(KTYPE_BEZIER_CORNER, LTYPE_CURVE, p, i, o);
              s.AddKnot(k);
            }
            s.SetClosed(flg & DAG_SPLINE_CLOSED ? 1 : 0);
            //                                              s.InvalidateGeomCache();
            s.ComputeBezPoints();
          }
          shp.UpdateSels();
          shp.InvalidateGeomCache();
          shp.InvalidateCapCache();
          obj = shape;
        }
        else if (blk_type() == DAG_OBJ_LIGHT)
        {
          DagLight dl;
          DagLight2 dl2;
          rd(&dl, sizeof(dl));
          Color c(dl.r, dl.g, dl.b);
          if (blk_rest() == sizeof(DagLight2))
          {
            rd(&dl2, sizeof(dl2));
          }
          else
          {
            dl2.mult = MaxVal(c);
            dl2.type = DAG_LIGHT_OMNI;
            dl2.hotspot = 0;
            dl2.falloff = 0;
          }
          TimeValue time = ip->GetTime();
          GenLight &lt = *(GenLight *)ip->CreateInstance(LIGHT_CLASS_ID,
            Class_ID((dl2.type == DAG_LIGHT_SPOT ? FSPOT_LIGHT_CLASS_ID
                                                 : (dl2.type == DAG_LIGHT_DIR ? DIR_LIGHT_CLASS_ID : OMNI_LIGHT_CLASS_ID)),
              0));
          assert(&lt);
          float mc = dl2.mult;
          lt.SetIntensity(time, mc);
          if (mc == 0)
            mc = 1;
          lt.SetRGBColor(time, Point3((float *)(c / mc)));
          if (dl2.type != DAG_LIGHT_OMNI)
          {
            lt.SetHotspot(time, dl2.hotspot);
            lt.SetFallsize(time, dl2.falloff);
          }
          lt.SetAtten(time, ATTEN_START, dl.range);
          lt.SetAtten(time, ATTEN_END, dl.range);
          lt.SetUseAtten(TRUE);
          // lt.SetAttenDisplay(TRUE);
          lt.SetDecayType(dl.decay);
          lt.SetDecayRadius(time, dl.drad);
          lt.SetShadow(nflg & DAG_NF_CASTSHADOW ? 1 : 0);
          lt.SetUseLight(TRUE);
          lt.Enable(TRUE);
          obj = &lt;
        }
        else if (blk_type() == DAG_OBJ_FACEFLG && obj && obj->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID, 0)))
        {
          TriObject *tri = (TriObject *)obj;
          Mesh &m = tri->mesh;
          if (n->GetNodeTM(0).Parity())
            flip_normals(m);
          if (m.numFaces == blk_rest())
          {
            for (int i = 0; i < m.numFaces; ++i)
            {
              Face &f = m.faces[i];
              char ef;
              rd(&ef, 1);
              f.flags &= ~(EDGE_ALL | FACE_HIDDEN);
              if (ef & DAG_FACEFLG_EDGE2)
                f.flags |= EDGE_A;
              if (ef & DAG_FACEFLG_EDGE1)
                f.flags |= EDGE_B;
              if (ef & DAG_FACEFLG_EDGE0)
                f.flags |= EDGE_C;
              if (ef & DAG_FACEFLG_HIDDEN)
                f.flags |= FACE_HIDDEN;
            }
          }
          if (n->GetNodeTM(0).Parity())
            flip_normals(m);
        }
        else if (blk_type() == DAG_OBJ_NORMALS && obj && obj->IsSubClassOf(Class_ID(TRIOBJ_CLASS_ID, 0)))
        {
          Mesh &m = ((TriObject *)obj)->mesh;

          if (!m.GetSpecifiedNormals())
          {
            m.SpecifyNormals();
          }
          MeshNormalSpec *normalSpec = m.GetSpecifiedNormals();
          if (normalSpec)
          {
            int numNormals;
            rd(&numNormals, 4);

            int rest = blk_rest();

            if (numNormals > 0 && rest == numNormals * 12 + m.numFaces * 12)
            {
              normalSpec->SetParent(&m);
              normalSpec->CheckNormals();
              normalSpec->SetNumNormals(numNormals);

              normalSpec->SetAllExplicit(true);

              rd(normalSpec->GetNormalArray(), numNormals * 12);

              int *indices = new int[m.numFaces * 3];
              rd(indices, m.numFaces * 3 * 4);

              for (int i = 0; i < m.numFaces; i++)
              {
                normalSpec->SetNormalIndex(i, 0, indices[i * 3]);
                normalSpec->SetNormalIndex(i, 1, indices[i * 3 + 2]);
                normalSpec->SetNormalIndex(i, 2, indices[i * 3 + 1]);
              }

              delete[] indices;
            }
          }
        }

        eblk;
      }
      DebugPrint(_T("   obj mesh ok\n"));
    }
    else if (blk_type() == DAG_NODE_CHILDREN)
    {
      while (blk_rest() > 0)
      {
        bblk;
        if (blk_type() == DAG_NODE)
        {
          if (!load_node(n, h, ii, ip, skin_data, node_id))
            goto read_err;
        }
        else if (blk_type() == DAG_MATER)
          goto read_err;
        eblk;
      }
    }
    eblk;
  }

  if (in)
  {
    if (!obj)
    {
      //                      obj=(Object*)ii->Create(HELPER_CLASS_ID,Class_ID(DUMMY_CLASS_ID,0));
      obj = (Object *)ii->Create(GEOMOBJECT_CLASS_ID, Dummy_CID);
      assert(obj);
    }
    in->Reference(obj);
    pnode->AttachChild(n, 0);
  }
  if (mtl)
    n->SetMtl(mtl);
  return 1;
read_err:
  DebugPrint(_T("read error in load_node() at %X of %s\n"), ftell(h), scene_name.data());
  return 0;
}

#if defined(MAX_RELEASE_R13) && MAX_RELEASE >= MAX_RELEASE_R13
static bool trail_stricmp(const TCHAR *str, const TCHAR *str2)
{
  size_t l = _tcslen(str), l2 = _tcslen(str2);
  return (l >= l2) ? _tcsncicmp(str + l - l2, str2, l2) == 0 : false;
}

static bool find_co_files(const TCHAR *fname, Tab<TCHAR *> &fnames)
{
  if (!trail_stricmp(fname, _T(".lod00.dag")))
    return false;
  int base_len = int(_tcslen(fname) - _tcslen(_T(".lod00.dag")));

  TCHAR *p[2];
  p[0] = _tcsdup(fname);
  p[1] = _tcsdup(_T("LOD00"));
  fnames.Append(2, p);

  TCHAR str_buf[512];
  int i;
  for (i = 1; i < 16; i++)
  {
    _stprintf(str_buf, _T("%.*s.lod%02d.dag"), base_len, fname, i);
    if (::_taccess(str_buf, 4) == 0)
    {
      p[0] = _tcsdup(str_buf);
      _stprintf(str_buf, _T("LOD%02d"), i);
      p[1] = _tcsdup(str_buf);
      fnames.Append(2, p);
    }
  }

  _stprintf(str_buf, _T("%.*s_destr.lod00.dag"), base_len, fname);
  if (::_taccess(str_buf, 4) == 0)
  {
    p[0] = _tcsdup(str_buf);
    p[1] = _tcsdup(_T("DESTR"));
    fnames.Append(2, p);
  }

  // if (base_len > 2 && _tcsncmp(&fname[base_len-2], _T("_a"), 2)==0)
  //   base_len -= 2;

  _stprintf(str_buf, _T("%.*s_dm.dag"), base_len, fname);
  if (::_taccess(str_buf, 4) == 0)
  {
    p[0] = _tcsdup(str_buf);
    p[1] = _tcsdup(_T("DM"));
    fnames.Append(2, p);
  }

  for (i = 0; i < 16; i++)
  {
    _stprintf(str_buf, _T("%.*s_dmg.lod%02d.dag"), base_len, fname, i);
    if (::_taccess(str_buf, 4) == 0)
    {
      p[0] = _tcsdup(str_buf);
      _stprintf(str_buf, _T("DMG_LOD%02d"), i);
      p[1] = _tcsdup(str_buf);
      fnames.Append(2, p);
    }
  }

  for (i = 0; i < 16; i++)
  {
    _stprintf(str_buf, _T("%.*s_dmg2.lod%02d.dag"), base_len, fname, i);
    if (::_taccess(str_buf, 4) == 0)
    {
      p[0] = _tcsdup(str_buf);
      _stprintf(str_buf, _T("DMG2_LOD%02d"), i);
      p[1] = _tcsdup(str_buf);
      fnames.Append(2, p);
    }
  }

  for (i = 0; i < 16; i++)
  {
    _stprintf(str_buf, _T("%.*s_expl.lod%02d.dag"), base_len, fname, i);
    if (::_taccess(str_buf, 4) == 0)
    {
      p[0] = _tcsdup(str_buf);
      _stprintf(str_buf, _T("EXPL_LOD%02d"), i);
      p[1] = _tcsdup(str_buf);
      fnames.Append(2, p);
    }
  }

  _stprintf(str_buf, _T("%.*s_xray.dag"), base_len, fname);
  if (::_taccess(str_buf, 4) == 0)
  {
    p[0] = _tcsdup(str_buf);
    p[1] = _tcsdup(_T("XRAY"));
    fnames.Append(2, p);
  }

  if (fnames.Count() > 1)
    return true;

  for (i = 0; i < fnames.Count(); i++)
    free(fnames[i]);
  fnames.ZeroCount();
  return false;
}
#endif

bool DagImp::separateLayers = true;

int DagImp::DoImport(const TCHAR *fname, ImpInterface *ii, Interface *ip, BOOL nomsg)
{
#if defined(MAX_RELEASE_R13) && MAX_RELEASE >= MAX_RELEASE_R13
  Tab<TCHAR *> fnames;

  if (!find_co_files(fname, fnames))
    return doImportOne(fname, ii, ip, nomsg);

  TCHAR buf[1024];
  int fn_len = (int)_tcslen(fname);
  _stprintf(buf, _T("We detected that %s%s\nhas %d linked DAGs.\nLoad them at once into separate layers?"),
    fn_len > 64 ? _T("...") : _T(""), fn_len > 64 ? fname + fn_len - 64 : fname, fnames.Count() / 2 - 1);
  if ((nomsg && !DagImp::separateLayers) ||
      (!nomsg && MessageBox(GetFocus(), buf, _T("Import layered DAGs"), MB_YESNO | MB_ICONQUESTION) != IDYES))
  {
    for (int i = 0; i < fnames.Count(); i++)
      free(fnames[i]);
    return doImportOne(fname, ii, ip, nomsg);
  }

  ILayerManager *manager = GetCOREInterface13()->GetLayerManager();
  manager->Reset();
  for (int i = 0; i < fnames.Count(); i += 2)
  {
    TSTR layer_nm(fnames[i + 1]);
    manager->AddLayer(manager->CreateLayer(layer_nm));
    manager->SetCurrentLayer(layer_nm);
    free(fnames[i + 1]);
    if (!doImportOne(fnames[i], ii, ip, nomsg))
    {
      free(fnames[i]);
      return 0;
    }
    free(fnames[i]);
  }
  manager->SetCurrentLayer();
  return 1;

#else
  return doImportOne(fname, ii, ip, nomsg);
#endif
}


#define rd(p, l)                \
  {                             \
    if (fread(p, l, 1, h) != 1) \
      goto read_err;            \
  }


int DagImp::doImportOne(const TCHAR *fname, ImpInterface *ii, Interface *ip, BOOL nomsg)
{
  DebugPrint(_T("import...\n"));

  cleanup();
  Tab<SkinData *> skin_data;
  Tab<NodeId> node_id;
  SplitFilename(TSTR(fname), NULL, &scene_name, NULL);
  FILE *h = _tfopen(fname, _T("rb"));
  if (!h)
  {
    if (!nomsg)
      ip->DisplayTempPrompt(GetString(IDS_FILE_OPEN_ERR), ERRMSG_DELAY);
    return 0;
  }
  {
    int id;
    rd(&id, 4);
    if (id != DAG_ID)
    {
      fclose(h);
      if (!nomsg)
        ip->DisplayTempPrompt(GetString(IDS_INVALID_DAGFILE), ERRMSG_DELAY);
      return 0;
    }
  }

  // Set units to meters.

#if defined(MAX_RELEASE_R24) && MAX_RELEASE >= MAX_RELEASE_R24
  ::SetSystemUnitInfo(UNITS_METERS, 1.f);
#else
  ::SetMasterUnitInfo(UNITS_METERS, 1.f);
#endif


  init_blk(h);
  bblk;
  DebugPrint(_T("imp_a\n"));
  if (blk_type() != DAG_ID)
  {
    fclose(h);
    if (!nomsg)
      ip->DisplayTempPrompt(GetString(IDS_INVALID_DAGFILE), ERRMSG_DELAY);
    return 0;
  }
  for (; blk_rest() > 0;)
  {
    bblk;
    DebugPrint(_T("imp %d\n"), blk_type());
    if (blk_type() == DAG_END)
    {
      eblk;
      break;
    }
    if (blk_type() == DAG_TEXTURES)
    {
      int n = 0;
      rd(&n, 2);
      tex.SetCount(n);
      for (int i = 0; i < n; ++i)
      {
        int l = 0;
        rd(&l, 1);

        tex[i] = ReadCharString(l, h);
        if (!tex[i])
          goto read_err;

        /*tex[i] = (TCHAR *) malloc(l + 1);
        assert(tex[i]);
        if (l)
        {
          rd(tex[i], l);
        }
        tex[i][l] = 0;*/
      }
    }
    else if (blk_type() == DAG_MATER)
    {
      ImpMat m;
      int l = 0;
      rd(&l, 1);
      if (l > 0)
      {
        /* m.name = (TCHAR *) malloc(l + 1);
         assert(m.name);
         rd(m.name, l);
         m.name[l] = 0;*/

        m.name = ReadCharString(l, h);
        if (!m.name)
          goto read_err;
      }
      rd(&m.m, sizeof(m.m));
      l = 0;
      rd(&l, 1);
      if (l > 0)
      {
        /* m.clsname = (TCHAR *) malloc(l + 1);
         assert(m.clsname);
         rd(m.clsname, l);
         m.clsname[l] = 0;*/
        m.clsname = ReadCharString(l, h);
        if (!m.clsname)
          goto read_err;
      }
      l = blk_rest();
      if (l > 0)
      {
        /* m.script = (TCHAR *) malloc(l + 1);
         assert(m.script);
         rd(m.script, l);
         m.script[l] = 0;*/

        m.script = ReadCharString(l, h);
        if (!m.script)
          goto read_err;
      }
      m.mtl = NULL;
      mat.Append(1, &m);
    }
    else if (blk_type() == DAG_KEYLABELS)
    {
      int n = 0;
      rd(&n, 2);
      keylabel.SetCount(n);
      int i;
      for (i = 0; i < n; ++i)
        keylabel[i] = NULL;
      for (i = 0; i < n; ++i)
      {
        int l = 0;
        rd(&l, 1);

        /*char *s = (char *) malloc(l + 1);
        assert(s);
        rd(s, l); s[l] = 0;
        keylabel[i] = s;*/

        keylabel[i] = ReadCharString(l, h);
        if (!keylabel[i])
          goto read_err;
      }
    }
    else if (blk_type() == DAG_NOTETRACK)
    {
      int n = blk_rest() / sizeof(DagNoteKey);
      if (n > 0)
      {
        DagNoteKey *nk = (DagNoteKey *)malloc(n * sizeof(DagNoteKey));
        assert(nk);
        rd(nk, n * sizeof(*nk));
        /*
        ntrack.Append(1,&nk);
        ntrack_knum.Append(1,&n);
        */
        DefNoteTrack *nt = (DefNoteTrack *)NewDefaultNoteTrack();
        if (nt)
        {
          for (int i = 0; i < n; ++i)
          {
            TCHAR *nm = _T("");
            if (nk[i].id < keylabel.Count())
              nm = keylabel[nk[i].id];
            NoteKey *k = new NoteKey(nk[i].t, nm);
            nt->keys.Append(1, &k);
          }
        }
        ntrack.Append(1, &nt);
        free(nk);
      }
      else
      {
        DefNoteTrack *nt = (DefNoteTrack *)NewDefaultNoteTrack();
        ntrack.Append(1, &nt);
      }
    }
    else if (blk_type() == DAG_NODE)
    {
      if (!load_node(NULL, h, ii, ip, skin_data, node_id))
        goto read_err;
    }
    DebugPrint(_T("imp_b\n"));
    eblk;
  }
  eblk;
#undef rd
#undef bblk
#undef eblk
  fclose(h);

#if MAX_RELEASE >= 12000
  for (int i = 0; i < skin_data.Count(); i++)
  {
    SkinData &sd = *skin_data[i];
    Modifier *skinMod = (Modifier *)CreateInstance(OSM_CLASS_ID, SKIN_CLASSID);
    GetCOREInterface12()->AddModifier(*sd.skinNode, *skinMod);

    ISkin *iskin = (ISkin *)skinMod->GetInterface(I_SKIN);
    ISkinImportData *iskinImport = (ISkinImportData *)skinMod->GetInterface(I_SKINIMPORTDATA);
    assert(iskin);
    assert(iskinImport);

    Tab<INode *> bn;
    Tab<float> wt;
    bn.SetCount(sd.numb);
    wt.SetCount(sd.numb);
    for (int j = 0; j < bn.Count(); j++)
    {
      bn[j] = NULL;
      for (int k = 0; k < node_id.Count(); k++)
        if (sd.bones[j].id == node_id[k].id)
        {
          bn[j] = node_id[k].node;
          break;
        }
      assert(bn[j]);
      iskinImport->AddBoneEx(bn[j], j + 1 == sd.numb);
    }

    for (int j = 0; j < sd.numvert; j++)
    {
      memcpy(&wt[0], &sd.wt[j * wt.Count()], wt.Count() * sizeof(float));
      iskinImport->AddWeights(sd.skinNode, j, bn, wt);
    }

    delete skin_data[i];
  }
#endif

  DebugPrint(_T("clean up\n"));
  cleanup();
  //  collapse_materials(ip);
  //  DebugPrint(_T("collapsed\n"));
  convertnew(ip, true);

  DebugPrint(_T("import ok\n"));

  return 1;
read_err:
  DebugPrint(_T("read error at %X of %s\n"), ftell(h), scene_name.data());
  fclose(h);
  cleanup(1);
  if (!nomsg)
    ip->DisplayTempPrompt(GetString(IDS_FILE_READ_ERR), ERRMSG_DELAY);
  return 0;
}

//==========================================================================//

enum ImpOps
{
  fun_import
};

class IDagorImportUtil : public FPStaticInterface
{
public:
  DECLARE_DESCRIPTOR(IDagorImportUtil)
  BEGIN_FUNCTION_MAP FN_3(fun_import, TYPE_BOOL, import_dag, TYPE_STRING, TYPE_BOOL, TYPE_BOOL) END_FUNCTION_MAP

    BOOL import_dag(const TCHAR *fn, bool separateLayers, bool suppressPrompts)
  {
    DagImp::separateLayers = separateLayers;

    Interface *ip = GetCOREInterface();
    BOOL result = ip->ImportFromFile(fn, suppressPrompts);

    DagImp::separateLayers = true;

    return result;
  }
};

static IDagorImportUtil dagorimputiliface(Interface_ID(0x20906172, 0x435c11e0), _T("dagorImport"), IDS_DAGOR_IMPORT_IFACE, NULL,
  FP_CORE, fun_import, _T("import"), -1, TYPE_BOOL, 0, 3, _T("filename"), -1, TYPE_STRING,
  // f_keyArgDefault marks an optional keyArg param. The value
  // after that is its default value.
  _T("separateLayers"), -1, TYPE_BOOL, f_keyArgDefault, true, _T("suppressPrompts"), -1, TYPE_BOOL, f_keyArgDefault, false,
#if defined(MAX_RELEASE_R15) && MAX_RELEASE >= MAX_RELEASE_R15
  p_end
#else
  end
#endif
);
