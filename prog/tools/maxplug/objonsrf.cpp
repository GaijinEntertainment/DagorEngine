// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include <utilapi.h>
#include <bmmlib.h>
#include <stdmat.h>
#include <splshape.h>
#include <notetrck.h>
#include <modstack.h>
#include <random.h>
#include "dagor.h"
#include "enumnode.h"
#include "resource.h"
#include "debug.h"
#if defined(MAX_RELEASE_R13) && MAX_RELEASE >= MAX_RELEASE_R13
#include <INamedSelectionSetManager.h>
#endif

/*struct NodeInfo{
  Matrix3 tm;
  INode
};
class GenerateObjectOnSurfRestore : public RestoreObj {
public:
  Tab<NewNodeInfo> nodes;
  bool

  GenerateObjectOnSurfRestore() {
  }
  void Restore(int isUndo) {
    setList->AppendSet (set, 0, name);
    if (ep->ip) {
      ep->ip->NamedSelSetListChanged();
      ep->UpdateNamedSelDropDown ();
    }
  }
  void Redo() {
    setList->RemoveSet (name);
    if (ep->ip) {
      ep->ip->NamedSelSetListChanged();
      ep->UpdateNamedSelDropDown ();
    }
  }

  TSTR Description() {return TSTR(_T("Generate objects"));}
};*/


static void calc_mesh_info(Mesh &m, Point3 *vert, Tab<float> &faceS, bool selectedfaces)
{
  float sumS = 0;
  faceS.SetCount(m.numFaces);
  for (int i = 0; i < m.numFaces; ++i)
  {
    // Point3 n=CrossProd((vert[m.faces[i].v[2]]-vert[m.faces[i].v[0]]),(vert[m.faces[i].v[1]]-vert[m.faces[i].v[0]]));
    float S;
    if (selectedfaces && !mesh_face_sel(m)[i])
      S = 0;
    else
    {
      float lg = Length(m.FaceNormal(i));
      if (lg != 0)
      {
        S = 0.5f * lg;
      }
      else
      {
        S = 0;
      }
    }
    sumS += S;
    faceS[i] = sumS;
  }
}
#if defined(MAX_RELEASE_R13) && MAX_RELEASE >= MAX_RELEASE_R13
static int find_selset_by_name(Interface *, TCHAR *selsname)
{
  INamedSelectionSetManager *ip = INamedSelectionSetManager::GetInstance();
#else
static int find_selset_by_name(Interface *ip, TCHAR *selsname)
{
#endif
  int selset;
  for (selset = 0; selset < ip->GetNumNamedSelSets(); ++selset)
  {
    if (_tcscmp(ip->GetNamedSelSetName(selset), selsname) == 0)
      break;
  }
  if (selset >= ip->GetNumNamedSelSets())
    return -1;
  return selset;
}

static int find_face(float t, Tab<float> &faceS)
{
  int a = 0, b = faceS.Count() - 1;
  if (a == b)
    return a;
  if (faceS[a] >= t)
    return a;
  if (faceS[b - 1] <= t)
    return b;

  while (a <= b)
  {
    int c = (a + b) / 2;
    if (c == a)
      return a;
    if (faceS[c] < t)
      a = c;
    else if (faceS[c - 1] > t)
      b = c;
    else
      return c;
  }
  return b;
}

static int find_face(float t, Tab<float> &faceS, Tab<int> &face_count, int &founded_node)
{
  founded_node = 0;
  int fface = find_face(t, faceS);
  for (int i = 0; i < face_count.Count(); ++i)
    if (fface < face_count[i])
    {
      founded_node = i;
      break;
    }
  return fface;
}
static TriObject *GetTriObjectFromNode(INode *node, int &deleteIt)
{
  deleteIt = FALSE;
  Object *obj = node->EvalWorldState(0).obj;
  if (obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
  {
    TriObject *tri = (TriObject *)obj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0));
    if (obj != tri)
      deleteIt = TRUE;
    return tri;
  }
  else
  {
    return NULL;
  }
}
static Point3 get_normal(DWORD s, RVertex &rv)
{
  int fnc = rv.rFlags & NORCT_MASK;
  if (fnc <= 1)
    return rv.rn.getNormal();
  int i;
  for (i = 0; i < fnc - 1; ++i)
  {
    if (rv.ern[i].getSmGroup() & s)
      return rv.ern[i].getNormal();
  }
  return rv.ern[i].getNormal();
}


