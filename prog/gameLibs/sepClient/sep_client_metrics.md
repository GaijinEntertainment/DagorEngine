# SEP Client - Statistics Metrics Reference

This document describes all metrics reported by the SEP Client library
to the statistics service via `statsd`.

The library consists of two layers, each reporting its own set of metrics:

| Layer                | Namespace                          | Description                    |
|----------------------|------------------------------------|--------------------------------|
| `websocketJsonRpc`   | `sep_client.*` (connection events) | WebSocket connection lifecycle |
| `sepClient`          | `sep_client.*` (request events)    | JSON-RPC request outcomes      |

---

## Global Tags

The following tags are attached to **every** metric by the `statsd` library
at initialization time. Their values are fixed for the lifetime of the process.

| Tag           | Type   | Example                | Description                                              |
|---------------|--------|------------------------|----------------------------------------------------------|
| `env`         | string | `production`, `test`   | Deployment environment                                   |
| `circuit`     | string | `venus`                | Circuit / deployment slot (primary environment selector) |
| `application` | string | `client`, `dedicated`  | Application role                                         |
| `platform`    | string | `pc`, `ps4`, `xboxone` | OS / hardware platform                                   |
| `project`     | string | `enlisted`, `wt`       | Game title                                               |
| `host`        | string | `host_client`          | Constant string for game client (hostname for dedic srv) |

> **Note:** `circuit` is the primary environment separator.
> A single `circuit` may contain multiple `env` values.
> Never mix data from different circuits on the same dashboard view.

---

## Connection Events

These metrics are emitted by the `websocketJsonRpc` layer and track
the WebSocket connection lifecycle.
Time values count from the moment the **connection attempt starts**.

### `sep_client.connect_initiate` - Counter

Emitted when a new WebSocket connection attempt is initiated.

| Tag          | Type   | Example            | Description                                    |
|--------------|--------|--------------------|------------------------------------------------|
| `project_id` | string | `enlisted`         | SEP project ID (game name); may be omitted     |
| `hostname`   | string | `sep.example.com`  | SEP server hostname from URL                   |
| `ip_address` | string | `203.0.113.10`     | IP address hint used for this connect attempt  |

---

### `sep_client.connect_success` - Profile (timer)

Emitted when a WebSocket connection is fully established (authentication complete).

| Tag          | Type   | Example            | Description                           |
|--------------|--------|--------------------|---------------------------------------|
| `project_id` | string | `enlisted`         | SEP project ID; may be empty          |
| `hostname`   | string | `sep.example.com`  | SEP server hostname                   |
| `ip_address` | string | `203.0.113.10`     | Actual IP address of the connection   |

**Timing:** milliseconds from connection attempt start to authentication completion.

---

### `sep_client.connect_failure` - Profile (timer)

Emitted when a WebSocket connection attempt fails (including authentication failure).

| Tag          | Type   | Example                   | Description                           |
|--------------|--------|---------------------------|---------------------------------------|
| `error_name` | string | `CURLE_COULDNT_CONNECT`   | Symbolic error name (see error table) |
| `project_id` | string | `enlisted`                | SEP project ID; may be empty          |
| `hostname`   | string | `sep.example.com`         | SEP server hostname                   |
| `ip_address` | string | `203.0.113.10`            | IP address of the failed attempt      |

**Timing:** milliseconds from connection attempt start to failure detection.

---

### `sep_client.disconnect` - Profile (timer)

Emitted when an established WebSocket connection is closed (for any reason).

| Tag          | Type   | Example            | Description                           |
|--------------|--------|--------------------|---------------------------------------|
| `error_name` | string | `WS_GOING_AWAY`    | Symbolic error name (see error table) |
| `project_id` | string | `enlisted`         | SEP project ID; may be empty          |
| `hostname`   | string | `sep.example.com`  | SEP server hostname                   |
| `ip_address` | string | `203.0.113.10`     | Actual IP address of the connection   |

**Timing:** milliseconds from successful connection establishment to disconnect
(i.e. the connection lifetime).

---

## Request Events

