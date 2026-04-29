// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// SEP protocol
// Data schema is defined in `sep_protocol.md` file (in the same directory as this file).

namespace sepclient::protocol
{

namespace http::headers
{

inline constexpr char AUTHORIZATION[] = "Authorization";
inline constexpr char AUTHORIZATION_BEARER_PREFIX[] = "Bearer ";

inline constexpr char GAIJIN_SEP_PROTOCOL_VERSION[] = "GJ-SEP-proto-ver";
inline constexpr char GAIJIN_SEP_PROTOCOL_VERSION_VALUE_1_0[] = "1.0";

inline constexpr char GAIJIN_SEP_CLIENT_CAPABILITIES[] = "GJ-SEP-client-caps";
inline constexpr char GAIJIN_SEP_CLIENT_CAPABILITIES_VALUE_NONE[] = "none";
inline constexpr char GAIJIN_SEP_CLIENT_CAPABILITIES_VALUE_ZSTD[] = "zstd";

} // namespace http::headers


namespace rpc::params
{

inline constexpr char PROJECT_ID[] = "projectId";
inline constexpr char PROFILE_ID[] = "profileId";
inline constexpr char TRANSACTION_ID[] = "transactId";
inline constexpr char ACTION_PARAMS[] = "actionParams";

} // namespace rpc::params


} // namespace sepclient::protocol
