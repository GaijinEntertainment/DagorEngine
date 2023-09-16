#include <scene/dag_sh3LtMgr.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_patchTab.h>
#include <math/dag_SHlight.h>
#include <math/dag_Point3.h>
#include <ioSys/dag_genIo.h>
#include <debug/dag_debug.h>
#include <generic/dag_tab.h>


class SH3LightingData
{
public:
  struct TmpLtRec
  {
    real pwr;
    real coneCos;
    Color3 col;
    Point3 dir;
  };

  static SH3LightingData *loadBinary(IGenLoad &crd);
  bool getLight(const Point3 &pos, SH3Lighting &lt, SH3LightingManager::IRayTracer &rt);

  bool getLight(const Point3 &pos, SHDirLighting &lt, bool sun_as_dir1, int *lt1_id, int *lt2_id, real *lt1_conecos, real *lt2_conecos,
    SH3LightingManager::IRayTracer &rt);

  void setSun(const Point3 &dir, const Color3 &col, float ang_sz)
  {
    checkV3();

    sunDir = normalize(dir);
    sunCol = col;
    sunConeCos = cos(ang_sz / 2);
    sunColPwr = brightness(col) * (1 - sunConeCos);
  }
  void changeAmbientScale(const Color3 &col)
  {
    checkV3();
    ambColScale = col;
  }

  void tonemapLight(Color3 sh[4], SHDirLighting &lt) const;

protected:
  struct DirLight
  {
    Color3 lt;
    float maxDist;
    Point3 dir;
    float coneCosAlpha;
  };
  struct OmniLight
  {
    Point3 pt;
    float maxDist2;
    int cubeIdx, cubeCnt;
    float r2, r;
  };
  struct OmniLightCube
  {
    Point3 norm;
    int _resv;
    Color3 lt;
    float maxDist2;
  };

  SH3Lighting skyLt;
  int _resv;
  Color3 invGamma, postScale;
  PatchableTab<DirLight> dirLt;
  PatchableTab<OmniLight> omniLt;
  PatchableTab<OmniLightCube> omniLtCube;

  PatchableTab<TmpLtRec> stk;

  Point3 sunDir;
  Color3 sunCol;
  Color3 ambColScale;
  real sunColPwr, sunConeCos;

  SH3LightingData() {}

  void postLoadTransform()
  {
    // for ( int i = 0; i < SPHHARM_NUM3; i ++ )
    //   skyLt.sh[i] *= 0.5;
    for (int i = 0; i < dirLt.size(); i++)
      dirLt[i].lt /= 2 * PI;
    for (int i = 0; i < omniLtCube.size(); i++)
      omniLtCube[i].lt /= 4 * PI;
    for (int i = 0; i < omniLt.size(); i++)
      omniLt[i].r = sqrt(omniLt[i].r2);
  }

  void transformSH(const Color3 sh[SPHHARM_NUM3], Color3 outsh[SPHHARM_NUM3], int steps) const;
  void transformSH2(const Color3 sh[4], Color3 outsh[4], int steps) const;

  inline void tonemapColor(Color3 &c) const
  {
    c /= postScale;
    c.clamp0();
    c.r = powf(c.r, invGamma.r);
    c.g = powf(c.g, invGamma.g);
    c.b = powf(c.b, invGamma.b);
    c *= postScale * 0.5;
  }

  inline void checkV3()
  {
    G_ASSERT((!dirLt.data() || (ptrdiff_t)dirLt.data() > (ptrdiff_t)&sunConeCos) && "err: format <v3");
    G_ASSERT((!omniLt.data() || (ptrdiff_t)omniLt.data() > (ptrdiff_t)&sunConeCos) && "err: format <v3");
  }
};

struct SH3LightingManager::LightingContext
{
  struct DirLtSample
  {
    Color3 shAmb[4];
    Point3 dir[2];
    Color3 col[2];
    int ltIdx[2];
  };

  Color3 shAmb[4];

  Tab<SH3LightingData::TmpLtRec> lt;
  Tab<unsigned short> ltRemap;
  SmallTab<DirLtSample, MidmemAlloc> ringLt;
  SH3LightingData *ltData;
  int done;

