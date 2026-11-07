// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_localConv.h>

#include <libTools/util/strUtil.h>
#include <libTools/util/fileUtils.h>
#include <libTools/meshCSG/meshCSG.h>

#include <debug/dag_debug.h>
#include <3d/dag_materialData.h>
#include <math/dag_mesh.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/dagFileRW/dagExporter.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_bitArray.h>
#if _TARGET_PC_WIN
#include <startup/dag_mainCon.inc.cpp>
#endif

#include <stdio.h>

#define MIN_ARGUMENTS_COUNT 3

static void initDagor(const char *base_path);

static void enum_nodes(Node *root, Tab<Node *> &node_list)
{
  if (root->obj && root->obj->isSubOf(OCID_MESHHOLDER) && ((MeshHolderObj *)root->obj)->mesh)
    node_list.push_back(root);
  for (int i = 0; i < root->child.size(); ++i)
    enum_nodes(root->child[i], node_list);
}

static Mesh &node_mesh(Node *n) { return *((MeshHolderObj *)n->obj)->mesh; }

static void collect_nodes(Node *root, Tab<Node *> &nodes, Tab<mesh_csg::Input> &inputs)
{
  root->invalidate_wtm();
  root->calc_wtm();
  const uint32_t first = nodes.size();
  enum_nodes(root, nodes);
  for (Node *n : make_span_const(nodes).subspan(first))
    inputs.push_back(mesh_csg::Input{&node_mesh(n), n->wtm, n->name.str()});
}

// kept: a refused node stays in the scene uncut, as -r leaves it, else the result leaves it out
static bool report(const mesh_csg::Result &r, dag::ConstSpan<Node *> nodes, bool ok, bool kept)
{
  for (int i = 0; i < r.refused.size(); ++i)
    if (!r.refused[i].empty())
      printf("node %s %s: %s\n", nodes[i]->name.str(), kept ? "not cut" : "skipped", r.refused[i].c_str());
  for (const eastl::string &w : r.warnings)
    printf("warning: %s\n", w.c_str());
  if (!ok)
    printf("CSG failed: %s\n", r.error.c_str());
  return ok;
}

// node-local mat ids, modulo as in Max; a node without materials, or a face of no input, takes an empty one, not another
// node's; a refused node has no face in the result, so no material either; false: a merged id does not fit Face::mat
static bool merge_materials(dag::ConstSpan<Node *> nodes, const mesh_csg::Result &r, Mesh &mesh, Node &out)
{
  MaterialDataList *list = new MaterialDataList;
  Tab<int> base, count;
  bool bare = false;
  for (int k = 0; k < nodes.size(); ++k)
  {
    const Node *n = nodes[k];
    const bool refused = !r.refused[k].empty();
    base.push_back(list->list.size());
    count.push_back(n->mat && !refused ? n->mat->list.size() : 0);
    bare |= !count.back() && !refused;
    // the exporter skips a null slot, which would shift every later slot under its faces' ids
    for (int i = 0; i < count.back(); ++i)
    {
      MaterialData *m = n->mat->getSubMat(i);
      list->list.push_back(m ? m : new MaterialData);
    }
  }
  for (const uint32_t in : r.faceInput)
    bare |= in == mesh_csg::Result::NO_INPUT;
  const int empty = list->list.size();
  if (bare)
    list->list.push_back(new MaterialData);
  for (int f = 0; f < mesh.face.size(); ++f)
  {
    const uint32_t in = r.faceInput[f];
    mesh.face[f].mat = in < nodes.size() && count[in] ? base[in] + mesh.face[f].mat % count[in] : empty;
  }
  delete out.mat;
  out.mat = list;
  if (list->subMatCount() <= 0x10000)
    return true;
  printf("the result needs %d materials, more than the 65536 a face can index: no .dag written\n", list->subMatCount());
  return false;
}

// node becomes the scene's only node, the result's: every other node and its own mesh are deleted
static void set_result(Node *&root, Node *node, Mesh *mesh)
{
  if (node->parent)
    erase_item_by_value(node->parent->child, node);
  if (root != node)
    delete root;
  root = node;
  for (Node *child : node->child)
    delete child;
  node->child.clear();
  ((MeshHolderObj *)node->obj)->setmesh(mesh);
  node->name = "UNION";
  node->tm = TMatrix::IDENT;
  node->parent = NULL;
  node->invalidate_wtm();
  node->calc_wtm();
}

