// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaderCommon.h>
#include <scene/dag_loadLevelVer.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_btagCompr.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <generic/dag_patchTab.h>
#include <generic/dag_tab.h>
#include <math/dag_mathBase.h>
#include <math/dag_bounds3.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <util/dag_globDef.h>
#include <stdio.h>

// Lists land mesh triangles that use index 0xFFFF in a 16-bit index buffer: D3D11 takes it as the strip cut and
// drops the triangle. Tools before the MAX_VERTEX_16 fix could write such indices. No shaders are loaded, so the
// materials stay null and only the vertex data and the mesh elements are read.
namespace
{
const char *const MESH_KIND[] = {"land lod0", "land lod1", "decal", "combined", "patches"};
enum
{
  KIND_DECAL = 2,
  KIND_COMBINED,
  KIND_PATCHES,
  KIND_COUNT
};

enum PosFormat : uint8_t
{
  POS_UNKNOWN,
  POS_SHORT4,
  POS_FLOAT
};

struct KindStats
{
  int tris = 0;
  double maxArea = 0, totalArea = 0;
};

struct CheckCtx
{
  ShaderMatVdata *smvd = nullptr;
  Tab<uint8_t> posFormat; // per vdata: format of channel 0, the cell-local position (land: SHORT4, decals: SHORT4N or FLOAT)
  float cellSize = 0;
  IPoint2 origin;
  KindStats kinds[KIND_COUNT];
  int badTris = 0, badElems = 0;
};

// Positions of all cell meshes are in [-1, 1] of their cell (compress_land_position)
bool decode_xz(const CheckCtx &ctx, const GlobalVertexData &vd, int fmt, int vi, int cell_x, int cell_y, Point2 &out)
{
  const int vertNum = *(const int *)vd.getVBMem();
  if (vi >= vertNum)
    return false;
  const uint8_t *v = (const uint8_t *)(1 + (const int *)vd.getVBMem()) + vi * vd.getStride();
  float x, z;
  if (fmt == POS_SHORT4)
    x = ((const int16_t *)v)[0] / 32767.f, z = ((const int16_t *)v)[2] / 32767.f;
  else if (fmt == POS_FLOAT)
    x = ((const float *)v)[0], z = ((const float *)v)[2];
  else
    return false;
  out.set(ctx.cellSize * (cell_x + ctx.origin.x + 0.5f + x * 0.5f), ctx.cellSize * (cell_y + ctx.origin.y + 0.5f + z * 0.5f));
  return true;
}

void check_mesh(CheckCtx &ctx, const ShaderMesh *sm, int kind, int cell_x, int cell_y)
{
  if (!sm)
    return;
  for (const ShaderMesh::RElem &re : sm->getAllElems())
  {
    const GlobalVertexData *vd = re.vertexData;
    if (!vd || !re.numf || vd->getIbElemSz() != 2)
      continue;
    const uint16_t *idx = vd->getIBMem<uint16_t>(re.si, re.numf);
    if (!idx)
      continue;
    const int fmt = ctx.posFormat[vd - ctx.smvd->getGlobVData(0)];
    int elemBad = 0;
    for (int f = 0; f < re.numf; f++)
    {
      const uint16_t *t = idx + f * 3;
      if (t[0] != 0xFFFF && t[1] != 0xFFFF && t[2] != 0xFFFF)
        continue;
      elemBad++;
      Point2 p[3];
      bool hasPos = true;
      for (int c = 0; c < 3; c++)
        hasPos &= decode_xz(ctx, *vd, fmt, re.baseVertex + t[c], cell_x, cell_y, p[c]);

      KindStats &ks = ctx.kinds[kind];
      ks.tris++;
      if (hasPos)
      {
        double area = 0.5 * fabs(double(p[1].x - p[0].x) * (p[2].y - p[0].y) - double(p[1].y - p[0].y) * (p[2].x - p[0].x));
        ks.maxArea = max(ks.maxArea, area);
        ks.totalArea += area;
        printf("  cell %d,%d %s: tri %d idx %d,%d,%d (%.0f,%.0f) (%.0f,%.0f) (%.0f,%.0f) area %.0f m2\n", cell_x, cell_y,
          MESH_KIND[kind], f, t[0], t[1], t[2], p[0].x, p[0].y, p[1].x, p[1].y, p[2].x, p[2].y, area);
      }
      else
        printf("  cell %d,%d %s: tri %d idx %d,%d,%d (position format %d, stride %d: no coordinates)\n", cell_x, cell_y,
          MESH_KIND[kind], f, t[0], t[1], t[2], fmt, vd->getStride());
    }
    if (elemBad)
      ctx.badTris += elemBad, ctx.badElems++;
  }
}

void check_block(CheckCtx &ctx, IGenLoad &crd, int kind, int cell_x, int cell_y)
{
  crd.beginBlock();
  ShaderMesh *sm = crd.getBlockRest() ? ShaderMesh::load(crd, crd.getBlockRest(), *ctx.smvd) : nullptr;
  check_mesh(ctx, sm, kind, cell_x, cell_y);
  del_it(sm);
  crd.endBlock();
}

// ShaderMatVdata drops the vertex declarations after load, so read them from the header that loadMatVdata() reads next, unpacked the
// same way; the reader returns to where it was. The structs mirror ShaderMatVdata::MatVdataHdr in matVdataLoad.cpp.
bool read_pos_formats(IGenLoad &crd, int mvhdr_sz, int vdata_count, Tab<uint8_t> &out_fmt)
{
  struct VdataHdr
  {
    uint32_t vertNum, vertStride : 8, packedIdxSizeLo : 24, idxSize : 28, packedIdxSizeHi : 4, flags;
    PatchableTab<CompiledShaderChannelId> vDecl;
  };
  struct MatVdataHdr
  {
    PatchableTab<const uint8_t> mat;
    PatchableTab<VdataHdr> vdata;
  };

  int comprType = 0, hdrSz = mvhdr_sz;
  if ((hdrSz & 0xE0000000) == 0xC0000000)
    comprType = 3, hdrSz &= ~0xC0000000;
  else if (hdrSz < 0)
    comprType = 1, hdrSz = -hdrSz;
  else if (hdrSz & 0x40000000)
    comprType = 2, hdrSz &= ~0x40000000;

  const int startPos = crd.tell();
  Tab<uint8_t> buf;
  buf.resize(hdrSz);
  bool ok = true;
  if (comprType == 3)
  {
    unsigned tag = 0;
    crd.beginBlock(&tag);
    const int comprSz = crd.getBlockRest();
    if (tag == btag_compr::ZSTD)
    {
      ZstdLoadCB zcrd(crd, comprSz);
      zcrd.read(buf.data(), hdrSz);
      zcrd.ceaseReading();
    }
    else if (tag == btag_compr::OODLE)
    {
      const int rawSz = crd.readInt();
      OodleLoadCB zcrd(crd, comprSz - 4, rawSz);
      zcrd.read(buf.data(), hdrSz);
      zcrd.ceaseReading();
    }
    else
      ok = false;
    crd.endBlock();
  }
  else if (comprType == 1)
  {
    ZlibLoadCB zcrd(crd, 0x7FFFFFFF);
    zcrd.read(buf.data(), hdrSz);
    zcrd.ceaseReading();
  }
  else if (comprType == 2)
  {
    LzmaLoadCB zcrd(crd, 0x7FFFFFFF);
    zcrd.read(buf.data(), hdrSz);
    zcrd.ceaseReading();
  }
  else
    crd.read(buf.data(), hdrSz);
  crd.seekto(startPos);

  MatVdataHdr *hdr = (MatVdataHdr *)buf.data();
  hdr->vdata.patch(hdr);
  if (!ok || hdr->vdata.size() != vdata_count)
    return false;
  out_fmt.resize(vdata_count);
  for (int i = 0; i < vdata_count; i++)
  {
    hdr->vdata[i].vDecl.patch(hdr);
    const int t = hdr->vdata[i].vDecl.size() ? hdr->vdata[i].vDecl[0].t : -1;
    out_fmt[i] = (t == SCTYPE_SHORT4N || t == SCTYPE_SHORT4)  ? POS_SHORT4
                 : (t == SCTYPE_FLOAT3 || t == SCTYPE_FLOAT4) ? POS_FLOAT
                                                              : POS_UNKNOWN;
  }
  return true;
}
} // namespace

