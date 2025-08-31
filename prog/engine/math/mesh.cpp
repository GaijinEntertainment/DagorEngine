// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_mesh.h>
#include <generic/dag_tab.h>
#include <generic/dag_qsort.h>
#include <generic/dag_carray.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_e3dColor.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_Matrix3.h>
#include <math/dag_TMatrix.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_bounds3.h>
#include <math/dag_tangentSpace.h>
#include <debug/dag_log.h>
#include <ioSys/dag_genIo.h>
#include <util/dag_bitArray.h>
#include <util/dag_stlqsort.h>
#include <EASTL/hash_map.h>
#include <hash/mum_hash.h>

#include <debug/dag_debug.h>

MeshData::ExtraChannel::ExtraChannel() : fc(tmpmem), vt(tmpmem)
{
  type = CHT_UNKNOWN;
  vsize = 0;
  usg = usgi = 0;
}

void MeshData::ExtraChannel::setmem(IMemAlloc *mem)
{
  dag::set_allocator(vt, mem);
  dag::set_allocator(fc, mem);
}

MeshData::MeshData() :
  vert(tmpmem_ptr()),
  face(tmpmem_ptr()),
  ntab(tmpmem_ptr()),
  ngr(tmpmem_ptr()),
  facengr(tmpmem_ptr()),
  facenorm(tmpmem_ptr()),
  vertnorm(tmpmem_ptr()),
  cvert(tmpmem_ptr()),
  faceflg(tmpmem_ptr()),
  extra(tmpmem_ptr())
{}

bool MeshData::ExtraChannel::resize_verts(int n, int t)
{
  type = t;
  switch (t)
  {
    case CHT_FLOAT1: vsize = sizeof(float) * 1; break;
    case CHT_FLOAT2: vsize = sizeof(float) * 2; break;
    case CHT_FLOAT3: vsize = sizeof(float) * 3; break;
    case CHT_FLOAT4: vsize = sizeof(float) * 4; break;
    case CHT_E3DCOLOR: vsize = sizeof(E3DCOLOR); break;
    case CHT_UNKNOWN: vsize = 0; break;
    default: vsize = 0; LOGERR_CTX("unknown mesh channel type %d", t);
  }
  if (vsize == 0)
    return false;
  vt.resize(n * vsize);
  return true;
}

int MeshData::find_extra_channel(int u, int ui) const
{
  int c;
  for (c = 0; c < extra.size(); ++c)
    if (extra[c].usg == u && extra[c].usgi == ui)
      return c;
  return -1;
}

int MeshData::add_extra_channel(int t, int u, int ui)
{
  int c = find_extra_channel(u, ui);
  if (c >= 0)
    return -1;
  c = append_items(extra, 1);
  if (c < 0)
    return c;
  extra[c].resize_verts(0, t);
  extra[c].usg = u;
  extra[c].usgi = ui;
  extra[c].fc.resize(face.size());
  return c;
}

Mesh::Mesh() {}

static Face *faces;

#ifndef __clang__
struct FaceMatCompare
{
  static int compare(const int a, const int b) { return faces[a].mat - faces[b].mat; }
};
#endif

void MeshData::erasefaces(int at, int n)
{
  erase_items(face, at, n);
  safe_erase_items(facengr, at, n);
  safe_erase_items(facenorm, at, n);
  safe_erase_items(faceflg, at, n);
  if (cface.size())
    erase_items(cface, at, n);
  int c;
  for (c = 0; c < NUMMESHTCH; ++c)
    if (tface[c].size())
      erase_items(tface[c], at, n);
  for (c = 0; c < extra.size(); ++c)
    safe_erase_items(extra[c].fc, at, n);
}


void MeshData::removeFacesFast(int at, int n)
{
  int nf = face.size();
  erase_items_fast(face, at, n);

#define ERASE(arr)      \
  if (arr.size() == nf) \
    erase_items_fast(arr, at, n);

  ERASE(facengr)
  ERASE(facenorm)
  ERASE(faceflg)
  ERASE(cface)

  for (int c = 0; c < NUMMESHTCH; ++c)
    ERASE(tface[c])

  for (int c = 0; c < extra.size(); ++c)
    ERASE(extra[c].fc)

#undef ERASE
  if (n && ntab.size()) // if ntab was build for older faces we shall force rebuilding it
  {
    ntab.clear();
    ngr.clear();
  }
}

void MeshData::clampMaterials(uint16_t max_val, int sf, int numf)
{
  if (numf < 0)
    numf = face.size() - sf;
  numf += sf;
  for (int i = sf; i < numf; ++i)
    if (face[i].mat > max_val)
      face[i].mat = max_val;
}

bool MeshData::setnumfaces(int n) { return setNumFaces(n, *this, true); }

bool MeshData::setNumFaces(int n, const MeshData &m, bool set0)
{
  if (face.size() == n)
    return true;
  face.resize(n);
  if (m.faceflg.size() && faceflg.size() != n)
  {
    int f = faceflg.size();
    faceflg.resize(n);
    if (set0 && faceflg.size() > f)
      memset(&faceflg[f], FACEFLG_ALLEDGES, faceflg.size() - f);
  }
  if (m.cface.size() && cface.size() != n)
  {
    int f = cface.size();
    cface.resize(n);
    if (set0 && cface.size() > f)
      memset(&cface[f], 0, (cface.size() - f) * sizeof(TFace));
  }
  if (m.facengr.size() && facengr.size() != n)
  {
    int f = facengr.size();
    facengr.resize(n);
    if (set0 && facengr.size() > f)
      memset(&facengr[f][0], 0, (facengr.size() - f) * sizeof(FaceNGr));
  }
  int c;
  for (c = 0; c < NUMMESHTCH; ++c)
    if (m.tface[c].size() && tface[c].size() != n)
    {
      int f = tface[c].size();
      tface[c].resize(n);
      if (set0 && tface[c].size() > f)
        memset(&tface[c][f], 0, (tface[c].size() - f) * sizeof(TFace));
    }
  for (c = 0; c < extra.size(); ++c)
    if (m.find_extra_channel(extra[c].usg, extra[c].usgi) != -1 && extra[c].fc.size() != n)
    {
      int f = extra[c].fc.size();
      extra[c].fc.resize(n);
      if (set0 && extra[c].fc.size() > f)
        memset(&extra[c].fc[f], 0, (extra[c].fc.size() - f) * sizeof(TFace));
    }
  return true;
}

bool MeshData::sort_faces_by_mat()
{
  G_ASSERTF(face.size() == faceflg.size() || !faceflg.size(), "face.size()=%d faceflg.size()=%d", face.size(), faceflg.size());
  int i;
  for (i = 1; i < face.size(); ++i)
    if (face[i - 1].mat > face[i].mat)
      break;
  if (i >= face.size())
    return true; // already sorted

  if (face.size() > (64 << 10))
  {
    carray<int, 256> matCnt, matIdx;

    mem_set_0(matCnt);
    for (i = 0; i < face.size(); ++i)
      if (face[i].mat < 256)
        matCnt[face[i].mat]++;
      else
        goto sort_codepath;
    mem_set_0(matIdx);
    int idx = 0;
    for (i = 0; i < matCnt.size(); ++i)
    {
      matIdx[i] = idx;
      idx += matCnt[i];
    }

    Tab<int> fi(tmpmem);
    fi.resize(face.size());
    for (i = 0; i < face.size(); ++i)
    {
      int mat = face[i].mat;
      fi[matIdx[mat]] = i;
      matIdx[mat]++;
    }
    return reorder_faces(fi.data());
  }

sort_codepath:
  Tab<int> fi(tmpmem);
  fi.resize(face.size());
  for (int j = 0; j < fi.size(); ++j)
    fi[j] = j;
  faces = &face[0];
#ifdef __clang__
  stlsort::sort(fi.data(), fi.data() + fi.size(), [&](auto a, auto b) { return faces[a].mat < faces[b].mat; });
#else
  SimpleQsort<int, FaceMatCompare>::sort(&fi[0], fi.size());
#endif
  return reorder_faces(fi.data());
}

