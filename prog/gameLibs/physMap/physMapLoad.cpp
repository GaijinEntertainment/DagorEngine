// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_tab.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_btagCompr.h>
#include <memory/dag_framemem.h>
#include <memory/dag_physMem.h>
#include <util/dag_stlqsort.h>
#include <util/dag_simpleString.h>
#include <util/dag_treeBitmap.h>
#include <scene/dag_physMat.h>
#include <physMap/physMap.h>
#include <physMap/physMapLoad.h>
#include <physMap/physMapCompactDecals.h>
#include <math/integer/dag_IPoint2.h>
#include <physMap/physMatSwRenderer.h>
#include <supp/dag_alloca.h>

// #define UNIT_TEST_DATA 1
#if UNIT_TEST_DATA
#include <image/dag_tga.h>
#include <perfMon/dag_perfTimer.h>
#include <util/dag_string.h>
#endif

static inline IGenLoad &ptr_to_ref(IGenLoad *crd) { return *crd; }

void make_grid_decals(PhysMap &phys_map, int sz)
{
  phys_map.compactDecals = new PhysMapCompactDecals;
  if (!build_compact_decals(phys_map, sz, *phys_map.compactDecals))
  {
    // encoder logerr'd why; the source decals stay and render un-gridded;
    // grid fields stay unset so gridSz > 0 implies compact storage exists
    del_it(phys_map.compactDecals);
    return;
  }
  phys_map.gridScale = float(phys_map.size) / sz * phys_map.scale;
  phys_map.invGridScale = safeinv(phys_map.gridScale);
  phys_map.gridSz = sz;
  debug("[physmap] compact grid decals: %d chunks, %d cell entries, %dK", (int)phys_map.compactDecals->chunks.size(),
    (int)phys_map.compactDecals->cellEntries.size(), int(phys_map.compactDecals->memBytes() >> 10));
  clear_and_shrink(phys_map.decals);

#if UNIT_TEST_DATA
  static constexpr int width = 64;
  Tab<uint8_t> ids(width * width);
  int64_t total = 0;
  const int quads = 512;
  float quadsScale = float(phys_map.size) / quads * phys_map.scale;
  for (int i = 0, ie = quads * quads; i < ie; ++i)
  {
    int x = i % quads, y = i / quads;
    Point2 lt = Point2(x, y) * quadsScale + phys_map.worldOffset;
    BBox2 region(lt, lt + Point2(quadsScale, quadsScale));
    RenderDecalMaterials<width, width> decalRenderer(make_span(ids));
    int64_t ref = profile_ref_ticks();
    decalRenderer.renderPhysMap(phys_map, region);
    total += profile_ref_ticks() - ref;

#if UNIT_TEST_DATA == 2
    if (eastl::find_if(ids.begin(), ids.end(), [&](auto a) { return a != 0xFF; }) != ids.end())
      save_tga8(String(64, "physmap/physmap_%03d.tga", i), ids.begin(), width, width, width);
#endif
  }
  debug("rendererd in %dus", profile_usec_from_ticks_delta(total));
#endif
}

