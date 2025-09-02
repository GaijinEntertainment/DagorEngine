// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../collision_builder.h"
#include <phys/dag_physics.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <scene/dag_physMat.h>
#include <coolConsole/coolConsole.h>
#include <de3_interface.h>
#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/staticGeom/matFlags.h>
#include <libTools/util/makeBindump.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_Point4.h>
#include <math/dag_capsule.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_ioUtils.h>
#include <util/dag_oaHashNameMap.h>

#include "../physMesh.h"

#define BULLET_SCENE_COLLISION_VERSION 1

extern Tab<BBox3> phys_actors_bbox;
extern Tab<TMatrix> phys_box_actors;
extern Tab<Point4> phys_sph_actors;
extern Tab<TMatrix> phys_cap_actors;

class BulletSceneCollisionSaver
{
public:
  struct CollPrim
  {
    Tab<TMatrix> box;
    Tab<Point4> sph;
    Tab<TMatrix> cap;

    CollPrim() : box(tmpmem), sph(tmpmem), cap(tmpmem) {}
  };
  struct SingleActor : public PhysMesh
  {
    BBox3 box;

    SingleActor() {}
    void buildAndSaveCollisionMesh(mkbindump::BinDumpSaveCB &cwr);
    void attach(const PhysMesh &pm, bool opt_vert = false)
    {
      int vs = vert.size();
      attachMesh(pm, opt_vert);
      calcBbox(box, vs);
    }
  };
  HierGrid2<SingleActor, 0> tiles;

public:
  BulletSceneCollisionSaver() : perMatColl(tmpmem), actors(tmpmem), tiles(5)
  {
    physInvTm.identity();
    physInvTm.setcol(1, Point3(0, 0, 1));
    physInvTm.setcol(2, -Point3(0, 1, 0));
  }

  ~BulletSceneCollisionSaver() {}

  void destroy() { delete this; }

  void addMesh(const Mesh &m, MaterialDataList *mat_list, int physmat_id, bool pmid_force, StaticGeometryMesh *sgm)
  {
    static PhysMesh pm;
    pm.buildFromMesh(m, mat_list, physmat_id, pmid_force, sgm);

    if (cook::gridStep < 1.0)
    {
      for (int i = 0; i < actors.size(); i++)
      {
        BBox3 b = actors[i]->box;
        pm.calcBbox(b);
        Point3 sz = b.width();
        if (sz.x < 20 && sz.y < 5 && sz.z < 20)
        {
          actors[i]->attach(pm);
          return;
        }
      }

      SingleActor *a = new (tmpmem) SingleActor;
      actors.push_back(a);
      a->attach(pm);
      return;
    }

    static Tab<SingleActor *> amap(tmpmem);
    static Tab<int> facesel(tmpmem);

    int fc = pm.face.size();
    amap.resize(fc);
    facesel.reserve(fc);

    float inv_step = 1.0 / 3.0 / cook::gridStep;
    for (int i = 0; i < fc; i++)
    {
      float x = pm.vert[pm.face[i].v[0]].x + pm.vert[pm.face[i].v[1]].x + pm.vert[pm.face[i].v[2]].x;
      float z = pm.vert[pm.face[i].v[0]].z + pm.vert[pm.face[i].v[1]].z + pm.vert[pm.face[i].v[2]].z;
      amap[i] = tiles.create_leaf((int)floor(x * inv_step), (int)floor(z * inv_step));
    }

    // debug("addMesh: faces=%d verts=%d step=%.3f", pm.face.size(), pm.vert.size(), cook::gridStep);
    for (;;) // infinite cycle with explicit break
    {
      SingleActor *a = NULL;
      int i;

      facesel.clear();
      for (i = 0; i < amap.size(); i++)
        if (amap[i])
        {
          a = amap[i];
          break;
        }
      if (!a)
        break;

      for (; i < fc; i++)
        if (amap[i] == a)
        {
          facesel.push_back(i);
          amap[i] = NULL;
        }
      // debug("attach to %p: %d faces", a, facesel.size());
      a->attachMeshSel(pm, facesel);
    }
    facesel.clear();
  }
  void addBox(const TMatrix &box_tm, const char *physmat)
  {
    int matId = PhysMesh::sceneMatNames.addNameId(physmat ? physmat : "::default");

    TMatrix tm;
    tm.setcol(0, box_tm.getcol(0));
    tm.setcol(1, box_tm.getcol(2));
    tm.setcol(2, -box_tm.getcol(1));
    tm.setcol(3, box_tm.getcol(3));
    if (tm.det() < 0)
      tm.setcol(2, box_tm.getcol(1));
    getCollPrim(matId).box.push_back(tm);

    // DAEDITOR3.conNote("add box: " FMT_TM "", TMD(tm));
  }
  void addCapsule(const Capsule &c, const char *physmat)
  {
    int matId = PhysMesh::sceneMatNames.addNameId(physmat ? physmat : "::default");

    Point3 cwdir = normalize(c.b - c.a);

    TMatrix tm;
    tm.setcol(1, c.b - c.a);
    tm.setcol(3, (c.a + c.b) * 0.5);

    tm.setcol(2, Point3(0, 1, 0) % cwdir);
    float zlen = length(tm.getcol(2));
    if (zlen > 1.192092896e-06F)
      tm.setcol(2, tm.getcol(2) / zlen);
    else
      tm.setcol(2, normalize(Point3(0, 0, 1) % cwdir));
    tm.setcol(0, normalize(cwdir % tm.getcol(2)) * c.r);

    getCollPrim(matId).cap.push_back(tm);

    /*DAEDITOR3.conNote("add cap: (" FMT_P3 ", " FMT_P3 ", l=%.3f, r=%.3f) " FMT_TM "",
        P3D(c.a), P3D(c.b), (c.b-c.a).length(), c.r, TMD(tm));*/
  }
  void addSphere(const Point3 &c, float r, const char *physmat)
  {
    int matId = PhysMesh::sceneMatNames.addNameId(physmat ? physmat : "::default");

    getCollPrim(matId).sph.push_back(Point4(c.x, c.y, c.z, r));

    // DAEDITOR3.conNote("add sph: " FMT_P3 " r=%.3f", P3D(c), r);
  }

