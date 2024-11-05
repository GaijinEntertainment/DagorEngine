// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_smallTab.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_localConv.h>

#include <libTools/util/strUtil.h>
#include <libTools/util/fileUtils.h>

#include <debug/dag_debug.h>
#include <memory/dag_mem.h>
#include <3d/dag_materialData.h>
#include <3d/dag_texMgr.h>
#include <math/dag_mesh.h>
#include <libTools/util/strUtil.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/dagFileRW/dagExporter.h>
#include <perfMon/dag_cpuFreq.h>
#if _TARGET_PC_WIN
#include <startup/dag_mainCon.inc.cpp>
#endif

#include <stdio.h>
#include "csg.h"

#define PARALLELIZED_UNION  true
#define MIN_ARGUMENTS_COUNT 3

static void initDagor(const char *base_path);
Tab<Node *> node_list(tmpmem_ptr());

static void enum_nodes(Node *root, Tab<Node *> &node_list)
{
  if (root->obj && root->obj->isSubOf(OCID_MESHHOLDER))
    node_list.push_back(root);
  for (int i = 0; i < root->child.size(); ++i)
    enum_nodes(root->child[i], node_list);
}

static Node *attach_scene(AScene &scn)
{
  scn.root->invalidate_wtm();
  scn.root->calc_wtm();
  Tab<Node *> node_list;
  enum_nodes(scn.root, node_list);
  if (!node_list.size())
    return NULL;
  Mesh &mesh1 = *((MeshHolderObj *)node_list[0]->obj)->mesh;
  mesh1.transform(node_list[0]->wtm);
  for (int i = 1; i < node_list.size(); ++i)
  {
    ((MeshHolderObj *)node_list[i]->obj)->mesh->transform(node_list[i]->wtm);
    mesh1.attach(*((MeshHolderObj *)node_list[i]->obj)->mesh);
  }
  return node_list[0];
}


// could be even parallalized
static void *recursive_union(BaseCSG *csg, void **at, int cnt)
{
  if (!cnt)
    return 0;
  if (cnt == 1)
    return *at;
  int lcnt = cnt / 2;
  void *left = recursive_union(csg, at, lcnt);
  void *right = recursive_union(csg, at + lcnt, cnt - lcnt);
  void *opres = csg->op(BaseCSG::UNION, left, NULL, right, NULL);
  // csg->delete_poly(left);  //no memory clearing, faster!
  // csg->delete_poly(right);  //no memory clearing, faster!
  return opres;
}

static void attache_nodes(Node *&root, bool remove_internal_faces)
{
  if (!root)
    return;
  enum_nodes(root, node_list);
  root->invalidate_wtm();
  root->calc_wtm();
  if (!node_list.size())
    return;
  BaseCSG *csg = make_new_csg(1);
  int64_t reft;
  printf("converting\n");
  reft = ::ref_time_ticks();
  SmallTab<void *, TmpmemAlloc> poly_list;
  clear_and_resize(poly_list, node_list.size());
  SmallTab<int, TmpmemAlloc> poly_list_id;
  clear_and_resize(poly_list_id, node_list.size());
  int polycnt = 0;
  BBox3 box;
  for (int i = 0; i < node_list.size(); ++i)
  {
    poly_list[polycnt] = csg->create_poly(&node_list[i]->wtm, *((MeshHolderObj *)node_list[i]->obj)->mesh, i, true, box);
    poly_list_id[polycnt] = i;
    if (!poly_list[polycnt])
      printf("node %s not closed\n", node_list[i]->name);
    else
      polycnt++;
  }
  printf("converted in %g mseconds\n", ::get_time_usec(reft) / 1000.0);
  if (!polycnt)
    return;

  reft = ::ref_time_ticks();
  printf("union");
  void *result;
  if (PARALLELIZED_UNION)
  {
    result = recursive_union(csg, poly_list.data(), polycnt);
  }
  else
  {
    result = poly_list[0];
    for (int i = 1; i < polycnt; ++i)
    {
      void *opres = csg->op(BaseCSG::UNION, result, NULL, poly_list[i], NULL);
      // if (result != poly_list[0])
      //   csg->delete_poly(result);
      // csg->delete_poly(poly_list[i]);//no memory clearing, faster!
      result = opres;
      printf(".");
    }
  }
  printf("\nunion in %g mseconds\n", ::get_time_usec(reft) / 1000.0);
  reft = ::ref_time_ticks();

  ///*
  if (remove_internal_faces)
  {
    int totalFaces = 0, totalNewfaces = 0;
    for (int i = 0; i < polycnt; ++i)
    {
      int node_id = poly_list_id[i];
      Mesh *mesh = ((MeshHolderObj *)node_list[node_id]->obj)->mesh;
      totalFaces += mesh->getFace().size();
      csg->removeUnusedFaces(*mesh, result, node_id);
      mesh->optimize_tverts(-1);
      mesh->kill_unused_verts(-1);
      totalNewfaces += mesh->getFace().size();
    }
    printf("%d out of %d faces removed\n", totalFaces - totalNewfaces, totalFaces);
  }
  else
  {
    Mesh *resultMesh = new Mesh;
    csg->convert(result, *resultMesh);
    // result->canonicalize();
    ((MeshHolderObj *)node_list[0]->obj)->mesh = resultMesh;
    node_list[0]->name = "UNION";
    node_list[0]->tm = TMatrix::IDENT;
    node_list[0]->child.clear();
    root = node_list[0];
  }
  root->invalidate_wtm();
  root->calc_wtm();
  printf("write back %g mseconds\n", ::get_time_usec(reft) / 1000.0);
}