  LightingContext() : ltRemap(midmem), lt(midmem) {}
  void reset(int ring_sz)
  {
    clear_and_shrink(ltRemap);
    clear_and_shrink(lt);
    done = 0;

    clear_and_resize(ringLt, ring_sz);
    mem_set_0(ringLt);
    for (int i = 0; i < ringLt.size(); i++)
    {
      ringLt[i].ltIdx[0] = -1;
      ringLt[i].ltIdx[1] = -1;
    }
  }

  void updateLightRing(const SHDirLighting &l, int lt1_idx, int lt2_idx)
  {
    DirLtSample &s = ringLt[done % ringLt.size()];

    for (int i = 0; i < 4; i++)
      shAmb[i] -= s.shAmb[i];

    if (s.ltIdx[0] != -1)
    {
      SH3LightingData::TmpLtRec &dl = lt[ltRemap[s.ltIdx[0]] & 0xFF];
      dl.col -= s.col[0];
      dl.dir -= s.dir[0];
      if (s.ltIdx[1] != -1)
      {
        SH3LightingData::TmpLtRec &dl2 = lt[ltRemap[s.ltIdx[1]] & 0xFF];
        dl2.col -= s.col[1];
        dl2.dir -= s.dir[1];
      }
    }

    s.ltIdx[0] = lt1_idx;
    s.ltIdx[1] = lt2_idx;
    memcpy(s.shAmb, l.shAmb, sizeof(shAmb));
    if (lt1_idx != -1)
    {
      s.col[0] = l.col1;
      s.dir[0] = l.dir1;
      if (lt2_idx != -1)
      {
        s.col[1] = l.col2;
        s.dir[1] = l.dir2;
      }
    }
  }

  void addLight(const SHDirLighting &l, int lt1_idx, int lt2_idx, real lt1_conecos, real lt2_conecos, SH3LightingData *_lt_data)
  {
    ltData = _lt_data;
    if (done == 0)
      memcpy(shAmb, l.shAmb, sizeof(shAmb));
    else
      for (int i = 0; i < 4; i++)
        shAmb[i] += l.shAmb[i];

    if (ringLt.size() && done >= ringLt.size())
      updateLightRing(l, lt1_idx, lt2_idx);

    if (lt1_idx != -1)
    {
      if (lt1_idx >= ltRemap.size() || lt2_idx >= ltRemap.size())
      {
        int sz = ltRemap.size();
        ltRemap.resize((lt1_idx > lt2_idx ? lt1_idx : lt2_idx) + 1);
        for (int i = ltRemap.size() - 1; i >= sz; i--)
          ltRemap[i] = 0xFF;
      }

      if (ltRemap[lt1_idx] == 0xFF)
      {
        SH3LightingData::TmpLtRec &dl = lt.push_back();
        dl.col = l.col1;
        dl.dir = l.dir1;
        dl.coneCos = lt1_conecos;
        ltRemap[lt1_idx] = 0x100 | (lt.size() - 1);
      }
      else
      {
        SH3LightingData::TmpLtRec &dl = lt[ltRemap[lt1_idx] & 0xFF];
        dl.col += l.col1;
        dl.dir += l.dir1;
        dl.coneCos = lt1_conecos;
        ltRemap[lt1_idx] += 0x100;
      }

      if (lt2_idx != -1)
      {
        if (ltRemap[lt2_idx] == 0xFF)
        {
          SH3LightingData::TmpLtRec &dl = lt.push_back();
          dl.col = l.col2;
          dl.dir = l.dir2;
          dl.coneCos = lt2_conecos;
          ltRemap[lt2_idx] = 0x100 | (lt.size() - 1);
        }
        else
        {
          SH3LightingData::TmpLtRec &dl = lt[ltRemap[lt2_idx] & 0xFF];
          dl.col += l.col2;
          dl.dir += l.dir2;
          dl.coneCos = lt2_conecos;
          ltRemap[lt2_idx] += 0x100;
        }
      }
    }
    done++;
  }

