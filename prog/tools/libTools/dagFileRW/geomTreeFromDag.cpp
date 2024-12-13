// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/dagFileRW/nodeTreeBuilder.h>
#include <osApiWrappers/dag_direct.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/util/makeBindump.h>
#include <util/dag_globDef.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_btagCompr.h>


bool GeomNodeTreeBuilder::loadFromDag(const char *filename)
{
  //== TODO: refactor to use buildTree() method and move it away from the GeomNodeTree

  clear_and_shrink(data);
  nodes.init(NULL, 0);

  if (!filename)
    return false;
  if (!dd_file_exist(filename))
    return false;

  AScene sc;
  if (!load_ascene(filename, sc, LASF_NOMATS | LASF_NOMESHES | LASF_NOSPLINES | LASF_NOLIGHTS, false))
    return false;

  G_ASSERT(sc.root);

  sc.root->calc_wtm();
  buildFromDagNodes(sc.root);
  return true;
}

void GeomNodeTreeBuilder::buildFromDagNodes(Node *sc_root, const char *unimportant_nodes_prefix)
{
  // gather nodes
  Tab<Node *> dagNodes(tmpmem);
  Tab<NodeBuildData> nodeData(tmpmem);

  const int ALLOC_STEP = 500;

  int nameOfs = 0;

  dagNodes.push_back(sc_root);
  nodeData.push_back(NodeBuildData(0, 0, -1, 0));

  if (sc_root->name)
    nameOfs += (int)strlen(sc_root->name) + 1;
  else
    nameOfs++;

  for (int i = 0; i < dagNodes.size(); ++i)
  {
    Node &n = *dagNodes[i];

    nodeData[i].childId = dagNodes.size();

    for (int j = 0; j < n.child.size(); ++j)
      if (n.child[j])
      {
        dagNodes.push_back(n.child[j]);
        nodeData.push_back(NodeBuildData(0, 0, i, nameOfs));
        ++nodeData[i].childNum;

        if (n.child[j]->name)
          nameOfs += (int)strlen(n.child[j]->name) + 1;
        else
          nameOfs++;
      }
  }

  // build data
  unsigned ofs = (dagNodes.size() * sizeof(TreeNode) + nameOfs + 3) & ~3;
  invalidTmOfs = ofs;
  G_ASSERT(invalidTmOfs == ofs);
  clear_and_resize(data, invalidTmOfs + (dagNodes.size() + 7) / 8);
  mem_set_0(data);

  nodes.init((TreeNode *)data.data(), dagNodes.size());

  char *namePtr = &data[dagNodes.size() * sizeof(TreeNode)];

  for (int i = 0; i < dagNodes.size(); ++i)
  {
    Node &dn = *dagNodes[i];
    NodeBuildData &bd = nodeData[i];
    TreeNode &n = nodes[i];

    n.child.init(nodes.data() + bd.childId, bd.childNum);

    if (bd.parentId >= 0)
      n.parent = &nodes[bd.parentId];
    else
      n.parent = NULL;

    as_point4(&n.tm.col0).set(dn.tm.m[0][0], dn.tm.m[0][1], dn.tm.m[0][2], 0);
    as_point4(&n.tm.col1).set(dn.tm.m[1][0], dn.tm.m[1][1], dn.tm.m[1][2], 0);
    as_point4(&n.tm.col2).set(dn.tm.m[2][0], dn.tm.m[2][1], dn.tm.m[2][2], 0);
    as_point4(&n.tm.col3).set(dn.tm.m[3][0], dn.tm.m[3][1], dn.tm.m[3][2], 1);

    as_point4(&n.wtm.col0).set(dn.wtm.m[0][0], dn.wtm.m[0][1], dn.wtm.m[0][2], 0);
    as_point4(&n.wtm.col1).set(dn.wtm.m[1][0], dn.wtm.m[1][1], dn.wtm.m[1][2], 0);
    as_point4(&n.wtm.col2).set(dn.wtm.m[2][0], dn.wtm.m[2][1], dn.wtm.m[2][2], 0);
    as_point4(&n.wtm.col3).set(dn.wtm.m[3][0], dn.wtm.m[3][1], dn.wtm.m[3][2], 1);

    n.name = namePtr + bd.nameOfs;
    strcpy(namePtr + bd.nameOfs, dn.name ? dn.name.c_str() : "");
  }

  lastValidWtmIndex = -1;
  lastUnimportantCount = 0;

  if (unimportant_nodes_prefix && *unimportant_nodes_prefix)
  {
    int un_prefix_len = i_strlen(unimportant_nodes_prefix);
    Tab<int> n_remap;
    n_remap.resize(nodes.size());
    mem_set_ff(n_remap);
    n_remap[0] = 0;
    int n_idx = 1;
    for (int i = 1; i < n_remap.size(); i++)
    {
      int p_idx = nodes[i].parent.get() - nodes.data();
      if (strncmp(nodes[i].name, unimportant_nodes_prefix, un_prefix_len) != 0 && n_remap[p_idx] >= 0)
      {
        n_remap[i] = n_idx;
        n_idx++;
      }
      else if (n_remap[p_idx] >= 0)
      {
        for (int j = 0; j < nodes[i].parent->child.size(); j++)
          if (strncmp(nodes[i].parent->child[j].name, unimportant_nodes_prefix, un_prefix_len) != 0)
          {
            n_remap[i] = n_idx;
            n_idx++;
            break;
          }
      }
    }

    lastUnimportantCount = n_remap.size() - n_idx;
    G_ASSERT(lastUnimportantCount == n_remap.size() - n_idx);
    for (int i = 0; i < n_remap.size(); i++)
      if (n_remap[i] == -1)
      {
        n_remap[i] = n_idx;
        n_idx++;
      }

    Tab<TreeNode> new_nodes;
    new_nodes.resize(nodes.size());
    for (int i = 0; i < new_nodes.size(); i++)
    {
      memcpy(&new_nodes[n_remap[i]], &nodes[i], sizeof(nodes[i])); //-V780
      new_nodes[n_remap[i]].parent = nodes[i].parent.get() ? &nodes[n_remap[nodes[i].parent.get() - nodes.data()]] : nullptr;
      if (int cn = nodes[i].child.size())
        new_nodes[n_remap[i]].child.init(&nodes[n_remap[nodes[i].child.data() - nodes.data()]], cn);
      else
        new_nodes[n_remap[i]].child.init(NULL, 0);
    }
    mem_copy_from(nodes, new_nodes.data());
  }
}