  void save(mkbindump::BinDumpSaveCB &cwr)
  {
    bool no_grid_actors = actors.size();
    if (!no_grid_actors)
    {
      class CB : public HierGrid2LeafCB<SingleActor>
      {
        Tab<SingleActor *> &actors;

      public:
        CB(Tab<SingleActor *> &a) : actors(a) {}
        void leaf_callback(SingleActor *a, int uo, int vo) override
        {
          a->calcBbox(a->box);
          actors.push_back(a);
        }
      } cb(actors);

      tiles.enum_all_leaves(cb);
    }

    int orig_acnt = actors.size(), runs = 0;
    for (;;) // infinite cycle with explicit break
    {
      bool improved = false;

      for (int i = actors.size() - 1; i >= 0; i--)
      {
        BBox3 b = actors[i]->box;
        int afc = actors[i]->face.size();
        float xz_area = (b[1].x - b[0].x) * (b[1].z - b[0].z);
        if (xz_area <= 0)
          continue;

        b[0].y -= 1.0;
        b[1].y += 1.0;

        int join_1 = -1, join_2 = -1;
        float join_1_opt = cook::minSmallOverlap, join_2_opt = cook::minMutualOverlap;

        for (int j = actors.size() - 1; j >= 0; j--)
        {
          if (i == j)
            continue;
          BBox3 cb = b.getIntersection(actors[j]->box);
          if (cb.isempty())
            continue;

          float j_xz_area = actors[j]->box.width().x * actors[j]->box.width().z;
          if (j_xz_area <= 0)
            continue;

          float c_xz_area = (cb[1].x - cb[0].x) * (cb[1].z - cb[0].z);

          if (join_2 == -1 && afc <= cook::minFaceCnt && c_xz_area > join_1_opt * xz_area)
          {
            join_1_opt = c_xz_area / xz_area;
            join_1 = j;
          }
          if (c_xz_area > join_2_opt * xz_area && c_xz_area > join_2_opt * j_xz_area)
          {
            join_2_opt = (xz_area >= j_xz_area) ? c_xz_area / xz_area : c_xz_area / j_xz_area;
            join_2 = j;
          }
        }

        if (join_1 == -1 && join_2 == -1)
          continue;


        if (join_1 != -1)
        {
          // DAEDITOR3.conNote("attach small %d (%d faces) and %d (%d faces), ratio=%.3f",
          //   i, afc, join_1, actors[join_1]->face.size(), join_1_opt);
        }
        else
        {
          join_1 = join_2;
          // DAEDITOR3.conNote("attach overlapped %d (%d faces) and %d (%d faces), ratio=%.3f",
          //   i, afc, join_2, actors[join_2]->face.size(), join_2_opt);
        }

        actors[join_1]->attach(*actors[i], true);
        if (no_grid_actors)
          erase_ptr_items(actors, i, 1);
        else
          erase_items(actors, i, 1);
        improved = true;
      }
      if (!improved)
        break;
      runs++;
      // DAEDITOR3.conNote("-- next run");
    }
    if (runs)
      DAEDITOR3.conNote("reduced %d actors to %d in %d run(s)", orig_acnt, actors.size(), runs);

    // save version
    cwr.writeInt32e(BULLET_SCENE_COLLISION_VERSION);

    // save scene materials IDs
    const FastNameMapEx &mats = PhysMesh::sceneMatNames;
    cwr.beginBlock();
    cwr.setOrigin();
    cwr.writeRef(cwr.TAB_SZ, mats.nameCount());
    cwr.writeZeroes(cwr.PTR_SZ * mats.nameCount());
    for (int i = 0; i < mats.nameCount(); i++)
    {
      cwr.writeInt32eAt(cwr.tell(), cwr.TAB_SZ + cwr.PTR_SZ * i);
      cwr.writeRaw(mats.getName(i), (int)strlen(mats.getName(i)) + 1);
    }
    cwr.align4();
    cwr.popOrigin();
    cwr.endBlock();

    // store primitives counters
    int box_cnt = 0, sph_cnt = 0, cap_cnt = 0, conv_cnt = 0, cyl_cnt = 0;
    for (int i = 0; i < perMatColl.size(); i++)
    {
      box_cnt += perMatColl[i].box.size();
      sph_cnt += perMatColl[i].sph.size();
      cap_cnt += perMatColl[i].cap.size();
    }

    cwr.writeInt32e(actors.size()); // MESH
    cwr.writeInt32e(conv_cnt);      // CONV
    cwr.writeInt32e(cyl_cnt);       // CYL
    cwr.writeInt32e(box_cnt);       // BOX
    cwr.writeInt32e(sph_cnt);       // SPH
    cwr.writeInt32e(cap_cnt);       // CAP

    // write triangle meshes
    cwr.writeFourCC(_MAKE4C('MESH'));

    Point3 max_box(0, 0, 0), min_box(0, 0, 0), avg_box(0, 0, 0);
    float max_sq = 0, min_sq = MAX_REAL;
    int min_vert = 1000000, max_vert = 0;
    float avg_vert = 0;

    for (int ai = 0; ai < actors.size(); ai++)
    {
      SingleActor &a = *actors[ai];
      BBox3 &b = a.box;
      // a.calcBbox(b);
      phys_actors_bbox.push_back(b);

      float sq = b.width().x * b.width().z;
      avg_box += b.width() / actors.size();
      if (sq < min_sq)
      {
        min_sq = sq;
        min_box = b.width();
      }
      if (sq > max_sq)
      {
        max_sq = sq;
        max_box = b.width();
      }
      if (a.vert.size() < min_vert)
        min_vert = a.vert.size();
      if (a.vert.size() > max_vert)
        max_vert = a.vert.size();
      avg_vert += float(a.vert.size()) / actors.size();

      a.buildAndSaveCollisionMesh(cwr);
    }
    DAEDITOR3.conNote("Total %d tri-meshes", actors.size());
    DAEDITOR3.conNote("bbox: min=" FMT_P3 ", max=" FMT_P3 ", avg=" FMT_P3 "", P3D(min_box), P3D(max_box), P3D(avg_box));
    DAEDITOR3.conNote("vert: min=%d, max=%d, avg=%.3f", min_vert, max_vert, avg_vert);

    // write convex meshes
    cwr.writeFourCC(_MAKE4C('CONV'));

    // write convex meshes
    cwr.writeFourCC(_MAKE4C('CYL'));

    // write boxes
    cwr.writeFourCC(_MAKE4C('BOX'));
    for (int i = 0; i < perMatColl.size(); i++)
      if (perMatColl[i].box.size())
      {
        DAEDITOR3.conNote("  %s: %d boxes", PhysMesh::sceneMatNames.getName(i), perMatColl[i].box.size());
        cwr.writeInt32e(i);
        cwr.writeInt32e(perMatColl[i].box.size());
        cwr.writeTabData32ex(perMatColl[i].box);
        append_items(phys_box_actors, perMatColl[i].box.size(), perMatColl[i].box.data());
      }
    DAEDITOR3.conNote("Total %d boxes", box_cnt);

    // write spheres
    cwr.writeFourCC(_MAKE4C('SPH'));
    for (int i = 0; i < perMatColl.size(); i++)
      if (perMatColl[i].sph.size())
      {
        DAEDITOR3.conNote("  %s: %d spheres", PhysMesh::sceneMatNames.getName(i), perMatColl[i].sph.size());
        cwr.writeInt32e(i);
        cwr.writeInt32e(perMatColl[i].sph.size());
        cwr.writeTabData32ex(perMatColl[i].sph);
        append_items(phys_sph_actors, perMatColl[i].sph.size(), perMatColl[i].sph.data());
      }
    DAEDITOR3.conNote("Total %d spheres", sph_cnt);

    // write capsules
    cwr.writeFourCC(_MAKE4C('CAP'));
    for (int i = 0; i < perMatColl.size(); i++)
      if (perMatColl[i].cap.size())
      {
        DAEDITOR3.conNote("  %s: %d capsules", PhysMesh::sceneMatNames.getName(i), perMatColl[i].cap.size());
        cwr.writeInt32e(i);
        cwr.writeInt32e(perMatColl[i].cap.size());
        cwr.writeTabData32ex(perMatColl[i].cap);
        append_items(phys_cap_actors, perMatColl[i].cap.size(), perMatColl[i].cap.data());
      }
    DAEDITOR3.conNote("Total %d capsules", cap_cnt);

    if (no_grid_actors)
      clear_all_ptr_items(actors);
    else
      tiles.clear();
  }