  void getFinalLight(SHDirLighting &l) const
  {
    G_ASSERT(done > 0);
    SH3Lighting sh;

    real invTotal = (ringLt.size() && done >= ringLt.size()) ? 1.0 / ringLt.size() : 1.0 / done;
    for (int i = 0; i < 4; i++)
      sh.sh[i] = shAmb[i] * invTotal;

    int f1 = -1, f2 = -1;
    real pf1 = 0, pf2 = 0;

    for (int i = 0; i < lt.size(); i++)
    {
      real pwr = brightness(lt[i].col) * invTotal;

      if (f1 == -1)
      {
        f1 = i;
        pf1 = pwr;
      }
      else if (pwr > pf1)
      {
        f2 = f1;
        pf2 = pf1;
        f1 = i;
        pf1 = pwr;
      }
      else if (f2 == -1 || pwr > pf2)
      {
        f2 = i;
        pf2 = pwr;
      }
    }

    if (f1 == -1)
    {
      memset(&l, 0, sizeof(l));
      goto ret_light;
    }

    l.dir1 = normalize(lt[f1].dir);
    l.col1 = lt[f1].col * (1 - lt[f1].coneCos) * invTotal;

    if (f2 != -1)
    {
      l.dir2 = normalize(lt[f2].dir);
      l.col2 = lt[f2].col * (1 - lt[f2].coneCos) * invTotal;
    }
    else
    {
      l.dir2 = l.dir1;
      l.col2 = Color3(0, 0, 0);
    }

    if (f2 == -1)
    {
      for (int i = 0; i < f1; i++)
        ::add_hemisphere_sphharm(sh.sh, lt[i].dir, lt[i].coneCos, lt[i].col * invTotal);
      for (int i = f1 + 1; i < lt.size(); i++)
        ::add_hemisphere_sphharm(sh.sh, lt[i].dir, lt[i].coneCos, lt[i].col * invTotal);
    }
    else
    {
      for (int i = 0; i < lt.size(); i++)
        if (i != f1 && i != f2)
          ::add_hemisphere_sphharm(sh.sh, lt[i].dir, lt[i].coneCos, lt[i].col * invTotal);
    }

  ret_light:
    ltData->tonemapLight(sh.sh, l);
  }
};

//
// Lighting manager
//
SH3LightingManager::Element::Element() : ltData(NULL), id(unsigned(-1)) {}


SH3LightingManager::Element::~Element() { clear(); }


void SH3LightingManager::Element::clear()
{
  del_it(ltData);
  id = unsigned(-1);
}


SH3LightingManager::SH3LightingManager(int max_dump_reserve) : elems(midmem_ptr()), tracer(NULL) { elems.reserve(max_dump_reserve); }


SH3LightingManager::~SH3LightingManager() { delAllLtData(); }


bool SH3LightingManager::getLighting(const Point3 &pos, SH3Lighting &lt)
{
  G_ASSERT(tracer);

  for (int i = 0; i < elems.size(); ++i)
  {
    if (!elems[i].ltData)
      continue;
    if (elems[i].ltData->getLight(pos, lt, *tracer))
      return true;
  }

  lt.clear();
  return false;
}

void SH3LightingManager::getLighting(const Point3 &pos, SHDirLighting &lt, bool sun_as_dir1, int *lt1_id, int *lt2_id)
{
  G_ASSERT(tracer);

  for (int i = 0; i < elems.size(); ++i)
    if (elems[i].ltData && elems[i].ltData->getLight(pos, lt, sun_as_dir1, lt1_id, lt2_id, NULL, NULL, *tracer))
      return;
  memset(&lt, 0, sizeof(lt));
}

SH3LightingManager::LightingContext *SH3LightingManager::createContext() { return new LightingContext; }
void SH3LightingManager::destroyContext(SH3LightingManager::LightingContext *ctx) { delete ctx; }

void SH3LightingManager::startGetLighting(SH3LightingManager::LightingContext *ctx, int ring_sz) { ctx->reset(ring_sz); }
void SH3LightingManager::addLightingToCtx(const Point3 &pos, SH3LightingManager::LightingContext *ctx, bool sun_as_dir1)
{
  G_ASSERT(tracer);

  for (int i = 0; i < elems.size(); ++i)
    if (elems[i].ltData)
    {
      SHDirLighting lt;
      int lt1_id, lt2_id;
      real lt1_conecos, lt2_conecos;
      if (elems[i].ltData->getLight(pos, lt, sun_as_dir1, &lt1_id, &lt2_id, &lt1_conecos, &lt2_conecos, *tracer))
      {
        ctx->addLight(lt, lt1_id, lt2_id, lt1_conecos, lt2_conecos, elems[i].ltData);
        return;
      }
    }
}
void SH3LightingManager::getLighting(const SH3LightingManager::LightingContext *ctx, SHDirLighting &lt) { ctx->getFinalLight(lt); }