bool MeshData::reorder_faces(int *fi, int sf, int nf)
{
  if (nf < 0)
    nf = face.size() - sf;
  facenorm.clear();
  Face *f = new Face[nf];
  memcpy(f, &face[sf], sizeof(*f) * nf);
  int i;
  for (i = 0; i < nf; ++i)
    if (fi[i] != i)
      face[sf + i] = f[fi[i]];
  delete[] f;
  TFace *tf = new TFace[nf];
  if (faceflg.size())
  {
    uint8_t *ff = (uint8_t *)tf;
    memcpy(ff, &faceflg[sf], nf * elem_size(faceflg));
    for (i = 0; i < nf; ++i)
      if (fi[i] != i)
        faceflg[sf + i] = ff[fi[i]];
  }
  if (cface.size())
  {
    memcpy(tf, &cface[sf], sizeof(*tf) * nf);
    for (i = 0; i < nf; ++i)
      if (fi[i] != i)
        cface[sf + i] = tf[fi[i]];
  }
  int c;
  for (c = 0; c < NUMMESHTCH; ++c)
    if (tface[c].size())
    {
      memcpy(tf, &tface[c][sf], sizeof(*tf) * nf);
      for (i = 0; i < nf; ++i)
        if (fi[i] != i)
          tface[c][sf + i] = tf[fi[i]];
    }
  for (c = 0; c < extra.size(); ++c)
    if (extra[c].fc.size())
    {
      memcpy(tf, &extra[c].fc[sf], sizeof(*tf) * nf);
      for (i = 0; i < nf; ++i)
        if (fi[i] != i)
          extra[c].fc[sf + i] = tf[fi[i]];
    }
  delete[] tf;
  if (facengr.size() == face.size())
  {
    FaceNGr *fngr = new FaceNGr[nf];
    memcpy(fngr, &facengr[sf][0], sizeof(*fngr) * nf);
    for (i = 0; i < nf; ++i)
      if (fi[i] != i)
        memcpy(&facengr[sf + i][0], &fngr[fi[i]], sizeof(*fngr));
    delete[] fngr;
  }
  if (nf == face.size() && ngr.size())
  {
    Tab<int> ifi(tmpmem);
    ifi.resize(nf);
    for (i = 0; i < nf; ++i)
      ifi[fi[i]] = i;
    for (i = 0; i < ngr.size(); ++i)
    {
      int k = ngr[i];
      int cc = ntab[k++];
      for (; cc; --cc, ++k)
        ntab[k] = ifi[ntab[k]];
    }
  }
  return 1;
}
// setmem
void MeshData::setmem(IMemAlloc *m)
{
  dag::set_allocator(vert, m);
  dag::set_allocator(face, m);
  int i;
  for (i = 0; i < NUMMESHTCH; ++i)
  {
    dag::set_allocator(tvert[i], m);
    dag::set_allocator(tface[i], m);
  }
  for (i = 0; i < extra.size(); ++i)
    extra[i].setmem(m);
  dag::set_allocator(cvert, m);
  dag::set_allocator(cface, m);

  dag::set_allocator(ntab, m);
  dag::set_allocator(ngr, m);
  dag::set_allocator(facengr, m);
  dag::set_allocator(facenorm, m);
  dag::set_allocator(vertnorm, m);
  dag::set_allocator(faceflg, m);
}


// build map that maps each vertex to list of indices (face_index*3+in_face_vertex)
void MeshData::buildVertToFaceVertMap(F2V_Map &map, const Face *face, int numf, int vnum, int mulFI, int mulVI)
{
  int i, data_sz = 0;

  data_sz = numf * 3;

  map.data.clear();
  map.index.clear();
  map.data.resize(data_sz);
  map.index.resize(vnum * 2);
  mem_set_0(map.index);

  for (i = 0; i < numf; i++)
  {
    map.index[face[i].v[0]]++;
    map.index[face[i].v[1]]++;
    map.index[face[i].v[2]]++;
  }

  data_sz = 0;
  for (i = 0; i < vnum; i++)
  {
    int inc = map.index[i];
    map.index[i] = data_sz;
    data_sz += inc;
  }
  for (i = 0; i < numf; ++i)
  {
    int v0, v1, v2;
    v0 = face[i].v[0];
    v1 = face[i].v[1];
    v2 = face[i].v[2];

    map.data[map.index[v0] + map.index[v0 + vnum]] = i * mulFI + 0;
    map.index[v0 + vnum]++;
    map.data[map.index[v1] + map.index[v1 + vnum]] = i * mulFI + mulVI;
    map.index[v1 + vnum]++;
    map.data[map.index[v2] + map.index[v2 + vnum]] = i * mulFI + 2 * mulVI;
    map.index[v2 + vnum]++;
  }

  map.index.resize(vnum);
}

void MeshData::buildVertToFaceVertMap(VertToFaceVertMap &map)
{
  buildVertToFaceVertMap(map, face.data(), face.size(), vert.size(), 3, 1);
}

bool MeshData::get_vert2facemap_fast(F2V_Map &map, int sf, int numf)
{
  if (numf < 0)
    numf = face.size() - sf;
  buildVertToFaceVertMap(map, face.data() + sf, numf, vert.size(), 1, 0);
  return true;
}


bool MeshData::get_vert2facemap(Tab<Tabint> &map, int sf, int numf)
{
  map.resize(vert.size());
  int i;
  for (i = 0; i < map.size(); ++i)
    map[i].clear();
  if (numf < 0)
    numf = face.size() - sf;
  else
    numf += sf;
  for (i = sf; i < numf; ++i)
  {
    G_ASSERT(face[i].v[0] < vert.size());
    G_ASSERT(face[i].v[1] < vert.size());
    G_ASSERT(face[i].v[2] < vert.size());
    map[face[i].v[0]].push_back(i);
    map[face[i].v[1]].push_back(i);
    map[face[i].v[2]].push_back(i);
  }
  return true;
}

bool MeshData::areNormalsComputed()
{
  if (facengr.size() == face.size() && vertnorm.size() && !ngr.size())
    return false;
  return true;
}

void MeshData::transform(const TMatrix &wtm)
{
  for (int i = 0; i < vert.size(); ++i)
    vert[i] = wtm * vert[i];
  if (!vertnorm.size())
    return;
  Matrix3 ntm = transpose(inverse(Matrix3((float *)wtm.m)));
  for (int i = 0; i < vertnorm.size(); ++i)
    vertnorm[i] = normalize(ntm * vertnorm[i]);
  for (int i = 0; i < facenorm.size(); ++i)
    facenorm[i] = normalize(ntm * facenorm[i]);
  return;
}

bool MeshData::calc_ngr()
{
  if (!areNormalsComputed())
    return true;

  if (facengr.size() == face.size())
    return true;
  ntab.clear();
  ngr.clear();
  vertnorm.clear();
  facengr.resize(face.size());

  // int t0=get_time_msec(),t1;
  int i;
  Tab<Tabint> vrfs(tmpmem);
  get_vert2facemap(vrfs);
  // t1=get_time_msec(); console::print_d("ngr1: %dms",t1-t0); t0=t1;

  const int BADNGR = -1;
  for (i = 0; i < facengr.size(); ++i)
    facengr[i][0] = facengr[i][1] = facengr[i][2] = BADNGR;
  for (i = 0; i < vert.size(); ++i)
  {
    for (int j = 0; j < vrfs[i].size(); ++j)
    {
      int f = vrfs[i][j];
      int vi;
      for (vi = 0; vi < 2; ++vi)
        if (face[f].v[vi] == i)
          break;
      if (facengr[f][vi] != BADNGR)
        continue;
      if (face[f].smgr != 0)
      {
        uint32_t smgr = face[f].smgr;
        int o = append_items(ntab, 1);
        if (o < 0)
          return 0;
        int nf = 0;
        int g = ngr.size();
        ngr.push_back(o);
        if (g < 0)
          return 0;
        int k;
        for (k = j; k < vrfs[i].size(); ++k)
          if (smgr & face[vrfs[i][k]].smgr)
          {
            int ff = vrfs[i][k];
            for (vi = 0; vi < 2; ++vi)
              if (face[ff].v[vi] == i)
                break;
            facengr[ff][vi] = g;
            ntab.push_back(ff);
            ++nf;
          }
        ntab[o] = nf;
      }
      else
      {
        int o = append_items(ntab, 2);
        if (o < 0)
          return 0;
        ntab[o] = 1;
        ntab[o + 1] = f;
        int g = ngr.size();
        ngr.push_back(o);
        if (g < 0)
          return 0;
        facengr[f][vi] = g;
      }
    }
  }
  return true;
}