  CollPrim &getCollPrim(int matid)
  {
    if (matid >= perMatColl.size())
      perMatColl.resize(matid + 1);
    return perMatColl[matid];
  }

private:
  Tab<SingleActor *> actors;
  Tab<CollPrim> perMatColl;
  TMatrix physInvTm;
};

void BulletSceneCollisionSaver::SingleActor::buildAndSaveCollisionMesh(mkbindump::BinDumpSaveCB &cwr)
{
  Face ref;

  // DAEDITOR3.conNote("PhysMaterials: %d, f=%d v=%d",
  //   PhysMesh::sceneMatNames.nameCount(), face.size(), vert.size());

  // SPLIT
  Tab<Point3> v(tmpmem);
  Tab<Face> f(tmpmem);
  SmallTab<int, TmpmemAlloc> vremap;
  clear_and_resize(vremap, vert.size());

  for (int pmid = 0; pmid < PhysMesh::sceneMatNames.nameCount(); pmid++)
  {
    v.clear();
    f.clear();
    bool ready = false;

    for (int fi = 0; fi < face.size(); fi++)
    {
      if (face[fi].matId == pmid)
      {
        if (!ready)
        {
          mem_set_ff(vremap);
          ready = true;
        }

        Face f1;
        for (int i = 0; i < 3; i++)
        {
          f1.v[i] = face[fi].v[i];
          int idx = vremap[f1.v[i]];
          if (idx == -1)
          {
            vremap[f1.v[i]] = idx = v.size();
            v.push_back(vert[f1.v[i]]);
          }
          f1.v[i] = idx;
        }
        f1.matId = 0; // we dont use it!
        f.push_back(f1);
      }
    }

    PhysMesh::vertPerMat[pmid] += v.size();
    PhysMesh::facePerMat[pmid] += f.size();

    cwr.writeInt32e(f.size());
    if (f.size())
    {
      PhysMesh::objPerMat[pmid]++;
      // DAEDITOR3.conNote("- Exporting material %s", PhysMesh::sceneMatNames.getName(pmid));

      // setup cooking params
      btDefaultSerializer ser(8 << 20);
      ser.startSerialization();

      // Build the triangle mesh.
      {
        btTriangleIndexVertexArray meshInterface;
        btIndexedMesh part;

        part.m_vertexBase = (const unsigned char *)v.data();
        part.m_vertexStride = sizeof(Point3);
        part.m_numVertices = v.size();
        part.m_triangleIndexBase = (const unsigned char *)f.data();
        part.m_triangleIndexStride = sizeof(Face);
        part.m_numTriangles = f.size();
        part.m_indexType = PHY_INTEGER;
        part.m_vertexType = PHY_FLOAT;
        meshInterface.addIndexedMesh(part, PHY_INTEGER);

        btBvhTriangleMeshShape trimeshShape(&meshInterface, true, true);
        trimeshShape.serializeSingleShape(&ser);
        ser.finishSerialization();
      }

      cwr.writeInt32e(ser.getCurrentBufferSize());
      cwr.writeRaw(ser.getBufferPointer(), ser.getCurrentBufferSize());
      // DAEDITOR3.conNote("PhysMaterial %d (%s): %d faces, %d vertices, sz=%p",
      //   pmid, PhysMesh::sceneMatNames.getName(pmid),
      //   f.size(), v.size(), ser.getCurrentBufferSize());
    }
  }
}


