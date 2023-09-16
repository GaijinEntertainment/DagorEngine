#include <libTools/dagFileRW/sceneImpIface.h>
#include <3d/dag_drv3d.h>
#include <math/dag_mesh.h>
#include <libTools/dagFileRW/dagFileFormat.h>
#include <libTools/dagFileRW/splineShape.h>
#include <libTools/dagFileRW/textureNameResolver.h>
#include <ioSys/dag_fileIo.h>
#include <util/dag_simpleString.h>
#include <debug/dag_debug.h>
#include <stdio.h>

static const char *current_dag_fname = NULL;
const char *ITextureNameResolver::getCurrentDagName() { return current_dag_fname; }

class ImpBlkReader : public FullFileLoadCB
{
  ImpSceneCB &cb;
  SimpleString buf;
  Tab<int> blkTag;

public:
  SimpleString fname;
  char *str;
  char *str1;

public:
  ImpBlkReader(const char *fn, ImpSceneCB &_cb) : FullFileLoadCB(fn), cb(_cb), str(NULL), str1(NULL), blkTag(tmpmem)
  {
    if (fileHandle)
    {
      fname = fn;
      buf.allocBuffer(256 * 3);
      str = buf;
      str1 = str + 256;
    }
  }

  inline bool readSafe(void *buffer, int len) { return tryRead(buffer, len) == len; }
  inline bool seekrelSafe(int ofs)
  {
    DAGOR_TRY { seekrel(ofs); }
    DAGOR_CATCH(IGenLoad::LoadException e) { return false; }

    return true;
  }
  inline bool seektoSafe(int ofs)
  {
    DAGOR_TRY { seekto(ofs); }
    DAGOR_CATCH(IGenLoad::LoadException e) { return false; }

    return true;
  }

  int beginBlockSafe()
  {
    int tag = -1;
    DAGOR_TRY { tag = beginTaggedBlock(); }
    DAGOR_CATCH(IGenLoad::LoadException e) { return -1; }
    blkTag.push_back(tag);
    return getBlockLength() - 4;
  }
  bool endBlockSafe()
  {
    int tag = -1;
    DAGOR_TRY { endBlock(); }
    DAGOR_CATCH(IGenLoad::LoadException e) { return false; }
    blkTag.pop_back();
    return true;
  }
  int getBlockTag()
  {
    int i = blkTag.size() - 1;
    if (i < 0)
    {
      level_err();
      return -1;
    }
    return blkTag[i];
  }
  int getBlockLen()
  {
    int i = blkTag.size() - 1;
    if (i < 0)
    {
      level_err();
      return -1;
    }
    return getBlockLength() - 4;
  }

  void read_err() { cb.read_error(fname); }
  void level_err() { cb.format_error(fname); }
};

static int load_scene(ImpBlkReader &rdr, ImpSceneCB &);

int load_scene(const char *fn, ImpSceneCB &cb)
{
  ImpBlkReader rdr(fn, cb);
  if (!rdr.fileHandle)
    return 0;
  current_dag_fname = fn;
  int ret = load_scene(rdr, cb);
  current_dag_fname = NULL;
  return ret;
}


#define readerr     \
  do                \
  {                 \
    rdr.read_err(); \
    goto err;       \
  } while (0)
#define read(p, l)           \
  do                         \
  {                          \
    if (!rdr.readSafe(p, l)) \
      readerr;               \
  } while (0)
#define ifnread(p, l) if (!rdr.readSafe(p, l))
#define bblk                      \
  do                              \
  {                               \
    if (rdr.beginBlockSafe() < 0) \
      goto err;                   \
  } while (0)
#define eblk                 \
  do                         \
  {                          \
    if (!rdr.endBlockSafe()) \
      goto err;              \
  } while (0)