void SH3LightingManager::changeSunLight(const Point3 &dir, const Color3 &col, float ang_sz)
{
  for (int i = 0; i < elems.size(); ++i)
  {
    if (!elems[i].ltData)
      continue;

    elems[i].ltData->setSun(dir, col, ang_sz);
  }
}

void SH3LightingManager::changeAmbientScale(const Color3 &col)
{
  for (int i = 0; i < elems.size(); ++i)
    if (elems[i].ltData)
      elems[i].ltData->changeAmbientScale(col);
}

int SH3LightingManager::addLtData(SH3LightingData *lt_grid, unsigned id)
{
  int i;
  for (i = 0; i < elems.size(); ++i)
    if (!elems[i].ltData)
      break;

  if (i >= elems.size())
  {
    i = append_items(elems, 1);
  }

  elems[i].ltData = lt_grid;
  elems[i].id = id;

  return i;
}


int SH3LightingManager::loadLtDataBinary(IGenLoad &crd, unsigned id)
{
  SH3LightingData *ltData = SH3LightingData::loadBinary(crd);

  if (!ltData)
  {
    delete ltData;
    return -1;
  }

  return addLtData(ltData, id);
}


void SH3LightingManager::delLtData(unsigned id)
{
  int i;
  for (i = 0; i < elems.size(); ++i)
  {
    if (!elems[i].ltData)
      continue;
    if (elems[i].id != id)
      continue;

    elems[i].clear();
    break;
  }

  if (i >= elems.size())
    return;

  // erase empty tail elements
  int j;
  for (j = i + 1; j < elems.size(); ++j)
    if (elems[j].ltData)
      break;

  if (j >= elems.size())
    erase_items(elems, i, elems.size() - i);
}


void SH3LightingManager::delAllLtData() { elems.clear(); }

//
// Lighting data
//
SH3LightingData *SH3LightingData::loadBinary(IGenLoad &crd)
{
  int ver = crd.readInt();
  if (ver != _MAKE4C('v4'))
    return NULL;
  int sz = crd.readInt();
  SH3LightingData *data = new (memalloc(sz, midmem), _NEW_INPLACE) SH3LightingData;
  crd.read(data, sz);

  data->dirLt.patch(data);
  data->omniLt.patch(data);
  data->omniLtCube.patch(data);
  data->stk.patch(data);

  data->postLoadTransform();

  debug("invGamma =" FMT_P3 "", data->invGamma.r, data->invGamma.g, data->invGamma.b);
  debug("postScale=" FMT_P3 "", data->postScale.r, data->postScale.g, data->postScale.b);
  return data;
}


