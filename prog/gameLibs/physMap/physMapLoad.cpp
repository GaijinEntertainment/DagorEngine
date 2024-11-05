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
#include <math/integer/dag_IPoint2.h>
#include <physMap/physMatSwRenderer.h>
#include <supp/dag_alloca.h>

static inline IGenLoad &ptr_to_ref(IGenLoad *crd) { return *crd; }

void make_grid_decals(PhysMap &phys_map, int sz)
{
  phys_map.gridDecals.resize(sz * sz);
  phys_map.gridScale = float(phys_map.size) / sz * phys_map.scale;
  phys_map.invGridScale = safeinv(phys_map.gridScale);
  phys_map.gridSz = sz;

  Tab<int> gridIndex;
  gridIndex.resize(sz * sz);

  size_t indicesCount = 0;
  for (const PhysMap::DecalMesh &mesh : phys_map.decals)
    indicesCount += mesh.matIndices.size();

  debug("[physmap] Making grid for decals, num of decals %d, num of indices %d", phys_map.decals.size(), indicesCount);

  for (const PhysMap::DecalMesh &mesh : phys_map.decals)
  {
    mem_set_ff(gridIndex);
    // iterate over all indices
    for (const PhysMap::DecalMesh::MaterialIndices &indices : mesh.matIndices)
    {
      for (int i = 0; i < indices.indices.size(); i += 3)
      {
        uint16_t i0 = indices.indices[i + 0];
        uint16_t i1 = indices.indices[i + 1];
        uint16_t i2 = indices.indices[i + 2];
        Point2 v0 = mesh.vertices[i0];
        Point2 v1 = mesh.vertices[i1];
        Point2 v2 = mesh.vertices[i2];

        // Find boundings of this triangle
        Point2 leftTop = min(v0, min(v1, v2));
        Point2 rightBottom = max(v0, max(v1, v2));
        IPoint2 leftTopCell = IPoint2::xy((leftTop - phys_map.worldOffset) * phys_map.invGridScale);
        IPoint2 rightBottomCell = IPoint2::xy((rightBottom - phys_map.worldOffset) * phys_map.invGridScale);

        // Fill indices in the grid decal meshes in the proper grid cell
        for (int y = max(leftTopCell.y, 0); y <= min(rightBottomCell.y, sz - 1); ++y)
          for (int x = max(leftTopCell.x, 0); x <= min(rightBottomCell.x, sz - 1); ++x)
          {
            size_t cellId = y * sz + x;
            // Find/allocate gridIndex for this decal mesh
            if (gridIndex[cellId] < 0)
            {
              gridIndex[cellId] = phys_map.gridDecals[cellId].size();
              phys_map.gridDecals[cellId].push_back(PhysMap::DecalMesh());
            }
            PhysMap::DecalMesh &gridMesh = phys_map.gridDecals[cellId][gridIndex[cellId]];
            int matIdx = -1;
            for (int j = 0; j < gridMesh.matIndices.size(); ++j)
              if (gridMesh.matIndices[j].matId == indices.matId && gridMesh.matIndices[j].bitmapTexId == indices.bitmapTexId)
              {
                matIdx = j;
                break;
              }
            if (matIdx < 0)
            {
              // Allocate mat indices if we haven't found one
              matIdx = gridMesh.matIndices.size();
              gridMesh.matIndices.push_back();

              gridMesh.matIndices[matIdx].matId = indices.matId;
              gridMesh.matIndices[matIdx].bitmapTexId = indices.bitmapTexId;
            }

            // Push to this grid indices
            gridMesh.matIndices[matIdx].indices.push_back(i0);
            gridMesh.matIndices[matIdx].indices.push_back(i1);
            gridMesh.matIndices[matIdx].indices.push_back(i2);
            if (indices.tindices.size())
            {
              gridMesh.matIndices[matIdx].tindices.push_back(indices.tindices[i + 0]);
              gridMesh.matIndices[matIdx].tindices.push_back(indices.tindices[i + 1]);
              gridMesh.matIndices[matIdx].tindices.push_back(indices.tindices[i + 2]);
            }
          }
      }
    }

    // For each grid - populate vertices and remap indices
    // Each vertex in the original mesh is remapped to
    // another vertex in the grid cell mesh as it's being compacted.
    // Then indices are remapped, so they point to the proper vertices in the
    // grid cell mesh.
    for (size_t i = 0; i < phys_map.gridDecals.size(); ++i)
    {
      int cellId = gridIndex[i];
      if (cellId < 0)
        continue; // This mesh is not included in this grid cell

      PhysMap::DecalMesh &cellMesh = phys_map.gridDecals[i][cellId];
      Tab<uint16_t> usedIndices;
      Tab<uint16_t> usedTIndices;
      for (const PhysMap::DecalMesh::MaterialIndices &mi : cellMesh.matIndices)
      {
        for (uint16_t idx : mi.indices)
          usedIndices.push_back(idx);
        for (uint16_t idx : mi.tindices)
          usedTIndices.push_back(idx);
      }
      stlsort::sort(usedIndices.begin(), usedIndices.end(), [](const uint16_t &lhs, const uint16_t &rhs) { return lhs < rhs; });
      stlsort::sort(usedTIndices.begin(), usedTIndices.end(), [](const uint16_t &lhs, const uint16_t &rhs) { return lhs < rhs; });

      Tab<uint16_t> verticesRemap;
      Tab<uint16_t> texCoordsRemap;
      verticesRemap.resize(mesh.vertices.size());
      texCoordsRemap.resize(mesh.texCoords.size());
      mem_set_ff(verticesRemap);
      mem_set_ff(texCoordsRemap);

      // build remap
      for (size_t j = 0; j < usedIndices.size(); ++j)
        verticesRemap[usedIndices[j]] = j;
      for (size_t j = 0; j < usedTIndices.size(); ++j)
        texCoordsRemap[usedTIndices[j]] = j;

      // populate vertices and texcoords (separated from previous step for better
      // readability)
      for (size_t j = 0; j < usedIndices.size(); ++j)
      {
        const Point2 &v = mesh.vertices[usedIndices[j]];
        cellMesh.vertices.push_back(v);
        cellMesh.box += v;
      }
      for (size_t j = 0; j < usedTIndices.size(); ++j)
        cellMesh.texCoords.push_back(mesh.texCoords[usedTIndices[j]]);

      // complete a remap in matIndices
      for (PhysMap::DecalMesh::MaterialIndices &mi : cellMesh.matIndices)
      {
        for (uint16_t &idx : mi.indices)
          idx = verticesRemap[idx];
        for (uint16_t &idx : mi.tindices)
          idx = texCoordsRemap[idx];
      }
    }
  }
  // Clear original decals
  clear_and_shrink(phys_map.decals);
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
  const int decal_render_reg = 128;

  PhysMap *physMap = load_phys_map(crd, is_lmp2);
  if (physMap->size == 0)
    return physMap;

  int sz = physMap->size;
  int regions = sz / decal_render_reg;
  G_ASSERT_RETURN((sz % decal_render_reg) == 0, physMap);

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
          resultPhysData[(py + ofsy) * sz + ofsx + px] = matIdsRemap[decalRegData[py * decal_render_reg + px]];
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

  return physMap;
}
