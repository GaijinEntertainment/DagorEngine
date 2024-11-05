// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <generic/dag_carray.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_driver.h>
#include <util/dag_string.h>

#include <textureGen/textureRegManager.h>
#include <textureGen/textureGenShader.h>
#include <textureGen/textureGenerator.h>
#include <textureGen/textureGenLogger.h>
#include "hashSaver.h"
#include <EASTL/sort.h>
#include <EASTL/string.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <EASTL/vector.h>
#include <util/dag_bitArray.h>
#include <EASTL/vector_set.h>
#include <EASTL/unique_ptr.h>

static TextureGenLogger defaultLogger;

#define FALLBACK_TO_NO_OPTIMIZATION_ON_FAILURE 1

#if FALLBACK_TO_NO_OPTIMIZATION_ON_FAILURE
#define MAX_GEN_NODE_CALL_COUNT 100000000
static unsigned gen_node_call_count = 0;
static bool optimizer_failed_in_this_update = false;
#endif
static bool optimizer_failure = false;


struct NodeData : TextureGenNodeData
{
  TextureGenShader *shader = 0;
};

#define PARTICLES "particles"
#define SHADER    "shader"
#define BLENDING  "blending"

class TextureGenerator
{
public:
  TextureGenerator() : logger(&defaultLogger) {}
  ~TextureGenerator() { close(); }

  bool addShader(const char *name, TextureGenShader *shader); // shader will be called destroy on close. return false if such name
                                                              // exists
  TextureGenShader *getShader(const char *name);
  void close();
  TextureGenLogger *getLogger() const { return logger; }

  void setLogger(TextureGenLogger *logger_) { logger = logger_ ? logger_ : &defaultLogger; }

  int start(const DataBlock &nodes, TextureRegManager &reg_manager);
  void stop();
  int processNodes(unsigned count);

  bool startNode(NodeData &data, const DataBlock &node, eastl::hash_map<eastl::string, int> &regCnt, TextureRegManager &reg_manager);

  void endNode(NodeData &data, TextureRegManager &reg_manager);

  const char *getFinalRegName(const char *name) const
  {
    if (!name)
      return name;
    auto it = finalRegistersRenamed.find_as(name);
    if (it != finalRegistersRenamed.end())
      return it->second.c_str();
    return name;
  }

protected:
  void startProcessNodes(const DataBlock &nodes, TextureRegManager &regs);
  bool processNode(const DataBlock &node, int si, eastl::hash_map<eastl::string, int> &regCnt, TextureRegManager &regs);
  void calcRegCount(const DataBlock &node, eastl::hash_map<eastl::string, int> &regCnt);
  eastl::hash_map<eastl::string, TextureGenShader *> shaders;
  TextureGenLogger *logger;
  eastl::hash_map<eastl::string, int> regUsageCount;
  int cNode = -1;
  int cNodeSubStep = 0;
  unsigned cNodeStepsCount = 1;
  NodeData cNodeData;
  int getNodeCount() const { return nodesExecutionOrder.size(); }
  const DataBlock &getNode(int i) const { return *allNodesRemapped.getBlock(nodesExecutionOrder[i]); }
  Tab<int> nodesExecutionOrder;
  DataBlock allNodesRemapped;
  eastl::hash_map<eastl::string, eastl::string> finalRegistersRenamed;
  eastl::hash_map<eastl::string, uint64_t> nodesData;
  eastl::hash_set<eastl::string> changedShaders;
  TextureRegManager *regManager = 0;
};

static BlendingType parse_blending(const char *blend);

TextureGenShader *TextureGenerator::getShader(const char *name)
{
  auto found = shaders.find_as(name);
  if (found != shaders.end())
    return found->second;
  return NULL;
}

bool TextureGenerator::addShader(const char *name, TextureGenShader *shader)
{
  auto sh = shaders.find_as(name);
  if (sh != shaders.end())
  {
    if (sh->second != shader)
    {
      logger->log(LOGLEVEL_WARN, String(128, "tex gen shader name <%s> changed", name));
      sh->second->destroy();
      changedShaders.insert(name);
    }
    else
      logger->log(LOGLEVEL_REMARK, String(128, "replace shader with same <%s>", name));
  }
  else
  {
    logger->log(LOGLEVEL_REMARK, String(128, "add shader <%s>", name));
    changedShaders.insert(name);
  }
  shaders[name] = shader;
  return true;
}


void TextureGenerator::close()
{
  for (auto used = shaders.begin(); used != shaders.end(); ++used)
  {
    used->second->destroy();
  }
  shaders.clear();
}

static void dead_code_elimination(Bitarray &nodeUsed, const eastl::hash_set<int> &final,
  const eastl::vector<eastl::vector_set<int>> &nodesProducingCurrent)
{
  eastl::hash_set<int> nodeWorkingSet = final;
  for (; nodeWorkingSet.size();)
  {
    const int nodeI = *nodeWorkingSet.begin();
    nodeWorkingSet.erase(nodeWorkingSet.begin());
    nodeUsed.set(nodeI);
    for (auto producingNode : nodesProducingCurrent[nodeI])
    {
      if (producingNode >= 0 && !nodeUsed[producingNode])
        nodeWorkingSet.insert(producingNode);
    }
  }
}


// instruction selection by Andrew Appel
// Generalizations of the Sethi-Ullman algorithm for register allocation
// http://citeseerx.ist.psu.edu/viewdoc/download;jsessionid=80312C42098C825315D21D8103609E81?doi=10.1.1.54.319&rep=rep1&type=pdf
// simple Sethi-Ullman algorithm https://en.wikipedia.org/wiki/Sethi-Ullman_algorithm
struct AppelNode
{
  int64_t hold = 1; // register output size
  int64_t maxregs = 0;
  int nodeI = -1;
  AppelNode(int i, unsigned hold_) : nodeI(i), hold(hold_) {}
  AppelNode() = default;
  ~AppelNode() = default;
  AppelNode(const AppelNode &) = default;
  AppelNode(AppelNode &&) = default;
  AppelNode &operator=(const AppelNode &) = default;
  AppelNode &operator=(AppelNode &&) = default;
  eastl::vector<AppelNode> children;
};

static void reorder(AppelNode &node)
{
  if (node.children.size() == 0)
  {
    node.maxregs = node.hold;
    return;
  }

  for (auto &child : node.children)
  {
    if (!child.maxregs)
      reorder(child);
  }

  eastl::vector<AppelNode> &nodes = node.children;
  for (int j = 0; j < nodes.size(); j++)
    for (int i = j; i > 0; i--)
      if (nodes[i].maxregs - nodes[i].hold > nodes[i - 1].maxregs - nodes[i - 1].hold)
        eastl::swap(nodes[i], nodes[i - 1]);

  node.maxregs = 0;

  for (int i = nodes.size() - 1; i >= 0; i--)
  {
    node.maxregs += nodes[i].hold;
    if (node.maxregs < nodes[i].maxregs)
      node.maxregs = nodes[i].maxregs;
  }
}


