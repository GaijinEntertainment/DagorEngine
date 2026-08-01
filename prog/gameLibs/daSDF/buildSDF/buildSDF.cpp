// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_globalSettings.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_fileIo.h>
#include <math/dag_math3d.h>
#include <util/dag_globDef.h>
#include <EASTL/unique_ptr.h>
#include <sceneRay/dag_sceneRayBuildable.h>
#include <util/dag_threadPool.h>
#include <debug/dag_logSys.h>
#include <daSDF/generate_sdf.h>


static void show_usage();

struct Mesh
{
  dag::Vector<uint32_t> indices; // widened on read: addmesh takes uint32 either way, so both record forms land here
  dag::Vector<Point3> vertices;
  dag::Vector<mat43f> instances;
  BBox3 box;
  eastl::unique_ptr<BuildableStaticSceneRayTracer> tr;
};

#include <render/primitiveObjects.h>
#include <rendInst/riCollisionDump.h>

// Fills Mesh storage straight from the shared dump walk. Narrow (16-bit) records go through a
// scratch and are widened in endMesh; wide records are read in place. Every record is kept --
// unlike the SWRT sample path, SDF generation has no 16-bit index constraint.
struct DumpReadHandler
{
  dag::Vector<Mesh> &meshes;
  dag::Vector<uint16_t> indices16;
  bool wantMesh(int, bool)
  {
    meshes.push_back();
    return true;
  }
  void *indexBuffer(int count, bool wide)
  {
    if (wide)
    {
      meshes.back().indices.resize(count);
      return meshes.back().indices.data();
    }
    indices16.resize(count);
    return indices16.data();
  }
  Point3 *vertexBuffer(int count)
  {
    meshes.back().vertices.resize(count);
    return meshes.back().vertices.data();
  }
  mat43f *instanceBuffer(int count)
  {
    meshes.back().instances.resize(count);
    return meshes.back().instances.data();
  }
  void endMesh(int, bool wide, int index_count, int, int)
  {
    Mesh &mesh = meshes.back();
    if (!wide)
    {
      mesh.indices.resize(index_count);
      for (int i = 0; i < index_count; ++i)
        mesh.indices[i] = indices16[i];
    }
    for (auto &v : mesh.vertices)
      mesh.box += v;
  }
};