bool MeshData::calc_facenorms()
{
  if (facenorm.size() != face.size())
    facenorm.resize(face.size());
  for (int i = 0; i < face.size(); ++i)
    facenorm[i] = normalize((vert[face[i].v[2]] - vert[face[i].v[0]]) % (vert[face[i].v[1]] - vert[face[i].v[0]]));
  return true;
}

#if COMPUTE_NORMALS_LIKE_MAX
static void compute_vertex_normals_MAX(Mesh *mesh);
#endif

bool MeshData::calc_vertnorms()
{
  if (!areNormalsComputed())
    return true;

  // if(!calc_ngr()) return 0;
  if (!face.size())
    return 0;
  if (!calc_facenorms())
    return 0;
#if COMPUTE_NORMALS_LIKE_MAX
  compute_vertex_normals_MAX(this);
#else
  //
  if (vertnorm.size() != ngr.size())
    vertnorm.resize(ngr.size());
  for (int i = 0; i < ngr.size(); ++i)
  {
    Point3 n(0, 0, 0);
    int k = ngr[i];
    int c = ntab[k++];
    for (; c; --c, ++k)
      n += facenorm[ntab[k]];
    vertnorm[i] = normalize(n);
  }
#endif
  return true;
}

#if COMPUTE_NORMALS_LIKE_MAX

class MeshVNormal
{
public:
  Point3 norm;
  uint32_t smooth;
  MeshVNormal *next;
  bool init;
  int index, fi;
  MeshVNormal()
  {
    smooth = 0;
    next = NULL;
    init = false;
    index = 0;
    norm = Point3(0, 0, 0);
  }

  MeshVNormal(Point3 &n, uint32_t s, int f)
  {
    next = NULL;
    init = true;
    norm = n;
    smooth = s;
    fi = f;
  }

  ~MeshVNormal()
  {
    if (next)
      delete next;
    next = NULL;
  }

  void AddNormal(Point3 &n, uint32_t s, int fi);

  Point3 &GetNormal(uint32_t s, int f);

  void Normalize();
  void SetIndex(int &ind)
  {
    index = ind;
    ind++;
    if (next)
      next->SetIndex(ind);
  }
  int GetIndex(uint32_t s, int f)
  {
    if (!next)
      return index;
    if (!smooth && f == fi)
      return index;
    if (smooth & s)
      return index;
    else
      return next->GetIndex(s, f);
  }
};
DAG_DECLARE_RELOCATABLE(MeshVNormal);

// Add a normal to the list if the smoothing group bits overlap,
// otherwise create a new vertex normal in the list
void MeshVNormal::AddNormal(Point3 &n, uint32_t s, int f)
{

  if (!(s & smooth) && init)
  {
    if (next)
      next->AddNormal(n, s, f);
    else
      next = new MeshVNormal(n, s, f);
  }
  else
  {
    norm += n;
    smooth |= s;
    fi = f;
    init = true;
  }
}

// Retrieves a normal if the smoothing groups overlap or there is
// only one in the list

Point3 &MeshVNormal::GetNormal(uint32_t s, int f)
{
  if (!next)
    return norm;
  if (!smooth && f == fi)
    return norm;
  if (smooth & s)
    return norm;
  else
    return next->GetNormal(s, f);
}


// Normalize each normal in the list

void MeshVNormal::Normalize()
{
  MeshVNormal *ptr = next, *prev = this;
  while (ptr)
  {
    if (ptr->smooth & smooth)
    {
      norm += ptr->norm;
      prev->next = ptr->next;
      delete ptr;
      ptr = prev->next;
    }
    else
    {
      prev = ptr;
      ptr = ptr->next;
    }
  }

  norm = normalize(norm);
  if (next)
    next->Normalize();
}

// Compute the face and vertex normals

static void compute_vertex_normals_MAX(Mesh *mesh)
{
  if (mesh->facenorm.size() != mesh->face.size())
    mesh->calc_facenorms();

  Face *face;
  Point3 v0, v1, v2;
  Tab<MeshVNormal> vnorms(tmpmem);
  vnorms.resize(mesh->vert.size());

  mesh->facengr.resize(mesh->face.size());

  face = mesh->face.data();


  // Compute face and vertex surface normals

  for (int i = 0; i < mesh->face.size(); i++, face++)
    for (int j = 0; j < 3; j++)
      vnorms[face->v[j]].AddNormal(mesh->facenorm[i], face->smgr, i);

  int totalnorms = 0;
  for (int i = 0; i < mesh->vert.size(); i++)
  {
    vnorms[i].Normalize();
    vnorms[i].SetIndex(totalnorms);
  }
  mesh->vertnorm.resize(totalnorms);

  face = mesh->face.data();
  for (int i = 0; i < mesh->face.size(); i++, face++)
  {
    for (int j = 0; j < 3; j++)
    {
      mesh->facengr[i][j] = vnorms[face->v[j]].GetIndex(face->smgr, i);
      mesh->vertnorm[mesh->facengr[i][j]] = vnorms[face->v[j]].GetNormal(face->smgr, i);
    }
  }
}

#endif

real MeshData::calc_mesh_rad(Point3 &c)
{
  BBox3 b;
  int i;
  for (i = 0; i < vert.size(); ++i)
    b += vert[i];
  c = b.center();
  real r2 = 0;
  for (i = 0; i < vert.size(); ++i)
  {
    real d = lengthSq(vert[i] - c);
    if (d > r2)
      r2 = d;
  }
  return sqrtf(r2);
}

void MeshData::flip_normals(int sf, int nf)
{
  if (nf < 0)
    nf = face.size() - sf;
  int ef = sf + nf, i;
  for (i = sf; i < ef; ++i)
  {
    int v = face[i].v[1];
    face[i].v[1] = face[i].v[2];
    face[i].v[2] = v;
  }
  if (faceflg.size())
    for (i = sf; i < ef; ++i)
      faceflg[i] = (faceflg[i] & ~5) | ((faceflg[i] & 1) << 2) | ((faceflg[i] & 4) >> 2);
  if (cface.size())
    for (i = sf; i < ef; ++i)
    {
      int v = cface[i].t[1];
      cface[i].t[1] = cface[i].t[2];
      cface[i].t[2] = v;
    }
  if (facengr.size() == face.size())
    for (i = sf; i < ef; ++i)
    {
      int v = facengr[i][1];
      facengr[i][1] = facengr[i][2];
      facengr[i][2] = v;
    }
  int t;
  for (t = 0; t < NUMMESHTCH; ++t)
    if (tface[t].size())
      for (i = sf; i < ef; ++i)
      {
        int v = tface[t][i].t[1];
        tface[t][i].t[1] = tface[t][i].t[2];
        tface[t][i].t[2] = v;
      }
  for (t = 0; t < extra.size(); ++t)
    if (extra[t].fc.size())
      for (i = sf; i < ef; ++i)
      {
        int v = extra[t].fc[i].t[1];
        extra[t].fc[i].t[1] = extra[t].fc[i].t[2];
        extra[t].fc[i].t[2] = v;
      }
  for (i = sf; i < facenorm.size() && i < ef; ++i)
    facenorm[i] = -facenorm[i];

  if (sf == 0 && nf == face.size())
    for (i = 0; i < vertnorm.size(); ++i)
      vertnorm[i] = -vertnorm[i];
  else if (facengr.size() == face.size())
  {
    Bitarray vused;
    vused.resize(vertnorm.size());
    for (i = sf; i < ef; ++i)
    {
      vused.set(facengr[i][0]);
      vused.set(facengr[i][1]);
      vused.set(facengr[i][2]);
    }
    for (i = 0; i < vertnorm.size(); ++i)
      if (vused[i])
        vertnorm[i] = -vertnorm[i];
  }
}