static int load_obj(ImpBlkReader &rdr, ImpSceneCB &cb)
{
  char havemesh = 0;
  Mesh *mMesh = NULL;
  while (rdr.getBlockRest() > 0)
  {
    bblk;
    if (rdr.getBlockTag() == DAG_OBJ_BIGMESH)
    {
      if (havemesh)
      {
        rdr.level_err();
        goto err;
      }
      havemesh = 1;
      mMesh = new Mesh;
      MeshData *m = &mMesh->getMeshData();
      int vn;
      read(&vn, 4);
      if (vn > 0)
      {
        m->vert.resize(vn);
        read(&m->vert[0], vn * 12);
      }
      int fn;
      read(&fn, 4);
      m->face.resize(fn);
      for (int i = 0; i < fn; ++i)
      {
        DagBigFace f;
        read(&f, sizeof(f));
        m->face[i].v[0] = f.v[0];
        m->face[i].v[1] = f.v[1];
        m->face[i].v[2] = f.v[2];
        m->face[i].smgr = f.smgr;
        m->face[i].mat = f.mat;
        m->face[i].forceAlignBy32BitDummy = 0;
      }
      uint8_t chn;
      read(&chn, 1);
      for (; chn; --chn)
      {
        int tn;
        read(&tn, 4);
        uint8_t co, chid;
        read(&co, 1);
        read(&chid, 1);
        int ch = int(chid) - 1;
        if (ch == -1)
        {
          m->cvert.resize(tn);
          for (int i = 0; i < tn; ++i)
          {
            if (co == 1)
            {
              read(&m->cvert[i], 4);
              m->cvert[i].g = m->cvert[i].b = m->cvert[i].a = 0;
            }
            else if (co == 2)
            {
              read(&m->cvert[i], 8);
              m->cvert[i].b = m->cvert[i].a = 0;
            }
            else
            {
              read(&m->cvert[i], 12);
              m->cvert[i].a = 0;
              if (co > 3)
                if (!rdr.seekrelSafe((co - 3) * 4))
                {
                  rdr.read_err();
                  goto err;
                }
            }
          }
          if (fn > 0)
          {
            m->cface.resize(fn);
            read(&m->cface[0], fn * sizeof(TFace));
          }
        }
        else if (ch < NUMMESHTCH)
        {
          m->tvert[ch].resize(tn);
          for (int i = 0; i < tn; ++i)
          {
            if (co == 1)
            {
              read(&m->tvert[ch][i], 4);
              m->tvert[ch][i].y = 0;
            }
            else
            {
              read(&m->tvert[ch][i], 8);
              m->tvert[ch][i].y = 1 - m->tvert[ch][i].y;
              if (co > 2)
                if (!rdr.seekrelSafe((co - 2) * 4))
                {
                  rdr.read_err();
                  goto err;
                }
            }
          }
          if (fn > 0)
          {
            m->tface[ch].resize(fn);
            read(&m->tface[ch][0], fn * sizeof(TFace));
          }
        }
        else
        {
          if (!rdr.seekrelSafe(co * 4 * tn))
          {
            rdr.read_err();
            goto err;
          }
          if (!rdr.seekrelSafe(fn * sizeof(TFace)))
          {
            rdr.read_err();
            goto err;
          }
        }
      }
    }
    else if (rdr.getBlockTag() == DAG_OBJ_MESH)
    {
      if (havemesh)
      {
        rdr.level_err();
        goto err;
      }
      havemesh = 1;
      mMesh = new Mesh;
      MeshData *m = &mMesh->getMeshData();
      int vn = 0;
      read(&vn, 2);
      if (vn > 0)
      {
        m->vert.resize(vn);
        read(&m->vert[0], vn * 12);
      }
      int fn = 0;
      read(&fn, 2);
      m->face.resize(fn);
      int i;
      for (i = 0; i < fn; ++i)
      {
        DagFace f;
        read(&f, sizeof(f));
        m->face[i].v[0] = f.v[0];
        m->face[i].v[1] = f.v[1];
        m->face[i].v[2] = f.v[2];
        m->face[i].smgr = f.smgr;
        m->face[i].mat = f.mat;
        m->face[i].forceAlignBy32BitDummy = 0;
      }
      uint8_t chn;
      read(&chn, 1);
      for (; chn; --chn)
      {
        int tn = 0;
        read(&tn, 2);
        uint8_t co, chid;
        read(&co, 1);
        read(&chid, 1);
        int ch = int(chid) - 1;
        if (ch == -1)
        {
          m->cvert.resize(tn);
          for (i = 0; i < tn; ++i)
          {
            if (co == 1)
            {
              read(&m->cvert[i], 4);
              m->cvert[i].g = m->cvert[i].b = m->cvert[i].a = 0;
            }
            else if (co == 2)
            {
              read(&m->cvert[i], 8);
              m->cvert[i].b = m->cvert[i].a = 0;
            }
            else
            {
              read(&m->cvert[i], 12);
              m->cvert[i].a = 0;
              if (co > 3)
                if (!rdr.seekrelSafe((co - 3) * 4))
                {
                  rdr.read_err();
                  goto err;
                }
            }
          }
          m->cface.resize(fn);
          for (i = 0; i < fn; ++i)
          {
            DagTFace f;
            read(&f, sizeof(f));
            m->cface[i].t[0] = f.t[0];
            m->cface[i].t[1] = f.t[1];
            m->cface[i].t[2] = f.t[2];
          }
        }
        else if (ch < NUMMESHTCH)
        {
          m->tvert[ch].resize(tn);
          for (i = 0; i < tn; ++i)
          {
            if (co == 1)
            {
              read(&m->tvert[ch][i], 4);
              m->tvert[ch][i].y = 0;
            }
            else
            {
              read(&m->tvert[ch][i], 8);
              m->tvert[ch][i].y = 1 - m->tvert[ch][i].y;
              if (co > 2)
                if (!rdr.seekrelSafe((co - 2) * 4))
                {
                  rdr.read_err();
                  goto err;
                }
            }
          }
          m->tface[ch].resize(fn);
          for (i = 0; i < fn; ++i)
          {
            DagTFace f;
            read(&f, sizeof(f));
            m->tface[ch][i].t[0] = f.t[0];
            m->tface[ch][i].t[1] = f.t[1];
            m->tface[ch][i].t[2] = f.t[2];
          }
        }
        else
        {
          if (!rdr.seekrelSafe(co * 4 * tn))
          {
            rdr.read_err();
            goto err;
          }
          if (!rdr.seekrelSafe(fn * sizeof(DagTFace)))
          {
            rdr.read_err();
            goto err;
          }
        }
      }
    }
    else if (rdr.getBlockTag() == DAG_OBJ_LIGHT)
    {
      D3dLight lt;
      DagLight d;
      DagLight2 d2;
      read(&d, sizeof(d));
      if (rdr.getBlockRest() == sizeof(d2))
      {
        read(&d2, sizeof(d2));
      }
      else
      {
        d2.type = DAG_LIGHT_OMNI;
        d2.hotspot = d2.falloff = 0;
        d2.mult = 1;
      }
      switch (d2.type)
      {
        case DAG_LIGHT_SPOT: lt.type = D3dLight::TYPE_SPOT; break;
        case DAG_LIGHT_DIR: lt.type = D3dLight::TYPE_DIR; break;
        default: lt.type = D3dLight::TYPE_POINT; break;
      }
      lt.inang = DegToRad(d2.hotspot);
      lt.outang = DegToRad(d2.falloff);
      lt.falloff = 1;
      lt.diff = lt.spec = D3dColor(d.r, d.g, d.b);
      lt.amb = D3dColor(0, 0, 0);
      lt.range = d.range;
      lt.atten0 = lt.atten1 = lt.atten2 = 0;
      if (d.decay == 1)
        lt.atten1 = 1 / d.drad;
      else if (d.decay == 2)
        lt.atten2 = 1 / (d.drad * d.drad);
      else
        lt.atten0 = 1;
      if (!cb.load_node_light(lt))
        goto err;
    }
    else if (rdr.getBlockTag() == DAG_OBJ_SPLINES)
    {
      SplineShape *shp = new SplineShape;
      int ns = 0;
      read(&ns, 4);
      shp->spl.resize(ns);
      int si;
      for (si = 0; si < ns; ++si)
        shp->spl[si] = NULL;
      for (si = 0; si < ns; ++si)
      {
        DagSpline *s = new DagSpline;
        shp->spl[si] = s;
        char flg;
        read(&flg, 1);
        s->closed = (flg & DAG_SPLINE_CLOSED) ? 1 : 0;
        int nk = 0;
        read(&nk, 4);
        s->knot.resize(nk);
        for (int ki = 0; ki < nk; ++ki)
        {
          char kt;
          read(&kt, 1);
          read(&s->knot[ki].i, 12);
          read(&s->knot[ki].p, 12);
          read(&s->knot[ki].o, 12);
        }
      }
      if (!cb.load_node_splines(shp))
        goto err;
    }
    else if (rdr.getBlockTag() == DAG_OBJ_BONES)
    {
      if (mMesh)
      {
        if (!cb.load_node_mesh(mMesh))
        {
          mMesh = NULL;
          goto err;
        }
        mMesh = NULL;
      }

      int bn = 0;
      read(&bn, 2);

      if (cb.load_node_bones(bn) && bn)
      {
        for (int i = 0; i < bn; ++i)
        {
          DagBone b;
          read(&b, sizeof(b));
          if (!cb.load_node_bone(i, b.id, *(TMatrix *)&b.tm))
            goto err;
        }

        int vn = 0;
        read(&vn, 4);
        if (cb.load_node_bonesinf(vn))
          if (vn)
          {
            real *inf = (real *)memalloc(bn * vn * sizeof(real), tmpmem);
            ifnread(inf, bn * vn * sizeof(real))
            {
              memfree(inf, tmpmem);
              readerr;
            }
            if (!cb.load_node_bonesinf(inf))
              goto err;
          }
      }
    }
    else if (rdr.getBlockTag() == DAG_OBJ_MORPH)
    {
      if (mMesh)
      {
        if (!cb.load_node_mesh(mMesh))
        {
          mMesh = NULL;
          goto err;
        }
        mMesh = NULL;
      }

      int tn = 0;
      read(&tn, 1);

      if (cb.load_node_morph(tn))
      {
        for (int i = 0; i < tn; ++i)
        {
          unsigned char len = 0;
          read(&len, 1);

          char name[256];
          read(name, len);
          name[len] = 0;

          unsigned short id = 0xFFFF;
          read(&id, sizeof(id));

          if (!cb.load_node_morph_target(i, id, name))
            goto err;
        }
      }
    }
    else if (rdr.getBlockTag() == DAG_OBJ_FACEFLG && mMesh)
    {
      int n = rdr.getBlockRest() / sizeof(uint8_t);
      mMesh->getMeshData().faceflg.resize(n);
      read(mMesh->getMeshData().faceflg.data(), data_size(mMesh->getMeshData().faceflg));
    }
    else if (rdr.getBlockTag() == DAG_OBJ_NORMALS && mMesh)
    {
      int numNormals;
      read(&numNormals, 4);

      if (numNormals > 0)
      {
        mMesh->getMeshData().vertnorm.resize(numNormals);
        read(mMesh->getMeshData().vertnorm.data(), data_size(mMesh->getMeshData().vertnorm));
        mMesh->getMeshData().facengr.resize(mMesh->getFace().size());
        read(mMesh->getMeshData().facengr.data(), data_size(mMesh->getMeshData().facengr));
      }
    }
    eblk;
  }

  if (mMesh)
  {
    if (!cb.load_node_mesh(mMesh))
    {
      mMesh = NULL;
      goto err;
    }
    mMesh = NULL;
  }

  return 1;
err:
  if (mMesh)
    delete mMesh;
  return 0;
}

