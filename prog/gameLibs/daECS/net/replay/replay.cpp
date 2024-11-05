// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/net/netbase.h>
#include <daECS/net/connection.h>
#include <daECS/net/message.h>
#include <daECS/net/replayEvents.h>
#include <daECS/net/replay.h>
#include <EASTL/unique_ptr.h>
#include <util/dag_baseDef.h>
#include <util/dag_strUtil.h>
#include <daNet/getTime.h>
#include <ioSys/dag_asyncWrite.h>
#include <osApiWrappers/dag_files.h>
#include <daNet/bitStream.h>
#include <daNet/messageIdentifiers.h>
#include <string.h>
#include <stdlib.h> // alloca
#include <memory/dag_framemem.h>
#include <supp/dag_alloca.h>

#define NET_REPLAY_ECS_EVENT ECS_REGISTER_EVENT
NET_REPLAY_ECS_EVENTS
#undef NET_REPLAY_ECS_EVENT

#if _TARGET_64BIT // TODO: also on windows in WOW64 mode
#define REPLAY_BUFFER_SIZE 512 << 10
#else
#define REPLAY_BUFFER_SIZE 128 << 10
#endif

#define RMAGIC    _MAKE4C('GJRP')
#define REC_ALIGN 4 // power of 2
static constexpr uint16_t INTERNAL_REPLAY_VERSION = 4;

// TODO: move this to shared code
#if _TARGET_64BIT
#define SAFE_STACK_ALLOC_SIZE (64 << 10)
#else
#define SAFE_STACK_ALLOC_SIZE (16 << 10)
#endif
#define SAFE_STACK_ALLOC(sz)   (((sz) <= SAFE_STACK_ALLOC_SIZE) ? alloca(sz) : framemem_ptr()->alloc(sz))
#define SAFE_STACK_FREE(p, sz) (((sz) > SAFE_STACK_ALLOC_SIZE) ? framemem_ptr()->free(p) : (void)0)

struct KeyFrameInfo
{
  uint32_t ts;
  uint32_t offset;
};

struct ReplayHdr
{
  uint32_t magic;
  uint32_t hdrSize;
  uint32_t version;
  uint32_t footerSize;
  uint32_t keyFrameTableOffset;
};

struct ReplayRecordHdr
{
  uint32_t ts;
  uint32_t size : 24;
  uint32_t isKeyFrame : 8;
};

struct ReplayWriter
{
  eastl::unique_ptr<IGenSave> wstream;
  ReplayHdr hdr;
  int lastWriteTime = 0;
  eastl::vector<KeyFrameInfo> keyFrameTable;
  net::get_replay_footer_data_cb_t getFooterCb;

  ReplayWriter(net::get_replay_footer_data_cb_t get_footer_cb) : getFooterCb(get_footer_cb) {} //-V730
  ReplayWriter(ReplayWriter &&) = default;

  ~ReplayWriter()
  {
    if (!wstream)
      return;

    const uint32_t keyFrameTableSize = (uint32_t)keyFrameTable.size();
    hdr.keyFrameTableOffset = wstream->tell();
    wstream->write(&keyFrameTableSize, sizeof(keyFrameTableSize));
    for (auto const &it : keyFrameTable)
      wstream->write(&it, sizeof(it));

    Tab<uint8_t> ftdata = getFooterCb ? getFooterCb() : Tab<uint8_t>();
    net::BaseReplayFooterData baseFooter{net::BaseReplayFooterData::MAGIC, sizeof(net::BaseReplayFooterData), lastWriteTime};
    if (!ftdata.empty()) // user provided footer
    {
      G_ASSERT(ftdata.size() >= sizeof(net::BaseReplayFooterData)); // shoud be at least BaseReplayFooterData bytes size
      memcpy(ftdata.data(), &baseFooter, sizeof(baseFooter));
      wstream->write(ftdata.data(), ftdata.size());
      hdr.footerSize = ftdata.size();
    }
    else
    {
      wstream->write(&baseFooter, sizeof(baseFooter));
      hdr.footerSize = sizeof(baseFooter);
    }
    wstream->flush();
    wstream->seekto(0);
    wstream->write(&hdr, sizeof(hdr));
  }