int MeshData::force_mapping(int ch)
{
  if (tface[ch].size() != face.size())
  {
    int nf = face.size();
    tface[ch].resize(nf);
    for (int i = 0; i < nf; ++i)
      for (int j = 0; j < 3; ++j)
        tface[ch][i].t[j] = 0;
    tvert[ch].resize(1);
    tvert[ch][0] = Point2(0, 0);
  }
  return 1;
}

struct GetFaceVert
{
  static uint32_t &get(Face &f, int vi) { return f.v[vi]; }
};
struct GetTFaceVert
{
  static uint32_t &get(TFace &f, int vi) { return f.t[vi]; }
};
struct GetNFaceVert
{
  static uint32_t &get(FaceNGr &f, int vi) { return f[vi]; }
};
static inline float lengthSq(float a) { return a * a; }

template <typename T>
struct VertexHasher
{
  size_t __forceinline operator()(const T &p) const { return mum_hash(&p, sizeof(p), 0); }
};

template <class GetFaceVert, class Face, class T>
static void optimize_tface_base(Tab<int> &nv, Tab<int> &map, Bitarray &vfm, Tab<Face> &tf, T *tvert, int tvertn, float threshold)
{
  vfm.resize(tvertn);
  vfm.reset();
  for (auto &f : tf)
  {
    vfm.set(GetFaceVert::get(f, 0));
    vfm.set(GetFaceVert::get(f, 1));
    vfm.set(GetFaceVert::get(f, 2));
  }
  nv.clear();
  nv.reserve(tvertn);
  map.resize(tvertn);
  bool chg = false;

  eastl::hash_map<T, int, VertexHasher<T>> vertex_hash_map;
  if (threshold == 0.0f)
    vertex_hash_map.reserve(tvertn);

  for (int i = 0; i < tvertn; ++i)
  {
    if (!vfm[i])
    {
      map[i] = 0;
      chg = true;
      continue;
    }
    if (threshold < 0.0f)
    {
      map[i] = nv.size();
      nv.push_back(i);
    }
    else if (threshold == 0.0f)
    {
      int j = (int)vertex_hash_map.size();
      G_ASSERT(j == nv.size());
      auto insert_pair = vertex_hash_map.emplace(tvert[i], j);
      if (insert_pair.second)
        nv.push_back(i);
      else
        chg = true;
      map[i] = insert_pair.first->second;
    }
    else
    {
      int j;
      for (j = 0; j < nv.size(); ++j)
      {
        if (lengthSq(tvert[nv[j]] - tvert[i]) <= threshold)
        {
          chg = true;
          break;
        }
      }
      if (j >= nv.size())
      {
        j = nv.size();
        nv.push_back(i);
      }
      map[i] = j;
    }
  }

  if (!chg)
    return;
  for (int i = 0; i < nv.size(); ++i)
    if (nv[i] != i)
      tvert[i] = tvert[nv[i]];
  for (auto &f : tf)
  {
    GetFaceVert::get(f, 0) = map[GetFaceVert::get(f, 0)];
    GetFaceVert::get(f, 1) = map[GetFaceVert::get(f, 1)];
    GetFaceVert::get(f, 2) = map[GetFaceVert::get(f, 2)];
  }
}

template <class GetFaceVert, class Face, class T>
static void optimize_tface(Tab<int> &nv, Tab<int> &map, Bitarray &vfm, Tab<Face> &tf, Tab<T> &tvert, float threshold, bool shrink)
{
  optimize_tface_base<GetFaceVert>(nv, map, vfm, tf, tvert.data(), tvert.size(), threshold);
  tvert.resize(nv.size());
  if (shrink)
    tvert.shrink_to_fit();
}

void MeshData::optimize_tverts(float sqThreshold)
{
  Tab<int> nv(tmpmem), map(tmpmem);
  Bitarray vfm;
  for (int ch = 0; ch < NUMMESHTCH; ++ch)
    if (tface[ch].size())
      ::optimize_tface<GetTFaceVert>(nv, map, vfm, tface[ch], tvert[ch], sqThreshold, false);

  if (cface.size())
    ::optimize_tface<GetTFaceVert>(nv, map, vfm, cface, cvert, sqThreshold, false);
  if (facengr.size())
    ::optimize_tface<GetNFaceVert>(nv, map, vfm, facengr, vertnorm, sqThreshold, false);
}

bool MeshData::optimize_extra(int id, float sqThreshold)
{
  if (id < 0 || id >= extra.size())
    return false;
  Tab<int> nv(tmpmem), map(tmpmem);
  Bitarray vfm;
  MeshData::ExtraChannel &channel = extra[id];
  switch (channel.type)
  {
    case CHT_FLOAT1:
      ::optimize_tface_base<GetTFaceVert>(nv, map, vfm, channel.fc, (float *)&channel.vt[0], channel.numverts(), sqThreshold);
      break;
    case CHT_FLOAT2:
      ::optimize_tface_base<GetTFaceVert>(nv, map, vfm, channel.fc, (Point2 *)&channel.vt[0], channel.numverts(), sqThreshold);
      break;
    case CHT_FLOAT3:
      ::optimize_tface_base<GetTFaceVert>(nv, map, vfm, channel.fc, (Point3 *)&channel.vt[0], channel.numverts(), sqThreshold);
      break;
    case CHT_FLOAT4:
      ::optimize_tface_base<GetTFaceVert>(nv, map, vfm, channel.fc, (Point4 *)&channel.vt[0], channel.numverts(), sqThreshold);
      break;
    case CHT_E3DCOLOR:
      ::optimize_tface_base<GetTFaceVert>(nv, map, vfm, channel.fc, (E3DCOLOR *)&channel.vt[0], channel.numverts(), sqThreshold);
      break;
    default: return false;
  }
  channel.resize_verts(nv.size());
  return true;
}

void MeshData::kill_unused_verts(float sqThreshold)
{
  Tab<int> nv(tmpmem), map(tmpmem);
  Bitarray vfm;
  ::optimize_tface<GetFaceVert>(nv, map, vfm, face, vert, sqThreshold, false);
}

struct RemoveDegenerate
{
  float faceAreaThres;
  RemoveDegenerate(float fath) : faceAreaThres(fath) {}
  bool can_remove(const Face &f, dag::ConstSpan<Point3> verts, int) const
  {
    if (f.v[0] == f.v[1] || f.v[0] == f.v[2] || f.v[1] == f.v[2])
      return true;

    vec3f v0 = v_ldu(&verts[f.v[0]].x);
    vec3f v1 = v_ldu(&verts[f.v[1]].x);
    vec3f v2 = v_ldu(&verts[f.v[2]].x);
    vec3f fa = v_cross3(v_sub(v1, v0), v_sub(v2, v0));
    if (DAGOR_UNLIKELY(v_extract_x(v_length3_sq_x(fa)) < faceAreaThres))
      return true;

    return false;
  }
};
template <class CanRemove>
static void remove_faces(MeshData &m, const CanRemove &c)
{
#define COPY(arr) \
  if (arr.size()) \
    memmove(arr.data() + i - delta, arr.data() + i, elem_size(arr) * count);
  int nf = m.face.size();
  dag::ConstSpan<Point3> verts(m.vert);
  int delta = 0;
  for (int i = 0; i < nf; ++i)
  {
    if (!c.can_remove(m.face[i], verts, i))
    {
      int j = i + 1;
      for (; j < nf; ++j)
        if (c.can_remove(m.face[j], verts, j))
          break;
      if (delta)
      {
        int count = j - i;
        COPY(m.face);
        COPY(m.facenorm)
        COPY(m.faceflg)
        COPY(m.cface)

        for (int cc = 0; cc < NUMMESHTCH; ++cc)
          COPY(m.tface[cc])

        for (int cc = 0; cc < m.extra.size(); ++cc)
          COPY(m.extra[cc].fc)

        COPY(m.facengr)
      }
      i = j - 1;
      continue;
    }
    int j = i + 1;
    for (; j < nf; ++j)
      if (!c.can_remove(m.face[j], verts, j))
        break;
    delta += j - i;
    i = j - 1;
  }
#undef COPY
  if (!delta)
    return;
  erase_items_fast(m.face, m.face.size() - delta, delta);

#define ERASE(arr) \
  if (arr.size())  \
    erase_items_fast(arr, m.face.size(), delta);

  ERASE(m.facengr)
  ERASE(m.facenorm)
  ERASE(m.faceflg)
  ERASE(m.cface)

  for (int cc = 0; cc < NUMMESHTCH; ++cc)
    ERASE(m.tface[cc])

  for (int cc = 0; cc < m.extra.size(); ++cc)
    ERASE(m.extra[cc].fc)
#undef ERASE
}

