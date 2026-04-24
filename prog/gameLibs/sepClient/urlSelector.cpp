// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sepClient/details/urlSelector.h>

#include <websocketJsonRpc/details/abstractionLayer.h>
#include <websocketJsonRpc/details/urlParse.h>

#include <debug/dag_log.h>

#include <EASTL/algorithm.h>


namespace sepclient::details
{

namespace abstraction = websocketjsonrpc::abstraction;


template <typename T>
static eastl::string join_strings(const T &collection)
{
  eastl::string result;
  result.reserve(1024);
  bool first = true;
  for (const auto &item : collection)
  {
    if (!first)
    {
      result += ", ";
    }
    result += item;
    first = false;
  }
  return result;
}


bool UrlSelector::haveSomeUrls() const { return !serverUrls.empty() && !remainingServerUrls.empty(); }


SelectedUrlInfo UrlSelector::pollSelectingRandomUrl()
{
  SelectedUrlInfo urlInfo;

  if (!ongoingHostnameResolver)
  {
    if (remainingServerUrls.empty())
    {
      return urlInfo;
    }

    const auto randomValue = abstraction::generate_random_uint32();
    preselectedUrl = remainingServerUrls[randomValue % remainingServerUrls.size()];
    preselectedUrlHost = eastl::string(websocketjsonrpc::details::url_to_hostname(preselectedUrl));

    if (hostnameResolverFactory)
    {
      // Don't request IPv6 addresses for now, as some platforms and clients may have problems with IPv6 connectivity
      ongoingHostnameResolver = hostnameResolverFactory(preselectedUrlHost, ResolveOptions::IPV4);
    }

    if (!ongoingHostnameResolver)
    {
      logdbg("[sep][sep_client] UrlSelector: hostname resolver is disabled on this platform (url: %s)", preselectedUrl.c_str());

      urlInfo.url = preselectedUrl;

      preselectedUrl.clear();
      preselectedUrlHost.clear();

      return urlInfo;
    }

    logdbg("[sep][sep_client] UrlSelector: started resolving hostname '%s' for url: %s", preselectedUrlHost.c_str(),
      preselectedUrl.c_str());
  }

  ongoingHostnameResolver->poll();

  if (!ongoingHostnameResolver->isFinished())
  {
    return urlInfo;
  }

  urlInfo.url = preselectedUrl;
  urlInfo.connectIpHint = selectIpFromResolverResults();

  ongoingHostnameResolver.reset();
  preselectedUrl.clear();
  preselectedUrlHost.clear();
  return urlInfo;
}


eastl::string UrlSelector::selectIpFromResolverResults()
{
  eastl::string selectedIp;

  if (!ongoingHostnameResolver || !ongoingHostnameResolver->isFinished())
  {
    return selectedIp;
  }

  const eastl::vector<eastl::string> &resolvedIps = ongoingHostnameResolver->getResults();
  const eastl::string resolvedIpsStr = join_strings(resolvedIps);
  const eastl::string failedIpsStr = join_strings(failedServerIPs);

  eastl::vector<eastl::string_view> filteredIps;
  filteredIps.reserve(resolvedIps.size());

  if (!failedServerIPs.empty())
  {
    const auto isGoodIp = [this](const eastl::string &ip) { return failedServerIPs.count(ip) == 0; };
    eastl::copy_if(resolvedIps.cbegin(), resolvedIps.cend(), eastl::back_inserter(filteredIps), isGoodIp);

    if (!resolvedIps.empty() && filteredIps.empty())
    {
      logdbg("[sep][sep_client] UrlSelector: all resolved IPs for hostname '%s' are already marked as failed. Reset failed IP list",
        preselectedUrlHost.c_str());

      failedServerIPs.clear();
    }
  }

  if (failedServerIPs.empty())
  {
    G_ASSERT(filteredIps.empty());
    eastl::copy(resolvedIps.cbegin(), resolvedIps.cend(), eastl::back_inserter(filteredIps));
  }

  if (!filteredIps.empty())
  {
    const auto randomValue = abstraction::generate_random_uint32();
    selectedIp = eastl::string{filteredIps[randomValue % filteredIps.size()]};
  }

  logdbg("[sep][sep_client] UrlSelector: resolved IPs for hostname '%s': [%s]; originally failed IPs: [%s]; selected IP: %s",
    preselectedUrlHost.c_str(), resolvedIpsStr.c_str(), failedIpsStr.c_str(), selectedIp.c_str());

  return selectedIp;
}


void UrlSelector::removeFailedServerUrl(const SelectedUrlInfo &failed_server_url)
{
  if (!failed_server_url.connectIpHint.empty())
  {
    logdbg("[sep][sep_client] UrlSelector: mark IP %s as failed", failed_server_url.connectIpHint.c_str());
    failedServerIPs.emplace(failed_server_url.connectIpHint);
  }

  if (serverUrls.size() <= 1)
  {
    // exit to save CPU when it is called many times with the same server URL (usual case during reconnect delay)
    return;
  }

  const auto removeFailedServer = [this, failed_server_url]() {
    logdbg("[sep][sep_client] UrlSelector: mark URL %s as failed", failed_server_url.url.c_str());

    eastl::erase_if(remainingServerUrls, [failed_server_url](eastl::string_view url) { return url == failed_server_url.url; });

    if (remainingServerUrls.empty())
    {
      logdbg("[sep][sep_client] UrlSelector: all URLs are marked as failed. Reset failed URL list");
      remainingServerUrls = serverUrls;
      return true;
    }

    return false;
  };

  const bool serverListWasReset = removeFailedServer();

  if (serverListWasReset)
  {
    removeFailedServer();
  }
}


} // namespace sepclient::details
