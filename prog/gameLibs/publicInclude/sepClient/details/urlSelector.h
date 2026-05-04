//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sepClient/details/asyncHostnameResolver.h>

#include <EASTL/unordered_set.h>


namespace sepclient::details
{


struct SelectedUrlInfo
{
  eastl::string url;
  eastl::string connectIpHint; // empty means no IP hint

  bool hasValue() const { return !url.empty(); }
};


class UrlSelector
{
public:
  UrlSelector(const eastl::vector<eastl::string> &server_urls, AsyncHostnameResolverFactory &&hostname_resolver_factory) :
    hostnameResolverFactory(eastl::move(hostname_resolver_factory)), serverUrls(server_urls), remainingServerUrls(server_urls)
  {}

  bool haveSomeUrls() const;

  // Caller may need to call this method multiple times before getting a valid URL info
  // Some time may be needed for hostname resolution inside.
  // IP hint may be missing if hostname resolution failed (for example there is not Internet connection)
  SelectedUrlInfo pollSelectingRandomUrl();

  void removeFailedServerUrl(const SelectedUrlInfo &failed_server_url);

private:
  eastl::string selectIpFromResolverResults();

private:
  const AsyncHostnameResolverFactory hostnameResolverFactory;
  const eastl::vector<eastl::string> serverUrls;
  eastl::vector<eastl::string> remainingServerUrls;
  eastl::unordered_set<eastl::string> failedServerIPs;

  eastl::string preselectedUrl;
  eastl::string preselectedUrlHost;
  AsyncHostnameResolverPtr ongoingHostnameResolver;
};


} // namespace sepclient::details