void MeshData::kill_bad_faces(float fa_thres) { remove_faces(*this, RemoveDegenerate(fa_thres)); }

void MeshData::kill_bad_faces2(float fa_thresh, float fa_to_check_thresh, float fa_to_perim_ratio_thresh)
{
  struct RemoveDegenerateByPerimeterToFaceAreaRatio
  {
    float faceAreaThres;
    float faceAreaCheckThres;
    float faceAreaToPerimeterRatioThres;
    bool can_remove(const Face &f, dag::ConstSpan<Point3> verts, int) const
    {
      if (f.v[0] == f.v[1] || f.v[0] == f.v[2] || f.v[1] == f.v[2])
        return true;

      vec3f v0 = v_ldu(&verts[f.v[0]].x);
      vec3f v1 = v_ldu(&verts[f.v[1]].x);
      vec3f v2 = v_ldu(&verts[f.v[2]].x);
      vec3f v10 = v_sub(v1, v0);
      vec3f v20 = v_sub(v2, v0);
      vec3f vfa2 = v_length3_sq_x(v_cross3(v10, v20));
      if (DAGOR_UNLIKELY(v_extract_x(vfa2) < faceAreaCheckThres))
      {
        if (v_extract_x(vfa2) < faceAreaThres)
          return true;
        vec4f v21fa2 = v_perm_xaxa(v_length3_sq_x(v_sub(v2, v1)), vfa2);
        vec4f vpfa = v_sqrt(v_perm_xyab(v_perm_xaxa(v_length3_sq_x(v10), v_length3_sq_x(v20)), v21fa2));
        if (v_extract_x(v_hadd3_x(vpfa)) / v_extract_w(vpfa) > faceAreaToPerimeterRatioThres)
          return true;
      }

      return false;
    }
  };
  RemoveDegenerateByPerimeterToFaceAreaRatio cb{fa_thresh, fa_to_check_thresh, fa_to_perim_ratio_thresh};
  remove_faces(*this, cb);
}

class RemoveUnused
{
  const Bitarray *used;

public:
  RemoveUnused(const Bitarray &us) : used(&us) {}
  inline bool can_remove(const Face &, dag::ConstSpan<Point3>, int fi) const { return used->get(fi) ? false : true; }
};
void MeshData::removeFacesFast(const Bitarray &used)
{
  if (used.size() != face.size())
    return;
  int old_faces = face.size();
  remove_faces(*this, RemoveUnused(used));
  if (old_faces != face.size() && ntab.size()) // if ntab was build for older faces we shall force rebuilding it
  {
    ntab.clear();
    ngr.clear();
  }
}


int MeshData::check_mesh()
{
  int i, j, c;
  for (i = 0; i < face.size(); ++i)
    for (j = 0; j < 3; ++j)
      if (face[i].v[j] >= vert.size())
      {
        debug("face %d has invalid vertex %u", i, face[i].v[j]);
        return 0;
      }
  for (c = 0; c < NUMMESHTCH; ++c)
    for (i = 0; i < tface[c].size(); ++i)
      for (j = 0; j < 3; ++j)
        if (tface[c][i].t[j] >= tvert[c].size())
        {
          debug("tface#%d %d has invalid vertex %u", c, i, tface[c][i].t[j]);
          return 0;
        }
  for (i = 0; i < cface.size(); ++i)
    for (j = 0; j < 3; ++j)
      if (cface[i].t[j] >= cvert.size())
      {
        debug("cface %d has invalid vertex %u", i, cface[i].t[j]);
        return 0;
      }
  for (i = 0; i < facengr.size(); ++i)
    for (j = 0; j < 3; ++j)
      if (facengr[i][j] >= vertnorm.size())
      {
        debug("facengr %d has invalid ngr %u", i, facengr[i][j]);
        return 0;
      }
  return 1;
}

struct SplittedVertex
{
  int fromFace;
  float edgeK;
  short v0, v1;
  int newV;
  SplittedVertex() = default;
  SplittedVertex(int f, int v0_, int v1_, float k) : fromFace(f), v0(v0_), v1(v1_), edgeK(k) {}
};

struct SplittedFace
{
  unsigned fromFace : 31;
  unsigned splitted : 1;
  int v[3]; // positive - splitted vert, negative - original vert
  SplittedFace() = default;
  SplittedFace(int f) : fromFace(f), splitted(0) {}
  SplittedFace(int fi, bool) : fromFace(fi), splitted(1) { v[0] = v[1] = v[2] = -1; }
};

template <class T>
static int interpolate_edge(Tab<T> &vert, uint32_t *v, const SplittedVertex &sv)
{
  int s_idx = vert.size();
  vert.push_back(lerp(vert[v[sv.v0]], vert[v[sv.v1]], sv.edgeK));
  return s_idx;
}

template <class T, class TF>
static void produce_faces(Tab<T> &vert, Tab<TF> &resface, const Tab<TF> &mface,
  const Tab<Face> &mgface, // geom faces
  SplittedFace *sf, int fcnt, dag::Span<SplittedVertex> sv)
{
  for (int si = 0; si < sv.size(); ++si)
    sv[si].newV = -1;
  for (int fi = 0; fi < fcnt; ++fi, ++sf)
  {
    memcpy(resface.data() + fi, mface.data() + sf->fromFace, elem_size(resface));
    if (!sf->splitted)
      continue;
    for (int vi = 0; vi < 3; ++vi)
    {
      if (sf->v[vi] < 0)
        continue;
      SplittedVertex &svrt = sv[sf->v[vi]];
      uint32_t *mfv = (uint32_t *)(mface.data() + svrt.fromFace); //==dirty hack: assuming indices are in the beginning of all faces
      if (svrt.newV < 0)
      {
        int v0 = mfv[svrt.v0], v1 = mfv[svrt.v1];

        int gv0 = mgface[svrt.fromFace].v[svrt.v0], gv1 = mgface[svrt.fromFace].v[svrt.v1];
        for (int si = 0; si < sv.size(); ++si)
        {
          if (sv[si].newV < 0)
            continue;
          int ffi = sv[si].fromFace;
          uint32_t *mfv2 = (uint32_t *)(mface.data() + ffi); //==dirty hack: assuming indices are in the beginning of all faces
          int v02 = mfv2[sv[si].v0], v12 = mfv2[sv[si].v1];
          int gv02 = mgface[ffi].v[sv[si].v0], gv12 = mgface[ffi].v[sv[si].v1];
          // either reverse or straight order of edge, and both edges (geom and atrributes) are common for faces
          if ((gv02 == gv0 && gv12 == gv1 && v02 == v0 && v12 == v1) || (gv02 == gv1 && gv12 == gv0 && v02 == v1 && v12 == v0))
          {
            svrt.newV = sv[si].newV;
            break;
          }
        }
        if (svrt.newV < 0)
          svrt.newV = ::interpolate_edge(vert, mfv, svrt);
      }
      uint32_t *fv = (uint32_t *)(resface.data() + fi);
      fv[vi] = svrt.newV;
    }
  }
}

