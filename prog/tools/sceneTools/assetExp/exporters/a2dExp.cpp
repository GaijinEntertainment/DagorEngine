// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildExpPluginChain.h>
#include "a2dOptimizer.h"
#include <assets/assetPlugin.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <generic/dag_sort.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/util/strUtil.h>
#include <util/dag_fastStrMap.h>
#include <gameRes/dag_stdGameRes.h>
#include <anim/dag_animChannels.h>
#include <math/dag_Point3.h>
#include <math/dag_Quat.h>
#include <vecmath/dag_vecMath.h>
#include <ioSys/dag_ioUtils.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_files.h>
#include <debug/dag_debug.h>
#include <util/dag_finally.h>
#include <perfMon/dag_perfTimer.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include "exp_skeleton_tools.h"
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_btagCompr.h>
#include <anim/dag_animKeyInterp.h>
#include <ioSys/dag_dataBlockUtils.h>

#include <acl/compression/pre_process.h>
#include <acl/compression/compress.h>

BEGIN_DABUILD_PLUGIN_NAMESPACE(a2d)
static const char *TYPE = "a2d";
static bool preferZstdPacking = false;
static bool allowOodlePacking = false;
static bool logExportTime = false;
static bool errorOnDuplicateNames = false;
static bool errorOnOldParams = false;

static TMatrix make_scale_tm(const TMatrix &tm, const Point3 &scale)
{
  TMatrix res = tm;
  res.setcol(0, res.getcol(0) * scale.x);
  res.setcol(1, res.getcol(1) * scale.y);
  res.setcol(2, res.getcol(2) * scale.z);
  return res;
}

static Point3 get_tm_scale(const TMatrix &tm) { return Point3(tm.getcol(0).length(), tm.getcol(1).length(), tm.getcol(2).length()); }

class AclAllocator : public acl::iallocator
{
  IMemAlloc *mem;

public:
  AclAllocator(IMemAlloc *m) : mem(m) {}

  virtual void *allocate(size_t size, size_t alignment) { return mem->allocAligned(size, alignment); }

  virtual void deallocate(void *ptr, size_t size)
  {
    G_UNUSED(size);
    mem->freeAligned(ptr);
  }
};

class A2dExporter : public IDagorAssetExporter
{
public:
  virtual const char *__stdcall getExporterIdStr() const { return "a2d exp"; }

  virtual const char *__stdcall getAssetType() const { return TYPE; }
  virtual unsigned __stdcall getGameResClassId() const { return Anim2DataGameResClassId; }
  virtual unsigned __stdcall getGameResVersion() const
  {
    static constexpr const int base_ver = 5;
    return base_ver * 3 + (!preferZstdPacking ? 0 : (allowOodlePacking ? 2 : 1 + 3));
  }

  virtual void __stdcall onRegister() {}
  virtual void __stdcall onUnregister() {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override
  {
    files.clear();
    files.push_back() = a.getTargetFilePath();
  }

  virtual bool __stdcall isExportableAsset(DagorAsset &a) { return true; }

  virtual bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log)
  {
    auto startTime = profile_ref_ticks();

    String fname(a.getTargetFilePath());
    FullFileLoadCB crd(fname);
    if (!crd.fileHandle)
    {
      log.addMessage(ILogWriter::ERROR, "Can't open a2d: %s", fname);
      return false;
    }

    if (!dagor_target_code_valid(cwr.getTarget()))
    {
      log.addMessage(ILogWriter::ERROR, "%s: no support for %c%c%c%c target!", a.getNameTypified(), _DUMP4C(cwr.getTarget()));
      return false;
    }

    const DataBlock &props = a.getProfileTargetProps(cwr.getTarget(), cwr.getProfile());
    if (props.paramExists("opt") || props.paramExists("posEps") || props.paramExists("rotEps") || props.paramExists("sclEps") ||
        props.paramExists("rotResampleFreq") || a.props.paramExists("opt") || a.props.paramExists("posEps") ||
        a.props.paramExists("rotEps") || a.props.paramExists("sclEps") || a.props.paramExists("rotResampleFreq"))
    {
      log.addMessage(errorOnOldParams ? ILogWriter::ERROR : ILogWriter::WARNING, "%s: obsolete a2d export parameters in blk",
        a.getName());
      debug_print_datablock("target props", &props);
      debug_print_datablock("asset props", &a.props);
    }

    float posPrecision = props.getReal("posPrecision", a.props.getReal("posPrecision", 0.0001f));
    float velPrecision = props.getReal("velPrecision", a.props.getReal("velPrecision", 0.001f));
    float shellDistance = props.getReal("shellDistance", a.props.getReal("shellDistance", 0.150f));
    float sampleRate = props.getReal("sampleRate", a.props.getReal("sampleRate", 0));
    debug("%s: posPrec=%.5fm velPrec=%.5f shell=%.5fm sr=%.2fHz", a.getName(), posPrecision, velPrecision, shellDistance, sampleRate);

    FastNameMap reqNodeMask, charDepNodes;
    bool strip_char_dep = false;
    if (const DataBlock *b = props.getBlockByName("applyNodeMask"))
    {
      for (int i = 0, nid = b->getNameId("node"); i < b->paramCount(); i++)
        if (b->getParamNameId(i) == nid && b->getParamType(i) == b->TYPE_STRING)
          reqNodeMask.addNameId(b->getStr(i));
      if (!reqNodeMask.nameCount())
        log.addMessage(ILogWriter::ERROR, "%s: applyNodeMask{} doesn't contain node:t=", a.getNameTypified());
    }
    if (const DataBlock *b = props.getBlockByName("stripChannelsForCharDep"))
    {
      strip_char_dep = b->getBool("strip", true);
      for (int i = 0, nid = b->getNameId("node"); i < b->paramCount(); i++)
        if (b->getParamNameId(i) == nid && b->getParamType(i) == b->TYPE_STRING)
          charDepNodes.addNameId(b->getStr(i));
    }

    AScene origSkeletonScene;
    AScene autoCompletedSkeletonScene;
    bool autoCompleted = false;
    if (const DataBlock *autoCompleteBlk = a.props.getBlockByName("autoComplete"))
    {
      const char *animSuffix = "_anim";
      const int flags = LASF_NOMATS | LASF_NOMESHES | LASF_NOSPLINES | LASF_NOLIGHTS;
      if (load_skeleton(a, animSuffix, flags, log, origSkeletonScene) &&
          load_skeleton(a, animSuffix, flags, log, autoCompletedSkeletonScene))
        autoCompleted = auto_complete_skeleton(a, animSuffix, flags, log, autoCompletedSkeletonScene.root);
    }

    mkbindump::BinDumpSaveCB cwrDump(128 << 10, cwr.getTarget(), cwr.WRITE_BE);
    bool failed = false;
    if (!convert(crd, cwrDump, posPrecision, velPrecision, shellDistance, sampleRate, props.getBool("makeAdditive", false),
          props.getStr("additiveRefPose", NULL), reqNodeMask, origSkeletonScene.root,
          autoCompleted ? autoCompletedSkeletonScene.root : nullptr, strip_char_dep, charDepNodes, a, log))
    {
      log.addMessage(ILogWriter::ERROR, "%s: error convert!", a.getNameTypified());
      cwr.beginBlock();
      cwr.endBlock();
      return false;
    }

    cwr.beginBlock();
    if ((preferZstdPacking || allowOodlePacking) && cwrDump.getSize() > 512)
    {
      mkbindump::BinDumpSaveCB mcwr(cwrDump.getSize(), cwr.getTarget(), cwr.WRITE_BE);
      MemoryLoadCB mcrd(cwrDump.getRawWriter().getMem(), false);

      if (allowOodlePacking)
      {
        mcwr.writeInt32e(cwrDump.getSize());
        oodle_compress_data(mcwr.getRawWriter(), mcrd, cwrDump.getSize());
      }
      else
        zstd_compress_data(mcwr.getRawWriter(), mcrd, cwrDump.getSize(), 256 << 10, 19);

      if (mcwr.getSize() < cwrDump.getSize() * 8 / 10 && mcwr.getSize() + 256 < cwrDump.getSize()) // enough profit in compression
      {
        mcwr.copyDataTo(cwr.getRawWriter());
        cwr.endBlock(allowOodlePacking ? btag_compr::OODLE : btag_compr::ZSTD);
      }
      else
      {
        cwrDump.copyDataTo(cwr.getRawWriter());
        cwr.endBlock(btag_compr::NONE);
      }
    }
    else
    {
      cwrDump.copyDataTo(cwr.getRawWriter());
      cwr.endBlock(btag_compr::NONE);
    }

    if (logExportTime)
      log.addMessage(ILogWriter::NOTE, "%s: exported in %d ms", a.getNameTypified(), profile_time_usec(startTime) / 1000);
    return true;
  }

protected:
  static const int RATE_DIV = 80;
  struct AnimDataHeader
  {
    unsigned label;    // MAKE4C('A','N','I','M')
    unsigned ver;      // 0x200
    unsigned hdrSize;  // =sizeof(AnimDataHeader)
    unsigned reserved; // 0x00000000
  };

