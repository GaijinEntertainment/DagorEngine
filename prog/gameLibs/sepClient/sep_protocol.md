# SEP network protocol for communicating via WebSocket

This new protocol is based on WebSocket + JSON RPC and it is designed
to be used between the Game Client and Gaijin servers.

The protocol is designed to replace existing HTTP protocols used in Gaijin in Game Clients.

## Authentication

Authentication is done using Gaijin SSO JWT.

SSO JWT token is passed as HTTP header during WebSocket upgrade request:

```yaml
Authorization: Bearer <SSO-JWT>
```

The following HTTP headers are also added by the Game Client:

```yaml
GJ-SEP-proto-ver  : 1.0   # fixed value for now
GJ-SEP-client-caps: none  # possible values: none, zstd
```

WebSocket upgrade is treated as an invisible authentication JSON RPC request with id `null`. \
Client must wait for authentication response before sending any RPC requests.

Client may or may not support `zstd` compression.
But it is **highly encouraged** to support the compression
since it reduces network latency and traffic by up to 20 times.

### FAQ

Q: What if client library cannot handle non-requested JSON RPC responses? I.e. it is dropping authentication response.

A: Such client library should send request with method `sep.waitForAuthenticationComplete` (without any params)
   after upgrading HTTP connection to WebSocket.
   The client must not send any other requests before response is received.
   The response with error code `-29502` (SEP_AUTHENTICATION_ERROR) should be treated as authentication failure.

### Authentication success

Authentication response in case of success:

```yaml
{
    "jsonrpc": "2.0",
    "id" : null,
    "result" : {
        "status" : "ok"
    }
}
```

### Authentication error

After sending automatic authentication error response to the Game Client, the SEP server keeps connection alive.
This allows custom clients to send request `sep.waitForAuthenticationComplete`.

Authentication response in case of failure:

```yaml
{
    "jsonrpc": "2.0",
    "id" : null,
    "error" : {
        "code"    : -29502,  # Gaijin's constant SEP_AUTHENTICATION_ERROR
        "message" : "JWT Token is expired",
    }
}
```

## JSON RPC Request

Example of the request message received from the Game Client via WebSocket:

```yaml
{
    "jsonrpc": "2.0",                     # required; value must be always "2.0"
    "id"     : 3,                         # optional; may be: int64, string (ASCII, length: [1..70])
    "method" : "profileSvc.changeProfileName", # length: [1..100]; ASCII
    "params" : {                          # object; required
        "projectId"       : "Enlisted",   # string; optional; length: [1..100]; ASCII
        "profileId"       : "2345",       # string; optional; currently non-negative int64; ASCII
        "transactId"      : "6544",       # string; required; currently non-negative int64; ASCII;
                                          # negative values are reserved for special requests; -1 is always invalid
        "actionParams"    : {             # object; optional; method specific params; ASCII
            "name"        : "Michael",
            "surname"     : "Johnson"
        }
    }
}
```

JSON RPC notification does not include `id` field.

### JSON RPC method schema

```yaml
method: <serviceNameInLowerCamelCase>.<actionName>
```

Currently, action name can contain dots, underscores.
In the future we plan to make action name lowerCamelCase.

Known existing service names:

- cloud
- contacts
- currency
- inventory
- leaderboard
- market
- profile
- ugcinfo
- userstat
- wtchar

## JSON RPC Response from SEP to the Game Client

Example of the response message going through WebSocket to the Game Client:

```yaml
{
    "jsonrpc": "2.0", # required; value must be always "2.0"
    "id"     : 3,     # possible types: int64, string (ASCII, length: [1..70]);
                      # required for request replies; missing for notifications from server
    "result" : {      # required on success, response must have "error" key on error
        "name"   : "Michael",
        "surname": "Johnson",
        "age"    : 23
    }
}
```

Response message may be compressed when it is sent to the Game Client via WebSocket.
In this case message will be sent as a binary WebSocket message (as opposite to usual text messages).

WebSocket binary message will have a zero-terminated compression name as a prefix.
Here is an example of such prefix for `zstd` algorithm:
```text
0000  7A 73 74 64 00  | zstd\0
```

Max allowed prefix length: 100 ASCII bytes including the terminating zero.

### zstd compression

Compressed part of the message consists of one or more separate compressed frames concatenated together.

### Server is shutting down notification

It is expected from the Game Client to start graceful reconnect
to another service after receiving this kind of notification.

The SEP server will provide only very limited time to reconnect
before it will forcibly close the WebSocket connection.

```yaml
{
    "jsonrpc": "2.0",
    "method" : "sep.NotifyClient.ServerIsGoingToShutdown"
}
```
