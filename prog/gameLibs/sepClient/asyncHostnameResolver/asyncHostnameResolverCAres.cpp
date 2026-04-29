// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sepClient/details/asyncHostnameResolver.h>

#include <debug/dag_log.h>

#include <ares.h>

#if _TARGET_PC_WIN | _TARGET_XBOX
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#endif

#include <algorithm>
#include <cstring>


#ifndef SEPCLIENT_USE_CARES
#error "SEPCLIENT_USE_CARES must be defined to compile this file"
#endif


namespace sepclient::details
{

constexpr int CARES_QUERY_TIMEOUT_MS = 5'000;
constexpr int CARES_QUERY_RETRIES = 4;


class CaresInitializer final
{
public:
  CaresInitializer() : status(ares_library_init(ARES_LIB_INIT_ALL)) {}

  ~CaresInitializer()
  {
    if (isInitialized())
    {
      ares_library_cleanup();
    }
  }

  bool isInitialized() const { return status == ARES_SUCCESS; }
  int getStatus() const { return status; }

private:
  const int status = ARES_ENOTINITIALIZED;
};


const CaresInitializer cares_initializer;


class AsyncHostnameResolverCAres final : public AsyncHostnameResolver
{
public:
  explicit AsyncHostnameResolverCAres(eastl::string_view hostname, ResolveOptions options)
  {
    if (!cares_initializer.isInitialized())
    {
      logwarn("[sep][sep_client] WARNING! UrlSelector: ares_library_init failed in AsyncHostnameResolverCAres: %s (code: %d)",
        ares_strerror(cares_initializer.getStatus()), cares_initializer.getStatus());
      finishedResolving = true;
      return;
    }

    ares_options channelOptions;
    memset(&channelOptions, 0, sizeof(channelOptions));
    channelOptions.timeout = CARES_QUERY_TIMEOUT_MS;
    channelOptions.tries = CARES_QUERY_RETRIES;
    constexpr int channelOptionsMask = ARES_OPT_TIMEOUTMS | ARES_OPT_TRIES;

    const int status = ares_init_options(&channel, &channelOptions, channelOptionsMask);
    if (status != ARES_SUCCESS)
    {
      logwarn("[sep][sep_client] WARNING! UrlSelector: ares_init_options failed in AsyncHostnameResolverCAres: %s (code: %d)",
        ares_strerror(status), status);
      finishedResolving = true;
      return;
    }

    const eastl::string hostnameStr{hostname};

    // Start queries
    if (options == ResolveOptions::IPV4 || options == ResolveOptions::IPV4_AND_IPV6)
    {
      ares_gethostbyname(channel, hostnameStr.c_str(), AF_INET, &callback_static, this);
      queriesInitiated++;
    }
    if (options == ResolveOptions::IPV6 || options == ResolveOptions::IPV4_AND_IPV6)
    {
      ares_gethostbyname(channel, hostnameStr.c_str(), AF_INET6, &callback_static, this);
      queriesInitiated++;
    }

    if (queriesInitiated == 0)
    {
      logdbg("[sep][sep_client] UrlSelector: AsyncHostnameResolverCAres no DNS queries were initiated");
      finishedResolving = true;
      return;
    }
    if (queriesCompleted >= queriesInitiated)
    {
      logdbg("[sep][sep_client] UrlSelector: resolved hostname locally (%d queries) in AsyncHostnameResolverCAres", queriesInitiated);
      finishedResolving = true;
      return;
    }
  }

  ~AsyncHostnameResolverCAres() override
  {
    if (channel)
    {
      ares_destroy(channel);
    }
  }