  struct AnimDataHeaderV200
  {
    unsigned namePoolSize;      // size of name pool, in bytes
    unsigned timeNum;           // number of entries in time pool
    unsigned keyPoolSize;       // size of key pool, in bytes
    unsigned totalAnimLabelNum; // number of keys in note track (time labels)
    unsigned dataTypeNum;       // number of data type records
  };

  struct AnimKeyLabel
  {
    int name;
    int time;
  };

  struct DataTypeHeader
  {
    unsigned dataType;
    unsigned offsetInKeyPool;
    unsigned offsetInTimePool;
    unsigned channelNum;

    int getKeySize()
    {
      switch (dataType)
      {
        case 0x577084E5u: return sizeof(OldAnimKeyPoint3); // DATATYPE_POINT3
        case 0x39056464u: return sizeof(OldAnimKeyQuat);   // DATATYPE_QUAT
        case 0xEE0BD66Au: return sizeof(OldAnimKeyReal);   // DATATYPE_REAL
      }
      return 0;
    }
    int convertDataType()
    {
      switch (dataType)
      {
        case 0x577084E5u: return AnimV20::DATATYPE_POINT3X; // DATATYPE_POINT3
        case 0x39056464u: return AnimV20::DATATYPE_QUAT;    // DATATYPE_QUAT
        case 0xEE0BD66Au: return AnimV20::DATATYPE_REAL;    // DATATYPE_REAL
      }
      DAG_FATAL("unknown type %08X", dataType);
      return 0;
    }
    static unsigned convertChanType(unsigned chanType)
    {
      switch (chanType)
      {
        case 0xE66CF0CCu: return AnimV20::CHTYPE_POSITION;
        case 0x48D43D77u: return AnimV20::CHTYPE_SCALE;
        case 0x8D490AE4u: return AnimV20::CHTYPE_ROTATION;
        case 0x31DE5F48u: return AnimV20::CHTYPE_ORIGIN_LINVEL;
        case 0x09FD7093u: return AnimV20::CHTYPE_ORIGIN_ANGVEL;
      }
      DAG_FATAL("unknown chan type %08X", chanType);
      return 0;
    }
  };


  static inline bool rot_equal(quat4f a, quat4f b, vec4f tcos)
  {
    return v_test_any_bit_set(
      v_or(v_cmp_ge(v_dot4(a, b), tcos), v_cmp_ge(v_max(v_sub(V_C_TWO, v_add(tcos, tcos)), V_C_EPS_VAL), v_length4_sq(v_sub(a, b)))));
  }

  static int find_node_index(const ChannelData *ch, const char *name, int index)
  {
    if (!ch)
      return -1;
    if (index < ch->nodeNum && ch->nodeName[index] == name)
      return index;
    for (int i = 0; i < ch->nodeNum; i++)
      if (ch->nodeName[i] == name)
        return i;
    return -1;
  }

