// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sepClient/details/urlSelector.h>

#include <catch2/catch_test_macros.hpp>
#include <unittest/catch2_eastl_tostring.h>


static sepclient::details::AsyncHostnameResolverPtr dummy_resolver_factory(eastl::string_view /* hostname */,
  sepclient::details::ResolveOptions /* options */)
{
  return nullptr;
}


class ConstAsyncHostnameResolver final : public sepclient::details::AsyncHostnameResolver
{
public:
  explicit ConstAsyncHostnameResolver(const eastl::vector<eastl::string> &ips_) : ips(ips_) {}
  void poll() override { pollCalls++; }
  bool isFinished() const override { return pollCalls >= 2; }
  const eastl::vector<eastl::string> &getResults() const override { return isFinished() ? ips : NO_IPS; }

private:
  int pollCalls = 0;
  const eastl::vector<eastl::string> ips;
  static inline const eastl::vector<eastl::string> NO_IPS;
};


static sepclient::details::AsyncHostnameResolverFactory create_const_resolver_factory(const eastl::vector<eastl::string> &ips)
{
  return [ips](eastl::string_view /* hostname */, sepclient::details::ResolveOptions /* options */) {
    return eastl::make_unique<ConstAsyncHostnameResolver>(ips);
  };
}


TEST_CASE("Check UrlSelector without hostnameResolver factory and without URLS", "[urlSelector]")
{
  sepclient::details::UrlSelector urlSelector({}, nullptr);
  CHECK(urlSelector.haveSomeUrls() == false);
  const auto selectedUrl = urlSelector.pollSelectingRandomUrl();
  CHECK(!selectedUrl.hasValue());
  CHECK(selectedUrl.url == "");
  CHECK(selectedUrl.connectIpHint == "");
}


TEST_CASE("Check UrlSelector with dummy_resolver_factory and without URLS", "[urlSelector]")
{
  sepclient::details::UrlSelector urlSelector({}, dummy_resolver_factory);
  CHECK(urlSelector.haveSomeUrls() == false);
  const auto selectedUrl = urlSelector.pollSelectingRandomUrl();
  CHECK(selectedUrl.hasValue() == false);
  CHECK(selectedUrl.url == "");
  CHECK(selectedUrl.connectIpHint == "");
}


TEST_CASE("Check UrlSelector with dummy_resolver_factory and with single URL", "[urlSelector]")
{
  sepclient::details::UrlSelector urlSelector({"https://example.com/path?arg"}, dummy_resolver_factory);

  CHECK(urlSelector.haveSomeUrls() == true);
  auto selectedUrl = urlSelector.pollSelectingRandomUrl();
  CHECK(selectedUrl.hasValue() == true);
  CHECK(selectedUrl.url == "https://example.com/path?arg");
  CHECK(selectedUrl.connectIpHint == "");

  urlSelector.removeFailedServerUrl(selectedUrl);

  CHECK(urlSelector.haveSomeUrls() == true);
  selectedUrl = urlSelector.pollSelectingRandomUrl();
  CHECK(selectedUrl.hasValue() == true);
  CHECK(selectedUrl.url == "https://example.com/path?arg");
  CHECK(selectedUrl.connectIpHint == "");
}