void GeomNodeTreeBuilder::save(mkbindump::BinDumpSaveCB &final_cwr, bool compr, bool allow_oodle)
{
  G_ASSERTF(nodes.size() < 0x80000000, "nodes.size()=%u", nodes.size());
  final_cwr.writeInt32e(invalidTmOfs | (lastUnimportantCount << 20));

  mkbindump::BinDumpSaveCB cwr(64 << 10, final_cwr.getTarget(), final_cwr.WRITE_BE);
  for (int i = 0; i < nodes.size(); ++i)
  {
    const TreeNode &n = nodes[i];
    // class GeomTreeNode
    cwr.write32ex(&n.tm, 2 * sizeof(mat44f));                                    // mat44f tm, wtm;
    cwr.writeRef((char *)n.child.data() - data.data(), n.child.size());          // PatchableTab<GeomTreeNode> child;
    cwr.writePtr64e(n.parent.get() ? (char *)n.parent.get() - data.data() : -1); // PatchablePtr<GeomTreeNode> parent;
    cwr.writePtr64e(n.name.get() - data.data());                                 // PatchablePtr<const char> name;
  }

  // save names
  cwr.writeRaw(&data[data_size(nodes)], invalidTmOfs - data_size(nodes));

  if (compr && cwr.getSize() > 512) // compressing is requested and has sence
  {
    mkbindump::BinDumpSaveCB zcwr(cwr.getSize(), final_cwr.getTarget(), final_cwr.WRITE_BE);
    MemoryLoadCB mcrd(cwr.getMem(), false);
    if (allow_oodle)
    {
      zcwr.writeInt32e(cwr.getSize());
      oodle_compress_data(zcwr.getRawWriter(), mcrd, cwr.getSize());
    }
    else
      // unlikely that skeleton exceeds 1M but limit window anyway
      zstd_compress_data(zcwr.getRawWriter(), mcrd, cwr.getSize(), 1 << 20, 18, 20);

    if (zcwr.getSize() < cwr.getSize() * 8 / 10 && zcwr.getSize() + 256 < cwr.getSize()) // enough profit in compression
    {
      // write compressed
      final_cwr.writeInt32e(nodes.size() | 0x80000000);
      final_cwr.beginBlock();
      zcwr.copyDataTo(final_cwr.getRawWriter());
      final_cwr.endBlock(allow_oodle ? btag_compr::OODLE : btag_compr::ZSTD);
      return;
    }
  }
  // write uncompressed
  final_cwr.writeInt32e(nodes.size());
  cwr.copyDataTo(final_cwr.getRawWriter());
}

void GeomNodeTreeBuilder::saveDump(IGenSave &cwr, unsigned target_code, bool compr, bool allow_oodle)
{
  bool write_be = dagor_target_code_be(target_code);
  mkbindump::BinDumpSaveCB bdcwr(256 << 10, target_code, write_be);
  save(bdcwr, compr, allow_oodle);
  bdcwr.copyDataTo(cwr);
}