void SH3LightingData::transformSH(const Color3 sh[SPHHARM_NUM3], Color3 outsh[SPHHARM_NUM3], int steps) const
{
  real delta = PI / steps;
  real delta2 = delta * delta;
  real halfDelta = delta * 0.5f;

  int i, j;

  for (i = 0; i < SPHHARM_NUM3; ++i)
    outsh[i] = Color3(0, 0, 0);

  for (i = 0; i < steps; ++i)
  {
    real theta = i * delta + halfDelta;

    real sinTheta = sin(theta);
    real z = cos(theta);
    real area = delta2 * sinTheta;

    for (j = 0; j < steps * 2; ++j)
    {
      real phi = j * delta + halfDelta;

      real x = sinTheta * cos(phi);
      real y = sinTheta * sin(phi);

      Color3 func = sh[SPHHARM_00] * SPHHARM_COEF_0 +
                    (sh[SPHHARM_1m1] * y + sh[SPHHARM_10] * z + sh[SPHHARM_1p1] * x) * SPHHARM_COEF_1 +
                    (sh[SPHHARM_2m2] * x * y + sh[SPHHARM_2m1] * y * z + sh[SPHHARM_2p1] * x * z) * SPHHARM_COEF_21 +
                    sh[SPHHARM_20] * (3 * z * z - 1) * SPHHARM_COEF_20 + sh[SPHHARM_2p2] * (x * x - y * y) * SPHHARM_COEF_22;

      tonemapColor(func);
      func *= area;

      outsh[SPHHARM_00] += func * SPHHARM_COEF_0;
      outsh[SPHHARM_1m1] += func * y * SPHHARM_COEF_1;
      outsh[SPHHARM_10] += func * z * SPHHARM_COEF_1;
      outsh[SPHHARM_1p1] += func * x * SPHHARM_COEF_1;
      outsh[SPHHARM_2m2] += func * x * y * SPHHARM_COEF_21;
      outsh[SPHHARM_2m1] += func * y * z * SPHHARM_COEF_21;
      outsh[SPHHARM_2p1] += func * x * z * SPHHARM_COEF_21;
      outsh[SPHHARM_20] += func * (3 * z * z - 1) * SPHHARM_COEF_20;
      outsh[SPHHARM_2p2] += func * (x * x - y * y) * SPHHARM_COEF_22;
    }
  }
}
void SH3LightingData::transformSH2(const Color3 sh[4], Color3 outsh[4], int steps) const
{
  real delta = PI / steps;
  real delta2 = delta * delta;
  real halfDelta = delta * 0.5f;

  int i, j;

  memset(outsh, 0, sizeof(outsh[0]) * 4);

  for (i = 0; i < steps; ++i)
  {
    real theta = i * delta + halfDelta;

    real sinTheta = sin(theta);
    real z = cos(theta);
    real area = delta2 * sinTheta;

    for (j = 0; j < steps * 2; ++j)
    {
      real phi = j * delta + halfDelta;

      real x = sinTheta * cos(phi);
      real y = sinTheta * sin(phi);

      Color3 func =
        sh[SPHHARM_00] * SPHHARM_COEF_0 + (sh[SPHHARM_1m1] * y + sh[SPHHARM_10] * z + sh[SPHHARM_1p1] * x) * SPHHARM_COEF_1;

      tonemapColor(func);
      func *= area;

      outsh[SPHHARM_00] += func * SPHHARM_COEF_0;
      outsh[SPHHARM_1m1] += func * y * SPHHARM_COEF_1;
      outsh[SPHHARM_10] += func * z * SPHHARM_COEF_1;
      outsh[SPHHARM_1p1] += func * x * SPHHARM_COEF_1;
    }
  }
}
void SH3LightingData::tonemapLight(Color3 sh[4], SHDirLighting &lt) const
{
  sh[SPHHARM_00] *= PI;
  sh[SPHHARM_1m1] *= 2.094395f;
  sh[SPHHARM_10] *= 2.094395f;
  sh[SPHHARM_1p1] *= 2.094395f;

  transformSH2(sh, lt.shAmb, 6);

  lt.shAmb[SPHHARM_00] /= PI;
  lt.shAmb[SPHHARM_1m1] /= 2.094395f;
  lt.shAmb[SPHHARM_10] /= 2.094395f;
  lt.shAmb[SPHHARM_1p1] /= 2.094395f;

  tonemapColor(lt.col1);
  tonemapColor(lt.col2);
  lt.label = realQNaN;
}

#ifndef M_SQRT1_2
#define M_SQRT1_2 0.707106781186547524401
#endif

static Point3 sky_free_rays[] = {Point3(0, 1, 0), Point3(M_SQRT1_2, M_SQRT1_2, 0), Point3(-M_SQRT1_2, M_SQRT1_2, 0),
  Point3(0, M_SQRT1_2, M_SQRT1_2), Point3(0, M_SQRT1_2, -M_SQRT1_2)};
static int sky_free_rays_count = sizeof(sky_free_rays) / sizeof(sky_free_rays[0]);
static float sky_free_dist = 20;

float SH3LightingManager::min_power_of_light_use = 0.005;