static bool loadObjects(IGenLoad &cb, dag::Vector<Mesh> &meshes)
{
  if (0)
  {
    Mesh &mesh = meshes.push_back();
    mesh.vertices.resize(8);
    BBox3 box;
    box[0] = -Point3(16, 16, 16);
    box[1] = Point3(16, 16, 16);
    for (int vertNo = 0; vertNo < 8; ++vertNo)
      mesh.vertices[vertNo] = box.point(vertNo);
    uint16_t cubeIdx[36];
    create_cubic_indices(dag::Span<uint8_t>((uint8_t *)cubeIdx, sizeof(cubeIdx)), 36, false);
    mesh.indices.assign(cubeIdx, cubeIdx + 36);
    for (auto &v : mesh.vertices)
      mesh.box += v;
  }
  if (0)
  {
    Mesh &mesh = meshes.push_back();
    mesh.vertices.resize(16);
    BBox3 box;
    box[0] = Point3(-16, -16, -16);
    box[1] = Point3(16, 16, 16);
    for (int vertNo = 0; vertNo < 8; ++vertNo)
      mesh.vertices[vertNo] = box.point(vertNo);
    box[0] = Point3(-12, -12, -12);
    box[1] = Point3(12, 12, 12);
    for (int vertNo = 0; vertNo < 8; ++vertNo)
      mesh.vertices[8 + vertNo] = box.point(7 - vertNo);
    uint16_t cubeIdx[36];
    create_cubic_indices(dag::Span<uint8_t>((uint8_t *)cubeIdx, sizeof(cubeIdx)), 36, false);
    mesh.indices.resize(36 * 2);
    for (int i = 0; i < 36; ++i)
    {
      mesh.indices[i] = cubeIdx[i];
      mesh.indices[36 + i] = cubeIdx[i] + 8;
    }
    for (auto &v : mesh.vertices)
      mesh.box += v;
  }
#if 0
  {
    Mesh &mesh = meshes.push_back();
    mesh.vertices.resize(8);
    BBox3 box;box[0] = -Point3(1,16,16);box[1] = Point3(1,16,16);
    for (int vertNo = 0; vertNo < 8; ++vertNo)
      mesh.vertices[vertNo] = box.point(7-vertNo);
    mesh.indices.resize(36);
	create_cubic_indices(dag::Span<uint8_t>((uint8_t*)mesh.indices.data(), 36*sizeof(uint16_t)), 36, false);
    for (auto &v:mesh.vertices)
      mesh.box += v;
  }
  {
    Mesh &mesh = meshes.push_back();
    mesh.vertices.resize(8);
    BBox3 box;box[0] = -Point3(16,16,16);box[1] = Point3(16,16,16);
    for (int vertNo = 0; vertNo < 8; ++vertNo)
      mesh.vertices[vertNo] = box.point(7-vertNo);
    mesh.indices.resize(36);
	create_cubic_indices(dag::Span<uint8_t>((uint8_t*)mesh.indices.data(), 36*sizeof(uint16_t)), 36, false);
    for (auto &v:mesh.vertices)
      mesh.box += v;
  }
  {
    Mesh &mesh = meshes.push_back();
    mesh.vertices.resize(16);
    BBox3 box;
    box[0] = Point3(-16,-16,-16);box[1] = Point3(-14,16,16);
    for (int vertNo = 0; vertNo < 8; ++vertNo)
      mesh.vertices[vertNo] = box.point(7-vertNo);
    box[0] = Point3(14,-16,-16);box[1] = Point3(16,16,16);
    for (int vertNo = 0; vertNo < 8; ++vertNo)
      mesh.vertices[8 + vertNo] = box.point(7-vertNo);
    mesh.indices.resize(36*2);
	create_cubic_indices(dag::Span<uint8_t>((uint8_t*)mesh.indices.data(), 36*sizeof(uint16_t)), 36, false);
	create_cubic_indices(dag::Span<uint8_t>((uint8_t*)(mesh.indices.data()+36), 36*sizeof(uint16_t)), 36, false);
    for (int i = 36; i < 72; ++i)
      mesh.indices[i] += 8;
    for (auto &v:mesh.vertices)
      mesh.box += v;
  }
  {
    Mesh &mesh = meshes.push_back();
    uint32_t vc, fc;
    calc_sphere_vertex_face_count(32, 32, false, vc, fc);
    mesh.indices.resize(fc*3);
    mesh.vertices.resize(vc);
    create_sphere_mesh(dag::Span<uint8_t>((uint8_t*)mesh.vertices.data(), vc*sizeof(Point3)),
                       dag::Span<uint8_t>((uint8_t*)mesh.indices.data(), fc*3*sizeof(uint16_t)),
                       8.0f, 32, 32, sizeof(Point3), false, false, false, false);
    for (auto &v:mesh.vertices)
      mesh.box += v;
  }
#endif
  DumpReadHandler h{meshes};
  return read_ri_collision_dump(cb, h);
}