  static bool convert(IGenLoad &crd, mkbindump::BinDumpSaveCB &cwr, float pos_prec, float vel_prec, float shell_dist,
    float sample_rate, bool make_additive, const char *additive_key_suffix, FastNameMap &req_node_mask, Node *original_skeleton,
    Node *auto_completed_skeleton, bool strip_for_char_dep, FastNameMap &char_dep_nodes, const DagorAsset &a, ILogWriter &log)
  {
    AnimDataHeader hdr;

    if (pos_prec <= 0)
      pos_prec = 1e-5f;
    if (vel_prec <= 0)
      vel_prec = 1e-5f;
    if (shell_dist <= 0)
      shell_dist = 1e-5f;

    const float shellDistance = shell_dist;
    const float qvvfPrecision = pos_prec;

    const float quatEps = cosf(pos_prec / shell_dist);
    const float sclPrecision = pos_prec / shell_dist;

    const float angVelPrecision = vel_prec / shell_dist;

    // read header and check integrity
    crd.read(&hdr, sizeof(hdr));
    if (hdr.label != MAKE4C('A', 'N', 'I', 'M'))
    {
      log.addMessage(ILogWriter::ERROR, "%s: unrecognized label %c%c%c%c", a.getNameTypified(), _DUMP4C(hdr.label));
      return false;
    }

    if (hdr.ver != 0x200)
    {
      log.addMessage(ILogWriter::ERROR, "%s: unsupported version 0x%08X", a.getNameTypified(), hdr.ver);
      return false;
    }

    AnimDataHeaderV200 hdrV200;
    crd.read(&hdrV200, sizeof(hdrV200));

    const unsigned hdrSize = sizeof(AnimDataHeader) + sizeof(hdrV200);

    // read pools
    Tab<char> namePool;
    namePool.resize(hdrV200.namePoolSize);
    crd.read(namePool.data(), data_size(namePool));

    Tab<char> timeKeyPool;
    timeKeyPool.resize(sizeof(unsigned) * hdrV200.timeNum + hdrV200.keyPoolSize);
    char *keyPool = (char *)timeKeyPool.data(); // to make keyPool 16-byte aligned!
    int *timePool = (int *)(keyPool + hdrV200.keyPoolSize);

    crd.read(timePool, sizeof(unsigned) * hdrV200.timeNum);
    crd.read(keyPool, hdrV200.keyPoolSize);

    // remap pointers

    Tab<AnimKeyLabel> noteTrackPool;
    noteTrackPool.resize(hdrV200.totalAnimLabelNum);
    crd.read(noteTrackPool.data(), data_size(noteTrackPool));
    // strip unnamed note track keys
    for (int i = hdrV200.totalAnimLabelNum - 1; i >= 0; i--)
      if (!namePool[noteTrackPool[i].name])
        erase_items(noteTrackPool, i, 1);
    hdrV200.totalAnimLabelNum = noteTrackPool.size();

    FastStrMap namedKey;
    for (int i = 0; i < hdrV200.totalAnimLabelNum; i++)
    {
      char *name = &namePool[noteTrackPool[i].name];
      while (strchr(" \r\n\t", *name))
      {
        noteTrackPool[i].name++;
        name++;
      }

      char *name_end = name + strlen(name) - 1;
      while (name_end >= name && strchr(" \r\n\t", *name_end))
      {
        *name_end = '\0';
        name_end--;
      }
      namedKey.addStrId(&namePool[noteTrackPool[i].name], noteTrackPool[i].time);
    }

    if (a.props.getBool("removeKeys", false))
      noteTrackPool.clear();

    // remove duplicate keys
    for (int i = 1; i < noteTrackPool.size(); i++)
      for (int j = 0; j < i; j++)
        if (noteTrackPool[i].time == noteTrackPool[j].time &&
            strcmp(&namePool[noteTrackPool[i].name], &namePool[noteTrackPool[j].name]) == 0)
        {
          erase_items(noteTrackPool, i, 1);
          i--;
          break;
        }
    hdrV200.totalAnimLabelNum = noteTrackPool.size();

    int nid_requireKey = a.props.getNameId("requireKey");
    if (nid_requireKey >= 0)
      for (int i = 0; i < a.props.paramCount(); i++)
        if (a.props.getParamNameId(i) == nid_requireKey && a.props.getParamType(i) == DataBlock::TYPE_STRING)
        {
          const char *key = a.props.getStr(i);
          if (namedKey.getStrId(key) < 0)
          {
            log.addMessage(ILogWriter::ERROR, "%s: missing mandatory label <%s>", a.getNameTypified(), key);
            return false;
          }
        }

    // find minimax timing
    int mint = 0, maxt = 0, minStep = 4800 * 60; // 1 minute step max
    if (hdrV200.timeNum)
      mint = maxt = timePool[0];
    for (int i = 1; i < hdrV200.timeNum; i++)
    {
      if (mint > timePool[i])
        mint = timePool[i];
      else if (maxt < timePool[i])
        maxt = timePool[i];
      int dt = timePool[i] - timePool[i - 1];
      if (dt > 0 && minStep > dt)
        minStep = dt;
    }

    // complement notetrack with default keys
    if (const DataBlock *b = a.props.getBlockByName("defaultKeys"))
      for (int i = 0; i < b->paramCount(); i++)
        if (b->getParamType(i) == DataBlock::TYPE_INT)
        {
          const char *key = b->getParamName(i);
          bool found = false;
          for (int j = 0; j < noteTrackPool.size(); j++)
            if (strcmp(&namePool[noteTrackPool[j].name], key) == 0)
            {
              found = true;
              break;
            }
          if (!found)
          {
            AnimKeyLabel &k = noteTrackPool.push_back();
            k.name = 0;
            k.time = b->getInt(i) < 0 ? maxt : b->getInt(i) * RATE_DIV;
            while (k.name < namePool.size() && strcmp(&namePool[k.name], key) != 0)
              k.name += i_strlen(&namePool[k.name]) + 1;
            if (k.name >= namePool.size())
              k.name = append_items(namePool, strlen(key) + 1, key);
            // debug("added key (%d %d) %s, poolSz=%d", k.name, k.time, &namePool[k.name], namePool.size());
          }
        }
    hdrV200.totalAnimLabelNum = noteTrackPool.size();
    hdrV200.namePoolSize = data_size(namePool);

    // find minimax timing including notetrack
    for (int i = 0; i < hdrV200.totalAnimLabelNum; i++)
      if (mint > noteTrackPool[i].time)
        mint = noteTrackPool[i].time;
      else if (maxt < noteTrackPool[i].time)
        maxt = noteTrackPool[i].time;
    debug("mint=%d maxt=%d (%d at %dHz, or %dsec)", mint, maxt, unsigned(maxt - mint) / RATE_DIV, 4800 / RATE_DIV,
      unsigned(maxt - mint) / 4800);

    // rebase timing
    for (int i = 0; i < hdrV200.timeNum; i++)
      timePool[i] -= mint;
    for (int i = 0; i < hdrV200.totalAnimLabelNum; i++)
      noteTrackPool[i].time -= mint;
    for (int i = 0; i < namedKey.getMapRaw().size(); i++)
      const_cast<int &>(namedKey.getMapRaw()[i].id) -= mint;

    // read data of animation
    Tab<ChannelData> chan(tmpmem);
    Tab<int> nodeRemap;

    for (int dataTypeId = 0; dataTypeId < hdrV200.dataTypeNum; dataTypeId++)
    {
      DataTypeHeader dtHdr;
      crd.read(&dtHdr, sizeof(dtHdr));
      int keysz = dtHdr.getKeySize();

      for (int channelId = 0; channelId < dtHdr.channelNum; channelId++)
      {
        int type = crd.readInt();
        int node_num = crd.readInt();
        if (!node_num)
          continue;

        ChannelData &ch = chan.push_back();
        ch.dataType = dtHdr.convertDataType();
        ch.channelType = dtHdr.convertChanType(type);
        ch.alloc(node_num);

        for (int i = 0; i < ch.nodeNum; i++)
        {
          int keyNum = crd.readInt();
          auto &anim = ch.nodeAnim[i];
          anim.keyOfs = anim.keyTimeOfs = -1;
          anim.keyNum = keyNum;
          anim.key = keyPool;
          anim.keyTime = timePool;
          keyPool += keyNum * keysz;
          timePool += keyNum;

          if (ch.dataType == AnimV20::DATATYPE_QUAT)
          {
            auto keys = (OldAnimKeyQuat *)anim.key;
            for (int j = 0; j < keyNum; j++)
            {
              keys[j].p = keys[j].p.normalize();
              keys[j].b0 = keys[j].b0.normalize();
              keys[j].b1 = keys[j].b1.normalize();
            }
          }
        }

        for (int i = 0; i < ch.nodeNum; i++)
          ch.nodeName[i] = (char *)(intptr_t)crd.readInt();
        crd.read(ch.nodeWt, ch.nodeNum * sizeof(float));

        if (req_node_mask.nameCount() || strip_for_char_dep)
        {
          bool ps_type = (ch.channelType == AnimV20::CHTYPE_POSITION || ch.channelType == AnimV20::CHTYPE_SCALE);
          ChannelData ch2;
          nodeRemap.resize(ch.nodeNum);
          mem_set_ff(nodeRemap);
          for (int i = 0; i < ch.nodeNum; i++)
          {
            const char *node_nm = namePool.data() + (intptr_t)ch.nodeName[i];
            if (!(req_node_mask.nameCount() && req_node_mask.getNameId(node_nm) < 0) &&
                !(strip_for_char_dep && ps_type && char_dep_nodes.getNameId(node_nm) < 0))
              nodeRemap[i] = ch2.nodeNum++;
          }

          if (!ch2.nodeNum)
          {
            // just remove empty channel (no nodes)
            // debug("dataTypeId=%d channelId=%d remove all %d nodes", dataTypeId, channelId, ch.nodeNum);
            chan.pop_back();
          }
          else if (ch2.nodeNum < ch.nodeNum)
          {
            // strip and remap nodes
            // debug("dataTypeId=%d channelId=%d stripping %d nodes -> %d", dataTypeId, channelId, ch.nodeNum, ch2.nodeNum);
            ch2.alloc(ch2.nodeNum);

            for (int i = 0; i < ch.nodeNum; i++)
            {
              int j = nodeRemap[i];
              if (j < 0)
                continue;
              ch2.nodeAnim[j] = ch.nodeAnim[i];
              ch2.nodeName[j] = ch.nodeName[i];
              ch2.nodeWt[j] = ch.nodeWt[i];
            }

            ch.moveDataFrom(ch2);
          }
        }
      }
    }
    sort(chan, &ChannelData::cmp);

    // process additive animations
    if (make_additive)
    {
      String end_key_name, ref_key_name;
      debug("additive=%d (%s)", make_additive, additive_key_suffix);
      Tab<int> addAnimRangeTriple;
      for (int i = 0; i < namedKey.getMapRaw().size(); i++)
      {
        debug("%s=%d", namedKey.getMapRaw()[i].name, namedKey.getMapRaw()[i].id);
        if (strcmp(namedKey.getMapRaw()[i].name, "start") == 0)
        {
          end_key_name = "end";
          ref_key_name = additive_key_suffix;
        }
        else if (trail_strcmp(namedKey.getMapRaw()[i].name, "_start"))
        {
          int prefix_len = i_strlen(namedKey.getMapRaw()[i].name) - 6;
          end_key_name.printf(0, "%.*s_end", prefix_len, namedKey.getMapRaw()[i].name);
          ref_key_name.printf(0, "%.*s_%s", prefix_len, namedKey.getMapRaw()[i].name, additive_key_suffix);
        }
        else
          continue;

        int t0 = namedKey.getMapRaw()[i].id;
        int t1 = namedKey.getStrId(end_key_name);
        int tref = namedKey.getStrId(ref_key_name);
        debug("make additive (%s...%s ref=%s)  %d..%d  ref=%d", namedKey.getMapRaw()[i].name, end_key_name, ref_key_name, t0, t1,
          tref);
        if (t0 < 0 || t1 < 0 || tref < 0)
        {
          log.addMessage(ILogWriter::ERROR, "%s: bad labels for additive anim", a.getNameTypified());
          return false;
        }
        addAnimRangeTriple.push_back(t0);
        addAnimRangeTriple.push_back(t1);
        addAnimRangeTriple.push_back(tref);
      }

      Tab<animopt::PosKey> pt3x;
      Tab<animopt::RotKey> quat;
      for (int channelId = 0; channelId < chan.size(); channelId++)
      {
        ChannelData &ch = chan[channelId];
        for (int nodeId = 0; nodeId < ch.nodeNum; nodeId++)
        {
          ChannelData::Anim &anim = ch.nodeAnim[nodeId];
          if (!anim.keyNum)
            continue;

          switch (ch.dataType)
          {
            case AnimV20::DATATYPE_POINT3X:
              animopt::convert_to_PosKey(pt3x, reinterpret_cast<OldAnimKeyPoint3 *>(anim.key), anim.keyTime, anim.keyNum);
              break;
            case AnimV20::DATATYPE_QUAT:
              animopt::convert_to_RotKey(quat, reinterpret_cast<OldAnimKeyQuat *>(anim.key), anim.keyTime, anim.keyNum);
              break;
          }

          int thint = INITIAL_INTERP_HINT;

          for (int ai = 0; ai < addAnimRangeTriple.size(); ai += 3)
          {
            Point3_vec4 base_p;
            Quat_vec4 base_q;
            int t0 = addAnimRangeTriple[ai + 0];
            int t1 = addAnimRangeTriple[ai + 1];
            int tref = addAnimRangeTriple[ai + 2];

            switch (ch.dataType)
            {
              case AnimV20::DATATYPE_POINT3X:
                v_st(&base_p, interp_point3(anim, tref, thint));
                if (ch.channelType == AnimV20::CHTYPE_POSITION)
                  for (int i = 0; i < pt3x.size(); i++)
                  {
                    if (pt3x[i].t < t0 || pt3x[i].t > t1)
                      continue;
                    pt3x[i].p -= base_p;
                    pt3x[i].i -= base_p;
                    pt3x[i].o -= base_p;
                  }
                else if (ch.channelType == AnimV20::CHTYPE_SCALE && base_p.x && base_p.y && base_p.z)
                  for (int i = 0; i < pt3x.size(); i++)
                  {
                    if (pt3x[i].t < t0 || pt3x[i].t > t1)
                      continue;
                    pt3x[i].p.set(pt3x[i].p.x / base_p.x, pt3x[i].p.y / base_p.y, pt3x[i].p.z / base_p.z);
                    pt3x[i].i.set(pt3x[i].i.x / base_p.x, pt3x[i].i.y / base_p.y, pt3x[i].i.z / base_p.z);
                    pt3x[i].o.set(pt3x[i].o.x / base_p.x, pt3x[i].o.y / base_p.y, pt3x[i].o.z / base_p.z);
                  }
                break;

              case AnimV20::DATATYPE_QUAT:
                v_st(&base_q, interp_quat(anim, tref, thint));
                base_q = normalize(inverse(base_q));

                for (int i = 0; i < quat.size(); i++)
                {
                  if (quat[i].t < t0 || quat[i].t > t1)
                    continue;
                  quat[i].p = base_q * quat[i].p;
                  quat[i].i = base_q * quat[i].i;
                  quat[i].o = base_q * quat[i].o;
                }
                break;
            }
          }

          // clear keys outside anim ranges
          if (ch.dataType == AnimV20::DATATYPE_POINT3X)
            for (int i = 0; i < pt3x.size(); i++)
            {
              bool found = false;
              for (int ai = 0; ai < addAnimRangeTriple.size(); ai += 3)
                if (pt3x[i].t >= addAnimRangeTriple[ai + 0] && pt3x[i].t <= addAnimRangeTriple[ai + 1])
                {
                  found = true;
                  break;
                }
              if (found)
                continue;
              if (ch.channelType == AnimV20::CHTYPE_POSITION)
                pt3x[i].p = pt3x[i].i = pt3x[i].o = Point3(0, 0, 0);
              else if (ch.channelType == AnimV20::CHTYPE_SCALE)
                pt3x[i].p = pt3x[i].i = pt3x[i].o = Point3(1, 1, 1);
            }
          else if (ch.dataType == AnimV20::DATATYPE_QUAT)
            for (int i = 0; i < quat.size(); i++)
            {
              bool found = false;
              for (int ai = 0; ai < addAnimRangeTriple.size(); ai += 3)
                if (quat[i].t >= addAnimRangeTriple[ai + 0] && quat[i].t <= addAnimRangeTriple[ai + 1])
                {
                  found = true;
                  break;
                }
              if (found)
                continue;
              quat[i].p = quat[i].i = quat[i].o = Quat(0, 0, 0, 1);
            }

          switch (ch.dataType)
          {
            case AnimV20::DATATYPE_POINT3X:
              animopt::convert_to_OldAnimKeyPoint3(reinterpret_cast<OldAnimKeyPoint3 *>(anim.key), anim.keyTime, pt3x.data(),
                pt3x.size());
              break;
            case AnimV20::DATATYPE_QUAT:
              animopt::convert_to_OldAnimKeyQuat(reinterpret_cast<OldAnimKeyQuat *>(anim.key), anim.keyTime, quat.data(), quat.size());
              break;
          }
        }
      }
    }

    // optimize tracks
    int timeLength = 0;
    const int timeStep = sample_rate > 0 ? roundf(4800.0f / sample_rate) : minStep;

    for (auto &ch : chan)
      for (int i = 0; i < ch.nodeNum; i++)
      {
        auto &anim = ch.nodeAnim[i];
        if (anim.keyNum <= 0)
          continue;

        int tlen = anim.keyTime[anim.keyNum - 1];

        switch (ch.dataType)
        {
          case AnimV20::DATATYPE_POINT3X:
          {
            auto keys = (OldAnimKeyPoint3 *)anim.key;
            auto p0 = v_ldu_p3(&keys[0].p.x);
            auto thr = v_sqr(v_splats(ch.channelType == AnimV20::CHTYPE_SCALE ? sclPrecision : pos_prec));
            bool isConst = true;
            int hint = INITIAL_INTERP_HINT;
            for (int t = timeStep / 2; t <= tlen; t += timeStep / 2)
              if (!v_test_vec_x_eqi_0(v_cmp_gt(v_length3_sq(v_sub(interp_point3(anim, t, hint), p0)), thr)))
              {
                isConst = false;
                break;
              }

            if (isConst)
            {
              debug("removing ~const %s keys for %s", (ch.channelType == AnimV20::CHTYPE_SCALE ? "scl" : "pos"),
                namePool.data() + (intptr_t)ch.nodeName[i]);
              anim.keyNum = 1;
            }
            break;
          }
          case AnimV20::DATATYPE_QUAT:
          {
            auto keys = (OldAnimKeyQuat *)anim.key;
            auto q0 = v_ldu(&keys[0].p.x);
            q0 = v_norm4_safe(q0, V_C_UNIT_0001);

            auto thr = v_splats(quatEps);
            bool isConst = true;
            int hint = INITIAL_INTERP_HINT;
            for (int t = timeStep / 2; t <= tlen; t += timeStep / 2)
            {
              auto q = interp_quat(anim, t, hint);
              q = v_norm4_safe(q, V_C_UNIT_0001);
              if (!rot_equal(q, q0, thr))
              {
                isConst = false;
                break;
              }
            }

            if (isConst)
            {
              debug("removing ~const rot keys for %s", namePool.data() + (intptr_t)ch.nodeName[i]);
              anim.keyNum = 1;
            }
            break;
          }
        }

        int t = anim.keyTime[anim.keyNum - 1];
        if (timeLength < t)
          timeLength = t;
      }

    // compute sample rate and length
    const float sampleRate = 4800.0f / timeStep;
    const int numSamples = (timeLength + timeStep / 2) / timeStep + 1;
    debug("ACL length: %d sample rate: %g, number of samples: %d", timeLength, sampleRate, numSamples);
    if ((numSamples - 1) * timeStep != timeLength)
      debug("WARNING: last sample doesn't match original end time exactly");

    // find PRS tracks
    ChannelData *posCh = nullptr;
    ChannelData *rotCh = nullptr;
    ChannelData *sclCh = nullptr;

    for (auto &ch : chan)
      switch (ch.channelType)
      {
        case AnimV20::CHTYPE_POSITION:
          if (posCh)
            log.addMessage(ILogWriter::ERROR, "two or more channels of 'position' type found");
          else if (ch.dataType != AnimV20::DATATYPE_POINT3X)
            log.addMessage(ILogWriter::ERROR, "'position' channel has invalid data type %d", ch.dataType);
          else
          {
            posCh = &ch;
            debug("got pos channel");
          }
          break;

        case AnimV20::CHTYPE_ROTATION:
          if (rotCh)
            log.addMessage(ILogWriter::ERROR, "two or more channels of 'rotation' type found");
          else if (ch.dataType != AnimV20::DATATYPE_QUAT)
            log.addMessage(ILogWriter::ERROR, "'rotation' channel has invalid data type %d", ch.dataType);
          else
          {
            rotCh = &ch;
            debug("got rot channel");
          }
          break;

        case AnimV20::CHTYPE_SCALE:
          if (sclCh)
            log.addMessage(ILogWriter::ERROR, "two or more channels of 'scale' type found");
          else if (ch.dataType != AnimV20::DATATYPE_POINT3X)
            log.addMessage(ILogWriter::ERROR, "'scale' channel has invalid data type %d", ch.dataType);
          else
          {
            sclCh = &ch;
            debug("got scl channel");
          }
          break;
      }

    // repack names
    FastStrMap new_names;
    int new_names_sz = 0;
    for (auto &c : chan)
      for (int j = 0; j < c.nodeNum; j++)
      {
        const char *nm = namePool.data() + (intptr_t)c.nodeName[j];
        if (new_names.getStrId(nm) < 0)
        {
          new_names.addStrId(nm, new_names_sz);
          new_names_sz += i_strlen(nm) + 1;
        }
      }
    for (auto &k : noteTrackPool)
    {
      const char *nm = &namePool[k.name];
      if (new_names.getStrId(nm) < 0)
      {
        new_names.addStrId(nm, new_names_sz);
        new_names_sz += i_strlen(nm) + 1;
      }
    }

    if (new_names_sz < namePool.size())
    {
      debug("remap namePool: %d -> %d bytes", namePool.size(), new_names_sz);
      Tab<char> new_pool;
      new_pool.resize(new_names_sz);
      for (auto &id : new_names.getMapRaw())
        memcpy(&new_pool[id.id], id.name, strlen(id.name) + 1);

      for (auto &k : noteTrackPool)
        k.name = new_names.getStrId(&namePool[k.name]);
      for (auto &c : chan)
        for (int j = 0; j < c.nodeNum; j++)
          *(intptr_t *)&c.nodeName[j] = new_names.getStrId(&namePool[(intptr_t)c.nodeName[j]]);

      namePool = eastl::move(new_pool);
      new_names.reset();
    }

    for (auto &ch : chan)
      for (int i = 1; i < ch.nodeNum; i++)
        for (int j = 0; j < i; j++)
          if (ch.nodeName[i] == ch.nodeName[j])
            log.addMessage(errorOnDuplicateNames ? ILogWriter::ERROR : ILogWriter::WARNING, "%s: duplicate node name '%s'",
              a.getNameTypified(), namePool.data() + (intptr_t)ch.nodeName[i]);

    //
    // write ANIM v300 dump
    //
    Tab<float> tmpKeys(tmpmem);
    int start_ofs, name_add_ofs, chan_hdr_ofs, note_ofs, dumpSz;
    int has_non_even_time = 0;

    start_ofs = cwr.tell();
    cwr.writeFourCC(hdr.label);
    cwr.writeInt32e(make_additive ? 0x301 : 0x300);
    cwr.writeInt32e(16);
    cwr.writeInt32e(0);

    cwr.setOrigin();

    // clear extra data in the last key
    for (int channelId = 0; channelId < chan.size(); channelId++)
    {
      ChannelData &ch = chan[channelId];
      ch.nodeAnimOfs = -1;

      for (int i = 0; i < ch.nodeNum; i++)
      {
        int last = ch.nodeAnim[i].keyNum - 1;
        if (last < 0)
          continue;
        switch (ch.dataType)
        {
          case AnimV20::DATATYPE_POINT3X:
            if (OldAnimKeyPoint3 *lk = ((OldAnimKeyPoint3 *)ch.nodeAnim[i].key) + last)
              lk->k1 = lk->k2 = lk->k3 = Point3(0, 0, 0);
            break;
          case AnimV20::DATATYPE_QUAT:
            if (OldAnimKeyQuat *lk = ((OldAnimKeyQuat *)ch.nodeAnim[i].key) + last)
              lk->b0 = lk->b1 = Quat(0, 0, 0, 0);
            break;
          case AnimV20::DATATYPE_REAL:
            if (OldAnimKeyReal *lk = ((OldAnimKeyReal *)ch.nodeAnim[i].key) + last)
              lk->k1 = lk->k2 = lk->k3 = 0;
            break;
        }
      }
    }

    // use original skeleton to convert all animation transformations from parent node tm to node tm
    AclAllocator alloc(tmpmem);

    auto checkNeedConvert = [original_skeleton, auto_completed_skeleton](const char *nodeName, bool &convertAnim,
                              TMatrix &origParentTm, TMatrix &parentTmInv) {
      if (original_skeleton != nullptr && auto_completed_skeleton != nullptr)
      {
        if (const Node *origNode = original_skeleton->find_node(nodeName))
          if (const Node *origParentNode = origNode->parent)
          {
            origParentTm = origParentNode->wtm;
            if (const Node *node = auto_completed_skeleton->find_node(nodeName))
              if (const Node *parentNode = node->parent)
              {
                if (strcmp(origParentNode->name.str(), parentNode->name.str()) != 0)
                {
                  parentTmInv = inverse(parentNode->wtm);
                  convertAnim = true;
                }
              }
          }
      }
    };

    auto freeNodeAnim = [&timeKeyPool](ChannelData::Anim &anim) {
      if ((void *)anim.keyTime < timeKeyPool.data() || (void *)anim.keyTime >= timeKeyPool.data() + timeKeyPool.size())
        memfree(anim.keyTime, tmpmem);
      anim.keyTime = nullptr;

      if (anim.key < timeKeyPool.data() || anim.key >= timeKeyPool.data() + timeKeyPool.size())
        memfree(anim.key, tmpmem);
      anim.key = nullptr;
    };

    // compress Quat channels first
    int qstart, qnum;
    ChannelData::findDataTypeRange(AnimV20::DATATYPE_QUAT, chan, qstart, qnum);
    for (int channelId = qstart; channelId < qstart + qnum; channelId++)
    {
      auto &ch = chan[channelId];
      acl::track_array_qvvf trackArray(alloc, ch.nodeNum);

      for (int i = 0; i < ch.nodeNum; i++)
      {
        bool convertAnim = false;
        TMatrix origParentTm = TMatrix::IDENT;
        TMatrix parentTmInv = TMatrix::IDENT;
        const char *nodeName = namePool.data() + (intptr_t)ch.nodeName[i];
        checkNeedConvert(nodeName, convertAnim, origParentTm, parentTmInv);

        int parentIndex = -1;
        auto findParentIndex = [&parentIndex, &ch, &namePool](const Node *parent) {
          if (!parent)
            return;
          for (int pi = 0; pi < ch.nodeNum; pi++)
            if (strcmp(parent->name.str(), namePool.data() + (intptr_t)ch.nodeName[pi]) == 0)
            {
              parentIndex = pi;
              break;
            }
        };

        if (auto_completed_skeleton)
          if (auto node = auto_completed_skeleton->find_node(nodeName))
            findParentIndex(node->parent);

        if (parentIndex == -1 && original_skeleton)
          if (auto node = original_skeleton->find_node(nodeName))
            findParentIndex(node->parent);

        int keyNum = ch.nodeAnim[i].keyNum;

        ChannelData::Anim rotAnim = ch.nodeAnim[i];
        ChannelData::Anim posAnim, sclAnim;
        memset(&posAnim, 0, sizeof(posAnim));
        memset(&sclAnim, 0, sizeof(sclAnim));

        ch.nodeAnim[i].keyOfs = i;

        if (&ch == rotCh)
        {
          if (int j = find_node_index(posCh, ch.nodeName[i], i); j >= 0)
          {
            posAnim = posCh->nodeAnim[j];
            posCh->nodeAnim[j].keyOfs = i;
          }
          if (int j = find_node_index(sclCh, ch.nodeName[i], i); j >= 0)
          {
            sclAnim = sclCh->nodeAnim[j];
            sclCh->nodeAnim[j].keyOfs = i;
          }
        }

        if (convertAnim)
        {
          // convert all PRS tracks at once
          int dataSize = keyNum * sizeof(OldAnimKeyQuat) / sizeof(tmpKeys[0]);
          if (posAnim.keyNum > 0)
            dataSize += posAnim.keyNum * sizeof(OldAnimKeyPoint3) / sizeof(tmpKeys[0]);
          if (sclAnim.keyNum > 0)
            dataSize += sclAnim.keyNum * sizeof(OldAnimKeyPoint3) / sizeof(tmpKeys[0]);
          tmpKeys.resize(dataSize);

          auto rotKeys = make_span((OldAnimKeyQuat *)tmpKeys.data(), keyNum);
          mem_set_0(rotKeys);
          rotAnim.key = rotKeys.data();

          for (int j = 0; j < keyNum; j++)
          {
            const OldAnimKeyQuat &old = ((OldAnimKeyQuat *)ch.nodeAnim[i].key)[j];

            OldAnimKeyQuat &newKey = rotKeys[j];
            TMatrix ptm;
            ptm.makeTM(old.p);
            TMatrix b0tm;
            b0tm.makeTM(old.b0);
            TMatrix b1tm;
            b1tm.makeTM(old.b1);

            TMatrix tm1 = (parentTmInv % (origParentTm % ptm));
            tm1.orthonormalize();

            TMatrix tm2 = (parentTmInv % (origParentTm % b0tm));
            tm2.orthonormalize();

            TMatrix tm3 = (parentTmInv % (origParentTm % b1tm));
            tm3.orthonormalize();

            newKey.p = Quat(tm1);
            newKey.b0 = Quat(tm2);
            newKey.b1 = Quat(tm3);
          }

          void *end = rotKeys.end();
          if (posAnim.keyNum > 0)
          {
            auto keys = make_span((OldAnimKeyPoint3 *)end, posAnim.keyNum);
            end = keys.end();

            for (int j = 0; j < posAnim.keyNum; j++)
            {
              const OldAnimKeyPoint3 &old = ((OldAnimKeyPoint3 *)posAnim.key)[j];

              Point3 p = old.p, k1 = old.k1, k2 = old.k2, k3 = old.k3;
              p = parentTmInv * (origParentTm * old.p);
              k1 = parentTmInv % (origParentTm % old.k1);
              k2 = parentTmInv % (origParentTm % old.k2);
              k3 = parentTmInv % (origParentTm % old.k3);

              keys[j].p = p;
              keys[j].k1 = k1;
              keys[j].k2 = k2;
              keys[j].k3 = k3;
            }

            posAnim.key = keys.data();
          }

          if (sclAnim.keyNum > 0)
          {
            auto keys = make_span((OldAnimKeyPoint3 *)end, sclAnim.keyNum);

            for (int j = 0; j < sclAnim.keyNum; j++)
            {
              const OldAnimKeyPoint3 &old = ((OldAnimKeyPoint3 *)sclAnim.key)[j];

              Point3 p = old.p, k1 = old.k1, k2 = old.k2, k3 = old.k3;
              p = get_tm_scale(parentTmInv % (origParentTm % make_scale_tm(TMatrix::IDENT, old.p)));
              k1 = get_tm_scale(parentTmInv % (origParentTm % make_scale_tm(TMatrix::IDENT, old.k1)));
              k2 = get_tm_scale(parentTmInv % (origParentTm % make_scale_tm(TMatrix::IDENT, old.k2)));
              k3 = get_tm_scale(parentTmInv % (origParentTm % make_scale_tm(TMatrix::IDENT, old.k3)));

              keys[j].p = p;
              keys[j].k1 = k1;
              keys[j].k2 = k2;
              keys[j].k3 = k3;
            }

            sclAnim.key = keys.data();
          }
        }

        // make ACL track
        acl::track_desc_transformf desc;
        desc.output_index = i;
        if (parentIndex != -1)
          desc.parent_index = parentIndex;
        desc.shell_distance = shellDistance;
        desc.precision = qvvfPrecision;

        auto tr = acl::track_qvvf::make_reserve(desc, alloc, numSamples, sampleRate);

        int posHint = INITIAL_INTERP_HINT;
        int rotHint = INITIAL_INTERP_HINT;
        int sclHint = INITIAL_INTERP_HINT;
        for (int si = 0; si < numSamples; si++)
        {
          int t = si * timeStep;
          auto q = interp_quat(rotAnim, t, rotHint);
          auto p = v_zero();
          auto s = V_C_ONE;
          if (posAnim.keyNum > 0)
            p = interp_point3(posAnim, t, posHint);
          if (sclAnim.keyNum > 0)
            s = interp_point3(sclAnim, t, sclHint);
          tr[si].rotation = q;
          tr[si].translation = p;
          tr[si].scale = s;
        }

        trackArray[i] = eastl::move(tr);

        freeNodeAnim(ch.nodeAnim[i]);
      }

      // compress
      acl::qvvf_transform_error_metric errorMetric;
      {
        acl::pre_process_settings_t settings;
        settings.precision_policy = acl::pre_process_precision_policy::lossy;
        settings.error_metric = &errorMetric;
        pre_process_track_list(alloc, settings, trackArray);
      }

      {
        acl::compression_settings settings;
        settings.level = acl::compression_level8::highest;
        settings.rotation_format = acl::rotation_format8::quatf_drop_w_variable;
        settings.translation_format = acl::vector_format8::vector3f_variable;
        settings.scale_format = acl::vector_format8::vector3f_variable;
        settings.error_metric = &errorMetric;

        acl::output_stats stats;
        acl::compressed_tracks *compressedTracks = nullptr;
        FINALLY([&] {
          if (compressedTracks)
            alloc.deallocate(compressedTracks, compressedTracks->get_size());
        });

        acl::error_result result = acl::compress_track_list(alloc, trackArray, settings, compressedTracks, stats);
        if (result.any() || compressedTracks == nullptr)
        {
          log.addMessage(ILogWriter::ERROR, "error ACL-compressing: %s", result.c_str());
          return false;
        }

        cwr.align16();
        ch.nodeAnimOfs = cwr.tell();
        cwr.writeRaw(compressedTracks, compressedTracks->get_size());

        if (&ch == rotCh)
        {
          if (posCh)
            posCh->nodeAnimOfs = ch.nodeAnimOfs;
          if (sclCh)
            sclCh->nodeAnimOfs = ch.nodeAnimOfs;
        }
      }
    }

    // compress Point3 channels
    int p3start, p3num;
    ChannelData::findDataTypeRange(AnimV20::DATATYPE_POINT3X, chan, p3start, p3num);
    for (int channelId = p3start; channelId < p3start + p3num; channelId++)
    {
      ChannelData &ch = chan[channelId];
      if (ch.nodeAnimOfs != -1)
        continue;

      acl::track_array_float3f trackArray3f(alloc, ch.nodeNum);

      for (int i = 0; i < ch.nodeNum; i++)
      {
        bool convertAnim = false;
        TMatrix origParentTm = TMatrix::IDENT;
        TMatrix parentTmInv = TMatrix::IDENT;
        checkNeedConvert(namePool.data() + (intptr_t)ch.nodeName[i], convertAnim, origParentTm, parentTmInv);

        int keyNum = ch.nodeAnim[i].keyNum;
        ch.nodeAnim[i].keyOfs = i;

        dag::Span<OldAnimKeyPoint3> keys;

        if (convertAnim)
        {
          tmpKeys.resize(keyNum * sizeof(OldAnimKeyPoint3) / sizeof(tmpKeys[0]));
          keys = make_span((OldAnimKeyPoint3 *)tmpKeys.data(), keyNum);

          for (int j = 0; j < keyNum; j++)
          {
            const OldAnimKeyPoint3 &old = ((OldAnimKeyPoint3 *)ch.nodeAnim[i].key)[j];

            Point3 p = old.p, k1 = old.k1, k2 = old.k2, k3 = old.k3;
            if (ch.channelType == AnimV20::CHTYPE_POSITION)
            {
              p = parentTmInv * (origParentTm * old.p);
              k1 = parentTmInv % (origParentTm % old.k1);
              k2 = parentTmInv % (origParentTm % old.k2);
              k3 = parentTmInv % (origParentTm % old.k3);
            }
            else if (ch.channelType == AnimV20::CHTYPE_ORIGIN_LINVEL || ch.channelType == AnimV20::CHTYPE_ORIGIN_ANGVEL)
            {
              p = parentTmInv % (origParentTm % old.p);
              k1 = parentTmInv % (origParentTm % old.k1);
              k2 = parentTmInv % (origParentTm % old.k2);
              k3 = parentTmInv % (origParentTm % old.k3);
            }
            else if (ch.channelType == AnimV20::CHTYPE_SCALE)
            {
              p = get_tm_scale(parentTmInv % (origParentTm % make_scale_tm(TMatrix::IDENT, old.p)));
              k1 = get_tm_scale(parentTmInv % (origParentTm % make_scale_tm(TMatrix::IDENT, old.k1)));
              k2 = get_tm_scale(parentTmInv % (origParentTm % make_scale_tm(TMatrix::IDENT, old.k2)));
              k3 = get_tm_scale(parentTmInv % (origParentTm % make_scale_tm(TMatrix::IDENT, old.k3)));
            }

            keys[j].p = p;
            keys[j].k1 = k1;
            keys[j].k2 = k2;
            keys[j].k3 = k3;
          }
          // scale origin vel due to sampling rate changed from 4800 to (4800/RATE_DIV)<<TIME_SubdivExp
          if (ch.channelType == AnimV20::CHTYPE_ORIGIN_LINVEL || ch.channelType == AnimV20::CHTYPE_ORIGIN_ANGVEL)
            for (int j = 0; j < tmpKeys.size(); j++)
              tmpKeys[j] = tmpKeys[j] * RATE_DIV / (1 << AnimV20::TIME_SubdivExp);
        }
        else if (ch.channelType == AnimV20::CHTYPE_ORIGIN_LINVEL || ch.channelType == AnimV20::CHTYPE_ORIGIN_ANGVEL)
        {
          tmpKeys.resize(keyNum * sizeof(OldAnimKeyPoint3) / sizeof(tmpKeys[0]));
          keys = make_span((OldAnimKeyPoint3 *)tmpKeys.data(), keyNum);
          mem_copy_from(tmpKeys, ch.nodeAnim[i].key);

          // scale origin vel due to sampling rate changed from 4800 to (4800/RATE_DIV)<<TIME_SubdivExp
          for (int j = 0; j < tmpKeys.size(); j++)
            tmpKeys[j] = tmpKeys[j] * RATE_DIV / (1 << AnimV20::TIME_SubdivExp);
        }
        else
        {
          keys = make_span((OldAnimKeyPoint3 *)ch.nodeAnim[i].key, keyNum);
        }

        // make ACL track
        acl::track_desc_scalarf desc;
        desc.output_index = i;

        if (ch.channelType == AnimV20::CHTYPE_ORIGIN_LINVEL)
          desc.precision = vel_prec;
        else if (ch.channelType == AnimV20::CHTYPE_ORIGIN_ANGVEL)
          desc.precision = angVelPrecision;
        else if (ch.channelType == AnimV20::CHTYPE_SCALE)
          desc.precision = sclPrecision;
        else
          desc.precision = pos_prec;

        auto tr = acl::track_float3f::make_reserve(desc, alloc, numSamples, sampleRate);
        ChannelData::Anim anim = ch.nodeAnim[i];
        anim.key = keys.data();

        int hint = INITIAL_INTERP_HINT;
        for (int si = 0; si < numSamples; si++)
        {
          rtm::vector_store3(interp_point3(anim, si * timeStep, hint), &tr[si]);
        }

        trackArray3f[i] = eastl::move(tr);

        freeNodeAnim(ch.nodeAnim[i]);
      }

      // compress
      {
        acl::pre_process_settings_t settings;
        settings.precision_policy = acl::pre_process_precision_policy::lossy;
        pre_process_track_list(alloc, settings, trackArray3f);
      }

      {
        acl::compression_settings settings;
        settings.level = acl::compression_level8::highest;

        acl::output_stats stats;
        acl::compressed_tracks *compressedTracks = nullptr;
        FINALLY([&] {
          if (compressedTracks)
            alloc.deallocate(compressedTracks, compressedTracks->get_size());
        });

        acl::error_result result = acl::compress_track_list(alloc, trackArray3f, settings, compressedTracks, stats);
        if (result.any() || compressedTracks == nullptr)
        {
          log.addMessage(ILogWriter::ERROR, "error ACL-compressing: %s", result.c_str());
          return false;
        }

        cwr.align16();
        ch.nodeAnimOfs = cwr.tell();
        cwr.writeRaw(compressedTracks, compressedTracks->get_size());
      }
    }

    name_add_ofs = cwr.tell();
    cwr.writeRaw(namePool.data(), data_size(namePool));
    cwr.align8();
    for (int channelId = 0; channelId < chan.size(); channelId++)
    {
      ChannelData &ch = chan[channelId];
      ch.nodeNameOfs = cwr.tell();
      for (int i = 0; i < ch.nodeNum; i++)
      {
        ch.nodeName[i] += name_add_ofs;
        cwr.writePtr64e(ptrdiff_t(ch.nodeName[i]));
      }
    }

    cwr.align8();
    for (int channelId = 0; channelId < chan.size(); channelId++)
    {
      ChannelData &ch = chan[channelId];
      ch.nodeTrackOfs = cwr.tell();
      for (int i = 0; i < ch.nodeNum; i++)
        cwr.writeInt16e(ch.nodeAnim[i].keyOfs);
    }

    cwr.align16();
    for (int channelId = 0; channelId < chan.size(); channelId++)
    {
      ChannelData &ch = chan[channelId];
      ch.nodeWtOfs = cwr.tell();
      cwr.write32ex(ch.nodeWt, ch.nodeNum * sizeof(float));
    }
    cwr.align16();

    chan_hdr_ofs = cwr.tell();
    for (int channelId = 0; channelId < chan.size(); channelId++)
    {
      ChannelData &ch = chan[channelId];
      cwr.writePtr64e(ch.nodeAnimOfs);
      cwr.writePtr64e(ch.nodeTrackOfs);
      cwr.writeInt32e(ch.nodeNum);
      cwr.writeInt32e(ch.channelType);
      cwr.writePtr64e(ch.nodeWtOfs);
      cwr.writePtr64e(ch.nodeNameOfs);
    }

    note_ofs = cwr.tell();
    for (int i = 0; i < hdrV200.totalAnimLabelNum; i++)
    {
      noteTrackPool[i].name += name_add_ofs;
      int t = noteTrackPool[i].time;
      noteTrackPool[i].time = ((noteTrackPool[i].time + RATE_DIV / 2) / RATE_DIV);
      noteTrackPool[i].time <<= AnimV20::TIME_SubdivExp;

      if (t != (noteTrackPool[i].time >> AnimV20::TIME_SubdivExp) * RATE_DIV)
        has_non_even_time++;
      cwr.writePtr64e(noteTrackPool[i].name);
      cwr.writeInt32e(noteTrackPool[i].time);
      cwr.writeInt32e(0);
    }

    dumpSz = cwr.tell();
    cwr.popOrigin();
    cwr.writeInt32eAt(dumpSz, start_ofs + 12);

    static const int CHAN_REC_SZ = mkbindump::BinDumpSaveCB::PTR_SZ * 4 + 4 * 2;
    cwr.writeRef(p3num ? chan_hdr_ofs + p3start * CHAN_REC_SZ : -1, p3num);
    cwr.writeRef(qnum ? chan_hdr_ofs + qstart * CHAN_REC_SZ : -1, qnum);

    int dt_start, dt_num;
    ChannelData::findDataTypeRange(AnimV20::DATATYPE_REAL, chan, dt_start, dt_num);
    cwr.writeRef(dt_num ? chan_hdr_ofs + dt_start * CHAN_REC_SZ : -1, dt_num);
    if (dt_num > 0)
      log.addMessage(ILogWriter::ERROR, "real-typed channels are not supported by ACL exporter yet");

    cwr.writeRef(note_ofs, hdrV200.totalAnimLabelNum);

    if (has_non_even_time)
      debug("found %d non-even time keys", has_non_even_time);
    return true;
  }
};

