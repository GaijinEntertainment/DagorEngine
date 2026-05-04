// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// JSON RPC 2.0 protocol constants
// https://www.jsonrpc.org/specification
namespace websocketjsonrpc::protocol
{

inline constexpr char VERSION[] = "jsonrpc";
inline constexpr char VERSION_VALUE[] = "2.0";
inline constexpr char MESSAGE_ID[] = "id";
inline constexpr char METHOD[] = "method";

inline constexpr char PARAMS[] = "params";

inline constexpr char RESULT[] = "result";

inline constexpr char ERROR[] = "error";
inline constexpr char ERROR_CODE[] = "code";
inline constexpr char ERROR_MESSAGE[] = "message";
inline constexpr char ERROR_DATA[] = "data";

// Gaijin Char-backend-specific error details format
inline constexpr char ERROR_DATA_CONTEXT[] = "contextName";
// Client-side error code for passing data inside of SepClient
inline constexpr char ERROR_DATA_REASON_ERROR_CODE[] = "reasonErrorCode";


namespace binary_message
{
inline constexpr size_t MESSAGE_PREFIX_MAX_LENGTH = 100; // including the terminating zero

inline constexpr char COMRESSION_ALGO_ZSTD[] = "zstd";
} // namespace binary_message


} // namespace websocketjsonrpc::protocol