void show_usage()
{
  printf("Usage: dagCSG <DAG file in> <DAG file out> [options]\n");
  printf("where\n");
  printf("DAG file\tfile to union\n");
  printf("options\t\tadditional settings (-r - only remove internal faces)\n");
  printf("  -h\thide copyright message\n");
  printf("  -r\tonly remove internal faces\n");
  printf("OR Usage: dagCSG <DAG_A> <op> <DAG_B> <DAG file out>\n");
  printf("where\n");
  printf("op is one of:\n");
  printf("+|UNION        \n");
  printf("-|A_MINUS_B    \n");
  printf("INTERSECTION   \n");
  printf("DIFFERENCE     \n");
  printf("SLICE_IN    \n");
  printf("SLICE_OUT    \n");
}

static BaseCSG::OpType parse_type(const char *tp)
{
  if (stricmp(tp, "+") == 0 || stricmp(tp, "UNION") == 0)
    return BaseCSG::UNION;
  else if (stricmp(tp, "-") == 0 || stricmp(tp, "A_MINUS_B") == 0)
    return BaseCSG::A_MINUS_B;
  else if (stricmp(tp, "INTERSECTION") == 0)
    return BaseCSG::INTERSECTION;
  else if (stricmp(tp, "DIFFERENCE") == 0)
    return BaseCSG::SYMMETRIC_DIFFERENCE;
  else if (stricmp(tp, "SLICE_IN") == 0)
    return BaseCSG::SLICE_IN;
  else if (stricmp(tp, "SLICE_OUT") == 0)
    return BaseCSG::SLICE_OUT;
  return BaseCSG::UNKNOWN;
}

