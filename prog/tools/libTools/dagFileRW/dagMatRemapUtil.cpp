// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/dagFileRW/dagMatRemapUtil.h>

#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_files.h>

#include <stdio.h>

static void add_color_if_not_default(DataBlock &mat_blk, const char *name, const DagColor &c, int def = 255)
{
  if (c.r != def || c.g != def || c.b != def)
    mat_blk.addIPoint3(name, IPoint3(c.r, c.g, c.b));
}

DataBlock make_dag_one_mat_blk(const DagData &data, int mat_id, bool put_all_params)
{
  const DagData::Material &m = data.matlist[mat_id];
  DataBlock matBlk;
  matBlk.addStr("name", m.name.c_str());
  matBlk.addStr("class", m.classname.c_str());
  if (put_all_params || (m.mater.flags & DAG_MF_16TEX))
    matBlk.addBool("tex16support", m.mater.flags & DAG_MF_16TEX);
  if (put_all_params || (m.mater.flags & DAG_MF_2SIDED))
    matBlk.addBool("twosided", (m.mater.flags & DAG_MF_2SIDED) != 0);

  add_color_if_not_default(matBlk, "amb", m.mater.amb, put_all_params ? -1 : 255);
  add_color_if_not_default(matBlk, "diff", m.mater.diff, put_all_params ? -1 : 255);
  add_color_if_not_default(matBlk, "spec", m.mater.spec, put_all_params ? -1 : 255);
  add_color_if_not_default(matBlk, "emis", m.mater.emis, put_all_params ? -1 : 0);

  if (put_all_params || m.mater.power != 8.0f)
    matBlk.addReal("power", m.mater.power);

  String script;
  for (int si = 0; si <= m.script.length(); ++si)
  {
    if (m.script[si] == '\n' || m.script[si] == '\0')
    {
      if (script.length())
      {
        script.push_back(0);
        matBlk.addStr("script", script.c_str());
        script.clear();
      }
    }
    else if (m.script[si] != '\r')
      script.push_back(m.script[si]);
  }

  for (int ti = 0; ti < DAGTEXNUM; ++ti)
    if (m.mater.texid[ti] != DAGBADMATID && (ti < 8 || (m.mater.flags & DAG_MF_16TEX)))
      matBlk.addStr(String(0, "tex%d", ti).c_str(), data.texlist[m.mater.texid[ti]].c_str());

  return matBlk;
}

DataBlock make_dag_mats_blk(const DagData &data)
{
  DataBlock result;
  for (int matId = 0; matId < data.matlist.size(); ++matId)
  {
    DataBlock matBlk = make_dag_one_mat_blk(data, matId, matId == 0);
    result.addNewBlock(&matBlk, "material");
  }
  return result;
}

int remove_unused_textures(DagData &data)
{
  if (!data.texlist.size())
    return 0;
  Tab<int> texturesMap(tmpmem);
  texturesMap.resize(data.texlist.size());
  mem_set_ff(texturesMap);

  for (int i = 0; i < data.matlist.size(); ++i)
  {
    DagData::Material &m = data.matlist[i];
    for (int ti = 0; ti < DAGTEXNUM; ++ti)
      if (m.mater.texid[ti] != DAGBADMATID && (ti < 8 || (m.mater.flags & DAG_MF_16TEX)))
      {
        texturesMap[m.mater.texid[ti]] = m.mater.texid[ti];
      }
  }
  int count = 0;
  for (int i = texturesMap.size() - 1; i >= 0; i--)
  {
    if (texturesMap[i] < 0)
    {
      for (int j = i + 1; j < texturesMap.size(); ++j)
        if (texturesMap[j] >= 0)
          texturesMap[j]--;
      printf("%s\n", (char *)data.texlist[i]);
      erase_items(data.texlist, i, 1);
      count++;
    }
  }

  for (int i = 0; i < data.matlist.size(); ++i)
  {
    DagData::Material &m = data.matlist[i];
    for (int ti = 0; ti < DAGTEXNUM; ++ti)
      if (m.mater.texid[ti] != DAGBADMATID && (ti < 8 || (m.mater.flags & DAG_MF_16TEX)))
        m.mater.texid[ti] = texturesMap[m.mater.texid[ti]];
  }
  return count;
}

class MatRemapImpBlkReader : public FullFileLoadCB
{
  SimpleString buf;
  Tab<int> blkTag;

public:
  SimpleString fname;
  char *str;
  char *str1;

public:
  MatRemapImpBlkReader(const char *fn) : FullFileLoadCB(fn), str(NULL), str1(NULL), blkTag(tmpmem)
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
    DAGOR_CATCH(const IGenLoad::LoadException &e) { return false; }

