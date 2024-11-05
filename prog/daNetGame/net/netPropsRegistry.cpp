// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "net/netPropsRegistry.h"
#include <daECS/net/msgDecl.h>
#include <daECS/net/netbase.h>
#include "net.h"
#include "net/channel.h"
#include <daECS/net/netEvents.h>
#include <daECS/net/msgSink.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <propsRegistry/propsRegistry.h>
#include <util/dag_string.h>
#include <ADT/fastHashNameMap.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <daNet/bitStream.h>
#include <debug/dag_debug.h>

#if DAGOR_DBGLEVEL > 0
static int do_trace_np()
{
  static int doTraceNp = dgs_get_settings()->getBlockByNameEx("debug")->getInt("traceNetProps", 0);
  return doTraceNp;
}
#define DO_TRACE_NP do_trace_np()
#else
#define DO_TRACE_NP 0
#endif

#define TRACE_NP(blkName, clsName, propId)                                     \
  do                                                                           \
  {                                                                            \
    int traceNp = DO_TRACE_NP;                                                 \
    if (traceNp > 0)                                                           \
    {                                                                          \
      debug("%s blk=%s cls=%s -> %d", __FUNCTION__, blkName, clsName, propId); \
      if (traceNp > 1)                                                         \
        debug_dump_stack();                                                    \
    }                                                                          \
  } while (0)

static void on_receive_net_props_registry_append(const net::IMessage *imsg);
static void on_receive_net_props_registry_initial(const net::IMessage *imsg);

ECS_NET_DECL_MSG(NetPropsRegistryAppend, String, String, short); // blkName, className, propsId
ECS_NET_IMPL_MSG(NetPropsRegistryAppend,
  net::ROUTING_SERVER_TO_CLIENT,
  &net::broadcast_rcptf,
  RELIABLE_ORDERED,
  NC_DEFAULT,
  net::MF_DEFAULT_FLAGS,
  ECS_NET_NO_DUP,
  &on_receive_net_props_registry_append);
ECS_NET_DECL_MSG(NetPropsRegistryInitial, danet::BitStream);
ECS_NET_IMPL_MSG(NetPropsRegistryInitial,
  net::ROUTING_SERVER_TO_CLIENT,
  &net::direct_connection_rcptf,
  RELIABLE_ORDERED,
  NC_DEFAULT,
  net::MF_DEFAULT_FLAGS,
  ECS_NET_NO_DUP,
  &on_receive_net_props_registry_initial);

static inline void serialize_nm(const FastHashNameMap &nm, danet::BitStream &bs)
{
  int sz = (int)nm.size();
  bs.WriteCompressed((uint16_t)sz);
  for (int i = 0; i < sz; ++i)
  {
    const char *name = nm.getNameSlow(i);
    G_ASSERT(name);
    bs.Write(name);
  }
}

struct NetPropsRegistryCtx
{
  FastHashNameMap blkNames, classNames;
  eastl::vector<eastl::pair<uint16_t, uint16_t>> registrySeq; // blkId, clsId

  void registerNetProp(const char *blkName, const char *clsName)
  {
    auto blkIns = blkNames.insert(blkName);
    auto clsIns = classNames.insert(clsName);
    registrySeq.push_back(eastl::make_pair((uint16_t)blkIns.first, (uint16_t)clsIns.first));
  }

  void flushTo(net::IConnection &conn)
  {
    if (registrySeq.empty())
      return;

    NetPropsRegistryInitial msg;
    msg.connection = &conn;
    danet::BitStream &bs = msg.get<0>();
    serialize(bs);
    debug("flush initial props registry of size %d bytes / %d elems to #%d", bs.GetNumberOfBytesUsed(), (int)registrySeq.size(),
      (int)conn.getId());
    send_net_msg(net::get_msg_sink(), eastl::move(msg));
  }