  bool openForWrite(IGenSave *wsave, uint16_t version)
  {
    if (!wsave)
      return false;
    wstream.reset(wsave);
    hdr = ReplayHdr{RMAGIC, sizeof(hdr), net::get_replay_proto_version(version), 0u};
    // reserve space for header
    ReplayHdr zhdr;
    memset(&zhdr, 0, sizeof(zhdr));
    wstream->write(&zhdr, sizeof(zhdr));
    return true;
  }

  bool openForWriteTemp(char *write_fname, uint16_t version)
  {
    G_ASSERT(str_ends_with(write_fname, "XXXXXX")); // dd_mkstemp requirement
    return openForWrite(create_async_writer_temp(write_fname, REPLAY_BUFFER_SIZE), version);
  }

  bool openForWrite(const char *write_fname, uint16_t version)
  {
    return openForWrite(create_async_writer(write_fname, REPLAY_BUFFER_SIZE), version);
  }

  enum class KeyFrame
  {
    No,
    Yes
  };

  void write(int cur_time, const void *data, size_t dsz, KeyFrame key_frame = KeyFrame::No)
  {
    G_FAST_ASSERT(wstream);

    lastWriteTime = cur_time;
    if (key_frame == KeyFrame::Yes)
      keyFrameTable.push_back({(uint32_t)cur_time, (uint32_t)wstream->tell()});

    size_t sz = ((int)sizeof(ReplayRecordHdr) + dsz + REC_ALIGN - 1) & ~(REC_ALIGN - 1);
    char *mem = (char *)SAFE_STACK_ALLOC(sz); // do write in one call or write will delayed (due to current double-buffer
                                              // implementation of async writer)
    ((ReplayRecordHdr *)mem)->ts = cur_time;
    ((ReplayRecordHdr *)mem)->size = dsz;
    ((ReplayRecordHdr *)mem)->isKeyFrame = key_frame == KeyFrame::Yes;
    memcpy(mem + sizeof(ReplayRecordHdr), data, dsz);
    wstream->write(mem, sz); // To consider: add something like 'writeBuffer' method to IGenSave to be able pass >1 buffers in one call
    SAFE_STACK_FREE(mem, sz);
  }
};

class ReplayWriterConnection final : public net::Connection, public ReplayWriter
{
private:
  net::dump_replay_key_frame_cb_t dump_replay_key_frame_cb;

public:
  ReplayWriterConnection(ecs::EntityManager &mgr, net::ConnectionId id_, net::get_replay_footer_data_cb_t get_footer_cb,
    net::scope_query_cb_t &&scope_query_, net::dump_replay_key_frame_cb_t dump_replay_key_frame_cb) :
    net::Connection(mgr, id_, eastl::move(scope_query_)),
    ReplayWriter(get_footer_cb),
    dump_replay_key_frame_cb(dump_replay_key_frame_cb)
  {
    net::Connection::isFiltering = false;
  }

  virtual bool isBlackHole() const override { return true; }
  virtual void sendEcho(const char *, uint32_t) override { G_ASSERTF(0, "Echo unsupported for ReplayWriterConnection"); }
  void saveKeyFrame(int cur_time)
  {
    G_ASSERT(dump_replay_key_frame_cb);
    danet::BitStream keyFrameBs(2 << 10, framemem_ptr());
    dump_replay_key_frame_cb(keyFrameBs, cur_time);
    ReplayWriter::write(cur_time, keyFrameBs.GetData(), keyFrameBs.GetNumberOfBytesUsed(), KeyFrame::Yes);
    debug("[Replay] dump key frame at %i", cur_time);
  }

  virtual bool send(int cur_time, const danet::BitStream &bs, PacketPriority, PacketReliability, uint8_t, int) override
  {
    ReplayWriter::write(cur_time, bs.GetData(), bs.GetNumberOfBytesUsed());
    return true;
  }
  virtual int getMTU() const override { return 1 << 20; } // unlimited
  virtual danet::PeerQoSStat getPeerQoSStat() const override { return danet::PeerQoSStat{}; }
  SystemAddress getIP() const override { return SystemAddress(); }
  const char *getIPStr() const override { return nullptr; }
  virtual void disconnect(DisconnectionCause) override { G_ASSERTF(0, "Not supported!"); }
  virtual bool isResponsive() const override { return true; }
};

