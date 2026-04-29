// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <websocketJsonRpc/details/urlParse.h>

#include <catch2/catch_test_macros.hpp>
#include <unittest/catch2_eastl_tostring.h>


static eastl::string_view to_host(eastl::string_view url) { return websocketjsonrpc::details::url_to_hostname(url); }


TEST_CASE("Test extracting hostname from URLs", "[urlParse]")
{
  CHECK(to_host("") == "");
  CHECK(to_host("a") == "a");
  CHECK(to_host("example.com") == "example.com");
  CHECK(to_host("example.com/") == "example.com");
  CHECK(to_host("http://example.com") == "example.com");
  CHECK(to_host("http://example.com?arg1=1") == "example.com");
  CHECK(to_host("http://example.com#param") == "example.com");
  CHECK(to_host("http://example.com?arg1=1#param") == "example.com");

  CHECK(to_host("http://example.com/") == "example.com");
  CHECK(to_host("https://example.com/") == "example.com");
  CHECK(to_host("http://example.com:1234/") == "example.com");
  CHECK(to_host("https://example.com:1234/") == "example.com");

  CHECK(to_host("https://@example.com:1234") == "example.com");
  CHECK(to_host("https://user@example.com:1234") == "example.com");
  CHECK(to_host("https://user:@example.com:1234") == "example.com");
  CHECK(to_host("https://user:pwd@example.com:1234") == "example.com");

  CHECK(to_host("user:pwd@example.com:1234") == "example.com");
  CHECK(to_host("user@example.com:1234") == "example.com");
  CHECK(to_host("user@example.com/") == "example.com");
  CHECK(to_host("user@example.com?arg") == "example.com");

  CHECK(to_host("https://@example.com:1234/") == "example.com");
  CHECK(to_host("https://user@example.com:1234/") == "example.com");
  CHECK(to_host("https://user:@example.com:1234/") == "example.com");
  CHECK(to_host("https://user:pwd@example.com:1234/") == "example.com");

  CHECK(to_host("https://user:pwd@example.sub.com:1234/path/sub.dir/doc.ext?arg1=@:&arg2=%77#param1") == "example.sub.com");
}