static int load_node(ImpBlkReader &rdr, ImpSceneCB &cb)
{
  if (!cb.load_node())
    return 1;
  while (rdr.getBlockRest() > 0)
  {
    bblk;
    switch (rdr.getBlockTag())
    {
      case DAG_NODE_CHILDREN:
        if (cb.load_node_children())
          while (rdr.getBlockRest() > 0)
          {
            bblk;
            if (rdr.getBlockTag() == DAG_NODE)
            {
              if (!load_node(rdr, cb))
                return 0;
            }
            eblk;
          }
        break;
      case DAG_NODE_DATA:
        if (cb.load_node_data())
        {
          DagNodeData dat;
          read(&dat, sizeof(dat));
          int l = rdr.getBlockRest();
          if (l > 255)
            l = 255;
          read(rdr.str, l);
          rdr.str[l] = 0;
          if (!cb.load_node_data(rdr.str, dat.id, dat.cnum, dat.flg))
            return 0;
        }
        break;
      case DAG_NODE_MATER:
      {
        int num = rdr.getBlockLen() / sizeof(uint16_t);
        if (cb.load_node_mat(num))
        {
          for (int i = 0; i < num; ++i)
          {
            uint16_t id;
            read(&id, sizeof(id));
            if (!cb.load_node_submat(i, id))
              return 0;
          }
        }
      }
      break;
      case DAG_NODE_TM:
        if (cb.load_node_tm())
        {
          TMatrix tm;
          read(&tm, sizeof(tm));

          if (!cb.load_node_tm(tm))
            return 0;
        }
        break;
      case DAG_NODE_NOTETRACK: break;
      case DAG_NODE_ANIM: break;
      case DAG_NODE_SCRIPT:
        if (cb.load_node_script())
        {
          int l = rdr.getBlockLen();
          char *s = (char *)memalloc(l + 1, strmem);
          if (rdr.tryRead(s, l) != l)
          {
            rdr.read_err();
            memfree(s, strmem);
            return 0;
          }
          s[l] = 0;
          if (!cb.load_node_script(s))
            return 0;
        }
        break;
      case DAG_NODE_OBJ:
        if (cb.load_node_obj())
        {
          if (!load_obj(rdr, cb))
            return 0;
        }
        break;
    }
    eblk;
  }
  return 1;

err:
  return 0;
}