static bool union_nodes(Node *&root, bool remove_internal_faces)
{
  if (!root)
    return true;
  Tab<Node *> nodes;
  Tab<mesh_csg::Input> inputs;
  collect_nodes(root, nodes, inputs);
  if (!nodes.size())
    return true;
  printf("union of %d nodes\n", nodes.size());
  int64_t reft = ::ref_time_ticks();
  mesh_csg::Result r;
  const bool ok = mesh_csg::evaluate(inputs, mesh_csg::Op::Union, {}, r);
  printf("union in %g mseconds\n", ::get_time_usec(reft) / 1000.0);
  if (!report(r, nodes, ok, remove_internal_faces))
    return false;

  if (remove_internal_faces)
  {
    int totalFaces = 0, totalNewfaces = 0;
    for (int i = 0; i < nodes.size(); ++i)
    {
      Mesh &mesh = node_mesh(nodes[i]);
      if (r.refused[i].empty())
      {
        totalFaces += mesh.face.size();
        mesh_csg::keep_surface_faces(r, i, mesh);
        mesh.optimize_tverts(-1);
        mesh.kill_unused_verts(-1);
        totalNewfaces += mesh.face.size();
      }
      // no mesh object, as a reader loads such a node: the exporter reads an empty mesh's first vertex and face
      if (!mesh.face.size() && r.refused[i].empty())
        nodes[i]->setobj(nullptr);
    }
    printf("%d out of %d faces removed\n", totalFaces - totalNewfaces, totalFaces);
    return true;
  }
  if (!r.mesh.faceCount())
  {
    printf("the union has no faces: the scene is kept\n");
    return true;
  }
  Mesh *mesh = new Mesh;
  mesh_csg::to_engine_mesh(r, *mesh);
  const bool fits = merge_materials(nodes, r, *mesh, *nodes[0]);
  set_result(root, nodes[0], mesh);
  return fits;
}

static int two_scenes(mesh_csg::Op op, bool slice, const char *path_a, const char *path_b, const char *path_out)
{
  AScene scn1, scn2;
  if (!::load_ascene(path_a, scn1, LASF_MATNAMES | LASF_NULLMATS, false))
  {
    printf("\nFATAL: error loading file \"%s\"", path_a);
    return 1;
  }
  if (!::load_ascene(path_b, scn2, LASF_MATNAMES | LASF_NULLMATS, false))
  {
    printf("\nFATAL: error loading file \"%s\"", path_b);
    return 1;
  }
  Tab<Node *> nodes;
  Tab<mesh_csg::Input> a, b;
  collect_nodes(scn1.root, nodes, a);
  collect_nodes(scn2.root, nodes, b);
  if (!a.size() || !b.size())
  {
    printf("\nFATAL: no meshes in file \"%s\"", a.size() ? path_b : path_a);
    return 1;
  }

  int64_t reft = ::ref_time_ticks();
  mesh_csg::Result r;
  const bool ok = mesh_csg::evaluate(a, op, b, r);
  printf("performed in %g mseconds\n", ::get_time_usec(reft) / 1000.0);
  if (!report(r, nodes, ok, false))
    return 1;
  for (int i = 0; i < r.refused.size(); ++i)
    if (!r.refused[i].empty())
    {
      printf("a refused node of A or B stops the run: no .dag written\n");
      return 1;
    }

  Mesh *mesh = new Mesh;
  mesh_csg::to_engine_mesh(r, *mesh);
  // a slice keeps A's faces alone
  const bool fits = merge_materials(slice ? make_span_const(nodes.data(), a.size()) : make_span_const(nodes), r, *mesh, *nodes[0]);
  if (slice)
  {
    Bitarray ofA;
    ofA.resize(mesh->face.size());
    ofA.reset();
    for (int f = 0; f < mesh->face.size(); ++f)
      if (r.faceInput[f] < a.size())
        ofA.set(f);
    mesh->removeFacesFast(ofA);
    mesh->kill_unused_verts(-1);
  }
  if (!mesh->face.size())
  {
    delete mesh;
    printf("the result is empty: no .dag written\n");
    return 1;
  }
  set_result(scn1.root, nodes[0], mesh);
  if (!fits)
    return 1;
  printf(" exporting...\n");
  DagExporter::exportGeometry(path_out, scn1.root);
  printf(" OK\n");
  return 0;
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
  printf("SLICE_IN       keeps A's faces of the intersection\n");
  printf("SLICE_OUT      keeps A's faces of A minus B\n");
  printf("each mesh node of A and B is repaired first; one that still fails is refused with its reason, and no .dag is written\n");
}

static bool parse_type(const char *tp, mesh_csg::Op &op, bool &slice)
{
  slice = stricmp(tp, "SLICE_IN") == 0 || stricmp(tp, "SLICE_OUT") == 0;
  if (stricmp(tp, "+") == 0 || stricmp(tp, "UNION") == 0)
    op = mesh_csg::Op::Union;
  else if (stricmp(tp, "-") == 0 || stricmp(tp, "A_MINUS_B") == 0 || stricmp(tp, "SLICE_OUT") == 0)
    op = mesh_csg::Op::Subtract;
  else if (stricmp(tp, "INTERSECTION") == 0 || stricmp(tp, "SLICE_IN") == 0)
    op = mesh_csg::Op::Intersect;
  else if (stricmp(tp, "DIFFERENCE") == 0)
    op = mesh_csg::Op::Xor;
  else
    return false;
  return true;
}

void DagorWinMainInit(bool /*debugmode*/) {}
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
    printf("Copyright (c) Gaijin Games KFT, 2026\n");
  }
  mesh_csg::Op op;
  bool slice;
  if (argc > 4 && parse_type(argv[2], op, slice) && strstr(argv[3], ".dag") && strstr(argv[4], ".dag"))
  {
    printf("Perform: <%s> %s <%s>\n", argv[1], argv[2], argv[3]);
    return two_scenes(op, slice, argv[1], argv[3], argv[4]);
  }

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
  printf(" union...\n");
  if (!union_nodes(scn.root, remove_internal_faces))
    return 1;
  printf(" exporting...\n");
  DagExporter::exportGeometry(dagPathOUT, scn.root);
  printf(" OK\n");
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