static void gen_appel_node(AppelNode &node, const Bitarray &nodeUsed, const Bitarray &nodeGenerated,
  const eastl::vector<unsigned> &nodesSize, const eastl::vector<eastl::vector_set<int>> &nodesProducingCurrent,
  const eastl::vector<eastl::vector_set<int>> &nodesProducedByCurrent)
{
  if (!nodeUsed[node.nodeI] || nodeGenerated[node.nodeI])
    return;

#if FALLBACK_TO_NO_OPTIMIZATION_ON_FAILURE
  gen_node_call_count++;
  if (gen_node_call_count > MAX_GEN_NODE_CALL_COUNT)
  {
    optimizer_failure = true;
    optimizer_failed_in_this_update = true;
    return;
  }
#endif

  // debug("%d: node Size %d produced = %d", node.nodeI, nodesSize[node.nodeI], nodesProducedByCurrent[node.nodeI].size());
  // node.hold = max(1U, 4 * nodesProducedByCurrent[node.nodeI].size());
  node.hold = nodesSize[node.nodeI];
  for (auto chNode : nodesProducingCurrent[node.nodeI])
  {
    if (!nodeUsed[chNode] || nodeGenerated[chNode])
      continue;
    node.children.emplace_back(chNode, nodesSize[chNode]); //
    gen_appel_node(node.children.back(), nodeUsed, nodeGenerated, nodesSize, nodesProducingCurrent, nodesProducedByCurrent);
  }
  reorder(node);
}

static void generate_appel_node(AppelNode &node, Bitarray &nodeGenerated, Tab<int> &nodesExecutionOrder)
{
  if (node.nodeI >= 0 && nodeGenerated[node.nodeI])
    return;

  if (node.children.size() != 0)
  {
    for (auto &child : node.children)
      generate_appel_node(child, nodeGenerated, nodesExecutionOrder);
  }

  if (node.nodeI < 0 || nodeGenerated[node.nodeI])
    return;

  nodeGenerated.set(node.nodeI, 1);
  nodesExecutionOrder.push_back(node.nodeI);
}

/*
static void simple_instruction_selection(Bitarray &nodeGenerated, Tab<int> &nodesExecutionOrder,
  const Bitarray &nodeUsed, // alive nodes
  const eastl::hash_set<int> &finalNodes, const eastl::vector<eastl::vector_set<int>> &nodesProducingCurrent)
{
  if (!finalNodes.size())
    return;

  Bitarray nodeScheduled;
  nodeScheduled.resize(nodeGenerated.size());
  nodeScheduled.reset();

  eastl::hash_set<int> nodeWorkingSet = finalNodes;
  for (auto nodeI : nodeWorkingSet)
    nodeScheduled.set(nodeI, 1);

  for (; nodeWorkingSet.size();)
  {
    const int workingSetSize = nodeWorkingSet.size(), generated = nodesExecutionOrder.size();
    for (auto nit = nodeWorkingSet.begin(); nit != nodeWorkingSet.end(); ++nit)
    {
      const int nodeI = *nit;
      bool allNodesGenerated = true;
      bool nodesScheduled = false;

      for (auto producingNode : nodesProducingCurrent[nodeI])
        if (nodeUsed[producingNode] && !nodeGenerated[producingNode])
        {
          allNodesGenerated = false;
          if (nodeScheduled[producingNode])
            continue;
          nodeScheduled.set(producingNode, 1);
          nodeWorkingSet.insert(producingNode);
          nodesScheduled = true;
        }

      if (nodesScheduled)
        break;

      if (allNodesGenerated)
      {
        nodeWorkingSet.erase(nit);
        nodeGenerated.set(nodeI, 1);
        nodesExecutionOrder.push_back(nodeI);
        break;
      }
    }
    G_ASSERT(workingSetSize != nodeWorkingSet.size() || generated != nodesExecutionOrder.size());
  }
}
*/

static void instruction_selection_appel(Bitarray &nodeGenerated, Tab<int> &nodesExecutionOrder,
  const Bitarray &nodeUsed, // alive nodes
  const eastl::vector<unsigned> &nodesSize, const eastl::hash_set<int> &finalNodes,
  const eastl::vector<eastl::vector_set<int>> &nodesProducingCurrent,
  const eastl::vector<eastl::vector_set<int>> &nodesProducedByCurrent)
{
  AppelNode pseudo_node = {-1, 0};

  for (auto genNode : finalNodes)
  {
    if (nodeGenerated[genNode] || !nodeUsed[genNode])
      continue;
    pseudo_node.hold += nodesSize[genNode];
    pseudo_node.children.emplace_back(genNode, nodesSize[genNode]);
    gen_appel_node(pseudo_node.children.back(), nodeUsed, nodeGenerated, nodesSize, nodesProducingCurrent, nodesProducedByCurrent);
  }

  reorder(pseudo_node);
  generate_appel_node(pseudo_node, nodeGenerated, nodesExecutionOrder);
}

struct NodePriority
{
  int ni = -1;
  int prio = 0;
  NodePriority() = default;
  NodePriority(int prio_, int ni_) : ni(ni_), prio(prio_) {}
};

static void node_cse_priority(eastl::vector<NodePriority> &nodePriority, const Bitarray &nodeUsed,
  const eastl::vector<eastl::vector_set<int>> &nodesProducedByCurrent)
{
  nodePriority.reserve(nodesProducedByCurrent.size());

  for (int i = 0; i < nodesProducedByCurrent.size(); ++i)
  {
    if (!nodeUsed[i])
      continue;
    nodePriority.push_back(NodePriority(nodesProducedByCurrent[i].size(), i));
  }
}

static void instruction_selection_one_cse(Bitarray &nodeGenerated, Tab<int> &nodesExecutionOrder,
  const Bitarray &nodeUsed, // alive nodes
  const eastl::vector<unsigned> &nodesSize, const eastl::hash_set<int> &finalNodes,
  const eastl::vector<eastl::vector_set<int>> &nodesProducingCurrent,
  const eastl::vector<eastl::vector_set<int>> &nodesProducedByCurrent)
{
  if (!finalNodes.size())
    return;
  eastl::vector<NodePriority> nodePriority;
  node_cse_priority(nodePriority, nodeUsed, nodesProducedByCurrent);
  eastl::sort(nodePriority.begin(), nodePriority.end(), [](const auto &a, const auto &b) { return b.prio < a.prio; });

  // find one 'best' common subexpression (it should have a lot of outputs), and evaluate it first.
  for (int i = 0; i < nodePriority.size(); ++i)
  {
    int nodeI = nodePriority[i].ni;
    if (!nodeUsed[nodeI] || nodeGenerated[nodeI])
      continue;
    if (nodesProducingCurrent[nodeI].size() < 1)
      continue;
    if (nodePriority[i].prio <= 2)
      break;
    eastl::hash_set<int> finalNode;
    finalNode.insert(nodeI);
    // simple_instruction_selection(nodeGenerated, nodesExecutionOrder, nodeUsed, finalNode, nodesProducingCurrent);
    instruction_selection_appel(nodeGenerated, nodesExecutionOrder, nodeUsed, nodesSize, finalNode, nodesProducingCurrent,
      nodesProducedByCurrent);
    break;
  }

  instruction_selection_appel(nodeGenerated, nodesExecutionOrder, nodeUsed, nodesSize, finalNodes, nodesProducingCurrent,
    nodesProducedByCurrent);
}

static void instruction_selection(Bitarray &nodeGenerated, Tab<int> &nodesExecutionOrder,
  const Bitarray &nodeUsed, // alive nodes
  const eastl::vector<unsigned> &nodesSize, const eastl::hash_set<int> &finalNodes,
  const eastl::vector<eastl::vector_set<int>> &nodesProducingCurrent,
  const eastl::vector<eastl::vector_set<int>> &nodesProducedByCurrent)
{
  if (!finalNodes.size())
    return;
  instruction_selection_one_cse(nodeGenerated, nodesExecutionOrder, nodeUsed, nodesSize, finalNodes, nodesProducingCurrent,
    nodesProducedByCurrent);
  /*
  return;
  instruction_selection_appel(nodeGenerated, nodesExecutionOrder, nodeUsed, nodesSize, finalNodes, nodesProducingCurrent,
    nodesProducedByCurrent);
  return;
  simple_instruction_selection(nodeGenerated, nodesExecutionOrder, nodeUsed, finalNodes, nodesProducingCurrent);
  */
}
//--end of instruction selection

