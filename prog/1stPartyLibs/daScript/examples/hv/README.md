# hv/ — WebSocket Chat Example

A WebSocket chat room powered by daslang's `dashv` module.
The server hosts an HTML chat page *and* speaks WebSocket — open a
browser tab and you're in the chat.

## Files

| File | Description |
|------|-------------|
| `ws_chat_server.das` | Chat server — serves HTML at `/`, WebSocket at `/chat` |
| `ws_chat_client.das` | Terminal chat client — type messages in the console |
| `chat.html`          | Standalone HTML chat client (can also be opened as a local file) |

## Quick Start

### 1. Start the server

```
daslang.exe examples/hv/ws_chat_server.das
```

You should see:

```
Starting chat server on port 9090...
  Open http://localhost:9090 in your browser
  Or run: daslang.exe examples/hv/ws_chat_client.das
  Press Ctrl+C to stop
```

### 2. Connect from a browser

Open **http://localhost:9090** in one or more browser tabs.
The server uses `set_document_root` + `set_home_page("chat.html")` so the
root URL serves the chat page automatically.

Alternatively, open `chat.html` as a local file (`file://...`).
It defaults to `ws://localhost:9090/chat` and auto-connects.

### 3. Connect from the terminal

In a second terminal:

```
daslang.exe examples/hv/ws_chat_client.das
```

Type messages and press Enter.  Messages from browser users and other
terminal clients appear inline.

### 4. Mix and match

Run any combination of browser tabs and terminal clients — they all
share the same chat room.  The server broadcasts every message to all
connected clients.

## Chat Commands

| Command | Description |
|---------|-------------|
| `/nick <name>` | Change your display name (1–20 chars) |
| `/quit`         | Disconnect (terminal client only) |

## Server Endpoints

| Endpoint | Description |
|----------|-------------|
| `GET /`         | Serves `chat.html` (home page) |
| `GET /ping`     | Health check (returns `pong`) |
| `GET /status`   | JSON `{"clients": N}` |
| `ws://host/chat` | WebSocket endpoint |

## Architecture

```
┌──────────────┐     ws://host/chat     ┌──────────────────┐
│ Browser tab  │◄──────────────────────►│                  │
└──────────────┘                        │  ws_chat_server  │
┌──────────────┐     ws://host/chat     │   (daslang)      │
│ Browser tab  │◄──────────────────────►│                  │
└──────────────┘                        │  Broadcasts to   │
┌──────────────┐     ws://host/chat     │  all clients     │
│ Terminal     │◄──────────────────────►│                  │
│ (das client) │                        └──────────────────┘
└──────────────┘          ▲
                          │ GET /
                    serves chat.html
```

The server uses `set_document_root(dir)` + `set_home_page("chat.html")`
to serve the chat page at `/`.  The HTML file can also be opened directly
as a local file (`file://...`) — it defaults to `ws://localhost:9090/chat`.