These metrics are emitted by the `sepClient` layer and track
individual JSON-RPC call outcomes.
Time values count from the moment the **request was last sent** over WebSocket.

The `service` and `action` tags are extracted from the JSON-RPC method name:

```
method = "<service>.<action>"   e.g. "profileSvc.changeProfileName"
```

### `sep_client.request_success` - Profile (timer)

Emitted when a JSON-RPC request receives a successful response.

| Tag          | Type   | Example               | Description                                      |
|--------------|--------|-----------------------|--------------------------------------------------|
| `project_id` | string | `enlisted`            | SEP project ID from request params; may be empty |
| `service`    | string | `profileSvc`          | Service name (part before the first `.`)         |
| `action`     | string | `changeProfileName`   | Action name (part after the first `.`)           |

**Timing:** milliseconds from last send to receiving the response.

---

### `sep_client.request_fail` - Profile (timer)

Emitted when a JSON-RPC request ultimately fails (after all retries).

| Tag          | Type   | Example                                      | Description                                           |
|--------------|--------|----------------------------------------------|-------------------------------------------------------|
| `reason`     | string | `network_error`, `timeout`, `json_rpc_error` | Failure category (see table below)                    |
| `error_name` | string | `WS_ABNORMAL_CLOSURE`, `SEP_APP_RETRY_LATER` | Symbolic error name; **absent** when `reason=timeout` |
| `project_id` | string | `enlisted`                                   | SEP project ID; may be empty                          |
| `service`    | string | `profileSvc`                                 | Service name                                          |
| `action`     | string | `changeProfileName`                          | Action name                                           |

**Failure reasons:**

| `reason`         | `error_name` present | Description                                                                                                                                                 |
|------------------|----------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `network_error`  | yes                  | WebSocket connection was lost after the request was sent (no retries left)                                                                                  |
| `timeout`        | no                   | No response received within `requestTimeoutMs`                                                                                                              |
| `json_rpc_error` | yes                  | Any other error response: server-returned JSON-RPC errors (−29xxx, −32xxx) or client-side parse failures (e.g. `SEP_CLIENT_INVALID_SERVER_RESPONSE` −40301) |

**Timing:** milliseconds from last send to the failure event.

---

### `sep_client.request_retry` - Profile (timer)

Emitted when a request is automatically retried after the WebSocket connection
was lost (before the retry budget is exhausted).

| Tag          | Type   | Example               | Description                               |
|--------------|--------|-----------------------|-------------------------------------------|
| `reason`     | string | `network_error`       | Always `network_error` for retries        |
| `error_name` | string | `WS_ABNORMAL_CLOSURE` | Symbolic name of the disconnect error     |
| `project_id` | string | `enlisted`            | SEP project ID; may be empty              |
| `service`    | string | `profileSvc`          | Service name                              |
| `action`     | string | `changeProfileName`   | Action name                               |

**Timing:** milliseconds from last send to initiating the retry.

> A request can be retried at most once (the library limits retries to 1
> to avoid traffic multiplication on consecutive disconnects).

---

## Error Name Reference

Error names reported in `error_name` tags are symbolic constants
resolved from numeric error codes.

Here you can find error examples. This is not a full list of errors.

### Transport / Network Errors (CURL codes, 1-99)

| `error_name`                     | Code | Description                     |
|----------------------------------|------|---------------------------------|
| `CURLE_COULDNT_RESOLVE_HOST`     |    6 | DNS resolution failed           |
| `CURLE_COULDNT_CONNECT`          |    7 | TCP connection refused/timeout  |
| `CURLE_OPERATION_TIMEDOUT`       |   28 | Network operation timed out     |
| `CURLE_SSL_CONNECT_ERROR`        |   35 | TLS/SSL handshake failed        |
| `CURLE_PEER_FAILED_VERIFICATION` |   60 | Certificate verification failed |
| `CURLE_LOGIN_DENIED`             |   67 | Authentication rejected         |

### WebSocket Close Codes (1000-3999)