static void get_nodes_generating_regs(eastl::hash_set<int> &nodes, const eastl::hash_set<eastl::string> &regs,
  const eastl::hash_map<eastl::string, int> &regNamesMap, const eastl::vector<int> &nodeProducingReg)
{
  for (const auto &name : regs)
  {
    auto reg = regNamesMap.find_as(name.c_str());
    G_ASSERTF(reg != regNamesMap.end(), "can't find %s", name.c_str());
    if (reg != regNamesMap.end())
    {
      if (reg->second < nodeProducingReg.size() && nodeProducingReg[reg->second] >= 0)
        nodes.insert(nodeProducingReg[reg->second]);
    }
  }
}

static void remove_unchanged_nodes(eastl::hash_set<int> &nodes, const Bitarray &dirty_nodes)
{
  for (auto nodeIt = nodes.begin(); nodeIt != nodes.end();)
  {
    if (!dirty_nodes[*nodeIt])
      nodeIt = nodes.erase(nodeIt);
    else
      ++nodeIt;
  }
}


static void get_nodes_producing_current(const eastl::vector<eastl::vector<int>> controlNodesRegs[TSHADER_REG_TYPE_CNT],
  eastl::vector<int> &nodeProducingReg, eastl::vector<eastl::vector_set<int>> &nodesProducingCurrent)
{
  const int nodesCount = controlNodesRegs[TSHADER_REG_TYPE_OUTPUT].size();

  nodeProducingReg.resize(0);
  for (int ni = 0; ni < controlNodesRegs[TSHADER_REG_TYPE_OUTPUT].size(); ++ni)
  {
    for (const auto &regNo : controlNodesRegs[TSHADER_REG_TYPE_OUTPUT][ni])
    {
      if (nodeProducingReg.size() <= regNo)
      {
        const int oldSize = nodeProducingReg.size();
        nodeProducingReg.resize(regNo + 1);
        memset(&nodeProducingReg[oldSize], 0xFF, (nodeProducingReg.size() - oldSize) * sizeof(nodeProducingReg[0]));
      }
      G_ASSERTF(nodeProducingReg[regNo] < 0, "register is already produced by other node");
      /*G_ASSERTF(nodeProducingReg[regNo] < 0,
        "reg <%s> produced by %s is already produced by node <%s>", regNames[regNo].c_str(),
          nodes.getBlock(ni)->getBlockName(), nodes.getBlock(nodeProducingReg[regNo])->getBlockName());*/
      nodeProducingReg[regNo] = ni;
    }
  }
  nodesProducingCurrent.resize(nodesCount);

  for (int ni = 0; ni < nodesCount; ++ni)
  {
    for (auto sourceReg : controlNodesRegs[TSHADER_REG_TYPE_INPUT][ni])
      if (sourceReg < nodeProducingReg.size() && nodeProducingReg[sourceReg] >= 0)
        nodesProducingCurrent[ni].insert(nodeProducingReg[sourceReg]);
  }
}

static void get_nodes_produced_by_current(const eastl::vector<eastl::vector<int>> controlNodesRegs[TSHADER_REG_TYPE_CNT],
  eastl::vector<eastl::vector<int>> &nodesUsingReg, eastl::vector<eastl::vector_set<int>> &nodesProducedByCurrent)
{
  const int nodesCount = controlNodesRegs[TSHADER_REG_TYPE_INPUT].size();
  for (int ni = 0; ni < controlNodesRegs[TSHADER_REG_TYPE_INPUT].size(); ++ni)
  {
    for (const auto &regNo : controlNodesRegs[TSHADER_REG_TYPE_INPUT][ni])
    {
      if (nodesUsingReg.size() <= regNo)
        nodesUsingReg.resize(regNo + 1);
      nodesUsingReg[regNo].push_back(ni);
    }
  }
  nodesProducedByCurrent.resize(nodesCount);
  for (int ni = 0; ni < nodesCount; ++ni)
  {
    for (auto outReg : controlNodesRegs[TSHADER_REG_TYPE_OUTPUT][ni])
    {
      if (outReg < nodesUsingReg.size() && nodesUsingReg[outReg].size())
        nodesProducedByCurrent[ni].insert(nodesUsingReg[outReg].begin(), nodesUsingReg[outReg].end());
    }
  }
}

static void remove_reg(eastl::vector<int> &regs, int regNo)
{
  for (auto i = 0; i < regs.size(); ++i)
    if (regs[i] == regNo)
    {
      eastl::swap(regs.back(), regs[i]);
      regs.resize(regs.size() - 1);
    }
}

static constexpr int PAGE_SIZE = 4096; // any texture is no smaller than PAGE_SIZE