  void poll() override
  {
    if (finishedResolving)
    {
      return;
    }

    timeval timeoutStorage{};
    const timeval *nextStopOrNull = ares_timeout(channel, nullptr, &timeoutStorage);
    const bool stopPolling = nextStopOrNull && nextStopOrNull->tv_sec == 0 && nextStopOrNull->tv_usec == 0;
    if (stopPolling)
    {
      logdbg("[sep][sep_client] UrlSelector: Timeout resolving hostname using AsyncHostnameResolverCAres");
      finishedResolving = true;
      return;
    }

    ares_socket_t socks[ARES_GETSOCK_MAXNUM];
    const int bitmask = ares_getsock(channel, socks, ARES_GETSOCK_MAXNUM);

    pollfd pollfds[ARES_GETSOCK_MAXNUM];
    int pollfdCount = 0;

    for (int i = 0; i < ARES_GETSOCK_MAXNUM; ++i)
    {
      if (ARES_GETSOCK_READABLE(bitmask, i) || ARES_GETSOCK_WRITABLE(bitmask, i))
      {
        pollfd &pfd = pollfds[pollfdCount++];
        pfd.fd = socks[i];
        pfd.events = 0;
        if (ARES_GETSOCK_READABLE(bitmask, i))
          pfd.events |= POLLIN;
        if (ARES_GETSOCK_WRITABLE(bitmask, i))
          pfd.events |= POLLOUT;
      }
    }

    if (pollfdCount == 0)
    {
      return;
    }

    constexpr int timeoutMs = 0; // Non-blocking poll
#if _TARGET_PC_WIN
    const int ret = ::WSAPoll(pollfds, pollfdCount, timeoutMs);
#else
    const int ret = ::poll(pollfds, pollfdCount, timeoutMs);
#endif
    if (ret == 0)
    {
      // No events. Nothing to do
      return;
    }
    if (ret < 0)
    {
      logwarn("[sep][sep_client] WARNING! UrlSelector: poll() failed in AsyncHostnameResolverCAres: error code: %d", ret);
      finishedResolving = true;
      return;
    }

    for (int i = 0; i < pollfdCount; ++i)
    {
      const pollfd &pfd = pollfds[i];
      // `callback_static` is called here from inside of `ares_process_fd`:
      ares_process_fd(channel, (pfd.revents & POLLIN) ? pfd.fd : ARES_SOCKET_BAD, (pfd.revents & POLLOUT) ? pfd.fd : ARES_SOCKET_BAD);
    }

    if (!finishedResolving && queriesCompleted >= queriesInitiated)
    {
      logdbg("[sep][sep_client] UrlSelector: finished all DNS queries (%d queries) in AsyncHostnameResolverCAres", queriesInitiated);
      finishedResolving = true;
    }
  }

  bool isFinished() const override { return finishedResolving; }

  const eastl::vector<eastl::string> &getResults() const override { return results; }

private:
  static void callback_static(void *arg, int status, int timeouts, hostent *host)
  {
    static_cast<AsyncHostnameResolverCAres *>(arg)->callback(status, timeouts, host);
  }

  void callback(int status, int timeouts, hostent *host)
  {
    if (status == ARES_SUCCESS && host && host->h_addr_list)
    {
      for (int i = 0; host->h_addr_list[i] != nullptr; ++i)
      {
        constexpr size_t formatted_ip_max_len = std::max(INET_ADDRSTRLEN, INET6_ADDRSTRLEN);
        char ipbuf[formatted_ip_max_len];
        const char *formattedIp = inet_ntop(host->h_addrtype, host->h_addr_list[i], ipbuf, sizeof(ipbuf));
        if (formattedIp != nullptr)
        {
          results.emplace_back(formattedIp);
        }
        else
        {
          logwarn("[sep][sep_client] WARNING! UrlSelector: inet_ntop failed in AsyncHostnameResolverCAres callback");
        }
      }
    }
    else
    {
      logwarn(
        "[sep][sep_client] WARNING! UrlSelector: DNS query failed in AsyncHostnameResolverCAres callback: %s (code: %d); timeout = %d",
        ares_strerror(status), status, timeouts);
    }

    queriesCompleted++;
  }

private:
  ares_channel channel = nullptr;
  eastl::vector<eastl::string> results;

  int queriesInitiated = 0;
  int queriesCompleted = 0;
  bool finishedResolving = false;
};


AsyncHostnameResolverPtr create_async_hostname_resolver(eastl::string_view hostname, ResolveOptions options)
{
  return eastl::make_unique<AsyncHostnameResolverCAres>(hostname, options);
}


} // namespace sepclient::details
