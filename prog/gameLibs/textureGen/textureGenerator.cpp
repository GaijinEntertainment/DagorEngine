// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <generic/dag_carray.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_driver.h>
#include <util/dag_string.h>
#include <math/dag_TMatrix.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IPoint4.h>
#include <math/dag_e3dColor.h>

#include <textureGen/textureDataCache.h>
#include <textureGen/textureRegManager.h>
#include <textureGen/textureGenShader.h>
#include <textureGen/textureGenerator.h>
#include <textureGen/textureGenLogger.h>
#include <util/dag_hash.h>
#include <EASTL/algorithm.h>
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
static bool optimizer_failed_in_this_update = false;
#endif

static void hash_str(uint64_t &h, const char *s) { h = fnv1_step<64>(0, str_hash_fnv1<64>(s, h)); }

template <class T>
static void hash_val(uint64_t &h, const T &v)
{
  h = mem_hash_fnv1<64>((const char *)&v, sizeof(v), h);
}

static void hash_block_content(uint64_t &h, const DataBlock &blk, int skip_param_name_id = -1)
{
  hash_val(h, blk.paramCount());
  for (uint32_t i = 0; i < blk.paramCount(); ++i)
  {
    if (blk.getParamNameId(i) == skip_param_name_id)
      continue;
    hash_str(h, blk.getParamName(i));
    const int type = blk.getParamType(i);
    hash_val(h, type);
    switch (type)
    {
      case DataBlock::TYPE_STRING: hash_str(h, blk.getStr(i)); break;
      case DataBlock::TYPE_INT: hash_val(h, blk.getInt(i)); break;
      case DataBlock::TYPE_REAL: hash_val(h, blk.getReal(i)); break;
      case DataBlock::TYPE_POINT2: hash_val(h, blk.getPoint2(i)); break;
      case DataBlock::TYPE_POINT3: hash_val(h, blk.getPoint3(i)); break;
      case DataBlock::TYPE_POINT4: hash_val(h, blk.getPoint4(i)); break;
      case DataBlock::TYPE_IPOINT2: hash_val(h, blk.getIPoint2(i)); break;
      case DataBlock::TYPE_IPOINT3: hash_val(h, blk.getIPoint3(i)); break;
      case DataBlock::TYPE_IPOINT4: hash_val(h, blk.getIPoint4(i)); break;
      case DataBlock::TYPE_BOOL: hash_val(h, (uint8_t)blk.getBool(i)); break;
      case DataBlock::TYPE_E3DCOLOR: hash_val(h, blk.getE3dcolor(i).u); break;
      case DataBlock::TYPE_MATRIX: hash_val(h, blk.getTm(i)); break;
      case DataBlock::TYPE_INT64: hash_val(h, blk.getInt64(i)); break;
      default: G_ASSERT(0); break;
    }
  }
  hash_val(h, blk.blockCount());
  for (uint32_t i = 0; i < blk.blockCount(); ++i)
  {
    const DataBlock &sub = *blk.getBlock(i);
    hash_str(h, sub.getBlockName());
    hash_block_content(h, sub, skip_param_name_id);
  }
}

uint64_t texgen_hash_block_content(const DataBlock &blk)
{
  uint64_t h = FNV1Params<64>::offset_basis;
  hash_block_content(h, blk);
  return h;
}

// Leaves out register names: nodes that differ only in them do the same GPU work.
static uint64_t node_cost_key(const DataBlock &node)
{
  uint64_t h = FNV1Params<64>::offset_basis;
  hash_block_content(h, node, node.getNameId("reg"));
  return h;
}


struct NodeData : TextureGenNodeData
{
  TextureGenShader *shader = 0;
  eastl::vector<int> finalRegs;
};

struct OrderSearchState
{
  static constexpr int ROUND_EVALS = 2000;
  uint64_t key = 0;
  int evals = ROUND_EVALS;
  uint32_t rounds = 0;
  eastl::vector<int> order;
};

#define PARTICLES "particles"
#define SHADER    "shader"
#define BLENDING  "blending"
static constexpr int PARTICLES_PER_PIXEL = 2;

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
  bool peekNextCommand(TextureGenCommand &command);
  void getUpcomingTexInputs(int node_count, eastl::vector<const char *> &reg_names) const;
  int getDoneNodeCount() const { return cNode < 0 ? 0 : cNode; }
  const char *getCurrentNodeName() const { return cNode >= 0 && cNode < getNodeCount() ? getNode(cNode).getBlockName() : nullptr; }
  void getPassNodeNames(eastl::vector<const char *> &names) const
  {
    names.clear();
    for (int i = 0; i < getNodeCount(); ++i)
    {
      names.push_back(getNode(i).getBlockName());
    }
  }
  uint64_t getNodeKey(const char *name) const
  {
    const auto it = nodesData.find_as(name);
    return it != nodesData.end() ? it->second : 0;
  }
  int getLastCommandPixels() const { return lastCommandPixels; }

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

  void setFinalCallback(TextureGenFinalCallback cb, void *ctx)
  {
    finalCb = cb;
    finalCbCtx = ctx;
  }
  void setOrderSearchEvals(int evals) { orderSearch.evals = evals; }