static void optimize(const DataBlock &nodes, const eastl::vector<int> &initial_dirty_nodes,
  const eastl::hash_set<eastl::string> &alive_regs, const eastl::hash_set<eastl::string> &final_regs,
  const eastl::hash_set<eastl::string> &important_regs, DataBlock &resultNodes, Tab<int> &nodesExecutionOrder,
  eastl::vector<eastl::string> &regs_for_scratch_pad, int scratch_pad_size, bool is_optimization_available)
{
  if (!is_optimization_available)
  {
    regs_for_scratch_pad.clear();
    resultNodes = nodes;
    nodesExecutionOrder.resize(resultNodes.blockCount());
    for (int i = 0; i < nodesExecutionOrder.size(); ++i)
      nodesExecutionOrder[i] = i;
    return;
  }

  eastl::hash_map<eastl::string, int> regNamesMap;
  eastl::vector<eastl::string> regNames;
  eastl::vector<unsigned> regSize;
  eastl::vector<eastl::vector<int>> controlNodesRegs[TSHADER_REG_TYPE_CNT];
  const int nodesCount = nodes.blockCount();

  controlNodesRegs[TSHADER_REG_TYPE_INPUT].resize(nodesCount);
  controlNodesRegs[TSHADER_REG_TYPE_OUTPUT].resize(nodesCount);

  const int outputNameId = nodes.getNameId(get_tgen_type_name(TSHADER_REG_TYPE_OUTPUT));
  const int inputNameId = nodes.getNameId(get_tgen_type_name(TSHADER_REG_TYPE_INPUT));
  const int particlesNameId = nodes.getNameId(PARTICLES);

  for (int ni = 0; ni < nodesCount; ++ni)
  {
    const DataBlock &node = *nodes.getBlock(ni);
    for (int bi = 0; bi < node.blockCount(); ++bi)
    {
      const DataBlock &reg = *node.getBlock(bi);
      const int nameId = reg.getBlockNameId();
      if (nameId != outputNameId && nameId != inputNameId && nameId != particlesNameId)
        continue;
      TShaderRegType type = (nameId == outputNameId) ? TSHADER_REG_TYPE_OUTPUT : TSHADER_REG_TYPE_INPUT;

      const char *name = reg.getStr("reg", "null");
      if (stricmp(name, "null") == 0)
        continue;

      int regNo = regNames.size();
      auto it = regNamesMap.find_as(name);

      if (it == regNamesMap.end())
      {
        regNames.push_back(name);
        regNamesMap[name] = regNo;
      }
      else
        regNo = it->second;

      controlNodesRegs[type][ni].push_back(regNo);

      if (type == TSHADER_REG_TYPE_OUTPUT)
      {
        if (regSize.size() <= regNo)
        {
          const int cnt = regSize.size();
          regSize.resize(regNo + 1);
          memset(&regSize[cnt], 0, (regNo + 1 - cnt) * sizeof(regSize[cnt]));
        }

        int nodeW = node.getInt("width", 1);
        int nodeH = node.getInt("height", 1);
        uint32_t pixels = nodeW * nodeH;

        const char *formatStr = reg.getStr("fmt", "");
        int formatSize = !strcmp(formatStr, "particles")
                           ? PARTICLE_SIZE
                           : get_tex_format_desc(parse_tex_format(formatStr, TEXFMT_A32B32G32R32F)).bytesPerElement;

        regSize[regNo] = pixels * formatSize; // somehow using actual size result in less optimal selection
      }
    }
  }

  Bitarray dirtyNodes;
  dirtyNodes.resize(nodesCount);
  dirtyNodes.reset();

  {
    eastl::vector<eastl::vector<int>> nodesUsingReg;
    eastl::vector<eastl::vector_set<int>> nodesProducedByCurrent;
    get_nodes_produced_by_current(controlNodesRegs, nodesUsingReg, nodesProducedByCurrent);
    // mark nodes as dirty
    for (eastl::vector<int> workingSet = initial_dirty_nodes; workingSet.size();)
    {
      const int ni = workingSet.back();
      workingSet.pop_back();
      if (dirtyNodes[ni])
        continue;
      dirtyNodes.set(ni, 1);
      for (auto i : nodesProducedByCurrent[ni])
        if (!dirtyNodes[i])
          workingSet.push_back(i);
    }
    debug("dirty nodes %d out of %d", dirtyNodes.numberset(), nodesCount);

    eastl::vector<int> nodeProducingReg;
    eastl::vector<eastl::vector_set<int>> nodesProducingCurrent;
    get_nodes_producing_current(controlNodesRegs, nodeProducingReg, nodesProducingCurrent);

    if (regs_for_scratch_pad.size() < scratch_pad_size)
    {
      for (const auto &ir : important_regs)
      {
        auto regIt = regNamesMap.find(ir);
        if (regIt == regNamesMap.end() || regIt->second < 0 || regIt->second >= nodeProducingReg.size())
          continue;

        int ni = nodeProducingReg[regIt->second];
        for (auto reg : controlNodesRegs[TSHADER_REG_TYPE_INPUT][ni])
        {
          regs_for_scratch_pad.push_back(regNames[reg]);
          if (regs_for_scratch_pad.size() >= scratch_pad_size)
            break;
        }

        if (regs_for_scratch_pad.size() >= scratch_pad_size)
          break;
      }
    }

    // break edges for nodes which are producing already alive and not dirty nodes
    for (const auto &aliveReg : alive_regs)
    {
      auto reg = regNamesMap.find_as(aliveReg.c_str());
      if (reg == regNamesMap.end())
        continue;
      const int regNo = reg->second;
      const int nodeI = nodeProducingReg[regNo];
      if (nodeI < 0)
        continue;
      if (dirtyNodes[nodeI])
        continue;
      // ok, this node's reg is not dirty, and it's reg is already produced. we don't need this node for next nodes. break edges.
      remove_reg(controlNodesRegs[TSHADER_REG_TYPE_OUTPUT][nodeI], regNo);
      for (auto ni : nodesProducedByCurrent[nodeI])
        remove_reg(controlNodesRegs[TSHADER_REG_TYPE_INPUT][ni], regNo);
    }
  }

  eastl::vector<int> nodeProducingReg;
  eastl::vector<eastl::vector_set<int>> nodesProducingCurrent;
  get_nodes_producing_current(controlNodesRegs, nodeProducingReg, nodesProducingCurrent);

  eastl::vector<eastl::vector<int>> nodesUsingReg;
  eastl::vector<eastl::vector_set<int>> nodesProducedByCurrent;
  get_nodes_produced_by_current(controlNodesRegs, nodesUsingReg, nodesProducedByCurrent);

  eastl::hash_set<int> finalNodes;
  get_nodes_generating_regs(finalNodes, final_regs, regNamesMap, nodeProducingReg);

  eastl::hash_set<int> importantNodes;
  get_nodes_generating_regs(importantNodes, important_regs, regNamesMap, nodeProducingReg);


  remove_unchanged_nodes(finalNodes, dirtyNodes);
  remove_unchanged_nodes(importantNodes, dirtyNodes);

  // add back unchanged final nodes which final regs do not exist now
  {
    eastl::hash_set<eastl::string> newFinalRegs;
    for (const auto &fi : final_regs)
    {
      if (alive_regs.find_as(fi.c_str()) != alive_regs.end())
        continue;
      newFinalRegs.insert(fi);
    }
    get_nodes_generating_regs(finalNodes, newFinalRegs, regNamesMap, nodeProducingReg);
  }
  //--

  // dead code elimination
  //  mark used nodes
  Bitarray nodeUsed;
  nodeUsed.resize(nodesCount);
  nodeUsed.reset();

  dead_code_elimination(nodeUsed, finalNodes, nodesProducingCurrent);
  debug("undead nodes %d out of %d", nodeUsed.numberset(), nodesCount);
  //--

  if (regSize.size() < nodesUsingReg.size())
    regSize.resize(nodesUsingReg.size(), 0);

  for (int regNo = 0; regNo < nodesUsingReg.size(); ++regNo)
  {
    bool used = false;
    if (regNo >= nodesUsingReg.size())
      regSize[regNo] = 0;

    for (auto ni : nodesUsingReg[regNo])
    {
      if (nodeUsed[ni])
      {
        used = true;
        break;
      }
    }

    if (!used)
      regSize[regNo] = 0;
  }


  eastl::vector<unsigned> nodesSize;
  nodesSize.resize(nodesCount);
  for (auto &sz : nodesSize)
    sz = 0;

  for (int ni = 0; ni < controlNodesRegs[TSHADER_REG_TYPE_OUTPUT].size(); ++ni)
  {
    uint64_t memSize = 0;
    for (const auto &regNo : controlNodesRegs[TSHADER_REG_TYPE_OUTPUT][ni])
      if (regNo >= 0)
        memSize += regSize[regNo];

    nodesSize[ni] = max(uint32_t((memSize + PAGE_SIZE - 1) / PAGE_SIZE), uint32_t(1));
  }
  // prepare scratch pad
  /*
  if (regs_for_scratch_pad.size() < scratch_pad_size)
  {
    // fixme: scratch pad priority should rely on amount of regs that node produce, or just amount of time needed to generate node
    // itself; this implentation below will oscillate (although still speeds)
    eastl::vector<NodePriority> scratchNodePriority;
    node_cse_priority(scratchNodePriority, nodeUsed, nodesProducedByCurrent);
    Bitarray scratchNodes;
    scratchNodes.resize(nodesCount);
    scratchNodes.reset();
    for (const auto &n : scratchNodePriority)
      scratchNodes.set(n.ni);
    eastl::sort(scratchNodePriority.begin(), scratchNodePriority.end(), [](const auto &a, const auto &b) { return b.prio < a.prio; });

    for (const auto &n : scratchNodePriority)
    {
      const int ni = n.ni;
      for (auto reg : controlNodesRegs[TSHADER_REG_TYPE_OUTPUT][ni])
      {
        regs_for_scratch_pad.push_back(regNames[reg]);
        if (regs_for_scratch_pad.size() >= scratch_pad_size)
          break;
      }
      if (regs_for_scratch_pad.size() >= scratch_pad_size)
        break;
    }
  }
  */
  ///--prepare scratch


  // register pressure instruction scheduling
  nodesExecutionOrder.clear();
  Bitarray nodeGenerated;
  nodeGenerated.resize(nodesCount);
  nodeGenerated.reset();

  for (const auto &aliveReg : alive_regs)
  {
    auto reg = regNamesMap.find_as(aliveReg.c_str());
    if (reg == regNamesMap.end() || reg->second < 0 || reg->second >= nodeProducingReg.size())
      continue;
    const int nodeI = nodeProducingReg[reg->second];
    if (nodeI < 0)
      continue;
    if (dirtyNodes[nodeI])
      continue;
    nodeGenerated.set(nodeI, 1);
  }

  // first execute important nodes
  instruction_selection_appel(nodeGenerated, nodesExecutionOrder, nodeUsed, nodesSize, importantNodes, nodesProducingCurrent,
    nodesProducedByCurrent);
  debug("important nodes to generate %d out of %d", nodesExecutionOrder.size(), nodesCount);
  // now all other final nodes
  instruction_selection(nodeGenerated, nodesExecutionOrder, nodeUsed, nodesSize, finalNodes, nodesProducingCurrent,
    nodesProducedByCurrent);

  debug("nodes to generate %d out of %d", nodesExecutionOrder.size(), nodesCount);

  /*
  Bitarray validateGeneration;
  validateGeneration.resize(nodesCount);
  validateGeneration.reset();
  for (auto nodeI : nodesExecutionOrder)
  {
    for (auto producingNode : nodesProducingCurrent[nodeI])
    {
      G_ASSERTF(validateGeneration[producingNode] || alive_regs.find_as() != alive_regs.end(),
        "node %d <%s> requires %d <%s> to be generated", nodes.getBlock(nodeI)->getBlockName(), nodeI,
        nodes.getBlock(producingNode)->getBlockName(), producingNode);
    }
    validateGeneration.set(nodeI, 1);
  }
  */

  resultNodes = nodes;
}