void DagorWinMainInit(bool /*debugmode*/) {}
// int __cdecl main(int argc, char** argv)
int DagorWinMain(bool /*debugmode*/)
{
  static char *const *argv = dgs_argv;
  static const int argc = dgs_argc;
  if (argc < MIN_ARGUMENTS_COUNT)
  {
    show_usage();
    return -1;
  }

  initDagor(argv[0]);

  String dagPath, dagPathOUT;
  bool showCopyright = true;
  int hasIN = 0;
  bool remove_internal_faces = false;

  for (int i = 1; i < argc; ++i)
  {
    if (argv[i][0] == '-')
    {
      switch (argv[i][1])
      {
        case 'h':
        case 'H': showCopyright = false; break;

        case 'r':
        case 'R': remove_internal_faces = true; break;

        default: printf("unknown option %s\n", argv[i]);
      }
    }
    else
    {
      if (hasIN)
        dagPathOUT = argv[i];
      else
        dagPath = argv[i];
      hasIN++;
    }
  }
  if (showCopyright)
  {
    printf("Dag CSG tool\n");
    printf("Copyright (c) Gaijin Games KFT, 2023\n");
  }
  BaseCSG::OpType op_tp = parse_type(argv[2]);
  if (argc > 4 && op_tp != BaseCSG::UNKNOWN && strstr(argv[3], ".dag") && strstr(argv[4], ".dag"))
  {
    AScene scn1, scn2;
    if (!::load_ascene(argv[1], scn1, LASF_MATNAMES | LASF_NULLMATS, false))
    {
      printf("\nFATAL: error loading file \"%s\"", argv[1]);
      return 1;
    }
    if (!::load_ascene(argv[3], scn2, LASF_MATNAMES | LASF_NULLMATS, false))
    {
      printf("\nFATAL: error loading file \"%s\"", argv[3]);
      return 1;
    }
    printf("Perform: <%s> %s <%s>\n", argv[1], argv[2], argv[3]);
    int64_t reft;
    printf("converting\n");
    reft = ::ref_time_ticks();
    Node *A, *B;
    if (!(A = attach_scene(scn1)))
      printf("\nFATAL: no meshes in file \"%s\"", argv[1]);
    if (!(B = attach_scene(scn2)))
      printf("\nFATAL: no meshes in file \"%s\"", argv[3]);
    BBox3 box;
    int tcUsed = 0;
    for (; tcUsed < NUMMESHTCH; ++tcUsed)
      if (!((MeshHolderObj *)A->obj)->mesh->tface[tcUsed].size() && !((MeshHolderObj *)B->obj)->mesh->tface[tcUsed].size())
        break;
    BaseCSG *csg = make_new_csg(tcUsed);
    void *poly_A =
      csg->create_poly(NULL, *((MeshHolderObj *)A->obj)->mesh, 0, !(op_tp == BaseCSG::SLICE_IN || op_tp == BaseCSG::SLICE_OUT), box);
    if (!poly_A)
    {
      printf("Can not convert %s to CSG poly", argv[1]);
      return 1;
    }
    void *poly_B = csg->create_poly(NULL, *((MeshHolderObj *)B->obj)->mesh, 0, true, box);
    if (!poly_B)
    {
      printf("Can not convert %s to CSG poly", argv[3]);
      return 1;
    }
    G_ASSERT(poly_A);

    printf("converted in %g mseconds\n", ::get_time_usec(reft) / 1000.0);
    reft = ::ref_time_ticks();
    Mesh *resultMesh = new Mesh;
    if (op_tp == BaseCSG::SLICE_IN || op_tp == BaseCSG::SLICE_OUT)
    {
      void *in, *on_in;
      void *out, *on_out;
      bool ret = csg->slice_and_classify(poly_A, NULL, poly_B, NULL, in, on_in, out, on_out);
      if (!ret)
      {
        printf("error\n");
        return 1;
      }
      printf("performed %d in %g mseconds\n", ret, ::get_time_usec(reft) / 1000.0);
      reft = ::ref_time_ticks();
      Mesh onMesh;
      if (op_tp == BaseCSG::SLICE_IN)
      {
        csg->convert(in, *resultMesh);
        if (on_in)
        {
          csg->convert(on_in, onMesh);
          resultMesh->attach(onMesh);
        }
      }
      else
      {
        csg->convert(out, *resultMesh);
        if (on_out)
        {
          csg->convert(on_out, onMesh);
          resultMesh->attach(onMesh);
        }
      }
      printf("saved in %g mseconds\n", ::get_time_usec(reft) / 1000.0);
    }
    else
    {
      void *result = csg->op(op_tp, poly_A, NULL, poly_B, NULL);
      printf("performed in %g mseconds\n", ::get_time_usec(reft) / 1000.0);
      reft = ::ref_time_ticks();
      csg->convert(result, *resultMesh);
      printf("saved in %g mseconds\n", ::get_time_usec(reft) / 1000.0);
    }
    // result->canonicalize();
    ((MeshHolderObj *)node_list[0]->obj)->mesh = resultMesh;
    scn1.root = node_list[0];
    scn1.root->name = "UNION";
    scn1.root->tm = TMatrix::IDENT;
    scn1.root->child.clear();
    printf(" exporting...\n");
    DagExporter::exportGeometry(argv[4], scn1.root);
    printf(" OK\n");
  }
  else
  {
    if (hasIN != 2)
    {
      printf("no input/output specified\n");
      show_usage();
      return 1;
    }
    AScene scn;
    if (!::load_ascene(dagPath, scn, LASF_MATNAMES | LASF_NULLMATS, false))
    {
      printf("\nFATAL: error loading file \"%s\"", dagPath.data());
      return 1;
    }
    printf(" attaching...\n");
    attache_nodes(scn.root, remove_internal_faces);
    printf(" OK\n");
    printf(" exporting...\n");
    DagExporter::exportGeometry(dagPathOUT, scn.root);
    printf(" OK\n");
  }

  return 0;
}


void initDagor(const char *base_path)
{
  ::dd_init_local_conv();

  String dir(base_path);
  ::location_from_path(dir);
  ::append_slash(dir);

  ::dd_add_base_path("");
  ::dd_add_base_path(dir);
  measure_cpu_freq();
}