static void produce_mesh(MeshData &res, const MeshData &m, SplittedFace *sf, int fcnt, dag::Span<SplittedVertex> sv)
{
  res.setNumFaces(fcnt, m, false);
  //== todo: we can know, what vertices are really used.
  res.vert = m.vert;
  res.cvert = m.cvert;
  res.vertnorm = m.vertnorm;
  int vertNormCount = res.vertnorm.size();

  for (int i = 0; i < NUMMESHTCH; ++i)
    if (res.tface[i].size())
      res.tvert[i] = m.tvert[i];
  clear_and_shrink(res.extra);                         //== extras not supported!
  G_ASSERT(&m.face[0].v[0] == (uint32_t *)&m.face[0]); // assert for dirty hack in produce_faces
  ::produce_faces(res.vert, res.face, m.face, m.face, sf, fcnt, sv);
  if (res.facengr.size())
    ::produce_faces(res.vertnorm, res.facengr, m.facengr, m.face, sf, fcnt, sv);
  if (res.cface.size())
    ::produce_faces(res.cvert, res.cface, m.cface, m.face, sf, fcnt, sv);
  for (int i = 0; i < NUMMESHTCH; ++i)
    if (res.tface[i].size())
      ::produce_faces(res.tvert[i], res.tface[i], m.tface[i], m.face, sf, fcnt, sv);

  if (res.faceflg.size())
    for (int fi = 0; fi < fcnt; ++fi, ++sf)
      res.faceflg[fi] = m.faceflg[sf->fromFace];

  // normalize new vert norms
  for (int i = vertNormCount; i < res.vertnorm.size(); ++i)
    res.vertnorm[i].normalize();
}

void Mesh::split(Mesh &nm, Mesh &pm, real A, real B, real C, real D, real eps)
{
  Point3 N(A, B, C);
  Tab<real> vd(tmpmem);
  Tab<char> vs(tmpmem);
  vd.resize(vert.size());
  vs.resize(vert.size());

  bool allNeg = true, allPos = true;
  for (int i = 0; i < vert.size(); ++i)
  {
    vd[i] = vert[i] * N + D;
    if (vd[i] > eps)
    {
      vs[i] = 1;
      allNeg = false;
    }
    else if (vd[i] < -eps)
    {
      vs[i] = 2;
      allPos = false;
    }
    else
    {
      vs[i] = 3;
      allNeg = allPos = false;
    }
  }

  if (allNeg)
  {
    nm = *this;
    return;
  }

  if (allPos)
  {
    pm = *this;
    return;
  }

  Tab<SplittedFace> posFaces(tmpmem), negFaces(tmpmem);
  posFaces.reserve(face.size());
  negFaces.reserve(face.size());
  Tab<SplittedVertex> splitV(tmpmem);

  for (int fi = 0; fi < face.size(); ++fi)
  {
    Face &f = face[fi];
    int pv = -1, nv = -1;
    for (int vi = 0; vi < 3; ++vi)
    {
      if (vs[f.v[vi]] == 1)
        pv = vi;
      else if (vs[f.v[vi]] == 2)
        nv = vi;
    }
    if (pv == -1)
    {
      //== keep triangle on negative side
      // nm.face.push_back(f);
      negFaces.push_back(fi);
      continue;
    }
    else if (nv == -1)
    {
      //== move triangle to positive side
      // pm.face.push_back(f);
      posFaces.push_back(fi);
      continue;
    }

    // cut triangle
    int ov = 0;
    if (ov == pv || ov == nv)
      ++ov;
    if (ov == pv || ov == nv)
      ++ov;
    float d[3] = {vd[f.v[0]], vd[f.v[1]], vd[f.v[2]]};

    if (vs[f.v[ov]] == 3)
    {
      // cut through ov
      float k = d[nv] / (d[nv] - d[pv]);
      int newV = splitV.size();
      splitV.push_back(SplittedVertex(fi, nv, pv, k));
      SplittedFace posf(fi, true), negf(fi, true);
      posf.v[nv] = newV;
      posFaces.push_back(posf);
      negf.v[pv] = newV;
      negFaces.push_back(negf);
    }
    else if (vs[f.v[ov]] == 1)
    {
      // 1 vertex on negative side
      float k = d[nv] / (d[nv] - d[pv]);
      int ppi = splitV.size();
      splitV.push_back(SplittedVertex(fi, nv, pv, k));

      k = d[nv] / (d[nv] - d[ov]);
      int opi = splitV.size();
      splitV.push_back(SplittedVertex(fi, nv, ov, k));

      SplittedFace posf1(fi, true), posf2(fi, true), negf(fi, true);
      posf1.v[nv] = ppi;
      posFaces.push_back(posf1);

      posf2.v[pv] = ppi;
      posf2.v[nv] = opi;
      posFaces.push_back(posf2);

      negf.v[pv] = ppi;
      negf.v[ov] = opi;
      negFaces.push_back(negf);
    }
    else
    {
      // 1 vertex on positive side
      float k = d[nv] / (d[nv] - d[pv]);

      int npi = splitV.size();
      splitV.push_back(SplittedVertex(fi, nv, pv, k));

      k = d[ov] / (d[ov] - d[pv]);
      int opi = splitV.size();
      splitV.push_back(SplittedVertex(fi, ov, pv, k));

      SplittedFace posf(fi, true), negf1(fi, true), negf2(fi, true);
      posf.v[nv] = npi;
      posf.v[ov] = opi;
      posFaces.push_back(posf);

      negf1.v[pv] = npi;
      negFaces.push_back(negf1);

      negf2.v[nv] = npi;
      negf2.v[pv] = opi;
      negFaces.push_back(negf2);
    }
  }

  if (posFaces.size() == 0)
  {
    nm = *this;
    return;
  }

  if (negFaces.size() == 0)
  {
    pm = *this;
    return;
  }

  ::produce_mesh(nm, *this, negFaces.data(), negFaces.size(), make_span(splitV));
  ::produce_mesh(pm, *this, posFaces.data(), posFaces.size(), make_span(splitV));
  nm.kill_unused_verts();
  pm.kill_unused_verts();
}


static void calculateTangentSpaceVector(const Point3 &normal, const Point3 &position1, const Point3 &position2,
  const Point3 &position3, const Point2 &uv1, const Point2 &uv2, const Point2 &uv3, Point3 &tangent, Point3 &binormal)
{
  // side0 is the vector along one side of the triangle of vertices passed in,
  // and side1 is the vector along another side. Taking the cross product of these returns the normal.

  Point3 side1 = Point3(position2.x - position1.x, uv2.x - uv1.x, uv2.y - uv1.y);
  Point3 side2 = Point3(position3.x - position1.x, uv3.x - uv1.x, uv3.y - uv1.y);
  Point3 crossProduct = side1 % side2; // cross product
  tangent.x = -crossProduct.y / crossProduct.x;
  binormal.x = -crossProduct.z / crossProduct.x;

  side1 = Point3(position2.y - position1.y, uv2.x - uv1.x, uv2.y - uv1.y);
  side2 = Point3(position3.y - position1.y, uv3.x - uv1.x, uv3.y - uv1.y);
  crossProduct = side1 % side2; // cross product
  tangent.y = -crossProduct.y / crossProduct.x;
  binormal.y = -crossProduct.z / crossProduct.x;

  side1 = Point3(position2.z - position1.z, uv2.x - uv1.x, uv2.y - uv1.y);
  side2 = Point3(position3.z - position1.z, uv3.x - uv1.x, uv3.y - uv1.y);
  crossProduct = side1 % side2; // cross product
  tangent.z = -crossProduct.y / crossProduct.x;
  binormal.z = -crossProduct.z / crossProduct.x;

  tangent.normalize();
  binormal.normalize();

  Point3 tangentCross = tangent % binormal;
  float dot = tangentCross * normal;
  if (dot < 0.0f)
  {
    tangent = -tangent;
    binormal = -binormal;
  }
}