TEST_CASE("Check UrlSelector with dummy_resolver_factory and with two URLs", "[urlSelector]")
{
  sepclient::details::UrlSelector urlSelector(
    {
      "https://sub1.example.com/path?arg",
      "https://sub2.example.com/path?arg",
    },
    dummy_resolver_factory);

  CHECK(urlSelector.haveSomeUrls() == true);
  auto selectedUrl = urlSelector.pollSelectingRandomUrl();
  CHECK(selectedUrl.hasValue() == true);
  const eastl::string originalUrl = selectedUrl.url;
  CAPTURE(originalUrl);
  CHECK((selectedUrl.url == "https://sub1.example.com/path?arg" || selectedUrl.url == "https://sub2.example.com/path?arg"));
  CHECK(selectedUrl.connectIpHint == "");

  selectedUrl.connectIpHint = "1.2.3.4"; // simulate that IP hint was provided by cURL after connection
  urlSelector.removeFailedServerUrl(selectedUrl);

  {
    INFO("Another URL will be selected");
    CHECK(urlSelector.haveSomeUrls() == true);
    selectedUrl = urlSelector.pollSelectingRandomUrl();
    CHECK(selectedUrl.hasValue() == true);
    CHECK((selectedUrl.url == "https://sub1.example.com/path?arg" || selectedUrl.url == "https://sub2.example.com/path?arg"));
    CHECK(selectedUrl.url != originalUrl);
    CHECK(selectedUrl.connectIpHint == "");
  }

  urlSelector.removeFailedServerUrl(selectedUrl);

  {
    INFO("Any of the URLs will be selected again");
    CHECK(urlSelector.haveSomeUrls() == true);
    selectedUrl = urlSelector.pollSelectingRandomUrl();
    CHECK(selectedUrl.hasValue() == true);
    CHECK((selectedUrl.url == "https://sub1.example.com/path?arg" || selectedUrl.url == "https://sub2.example.com/path?arg"));
    CHECK(selectedUrl.connectIpHint == "");
  }
}


TEST_CASE("Check UrlSelector without hostnameResolver factory and with two URLs (should not crash)", "[urlSelector]")
{
  sepclient::details::UrlSelector urlSelector(
    {
      "https://sub1.example.com/path?arg",
      "https://sub2.example.com/path?arg",
    },
    nullptr);

  CHECK(urlSelector.haveSomeUrls() == true);
  auto selectedUrl = urlSelector.pollSelectingRandomUrl();
  CHECK(selectedUrl.hasValue() == true);
  CHECK((selectedUrl.url == "https://sub1.example.com/path?arg" || selectedUrl.url == "https://sub2.example.com/path?arg"));
  CHECK(selectedUrl.connectIpHint == "");
}


// ===========================================================
// Try hostname resolver
// ===========================================================


TEST_CASE("Check UrlSelector with const resolver (no IPs e.g missing Internet connection) and with single URL", "[urlSelector]")
{
  sepclient::details::UrlSelector urlSelector({"https://example.com/path?arg"}, create_const_resolver_factory({}));

  sepclient::details::SelectedUrlInfo selectedUrl;
  CHECK(selectedUrl.hasValue() == false);

  CHECK(urlSelector.haveSomeUrls() == true);
  do
  {
    selectedUrl = urlSelector.pollSelectingRandomUrl();
  } while (!selectedUrl.hasValue());
  CHECK(selectedUrl.url == "https://example.com/path?arg");
  CHECK(selectedUrl.connectIpHint == "");

  urlSelector.removeFailedServerUrl(selectedUrl);

  CHECK(urlSelector.haveSomeUrls() == true);
  do
  {
    selectedUrl = urlSelector.pollSelectingRandomUrl();
  } while (!selectedUrl.hasValue());
  CHECK(selectedUrl.url == "https://example.com/path?arg");
  CHECK(selectedUrl.connectIpHint == "");
}


TEST_CASE("Check UrlSelector with const resolver (single IP) and with single URL", "[urlSelector]")
{
  sepclient::details::UrlSelector urlSelector({"https://example.com/path?arg"}, create_const_resolver_factory({"111.222.33.44"}));

  sepclient::details::SelectedUrlInfo selectedUrl;
  CHECK(selectedUrl.hasValue() == false);

  CHECK(urlSelector.haveSomeUrls() == true);
  do
  {
    selectedUrl = urlSelector.pollSelectingRandomUrl();
  } while (!selectedUrl.hasValue());
  CHECK(selectedUrl.url == "https://example.com/path?arg");
  CHECK(selectedUrl.connectIpHint == "111.222.33.44");

  urlSelector.removeFailedServerUrl(selectedUrl);

  CHECK(urlSelector.haveSomeUrls() == true);
  do
  {
    selectedUrl = urlSelector.pollSelectingRandomUrl();
  } while (!selectedUrl.hasValue());
  CHECK(selectedUrl.url == "https://example.com/path?arg");
  CHECK(selectedUrl.connectIpHint == "111.222.33.44");
}