void TextureGenerator::calcRegCount(const DataBlock &node, eastl::hash_map<eastl::string, int> &regCnt)
{
  const int inputNameId = node.getNameId(get_tgen_type_name(TSHADER_REG_TYPE_INPUT));

  for (int bi = 0; bi < node.blockCount(); ++bi)
  {
    if (node.getBlock(bi)->getBlockNameId() != inputNameId)
      continue;
    const DataBlock &reg = *node.getBlock(bi);
    const char *name = reg.getStr("reg", "null");
    if (stricmp(name, "null") != 0)
    {
      auto regsUsed = regCnt.find_as(name);
      if (regsUsed == regCnt.end())
        regCnt[name] = 1;
      else
        regsUsed->second++;
    }
  }

  const DataBlock *particlesBlock = node.getBlockByName(PARTICLES);
  const char *particlesRegName = particlesBlock ? particlesBlock->getStr("reg", NULL) : 0;
  if (particlesRegName)
  {
    auto regsUsed = regCnt.find_as(particlesRegName);
    if (regsUsed == regCnt.end())
      regCnt[particlesRegName] = 1;
    else
      regsUsed->second++;
  }
}

// this function generate mapping of producing_node_name.NoofOutput
static void regs_name_mapping(const DataBlock &nodes, eastl::hash_map<eastl::string, eastl::string> &map)
{
  const int outputNameId = nodes.getNameId(get_tgen_type_name(TSHADER_REG_TYPE_OUTPUT));
  String str;
  for (int ni = 0; ni < nodes.blockCount(); ++ni)
  {
    const DataBlock &node = *nodes.getBlock(ni);
    const char *blockName = node.getBlockName();
    for (int oi = 0, bi = node.findBlock(outputNameId); bi != -1; bi = node.findBlock(outputNameId, bi))
    {
      const char *regName = node.getBlock(bi)->getStr("reg", 0);
      if (regName)
      {
        str.printf(64, "%s.%d", blockName, oi++);
        map[regName] = str.str();
      }
    }
  }
}

