//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_carray.h>
#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <EASTL/hash_map.h>

#include <webui/bson.h>

class DataBlock;

namespace websocket
{
class MessageListener
{
public:
  typedef void (MessageListener::*Method)(const DataBlock &);

  void addCommand(const char *name, const Method &method);
  bool processCommand(const char *name, const DataBlock &blk);

  virtual void update() {}
  virtual void onMessage(const DataBlock & /*blk*/) {}

protected:
  typedef eastl::hash_map<SimpleString, Method, eastl::string_hash<SimpleString>> CommandMap;
  CommandMap commands;
};

bool start(int port);
void update();
void stop();
int get_port();

void send_log(const char *message);
void send_message(const DataBlock &blk);
void send_buffer(const char *buffer, int length);
void send_binary(dag::ConstSpan<uint8_t> buffer);
void send_binary(const uint8_t *buffer, int length);
void send_binary(BsonStream &bson);
void add_message_listener(MessageListener *listener);
void remove_message_listener(MessageListener *listener);
} // namespace websocket