class A2dExporterPlugin : public IDaBuildPlugin
{
public:
  virtual bool __stdcall init(const DataBlock &appblk)
  {
    const DataBlock *a2dBlk = appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("a2d");

    preferZstdPacking = a2dBlk->getBool("preferZSTD", false);
    if (preferZstdPacking)
      debug("a2d prefers ZSTD");
    allowOodlePacking = a2dBlk->getBool("allowOODLE", false);
    if (allowOodlePacking)
      debug("a2d allows OODLE");
    logExportTime = a2dBlk->getBool("logExportTime", false);
    if (logExportTime)
      debug("a2d logExportTime");
    errorOnDuplicateNames = a2dBlk->getBool("errorOnDuplicateNames", false);
    if (errorOnDuplicateNames)
      debug("a2d errorOnDuplicateNames");
    errorOnOldParams = a2dBlk->getBool("errorOnOldParams", false);
    if (errorOnOldParams)
      debug("a2d errorOnOldParams");
    return true;
  }
  virtual void __stdcall destroy() { delete this; }

  virtual int __stdcall getExpCount() { return 1; }
  virtual const char *__stdcall getExpType(int idx) { return TYPE; }
  virtual IDagorAssetExporter *__stdcall getExp(int idx) { return &exp; }

  virtual int __stdcall getRefProvCount() { return 0; }
  virtual const char *__stdcall getRefProvType(int idx) { return NULL; }
  virtual IDagorAssetRefProvider *__stdcall getRefProv(int idx) { return NULL; }

protected:
  A2dExporter exp;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) A2dExporterPlugin; }

END_DABUILD_PLUGIN_NAMESPACE(a2d)
REGISTER_DABUILD_PLUGIN(a2d, nullptr)
