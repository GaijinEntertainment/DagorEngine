// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <websocketJsonRpc/details/newConnectionIncubator.h>


#include <debug/dag_assert.h>
#include <debug/dag_log.h>


namespace websocketjsonrpc::details
{


NewConnectionIncubator::NewConnectionIncubator(ConnectionFactory &&connection_factory,
  const WsJsonRpcConnectionPtr &optional_failed_connection, eastl::string_view log_prefix, EventCallback &&on_initiate_connection,
  EventCallback &&on_connection_failure) :
  logPrefix(log_prefix),
  connectionFactory(eastl::move(connection_factory)),
  onInitiateConnection(eastl::move(on_initiate_connection)),
  onConnectionFailure(eastl::move(on_connection_failure)),
  previousFailedConnection(optional_failed_connection)
{
  const auto reentranceGuard = reentranceProtection.capture();
  G_ASSERT_RETURN(reentranceGuard.isCaptured(), );

  G_ASSERT(connectionFactory);
  G_ASSERT(onInitiateConnection);
  G_ASSERT(onConnectionFailure);

  if (previousFailedConnection)
  {
    G_ASSERT(previousFailedConnection->isClosed());
    previousFailedConnectionIsNew = true;
  }
}


void NewConnectionIncubator::trySetupNewConnection()
{
  G_ASSERT(!nextConnection);

  if (previousFailedConnection && previousFailedConnectionIsNew)
  {
    previousFailedConnectionIsNew = false;
    if (connectRetryNumber > 0)
    {
      logwarn("%sWARN! Connection #%u (next) failed to connect! (url: %s; connect retry #: %d)", logPrefix.c_str(),
        details::get_connection_id(previousFailedConnection), previousFailedConnection->getConfig().serverUrl.c_str(),
        connectRetryNumber);

      if (onConnectionFailure)
      {
        onConnectionFailure(previousFailedConnection);
      }
    }
  }

  G_ASSERT_RETURN(connectionFactory, );
  nextConnection = connectionFactory(previousFailedConnection, connectRetryNumber); // application callbacks may be called here
  if (!nextConnection)
  {
    return;
  }
  G_ASSERT(nextConnection->isConnecting());

  logdbg("%sConnection #%u is starting (url: %s; connect retry #: %d)", logPrefix.c_str(), details::get_connection_id(nextConnection),
    nextConnection->getConfig().serverUrl, connectRetryNumber);

  if (onInitiateConnection)
  {
    onInitiateConnection(nextConnection);
  }

  connectRetryNumber++;

  // Force start connecting as soon as possible:
  nextConnection->poll(); // application callbacks may be called here
}


void NewConnectionIncubator::internalPoll()
{
  if (nextConnection && !nextConnection->isClosed())
  {
    G_ASSERT(!nextConnection->isConnected()); // may be connecting or stopping

    nextConnection->poll(); // application callbacks may be called here
  }

  if (nextConnection && nextConnection->isClosed())
  {
    previousFailedConnection = eastl::move(nextConnection);
    previousFailedConnectionIsNew = true;
    nextConnection = nullptr; // does nothing, put here to simplify code readability
  }

  if (!nextConnection)
  {
    trySetupNewConnection(); // application callbacks may be called here
  }
}


WsJsonRpcConnectionPtr NewConnectionIncubator::poll()
{
  const auto reentranceGuard = reentranceProtection.capture();
  G_ASSERT_RETURN(reentranceGuard.isCaptured(), nullptr);

  G_ASSERT_RETURN(!finishedWork, nullptr);

  internalPoll(); // application callbacks may be called here

  if (nextConnection && nextConnection->isConnected())
  {
    return finishAndExtract();
  }

  return nullptr;
}


WsJsonRpcConnectionPtr NewConnectionIncubator::cancel()
{
  const auto reentranceGuard = reentranceProtection.capture();
  G_ASSERT_RETURN(reentranceGuard.isCaptured(), nullptr);

  if (nextConnection)
  {
    nextConnection->initiateClose();
  }

  return finishAndExtract();
}


WsJsonRpcConnectionPtr NewConnectionIncubator::finishAndExtract()
{
  previousFailedConnection.reset();
  finishedWork = true;
  return eastl::move(nextConnection);
}


} // namespace websocketjsonrpc::details