// calculate data for tangents & binormal & place it in new extra channel. if channel is exists,
// no work will be processed. return false, if any error occurs
bool MeshData::createTangentsData(int usage, int usage_index)
{
  int chId = find_extra_channel(usage, usage_index);
  if (chId >= 0)
    return false;

  chId = add_extra_channel(CHT_FLOAT3, usage, usage_index);
  if (chId < 0)
    return false;

  ExtraChannel &tangChannel = extra[chId];

  static int texChannel = 0; // teture channel used for caculation

  const int vertCount = face.size() * 3;

  tangChannel.resize_verts(vertCount);
  tangChannel.fc.resize(face.size());

  Tab<TFace> &tangface = tangChannel.fc;
  Point3 *tangvert = (Point3 *)&tangChannel.vt[0];

  for (int vertIndex = 0; vertIndex < vertCount; ++vertIndex)
  {
    tangvert[vertIndex] = Point3(0, 0, 0);
  }

  if (!facenorm.size())
    calc_facenorms();
  if (tface[texChannel].size() != face.size() || facenorm.size() != face.size())
    DAG_FATAL("No texture channel or not enough face in texture channel (face=%d norm=%d tface=%d)", face.size(), facenorm.size(),
      tface[texChannel].size());

  for (int faceIndex = 0, j = 0; faceIndex < face.size(); ++faceIndex)
  {
    Point3 v0 = vert[face[faceIndex].v[0]];
    Point3 v1 = vert[face[faceIndex].v[1]];
    Point3 v2 = vert[face[faceIndex].v[2]];

    Point2 uv0 = tvert[texChannel][tface[texChannel][faceIndex].t[0]]; // tc - 0
    Point2 uv1 = tvert[texChannel][tface[texChannel][faceIndex].t[1]]; // tc - 0
    Point2 uv2 = tvert[texChannel][tface[texChannel][faceIndex].t[2]]; // tc - 0

    Point3 tangent, binormal;
    calculateTangentSpaceVector(facenorm[faceIndex], v0, v1, v2, uv0, uv1, uv2, tangent, binormal);

    tangface[faceIndex].t[0] = j;
    tangvert[j] = tangent;
    j++;
    tangface[faceIndex].t[1] = j;
    tangvert[j] = tangent;
    j++;
    tangface[faceIndex].t[2] = j;
    tangvert[j] = tangent;
    j++;
    //        tangChannelPtr[faceIndex]=tangent;
    //        tangChannel.fc[faceIndex].t[0]=face[faceIndex].v[0];
    //        tangChannel.fc[faceIndex].t[1]=face[faceIndex].v[1];
    //        tangChannel.fc[faceIndex].t[2]=face[faceIndex].v[2];
    //        tangChannel.fc[faceIndex].t[0]=faceIndex;
    //        tangChannel.fc[faceIndex].t[1]=faceIndex;
    //        tangChannel.fc[faceIndex].t[2]=faceIndex;

    // average vertices
    // if(tangvert[j].lengthSq()>0.0f)
    //{
    // tangvert[j]=tangvert[j]+tangent;
    // tangvert[j].normalize();
    // }
  }

  return true;
}

bool MeshData::createTextureSpaceData(int usage, int usage_index_du, int usage_index_dv)
{
  static const int texChannel = 0; // texture channel used for caculation
  if (tface[texChannel].size() != face.size())
    return false;
  kill_bad_faces();

  int chIdDu = find_extra_channel(usage, usage_index_du);
  if (chIdDu >= 0)
    return false;

  int chIdDv = find_extra_channel(usage, usage_index_dv);
  if (chIdDv >= 0)
    return false;

  chIdDu = add_extra_channel(CHT_FLOAT4, usage, usage_index_du);
  if (chIdDu < 0)
    return false;

  chIdDv = add_extra_channel(CHT_FLOAT3, usage, usage_index_dv);
  if (chIdDv < 0)
    return false;

  ExtraChannel &tangChannelDu = extra[chIdDu];
  ExtraChannel &tangChannelDv = extra[chIdDv];

  if (facengr.size() != face.size())
    calc_ngr();
  if (facenorm.size() != face.size())
    calc_facenorms();
  if (!vertnorm.size())
    calc_vertnorms();

  tangChannelDu.fc.resize(face.size());
  tangChannelDv.fc.resize(face.size());

  mem_set_ff(tangChannelDu.fc);

  Tab<TFace> &tangfaceDu = tangChannelDu.fc;
  Tab<TFace> &tangfaceDv = tangChannelDv.fc;
  VertToFaceVertMap map;
  buildVertToFaceVertMap(map);

  Tab<IPoint3> bvert(tmpmem);
  int append_vertex_count = 0;

  for (int vni = 0; vni < vert.size(); ++vni)
  {
    int lv = bvert.size();
    int lbv = append_vertex_count;

    for (int numf = map.getNumFaces(vni), i = 0; numf; ++i, --numf)
    {
      int fid = map.getVertFaceIndex(vni, i);
      int vi = fid % 3, fi = fid / 3;

      IPoint3 ind;
      ind[0] = vni;
      ind[1] = tface[texChannel][fi].t[vi];
      ind[2] = facengr[fi][vi];

      int id, bi;
      for (id = lv, bi = lbv; id < bvert.size(); id++, ++bi)
        if (ind == bvert[id])
          break;

      if (id >= bvert.size())
      {
        bvert.push_back(ind);
        bi = append_vertex_count++;
      }
      tangfaceDu[fi].t[vi] = bi;
      tangfaceDv[fi].t[vi] = bi;
    }
    // bvert.clear();
  }

  tangChannelDu.resize_verts(append_vertex_count);
  tangChannelDv.resize_verts(append_vertex_count);
  if (!append_vertex_count)
    return true;
  Point4 *tangvertDu = (Point4 *)tangChannelDu.vt.data();
  Point3 *tangvertDv = (Point3 *)tangChannelDv.vt.data();

  mem_set_0(tangChannelDu.vt);
  mem_set_0(tangChannelDv.vt);
  for (int faceIndex = 0; faceIndex < face.size(); ++faceIndex)
  {
    Point3 v0 = vert[face[faceIndex].v[0]];
    Point3 v1 = vert[face[faceIndex].v[1]];
    Point3 v2 = vert[face[faceIndex].v[2]];

    Point2 uv0 = tvert[texChannel][tface[texChannel][faceIndex].t[0]]; // tc - 0
    Point2 uv1 = tvert[texChannel][tface[texChannel][faceIndex].t[1]]; // tc - 0
    Point2 uv2 = tvert[texChannel][tface[texChannel][faceIndex].t[2]]; // tc - 0

    Point3 du, dv;
    // calculateTangentSpaceVector(facenorm[faceIndex],v0,v1,v2,uv0,uv1,uv2, du, dv);
    calculate_dudv(v0, v1, v2, uv0, uv1, uv2, du, dv);
    for (int vi = 0; vi < 3; ++vi)
    {
      tangvertDu[tangfaceDu[faceIndex].t[vi]] += Point4(du.x, du.y, du.z, 0);
      tangvertDv[tangfaceDv[faceIndex].t[vi]] += dv;
    }
  }

  for (int vertIndex = 0; vertIndex < tangChannelDv.numverts(); ++vertIndex)
  {
    tangvertDv[vertIndex].normalize();
    Point3 p = normalize(Point3(tangvertDu[vertIndex].x, tangvertDu[vertIndex].y, tangvertDu[vertIndex].z));
    float sgncross = (p % tangvertDv[vertIndex]) * vertnorm[bvert[vertIndex][2]] >= 0.0f ? 1.0f : -1.0f;

    tangvertDu[vertIndex] = Point4(p.x, p.y, p.z, sgncross);
  }
  return true;
}


void MeshData::ExtraChannel::save(IGenSave &cb)
{
  cb.write(&type, sizeof(type));
  cb.write(&vsize, sizeof(vsize));
  cb.write(&usg, sizeof(usg));
  cb.write(&usgi, sizeof(usgi));

  cb.writeTab(fc);
  cb.writeTab(vt);
}