protected:
  void startProcessNodes(const DataBlock &nodes, TextureRegManager &regs);
  void markUnrunNodesDirty(int first);
  bool processNode(const DataBlock &node, int si, eastl::hash_map<eastl::string, int> &regCnt, TextureRegManager &regs);
  void calcRegCount(const DataBlock &node, eastl::hash_map<eastl::string, int> &regCnt);
  void cancelUnfinishedNode(TextureRegManager &regs);
  eastl::hash_map<eastl::string, TextureGenShader *> shaders;
  TextureGenLogger *logger;
  eastl::hash_map<eastl::string, int> regUsageCount;
  int cNode = -1;
  int cNodeSubStep = 0;
  unsigned cNodeStepsCount = 1;
  int lastCommandPixels = 0;
  int costKeyNode = -1;
  uint64_t costKeyOfNode = 0;
  NodeData cNodeData;
  int getNodeCount() const { return nodesExecutionOrder.size(); }
  const DataBlock &getNode(int i) const { return *allNodesRemapped.getBlock(nodesExecutionOrder[i]); }
  Tab<int> nodesExecutionOrder;
  DataBlock allNodesRemapped;
  eastl::hash_map<eastl::string, eastl::string> finalRegistersRenamed;
  eastl::hash_map<eastl::string, eastl::string> finalNameOfReg;
  TextureGenFinalCallback finalCb = nullptr;
  void *finalCbCtx = nullptr;
  OrderSearchState orderSearch;
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
      logger->log(LOGLEVEL_REMARK, String(128, "tex gen shader name <%s> changed", name));
      // start() cancels an unfinished node through cNodeData.shader.
      if (cNodeData.shader == sh->second)
      {
        cNodeData.shader = nullptr;
      }
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
// need[] is computed once per node on the DAG; each walk uses an explicit stack, as graphs can be deep.
static void instruction_selection_appel(Bitarray &nodeGenerated, Tab<int> &nodesExecutionOrder,
  const Bitarray &nodeUsed, // alive nodes
  const eastl::vector<unsigned> &nodesSize, const eastl::hash_set<int> &finalNodes,
  const eastl::vector<eastl::vector_set<int>> &nodesProducingCurrent)
{
  const int nodesCount = nodeGenerated.size();
  auto isPending = [&](int ni) { return nodeUsed[ni] && !nodeGenerated[ni]; };

  // 0 until done, so a self-read is no child
  eastl::vector<int64_t> need(nodesCount, 0);
  auto byNeed = [&](int a, int b) {
    const int64_t ka = need[a] - int64_t(nodesSize[a]), kb = need[b] - int64_t(nodesSize[b]);
    return ka != kb ? ka > kb : a < b;
  };

  // Flat child lists: children of n are children[childrenStart[n] .. childrenEnd[n]), sorted by byNeed.
  eastl::vector<int> children, childrenStart(nodesCount, 0), childrenEnd(nodesCount, 0);
  Bitarray nodeVisiting;
  nodeVisiting.resize(nodesCount);

  struct Frame
  {
    int nodeI;
    int next;
  };
  eastl::vector<Frame> stack;
  eastl::vector<int> roots;
  for (int root : finalNodes)
  {
    if (!isPending(root))
      continue;
    roots.push_back(root);
    if (need[root])
      continue;
    stack.push_back({root, 0});
    nodeVisiting.set(root);
    while (!stack.empty())
    {
      const int ni = stack.back().nodeI;
      const eastl::vector_set<int> &inputs = nodesProducingCurrent[ni];
      if (stack.back().next < (int)inputs.size())
      {
        const int ch = *(inputs.begin() + stack.back().next++);
        if (!isPending(ch) || need[ch])
          continue;
        if (nodeVisiting[ch])
        {
#if FALLBACK_TO_NO_OPTIMIZATION_ON_FAILURE
          optimizer_failed_in_this_update = true;
#endif
          continue;
        }
        nodeVisiting.set(ch);
        stack.push_back({ch, 0});
        continue;
      }
      stack.pop_back();
      nodeVisiting.reset(ni);

      childrenStart[ni] = children.size();
      for (int ch : inputs)
        if (isPending(ch) && need[ch])
          children.push_back(ch);
      childrenEnd[ni] = children.size();
      if (childrenStart[ni] == childrenEnd[ni])
      {
        need[ni] = nodesSize[ni];
        continue;
      }
      const auto first = children.begin() + childrenStart[ni];
      eastl::sort(first, children.end(), byNeed);
      // A shared child counts in each parent's fold as if computed there, so on a DAG need is an estimate.
      int64_t m = 0;
      for (auto c = children.end(); c != first;)
      {
        --c;
        m = max(m + int64_t(nodesSize[*c]), need[*c]);
      }
      need[ni] = m;
    }
  }

  eastl::sort(roots.begin(), roots.end(), byNeed);
  for (int root : roots)
  {
    if (nodeGenerated[root])
      continue;
    stack.push_back({root, childrenStart[root]});
    while (!stack.empty())
    {
      Frame &f = stack.back();
      if (f.next < childrenEnd[f.nodeI])
      {
        const int ch = children[f.next++];
        if (!nodeGenerated[ch])
          stack.push_back({ch, childrenStart[ch]});
        continue;
      }
      nodeGenerated.set(f.nodeI);
      nodesExecutionOrder.push_back(f.nodeI);
      stack.pop_back();
    }
  }
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
    instruction_selection_appel(nodeGenerated, nodesExecutionOrder, nodeUsed, nodesSize, finalNode, nodesProducingCurrent);
    break;
  }

  instruction_selection_appel(nodeGenerated, nodesExecutionOrder, nodeUsed, nodesSize, finalNodes, nodesProducingCurrent);
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
}

struct PeakMemorySearch
{
  const eastl::vector<eastl::vector<int>> &inputs, &outputs, &readersOfReg;
  const eastl::vector<eastl::vector_set<int>> &producers, &consumers;
  const eastl::vector<uint64_t> &regBytes;
  // cand matches order outside the move's window
  // events: the registers each node reads or releases
  eastl::vector<int> usesInit, uses, usesStamp, regProducer, pos, order, cand, stack, markStamp, peakTree, eventStart, events;
  eastl::vector<uint64_t> trace, liveIn, windowTrace, windowLiveIn, nodeAlloc;
  eastl::vector<char> releasedAtProducer;
  int stamp = 0, markId = 0, winLo = 0, winHi = 0, treeBase = 1;
  struct Waiter
  {
    uint64_t weightEnd; // running sum of the weights
    int producerAt, readerAt;
  };
  eastl::vector<Waiter> waiters;
  struct Cost
  {
    uint64_t peak = 0, area = 0;
    int at = 0;
    bool operator<(const Cost &c) const { return peak != c.peak ? peak < c.peak : area < c.area; }
  };
  int split = 0; // no move crosses it
  uint32_t rng = 0;

  int segBegin(int k) const { return k < split ? 0 : split; }
  int segEnd(int k) const { return k < split ? split : (int)order.size(); }

  uint32_t rand()
  {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
  }
  int randRange(int lo, int hi) { return lo + int(rand() % uint32_t(hi - lo + 1)); }

  // uses of r left when cand[lo] starts
  int &usesAt(int r, int lo)
  {
    if (usesStamp[r] != stamp)
    {
      usesStamp[r] = stamp;
      int u = usesInit[r];
      for (int x : readersOfReg[r])
        u -= pos[x] >= 0 && pos[x] < lo;
      u -= releasedAtProducer[r] && regProducer[r] >= 0 && pos[regProducer[r]] < lo;
      uses[r] = u;
    }
    return uses[r];
  }

  int higherPeak(int a, int b) const { return b >= 0 && (a < 0 || trace[b] > trace[a] || (trace[b] == trace[a] && b < a)) ? b : a; }

  void buildPeakTree()
  {
    for (treeBase = 1; treeBase < trace.size();)
      treeBase *= 2;
    peakTree.assign(treeBase * 2, -1);
    for (int k = 0; k < trace.size(); ++k)
      peakTree[treeBase + k] = k;
    for (int i = treeBase - 1; i > 0; --i)
      peakTree[i] = higherPeak(peakTree[i * 2], peakTree[i * 2 + 1]);
  }