// void put_meshes_on_mesh(Interface *ip,TCHAR *selsname,Tab<INode *> &snode,int objnum,int seed,bool set_to_norm,bool use_smgr,bool
// rotatez,float slope,bool selfaces){
void put_meshes_on_mesh(Interface *ip, TCHAR *selsname, Tab<INode *> &snode, int objnum, int seed, bool set_to_norm, bool use_smgr,
  bool rotatez, float slope, bool selfaces, float xydiap[2], float zdiap[2], bool xys, char zs)
{
#if defined(MAX_RELEASE_R13) && MAX_RELEASE >= MAX_RELEASE_R13
  INamedSelectionSetManager *IPNSS = INamedSelectionSetManager::GetInstance();
#else
#define IPNSS ip
#endif
  Tab<int> deleteIt;
  Tab<TriObject *> triObjects;
  Tab<int> face_count;
  if (!snode.Count())
    return;
  face_count.SetCount(snode.Count());
  triObjects.SetCount(snode.Count());
  deleteIt.SetCount(snode.Count());
  // debug("11");
  {
    for (int i = 0; i < snode.Count(); ++i)
    {
      triObjects[i] = GetTriObjectFromNode(snode[i], deleteIt[i]);
      if (triObjects[i] && deleteIt[i])
        mesh_face_sel(triObjects[i]->mesh).SetAll();
    }
  }
  // debug("21");
  int sels = find_selset_by_name(ip, selsname);
  if (sels < 0)
  {
    TSTR s;
    s.printf(GetString(IDS_NO_SUCHSELSET), selsname);
    ip->DisplayTempPrompt(s);
    {
      for (int i = 0; i < snode.Count(); ++i)
      {
        if (deleteIt[i])
          triObjects[i]->DeleteMe();
      }
    }
    return;
  }
  // debug("31");
  if (!IPNSS->GetNamedSelSetItemCount(sels))
  {
    {
      for (int i = 0; i < snode.Count(); ++i)
      {
        if (deleteIt[i])
          triObjects[i]->DeleteMe();
      }
    }
    return;
  }
  // debug("41");
  Mesh surf;
  {
    for (int fc = 0, i = 0; i < triObjects.Count(); ++i)
      if (triObjects[i])
      {
        fc += triObjects[i]->mesh.numFaces;
        face_count[i] = fc;
        Matrix3 tm = snode[i]->GetObjectTM(ip->GetTime());
        Mesh surf1(surf);
        CombineMeshes(surf, surf1, triObjects[i]->mesh, NULL, &tm, -1);
        {
          for (int fsi = 0; fsi < surf1.numFaces; ++fsi)
            if (mesh_face_sel(surf1)[fsi])
              mesh_face_sel(surf).Set(fsi);
        }
        {
          for (int fsi1 = 0, fsi = surf1.numFaces; fsi < surf.numFaces; ++fsi, ++fsi1)
            if (mesh_face_sel(triObjects[i]->mesh)[fsi1])
              mesh_face_sel(surf).Set(fsi);
        }
      }
      else
      {
        face_count[i] = fc;
      }
  }
  // debug("51");
  if (!surf.numVerts || !surf.numFaces)
  {
    {
      for (int i = 0; i < snode.Count(); ++i)
      {
        if (deleteIt[i])
          triObjects[i]->DeleteMe();
      }
    }
    return;
  }
  // debug("61");

  Tab<float> faceS;
  calc_mesh_info(surf, &surf.verts[0], faceS, selfaces);
  // debug("71");

  Random r;
  if (!seed)
    seed = 1;
  r.srand(seed);
  int selnodecnt = IPNSS->GetNamedSelSetItemCount(sels);
  float sumS = faceS[faceS.Count() - 1];

  if (set_to_norm && use_smgr)
    surf.buildNormals();

  if (xydiap[0] > xydiap[1])
  {
    float a = xydiap[0];
    xydiap[0] = xydiap[1];
    xydiap[1] = a;
  }
  if (zdiap[0] > zdiap[1])
  {
    float a = zdiap[0];
    zdiap[0] = zdiap[1];
    zdiap[1] = a;
  }

  theHold.Begin();

  for (int i = 0; i < objnum; ++i)
  {
    int nodnm = r.get(selnodecnt);
    INode *putn = IPNSS->GetNamedSelSetItem(sels, nodnm);
    float Sval = r.getf(sumS);
    float u = r.getf();
    float v = r.getf();
    float rota = r.getf(TWOPI);
    float slopea = r.getf(slope);
    float sloperota = r.getf(TWOPI);
    float scalexy, scalez;
    if (xydiap[0] != xydiap[1])
      scalexy = r.getf(xydiap[1], xydiap[0]);
    else
    {
      r.getf();
      scalexy = xydiap[0];
    }
    if (zdiap[0] != zdiap[1])
      scalez = r.getf(zdiap[1], zdiap[0]);
    else
    {
      r.getf();
      scalez = zdiap[0];
    }
    int founded_node;
    int fi = find_face(Sval, faceS, face_count, founded_node);

    if (u + v > 1)
    {
      u = 1 - u;
      v = 1 - v;
    }
    Point3 &v0 = surf.verts[surf.faces[fi].v[0]];
    Point3 p = (surf.verts[surf.faces[fi].v[1]] - v0) * u + (surf.verts[surf.faces[fi].v[2]] - v0) * v + v0;
    INode *nnode = ip->CreateObjectNode(putn->GetObjectRef());
    TSTR name;
    name.printf(_T("%s_on_%s_"), putn->GetName(), snode[founded_node]->GetName());
    ip->MakeNameUnique(name);
    nnode->SetName(name);
    Matrix3 ntm = putn->GetNodeTM(ip->GetTime());
    if (set_to_norm)
    {
      Point3 a2;
      if (use_smgr)
      {
        Point3 n0 = get_normal(surf.faces[fi].smGroup, surf.getRVert(surf.faces[fi].v[0]));
        Point3 n1 = get_normal(surf.faces[fi].smGroup, surf.getRVert(surf.faces[fi].v[1]));
        Point3 n2 = get_normal(surf.faces[fi].smGroup, surf.getRVert(surf.faces[fi].v[2]));
        a2 = Normalize((n1 - n0) * u + (n2 - n0) * v + n0);
      }
      else
      {
        a2 = surf.FaceNormal(fi, TRUE);
      }
      Point3 p;
      if (a2.x > 0.9 || a2.x < -0.9)
      {
        p = Normalize(Point3(0, -1, 0) ^ a2);
      }
      else
        p = Normalize(Point3(-1, 0, 0) ^ a2);
      Matrix3 positm;
      positm.SetRow(0, p);
      positm.SetRow(1, Normalize(a2 ^ p));
      positm.SetRow(2, a2);
      positm.SetRow(3, Point3(0, 0, 0));
      // ntm=ntm*positm;
      ntm = positm;
    }
    if (slopea != 0)
      ntm = ntm * RotAngleAxisMatrix(Point3(cosf(sloperota), sinf(sloperota), 0.0f), slopea);
    if (rotatez)
      ntm.PreRotateZ(rota); //*RotateZMatrix(rota);
    ntm.SetTrans(p);
    if (!xys && !zs)
    {
      ntm.PreScale(Point3(scalexy, scalexy, scalexy));
    }
    else
    {
      if (!xys)
        scalexy = 1;
      if (!zs)
        scalez = 1;
      ntm.PreScale(Point3(scalexy, scalexy, scalez));
    }

    nnode->SetNodeTM(ip->GetTime(), ntm);
    nnode->SetObjOffsetPos(putn->GetObjOffsetPos());
    nnode->SetObjOffsetRot(putn->GetObjOffsetRot());
    nnode->SetObjOffsetScale(putn->GetObjOffsetScale());
  }
  TSTR holdopname;
  holdopname.printf(_T("Generating %d objects"), objnum);
  theHold.Accept(holdopname);
  // debug("81");
  {
    for (int i = 0; i < snode.Count(); ++i)
    {
      if (deleteIt[i])
        triObjects[i]->DeleteMe();
    }
  }
  // debug("91");
  ip->RedrawViews(ip->GetTime());
  // debug("01");
}
