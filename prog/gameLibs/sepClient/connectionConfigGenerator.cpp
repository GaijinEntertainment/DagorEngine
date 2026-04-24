// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sepClient/details/connectionConfigGenerator.h>

#include "sepClientVersion.h"
#include "sepProtocol.h"

#include <sepClient/details/asyncHostnameResolver.h>
#include <sepClient/sepClient.h>
#include <websocketJsonRpc/details/abstractionLayer.h>

#include <debug/dag_assert.h>
#include <debug/dag_log.h>
#include <osApiWrappers/dag_miscApi.h>
#include <util/dag_globDef.h>


namespace sepclient::details
{

namespace abstraction = websocketjsonrpc::abstraction;


eastl::string generate_user_agent_string(const SepClientConfig &config)
{
  eastl::string userAgent(eastl::string::CtorSprintf{}, "SepClient/%s [%s], release date %s (projectId: %s; platform: %s)",
    sepclient::SEP_VERSION, sepclient::SEP_VERSION_DESCRIPTION, sepclient::SEP_VERSION_RELEASE_DATE, config.defaultProjectId.c_str(),
    get_platform_string_id());

  return userAgent;
}


ConnectionConfigGenerator::ConnectionConfigGenerator(const SepClientConfig &config) :
  verifyCert(config.verifyCert),
  verifyHost(config.verifyHost),
  logLevel(config.logLevel),
  connectionTryTimeoutMs(eastl::max(config.connectionTryTimeoutMs, 0)),
  reconnectDelayConfig(config.reconnectDelay),
  enableZstdDecompressionSupport(config.enableZstdDecompressionSupport),
  userAgent(generate_user_agent_string(config)),
  authorizationToken(config.authorizationToken),
  urlSelector(config.serverUrls, sepclient::details::create_async_hostname_resolver)
{
  G_ASSERT(urlSelector.haveSomeUrls());
}


bool ConnectionConfigGenerator::updateAuthorizationToken(eastl::string_view new_authorization_token)
{
  if (authorizationToken == new_authorization_token)
  {
    return false;
  }

  authorizationToken = new_authorization_token;
  return true;
}


bool ConnectionConfigGenerator::resetAuthorizationTokenIfTheSame(eastl::string_view previous_authorization_token)
{
  if (authorizationToken == previous_authorization_token)
  {
    authorizationToken.clear();
    return true;
  }

  return false;
}


websocketjsonrpc::WsJsonRpcConnectionConfigPtr ConnectionConfigGenerator::generate(
  const websocketjsonrpc::WsJsonRpcConnectionPtr &optional_failed_connection, int connect_retry_number)
{
  const TickCount now = abstraction::get_current_tick_count();

  // `connect_retry_number` may be zero again, but it may be
  // a new connect try sequence after we returned a valid connection config
  if (previousConnectRetryNumber != connect_retry_number || tickWhenToConnectAgain == 0)
  {
    previousConnectRetryNumber = connect_retry_number;

    if (optional_failed_connection)
    {
      const bool isAuthenticationError = optional_failed_connection->isAuthenticationError();
      if (!isAuthenticationError)
      {
        const auto &connectionConfig = optional_failed_connection->getConfig();

        SelectedUrlInfo failedServerUrlInfo;
        failedServerUrlInfo.url = connectionConfig.serverUrl;
        failedServerUrlInfo.connectIpHint = optional_failed_connection->getActualUrlIp();

        urlSelector.removeFailedServerUrl(failedServerUrlInfo);
      }
    }

    const auto randomNumber = abstraction::generate_random_uint8();
    const auto reconnectDelayMs = isVeryFirstConnectionTry ? 0 : reconnectDelayConfig.getDelayMs(connect_retry_number, randomNumber);
    isVeryFirstConnectionTry = false;
    const TickCount ticksPerSec = abstraction::get_ticks_per_second();

    tickWhenToConnectAgain = now + reconnectDelayMs * ticksPerSec / 1000;

    const eastl::string failedServerUrl = optional_failed_connection ? optional_failed_connection->getConfig().serverUrl : "";
    logdbg("[sep][sep_client] ConnectionConfigGenerator: next SEP connection try will be in %d ms (try #%d); failedServer: \"%s\"",
      reconnectDelayMs, connect_retry_number, failedServerUrl.c_str());
  }
  G_ASSERT(tickWhenToConnectAgain > 0);

  if (now < tickWhenToConnectAgain)
  {
    return nullptr;
  }

  auto connectionConfig = createConfig();
  if (!connectionConfig)
  {
    return nullptr;
  }

  tickWhenToConnectAgain = 0;
  return connectionConfig;
}


bool ConnectionConfigGenerator::haveEnoughConnectionParams() const
{
  return urlSelector.haveSomeUrls() && !authorizationToken.empty();
}


eastl::string ConnectionConfigGenerator::extractAuthorizationToken(const websocketjsonrpc::WsJsonRpcConnectionConfig &config)
{
  constexpr eastl::string_view authHeaderName = protocol::http::headers::AUTHORIZATION;
  constexpr eastl::string_view authValuePrefix = protocol::http::headers::AUTHORIZATION_BEARER_PREFIX;

  for (const auto &header : config.additionalHttpHeaders)
  {
    if (header.name == authHeaderName)
    {
      const eastl::string_view authValue = header.value;
      G_ASSERT(authValue.starts_with(authValuePrefix));
      const eastl::string_view token = authValue.substr(authValuePrefix.size());
      return eastl::string{token};
    }
  }

  G_ASSERT(false);
  return {};
}


websocketjsonrpc::WsJsonRpcConnectionConfigPtr ConnectionConfigGenerator::createConfig()
{
  if (authorizationToken.empty())
  {
    return nullptr;
  }

  const SelectedUrlInfo selectedUrl = urlSelector.pollSelectingRandomUrl();
  if (!selectedUrl.hasValue())
  {
    return nullptr;
  }

  G_ASSERT(haveEnoughConnectionParams());

  auto cfgPtr = eastl::make_unique<websocketjsonrpc::WsJsonRpcConnectionConfig>();
  websocketjsonrpc::WsJsonRpcConnectionConfig &cfg = *cfgPtr;

  cfg.serverUrl = selectedUrl.url;
  cfg.connectIpHint = selectedUrl.connectIpHint;

  cfg.additionalHttpHeaders.reserve(4);

  cfg.additionalHttpHeaders.emplace_back(websocket::Header{"User-Agent", userAgent});

  cfg.additionalHttpHeaders.emplace_back(websocket::Header{
    protocol::http::headers::GAIJIN_SEP_PROTOCOL_VERSION, protocol::http::headers::GAIJIN_SEP_PROTOCOL_VERSION_VALUE_1_0});

  const eastl::string_view clientCapabilities = enableZstdDecompressionSupport
                                                  ? protocol::http::headers::GAIJIN_SEP_CLIENT_CAPABILITIES_VALUE_ZSTD
                                                  : protocol::http::headers::GAIJIN_SEP_CLIENT_CAPABILITIES_VALUE_NONE;
  cfg.additionalHttpHeaders.emplace_back(
    websocket::Header{protocol::http::headers::GAIJIN_SEP_CLIENT_CAPABILITIES, eastl::string{clientCapabilities}});

  cfg.additionalHttpHeaders.emplace_back(websocket::Header{
    protocol::http::headers::AUTHORIZATION, protocol::http::headers::AUTHORIZATION_BEARER_PREFIX + authorizationToken});

  cfg.waitForAuthenticationResponse = true;

  cfg.verifyCert = verifyCert;
  cfg.verifyHost = verifyHost;

  cfg.connectTimeoutMs = connectionTryTimeoutMs;

  cfg.maxReceivedMessageSize = 0; // 0 means unlimited message size; value can be ignored on some platforms

  bool logMajorEvents = false;
  bool dumpAll = false;

  switch (logLevel)
  {
    case LogLevel::NONE:
    {
      logMajorEvents = false;
      dumpAll = false;
      break;
    }
    case LogLevel::FULL_DUMP:
    {
      logMajorEvents = true;
      dumpAll = true;
      break;
    }
    case LogLevel::MAJOR_EVENTS:
    {
      logMajorEvents = true;
      break;
    }
  }

  cfg.logOutgoingRequestEvents = logMajorEvents;
  cfg.logIncomingResponseEvents = logMajorEvents;
  cfg.dumpOutgoingRequests = dumpAll;
  cfg.dumpIncomingResponses = dumpAll;
  cfg.dumpIncomingErrorResponses = logMajorEvents;
  cfg.logIncomingRequestEvents = logMajorEvents;
  cfg.logOutgoingResponseEvents = logMajorEvents;
  cfg.dumpIncomingRequests = dumpAll;
  cfg.dumpOutgoingResponses = dumpAll;
  cfg.dumpOutgoingErrorResponses = logMajorEvents;

  return cfgPtr;
}


} // namespace sepclient::details