  void updatePeakTree(int lo, int hi)
  {
    for (lo = (lo + treeBase) / 2, hi = (hi + treeBase) / 2; lo > 0; lo /= 2, hi /= 2)
      for (int i = lo; i <= hi; ++i)
        peakTree[i] = higherPeak(peakTree[i * 2], peakTree[i * 2 + 1]);
  }

  // first one on ties, -1 when empty
  int peakIn(int lo, int hi) const
  {
    int at = -1;
    for (lo += treeBase, hi += treeBase + 1; lo < hi; lo /= 2, hi /= 2)
    {
      if (lo & 1)
        at = higherPeak(at, peakTree[lo++]);
      if (hi & 1)
        at = higherPeak(at, peakTree[--hi]);
    }
    return at;
  }

  uint64_t areaOf(uint64_t peak, int lo, int hi) const
  {
    const uint64_t high = peak - peak / 10;
    uint64_t area = 0;
    for (int k = 0; k < trace.size(); ++k)
    {
      const uint64_t v = k >= lo && k <= hi ? windowTrace[k - lo] : trace[k];
      area += v > high ? v - high : 0;
    }
    return area;
  }

  // simulates only cand[lo..hi]; no area above cur.peak
  Cost cost(int lo, int hi, const Cost &cur)
  {
    ++stamp;
    windowTrace.resize(hi - lo + 1);
    windowLiveIn.resize(hi - lo + 1);
    uint64_t live = liveIn[lo];
    for (int k = lo; k <= hi; ++k)
    {
      const int ni = cand[k];
      windowLiveIn[k - lo] = live;
      live += nodeAlloc[ni];
      windowTrace[k - lo] = live;
      for (int e = eventStart[ni]; e < eventStart[ni + 1]; ++e)
        if (--usesAt(events[e], lo) == 0)
          live -= regBytes[events[e]];
    }
    Cost c;
    if (const int left = peakIn(0, lo - 1); left >= 0)
      c.peak = trace[left], c.at = left;
    for (int k = lo; k <= hi; ++k)
      if (windowTrace[k - lo] > c.peak)
        c.peak = windowTrace[k - lo], c.at = k;
    if (const int right = peakIn(hi + 1, (int)trace.size() - 1); right >= 0 && trace[right] > c.peak)
      c.peak = trace[right], c.at = right;
    if (c.peak == cur.peak)
    {
      const uint64_t high = c.peak - c.peak / 10;
      c.area = cur.area;
      for (int k = lo; k <= hi; ++k)
        c.area = c.area + (windowTrace[k - lo] > high ? windowTrace[k - lo] - high : 0) - (trace[k] > high ? trace[k] - high : 0);
    }
    else if (c.peak < cur.peak)
      c.area = areaOf(c.peak, lo, hi);
    return c;
  }

  void rebuildTrace()
  {
    uses = usesInit;
    trace.resize(order.size());
    liveIn.resize(order.size());
    uint64_t live = 0;
    for (int k = 0; k < order.size(); ++k)
    {
      liveIn[k] = live;
      for (int r : outputs[order[k]])
        if (uses[r] > 0)
          live += regBytes[r];
      trace[k] = live;
      for (int r : inputs[order[k]])
        if (--uses[r] == 0)
          live -= regBytes[r];
      for (int r : outputs[order[k]])
        if (releasedAtProducer[r] && --uses[r] == 0)
          live -= regBytes[r];
    }
    usesStamp.assign(regBytes.size(), stamp);
  }

  void findWaiters(int at)
  {
    waiters.clear();
    uint64_t weightEnd = 0;
    for (int k = 0; k < at; ++k)
      for (int r : outputs[order[k]])
      {
        int next = INT_MAX;
        bool readAtPeak = false;
        for (int u : readersOfReg[r])
          if (pos[u] == at)
            readAtPeak = true;
          else if (pos[u] > at)
            next = min(next, pos[u]);
        if (regBytes[r] && next != INT_MAX && !readAtPeak)
        {
          weightEnd += regBytes[r] * uint64_t(next - at);
          waiters.push_back({weightEnd, k, next});
        }
      }
  }

  void accept(const Cost &c, int lo, int hi)
  {
    for (int k = lo; k <= hi; ++k)
    {
      order[k] = cand[k];
      pos[order[k]] = k;
      trace[k] = windowTrace[k - lo];
      liveIn[k] = windowLiveIn[k - lo];
    }
    updatePeakTree(lo, hi);
    findWaiters(c.at);
  }

  const Waiter &pickWaiter()
  {
    const uint64_t x = ((uint64_t(rand()) << 32) | rand()) % waiters.back().weightEnd;
    return *eastl::upper_bound(waiters.begin(), waiters.end(), x, [](uint64_t v, const Waiter &w) { return v < w.weightEnd; });
  }

  bool hoist(int reader_at, int to)
  {
    to = max(to, segBegin(reader_at));
    if (to >= reader_at)
      return false;
    ++markId;
    stack.assign(1, reader_at);
    markStamp[reader_at] = markId;
    while (!stack.empty())
    {
      const int k = stack.back();
      stack.pop_back();
      for (int p : producers[order[k]])
        if (pos[p] >= to && markStamp[pos[p]] != markId)
          markStamp[pos[p]] = markId, stack.push_back(pos[p]);
    }
    int at = to;
    for (int k = to; k <= reader_at; ++k)
      if (markStamp[k] == markId)
        cand[at++] = order[k];
    for (int k = to; k <= reader_at; ++k)
      if (markStamp[k] != markId)
        cand[at++] = order[k];
    winLo = to, winHi = reader_at;
    return true;
  }

  bool sink(int producer_at, int to)
  {
    to = min(to, segEnd(producer_at));
    ++markId;
    markStamp[producer_at] = markId;
    const int lowest = max(segBegin(producer_at), producer_at - 400);
    for (int k = producer_at - 1; k >= lowest; --k)
    {
      bool any = false, all = true;
      for (int u : consumers[order[k]])
        if (pos[u] >= 0)
          any = true, all &= markStamp[pos[u]] == markId;
      if (any && all)
        markStamp[k] = markId;
    }
    for (int k = lowest; k <= producer_at; ++k)
      if (markStamp[k] == markId)
        for (int u : consumers[order[k]])
          if (pos[u] >= 0 && markStamp[pos[u]] != markId)
            to = min(to, pos[u]);
    if (to <= producer_at)
      return false;
    winLo = lowest, winHi = to - 1;
    while (markStamp[winLo] != markId)
      ++winLo;
    int at = winLo;
    for (int k = winLo; k < to; ++k)
      if (markStamp[k] != markId)
        cand[at++] = order[k];
    for (int k = winLo; k < to; ++k)
      if (markStamp[k] == markId)
        cand[at++] = order[k];
    return true;
  }