bool SH3LightingData::getLight(const Point3 &pos, SH3Lighting &lt, SH3LightingManager::IRayTracer &rt)
{
  int sky_free = 0;
  for (int i = 0; i < sky_free_rays_count; i++)
    if (!rt.rayHit(pos, sky_free_rays[i], sky_free_dist))
      sky_free++;
  lt = skyLt;

  if (sky_free < sky_free_rays_count)
    for (int i = 0; i < SPHHARM_NUM3; i++)
      lt.sh[i] *= float(sky_free) / float(sky_free_rays_count);

  for (int i = 0; i < dirLt.size(); i++)
    if (!rt.rayHit(pos, dirLt[i].dir, dirLt[i].maxDist))
      ::add_hemisphere_sphharm(lt.sh, dirLt[i].dir, dirLt[i].coneCosAlpha, dirLt[i].lt);

  for (int i = 0; i < omniLt.size(); i++)
  {
    Point3 dir = omniLt[i].pt - pos;
    float dist2 = lengthSq(dir), dist;

    if (dist2 > omniLt[i].maxDist2 || dist2 < 1e-6)
      continue;
    dist = sqrtf(dist2);
    dir *= 1.0 / dist;
    if (dist2 < omniLt[i].r2 || !rt.rayHit(pos, dir, dist - omniLt[i].r))
    {
      OmniLightCube *c = &omniLtCube[omniLt[i].cubeIdx];
      Color3 col(0, 0, 0);

      for (int j = omniLt[i].cubeCnt - 1; j >= 0; j--)
        if (c[j].maxDist2 > dist2)
        {
          float cos_dir = dir * c[j].norm;
          if (cos_dir < 0)
            col += c[j].lt * (-cos_dir);
        }
      ::add_sphere_light_sphharm(lt.sh, dir * dist, omniLt[i].r2, col);
    }
  }
  lt.sh[SPHHARM_00] *= PI;

  lt.sh[SPHHARM_1m1] *= 2.094395f;
  lt.sh[SPHHARM_10] *= 2.094395f;
  lt.sh[SPHHARM_1p1] *= 2.094395f;

  lt.sh[SPHHARM_2m2] *= 0.785398f;
  lt.sh[SPHHARM_2m1] *= 0.785398f;
  lt.sh[SPHHARM_20] *= 0.785398f;
  lt.sh[SPHHARM_2p1] *= 0.785398f;
  lt.sh[SPHHARM_2p2] *= 0.785398f;

  Color3 sh[SPHHARM_NUM3];
  transformSH(lt.sh, sh, 6);

  lt.sh[SPHHARM_00] = sh[SPHHARM_00] / PI;

  lt.sh[SPHHARM_1m1] = sh[SPHHARM_1m1] / 2.094395f;
  lt.sh[SPHHARM_10] = sh[SPHHARM_10] / 2.094395f;
  lt.sh[SPHHARM_1p1] = sh[SPHHARM_1p1] / 2.094395f;

  lt.sh[SPHHARM_2m2] = sh[SPHHARM_2m2] / 0.785398f;
  lt.sh[SPHHARM_2m1] = sh[SPHHARM_2m1] / 0.785398f;
  lt.sh[SPHHARM_20] = sh[SPHHARM_20] / 0.785398f;
  lt.sh[SPHHARM_2p1] = sh[SPHHARM_2p1] / 0.785398f;
  lt.sh[SPHHARM_2p2] = sh[SPHHARM_2p2] / 0.785398f;
  return true;
}

float SH3LightingManager::max_sun_dist = 100.0;
float SH3LightingManager::start_sun_fade_dist = 0.3;