class BulletCollisionBuilder : public ICollisionDumpBuilder
{
public:
  BulletCollisionBuilder(bool need_bt_data) : collisionSaver(NULL)
  {
    supportMask = SUPPORT_BOX | SUPPORT_CAPSULE | SUPPORT_SPHERE;
    init_bullet_physics_engine();

    needBtData = need_bt_data;
    raytracer_dump_builder = create_dagor_raytracer_dump_builder();
  }

  ~BulletCollisionBuilder()
  {
    destroy_it(collisionSaver);
    destroy_it(raytracer_dump_builder);
    close_bullet_physics_engine();
    PhysMesh::clearTemp();
  }

  void destroy() override { delete this; }

  void start(const CollisionBuildSettings &stg) override
  {
    cook::gridStep = stg.gridStep;
    cook::minMutualOverlap = stg.minMutualOverlap;
    cook::minSmallOverlap = stg.minSmallOverlap;
    cook::minFaceCnt = stg.minFaceCnt;

    raytracer_dump_builder->start(stg);
    if (collisionSaver)
      collisionSaver->destroy();
    collisionSaver = new BulletSceneCollisionSaver;
    PhysMesh::sceneMatNames.reset();
    t0 = ref_time_ticks();

    phys_actors_bbox.clear();
    phys_box_actors.clear();
    phys_sph_actors.clear();
    phys_cap_actors.clear();
  }