    return true;
  }
  inline bool seektoSafe(int ofs)
  {
    DAGOR_TRY { seekto(ofs); }
    DAGOR_CATCH(const IGenLoad::LoadException &e) { return false; }

    return true;
  }

  int beginBlockSafe()
  {
    int tag = -1;
    DAGOR_TRY { tag = beginTaggedBlock(); }
    DAGOR_CATCH(const IGenLoad::LoadException &e) { return -1; }
    blkTag.push_back(tag);
    return getBlockLength() - 4;
  }
  bool endBlockSafe()
  {
    int tag = -1;
    DAGOR_TRY { endBlock(); }
    DAGOR_CATCH(const IGenLoad::LoadException &e) { return false; }
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

  void read_err()
  {
    // cb.read_error(fname);
  }
  void level_err()
  {
    // cb.format_error(fname);
  }
};

#define READ_ERR    \
  do                \
  {                 \
    rdr.read_err(); \
    goto err;       \
  } while (0)
#define READ(p, l)           \
  do                         \
  {                          \
    if (!rdr.readSafe(p, l)) \
      READ_ERR;              \
  } while (0)
#define BEGIN_BLOCK               \
  do                              \
  {                               \
    if (rdr.beginBlockSafe() < 0) \
      goto err;                   \
  } while (0)
#define END_BLOCK            \
  do                         \
  {                          \
    if (!rdr.endBlockSafe()) \
      goto err;              \
  } while (0)

static DagData::Node::NodeBlock &append_block(DagData::Node &node, int block_tag)
{
  append_items(node.blocks, 1);
  DagData::Node::NodeBlock &block = node.blocks.back();
  block.blockType = block_tag;
  return block;
}

static bool load_node(MatRemapImpBlkReader &rdr, DagData::Node &node)
{
  while (rdr.getBlockRest() > 0)
  {
    BEGIN_BLOCK;

    const int blockTag = rdr.getBlockTag();
    switch (blockTag)
    {
      case DAG_NODE_CHILDREN:
        while (rdr.getBlockRest() > 0)
        {
          BEGIN_BLOCK;
          append_items(node.children, 1);
          if (rdr.getBlockTag() == DAG_NODE)
          {
            if (!load_node(rdr, node.children.back()))
              return false;
          }
          else
          {
            DEBUG_CTX("load_node error");
            printf("error\n");
            return false;
          }
          END_BLOCK;
        }
        break;
      case DAG_NODE_DATA:
      {
        DagData::Node::NodeBlock &block = append_block(node, blockTag);
        READ(&block.data, sizeof(block.data));
        int l = rdr.getBlockRest();
        if (l > 255)
          l = 255;
        char str[256];
        READ(str, l);
        str[l] = 0;
        block.name = str;
        break;
      }
      case DAG_NODE_MATER:
      {
        DagData::Node::NodeBlock &block = append_block(node, blockTag);
        int num = rdr.getBlockLen() / sizeof(uint16_t);
        clear_and_resize(block.mater, num);
        READ(block.mater.data(), data_size(block.mater));
        break;
      }
      case DAG_NODE_TM:
      {
        DagData::Node::NodeBlock &block = append_block(node, blockTag);
        READ(&block.nodeTm, sizeof(block.nodeTm));
      }
      break;
      case DAG_NODE_SCRIPT:
      {
        DagData::Node::NodeBlock &block = append_block(node, blockTag);
        int l = rdr.getBlockLen();
        block.script.allocBuffer(l + 1);
        if (rdr.tryRead(&block.script[0], l) != l)
        {
          rdr.read_err();
          return 0;
        }
        block.script[l] = 0;
        break;
      }
      default:
      {
        DagData::Node::NodeBlock &block = append_block(node, blockTag);
        block.block.start = rdr.tell();
        block.block.tag = blockTag;
        block.block.data.reserve(rdr.getBlockRest());
        READ(block.block.data.data(), rdr.getBlockRest());
        block.block.end = rdr.tell();
        break;
      }
    }
    END_BLOCK;
  }
  return true;

err:
  DEBUG_CTX("load_node error");
  return false;
}

