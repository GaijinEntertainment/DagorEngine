//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sepClientInstance/sepClientInstanceTypes.h>

#include <sepClient/sepClient.h>


namespace sepclientinstance
{


class SepClientInstance
{
public:
  SepClientInstance(const SepClientInstanceConfig &config, AuthTokenProvider &&token_provider,
    const sepclient::SepClientStatsCallbackPtr &stats_callback);

  bool updateAuthToken(eastl::string_view token)
  {
    if (sepClient)
    {
      return sepClient->updateAuthorizationToken(token);
    }
    return false;
  }

  void poll();

  bool isCallable() const { return sepClient ? sepClient->isCallable() : false; }
  bool isCloseCompleted() const { return sepClient ? sepClient->isCloseCompleted() : true; }

  static sepclient::TransactionId generateMessageId() { return sepclient::SepClient::generateMessageId(); }
  static sepclient::TransactionId generateTransactionId() { return sepclient::SepClient::generateTransactionId(); }

  // Create a new RPC request object for using in `rpcCall()`.
  // Provided transaction id and JSON RPC message id
  // should be used and registered in the game before starting the RPC call.
  // This will avoid data races in case response callback is called too quickly.
  sepclient::RpcRequest createRpcRequest(eastl::string_view service_name, eastl::string_view action,
    eastl::optional<sepclient::RpcMessageId> message_id = eastl::nullopt,
    eastl::optional<sepclient::TransactionId> transaction_id = eastl::nullopt)
  {
    if (sepClient)
    {
      return sepClient->createRpcRequest(service_name, action, message_id, transaction_id);
    }
    return sepclient::RpcRequest::make_dummy();
  }

  void rpcCall(const sepclient::RpcRequest &request, sepclient::ResponseCallback &&response_callback)
  {
    if (sepClient)
    {
      sepClient->rpcCall(request, eastl::move(response_callback));
    }
  }

  void initiateClose()
  {
    if (sepClient)
    {
      sepClient->initiateClose();
    }
  }

  void destroy() { sepClient.reset(); }

private:
  sepclient::SepClientPtr sepClient;
};


} // namespace sepclientinstance