  void addCapsule(Capsule &c, int obj_id, int obj_flags, const char *physmat) override
  {
    if (needBtData)
      collisionSaver->addCapsule(c, physmat);
  }
  void addSphere(const Point3 &c, float rad, int obj_id, int obj_flags, const char *physmat) override
  {
    if (needBtData)
      collisionSaver->addSphere(c, rad, physmat);
  }
  void addBox(const TMatrix &box_tm, int obj_id, int obj_flags, const char *physmat) override
  {
    if (needBtData)
      collisionSaver->addBox(box_tm, physmat);
    raytracer_dump_builder->addBox(box_tm, obj_id, obj_flags, physmat);
  }
  void addConvexHull(Mesh &m, MaterialDataList *mat, TMatrix &wtm, int obj_id, int obj_flags, int physmat_id, bool pmid_force,
    StaticGeometryMesh *sgm = NULL) override
  {
    G_ASSERT(0 && "addConvexHull not implemented");
  }

  void addMesh(Mesh &m, MaterialDataList *mat, const TMatrix &wtm, int obj_id, int obj_flags, int physmat_id, bool pmid_force,
    StaticGeometryMesh *sgm) override
  {
    for (int i = 0; i < m.vert.size(); ++i)
      m.vert[i] = wtm * m.vert[i];

    raytracer_dump_builder->addMesh(m, mat, TMatrix::IDENT, obj_id, obj_flags, physmat_id, pmid_force, sgm);
    if (needBtData)
      collisionSaver->addMesh(m, mat, physmat_id, pmid_force, sgm);
  }

