//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/net/message.h>
#include <daECS/net/connid.h>
#include <daNet/bitStream.h>
#include <EASTL/tuple.h>

namespace net
{
void write_eid(danet::BitStream &bs, ecs::EntityId eid);
bool read_eid(const danet::BitStream &bs, ecs::EntityId &eid); // return false if read from stream failed
} // namespace net

namespace danet // To consider: move declaration (but not definition) of these functions to entityId.h?
{
inline void write_type(danet::BitStream &bs, ecs::EntityId eid) { net::write_eid(bs, eid); }
inline bool read_type(const danet::BitStream &bs, ecs::EntityId &eid) { return net::read_eid(bs, eid); }
} // namespace danet

namespace net
{
class IConnection;

namespace detail
{
template <typename T>
inline void write_value(danet::BitStream &bs, const T &val)
{
  bs.Write(val);
}
template <typename T>
inline bool read_value(const danet::BitStream &bs, T &val, const IConnection &)
{
  return bs.Read(val);
}

// Special meaning: ConnectionId is "virtual" member - no data is written, but on read it's
// substituted to id of connection this message was received from
inline void write_value(danet::BitStream &, ConnectionId) {} // Nothing is written
bool read_value(const danet::BitStream &bs, ConnectionId &cid, const IConnection &conn);

template <typename T, int I>
struct Serialize
{
  void operator()(const T &t, danet::BitStream &bs)
  {
    Serialize<T, I - 1>()(t, bs);
    write_value(bs, eastl::get<I - 1>(t));
  }
};
template <typename T>
struct Serialize<T, 0>
{
  void operator()(const T &, danet::BitStream &) {}
};

template <typename T, int I>
struct DeSerialize
{
  bool operator()(T &t, const danet::BitStream &bs, const IConnection &conn)
  {
    return DeSerialize<T, I - 1>()(t, bs, conn) && read_value(bs, eastl::get<I - 1>(t), conn);
  }
};
template <typename T>
struct DeSerialize<T, 0>
{
  bool operator()(T &, const danet::BitStream &, const IConnection &) { return true; }
};
}; // namespace detail

} // namespace net

#define ECS_NET_DECL_MSG_EX(Klass, BaseKlass, TU)                                                                       \
  struct Klass final : public BaseKlass, TU                                                                             \
  {                                                                                                                     \
    typedef TU Tuple;                                                                                                   \
    ECS_NET_DECL_MSG_CLASS_BASE(Klass, BaseKlass)                                                                       \
    {}                                                                                                                  \
    template <typename... Args>                                                                                         \
    Klass(Args &&...args) : Tuple(eastl::forward<Args>(args)...)                                                        \
    {}                                                                                                                  \
    Klass(Tuple &&tup) : Tuple(eastl::move(tup))                                                                        \
    {}                                                                                                                  \
    Klass(Klass &&) = default;                                                                                          \
    virtual void pack(danet::BitStream &bs) const override                                                              \
    {                                                                                                                   \
      net::detail::Serialize<Tuple, eastl::tuple_size<Tuple>::value>()(static_cast<const Tuple &>(*this), bs);          \
    }                                                                                                                   \
    virtual bool unpack(const danet::BitStream &bs, const net::IConnection &conn) override                              \
    {                                                                                                                   \
      return net::detail::DeSerialize<Tuple, eastl::tuple_size<Tuple>::value>()(static_cast<Tuple &>(*this), bs, conn); \
    }                                                                                                                   \
    template <size_t I>                                                                                                 \
    const typename eastl::tuple_element<I, Tuple>::type &get() const                                                    \
    {                                                                                                                   \
      return eastl::get<I>(static_cast<const Tuple &>(*this));                                                          \
    }                                                                                                                   \
    template <size_t I>                                                                                                 \
    typename eastl::tuple_element<I, Tuple>::type &get()                                                                \
    {                                                                                                                   \
      return eastl::get<I>(static_cast<Tuple &>(*this));                                                                \
    }                                                                                                                   \
    net::IMessage *moveHeap() && override                                                                               \
    {                                                                                                                   \
      return new Klass(eastl::move(*this));                                                                             \
    }                                                                                                                   \
  }

#define ECS_BUILD_TUPLE(...)               eastl::tuple<__VA_ARGS__>
#define ECS_NET_DECL_MSG(Klass, ...)       ECS_NET_DECL_MSG_EX(Klass, net::IMessage, ECS_BUILD_TUPLE(__VA_ARGS__))
#define ECS_NET_DECL_TIMED_MSG(Klass, ...) ECS_NET_DECL_MSG_EX(Klass, net::IMessageTimed, ECS_BUILD_TUPLE(__VA_ARGS__))
