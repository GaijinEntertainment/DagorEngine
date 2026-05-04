// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sepClient/sepClient.h>

#include <debug/dag_assert.h>
#include <debug/dag_log.h>


namespace sepclient
{


int ReconnectDelayConfig::getDelayMs(int connect_retry_number, uint8_t random_number) const
{
  // failoverDelayPerTry: [2.5 .. 7.6] sec, ~5 sec average
  const int failoverDelayPerTry = 2500 + random_number * 4 * 5;
  // failoverDelayMs: average: 5, 10, 15 sec... up to 5000 sec (1.4 hours)
  const int failoverDelayMs = failoverDelayPerTry * eastl::min(eastl::max(connect_retry_number + 1, 1), 1000);

  G_ASSERT_RETURN(connect_retry_number >= 0, failoverDelayMs);

  if (initialDelayMinMs < 0 || initialDelayMaxMs < 0 || maxDelayMs < 0 || initialDelayMaxMs < initialDelayMinMs)
  {
    logwarn("[sepClient] WARN! Incorrect one or more config values in ReconnectDelayConfig."
            " Returning failover connect delay value: %d ms for retry #%d;"
            " Config values: initialDelayMinMs: %d; initialDelayMaxMs: %d; maxDelayMs: %d",
      failoverDelayMs, connect_retry_number, initialDelayMinMs, initialDelayMaxMs, maxDelayMs);
    return failoverDelayMs;
  }

  int64_t minMs = std::min(initialDelayMinMs, maxDelayMs);
  int64_t maxMs = std::min(initialDelayMaxMs, maxDelayMs);

  for (int i = 1; i <= connect_retry_number; ++i)
  {
    minMs = std::min(minMs * 2, static_cast<int64_t>(maxDelayMs));
    maxMs = std::min(maxMs * 2, static_cast<int64_t>(maxDelayMs));
    if (minMs >= maxDelayMs) //-V1051 // Consider checking for misprints. It's possible that the 'maxMs' should be checked here.
      break;
  }

  const int64_t actualDelayMs = minMs + (maxMs - minMs) * random_number / 255; // 255 is a max value for randomNumber (uint8_t)
  return static_cast<int>(actualDelayMs);
}


} // namespace sepclient
