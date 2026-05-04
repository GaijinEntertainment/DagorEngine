//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/string.h>
#include <EASTL/string_view.h>


namespace sepclient
{


class RpcRequest final
{
private:
  friend class SepClient;
  RpcRequest(RpcMessageId json_rpc_message_id, TransactionId transaction_id, eastl::string_view service, eastl::string_view action) :
    jsonRpcMessageId(json_rpc_message_id), transactionId(transaction_id), service(service), action(action)
  {}

public:
  static RpcRequest make_dummy() { return RpcRequest(1, 1, "noService", "noAction"); }

  RpcRequest(const RpcRequest &) = default;
  RpcRequest(RpcRequest &&) noexcept = default;
  RpcRequest &operator=(const RpcRequest &) = default;
  RpcRequest &operator=(RpcRequest &&) noexcept = default;

  RpcMessageId getJsonRpcMessageId() const { return jsonRpcMessageId; }

  TransactionId getTransactionId() const { return transactionId; }

  const eastl::string &getService() const { return service; }
  eastl::string &getService() { return service; }

  const eastl::string &getAction() const { return action; }
  eastl::string &getAction() { return action; }

  RpcRequest &setProjectId(eastl::string_view project_id)
  {
    projectId = project_id;
    return *this;
  }
  const eastl::string &getProjectId() const { return projectId; }
  eastl::string &getProjectId() { return projectId; }

  RpcRequest &setProfileId(eastl::string_view profile_id)
  {
    profileId = profile_id;
    return *this;
  }
  const eastl::string &getProfileId() const { return profileId; }
  eastl::string &getProfileId() { return profileId; }

  RpcRequest &setActionParams(eastl::string &&action_params)
  {
    actionParams = eastl::move(action_params);
    return *this;
  }
  const eastl::string &getActionParams() const { return actionParams; }
  eastl::string &getActionParams() { return actionParams; }

private:
  RpcMessageId jsonRpcMessageId{}; // positive value
  eastl::string service;           // "inventory", "profile", "userstat" & etc.
  eastl::string action;            // must not be empty
  TransactionId transactionId{};   // positive value
  eastl::string projectId;         // empty string means no value
  eastl::string profileId;         // empty string means no value
  eastl::string actionParams;      // serialized JSON; empty string means no value
};


} // namespace sepclient
