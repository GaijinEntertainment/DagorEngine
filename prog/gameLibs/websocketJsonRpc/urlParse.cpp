// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <websocketJsonRpc/details/urlParse.h>


namespace websocketjsonrpc::details
{


eastl::string_view url_to_hostname(eastl::string_view url)
{
  // The most complicated URL format: scheme://[user:[password]@]host[:port][/path][?query][#fragment]
  //
  // Part "[user:[password]@]host[:port]" is usually called "authority".
  // "host" may be a bracketed IPv6 literal, e.g. [2001:db8::1], whose inner
  // colons are part of the address and must not be taken as the port delimiter.
  //
  // RFC 1738: https://datatracker.ietf.org/doc/html/rfc1738#section-3.1
  // RFC 3986: https://datatracker.ietf.org/doc/html/rfc3986#section-3.2

  constexpr eastl::string_view SCHEME_DELIMETER = "://";
  constexpr eastl::string_view CHARS_END_OF_AUTHORITY = "/?#";
  constexpr char END_OF_LOGIN_CHAR = '@';
  constexpr char PORT_DELIMETER_CHAR = ':';
  constexpr char IPV6_BEGIN_CHAR = '[';
  constexpr char IPV6_END_CHAR = ']';

  auto authorityBegin = url.find(SCHEME_DELIMETER);
  if (authorityBegin == eastl::string_view::npos)
    authorityBegin = 0;
  else
    authorityBegin += SCHEME_DELIMETER.size();

  const auto authorityEnd = url.find_first_of(CHARS_END_OF_AUTHORITY, authorityBegin);
  const auto authorityLength = authorityEnd == eastl::string_view::npos ? eastl::string_view::npos : authorityEnd - authorityBegin;
  const eastl::string_view authority = url.substr(authorityBegin, authorityLength);

  auto hostBegin = authority.find(END_OF_LOGIN_CHAR);
  if (hostBegin == eastl::string_view::npos)
    hostBegin = 0;
  else
    ++hostBegin;

  if (hostBegin < authority.size() && authority[hostBegin] == IPV6_BEGIN_CHAR)
  {
    const auto ipv6Begin = hostBegin + 1;
    const auto ipv6End = authority.find(IPV6_END_CHAR, ipv6Begin);
    if (ipv6End != eastl::string_view::npos)
      return authority.substr(ipv6Begin, ipv6End - ipv6Begin);
  }

  const auto hostEnd = authority.find(PORT_DELIMETER_CHAR, hostBegin);
  const auto hostLength = hostEnd == eastl::string_view::npos ? eastl::string_view::npos : hostEnd - hostBegin;

  const eastl::string_view host = authority.substr(hostBegin, hostLength);
  return host;
}


} // namespace websocketjsonrpc::details