void MeshData::ExtraChannel::load(IGenLoad &cb)
{
  cb.read(&type, sizeof(type));
  cb.read(&vsize, sizeof(vsize));
  cb.read(&usg, sizeof(usg));
  cb.read(&usgi, sizeof(usgi));

  cb.readTab(fc);
  cb.readTab(vt);
}


void MeshData::saveData(IGenSave &cb)
{
  cb.writeInt(2); // version

  cb.writeTab(vert);
  cb.writeTab(face);

  cb.writeInt(NUMMESHTCH);
  for (int i = 0; i < NUMMESHTCH; ++i)
  {
    cb.writeTab(tvert[i]);
    cb.writeTab(tface[i]);
  }

  cb.writeTab(cvert);
  cb.writeTab(cface);

  cb.writeTab(ntab);
  cb.writeTab(ngr);

  cb.writeTab(facengr);
  cb.writeTab(facenorm);
  cb.writeTab(vertnorm);
  cb.writeTab(faceflg);

  cb.writeInt(extra.size());

  for (int i = 0; i < extra.size(); ++i)
    extra[i].save(cb);
}


void MeshData::loadData(IGenLoad &cb)
{
  int version = cb.readInt();
  if (version != 2 && version != 1)
    DAGOR_THROW(IGenLoad::LoadException("wrond mesh version", cb.tell()));

  cb.readTab(vert);
  cb.readTab(face);

  int numTch = cb.readInt();
  int i;
  for (i = 0; i < numTch && i < NUMMESHTCH; ++i)
  {
    cb.readTab(tvert[i]);
    cb.readTab(tface[i]);
  }

  for (; i < numTch; ++i)
  {
    int n;
    n = cb.readInt();
    cb.seekrel(n * elem_size(tvert[0]));
    n = cb.readInt();
    cb.seekrel(n * elem_size(tface[0]));
  }

  if (version < 2)
  {
    // version 1: cvert was Tab<Point3>
    cvert.resize(cb.readInt());
    for (i = 0; i < cvert.size(); i++)
    {
      cb.read(&cvert[i], sizeof(Point3));
      cvert[i].a = 0;
    }
  }
  else
    cb.readTab(cvert);

  cb.readTab(cface);

  cb.readTab(ntab);
  cb.readTab(ngr);

  cb.readTab(facengr);
  cb.readTab(facenorm);
  cb.readTab(vertnorm);
  cb.readTab(faceflg);

  extra.resize(cb.readInt());
  for (i = 0; i < extra.size(); ++i)
    extra[i].load(cb);
}


void MeshData::attach(const MeshData &m)
{
  G_ASSERTF(face.size() == faceflg.size() || !faceflg.size(), "face.size()=%d faceflg.size()=%d", face.size(), faceflg.size());
  G_ASSERTF(m.face.size() == m.faceflg.size() || !m.faceflg.size(), "m.face.size()=%d m.faceflg.size()=%d", m.face.size(),
    m.faceflg.size());
  int vo, fi;

  const bool ourNormalsComputed = facengr.size() == face.size() && (vertnorm.size() || !face.size());
  const bool theirNormalsComputed = m.facengr.size() == m.face.size() && (m.vertnorm.size() || !m.face.size());
  if (!ourNormalsComputed && !theirNormalsComputed)
  {
    facengr.clear();
    vertnorm.clear();
  }
  else
  {
    if (!ourNormalsComputed)
    {
      // that's really not good we are loosing information
      if (facengr.size() != face.size())
        calc_ngr();
      calc_vertnorms();
    }
    if (!theirNormalsComputed)
    {
      // that's really not good we are loosing information
      MeshData tmp = m;
      if (tmp.facengr.size() != tmp.face.size())
        tmp.calc_ngr();
      tmp.calc_vertnorms();
      vo = append_items(vertnorm, tmp.vertnorm.size(), tmp.vertnorm.data());
      fi = append_items(facengr, tmp.facengr.size(), tmp.facengr.data());
    }
    else
    {
      vo = append_items(vertnorm, m.vertnorm.size(), m.vertnorm.data());
      fi = append_items(facengr, m.facengr.size(), m.facengr.data());
    }
    for (; fi < facengr.size(); ++fi)
    {
      facengr[fi][0] += vo;
      facengr[fi][1] += vo;
      facengr[fi][2] += vo;
    }
    G_ASSERTF(facengr.size() == face.size() + m.face.size(), "facengr=%d face=%d m.face=%d", facengr.size(), face.size(),
      m.face.size());
    ngr.clear();
  }
  // adding normals should be BEFORE adding faces, otherwise we can't 'calculate' vertex normals, after geometry is already attached

  int numf = face.size();

  vo = append_items(vert, m.vert.size(), m.vert.data());
  fi = append_items(face, m.face.size(), m.face.data());

  if (!numf || faceflg.size())
  {
    if (m.faceflg.size())
      append_items(faceflg, m.faceflg.size(), m.faceflg.data());
    else if (m.face.size())
    {
      int f = append_items(faceflg, m.face.size());
      memset(&faceflg[f], FACEFLG_ALLEDGES, faceflg.size() - f);
    }
  }
  G_ASSERTF(face.size() == faceflg.size() || !faceflg.size(), "face.size()=%d faceflg.size()=%d (added m.face=%d m.faceflg=%d)",
    face.size(), faceflg.size(), m.face.size(), m.faceflg.size());

  for (; fi < face.size(); ++fi)
  {
    face[fi].v[0] += vo;
    face[fi].v[1] += vo;
    face[fi].v[2] += vo;
  }

  for (int ch = 0; ch < NUMMESHTCH; ++ch)
  {
    if (tvert[ch].size())
      vo = append_items(tvert[ch], m.tvert[ch].size(), m.tvert[ch].data());
    else
      tvert[ch] = m.tvert[ch];

    if (tface[ch].size())
    {
      if (m.tface[ch].size())
      {
        fi = append_items(tface[ch], m.tface[ch].size(), m.tface[ch].data());
        for (; fi < tface[ch].size(); ++fi)
        {
          tface[ch][fi].t[0] += vo;
          tface[ch][fi].t[1] += vo;
          tface[ch][fi].t[2] += vo;
        }
      }
      else
      {
        fi = tface[ch].size();
        tface[ch].resize(face.size());
        for (; fi < tface[ch].size(); ++fi)
        {
          tface[ch][fi].t[0] = 0;
          tface[ch][fi].t[1] = 0;
          tface[ch][fi].t[2] = 0;
        }
      }
    }
    else if (m.tface[ch].size())
    {
      tface[ch].resize(face.size());
      for (fi = 0; fi < numf; ++fi)
      {
        tface[ch][fi].t[0] = 0;
        tface[ch][fi].t[1] = 0;
        tface[ch][fi].t[2] = 0;
      }

      memcpy(&tface[ch][fi], &m.tface[ch][0], (face.size() - fi) * elem_size(tface[ch]));
    }
  }

  if (cface.size())
  {
    if (m.cface.size())
    {
      vo = append_items(cvert, m.cvert.size(), m.cvert.data());
      fi = append_items(cface, m.cface.size(), m.cface.data());
      for (; fi < cface.size(); ++fi)
      {
        cface[fi].t[0] += vo;
        cface[fi].t[1] += vo;
        cface[fi].t[2] += vo;
      }
    }
    else
    {
      fi = cface.size();
      cface.resize(face.size());
      for (; fi < cface.size(); ++fi)
      {
        cface[fi].t[0] = 0;
        cface[fi].t[1] = 0;
        cface[fi].t[2] = 0;
      }
    }
  }
  else if (m.cface.size())
  {
    cvert = m.cvert;

    cface.resize(face.size());
    for (fi = 0; fi < numf; ++fi)
    {
      cface[fi].t[0] = 0;
      cface[fi].t[1] = 0;
      cface[fi].t[2] = 0;
    }

    memcpy(&cface[fi], &m.cface[0], (face.size() - fi) * elem_size(cface));
  }

  //== attach extra channels
}