| `error_name`          | Code | Description                      |
|-----------------------|------|----------------------------------|
| `WS_NORMAL_CLOSURE`   | 1000 | Clean close                      |
| `WS_GOING_AWAY`       | 1001 | Server/client going away         |
| `WS_PROTOCOL_ERROR`   | 1002 | Protocol violation               |
| `WS_ABNORMAL_CLOSURE` | 1006 | Connection dropped without close |
| `WS_SERVER_ERROR`     | 1011 | Server-side internal error       |
| `WS_SERVICE_RESTART`  | 1012 | Server restarting                |
| `WS_TRY_AGAIN_LATER`  | 1013 | Server overloaded, try later     |
| `WS_UNAUTHORIZED`     | 3000 | Authentication required          |
| `WS_FORBIDDEN`        | 3003 | Access denied                    |
| `WS_TIMEOUT`          | 3008 | Connection timed out by server   |

### SEP Server-Side JSON-RPC Errors (-29xxx)

| `error_name`               |   Code | Description               |
|----------------------------|--------|---------------------------|
| `SEP_APP_RETRY_LATER`      | -29501 | Temporary server overload |
| `SEP_AUTHENTICATION_ERROR` | -29502 | JWT authentication failed |
| `SEP_ACCESS_DENIED`        | -29503 | Permission denied         |

### Standard JSON-RPC 2.0 Errors (-32xxx)

| `error_name`                |   Code | Description                |
|-----------------------------|--------|----------------------------|
| `JSON_RPC_PARSE_ERROR`      | -32700 | Malformed JSON             |
| `JSON_RPC_INVALID_REQUEST`  | -32600 | Invalid JSON-RPC structure |
| `JSON_RPC_METHOD_NOT_FOUND` | -32601 | Unknown method             |
| `JSON_RPC_INVALID_PARAMS`   | -32602 | Invalid parameters         |
| `JSON_RPC_INTERNAL_ERROR`   | -32603 | Server internal error      |

### SEP Client-Side Errors (-40xxx)

| `error_name`                                         |   Code | Description                                      |
|------------------------------------------------------|--------|--------------------------------------------------|
| `SEP_CLIENT_WEBSOCKET_CONNECTION_IS_NOT_READY`       | -40101 | Request sent before connection was ready         |
| `SEP_CLIENT_LOST_WEBSOCKET_CONNECTION_AFTER_SENDING` | -40102 | Connection dropped after request was sent        |
| `SEP_CLIENT_CLIENT_LIBRARY_IS_NOT_INITIALIZED`       | -40201 | `initialize()` was not called before `rpcCall()` |
| `SEP_CLIENT_INVALID_SERVER_RESPONSE`                 | -40301 | Unparseable server response                      |
| `SEP_CLIENT_RPC_CALL_TIMEOUT`                        | -40302 | No response within `requestTimeoutMs`            |

> Unknown error codes are reported as `ERROR_<N>` or `ERROR_minus_<N>`
> for positive and negative codes respectively.

---

## Metric Type Notes

The library uses two statsd metric types:

| statsd type | Used for             | Graphite suffixes (typical)                                                                             |
|-------------|----------------------|---------------------------------------------------------------------------------------------------------|
| `counter`   | `connect_initiate`   | `.rate` (count per second)*                                                                             |
| `profile`   | all other metrics    | `.sum`, `.count`, `.lower`, `.upper`, `.stddev`, `.mean`, `.median`, `.10_percentile`, `.90_percentile` |

> Exact Graphite suffixes depend on your statsd daemon configuration.
> Adjust metric queries in dashboards accordingly.

---

## Implementation Reference

| File                                                                                      | Description                                            |
|-------------------------------------------------------------------------------------------|--------------------------------------------------------|
| [`sepClient/sepClientStats.h`](../publicInclude/sepClient/sepClientStats.h)               | `SepClientStatsCallback` interface (request events)    |
| [`websocketJsonRpc/wsJsonRpcStats.h`](../publicInclude/websocketJsonRpc/wsJsonRpcStats.h) | `WsJsonRpcStatsCallback` interface (connection events) |
| [`daNetGame/net/sepClientStats.cpp`](../../daNetGame/net/sepClientStats.cpp)              | Reference implementation using statsd                  |