bool SH3LightingData::getLight(const Point3 &pos, SHDirLighting &lt, bool sun_as_dir1, int *lt1_idx, int *lt2_idx, real *lt1_conecos,
  real *lt2_conecos, SH3LightingManager::IRayTracer &rt)
{
  checkV3();

  SH3Lighting sh;

  int sky_free = 0;
  for (int i = 0; i < sky_free_rays_count; i++)
    if (!rt.rayHit(pos, sky_free_rays[i], sky_free_dist))
      sky_free++;

  memset(&sh, 0, sizeof(sh));
  if (sky_free)
  {
    Color3 mul = ambColScale * float(sky_free) / float(sky_free_rays_count);
    for (int i = 0; i < 4; i++)
      sh.sh[i] = skyLt.sh[i] * mul;
  }

  TmpLtRec *cs = stk.data(), *cs_f1 = NULL, *cs_f2 = NULL;
  int f1 = -1, f2 = -1;
  float ray_dist = 0;
  float maxSunDist = SH3LightingManager::max_sun_dist;
  float sunFade = SH3LightingManager::start_sun_fade_dist;
  bool sun_free = sunColPwr > 0 && !rt.rayTrace(pos, sunDir, maxSunDist, ray_dist);

  if (sun_free || ray_dist > maxSunDist * sunFade)
  {
    if (ray_dist < maxSunDist)
      ray_dist = (ray_dist - maxSunDist * sunFade) / (maxSunDist * (1 - sunFade)) * 0.95;
    else
      ray_dist = 1.0;

    cs->col = sunCol * ray_dist;
    cs->coneCos = sunConeCos;
    cs->pwr = sun_as_dir1 ? 10e+12 : sunColPwr * ray_dist;
    cs->dir = sunDir;
    cs_f1 = cs;
    f1 = 0;

    cs++;
  }
  else if (sun_as_dir1)
  {
    cs->col = Color3(0, 0, 0);
    cs->coneCos = 1.0;
    cs->pwr = 10e+12;
    cs->dir = sunDir;
    cs_f1 = cs;
    f1 = 0;

    cs++;
  }
  for (int i = 1; i < dirLt.size(); i++)
    if (!rt.rayHit(pos, dirLt[i].dir, dirLt[i].maxDist))
    {
      cs->pwr = brightness(dirLt[i].lt) * (1 - dirLt[i].coneCosAlpha);
      cs->coneCos = dirLt[i].coneCosAlpha;
      cs->col = dirLt[i].lt;
      cs->dir = dirLt[i].dir;

      if (!cs_f1)
      {
        cs_f1 = cs;
        f1 = i;
      }
      else if (cs->pwr > cs_f1->pwr)
      {
        cs_f2 = cs_f1;
        cs_f1 = cs;
        f2 = f1;
        f1 = i;
      }
      else if (!cs_f2 || cs->pwr > cs_f2->pwr)
      {
        cs_f2 = cs;
        f2 = i;
      }

      cs++;
    }

  for (int i = 0; i < omniLt.size(); i++)
  {

    Point3 dir = omniLt[i].pt - pos;
    float dist2 = lengthSq(dir), dist;

    if (dist2 > omniLt[i].maxDist2 || dist2 < 1e-6)
      continue;
    // if ( )
    {
      dist = sqrtf(dist2);

      dir *= 1.0 / dist;
      G_ASSERT(omniLt[i].cubeIdx + omniLt[i].cubeCnt <= omniLtCube.size());
      OmniLightCube *c = &omniLtCube[omniLt[i].cubeIdx];
      Color3 col(0, 0, 0);

      for (int j = omniLt[i].cubeCnt - 1; j >= 0; j--)
        if (c[j].maxDist2 > dist2)
        {
          float cos_dir = dir * c[j].norm;
          if (cos_dir < 0)
            col += c[j].lt * (-cos_dir);
        }

      cs->coneCos = dist / sqrtf(dist2 + omniLt[i].r2);
      cs->pwr = brightness(col) * (1 - cs->coneCos);
      cs->col = col;
      cs->dir = dir;
      if (cs->pwr < SH3LightingManager::min_power_of_light_use)
        continue;
      if (dist2 > omniLt[i].r2 && rt.rayHit(pos, dir, dist - omniLt[i].r))
        continue;


      if (!cs_f1)
      {
        cs_f1 = cs;
        f1 = i + dirLt.size();
      }
      else if (cs->pwr > cs_f1->pwr)
      {
        cs_f2 = cs_f1;
        cs_f1 = cs;
        f2 = f1;
        f1 = i + dirLt.size();
      }
      else if (!cs_f2 || cs->pwr > cs_f2->pwr)
      {
        cs_f2 = cs;
        f2 = i + dirLt.size();
      }

      cs++;
    }
  }

  if (lt1_idx)
    *lt1_idx = f1;
  if (lt2_idx)
    *lt2_idx = f2;

  if (!cs_f1)
  {
    memset(&lt, 0, sizeof(lt));
    if (lt1_conecos)
      *lt1_conecos = 0;
    if (lt2_conecos)
      *lt2_conecos = 0;
    goto ret_light;
  }

  lt.dir1 = cs_f1->dir;
  lt.col1 = cs_f1->col;
  if (lt1_conecos)
    *lt1_conecos = cs_f1->coneCos;
  else
    lt.col1 *= (1 - cs_f1->coneCos);

  if (cs_f2)
  {
    lt.dir2 = cs_f2->dir;
    lt.col2 = cs_f2->col;
    if (lt2_conecos)
      *lt2_conecos = cs_f2->coneCos;
    else
      lt.col2 *= (1 - cs_f2->coneCos);
  }
  else
  {
    lt.dir2 = lt.dir1;
    lt.col2 = Color3(0, 0, 0);
    if (lt2_conecos)
      *lt2_conecos = 0;
  }

  if (!cs_f2)
  {
    for (TmpLtRec *i = stk.data(); i < cs_f1; i++)
      ::add_hemisphere_sphharm(sh.sh, i->dir, i->coneCos, i->col);
    for (TmpLtRec *i = cs_f1 + 1; i < cs; i++)
      ::add_hemisphere_sphharm(sh.sh, i->dir, i->coneCos, i->col);
  }
  else
  {
    for (TmpLtRec *i = stk.data(); i < cs; i++)
      if (i != cs_f1 && i != cs_f2)
        ::add_hemisphere_sphharm(sh.sh, i->dir, i->coneCos, i->col);
  }
ret_light:
  /*
  debug("pre tonemap sphharm=" FMT_P3 ", " FMT_P3 ", " FMT_P3 ", " FMT_P3 "",
    sh.sh[0].r, sh.sh[0].g, sh.sh[0].b,
    sh.sh[1].r, sh.sh[1].g, sh.sh[1].b,
    sh.sh[2].r, sh.sh[2].g, sh.sh[2].b,
    sh.sh[3].r, sh.sh[3].g, sh.sh[3].b);
  */
  if (!lt1_conecos && !lt2_conecos)
    tonemapLight(sh.sh, lt);
  else
  {
    G_ASSERT(lt1_conecos && lt2_conecos);
    memcpy(lt.shAmb, sh.sh, sizeof(sh.sh[0]) * 4);
    lt.label = realQNaN;
  }

  /*debug("conecos %g dir=" FMT_P3 " col=" FMT_P3 " acol=" FMT_P3 "", lt1_conecos? *lt1_conecos : 0,
    P3D(lt.dir1), lt.col1.r, lt.col1.g, lt.col1.b
    , lt.shAmb[0].r, lt.shAmb[0].g, lt.shAmb[0].b);

  debug("sphharm=" FMT_P3 ", " FMT_P3 ", " FMT_P3 ", " FMT_P3 "",
    lt.shAmb[0].r, lt.shAmb[0].g, lt.shAmb[0].b,
    lt.shAmb[1].r, lt.shAmb[1].g, lt.shAmb[1].b,
    lt.shAmb[2].r, lt.shAmb[2].g, lt.shAmb[2].b,
    lt.shAmb[3].r, lt.shAmb[3].g, lt.shAmb[3].b);

  const real c1 = 0.429043f;
  const real c2 = 0.511664f;
  const real c3 = 0.743125f;
  const real c4 = 0.886227f;
  const real c5 = 0.247708f;
  Color3 diff;

  diff=
    c4 * lt.shAmb[SPHHARM_00] +
    (2*c2 * 0) * lt.shAmb[SPHHARM_1p1] +
    (2*c2 * 0) * lt.shAmb[SPHHARM_10 ] +
    (2*c2 * 1) * lt.shAmb[SPHHARM_1m1];
  debug("amb(0,1,0)=" FMT_P3 "", diff.r, diff.g, diff.b);

  diff=
    c4 * lt.shAmb[SPHHARM_00] +
    (2*c2 * 1) * lt.shAmb[SPHHARM_1p1] +
    (2*c2 * 0) * lt.shAmb[SPHHARM_10 ] +
    (2*c2 * 0) * lt.shAmb[SPHHARM_1m1];
  debug("amb(1,0,0)=" FMT_P3 "", diff.r, diff.g, diff.b);
  */
  return true;
}