  bool finishAndWrite(const char *temp_fname, IGenSave &cwr, unsigned target_code) override
  {
    if (!dagor_target_code_valid(target_code))
    {
      DEBUG_CTX("unsupported target: %c%c%c%c", _DUMP4C(target_code));
      return false;
    }

    DAEDITOR3.conNote("gathered collision in %d ms", get_time_usec(t0) / 1000);
    t0 = ref_time_ticks();

    PhysMesh::facePerMat.resize(PhysMesh::sceneMatNames.nameCount());
    mem_set_0(PhysMesh::facePerMat);
    PhysMesh::vertPerMat.resize(PhysMesh::sceneMatNames.nameCount());
    mem_set_0(PhysMesh::vertPerMat);
    PhysMesh::objPerMat.resize(PhysMesh::sceneMatNames.nameCount());
    mem_set_0(PhysMesh::objPerMat);

    {
      FullFileSaveCB collSaver(temp_fname);
      if (!collSaver.fileHandle)
      {
        logerr("Can't save file to %s", temp_fname);
        return false;
      }
      int write_be = dagor_target_code_be(target_code);
      mkbindump::BinDumpSaveCB memCwr(8 << 20, target_code, write_be);
      collisionSaver->save(memCwr);

      DAEDITOR3.conNote("built collision in %d ms", get_time_usec(t0) / 1000);

      if (memCwr.getSize() < (512 << 10) || target_code != _MAKE4C('PC'))
      {
        DAEDITOR3.conNote("Collision size: %dK, stored unpacked", memCwr.getSize() >> 10);
        memCwr.copyDataTo(collSaver);
      }
      else
      {
        t0 = ref_time_ticks();
        // add 'pack' flag to version label
        collSaver.writeInt(mkbindump::le2be32_cond(BULLET_SCENE_COLLISION_VERSION | 0x80000000, memCwr.WRITE_BE));

        ZlibSaveCB zlib_cwr(collSaver, ZlibSaveCB::CL_BestSpeed + 5);
        memCwr.copyDataTo(zlib_cwr);
        zlib_cwr.finish();
        DAEDITOR3.conNote("packed built collision in %d ms", get_time_usec(t0) / 1000);
        DAEDITOR3.conNote("Collision size: %dK, packed: %dK", memCwr.getSize() >> 10, collSaver.tell() >> 10);
      }
    }

    copy_file_to_stream(temp_fname, cwr);
    for (int pmid = 0; pmid < PhysMesh::sceneMatNames.nameCount(); pmid++)
      DAEDITOR3.conNote("- physmat %s: %d faces, %d verts, %d actors", PhysMesh::sceneMatNames.getName(pmid),
        PhysMesh::facePerMat[pmid], PhysMesh::vertPerMat[pmid], PhysMesh::objPerMat[pmid]);

    return true;
  }

  ICollisionDumpBuilder *getSeparateRayTracer() override { return raytracer_dump_builder; }

private:
  BulletSceneCollisionSaver *collisionSaver;
  ICollisionDumpBuilder *raytracer_dump_builder;
  int64_t t0;
  bool needBtData = true;
};

ICollisionDumpBuilder *create_bullet_collision_dump_builder(bool need_bt_data)
{
  return new (tmpmem) BulletCollisionBuilder(need_bt_data);
}
