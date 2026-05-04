.. _tutorial_dasHV_websockets:

=================================
HV-06 — WebSockets
=================================

.. index::
    single: Tutorial; WebSocket
    single: Tutorial; WebSocket Client
    single: Tutorial; WebSocket Server
    single: Tutorial; Broadcasting
    single: Tutorial; dasHV

This tutorial covers bidirectional WebSocket communication — creating
a WebSocket server with ``HvWebServer``, connecting clients with
``HvWebSocketClient``, sending and receiving messages, broadcasting,
and mixing HTTP routes with WebSocket endpoints.

Prerequisites: :ref:`tutorial_dasHV_http_server`.

WebSocket Server
================

``HvWebServer`` supports WebSocket connections out of the box.
Override three callbacks:

- ``onWsOpen(channel, url)`` — a client connected
- ``onWsClose(channel)`` — a client disconnected
- ``onWsMessage(channel, msg)`` — a text message arrived

.. code-block:: das

   class ChatServer : HvWebServer {
       clients : table<WebSocketChannel?; string>
       next_id : int = 0

       def override onWsOpen(channel : WebSocketChannel?; url : string#) {
           next_id++
           let nickname = "user_{next_id}"
           clients |> insert_clone(channel, nickname)
           send(channel, "welcome {nickname}", ws_opcode.WS_OPCODE_TEXT, true)
           broadcast("{nickname} joined", channel)
       }

       def override onWsClose(channel : WebSocketChannel?) {
           let nickname = clients?[channel] ?? "unknown"
           clients |> erase(channel)
           broadcast("{nickname} left", null)
       }

       def override onWsMessage(channel : WebSocketChannel?; msg : string#) {
           let nickname = clients?[channel] ?? "unknown"
           send(channel, "echo: {msg}", ws_opcode.WS_OPCODE_TEXT, true)
           broadcast("{nickname}: {msg}", channel)
       }

       def broadcast(msg : string; exclude : WebSocketChannel?) {
           for (ch in keys(clients)) {
               if (ch != exclude) {
                   send(ch, msg, ws_opcode.WS_OPCODE_TEXT, true)
               }
           }
       }
   }

The ``send`` function takes a channel, a message string, an opcode
(``ws_opcode.WS_OPCODE_TEXT`` or ``WS_OPCODE_BINARY``), and a
``fin`` flag (``true`` for a complete frame).

WebSocket Client
================

Subclass ``HvWebSocketClient`` and override:

- ``onOpen()`` — connection established
- ``onClose()`` — connection lost
- ``onMessage(msg)`` — text message received

.. code-block:: das

   class ChatClient : HvWebSocketClient {
       name : string
       received : array<string>

       def override onOpen {
           print("[{name}] connected\n")
       }

       def override onClose {
           print("[{name}] disconnected\n")
       }

       def override onMessage(msg : string#) {
           received |> push_clone(string(msg))
       }
   }

Connecting and Receiving
========================

Create a client, call ``init(url)`` to connect, then pump the event
queue with ``process_event_que()`` to receive callbacks:

.. code-block:: das

   var alice = new ChatClient()
   alice.name = "alice"
   alice->init("{base_url}/chat")

   // Wait for the welcome message
   wait_for_messages(alice, 1)

The ``init`` URL uses the ``ws://`` scheme.  The path (``/chat``)
is passed to the server's ``onWsOpen`` as the ``url`` parameter.

Sending Messages
================

Call ``send(text)`` on the client to send a text frame:

.. code-block:: das

   alice->send("hello server!")
   wait_for_messages(alice, 2)   // welcome + echo

The server receives the message in ``onWsMessage`` and can respond
with ``send(channel, ...)`` or broadcast to all clients.

Multiple Clients
================

Multiple clients can connect to the same server.  Each gets its own
``onWsOpen`` callback and a unique channel:

.. code-block:: das

   var bob = new ChatClient()
   bob.name = "bob"
   bob->init("{base_url}/chat")
   wait_for_messages(bob, 1)   // bob gets welcome

Broadcasting
============

The ``broadcast`` method in the example server sends a message to
every connected client except the sender.  When Bob sends a message,
Alice receives the broadcast and Bob receives the echo:

.. code-block:: das

   bob->send("hi everyone!")
   wait_for_messages(bob, 2)                            // welcome + echo
   wait_for_messages(alice, length(alice.received) + 1)  // broadcast

HTTP Alongside WebSocket
========================

``HvWebServer`` supports HTTP routes and WebSocket on the same port.
Register routes in ``onInit`` as usual — they work independently of
WebSocket callbacks:

.. code-block:: das

   def override onInit {
       GET("/ping") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
           return resp |> TEXT_PLAIN("pong")
       }
       GET("/clients") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
           return resp |> TEXT_PLAIN("{length(self.clients)}")
       }
   }

HTTP requests work normally while WebSocket clients are connected:

.. code-block:: das

   GET("http://127.0.0.1:{SERVER_PORT}/clients") <| $(resp) {
       print("connected clients: {resp.body}\n")
   }

Graceful Close
==============

Call ``close()`` on the client to send a WebSocket close frame.  This
triggers ``onClose`` on both the client and server sides.  Pump the
event queue until ``is_connected()`` returns false:

.. code-block:: das

   alice->close()
   bob->close()

   var elapsed = 0
   while ((alice->is_connected() || bob->is_connected()) && elapsed < 2000) {
       alice->process_event_que()
       bob->process_event_que()
       sleep(20u)
       elapsed += 20
   }

Server Lifecycle
================

The server runs on a background thread.  Use ``with_job_status`` for
synchronization and ``with_atomic32`` for the stop signal:

.. code-block:: das

   def with_ws_server(port : int; blk : block<(base_url : string) : void>) {
       with_job_status(1) $(started) {
           with_job_status(1) $(finished) {
               with_atomic32() $(stop_flag) {
                   new_thread() @() {
                       var server = new ChatServer()
                       server->init(port)
                       server->start()
                       started |> notify_and_release
                       while (stop_flag |> get == 0) {
                           server->tick()
                           sleep(10u)
                       }
                       server->stop()
                       unsafe {
                           delete server
                       }
                       finished |> notify_and_release
                   }
                   started |> join
                   invoke(blk, "ws://127.0.0.1:{port}")
                   stop_flag |> set(1)
                   finished |> join
               }
           }
       }
   }

Quick Reference
===============

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - Function / Override
     - Description
   * - ``onWsOpen(channel, url)``
     - Server: client connected
   * - ``onWsClose(channel)``
     - Server: client disconnected
   * - ``onWsMessage(channel, msg)``
     - Server: text message received
   * - ``send(channel, msg, opcode, fin)``
     - Server: send to one client
   * - ``onOpen()``
     - Client: connection established
   * - ``onClose()``
     - Client: connection lost
   * - ``onMessage(msg)``
     - Client: text message received
   * - ``init(url) : int``
     - Client: connect to WebSocket server
   * - ``send(text)``
     - Client: send a text message
   * - ``close() : int``
     - Client: graceful disconnect (close frame)
   * - ``is_connected() : bool``
     - Client: check connection state
   * - ``process_event_que()``
     - Client: pump event queue (call regularly)
   * - ``ws_opcode.WS_OPCODE_TEXT``
     - Text frame opcode
   * - ``ws_opcode.WS_OPCODE_BINARY``
     - Binary frame opcode

.. seealso::

   Full source: :download:`tutorials/dasHV/06_websockets.das <../../../../tutorials/dasHV/06_websockets.das>`

   Previous tutorial: :ref:`tutorial_dasHV_cookies_and_forms`

   Next tutorial: :ref:`tutorial_dasHV_sse_and_streaming`