void TextureGenerator::startProcessNodes(const DataBlock &in_nodes, TextureRegManager &regs)
{
  finalRegistersRenamed.clear();
  eastl::hash_map<eastl::string, eastl::string> regMap;
  regs_name_mapping(in_nodes, regMap);

  eastl::hash_set<eastl::string> importantRegs;
  eastl::hash_set<eastl::string> finalRegs;
  eastl::hash_set<eastl::string> aliveRegs;
  const int finalNameId = in_nodes.getNameId("final");
  const int selectedNameId = in_nodes.getNameId("selected");

  for (int i = 0; i < in_nodes.paramCount(); ++i)
  {
    if (in_nodes.getParamType(i) != DataBlock::TYPE_STRING)
      continue;
    if (in_nodes.getParamNameId(i) == finalNameId || in_nodes.getParamNameId(i) == selectedNameId)
    {
      auto &set = in_nodes.getParamNameId(i) == finalNameId ? finalRegs : importantRegs;
      const char *finalName = in_nodes.getStr(i);
      auto it = regMap.find_as(finalName);

      if (it != regMap.end())
      {
        if (in_nodes.getParamNameId(i) == finalNameId)
          finalRegistersRenamed[it->first] = it->second;

        set.insert(it->second);
      }
      else
        set.insert(finalName); // not found in output nodes
    }
  }

  {
    eastl::hash_map<eastl::string, int> regsAlive = regs.getRegsAliveMap();
    for (const auto &i : regsAlive)
      aliveRegs.insert(i.first);
  }

  eastl::hash_set<eastl::string> allProducedRegs;
  for (const auto &reg : regMap)
    allProducedRegs.insert(reg.second);

  for (auto aliveRegIt = aliveRegs.begin(); aliveRegIt != aliveRegs.end();)
  {
    if (allProducedRegs.find(*aliveRegIt) == allProducedRegs.end())
    {
      int regNo = regs.getRegNo(aliveRegIt->c_str());
      if (regNo >= 0)
      {
        regs.setUseCount(regNo, 0);
        logger->log(LOGLEVEL_REMARK, String(128, "remove unused reg %s", aliveRegIt->c_str()));
        aliveRegIt = aliveRegs.erase(aliveRegIt);
        continue;
      }
    }
    aliveRegIt++;
  }

  // DataBlock nodes = in_nodes;
  DataBlock nodes;

  // rename registers
  const int outputNameId = in_nodes.getNameId(get_tgen_type_name(TSHADER_REG_TYPE_OUTPUT));
  const int inputNameId = in_nodes.getNameId(get_tgen_type_name(TSHADER_REG_TYPE_INPUT));
  const int particlesNameId = in_nodes.getNameId(PARTICLES);

  for (int i = 0; i < in_nodes.blockCount(); ++i)
  {
    const DataBlock &in_node = *in_nodes.getBlock(i);
    DataBlock &node = *nodes.addNewBlock(in_node.getBlockName());
    node.setParamsFrom(&in_node);

    for (int bi = 0; bi < in_node.blockCount(); ++bi)
    {
      const DataBlock &inReg = *in_node.getBlock(bi);
      const int nameId = inReg.getBlockNameId();
      DataBlock *newReg = node.addNewBlock(&inReg);
      if (nameId != outputNameId && nameId != inputNameId && nameId != particlesNameId)
        continue;

      const char *regName = inReg.getStr("reg", "null");
      auto it = regMap.find_as(regName);
      if (it != regMap.end())
        newReg->setStr("reg", it->second.c_str());
    }
  }
  // nodes.saveToTextFile("remapped.blk");

  eastl::vector<int> dirtyNodeLeaves;
  eastl::hash_map<eastl::string, uint64_t> newNodesData;

  const int nodeInputNameId = nodes.getNameId(get_tgen_type_name(TSHADER_REG_TYPE_INPUT));

  for (int ni = 0; ni < nodes.blockCount(); ++ni)
  {
    const DataBlock &node = *nodes.getBlock(ni);
    IHashCalc hashCalcer;
    node.saveToStream(hashCalcer);
    newNodesData[node.getBlockName()] = hashCalcer.hash;
    auto it = nodesData.find_as(node.getBlockName());
    if (it == nodesData.end() || it->second != hashCalcer.hash) // node had changed in it's params
    {
      dirtyNodeLeaves.push_back(ni);
    }
    else if (changedShaders.find_as(node.getStr(SHADER, "")) != changedShaders.end()) // node's shader had changed
    {
      dirtyNodeLeaves.push_back(ni);
    }
    else
    {
      for (int bi = node.findBlock(nodeInputNameId); bi != -1; bi = node.findBlock(nodeInputNameId, bi))
      {
        const DataBlock &inReg = *node.getBlock(bi);
        const char *regName = inReg.getStr("reg", "null");
        if (strstr(regName, "tex:") != regName)
          continue;
        if (regs.texHasChanged(regName))
        {
          dirtyNodeLeaves.push_back(ni);
          break;
        }
      }
    }
  }
  changedShaders.clear();
  nodesData = newNodesData;
  debug("there are %d dirty leaves", dirtyNodeLeaves.size());

  eastl::vector<eastl::string> regsForScratchPad;
  const int scratchPadSize = max((int)finalRegs.size(), (int)16);
  optimize(nodes, dirtyNodeLeaves, aliveRegs, finalRegs, importantRegs, allNodesRemapped, nodesExecutionOrder, regsForScratchPad,
    scratchPadSize, !optimizer_failure);

#if FALLBACK_TO_NO_OPTIMIZATION_ON_FAILURE
  gen_node_call_count = 0;
  if (optimizer_failed_in_this_update)
  {
    logerr("Optimizer failure. Fallback to 'no optimization'.");
    optimizer_failed_in_this_update = false;
    optimizer_failure = true;
    optimize(nodes, dirtyNodeLeaves, aliveRegs, finalRegs, importantRegs, allNodesRemapped, nodesExecutionOrder, regsForScratchPad,
      scratchPadSize, !optimizer_failure);
  }
#endif

  for (int i = 0; i < getNodeCount(); ++i)
    calcRegCount(getNode(i), regUsageCount);

  for (const auto &reg : regsForScratchPad) // infinite scratch pad!
  {
    auto regCnt = regUsageCount.find(reg);
    if (regCnt != regUsageCount.end())
      regCnt->second++;
  }

  /*
  DataBlock debugNodes;
  for (int i = 0; i < getNodeCount(); ++i)
    debugNodes.addNewBlock(&getNode(i));
  debugNodes.saveToTextFile("remappedToGen.blk");

  for (const auto &reg : allProducedRegs) // infinite scratch pad!
  {
    auto regCnt = regUsageCount.find(reg);
    if (regCnt != regUsageCount.end())
      regCnt->second++;
  }
  */

  for (const auto &fname : finalRegs)
  {
    const char *name = fname.c_str();
    auto regsUsed = regUsageCount.find_as(name);
    if (regsUsed == regUsageCount.end())
      regUsageCount[name] = 1;
    else
      regsUsed->second++;
  }
}

int TextureGenerator::start(const DataBlock &nodes, TextureRegManager &reg_manager)
{
  regUsageCount.clear();
  regManager = &reg_manager;
  startProcessNodes(nodes, reg_manager);
  cNode = 0;
  cNodeSubStep = 0;
  cNodeData = NodeData();
  return getNodeCount();
}

void TextureGenerator::stop()
{
  regManager = 0;
  cNode = -1;
}


int TextureGenerator::processNodes(unsigned count)
{
  if (cNode >= getNodeCount())
    return 0;

  if (cNode < 0 || !regManager)
  {
    logerr("not started!");
    return -1;
  }

  d3d::setvsrc(0, 0, 0); // can be called once
  TextureGenLogger *oldLogger = regManager->getLogger();
  regManager->setLogger(logger);

  int cnt = 0;

  for (int si = 0; si < count; ++si)
  {
    if (cNodeSubStep == 0)
    {
      if (cNode >= getNodeCount())
        break;

      const DataBlock &node = getNode(cNode);

      if (!startNode(cNodeData, node, regUsageCount, *regManager))
      {
        cnt = -1;
        break;
      }
      else
        cNodeStepsCount = cNodeData.shader->subSteps(cNodeData);
    }

    G_ASSERTF(cNodeSubStep < cNodeStepsCount, "%d < %d cNodeSubStep", cNodeSubStep, cNodeStepsCount);
    G_ASSERT(cNodeData.shader);

    if (!cNodeData.shader->process(*this, *regManager, cNodeData, cNodeSubStep))
    {
      logger->log(LOGLEVEL_ERR,
        String(128, "shader <%s> in node <%s> was unable to process", cNodeData.shader->getName(), getNode(cNode).getBlockName()));
      cnt = -1;
      break;
    }

    cnt++;
    cNodeSubStep++;

    if (cNodeSubStep == cNodeStepsCount)
    {
      endNode(cNodeData, *regManager);
      cNodeData = NodeData();
      cNodeSubStep = 0;
      cNode++;
    }
  }

  /*
  for (int i = cNode; i < allNodes.blockCount() && i < cNode + count; ++i)
  {
    const DataBlock &node = *allNodes.getBlock(cNode);
    if (!processNode(node, 0, regUsageCount, *regManager))
    {
      cnt = -1;
      break;
    }
    cnt++;
    cNode++;
  }
  */

  regManager->setLogger(oldLogger);
  d3d::set_program(BAD_PROGRAM);

  if (cnt < 0)
    cNode = -1;

  return cnt;
}

