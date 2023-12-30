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
#include <libTools/dagFileRW/dagFileNode.h>
#include "exp_skeleton_tools.h"
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_btagCompr.h>

BEGIN_DABUILD_PLUGIN_NAMESPACE(a2d)
static const char *TYPE = "a2d";
static bool preferZstdPacking = false;
static bool allowOodlePacking = false;

static TMatrix make_scale_tm(const TMatrix &tm, const Point3 &scale)
{
  TMatrix res = tm;
  res.setcol(0, res.getcol(0) * scale.x);
  res.setcol(1, res.getcol(1) * scale.y);
  res.setcol(2, res.getcol(2) * scale.z);
  return res;
}

static Point3 get_tm_scale(const TMatrix &tm) { return Point3(tm.getcol(0).length(), tm.getcol(1).length(), tm.getcol(2).length()); }

class A2dExporter : public IDagorAssetExporter
{
public:
  virtual const char *__stdcall getExporterIdStr() const { return "a2d exp"; }

  virtual const char *__stdcall getAssetType() const { return TYPE; }
  virtual unsigned __stdcall getGameResClassId() const { return Anim2DataGameResClassId; }
  virtual unsigned __stdcall getGameResVersion() const
  {
    static constexpr const int base_ver = 4;
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
    float pos_eps = -1, rot_eps = -1, scl_eps = -1;
    int rot_resample_freq = 0;
    if (props.getBool("opt", a.props.getBool("opt", false)))
    {
      pos_eps = props.getReal("posEps", a.props.getReal("posEps", -1));
      rot_eps = props.getReal("rotEps", a.props.getReal("rotEps", -1));
      scl_eps = props.getReal("sclEps", a.props.getReal("sclEps", -1));
      rot_resample_freq = props.getInt("rotResampleFreq", a.props.getInt("rotResampleFreq", 0));
      debug("%s: will optimize tracks, PRSeps=%.5fm, %.5fdeg, %.5f,  resampleR=%d fps", a.getName(), pos_eps, rot_eps, scl_eps,
        rot_resample_freq);
    }

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
    if (!convert(crd, cwrDump, pos_eps, rot_eps, scl_eps, rot_resample_freq, props.getBool("makeAdditive", false),
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


  static bool convert(IGenLoad &crd, mkbindump::BinDumpSaveCB &cwr, float pos_eps, float rot_eps, float scl_eps, int rot_resample_freq,
    bool make_additive, const char *additive_key_suffix, FastNameMap &req_node_mask, Node *original_skeleton,
    Node *auto_completed_skeleton, bool strip_for_char_dep, FastNameMap &char_dep_nodes, const DagorAsset &a, ILogWriter &log)
  {
    AnimDataHeader hdr;

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
    int mint = 0, maxt = 0;
    if (hdrV200.timeNum)
      mint = maxt = timePool[0];
    for (int i = 1; i < hdrV200.timeNum; i++)
      if (mint > timePool[i])
        mint = timePool[i];
      else if (maxt < timePool[i])
        maxt = timePool[i];

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
    G_ASSERT(unsigned(maxt - mint) / RATE_DIV < 65536);

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
          ch.nodeAnim[i].keyOfs = ch.nodeAnim[i].keyTimeOfs = -1;
          ch.nodeAnim[i].keyNum = keyNum;
          ch.nodeAnim[i].key = keyPool;
          ch.nodeAnim[i].keyTime = timePool;
          keyPool += keyNum * keysz;
          timePool += keyNum;
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
            ch2.dataType = ch.dataType;
            ch2.channelType = ch.channelType;
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

            ch.clear();
            memcpy(&ch, &ch2, sizeof(ch)); //-V780
          }
          memset(&ch2, 0, sizeof(ch2)); //-V780
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
                v_st(&base_p, interp_point3(anim, tref));
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
                v_st(&base_q, interp_quat(anim, tref));
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

    //
    // optimize ANIM tracks
    //
    optimize_keys(make_span(chan), pos_eps, rot_eps, scl_eps, rot_resample_freq);

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

    //
    // write ANIM v210 dump
    //
    Tab<unsigned short> tmpTimeKeys(tmpmem);
    Tab<float> tmpKeys(tmpmem);
    int start_ofs, name_add_ofs, chan_hdr_ofs, note_ofs, dumpSz;
    int has_non_even_time = 0;

    start_ofs = cwr.tell();
    cwr.writeFourCC(hdr.label);
    cwr.writeInt32e(make_additive ? 0x221 : 0x220);
    cwr.writeInt32e(16);
    cwr.writeInt32e(0);

    cwr.setOrigin();
    for (int channelId = 0; channelId < chan.size(); channelId++)
    {
      ChannelData &ch = chan[channelId];
      for (int i = 0; i < ch.nodeNum; i++)
      {
        tmpTimeKeys.resize(ch.nodeAnim[i].keyNum);
        for (int j = 0; j < ch.nodeAnim[i].keyNum; j++)
        {
          tmpTimeKeys[j] = (ch.nodeAnim[i].keyTime[j] + RATE_DIV / 2) / RATE_DIV;
          G_ASSERTF(j == 0 || tmpTimeKeys[j] != tmpTimeKeys[j - 1], "tmpTimeKeys[%d]=%d tmpTimeKeys[%d]=%d", j, tmpTimeKeys[j], j - 1,
            tmpTimeKeys[j - 1]);
          if (ch.nodeAnim[i].keyTime[j] != tmpTimeKeys[j] * RATE_DIV)
            has_non_even_time++;
        }
        ch.nodeAnim[i].keyTimeOfs = cwr.tell();
        cwr.writeTabData16ex(tmpTimeKeys);

        if ((void *)ch.nodeAnim[i].keyTime < timeKeyPool.data() ||
            (void *)ch.nodeAnim[i].keyTime >= timeKeyPool.data() + timeKeyPool.size())
          memfree(ch.nodeAnim[i].keyTime, tmpmem);
        ch.nodeAnim[i].keyTime = nullptr;
      }
    }
    cwr.align16();

    // clear extra data in the last key
    for (int channelId = 0; channelId < chan.size(); channelId++)
    {
      ChannelData &ch = chan[channelId];
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
    for (int channelId = 0; channelId < chan.size(); channelId++)
    {
      ChannelData &ch = chan[channelId];

      for (int i = 0; i < ch.nodeNum; i++)
      {
        const char *nodeName = namePool.data() + (intptr_t)ch.nodeName[i];

        bool convertAnim = false;
        TMatrix origParentTm = TMatrix::IDENT;
        TMatrix parentTmInv = TMatrix::IDENT;

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

        int keyNum = ch.nodeAnim[i].keyNum;
        ch.nodeAnim[i].keyOfs = cwr.tell();

        if (convertAnim)
        {
          switch (ch.dataType)
          {
            case AnimV20::DATATYPE_POINT3X:
              tmpKeys.resize(keyNum * 4 * 4);
              mem_set_0(tmpKeys);
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

                memcpy(&tmpKeys[j * 16], &p, 3 * 4);
                memcpy(&tmpKeys[j * 16 + 4], &k1, 3 * 4);
                memcpy(&tmpKeys[j * 16 + 8], &k2, 3 * 4);
                memcpy(&tmpKeys[j * 16 + 12], &k3, 3 * 4);
              }
              // scale origin vel due to sampling rate changed from 4800 to (4800/RATE_DIV)<<TIME_SubdivExp
              if (ch.channelType == AnimV20::CHTYPE_ORIGIN_LINVEL || ch.channelType == AnimV20::CHTYPE_ORIGIN_ANGVEL)
                for (int j = 0; j < tmpKeys.size(); j++)
                  tmpKeys[j] = tmpKeys[j] * RATE_DIV / (1 << AnimV20::TIME_SubdivExp);

              cwr.writeTabData32ex(tmpKeys);
              break;
            case AnimV20::DATATYPE_QUAT:
              tmpKeys.resize(keyNum * 3 * 4);
              mem_set_0(tmpKeys);
              for (int j = 0; j < keyNum; j++)
              {
                const OldAnimKeyQuat &old = ((OldAnimKeyQuat *)ch.nodeAnim[i].key)[j];

                OldAnimKeyQuat newKey;
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

                memcpy(&tmpKeys[j * 12], &newKey.p, 3 * 4 * 4); // copy quats from newKey //-V512
              }
              cwr.writeTabData32ex(tmpKeys);
              break;
            case AnimV20::DATATYPE_REAL: cwr.write32ex(ch.nodeAnim[i].key, keyNum * sizeof(OldAnimKeyReal)); break;
            default: DAG_FATAL("unsupported dataType=%d", ch.dataType);
          }
        }
        else
        {
          switch (ch.dataType)
          {
            case AnimV20::DATATYPE_POINT3X:
              tmpKeys.resize(keyNum * 4 * 4);
              mem_set_0(tmpKeys);
              for (int j = 0; j < keyNum; j++)
              {
                const OldAnimKeyPoint3 &old = ((OldAnimKeyPoint3 *)ch.nodeAnim[i].key)[j];
                memcpy(&tmpKeys[j * 16], &old.p, 3 * 4);
                memcpy(&tmpKeys[j * 16 + 4], &old.k1, 3 * 4);
                memcpy(&tmpKeys[j * 16 + 8], &old.k2, 3 * 4);
                memcpy(&tmpKeys[j * 16 + 12], &old.k3, 3 * 4);
              }
              // scale origin vel due to sampling rate changed from 4800 to (4800/RATE_DIV)<<TIME_SubdivExp
              if (ch.channelType == AnimV20::CHTYPE_ORIGIN_LINVEL || ch.channelType == AnimV20::CHTYPE_ORIGIN_ANGVEL)
                for (int j = 0; j < tmpKeys.size(); j++)
                  tmpKeys[j] = tmpKeys[j] * RATE_DIV / (1 << AnimV20::TIME_SubdivExp);

              cwr.writeTabData32ex(tmpKeys);
              break;
            case AnimV20::DATATYPE_QUAT:
              tmpKeys.resize(keyNum * 3 * 4);
              mem_set_0(tmpKeys);
              for (int j = 0; j < keyNum; j++)
              {
                const OldAnimKeyQuat &old = ((OldAnimKeyQuat *)ch.nodeAnim[i].key)[j];
                memcpy(&tmpKeys[j * 12], &old.p, 3 * 4 * 4);
              }
              cwr.writeTabData32ex(tmpKeys);
              break;
            case AnimV20::DATATYPE_REAL: cwr.write32ex(ch.nodeAnim[i].key, keyNum * sizeof(OldAnimKeyReal)); break;
            default: DAG_FATAL("unsupported dataType=%d", ch.dataType);
          }
        }

        if (ch.nodeAnim[i].key < timeKeyPool.data() || ch.nodeAnim[i].key >= timeKeyPool.data() + timeKeyPool.size())
          memfree(ch.nodeAnim[i].key, tmpmem);
        ch.nodeAnim[i].key = nullptr;
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

    cwr.align16();
    for (int channelId = 0; channelId < chan.size(); channelId++)
    {
      ChannelData &ch = chan[channelId];
      ch.nodeAnimOfs = cwr.tell();
      for (int i = 0; i < ch.nodeNum; i++)
      {
        cwr.writePtr64e(ch.nodeAnim[i].keyOfs);
        cwr.writePtr64e(ch.nodeAnim[i].keyTimeOfs);
        cwr.writeInt32e(ch.nodeAnim[i].keyNum);
        cwr.writeInt32e(0);
      }
      ch.nodeWtOfs = cwr.tell();
      cwr.write32ex(ch.nodeWt, ch.nodeNum * sizeof(float));
    }
    cwr.align16();

    chan_hdr_ofs = cwr.tell();
    for (int channelId = 0; channelId < chan.size(); channelId++)
    {
      ChannelData &ch = chan[channelId];
      cwr.writePtr64e(ch.nodeAnimOfs);
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

    static const int CHAN_REC_SZ = mkbindump::BinDumpSaveCB::PTR_SZ * 3 + 4 * 2;
    int dt_start, dt_num;
    ChannelData::findDataTypeRange(AnimV20::DATATYPE_POINT3X, chan, dt_start, dt_num);
    cwr.writeRef(dt_num ? chan_hdr_ofs + dt_start * CHAN_REC_SZ : -1, dt_num);

    ChannelData::findDataTypeRange(AnimV20::DATATYPE_QUAT, chan, dt_start, dt_num);
    cwr.writeRef(dt_num ? chan_hdr_ofs + dt_start * CHAN_REC_SZ : -1, dt_num);

    ChannelData::findDataTypeRange(AnimV20::DATATYPE_REAL, chan, dt_start, dt_num);
    cwr.writeRef(dt_num ? chan_hdr_ofs + dt_start * CHAN_REC_SZ : -1, dt_num);

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
    const DataBlock *collisionBlk = appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("a2d");

    preferZstdPacking = collisionBlk->getBool("preferZSTD", false);
    if (preferZstdPacking)
      debug("a2d prefers ZSTD");
    allowOodlePacking = collisionBlk->getBool("allowOODLE", false);
    if (allowOodlePacking)
      debug("a2d allows OODLE");
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