struct ReplayFileReader
{
  static constexpr int BLOCK_SIZE = 1024 * 1024 * 1; // 1mb
  file_ptr_t fp = nullptr;
  int bodyLen = 0, bodyOffset = 0, blocklen = 0, blockoffset = 0;
  eastl::vector<KeyFrameInfo> keyFrameTable;
  uint8_t buf[BLOCK_SIZE];

  ReplayFileReader() {} // -V730


  ~ReplayFileReader()
  {
    if (fp)
      df_close(fp);
  }

  const uint8_t *readNextBlock()
  {
    bodyOffset += blockoffset;
    if (bodyOffset >= bodyLen)
      return nullptr;

    int length = BLOCK_SIZE;
    if ((bodyOffset + length) > bodyLen)
      length = bodyLen - bodyOffset;

    df_seek_to(fp, bodyOffset);
    if (df_read(fp, (void *)buf, length) != length)
      return nullptr;
    blocklen = length;
    blockoffset = 0;
    return buf;
  }

  static bool readHeader(file_ptr_t fp, ReplayHdr &hdr) { return fp && df_read(fp, &hdr, sizeof(hdr)) == sizeof(ReplayHdr); }

  static bool readFooter(file_ptr_t fp, ReplayHdr const &hdr, int flen, Tab<uint8_t> &out_footer_data)
  {
    if (!fp || flen < 0)
      return false;

    bool ret = true;
    int pos = df_tell(fp);
    if (df_seek_to(fp, flen - hdr.footerSize) == 0)
    {
      out_footer_data.resize(hdr.footerSize);
      if (df_read(fp, (void *)out_footer_data.data(), hdr.footerSize) != hdr.footerSize)
        ret = false;
    }
    df_seek_to(fp, pos);
    return ret;
  }

  void readKeyFrameTable(ReplayHdr const &hdr)
  {
    int pos = df_tell(fp);
    if (df_seek_to(fp, hdr.keyFrameTableOffset) != -1)
    {
      uint32_t tableSize;
      if (df_read(fp, &tableSize, sizeof(tableSize)) == sizeof(tableSize))
      {
        keyFrameTable.resize(tableSize);
        for (uint16_t i = 0; i < tableSize; ++i)
        {
          if (df_read(fp, &keyFrameTable[i], sizeof(KeyFrameInfo)) != sizeof(KeyFrameInfo))
          {
            DAG_FATAL("[Replay] Failed read key frame %i", i);
            keyFrameTable.clear();
            df_seek_to(fp, pos);
            return;
          }
        }
      }
    }
    df_seek_to(fp, pos);
  }

  static inline bool isHeaderValid(ReplayHdr const &hdr, uint16_t version, int flen)
  {
    // Check for 0 version, if version is 0, then ignore proto version
    return !(hdr.magic != RMAGIC || (version != 0 && hdr.version != net::get_replay_proto_version(version)) || hdr.hdrSize < 16 ||
             hdr.hdrSize > flen);
  }

  bool openForRead(const char *fname, uint16_t version, Tab<uint8_t> *out_footer_data)
  {
    fp = df_open(fname, DF_READ | DF_IGNORE_MISSING);
    if (!fp)
    {
      debug("[Replay] failed to open file: '%s'", fname);
      return false;
    }

    int flen = df_length(fp);
    ReplayHdr hdr;
    if (!readHeader(fp, hdr))
      return false;

    if (!isHeaderValid(hdr, version, flen))
    {
      debug("[Replay] Invalid replay header\n"
            "magic = %i vs %i\n"
            "version = %i vs %i\n"
            "hdrSize = %i vs %i\n"
            "for '%s'",
        hdr.magic, RMAGIC, hdr.version, net::get_replay_proto_version(version), hdr.hdrSize, flen, fname);
      return false;
    }

    readKeyFrameTable(hdr);
    blockoffset = hdr.hdrSize;
    bodyLen = flen - hdr.footerSize - keyFrameTable.size() * sizeof(KeyFrameInfo);
    if (out_footer_data)
      return readFooter(fp, hdr, flen, *out_footer_data);
    return true;
  }
};