bool TextureGenerator::startNode(NodeData &data, const DataBlock &node, eastl::hash_map<eastl::string, int> &regCnt,
  TextureRegManager &reg_manager)
{
  auto shaderUsed = shaders.find_as(node.getStr(SHADER, ""));
  const char *nodeName = node.getBlockName();
  if (shaderUsed == shaders.end())
  {
    logger->log(LOGLEVEL_ERR, String(128, "shader <%s> used in node <%s> not found", node.getStr(SHADER, ""), nodeName));
    return false;
  }
  bool ret = true;
  data.shader = shaderUsed->second;
  // carray<Tab<D3dResource *>, TSHADER_REG_TYPE_CNT> regs;
  data.nodeW = node.getInt("width", -1);
  data.nodeH = node.getInt("height", data.nodeW);
  data.blending = parse_blending(node.getStr(BLENDING, "no"));
  data.params = *node.getBlockByNameEx("params");
  uint32_t autoFmt = 0;
  if (data.nodeW * data.nodeH <= 0)
  {
    logger->log(LOGLEVEL_ERR, String(128, "node <%s> of shader <%s> has invalid dimensions: %dx%d", data.nodeW, data.nodeH));
    return false;
  }


  for (TShaderRegType type = TSHADER_REG_TYPE_INPUT; type < TSHADER_REG_TYPE_CNT; type = (TShaderRegType)(type + 1))
  {
    const int typeNameId = node.getNameId(get_tgen_type_name(type));
    for (int bi = 0; bi < node.blockCount(); ++bi)
    {
      const DataBlock &reg = *node.getBlock(bi);
      if (reg.getBlockNameId() != typeNameId)
        continue;

      int regCount = (type == TSHADER_REG_TYPE_INPUT ? data.inputs.size() : data.outputs.size());
      if (data.shader->getRegCount(type) <= regCount)
      {
        logger->log(LOGLEVEL_ERR, String(128, "shader <%s> used in node <%s> has too many %ss %d (%d max)", data.shader->getName(),
                                    nodeName, get_tgen_type_name(type), regCount + 1, data.shader->getRegCount(type)));
        ret = false;
      }

      const char *name = reg.getStr("reg", "null");
      const char *wrapMode = reg.getStr("wrap_mode", "clamp");
      const char *fmtReg = reg.getStr("fmt", 0);
      bool wrap = !strcmp(wrapMode, "wrap");
      bool useUav = reg.getBool("is_uav", false);
      if (stricmp(name, "null") == 0)
      {
        if (!data.shader->isRegOptional(type, regCount))
        {
          logger->log(LOGLEVEL_ERR, String(128, "shader <%s> used in node <%s> has non-optional %s #%d", data.shader->getName(),
                                      nodeName, get_tgen_type_name(type), regCount));
          ret = false;
        }
        if (type == TSHADER_REG_TYPE_INPUT)
          data.inputs.push_back(TextureInput{NULL, wrap});
        else
          data.outputs.push_back(NULL);
      }
      else
      {
        if (type == TSHADER_REG_TYPE_INPUT)
        {
          BaseTexture *tex = 0;
          int treg = -1;
          if (strstr(name, "tex:") == name)
          {
            auto regsUsed = regCnt.find_as(name);
            if (regsUsed != regCnt.end())
            {
              treg = reg_manager.getRegNo(name);
              if (treg >= 0)
                reg_manager.setUseCount(treg, regsUsed->second);
              else
                treg = reg_manager.createTexture(name, 0, 0, 0, regsUsed->second);

              regCnt.erase(regsUsed);
            }
            else
              treg = reg_manager.getRegNo(name);
          }
          else
            treg = reg_manager.getRegNo(name);

          if (treg < 0)
          {
            logger->log(LOGLEVEL_ERR, String(128, "shader <%s> in node <%s> has invalid %s (non-existent) reg #%d (%s)",
                                        data.shader->getName(), nodeName, get_tgen_type_name(type), regCount, name));
            // ret = false;
          }

          {
            if (treg >= 0)
            {
              int usageLeft = reg_manager.getRegUsageLeft(treg);
              auto regsUsed = regCnt.find_as(name);
              if (regsUsed != regCnt.end())
              {
                if (usageLeft < regsUsed->second)
                  reg_manager.setUseCount(treg, regsUsed->second);
                regsUsed->second--;
              }
            }
            tex = reg_manager.getTexture(treg);
            data.inputs.push_back(TextureInput{tex, wrap});
            data.usedRegs.push_back(treg);
          }

          if (tex && (data.nodeW <= 0 || autoFmt == 0) && tex->restype() == RES3D_TEX)
          {
            TextureInfo tinfo;
            tex->getinfo(tinfo, 0);

            if (data.nodeW <= 0)
            {
              data.nodeW = tinfo.w;
              data.nodeH = tinfo.h;
            }

            if (autoFmt == 0)
              autoFmt = tinfo.cflg & TEXFMT_MASK;
          }
        }
        else // TSHADER_REG_TYPE_OUTPUT
        {
          auto regsUsed = regCnt.find_as(name);
          int useCount = 1;
          if (regsUsed == regCnt.end())
          {
            data.outputs.push_back(NULL);
            int regNo = reg_manager.getRegNo(name);
            int usageLeft = regNo >= 0 ? reg_manager.getRegUsageLeft(regNo) : 0;
            logger->log(usageLeft == 0 ? LOGLEVEL_WARN : LOGLEVEL_REMARK,
              String(128, "unknown register <%s> usage count in node <%s>!", name, nodeName));
            continue;
            // ret = false;
          }
          else
            useCount = regsUsed->second;

          if (useCount < 1)
          {
            logger->log(LOGLEVEL_ERR, String(128, "shader <%s> in node <%s> %s texture reg #%d has no usage", data.shader->getName(),
                                        nodeName, get_tgen_type_name(type), regCount));
            data.outputs.push_back(NULL);
            continue;
          }

          int treg = reg_manager.getRegNo(name);

          if (data.nodeW <= 0 || data.nodeH <= 0)
          {
            if (treg < 0 || reg_manager.getRegUsageLeft(treg) == 0 || reg_manager.getTexture(treg) == NULL)
            {
              logger->log(LOGLEVEL_ERR,
                String(128, "shader <%s> in node <%s> can't create %s texture reg #%d with invalid dimensions %dx%d",
                  data.shader->getName(), nodeName, get_tgen_type_name(type), regCount, data.nodeW, data.nodeH));
              ret = false;
            }
            else
            {
              BaseTexture *tex = reg_manager.getTexture(treg);
              TextureInfo tinfo;
              tex->getinfo(tinfo, 0);
              data.nodeW = tinfo.w;
              data.nodeH = tinfo.h;

              if (autoFmt == 0)
                autoFmt = tinfo.cflg & TEXFMT_MASK;
            }
          }

          if (data.nodeW > 0 && data.nodeH > 0)
          {
            const char *fmtStr = fmtReg ? fmtReg : "";
            uint32_t fmt = 0;
            if (treg >= 0 && reg_manager.getRegUsageLeft(treg) > 0)
            {
              bool shouldBeKilled = strcmp(fmtStr, PARTICLES) == 0;

              if (!shouldBeKilled)
              {
                D3dResource *tex = reg_manager.getResource(treg);
                if (tex && tex->restype() != RES3D_TEX)
                  shouldBeKilled = true;
                else if (tex)
                {
                  TextureInfo tinfo;
                  ((BaseTexture *)tex)->getinfo(tinfo, 0);
                  if (
                    tinfo.w != data.nodeW || tinfo.h != data.nodeH || parse_tex_format(fmtStr, autoFmt) != (tinfo.cflg & TEXFMT_MASK))
                    shouldBeKilled = true;
                }
              }

              if (shouldBeKilled)
                debug("killing reg <%s> due to it has changed", name);

              while (shouldBeKilled && reg_manager.getRegUsageLeft(treg) > 0)
                reg_manager.textureUsed(treg);

              if (shouldBeKilled)
                treg = -1;
            }

            if (treg < 0 || reg_manager.getRegUsageLeft(treg) == 0)
            {
              if (strcmp(fmtStr, PARTICLES) == 0)
              {
                // should be *4, if ALL particles are rendered four times. We just assume it is not happening
                const int maxCount = data.nodeW * data.nodeH * 2;
                treg = reg_manager.createParticlesBuffer(name, node.getInt(String(128, "%s_particles", name), maxCount), useCount);
                // debug("createParticlesBuffer = %s", name);
              }
              else
              {
                fmt = parse_tex_format(fmtStr, autoFmt);
                fmt |= useUav ? TEXCF_UNORDERED : 0;
                treg = reg_manager.createTexture(name, fmt, data.nodeW, data.nodeH, useCount);

                const DataBlock *particlesBlock = node.getBlockByName(PARTICLES);
                const char *particlesRegName = particlesBlock ? particlesBlock->getStr("reg", NULL) : 0;
                if (particlesRegName) // initialize to zero if it has particles, and it is not output
                {
                  BaseTexture *tex = reg_manager.getTexture(treg);
                  if (tex)
                  {
                    SCOPE_RENDER_TARGET;
                    d3d::set_render_target((Texture *)tex, 0);
                    d3d::clearview(CLEAR_TARGET, 0, 0, 0);
                  }
                }
              }
            }
            else
              reg_manager.setUseCount(treg, useCount);

            if (treg < 0)
            {
              logger->log(LOGLEVEL_ERR, String(128, "shader <%s> in node <%s> can't create %s texture reg #%d", data.shader->getName(),
                                          nodeName, get_tgen_type_name(type), regCount));
              ret = false;
            }
            else
            {
              D3dResource *output = reg_manager.getResource(treg);
              if (output && output->restype() == RES3D_TEX)
              {
                ((Texture *)output)->texaddr(wrap ? TEXADDR_WRAP : TEXADDR_CLAMP);
                const DataBlock *particlesBlock = node.getBlockByName(PARTICLES);
                const char *particlesRegName = particlesBlock ? particlesBlock->getStr("reg", NULL) : 0;
                if (particlesRegName) // initialize to zero if it has particles scatter
                {
                  BaseTexture *tex = reg_manager.getTexture(treg);
                  if (tex)
                  {
                    SCOPE_RENDER_TARGET;
                    d3d::set_render_target((Texture *)((Texture *)output), 0);
                    d3d::clearview(CLEAR_TARGET, 0, 0, 0);
                  }
                }
              }
              data.outputs.push_back(output);
            }
          }
        }
      }
    }
  }

  const DataBlock *particlesBlock = node.getBlockByName(PARTICLES);
  const char *particlesRegName = particlesBlock ? particlesBlock->getStr("reg", NULL) : 0;
  if (particlesRegName)
  {
    int preg = reg_manager.getRegNo(particlesRegName);
    if (preg < 0)
    {
      logger->log(LOGLEVEL_ERR, String(128, "Particles buffer <%s> is not available in node <%s>", particlesRegName, nodeName));
      ret = false;
    }
    else
    {
      data.particles = reg_manager.getParticlesBuffer(preg);
      if (!data.particles)
      {
        logger->log(LOGLEVEL_ERR, String(128, "reg <%s> is not a buffer in node <%s>", particlesRegName, nodeName));
        ret = false;
      }
      data.usedRegs.push_back(preg);
    }
  }

  if (ret)
  {
    for (int i = 0; i < data.inputs.size(); ++i)
      if (data.inputs[i].tex && data.inputs[i].tex->restype() == RES3D_TEX)
        ((Texture *)data.inputs[i].tex)->texaddr(data.inputs[i].wrap ? TEXADDR_WRAP : TEXADDR_CLAMP);
  }

  data.use_depth = data.blending == DEPTH_TEST || data.blending == NO_BLENDING || data.blending == ALPHA_BLENDING;
  //|| node.getBool("depth", false)

  return ret;
}