static void generateSDF(dag::Vector<Mesh> &meshes, IGenSave &scb, float density)
{
  int verts = 0, inds = 0, inst = 0;
  for (auto &m : meshes)
  {
    verts += m.vertices.size();
    inds += m.indices.size();
    inst += m.instances.size();
  }
  printf("total %d meshes, %d vertices, %d tri, %d instances\n", (int)meshes.size(), verts, inds / 3, inst);
  for (auto &m : meshes)
  {
    Point3 sz = m.box.width();
    float vol = max(sz.x, 1.0f) * max(sz.y, 1.0f) * max(sz.z, 1.0f);
    int facesCount = m.indices.size() / 3;
    float faceDensity = facesCount / vol;
    G_UNUSED(faceDensity);
    float maxSz = max(max(sz.x, sz.y), sz.z);
    float maxLeaf = min(1.0f, maxSz / 4.0f); // if object in max dimension is smaller than 4 meters, than use it max axis/4 as biggest
    sz = Point3(max(sz.x / 8.0f, maxLeaf), max(sz.y / 3.0f, maxLeaf), max(sz.z / 8.0f, maxLeaf));
    sz = max(sz, Point3(0.25f, 0.25f, 0.25f)); // there is no much sense in leaf size less than 0.25 cm. Even for raytracing it is too
                                               // detailed
    m.tr.reset(new BuildableStaticSceneRayTracer(sz, 3));
    m.tr->addmesh(m.vertices.data(), m.vertices.size(), m.indices.data(), sizeof(uint32_t) * 3, m.indices.size() / 3, nullptr, false);
    m.tr->setCullFlags(StaticSceneRayTracer::CULL_BOTH);
    m.tr->rebuild(true);
  }
  size_t cnt = 0, maxCnt = 64 << 10;
  // scb.writeInt(meshes.size());
  scb.writeInt(min<size_t>(maxCnt, meshes.size()));
  for (auto &m : meshes)
  {
    int c = cnt++;
    // if (c != 1263)
    //   continue;
    MippedMeshSDF meshSDF;
    generate_sdf(*m.tr, meshSDF, density, 512);
    if (c >= maxCnt)
      break;
    scb.write(&meshSDF.mipCountTwoSided, sizeof(meshSDF.mipCountTwoSided));
    scb.write(&meshSDF.localBounds, sizeof(meshSDF.localBounds));
    scb.write(meshSDF.mipInfo.data(), meshSDF.mipCount() * sizeof(meshSDF.mipInfo[0]));
    scb.writeInt(meshSDF.compressedMips.size());
    scb.write(meshSDF.compressedMips.data(), meshSDF.compressedMips.size());
    printf(" %3d/%3d\r", uint32_t(c), uint32_t(meshes.size()));
  }
}

int DagorWinMain(bool debugmode)
{
  start_classic_debug_system("debug", false);
  dd_get_fname(""); //== pull in directoryService.obj
  printf("SDF build generattor\n");
  printf("Copyright (c) Gaijin Games KFT, 2026\n");
  int ctime = time(NULL);

  if (dgs_argc < 2)
  {
    ::show_usage();
    return 1;
  }

  cpujobs::init(-1);
  int num_workers = cpujobs::get_core_count() - 1;
  if (dgs_argc > 2)
    num_workers = clamp(atoi(dgs_argv[2]), 0, 64);
  int stack_size = 128 << 10;
  threadpool::init(num_workers, 2048, stack_size);
  printf("process in %d workers\n", num_workers);

  dd_add_base_path("");

  printf("Loading file...\n");
  FullFileLoadCB cb(dgs_argv[1]);
  if (!cb.fileHandle)
  {
    printf("ERROR LOADING file <%s>\n", dgs_argv[1]);
    return 2;
  }
  init_sdf_generate();
  // Load before opening the output: a malformed/truncated dump must fail the run, not leave a
  // partial or empty sdf.bin behind.
  dag::Vector<Mesh> meshes;
  if (!loadObjects(cb, meshes))
  {
    printf("ERROR: malformed or truncated collision dump <%s>\n", dgs_argv[1]);
    return 3;
  }
  FullFileSaveCB scb(dgs_argc > 3 ? dgs_argv[3] : "sdf.bin");
  float density = dgs_argc > 4 ? atof(dgs_argv[4]) : 2.0f;
  printf("%f density\n", density);
  generateSDF(meshes, scb, density);
  printf("done in %ds\n", int(time(NULL) - ctime));
  threadpool::shutdown();
  cpujobs::term(true, 1000);
  return 0;
}


//==============================================================================
static void show_usage() { printf("usage: buildSDF <file name in> \n"); }