TEST_CASE("Check UrlSelector with const resolver (two IPs) and with single URL", "[urlSelector]")
{
  sepclient::details::UrlSelector urlSelector({"https://example.com/path?arg"},
    create_const_resolver_factory({"111.222.33.44", "55.44.33.22"}));

  sepclient::details::SelectedUrlInfo selectedUrl;
  CHECK(selectedUrl.hasValue() == false);

  CHECK(urlSelector.haveSomeUrls() == true);
  do
  {
    selectedUrl = urlSelector.pollSelectingRandomUrl();
  } while (!selectedUrl.hasValue());
  CHECK(selectedUrl.url == "https://example.com/path?arg");
  CHECK((selectedUrl.connectIpHint == "111.222.33.44" || selectedUrl.connectIpHint == "55.44.33.22"));
  const eastl::string originalIpHint = selectedUrl.connectIpHint;
  CAPTURE(originalIpHint);

  urlSelector.removeFailedServerUrl(selectedUrl);

  CHECK(urlSelector.haveSomeUrls() == true);
  do
  {
    selectedUrl = urlSelector.pollSelectingRandomUrl();
  } while (!selectedUrl.hasValue());
  CHECK(selectedUrl.url == "https://example.com/path?arg");
  CHECK((selectedUrl.connectIpHint == "111.222.33.44" || selectedUrl.connectIpHint == "55.44.33.22"));
  CHECK(selectedUrl.connectIpHint != originalIpHint);
}


TEST_CASE("Check UrlSelector with const resolver (two IPs) and with two URLs", "[urlSelector]")
{
  sepclient::details::UrlSelector urlSelector({"https://sub1.example.com/path?arg", "https://sub2.example.com/path?arg"},
    create_const_resolver_factory({"111.222.33.44", "55.44.33.22"}));

  sepclient::details::SelectedUrlInfo selectedUrl;
  CHECK(selectedUrl.hasValue() == false);

  CHECK(urlSelector.haveSomeUrls() == true);
  do
  {
    selectedUrl = urlSelector.pollSelectingRandomUrl();
  } while (!selectedUrl.hasValue());
  CHECK((selectedUrl.url == "https://sub1.example.com/path?arg" || selectedUrl.url == "https://sub2.example.com/path?arg"));
  CHECK((selectedUrl.connectIpHint == "111.222.33.44" || selectedUrl.connectIpHint == "55.44.33.22"));
  const eastl::string originalUrl = selectedUrl.url;
  const eastl::string originalIpHint = selectedUrl.connectIpHint;
  CAPTURE(originalUrl);
  CAPTURE(originalIpHint);

  urlSelector.removeFailedServerUrl(selectedUrl);

  CHECK(urlSelector.haveSomeUrls() == true);
  do
  {
    selectedUrl = urlSelector.pollSelectingRandomUrl();
  } while (!selectedUrl.hasValue());
  CHECK((selectedUrl.url == "https://sub1.example.com/path?arg" || selectedUrl.url == "https://sub2.example.com/path?arg"));
  CHECK((selectedUrl.connectIpHint == "111.222.33.44" || selectedUrl.connectIpHint == "55.44.33.22"));
  CHECK(selectedUrl.url != originalUrl);
  CHECK(selectedUrl.connectIpHint != originalIpHint);

  urlSelector.removeFailedServerUrl(selectedUrl);

  CHECK(urlSelector.haveSomeUrls() == true);
  do
  {
    selectedUrl = urlSelector.pollSelectingRandomUrl();
  } while (!selectedUrl.hasValue());
  CHECK((selectedUrl.url == "https://sub1.example.com/path?arg" || selectedUrl.url == "https://sub2.example.com/path?arg"));
  CHECK((selectedUrl.connectIpHint == "111.222.33.44" || selectedUrl.connectIpHint == "55.44.33.22"));
}