  void serialize(danet::BitStream &bs) const
  {
    serialize_nm(blkNames, bs); // TODO: compress this (zstd?)
    serialize_nm(classNames, bs);

    bs.WriteCompressed((uint16_t)registrySeq.size());
    for (auto &blkClsPair : registrySeq)
    {
      bs.WriteCompressed(blkClsPair.first);
      bs.WriteCompressed(blkClsPair.second);
    }
  }
};
static eastl::unique_ptr<NetPropsRegistryCtx> net_props_registry_ctx;

static void on_receive_net_props_registry_append(const net::IMessage *imsg)
{
  auto msg = imsg->cast<NetPropsRegistryAppend>();
  G_ASSERT(msg);
  int addedProp = propsreg::register_props(msg->get<0>().str(), msg->get<1>().str());
  G_UNUSED(addedProp);
  TRACE_NP(msg->get<0>().str(), msg->get<1>().str(), addedProp);
  G_ASSERT_LOG(addedProp == msg->get<2>(), "Net prop '%s:%s' desync %d(client) != %d(server)", msg->get<0>().str(),
    msg->get<1>().str(), addedProp, msg->get<2>());
}

static void on_receive_net_props_registry_initial(const net::IMessage *imsg)
{
  auto msg = imsg->cast<NetPropsRegistryInitial>();
  G_ASSERT(msg);
  const danet::BitStream &bs = msg->get<0>();
  propsreg::deserialize_net_registry(bs);
}

void propsreg::serialize_net_registry(danet::BitStream &bs)
{
  if (net_props_registry_ctx)
    net_props_registry_ctx->serialize(bs);
}

void propsreg::deserialize_net_registry(const danet::BitStream &bs)
{
  eastl::vector<String> blkNames, classNames;

  uint16_t numBlks = -1;
  G_VERIFY(bs.ReadCompressed(numBlks));
  for (int i = 0; i < numBlks; ++i)
  {
    blkNames.emplace_back();
    G_VERIFY(bs.Read(blkNames.back()));
  }

  uint16_t numClasses = -1;
  G_VERIFY(bs.ReadCompressed(numClasses));
  for (int i = 0; i < numClasses; ++i)
  {
    classNames.emplace_back();
    G_VERIFY(bs.Read(classNames.back()));
  }

  uint16_t numRecords = -1;
  G_VERIFY(bs.ReadCompressed(numRecords));
  for (int i = 0; i < numRecords; ++i)
  {
    uint16_t blkId = -1, classId = -1;
    G_VERIFY(bs.ReadCompressed(blkId));
    G_VERIFY(bs.ReadCompressed(classId));
    int addedProp = propsreg::register_props(blkNames[blkId].str(), classNames[classId].str());
    TRACE_NP(blkNames[blkId].str(), classNames[classId].str(), addedProp);
    G_UNUSED(addedProp);
  }

  debug("%s %d blks, %d classes, %d records, %d bytes", __FUNCTION__, (int)numBlks, (int)numClasses, (int)numRecords,
    bs.GetNumberOfBytesUsed());
}

int propsreg::register_net_props(const char *blk_name, const char *class_name)
{
  int propsId = propsreg::get_props_id(blk_name, class_name);
  if (propsId < 0)
  {
    if (DAGOR_UNLIKELY(!is_server()))
      logerr("Incorrect net props registry '%s:%s', blk not found?", blk_name, class_name);
    propsId = propsreg::register_props(blk_name, class_name);
    TRACE_NP(blk_name, class_name, propsId);
    if (net_props_registry_ctx)
    {
      net_props_registry_ctx->registerNetProp(blk_name, class_name);
      G_ASSERT(short(propsId) == propsId);
      send_net_msg(net::get_msg_sink(), NetPropsRegistryAppend(String(blk_name), String(class_name), short(propsId)));
    }
  }
  return propsId;
}

void propsreg::flush_net_registry(net::IConnection &conn)
{
  if (net_props_registry_ctx)
    net_props_registry_ctx->flushTo(conn);
}

void propsreg::init_net_registry_server() { net_props_registry_ctx = eastl::make_unique<NetPropsRegistryCtx>(); }
void propsreg::term_net_registry() { net_props_registry_ctx.reset(); }