bool load_scene(const char *fname, DagData &data, bool read_nodes)
{
  MatRemapImpBlkReader rdr(fname);
  if (!rdr.fileHandle)
    return false;
  if (!rdr.seektoSafe(4))
  {
    rdr.read_err();
    return false;
  }
  BEGIN_BLOCK;
  for (; rdr.getBlockRest() > 0;)
  {
    BEGIN_BLOCK;
    /*if (rdr.getBlockTag()==DAG_END)
    {
      END_BLOCK;break;
    }else */
    if (rdr.getBlockTag() == DAG_TEXTURES)
    {
      // DEBUG_CTX("begin tex %d", rdr.tell());
      data.texStart = rdr.tell();
      uint16_t num;
      READ(&num, 2);
      // DEBUG_CTX("textures %d", num);
      clear_and_shrink(data.texlist);
      DEBUG_CP();
      data.texlist.reserve(num);
      // DEBUG_CTX("textures %d", num);
      for (int i = 0; i < num; ++i)
      {
        int l = 0;
        char str[256];
        READ(&l, 1);
        READ(str, l);
        str[l] = 0;
        data.texlist.push_back(String(str));
      }
      data.texEnd = rdr.tell();
      // DEBUG_CTX("end tex %d", data.texEnd);
    }
    else if (rdr.getBlockTag() == DAG_MATER)
    {
      DagData::Material m;
      m.start = rdr.tell();
      int l = 0;
      char str[256];
      READ(&l, 1);
      if (l)
        READ(str, l);
      str[l] = 0;
      m.name = str;
      // DEBUG_CTX("mat <%s>", str);
      READ(&m.mater, sizeof(DagMater));
      l = 0;
      READ(&l, 1);
      if (l)
        READ(str, l);
      str[l] = 0;
      m.classname = str;
      l = rdr.getBlockRest();
      if (l > 0)
      {
        m.script.allocBuffer(l + 1);
        if (rdr.tryRead(&m.script[0], l) != l)
        {
          rdr.read_err();
          return 0;
        }
        m.script[l] = 0;
      }
      m.end = rdr.tell();
      data.matlist.push_back(m);
    }
    else if (rdr.getBlockTag() == DAG_NODE && read_nodes)
    {
      append_items(data.blocks, 1);
      data.blocks.back().start = rdr.tell();
      data.blocks.back().tag = rdr.getBlockTag();
      data.blocks.back().data.reserve(rdr.getBlockRest());
      rdr.read(data.blocks.back().data.data(), rdr.getBlockRest());
      data.blocks.back().end = rdr.tell();
      append_items(data.nodes, 1);
      rdr.seekto(data.blocks.back().start);
      if (load_node(rdr, data.nodes.back()))
      {
        data.blocks.pop_back();
      }
      else
      {
        data.nodes.pop_back();
        rdr.seekto(data.blocks.back().end);
      }
    }
    else
    {
      append_items(data.blocks, 1);
      data.blocks.back().start = rdr.tell();
      data.blocks.back().tag = rdr.getBlockTag();
      data.blocks.back().data.reserve(rdr.getBlockRest());
      rdr.read(data.blocks.back().data.data(), rdr.getBlockRest());
      data.blocks.back().end = rdr.tell();
    }
    END_BLOCK;
  }
  END_BLOCK;
  return true;

err:
  return false;
}

static bool write_dag_nodes(FullFileSaveCB &dag, const DagData::Node &node)
{
  dag.beginTaggedBlock(DAG_NODE);
  for (int i = 0; i < node.blocks.size(); ++i)
  {
    const DagData::Node::NodeBlock &block = node.blocks[i];
    dag.beginTaggedBlock(block.blockType);
    switch (block.blockType)
    {
      case DAG_NODE_DATA:
      {
        dag.write(&block.data, sizeof(block.data));
        dag.write((const char *)block.name, block.name.length());
        break;
      }
      case DAG_NODE_TM:
      {
        dag.write(&block.nodeTm, sizeof(block.nodeTm));
        break;
      }
      case DAG_NODE_SCRIPT:
      {
        dag.write((const char *)block.script, block.script.length());
        break;
      }
      case DAG_NODE_MATER:
      {
        dag.writeTabData(block.mater);
        break;
      }
      default:
      {
        dag.write(block.block.data.data(), block.block.end - block.block.start);
        break;
      }
    }
    dag.endBlock();
  }
  if (node.children.size())
  {
    dag.beginTaggedBlock(DAG_NODE_CHILDREN);
    for (int i = 0; i < node.children.size(); ++i)
      write_dag_nodes(dag, node.children[i]);
    dag.endBlock();
  }


  dag.endBlock();
  return true;
}

bool write_dag(const char *dest, const DagData &data)
{
  FullFileSaveCB dag(dest, DF_WRITE);

  if (!dag.fileHandle)
    return false;
  dag.writeInt(DAG_ID);
  dag.beginTaggedBlock(DAG_ID);

  dag.beginTaggedBlock(DAG_TEXTURES);
  dag.writeIntP<2>(data.texlist.size());
  for (int i = 0; i < data.texlist.size(); ++i)
  {
    const unsigned char len = data.texlist[i].length();
    dag.writeIntP<1>(len);
    dag.write((const char *)data.texlist[i].data(), len);
  }
  dag.endBlock();

  for (int i = 0; i < data.matlist.size(); ++i)
  {
    dag.beginTaggedBlock(DAG_MATER);
    unsigned char len = data.matlist[i].name.length();
    dag.writeIntP<1>(len);
    dag.write((const char *)data.matlist[i].name, len);
    dag.write(&data.matlist[i].mater, sizeof(DagMater));
    len = data.matlist[i].classname.length();
    dag.writeIntP<1>(len);
    dag.write((const char *)data.matlist[i].classname, len);
    dag.write((const char *)data.matlist[i].script, data.matlist[i].script.length());
    dag.endBlock();
  }

  for (int i = 0; i < data.nodes.size(); ++i)
    write_dag_nodes(dag, data.nodes[i]);

  for (int i = 0; i < data.blocks.size(); ++i)
  {
    dag.beginTaggedBlock(data.blocks[i].tag);
    dag.write(data.blocks[i].data.data(), data.blocks[i].end - data.blocks[i].start);
    dag.endBlock();
  }

  dag.endBlock();
  return true;
}