// Mirrors LandMeshManager::loadDump() and loadMeshData() up to the cell meshes. Returns -1 on a format error, else the count of bad
// triangles.
static int check_lmap(IGenLoad &crd)
{
  if (crd.readInt() != _MAKE4C('lndm'))
  {
    printf("  ERROR: lmap is not lndm\n");
    return -1;
  }
  if (int ver = crd.readInt(); ver != 4)
  {
    printf("  ERROR: unsupported lndm version %d\n", ver);
    return -1;
  }
  crd.readReal(); // gridCellSize
  CheckCtx ctx;
  ctx.cellSize = crd.readReal();
  int mapSizeX = crd.readInt(), mapSizeY = crd.readInt();
  ctx.origin.x = crd.readInt();
  ctx.origin.y = crd.readInt();
  crd.readInt(); // useTile
  int baseOfs = crd.tell();
  int meshMapOfs = baseOfs + crd.readInt(), detailDataOfs = baseOfs + crd.readInt();
  // -stripGfx keeps only the cell boundings between the two offsets
  if (meshMapOfs < baseOfs + 16 || detailDataOfs - meshMapOfs <= mapSizeX * mapSizeY * int(sizeof(BBox3) + sizeof(float)))
  {
    printf("  no land mesh render data (stripped)\n");
    return 0;
  }
  crd.seekto(meshMapOfs);

  int hdr[4];
  crd.read(hdr, sizeof(hdr));
  Ptr<ShaderMatVdata> smvd = ShaderMatVdata::create(hdr[0], hdr[1], hdr[2], hdr[3]);
  ctx.smvd = smvd;
  crd.seekrel(crd.readInt()); // texture names: not needed without materials
  if (!read_pos_formats(crd, hdr[3], hdr[2], ctx.posFormat))
  {
    printf("  ERROR: cannot read the vertex declarations\n");
    return -1;
  }
  smvd->loadMatVdata("landmesh", crd, VDATA_NO_IBVB);

  printf("  %dx%d cells of %.0f m, origin %d,%d, %d vdata\n", mapSizeX, mapSizeY, ctx.cellSize, ctx.origin.x, ctx.origin.y,
    smvd->getGlobVDataCount());

  crd.beginBlock();
  for (int y = 0; y < mapSizeY; y++)
    for (int x = 0; x < mapSizeX; x++)
    {
      crd.beginBlock();

      crd.beginBlock();
      for (int lod = 0; crd.getBlockRest() && lod < KIND_DECAL; lod++)
        check_block(ctx, crd, lod, x, y);
      crd.endBlock();

      for (int kind = KIND_DECAL; kind < KIND_COUNT && crd.getBlockRest(); kind++)
        check_block(ctx, crd, kind, x, y);

      crd.endBlock();
    }
  crd.endBlock();

  printf("  %d triangles in %d elements use index 0xFFFF\n", ctx.badTris, ctx.badElems);
  for (int k = 0; k < KIND_COUNT; k++)
    if (ctx.kinds[k].tris)
      printf("  %s: %d triangles, max area %.0f m2, total area %.0f m2\n", MESH_KIND[k], ctx.kinds[k].tris, ctx.kinds[k].maxArea,
        ctx.kinds[k].totalArea);
  return ctx.badTris;
}

// Returns 0 when the level is clean, 2 when it has dropped triangles (re-export it), 13 on an error.
// Not 1: a fatal error exits with 1.
int checkDbldLmeshIdx16(IGenLoad &crd)
{
  if (crd.readInt() != _MAKE4C('DBLD') || crd.readInt() != DBLD_Version)
  {
    printf("ERROR: invalid DBLD <%s>\n", crd.getTargetName());
    return 13;
  }
  crd.readInt(); // total tex number

  int bad = 0;
  bool hasLmap = false;
  for (;;)
  {
    int tag = crd.beginTaggedBlock();
    if (tag == _MAKE4C('END'))
    {
      crd.endBlock();
      break;
    }
    if (tag == _MAKE4C('lmap'))
    {
      hasLmap = true;
      int n = check_lmap(crd);
      if (n < 0)
        return 13;
      bad += n;
    }
    crd.endBlock();
  }
  if (!hasLmap)
    printf("  no lmap\n");
  printf("%s: %s\n", crd.getTargetName(), bad ? "RE-EXPORT (dropped land triangles)" : "ok");
  return bad ? 2 : 0;
}