  bool shift(int at)
  {
    const int k = clamp(at + randRange(-60, 60), 0, (int)order.size() - 1);
    const int v = order[k];
    int lo = segBegin(k), hi = segEnd(k) - 1;
    for (int p : producers[v])
      lo = max(lo, pos[p] + 1);
    for (int u : consumers[v])
      if (pos[u] >= 0)
        hi = min(hi, pos[u] - 1);
    if (hi <= lo)
      return false;
    const int to = randRange(lo, hi);
    if (to == k)
      return false;
    if (to < k)
      eastl::rotate(cand.begin() + to, cand.begin() + k, cand.begin() + k + 1);
    else
      eastl::rotate(cand.begin() + k, cand.begin() + k + 1, cand.begin() + to + 1);
    winLo = min(k, to), winHi = max(k, to);
    return true;
  }

  void run(Tab<int> &execution_order, int split_at, const eastl::vector<int> &held_regs, const eastl::vector<int> &released_regs,
    int max_evals, uint32_t round_no)
  {
    if (execution_order.size() < 2 || max_evals <= 0)
      return;
    split = split_at;
    rng = 0x9E3779B9u + round_no * 0x6D2B79F5u;
    pos.assign(inputs.size(), -1);
    usesInit.assign(regBytes.size(), 0);
    regProducer.assign(regBytes.size(), -1);
    releasedAtProducer.assign(regBytes.size(), 0);
    for (int ni : execution_order)
    {
      for (int r : inputs[ni])
        ++usesInit[r];
      for (int r : outputs[ni])
        regProducer[r] = ni;
    }
    for (int r : held_regs)
      ++usesInit[r];
    for (int r : released_regs)
      ++usesInit[r], releasedAtProducer[r] = 1;
    nodeAlloc.assign(inputs.size(), 0);
    eventStart.assign(inputs.size() + 1, 0);
    events.clear();
    for (int ni = 0; ni < inputs.size(); ++ni)
    {
      eventStart[ni] = events.size();
      events.insert(events.end(), inputs[ni].begin(), inputs[ni].end());
      for (int r : outputs[ni])
      {
        nodeAlloc[ni] += usesInit[r] > 0 ? regBytes[r] : 0;
        if (releasedAtProducer[r])
          events.push_back(r);
      }
    }
    eventStart[inputs.size()] = events.size();
    cand.assign(execution_order.begin(), execution_order.end());
    order = cand;
    markStamp.assign(order.size(), 0);
    for (int k = 0; k < order.size(); ++k)
      pos[order[k]] = k;
    rebuildTrace();
    buildPeakTree();
    Cost cur;
    cur.at = peakIn(0, (int)order.size() - 1);
    cur.peak = trace[cur.at];
    cur.area = areaOf(cur.peak, 1, 0);
    Cost best = cur;
    findWaiters(cur.at);
    eastl::vector<int> bestOrder = order;
    for (int evals = 0, tries = 0; evals < max_evals && tries < max_evals * 4; ++tries)
    {
      const uint32_t kind = rand() % 10;
      bool moved;
      if (kind < 8 && !waiters.empty())
      {
        const Waiter &w = pickWaiter();
        moved = kind < 4 ? hoist(w.readerAt, cur.at - randRange(0, 40)) : sink(w.producerAt, randRange(cur.at + 1, w.readerAt));
      }
      else
        moved = shift(cur.at);
      if (!moved)
        continue;
      ++evals;
      Cost c = cost(winLo, winHi, cur);
      const float temp = 0.004f * max(0.05f, 1.f - float(evals) / max_evals);
      if (c.peak <= cur.peak || rand() < expf(-float(c.peak - cur.peak) / (temp * float(cur.peak) + 1.f)) * float(UINT32_MAX))
      {
        const bool higher = c.peak > cur.peak;
        accept(c, winLo, winHi);
        if (higher)
          c.area = areaOf(c.peak, 1, 0);
        cur = c;
        if (c < best)
          best = c, bestOrder = order;
      }
      else
        eastl::copy(order.begin() + winLo, order.begin() + winHi + 1, cand.begin() + winLo);
    }
    execution_order.assign(bestOrder.begin(), bestOrder.end());
  }
};
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

static void remove_reg(eastl::vector<int> &regs, int regNo) { regs.erase(eastl::remove(regs.begin(), regs.end(), regNo), regs.end()); }

// Numbers the registers of nodes: regs[type][ni] lists the ones node ni reads (a particles block reads too) or writes.
// on_reg(reg_no, ni, reg) gets each register block of node ni that it numbers.
template <typename OnReg>
static void collect_node_regs(const DataBlock &nodes, eastl::vector<eastl::vector<int>> (&regs)[TSHADER_REG_TYPE_CNT],
  eastl::hash_map<eastl::string, int> &reg_names_map, eastl::vector<eastl::string> &reg_names, OnReg on_reg)
{
  const int nodesCount = nodes.blockCount();
  regs[TSHADER_REG_TYPE_INPUT].resize(nodesCount);
  regs[TSHADER_REG_TYPE_OUTPUT].resize(nodesCount);

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

      int regNo = reg_names.size();
      auto it = reg_names_map.find_as(name);

      if (it == reg_names_map.end())
      {
        reg_names.push_back(name);
        reg_names_map[name] = regNo;
      }
      else
        regNo = it->second;

      regs[type][ni].push_back(regNo);
      on_reg(regNo, ni, reg);
    }
  }
}

static constexpr int PAGE_SIZE = 4096; // any texture is no smaller than PAGE_SIZE