static bool textures_path_expand_disabled = false;
void disable_relative_textures_path_expansion() { textures_path_expand_disabled = true; }
void enable_relative_textures_path_expansion() { textures_path_expand_disabled = false; }


static void make_texture_name(const char *fname, const char *src_name, char *name, unsigned name_len)
{
  // src_name & name expected to be NOT OVERLAPPED

  //== quick and dirty solution for controlling path expansion
  if (textures_path_expand_disabled)
  {
    strcpy(name, src_name);
    return;
  }

  G_ASSERT(fname);
  G_ASSERT(name);
  if (src_name[0] == '.' && (src_name[1] == '\\' || src_name[1] == '/'))
  {
    const char *end = fname + strlen(fname) - 1;

    while (end > fname && !(*end == '\\' || *end == '/'))
      --end;

    char location[1024];
    if (end > fname)
      strncpy(location, fname, end - fname);
    location[end - fname] = '\0';
    _snprintf(name, name_len, "./%s/%s", location, src_name + 2);
  }
  else
  {
    strcpy(name, src_name);
  }
}


int load_scene(ImpBlkReader &rdr, ImpSceneCB &cb)
{
  if (!rdr.seektoSafe(4))
  {
    rdr.read_err();
    return 0;
  }
  bblk;
  for (; rdr.getBlockRest() > 0;)
  {
    bblk;
    if (rdr.getBlockTag() == DAG_END)
    {
      eblk;
      break;
    }
    else if (rdr.getBlockTag() == DAG_TEXTURES)
    {
      uint16_t num;
      read(&num, 2);
      if (int ld_type = cb.load_textures(num))
      {
        for (int i = 0; i < num; ++i)
        {
          int l = 0;
          read(&l, 1);
          read(rdr.str, l);
          rdr.str[l] = 0;

          char name[1024];
          make_texture_name(rdr.fname, rdr.str, name, countof(name));
          if (!cb.load_texture(i, name))
            return 0;
        }
        if (ld_type < 0)
        {
          eblk;
          break;
        }
      }
    }
    else if (rdr.getBlockTag() == DAG_MATER)
    {
      int l = 0;
      read(&l, 1);
      if (l)
        read(rdr.str, l);
      rdr.str[l] = 0;
      if (cb.load_material(rdr.str))
      {
        ImpMat m;
        m.name = rdr.str;
        m.classname = rdr.str1;
        m.script = NULL;
        read(&m, sizeof(DagMater));
        l = 0;
        read(&l, 1);
        if (l)
          read(m.classname, l);
        m.classname[l] = 0;
        l = rdr.getBlockRest();
        if (l > 0)
        {
          m.script = (char *)memalloc(l + 1, strmem);
          if (rdr.tryRead(m.script, l) != l)
          {
            rdr.read_err();
            memfree(m.script, strmem);
            return 0;
          }
          m.script[l] = 0;
        }
        if (!cb.load_material(m))
          return 0;
      }
    }
    else if (rdr.getBlockTag() == DAG_KEYLABELS)
      ; // no-op
    else if (rdr.getBlockTag() == DAG_NOTETRACK)
      ; // no-op
    else if (rdr.getBlockTag() == DAG_NODE)
    {
      if (!load_node(rdr, cb))
        return 0;
    }
    eblk;
  }
  eblk;
  return 1;

err:
  return 0;
}

#undef readerr
#undef read
#undef ifnread
#undef memchk
#undef bblk
#undef eblk
