// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sepClientInstance/sepClientInstance.h>

#include <sepClient/sepClient.h>

#include <debug/dag_assert.h>
#include <debug/dag_log.h>


namespace sepclientinstance
{

// ========================================================


SepClientInstance::SepClientInstance(const SepClientInstanceConfig &config, AuthTokenProvider &&token_provider,
  const sepclient::SepClientStatsCallbackPtr &stats_callback)
{
  if (config.sepUsagePermilleLimit < 0 || config.sepUsagePermilleLimit > 1000)
  {
    logwarn("[sep][sep_error] SepClientInstance: Unexpected sepUsagePermilleLimit value: %d (should be in [0..1000])",
      config.sepUsagePermilleLimit);
  }
  G_ASSERT(config.sepUsageCalculatedPermilleValue >= 0 && config.sepUsageCalculatedPermilleValue <= 999);
  G_ASSERT(config.isSepEnabled());

  for (const eastl::string &sepUrl : config.sepUrls)
  {
    logdbg("[sep] SepClientInstance: SEP URL: %s", sepUrl.c_str());
  }

  logdbg("[sep] SepClientInstance: sepUsagePermille: %d", config.sepUsagePermilleLimit);
  logdbg("[sep] SepClientInstance: supportCompression: %d", config.supportCompression);
  logdbg("[sep] SepClientInstance: verboseLogging: %d", config.verboseLogging);
  logdbg("[sep] SepClientInstance: defaultProjectId: %d", config.defaultProjectId);

#if DAGOR_DBGLEVEL == 0
  constexpr sepclient::LogLevel logLevel = sepclient::LogLevel::MAJOR_EVENTS;
#else
  constexpr sepclient::LogLevel logLevel = sepclient::LogLevel::DEFAULT;
#endif

  sepclient::SepClientConfig sepClientConfig;
  sepClientConfig.logLevel = config.verboseLogging ? sepclient::LogLevel::FULL_DUMP : logLevel;
  sepClientConfig.enableZstdDecompressionSupport = config.supportCompression;

  if (token_provider)
  {
    sepClientConfig.authorizationToken = token_provider();
  }

  sepClientConfig.serverUrls = config.sepUrls;

  sepClientConfig.defaultProjectId = config.defaultProjectId;

  sepClient = sepclient::SepClient::create(sepClientConfig);
  G_ASSERT(sepClient);

  sepclient::AuthenticationFailureCallback authFailureCallback = [this, tokenProvider = eastl::move(token_provider)](
                                                                   sepclient::AuthenticationFailurePtr &&authenticationFailure) {
    logwarn("[sep][sep_error] SepClientInstance: SEP authentication failed for server '%s': %s",
      authenticationFailure->serverUrl.c_str(), authenticationFailure->errorMessage.c_str());

    if (tokenProvider)
    {
      const eastl::string newAuthToken = tokenProvider();
      G_ASSERT(sepClient);
      sepClient->updateAuthorizationToken(newAuthToken);
    }
  };

  sepclient::IncomingNotificationCallback incomingNotificationCallback = nullptr;

  sepClient->initialize(eastl::move(authFailureCallback), eastl::move(incomingNotificationCallback), stats_callback);
}


void SepClientInstance::poll()
{
  if (!sepClient)
  {
    return;
  }

  const bool isClientCallable = sepClient->isCallable();
  if (!isClientCallable)
  {
    // This is OK when the game is shutting down:
    logdbg("[sep] SepClientInstance: polling SEP client after initiateClose");
  }

  sepClient->poll();
}


} // namespace sepclientinstance