static void optimize(const DataBlock &nodes, const eastl::vector<int> &initial_dirty_nodes,
  const eastl::hash_set<eastl::string> &alive_regs, const eastl::hash_set<eastl::string> &final_regs,
  const eastl::hash_set<eastl::string> &important_regs, Tab<int> &nodesExecutionOrder,
  eastl::vector<eastl::string> &regs_for_scratch_pad, int scratch_pad_size, bool is_optimization_available, bool finals_held,
  OrderSearchState &search)
{
  if (!is_optimization_available)
  {
    regs_for_scratch_pad.clear();
    nodesExecutionOrder.resize(nodes.blockCount());
    for (int i = 0; i < nodesExecutionOrder.size(); ++i)
      nodesExecutionOrder[i] = i;
    return;
  }

  eastl::hash_map<eastl::string, int> regNamesMap;
  eastl::vector<eastl::string> regNames;
  eastl::vector<uint64_t> regSize;
  eastl::vector<eastl::vector<int>> controlNodesRegs[TSHADER_REG_TYPE_CNT];
  const int nodesCount = nodes.blockCount();
  const int outputNameId = nodes.getNameId(get_tgen_type_name(TSHADER_REG_TYPE_OUTPUT));
  const int inputNameId = nodes.getNameId(get_tgen_type_name(TSHADER_REG_TYPE_INPUT));
  // parse_tex_format scans every format name
  eastl::vector<eastl::pair<const char *, int>> formatSizes;
  eastl::vector<int> firstInputReg(nodesCount, -1);

  collect_node_regs(nodes, controlNodesRegs, regNamesMap, regNames, [&](int regNo, int ni, const DataBlock &reg) {
    if (reg.getBlockNameId() == inputNameId && firstInputReg[ni] < 0)
      firstInputReg[ni] = regNo;
    if (reg.getBlockNameId() != outputNameId)
      return;

    if (regSize.size() <= regNo)
      regSize.resize(regNo + 1, 0);

    const char *formatStr = reg.getStr("fmt", "");
    auto known = eastl::find_if(formatSizes.begin(), formatSizes.end(), [&](auto &f) { return !strcmp(f.first, formatStr); });
    if (known == formatSizes.end())
      known = &formatSizes.emplace_back(formatStr,
        !strcmp(formatStr, "particles") ? PARTICLES_PER_PIXEL * PARTICLE_SIZE
                                        : get_tex_format_desc(parse_tex_format(formatStr, TEXFMT_A32B32G32R32F)).bytesPerElement);

    regSize[regNo] = known->second;
  });

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

    eastl::vector<uint64_t> nodePixels(nodesCount, 0);
    eastl::vector<int> chain;
    for (int regNo = 0; regNo < nodeProducingReg.size(); ++regNo)
    {
      uint64_t pixels = 1;
      chain.clear();
      for (int cur = nodeProducingReg[regNo]; cur >= 0;)
      {
        if (nodePixels[cur])
        {
          pixels = nodePixels[cur];
          break;
        }
        chain.push_back(cur);
        nodePixels[cur] = 1;
        const DataBlock &node = *nodes.getBlock(cur);
        if (const int w = node.getInt("width", -1); w > 0)
        {
          pixels = uint64_t(w) * max(node.getInt("height", w), 1);
          break;
        }
        const int reg = firstInputReg[cur];
        cur = reg >= 0 && reg < nodeProducingReg.size() ? nodeProducingReg[reg] : -1;
      }
      for (int c : chain)
        nodePixels[c] = pixels;
      if (nodeProducingReg[regNo] >= 0)
        regSize[regNo] *= pixels;
    }

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

  regSize.resize(regNames.size(), 0);
  nodesUsingReg.resize(regNames.size());

  eastl::vector<unsigned> nodesSize(nodesCount);
  for (int ni = 0; ni < nodesCount; ++ni)
  {
    uint64_t memSize = 0;
    for (int regNo : controlNodesRegs[TSHADER_REG_TYPE_OUTPUT][ni])
      if (eastl::any_of(nodesUsingReg[regNo].begin(), nodesUsingReg[regNo].end(), [&](int i) { return nodeUsed[i]; }))
        memSize += regSize[regNo];
    nodesSize[ni] = max(uint32_t((memSize + PAGE_SIZE - 1) / PAGE_SIZE), uint32_t(1));
  }

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
  instruction_selection_appel(nodeGenerated, nodesExecutionOrder, nodeUsed, nodesSize, importantNodes, nodesProducingCurrent);
  const int importantCount = nodesExecutionOrder.size();
  debug("important nodes to generate %d out of %d", importantCount, nodesCount);
  // now all other final nodes
  instruction_selection(nodeGenerated, nodesExecutionOrder, nodeUsed, nodesSize, finalNodes, nodesProducingCurrent,
    nodesProducedByCurrent);

  // Held to the end of the pass, or released when the node that writes it ends.
  eastl::vector<int> heldRegs, releasedRegs;
  for (const eastl::string &name : final_regs)
    if (auto it = regNamesMap.find(name); it != regNamesMap.end())
      (finals_held ? heldRegs : releasedRegs).push_back(it->second);
  for (const eastl::string &name : regs_for_scratch_pad)
    if (auto it = regNamesMap.find(name); it != regNamesMap.end())
      heldRegs.push_back(it->second);
  // same graph as last pass: continue its order
  uint64_t key = 0;
  for (int ni : nodesExecutionOrder)
  {
    hash_val(key, ni);
    for (int r : controlNodesRegs[TSHADER_REG_TYPE_OUTPUT][ni])
      hash_val(key, regSize[r]);
    for (int r : controlNodesRegs[TSHADER_REG_TYPE_INPUT][ni])
      hash_val(key, r);
    for (int p : nodesProducingCurrent[ni])
      hash_val(key, p);
  }
  for (const eastl::vector<int> *regs : {&heldRegs, &releasedRegs})
  {
    hash_val(key, -1);
    for (int r : *regs)
      hash_val(key, r);
  }
  hash_val(key, importantCount);
  const bool goOn = key == search.key && search.order.size() == nodesExecutionOrder.size();
  if (goOn)
    nodesExecutionOrder.assign(search.order.begin(), search.order.end());
  else
    search.rounds = 0;
  PeakMemorySearch peakSearch{controlNodesRegs[TSHADER_REG_TYPE_INPUT], controlNodesRegs[TSHADER_REG_TYPE_OUTPUT], nodesUsingReg,
    nodesProducingCurrent, nodesProducedByCurrent, regSize};
  // a round never raises the best peak
  for (int left = search.evals; left > 0; left -= OrderSearchState::ROUND_EVALS)
    peakSearch.run(nodesExecutionOrder, importantCount, heldRegs, releasedRegs, min(left, OrderSearchState::ROUND_EVALS),
      search.rounds++);
  search.key = key;
  search.order.assign(nodesExecutionOrder.begin(), nodesExecutionOrder.end());

  debug("nodes to generate %d out of %d", nodesExecutionOrder.size(), nodesCount);
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

static const char *cache_node_record_id(const DataBlock &node)
{
  return strcmp(node.getStr(SHADER, ""), "cache_node") == 0 ? node.getBlockByNameEx("params")->getStr("preceding_subgraph_hash", "")
                                                            : nullptr;
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

namespace
{
// Calls fn for the input or output blocks of node in order until fn returns true; returns whether one did.
template <typename Fn>
bool for_each_reg_block(const DataBlock &node, TShaderRegType type, Fn fn)
{
  const int nameId = node.getNameId(get_tgen_type_name(type));
  for (int bi = node.findBlock(nameId); bi != -1; bi = node.findBlock(nameId, bi))
  {
    if (fn(*node.getBlock(bi)))
    {
      return true;
    }
  }
  return false;
}

bool node_loads_files(const DataBlock &node, const TextureRegManager &regs)
{
  return for_each_reg_block(node, TSHADER_REG_TYPE_INPUT,
    [&regs](const DataBlock &input) { return regs.isFileLoadPending(input.getStr("reg", "")); });
}

void take_size_and_auto_fmt(BaseTexture *tex, int &w, int &h, uint32_t &auto_fmt)
{
  if (!tex || (w > 0 && auto_fmt != 0) || tex->getType() != D3DResourceType::TEX)
  {
    return;
  }
  TextureInfo info;
  tex->getinfo(info, 0);
  if (w <= 0)
  {
    w = info.w;
    h = info.h;
  }
  if (auto_fmt == 0)
  {
    auto_fmt = info.cflg & TEXFMT_MASK;
  }
}

// fmt holds TEXCF_UNORDERED for a UAV output.
bool live_output_changed(D3dResource *res, uint32_t fmt, int w, int h)
{
  if (!res)
  {
    return false;
  }
  if (res->getType() != D3DResourceType::TEX)
  {
    return true;
  }
  TextureInfo info;
  static_cast<BaseTexture *>(res)->getinfo(info, 0);
  return info.w != w || info.h != h || fmt != (info.cflg & (TEXFMT_MASK | TEXCF_UNORDERED));
}

// Matches the registers startNode passes to createTexture (outputs, and "tex:" inputs on their first use) and
// the size and format it gives an output.
bool node_creates_textures(const DataBlock &node, const TextureRegManager &regs, const eastl::hash_map<eastl::string, int> &reg_usage)
{
  int w = node.getInt("width", -1);
  int h = node.getInt("height", w);
  uint32_t autoFmt = 0;
  const bool inputCreates = for_each_reg_block(node, TSHADER_REG_TYPE_INPUT, [&](const DataBlock &input) {
    const char *name = input.getStr("reg", "null");
    if (reg_usage.find_as(name) != reg_usage.end() && regs.isGeneratedTexturePending(name))
    {
      return true;
    }
    const int reg = regs.getRegNo(name);
    take_size_and_auto_fmt(reg >= 0 ? regs.getTexture(reg) : nullptr, w, h, autoFmt);
    return false;
  });
  int pooledOutputs = 0;
  return inputCreates || for_each_reg_block(node, TSHADER_REG_TYPE_OUTPUT, [&](const DataBlock &output) {
    const char *name = output.getStr("reg", "null");
    const char *fmtStr = output.getStr("fmt", "");
    const auto usage = reg_usage.find_as(name);
    if (strcmp(fmtStr, PARTICLES) == 0 || usage == reg_usage.end() || usage->second < 1)
    {
      return false;
    }
    const uint32_t fmt = parse_tex_format(fmtStr, autoFmt) | (output.getBool("is_uav", false) ? TEXCF_UNORDERED : 0);
    const int reg = regs.getRegNo(name);
    if (reg >= 0 && regs.getRegUsageLeft(reg) > 0)
    {
      // startNode pools a changed output, which can push the matching target out of the pool.
      return live_output_changed(regs.getResource(reg), fmt, w, h);
    }
    // Each pooled take can evict a retained register into the pool, and push out an older match.
    return ++pooledOutputs > 1 || regs.pooledTargetCount(fmt, w, h) == 0;
  });
}
} // namespace

void TextureGenerator::startProcessNodes(const DataBlock &in_nodes, TextureRegManager &regs)
{
  finalRegistersRenamed.clear();
  finalNameOfReg.clear();
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
        {
          finalRegistersRenamed[it->first] = it->second;
          finalNameOfReg[it->second] = finalName;
        }

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
        regs.discard(regNo);
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

  eastl::vector<int> dirtyNodeLeaves;
  eastl::hash_map<eastl::string, uint64_t> newNodesData;

  const int nodeInputNameId = nodes.getNameId(get_tgen_type_name(TSHADER_REG_TYPE_INPUT));

  for (int ni = 0; ni < nodes.blockCount(); ++ni)
  {
    DataBlock &node = *nodes.getBlock(ni);
    const uint64_t nodeHash = texgen_hash_block_content(node);
    newNodesData[node.getBlockName()] = nodeHash;
    auto it = nodesData.find_as(node.getBlockName());
    if (it == nodesData.end() || it->second != nodeHash) // node had changed in it's params
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

    // Rewritten after hashing, so the dirty check does not see it.
    const char *id = cache_node_record_id(node);
    const int inputIdx = id ? node.findBlock(nodeInputNameId) : -1;
    if (inputIdx >= 0 && regcache::is_record_exists(id))
    {
      node.getBlock(inputIdx)->setStr("reg", "tex:const:0");
      node.addBlock("params")->setBool("use_data_from_cache", true);
    }
  }
  changedShaders.clear();
  nodesData = newNodesData;
  debug("there are %d dirty leaves", dirtyNodeLeaves.size());

  eastl::vector<eastl::string> regsForScratchPad;
  const int scratchPadSize = max((int)finalRegs.size(), (int)16);
  optimize(nodes, dirtyNodeLeaves, aliveRegs, finalRegs, importantRegs, nodesExecutionOrder, regsForScratchPad, scratchPadSize, true,
    /*finals_held=*/!finalCb, orderSearch);

#if FALLBACK_TO_NO_OPTIMIZATION_ON_FAILURE
  if (optimizer_failed_in_this_update)
  {
    logerr("Optimizer failure. Fallback to 'no optimization'.");
    optimizer_failed_in_this_update = false;
    optimize(nodes, dirtyNodeLeaves, aliveRegs, finalRegs, importantRegs, nodesExecutionOrder, regsForScratchPad, scratchPadSize,
      false, /*finals_held=*/!finalCb, orderSearch);
  }
#endif
  allNodesRemapped = eastl::move(nodes);

  for (int i = 0; i < getNodeCount(); ++i)
    calcRegCount(getNode(i), regUsageCount);

  for (const auto &reg : regsForScratchPad) // infinite scratch pad!
  {
    auto regCnt = regUsageCount.find(reg);
    if (regCnt != regUsageCount.end())
      regCnt->second++;
  }

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

namespace
{
// Released registers are retained as finished results.
void discard_unfinished_node_outputs(const DataBlock &node, TextureRegManager &regs)
{
  const int outputNameId = node.getNameId(get_tgen_type_name(TSHADER_REG_TYPE_OUTPUT));
  for (int bi = node.findBlock(outputNameId); bi != -1; bi = node.findBlock(outputNameId, bi))
  {
    const int regNo = regs.getRegNo(node.getBlock(bi)->getStr("reg", "null"));
    if (regNo >= 0)
      regs.discard(regNo);
  }
}
} // namespace

// Hashes are stored when a pass starts.
void TextureGenerator::markUnrunNodesDirty(int first)
{
  for (int i = first; first >= 0 && i < getNodeCount(); ++i)
    nodesData.erase(getNode(i).getBlockName());
}

void TextureGenerator::cancelUnfinishedNode(TextureRegManager &regs)
{
  if (cNode >= 0 && cNode < getNodeCount() && cNodeSubStep > 0)
  {
    if (cNodeData.shader)
      cNodeData.shader->cancelSubSteps(regs, cNodeData, cNodeSubStep);
    for (int reg : cNodeData.usedRegs)
      regs.textureUsed(reg);
    discard_unfinished_node_outputs(getNode(cNode), regs);
  }
  markUnrunNodesDirty(cNode);
}

int TextureGenerator::start(const DataBlock &nodes, TextureRegManager &reg_manager)
{
  cancelUnfinishedNode(reg_manager);
  regUsageCount.clear();
  regManager = &reg_manager;
  startProcessNodes(nodes, reg_manager);
  // A retained register that this pass reads gets its use count now, so creations before its first read do not evict it.
  for (const auto &regCnt : regUsageCount)
  {
    const int reg = reg_manager.getRegNo(regCnt.first.c_str());
    if (reg >= 0 && reg_manager.getRegUsageLeft(reg) == 0)
      reg_manager.setUseCount(reg, regCnt.second);
  }
  cNode = 0;
  cNodeSubStep = 0;
  cNodeData = NodeData();
  costKeyNode = -1;
  return getNodeCount();
}

void TextureGenerator::stop()
{
  if (regManager)
    cancelUnfinishedNode(*regManager);
  regManager = 0;
  cNode = -1;
}


bool TextureGenerator::peekNextCommand(TextureGenCommand &command)
{
  command = TextureGenCommand();
  if (cNode < 0 || cNode >= getNodeCount())
  {
    return false;
  }
  const DataBlock &node = getNode(cNode);
  const TextureGenShader *shader = cNodeData.shader;
  if (cNodeSubStep == 0)
  {
    auto found = shaders.find_as(node.getStr(SHADER, ""));
    shader = found != shaders.end() ? found->second : nullptr;
    if (regManager)
    {
      command.loadsFiles = node_loads_files(node, *regManager);
      command.createsTextures = node_creates_textures(node, *regManager, regUsageCount);
    }
  }
  command.shaderName = shader ? shader->getName() : node.getStr(SHADER, "");
  if (cNodeSubStep > 0 && shader)
  {
    command.pixels = shader->subStepPixels(cNodeData, cNodeSubStep);
    command.shaderName = shader->subStepShaderName(cNodeData, cNodeSubStep);
  }
  // A pixel-shader pass reads its particles back on the CPU when an entities saver is set.
  const bool readsBackParticles = regManager && regManager->getEntitiesSaver() && node.getBlockByName(PARTICLES);
  command.mapsOnCpu = shader && (shader->mapsResourcesOnCpu(cNodeSubStep) || readsBackParticles);
  if (costKeyNode != cNode)
  {
    costKeyOfNode = node_cost_key(node);
    costKeyNode = cNode;
  }
  command.costKey = costKeyOfNode;
  hash_str(command.costKey, command.shaderName);
  return true;
}

void TextureGenerator::getUpcomingTexInputs(int node_count, eastl::vector<const char *> &reg_names) const
{
  reg_names.clear();
  if (cNode < 0)
  {
    return;
  }
  const int first = cNodeSubStep == 0 ? cNode : cNode + 1;
  for (int ni = first; ni < getNodeCount() && ni < first + node_count; ++ni)
  {
    for_each_reg_block(getNode(ni), TSHADER_REG_TYPE_INPUT, [&reg_names](const DataBlock &input) {
      const char *reg = input.getStr("reg", "");
      if (strstr(reg, "tex:") == reg)
      {
        reg_names.push_back(reg);
      }
      return false;
    });
  }
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

    lastCommandPixels = cNodeData.shader->subStepPixels(cNodeData, cNodeSubStep);
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

  regManager->setLogger(oldLogger);
  d3d::set_program(BAD_PROGRAM);

  if (cnt < 0)
  {
    if (cNodeSubStep == 0)
    {
      eastl::hash_map<eastl::string, int> uses;
      calcRegCount(getNode(cNode), uses);
      for (auto &use : uses)
        for (const int regNo = regManager->getRegNo(use.first.c_str()); use.second-- > 0 && regManager->getRegUsageLeft(regNo) > 0;)
          regManager->textureUsed(regNo);
      discard_unfinished_node_outputs(getNode(cNode), *regManager);
    }
    stop();
  }

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
  data.nodeName = nodeName;
  // carray<Tab<D3dResource *>, TSHADER_REG_TYPE_CNT> regs;
  data.nodeW = node.getInt("width", -1);
  data.nodeH = node.getInt("height", data.nodeW);
  data.blending = parse_blending(node.getStr(BLENDING, "no"));
  data.params = *node.getBlockByNameEx("params");
  uint32_t autoFmt = 0;
  if (data.nodeW * data.nodeH <= 0)
  {
    logger->log(LOGLEVEL_ERR,
      String(128, "node <%s> of shader <%s> has invalid dimensions: %dx%d", nodeName, data.shader->getName(), data.nodeW, data.nodeH));
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
              tex = reg_manager.getTexture(treg);
              data.usedRegs.push_back(treg);
            }
            data.inputs.push_back(TextureInput{tex, wrap});
          }

          take_size_and_auto_fmt(tex, data.nodeW, data.nodeH, autoFmt);
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
              const bool shouldBeKilled =
                strcmp(fmtStr, PARTICLES) == 0 ||
                live_output_changed(reg_manager.getResource(treg), parse_tex_format(fmtStr, autoFmt) | (useUav ? TEXCF_UNORDERED : 0),
                  data.nodeW, data.nodeH);

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
                treg = reg_manager.createParticlesBuffer(name, data.nodeW * data.nodeH * PARTICLES_PER_PIXEL, useCount);
              }
              else
              {
                fmt = parse_tex_format(fmtStr, autoFmt);
                fmt |= useUav ? TEXCF_UNORDERED : 0;
                treg = reg_manager.createTexture(name, fmt, data.nodeW, data.nodeH, useCount);
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
              if (output && output->getType() == D3DResourceType::TEX)
              {
                const DataBlock *particlesBlock = node.getBlockByName(PARTICLES);
                const char *particlesRegName = particlesBlock ? particlesBlock->getStr("reg", NULL) : 0;
                if (particlesRegName) // initialize to zero if it has particles scatter
                {
                  BaseTexture *tex = reg_manager.getTexture(treg);
                  if (tex)
                  {
                    SCOPE_RENDER_TARGET;
                    d3d::set_render_target({}, DepthAccess::RW, {{(Texture *)output, 0, 0}});
                    d3d::clearview(CLEAR_TARGET, 0, 0, 0);
                  }
                }
              }
              data.outputs.push_back(output);
              if (finalCb && finalNameOfReg.find_as(name) != finalNameOfReg.end())
                data.finalRegs.push_back(treg);
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

  data.use_depth = data.blending == DEPTH_TEST || data.blending == NO_BLENDING || data.blending == ALPHA_BLENDING;
  //|| node.getBool("depth", false)

  return ret;
}

void TextureGenerator::endNode(NodeData &data, TextureRegManager &reg_manager)
{
  for (int i = 0; i < data.usedRegs.size(); ++i)
    reg_manager.textureUsed(data.usedRegs[i]);
  // The callback owns the final now; a later reader must not count the final's use back in.
  for (int reg : data.finalRegs)
  {
    const char *regName = reg_manager.getRegName(reg);
    finalCb(finalCbCtx, finalNameOfReg.find_as(regName)->second.c_str(), reg_manager.getTexture(reg));
    if (auto it = regUsageCount.find_as(regName); it != regUsageCount.end())
      --it->second;
    reg_manager.textureUsed(reg);
  }
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
    case TEXFMT_R8: return TEXFMT_R32F;

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

bool texgen_get_next_command(TextureGenerator &s, TextureGenCommand &command) { return s.peekNextCommand(command); }

void texgen_get_upcoming_tex_inputs(TextureGenerator &s, int node_count, eastl::vector<const char *> &reg_names)
{
  s.getUpcomingTexInputs(node_count, reg_names);
}

int texgen_get_done_node_count(TextureGenerator &s) { return s.getDoneNodeCount(); }

const char *texgen_get_current_node_name(TextureGenerator &s) { return s.getCurrentNodeName(); }

void texgen_get_pass_node_names(TextureGenerator &s, eastl::vector<const char *> &names) { s.getPassNodeNames(names); }

uint64_t texgen_get_node_key(TextureGenerator &s, const char *name) { return s.getNodeKey(name); }

int texgen_get_last_command_pixels(TextureGenerator &s) { return s.getLastCommandPixels(); }

void texgen_stop_process_commands(TextureGenerator &s) { return s.stop(); }

void delete_texture_generator(TextureGenerator *&gen) { del_it(gen); }

void texgen_pin_cache_node_records(const DataBlock &commands)
{
  for (int i = 0; i < commands.blockCount(); ++i)
  {
    const char *id = cache_node_record_id(*commands.getBlock(i));
    if (id && regcache::is_record_exists(id))
    {
      regcache::pin(id);
    }
  }
}

// Block names of the nodes that write root_regs and of every node they read, directly or through other nodes.
static void feeding_node_names(const DataBlock &commands, const eastl::vector<const char *> &root_regs,
  eastl::vector<const char *> &names)
{
  eastl::vector<eastl::vector<int>> nodeRegs[TSHADER_REG_TYPE_CNT];
  eastl::hash_map<eastl::string, int> regNamesMap;
  eastl::vector<eastl::string> regNames;
  collect_node_regs(commands, nodeRegs, regNamesMap, regNames, [](int, int, const DataBlock &) {});
  eastl::vector<int> nodeProducingReg;
  eastl::vector<eastl::vector_set<int>> nodesProducingCurrent;
  get_nodes_producing_current(nodeRegs, nodeProducingReg, nodesProducingCurrent);

  // Not get_nodes_generating_regs: it asserts on a register that no node reads or writes.
  eastl::hash_set<int> roots;
  for (const char *name : root_regs)
  {
    auto reg = regNamesMap.find_as(name);
    if (reg != regNamesMap.end() && reg->second < nodeProducingReg.size() && nodeProducingReg[reg->second] >= 0)
    {
      roots.insert(nodeProducingReg[reg->second]);
    }
  }

  Bitarray nodeUsed;
  nodeUsed.resize(commands.blockCount());
  nodeUsed.reset();
  dead_code_elimination(nodeUsed, roots, nodesProducingCurrent);
  names.clear();
  for (int ni = 0; ni < commands.blockCount(); ++ni)
  {
    if (nodeUsed[ni])
    {
      names.push_back(commands.getBlock(ni)->getBlockName());
    }
  }
}

void texgen_get_used_node_names(const DataBlock &commands, eastl::vector<const char *> &names)
{
  eastl::vector<const char *> finals;
  const int finalNameId = commands.getNameId("final");
  for (int i = 0; i < commands.paramCount(); ++i)
  {
    if (commands.getParamNameId(i) == finalNameId && commands.getParamType(i) == DataBlock::TYPE_STRING)
    {
      finals.push_back(commands.getStr(i));
    }
  }
  feeding_node_names(commands, finals, names);
}

const char *texgen_get_cache_node_record_id(const DataBlock &node) { return cache_node_record_id(node); }

void texgen_get_node_input_names(const DataBlock &commands, const char *node, eastl::vector<const char *> &names)
{
  names.clear();
  const DataBlock *nodeBlk = commands.getBlockByName(node);
  if (!nodeBlk)
  {
    return;
  }
  eastl::vector<const char *> inputs;
  const int inputNameId = commands.getNameId(get_tgen_type_name(TSHADER_REG_TYPE_INPUT));
  const int particlesNameId = commands.getNameId(PARTICLES);
  for (int bi = 0; bi < nodeBlk->blockCount(); ++bi)
  {
    const DataBlock &reg = *nodeBlk->getBlock(bi);
    if (reg.getBlockNameId() == inputNameId || reg.getBlockNameId() == particlesNameId)
    {
      inputs.push_back(reg.getStr("reg", "null"));
    }
  }
  feeding_node_names(commands, inputs, names);
  // In a cycle the node reads itself.
  names.erase(eastl::remove_if(names.begin(), names.end(), [node](const char *n) { return strcmp(n, node) == 0; }), names.end());
}

void texgen_set_logger(TextureGenerator *s, TextureGenLogger *logger) { s->setLogger(logger); }

TextureGenLogger *texgen_get_logger(TextureGenerator *s) { return s->getLogger(); }

extern const char *texgen_get_final_reg_name(TextureGenerator *s, const char *name) { return s ? s->getFinalRegName(name) : name; }

void texgen_set_final_callback(TextureGenerator *s, TextureGenFinalCallback cb, void *ctx) { s->setFinalCallback(cb, ctx); }

void texgen_set_order_search_evals(TextureGenerator *s, int evals) { s->setOrderSearchEvals(evals); }
