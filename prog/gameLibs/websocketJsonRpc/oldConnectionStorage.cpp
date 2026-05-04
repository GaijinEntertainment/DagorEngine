// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <websocketJsonRpc/details/oldConnectionStorage.h>

#include <debug/dag_assert.h>
#include <debug/dag_log.h>


namespace websocketjsonrpc::details
{


OldConnectionStorage::OldConnectionStorage(eastl::string_view log_prefix) : logPrefix(log_prefix) {}


void OldConnectionStorage::addDrainingConnection(const WsJsonRpcConnectionPtr &connection)
{
  G_ASSERT_RETURN(connection, );

  logdbg("%sConnection #%u is marked for graceful closing", logPrefix.c_str(), details::get_connection_id(connection));

  drainingConnections.emplace_back(connection);
}


void OldConnectionStorage::addClosingConnection(const WsJsonRpcConnectionPtr &connection)
{
  G_ASSERT_RETURN(connection, );

  logdbg("%sConnection #%u is marked as closing", logPrefix.c_str(), details::get_connection_id(connection));

  connection->initiateClose();

  closingConnections.emplace_back(connection);
}


void OldConnectionStorage::pollAllConnections()
{
  // NOTE: do not use collection iterators here since they may be invalidated by the application callbacks!

  for (size_t i = 0; i < drainingConnections.size(); ++i)
  {
    const WsJsonRpcConnectionPtr connectionRefCopy = drainingConnections[i];
    if (connectionRefCopy && !connectionRefCopy->isClosed())
    {
      connectionRefCopy->poll(); // application callbacks may be called here
    }
  }

  for (size_t i = 0; i < closingConnections.size(); ++i)
  {
    const WsJsonRpcConnectionPtr connectionRefCopy = closingConnections[i];
    if (connectionRefCopy && !connectionRefCopy->isClosed())
    {
      connectionRefCopy->poll(); // application callbacks may be called here
    }
  }
}


void OldConnectionStorage::processOutdatedConnections()
{
  const auto destroyIfClosed = [this](WsJsonRpcConnectionPtr &connection) {
    if (connection && connection->isClosed())
    {
      logdbg("%sConnection #%u was found fully closed", logPrefix.c_str(), details::get_connection_id(connection));
      connection.reset();
    }
  };

  for (WsJsonRpcConnectionPtr &connection : drainingConnections)
  {
    destroyIfClosed(connection);

    if (connection && connection->getInProgressCallCount() == 0) // no more pending calls initiated by client
    {
      addClosingConnection(connection);
      connection.reset();
    }
  }

  for (WsJsonRpcConnectionPtr &connection : closingConnections)
  {
    destroyIfClosed(connection);
  }
}


void OldConnectionStorage::removeNullPointers()
{
  while (!drainingConnections.empty() && !drainingConnections.front())
  {
    drainingConnections.pop_front();
  }

  while (!closingConnections.empty() && !closingConnections.front())
  {
    closingConnections.pop_front();
  }
}


void OldConnectionStorage::poll()
{
  pollAllConnections(); // application callbacks may be called here

  processOutdatedConnections();

  removeNullPointers();
}


void OldConnectionStorage::initiateClose()
{
  for (WsJsonRpcConnectionPtr &connection : drainingConnections)
  {
    if (connection)
    {
      addClosingConnection(connection);
      connection.reset();
    }
  }
}


} // namespace websocketjsonrpc::details
