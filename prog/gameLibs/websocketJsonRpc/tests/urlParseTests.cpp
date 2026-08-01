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

  // An '@' or ':' inside the path/query is not part of the authority and must
  // not be mistaken for a userinfo or port delimiter.
  CHECK(to_host("https://domain1.com/path1/@domain2.com/path2") == "domain1.com");
  CHECK(to_host("https://domain1.com/@domain2.com") == "domain1.com");
  CHECK(to_host("https://user:domain1.com/path1/@domain2.com/path2") == "user");
  CHECK(to_host("https://user:domain1.com/@domain2.com") == "user");

  CHECK(to_host("https://domain1.com?domain2.com") == "domain1.com");
  CHECK(to_host("https://domain1.com@domain2.com") == "domain2.com");

  // Bracketed IPv6 literal host: inner colons belong to the address, not the port.
  CHECK(to_host("https://[::1]") == "::1");
  CHECK(to_host("https://[::1]/") == "::1");
  CHECK(to_host("https://[::1]:1234") == "::1");
  CHECK(to_host("https://[::1]:1234/path?arg#param") == "::1");
  CHECK(to_host("https://[2001:db8::1]:443") == "2001:db8::1");
  CHECK(to_host("[2001:db8::1]:443") == "2001:db8::1");
  CHECK(to_host("https://user:pwd@[2001:db8::1]:443/path") == "2001:db8::1");

  // Malformed unterminated bracket: no ']', so parsing falls back to the plain
  // path where the first inner ':' ends the host. Best-effort, not a valid URL.
  CHECK(to_host("https://[2001:db8::1") == "[2001");
  CHECK(to_host("https://[2001:db8::1:443/path]/path2") == "[2001");
}