class ReplayServerDriver final : public net::INetDriver, public ReplayFileReader
{
public:
  Packet pkt;
  uint8_t disc[2] = {ID_DISCONNECT, DC_CONNECTION_CLOSED};
  net::restore_replay_key_frame_cb_t restore_replay_key_frame_cb;

  ReplayServerDriver(net::restore_replay_key_frame_cb_t restore_replay_key_frame_cb) :
    restore_replay_key_frame_cb(restore_replay_key_frame_cb)
  {
    memset(&pkt, 0, sizeof(pkt));
  }

  virtual void *getControlIface() const override { return NULL; }
  bool rewind(int rewind_time)
  {
    auto it = eastl::find_if(keyFrameTable.rbegin(), keyFrameTable.rend(), [=](const auto &rec) { return rec.ts <= rewind_time; });
    if (it == keyFrameTable.rend())
      return true;

    ReplayRecordHdr hdr{};
    if (df_seek_to(fp, it->offset) == -1 || df_read(fp, &hdr, sizeof(hdr)) != sizeof(hdr) || !hdr.isKeyFrame)
    {
      G_ASSERTF(hdr.isKeyFrame, "[Replay] Not a key frame at offset %i", it->offset);
      return false;
    }

    const int lenWithAlign = (hdr.size + REC_ALIGN - 1) & ~(REC_ALIGN - 1);
    const bool needAlloc = lenWithAlign > BLOCK_SIZE;
    blockoffset = it->offset + sizeof(hdr) + lenWithAlign;

    uint8_t *keyFrameData = needAlloc ? (uint8_t *)framemem_ptr()->alloc(lenWithAlign) : buf;
    G_ASSERTF_RETURN(keyFrameData, false, "[Replay] failed allocate memory for key frame %i", lenWithAlign);

    if (df_read(fp, keyFrameData, lenWithAlign) != lenWithAlign)
    {
      if (needAlloc)
        framemem_ptr()->free(keyFrameData);
      G_ASSERTF(NULL, "[Replay] failed to read key frame at offset %i", it->offset);
      return false;
    }

    danet::BitStream bs(keyFrameData, lenWithAlign, false /*copy*/);
    restore_replay_key_frame_cb(bs);
    if (needAlloc)
      framemem_ptr()->free(keyFrameData);
    return true;
  }

  virtual Packet *receive(int cur_time_ms) override
  {
    if (bodyOffset >= bodyLen)
      return NULL;

    const bool eof = bodyOffset + blockoffset >= bodyLen;
    if (eof)
      return nullptr;

    auto connectionLost = [&]() {
      disc[1] = DC_CONNECTION_LOST;
      pkt.data = disc;
      pkt.length = sizeof(disc);
      bodyOffset = bodyLen; // file is end, force reset size
      return &pkt;
    };

    int offsetDelta = sizeof(ReplayRecordHdr);
    if (DAGOR_UNLIKELY(blockoffset + offsetDelta > blocklen && !readNextBlock()))
      return connectionLost();

    const ReplayRecordHdr *hdr = (ReplayRecordHdr *)(buf + blockoffset);
    if (cur_time_ms < hdr->ts) // too early
      return NULL;

    pkt.length = hdr->size;
    pkt.receiveTime = danet::GetTime() - (cur_time_ms - hdr->ts);

    const int lenWithAlign = (pkt.length + REC_ALIGN - 1) & ~(REC_ALIGN - 1);
    if (hdr->isKeyFrame) // ignore keyframe
    {
      blockoffset += sizeof(*hdr) + lenWithAlign;
      return nullptr;
    }

    G_ASSERTF_RETURN(lenWithAlign <= BLOCK_SIZE, connectionLost(), "[Replay] packet is to big %i vs %i", lenWithAlign, BLOCK_SIZE);
    offsetDelta += lenWithAlign;
    if (DAGOR_UNLIKELY(blockoffset + offsetDelta > blocklen && !readNextBlock()))
      return connectionLost();

    blockoffset += sizeof(*hdr);
    if (bodyOffset + blockoffset + pkt.length <= bodyLen)
    {
      pkt.data = (uint8_t *)(buf + blockoffset);
      blockoffset += lenWithAlign;
    }
    else // invalid length?
      return connectionLost();
    return &pkt;
  }
  virtual void free(Packet *pkt_) override
  {
    G_ASSERT(pkt_ == &pkt);
    (void)pkt_;
  }