PhysMap *load_phys_map(IGenLoad &crd, bool is_lmp2)
{
  int version = 0;
  if (is_lmp2)
    crd.readInt(version);

  int nameCount = 0;
  crd.readInt(nameCount);

  Tab<int> matIds(framemem_ptr());
  matIds.resize(nameCount);
  for (int i = 0; i < nameCount; ++i)
  {
    SimpleString matName;
    crd.readString(matName);
    matIds[i] = PhysMat::getMaterialId(matName.str());
#if DAGOR_DBGLEVEL > 0
    if (matIds[i] != PHYSMAT_INVALID)
      debug("material '%s' found in LMpm", matName.str());
#endif
  }

  if (version == 0)
  {
    int physMap1Width = 0;
    int physMap1Height = 0;
    crd.readInt(physMap1Width);
    crd.readInt(physMap1Height);

    Point2 hmapOffset(0.f, 0.f);
    crd.readReal(hmapOffset.x);
    crd.readReal(hmapOffset.y);

    float gridCellSizeDivLcmScale = 1.f;
    crd.readReal(gridCellSizeDivLcmScale);
  }

  int physMapWidth = 0;
  int physMapHeight = 0;
  crd.readInt(physMapWidth);
  crd.readInt(physMapHeight);

  Point2 detRect(0.f, 0.f);
  crd.readReal(detRect.x);
  crd.readReal(detRect.y);

  float detScale = 1.f;
  crd.readReal(detScale);

  unsigned fmt = 0;
  int blockSize = crd.beginBlock(&fmt);
  IGenLoad *zcrd_p = NULL;
  if (fmt == btag_compr::OODLE)
  {
    int src_sz = crd.readInt();
    zcrd_p = new (alloca(sizeof(OodleLoadCB)), _NEW_INPLACE) OodleLoadCB(crd, crd.getBlockRest(), src_sz);
  }
  else if (fmt == btag_compr::ZSTD)
    zcrd_p = new (alloca(sizeof(ZstdLoadCB)), _NEW_INPLACE) ZstdLoadCB(crd, blockSize);
  else
    zcrd_p = new (alloca(sizeof(LzmaLoadCB)), _NEW_INPLACE) LzmaLoadCB(crd, blockSize);
  IGenLoad &zcrd = *zcrd_p;

  if (version == 0)
  {
    TreeBitmapNode temp; // read lvl1 bitmap, but ignore it, we don't use it atm
    temp.load(zcrd);
  }
  PhysMap *physMap = new PhysMap;
  physMap->parent = new TreeBitmapNode;
  physMap->parent->load(zcrd);

  int decalNodes = zcrd.readIntP<2>();
  for (int i = 0; i < decalNodes; ++i)
  {
    PhysMap::DecalMesh &decal = physMap->decals.push_back();
    int verticesCount = zcrd.readIntP<2>();
    if (verticesCount > 0)
    {
      decal.vertices.resize(verticesCount);
      zcrd.readTabData(decal.vertices);
    }
    if (version > 0)
    {
      int texVertCount = zcrd.readIntP<2>();
      decal.texCoords.resize(texVertCount);
      zcrd.readTabData(decal.texCoords);
    }
    for (int vi = 0; vi < verticesCount; ++vi)
      decal.box += decal.vertices[vi];
    for (;;)
    {
      int curMatId = zcrd.readIntP<1>();
      int numFacesToRead = zcrd.readIntP<2>();
      if (numFacesToRead <= 0)
        break;

      PhysMap::DecalMesh::MaterialIndices &matIndices = decal.matIndices.push_back();
      matIndices.matId = matIds[curMatId];
      matIndices.indices.resize(numFacesToRead * 3);
      matIndices.bitmapTexId = -1;
      zcrd.readTabData(matIndices.indices);
      if (version > 0)
      {
        matIndices.bitmapTexId = zcrd.readIntP<2>();
        matIndices.tindices.resize(numFacesToRead * 3);
        zcrd.readTabData(matIndices.tindices);
      }
    }
  }
  if (version > 0)
  {
    int numTextures = zcrd.readIntP<2>();
    debug("numTextures '%d'", numTextures);
    if (numTextures > 0)
      physMap->physTextures.resize(numTextures);
    for (int i = 0; i < numTextures; ++i)
      zcrd.read(physMap->physTextures[i].pixels.data(), data_size(physMap->physTextures[i].pixels));
  }
  zcrd.ceaseReading();
  zcrd.~IGenLoad();
  crd.endBlock();

  physMap->worldOffset = detRect;
  physMap->scale = detScale;
  physMap->invScale = safeinv(physMap->scale);
  physMap->size = physMapWidth;
  physMap->materials = matIds;

  return physMap;
}

PhysMap *load_phys_map_with_decals(IGenLoad &crd, bool is_lmp2)
{
  PhysMap *physMap = load_phys_map(crd, is_lmp2);
  if (physMap->size != 0)
    load_phys_map_decals(physMap);
  return physMap;
}

void load_phys_map_decals(PhysMap *physMap)
{
  const int decal_render_reg = 128;
  int sz = physMap->size;
  int regions = sz / decal_render_reg;
  G_ASSERT_RETURN((sz % decal_render_reg) == 0, );

  Tab<int> matIdsRemap(framemem_ptr());
  for (int i = 0; i < physMap->materials.size(); ++i)
  {
    int id = physMap->materials[i];
    while (id >= matIdsRemap.size())
      matIdsRemap.push_back(0xff);
    matIdsRemap[id] = i;
  }

  Tab<uint8_t> decalRegData(decal_render_reg * decal_render_reg, framemem_ptr());

  using namespace dagor_phys_memory;
  uint8_t *resultPhysData = (uint8_t *)defaultmem->tryAlloc(sz * sz);
  bool resultPhysDataAtPhysMem = !resultPhysData;
  if (!resultPhysData)
    resultPhysData = (uint8_t *)alloc_phys_mem(sz * sz);

  RenderDecalMaterials<decal_render_reg, decal_render_reg> decalRender(make_span(decalRegData));

  for (int ry = 0; ry < regions; ++ry)
  {
    for (int rx = 0; rx < regions; ++rx)
    {
      BBox2 box(Point2(rx * decal_render_reg, ry * decal_render_reg) * physMap->scale,
        Point2((rx + 1) * decal_render_reg, (ry + 1) * decal_render_reg) * physMap->scale);
      box.lim[0] += physMap->worldOffset;
      box.lim[1] += physMap->worldOffset;

      decalRender.renderPhysMap(*physMap, box);

      int ofsx = rx * decal_render_reg;
      int ofsy = ry * decal_render_reg;
      for (int py = 0; py < decal_render_reg; ++py)
      {
        for (int px = 0; px < decal_render_reg; ++px)
        {
          uint8_t matId = decalRegData[py * decal_render_reg + px];
          resultPhysData[(py + ofsy) * sz + ofsx + px] = matId < matIdsRemap.size() ? matIdsRemap[matId] : 0xff;
        }
      }
    }
  }

  del_it(physMap->parent);
  physMap->parent = new TreeBitmapNode;
  physMap->parent->create(make_span_const(resultPhysData, sz * sz), IPoint2(sz, sz));

  if (!resultPhysDataAtPhysMem)
    defaultmem->free(resultPhysData);
  else
    free_phys_mem(resultPhysData);
}