void TextureGenerator::endNode(NodeData &data, TextureRegManager &reg_manager)
{
  for (int i = 0; i < data.usedRegs.size(); ++i)
    reg_manager.textureUsed(data.usedRegs[i]);
}

bool TextureGenerator::processNode(const DataBlock &node, int subStepI, eastl::hash_map<eastl::string, int> &regCnt,
  TextureRegManager &reg_manager)
{
  if (subStepI == 0)
  {
    if (!startNode(cNodeData, node, regCnt, reg_manager))
      return false;
    else
      cNodeStepsCount = cNodeData.shader->subSteps(cNodeData);
  }

  G_ASSERT(subStepI < cNodeStepsCount);

  if (!cNodeData.shader->processAll(*this, reg_manager, cNodeData))
  {
    logger->log(LOGLEVEL_ERR,
      String(128, "shader <%s> in node <%s> was unable to process", cNodeData.shader->getName(), node.getBlockName()));
    return false;
  }

  if (subStepI == cNodeStepsCount - 1)
  {
    endNode(cNodeData, reg_manager);
    cNodeData = NodeData();
  }

  return true;
}

uint32_t extend_tex_fmt_to_32f(const BaseTexture *texture)
{
  Texture *tex = (Texture *)texture;
  TextureInfo texInfo;
  tex->getinfo(texInfo, 0);

  switch (texInfo.cflg & TEXFMT_MASK)
  {
    case TEXFMT_R16F:
    case TEXFMT_R32F:
    case TEXFMT_L16:
    case TEXFMT_L8: return TEXFMT_R32F;

    case TEXFMT_G16R16:
    case TEXFMT_G16R16F:
    case TEXFMT_G32R32F: return TEXFMT_G32R32F;

    case TEXFMT_A8R8G8B8:
    case TEXFMT_A16B16G16R16F:
    case TEXFMT_A16B16G16R16:
    case TEXFMT_A32B32G32R32F:
    case TEXFMT_R11G11B10F: return TEXFMT_A32B32G32R32F;

    default: return TEXFMT_A32B32G32R32F;
  }
}


static BlendingType parse_blending(const char *fmt)
{
  if (!fmt || strlen(fmt) <= 1)
    return NO_BLENDING;
  if (stricmp(fmt, "no") == 0)
    return NO_BLENDING;
  if (stricmp(fmt, "max") == 0)
    return MAX_BLENDING;
  if (stricmp(fmt, "min") == 0)
    return MIN_BLENDING;
  if (stricmp(fmt, "add") == 0)
    return ADD_BLENDING;
  if (stricmp(fmt, "mul") == 0)
    return MUL_BLENDING;
  if (stricmp(fmt, "alpha") == 0)
    return ALPHA_BLENDING;
  if (stricmp(fmt, "depth") == 0)
    return DEPTH_TEST;
  logwarn("unknown blending %s", fmt);
  return NO_BLENDING;
}


TextureGenerator *create_texture_generator() { return new TextureGenerator; }

TextureGenShader *texgen_get_shader(TextureGenerator *s, const char *name)
{
  if (!s)
    return NULL;
  return s->getShader(name);
}

bool texgen_add_shader(TextureGenerator *s, const char *name, TextureGenShader *shader)
{
  if (!s)
    return false;
  return s->addShader(name, shader);
}

int texgen_start_process_commands(TextureGenerator &s, const DataBlock &commands, TextureRegManager &regs)
{
  return s.start(commands, regs);
}

int texgen_process_commands(TextureGenerator &s, const unsigned count) { return s.processNodes(count); }

void texgen_stop_process_commands(TextureGenerator &s) { return s.stop(); }

void delete_texture_generator(TextureGenerator *&gen) { del_it(gen); }

void texgen_set_logger(TextureGenerator *s, TextureGenLogger *logger) { s->setLogger(logger); }

TextureGenLogger *texgen_get_logger(TextureGenerator *s) { return s->getLogger(); }

extern const char *texgen_get_final_reg_name(TextureGenerator *s, const char *name) { return s ? s->getFinalRegName(name) : name; }