  virtual void stopAll(DisconnectionCause) override {}
  virtual void shutdown(int) override {}
  virtual bool isServer() const override { return false; }
};

class ReplayClientDriver : public net::INetDriver, public ReplayWriter
{
public:
  eastl::unique_ptr<INetDriver, DestroyDeleter<INetDriver>> drv;

  ReplayClientDriver(net::INetDriver *drv_, ReplayWriter &&rw) : ReplayWriter(eastl::move(rw)), drv(drv_)
  {
    G_ASSERT(drv);
    G_ASSERT(ReplayWriter::wstream);
  }

  virtual void *getControlIface() const override { return drv->getControlIface(); }
  virtual eastl::optional<danet::EchoResponse> receiveEchoResponse() override { return drv->receiveEchoResponse(); }
  virtual Packet *receive(int cur_time_ms) override
  {
    Packet *pkt = drv->receive(cur_time_ms);
    if (pkt)
      ReplayWriter::write(cur_time_ms, pkt->data, pkt->length);
    return pkt;
  }
  virtual void free(Packet *pkt) override { drv->free(pkt); }
  virtual void stopAll(DisconnectionCause) override {}
  virtual void shutdown(int ms) override { drv->shutdown(ms); }
  virtual bool isServer() const override { return drv->isServer(); }
};

namespace net
{

Connection *create_replay_connection(ConnectionId id, char *write_fname, uint16_t version, get_replay_footer_data_cb_t get_footer_cb,
  scope_query_cb_t &&scope_query, dump_replay_key_frame_cb_t dump_replay_key_frame_cb)
{
  auto conn =
    eastl::make_unique<ReplayWriterConnection>(*g_entity_mgr, id, get_footer_cb, eastl::move(scope_query), dump_replay_key_frame_cb);
  return conn->openForWriteTemp(write_fname, version) ? conn.release() : NULL;
}

INetDriver *create_replay_net_driver(const char *read_fname, uint16_t version, Tab<uint8_t> *out_footer_data,
  restore_replay_key_frame_cb_t restore_replay_key_frame_cb)
{
  auto drv = eastl::make_unique<ReplayServerDriver>(restore_replay_key_frame_cb);
  return drv->openForRead(read_fname, version, out_footer_data) ? drv.release() : NULL;
}

INetDriver *create_replay_client_net_driver(INetDriver *drv, const char *write_fname, uint16_t version,
  get_replay_footer_data_cb_t get_footer_cb)
{
  ReplayWriter rw(get_footer_cb);
  return rw.openForWrite(write_fname, version) ? new ReplayClientDriver(drv, eastl::move(rw)) : nullptr;
}


bool load_replay_footer_data(const char *path, uint16_t version,
  eastl::function<void(Tab<uint8_t> const &, const uint32_t)> footer_data_cb)
{
  eastl::unique_ptr<void, decltype(&df_close)> fp(df_open(path, DF_READ | DF_IGNORE_MISSING), &df_close);
  if (!fp.get())
    return false;

  ReplayHdr hdr;
  if (!ReplayFileReader::readHeader(fp.get(), hdr))
    return false;

  int flen = df_length(fp.get());
  if (!ReplayFileReader::isHeaderValid(hdr, version, flen))
    return false;

  Tab<uint8_t> footerData;
  if (!ReplayFileReader::readFooter(fp.get(), hdr, flen, footerData))
    return false;

  footer_data_cb(footerData, hdr.version);
  return true;
}

void replay_save_keyframe(IConnection *conn, int cur_time) { ((ReplayWriterConnection *)conn)->saveKeyFrame(cur_time); }

bool replay_rewind(INetDriver *drv, int rewind_time) { return ((ReplayServerDriver *)drv)->rewind(rewind_time); }

uint32_t get_replay_proto_version(uint16_t net_proto_version)
{
  return ((uint32_t(INTERNAL_REPLAY_VERSION & 15) << 28) | (net::MessageClass::calcNumMessageClasses() << 16) | (net_proto_version));
}

} // namespace net
