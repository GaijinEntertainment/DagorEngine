.. _tutorial_dasHV_sse_and_streaming:

=================================
HV-07 — SSE and Streaming
=================================

.. index::
    single: Tutorial; SSE
    single: Tutorial; Server-Sent Events
    single: Tutorial; Streaming
    single: Tutorial; request_cb
    single: Tutorial; dasHV

This tutorial covers Server-Sent Events (SSE) — a standard protocol for
server-to-client streaming over HTTP.  We build an SSE endpoint on the
server, then consume it from the client using ``request_cb``, which
delivers the response body incrementally as chunks arrive.

Prerequisites: :ref:`tutorial_dasHV_http_requests` and :ref:`tutorial_dasHV_http_server`.

What is SSE?
============

SSE (Server-Sent Events) is a one-way streaming protocol built on plain
HTTP.  The server sends a series of events as text lines:

.. code-block:: text

   event: message
   data: first

   event: message
   data: second

   event: complete
   data: all done

Each event has an ``event:`` type and a ``data:`` payload, separated by
a blank line.  The response uses ``Content-Type: text/event-stream``.

Unlike WebSockets, SSE requires no upgrade handshake — it's just HTTP
with a streaming body.  This makes it ideal for live feeds, log tailing,
and AI API responses (e.g., streaming chat completions).

Server-Side SSE Endpoint
=========================

Register an SSE endpoint with ``SSE()``.  It has the same handler
signature as ``GET`` or ``POST`` — receive ``(req, resp)`` and return
an ``http_status``.  Set the SSE headers and write events to the body:

.. code-block:: das

   class SseTutorialServer : HvWebServer {
       def override onInit {
           SSE("/events") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
               set_header(resp, "Content-Type", "text/event-stream")
               set_header(resp, "Cache-Control", "no-cache")
               var body = sse_event_string("first", "message")
               body += sse_event_string("second", "message")
               body += sse_event_string("all done", "complete")
               return resp |> TEXT_PLAIN(body)
           }
       }
   }

``SSE()`` uses ``ANY`` method matching, so both GET and POST requests
reach the handler.  The helper ``sse_event_string`` formats one event:

.. code-block:: das

   def sse_event_string(data, event : string) : string {
       return "event: {event}\ndata: {data}\n\n"
   }

Client-Side Streaming with request_cb
======================================

``request_cb`` works like ``request()`` but calls a per-chunk callback
as body data arrives.  This is the building block for consuming SSE
streams.

Two overloads are available:

- **String chunks** — convenient for text protocols like SSE:

.. code-block:: das

   with_http_request() <| $(var req) {
       req.method = http_method.GET
       req.url := "{base_url}/events"
       var chunks : array<string>
       request_cb(req, $(data : string) {
           chunks |> push(clone_string(data))
       }) <| $(var resp) {
           print("status: {int(resp.status_code)}\n")
       }
       for (chunk in chunks) {
           print("chunk: \"{chunk}\"\n")
       }
   }

- **Raw bytes** — useful for binary data or exact byte counts:

.. code-block:: das

   with_http_request() <| $(var req) {
       req.method = http_method.GET
       req.url := "{base_url}/events"
       var total_bytes = 0
       request_cb(req, $(data; var size : int) {
           total_bytes += size
       }) <| $(var resp : HttpResponse?) {
           print("status: {int(resp.status_code)}\n")
       }
       print("received {total_bytes} total bytes\n")
   }

.. note::

   ``request_cb`` works with any HTTP endpoint — it's not SSE-specific.
   The callback fires for every body chunk regardless of Content-Type.

POST with Streaming Response
============================

``request_cb`` works with any HTTP method.  Here we POST a body and
stream the SSE response:

.. code-block:: das

   with_http_request() <| $(var req) {
       req.method = http_method.POST
       req.url := "{base_url}/stream-echo"
       req.body := "hello from POST"
       set_content_type(req, "text/plain")
       var chunks : array<string>
       request_cb(req, $(data : string) {
           chunks |> push(clone_string(data))
       }) <| $(var resp) {
           print("status: {int(resp.status_code)}\n")
       }
   }

Parsing SSE Events
==================

SSE events are line-based.  A minimal parser accumulates chunks into a
buffer, splits on newlines, and extracts ``event:`` and ``data:`` fields.
A blank line signals the end of one event:

.. code-block:: das

   struct SseEvent {
       event : string
       data : string
   }

   def parse_sse_events(body : string) : array<SseEvent> {
       var events : array<SseEvent>
       var current_event = ""
       var current_data = ""
       peek_data(body) $(bytes) {
           var pos = 0
           let len = length(bytes)
           while (pos < len) {
               var eol = pos
               while (eol < len && int(bytes[eol]) != '\n') {
                   eol++
               }
               let line_len = eol - pos
               if (line_len == 0) {
                   if (!empty(current_data) || !empty(current_event)) {
                       events |> emplace(SseEvent(event = current_event, data = current_data))
                       current_event = ""
                       current_data = ""
                   }
               } else {
                   let line = slice(body, pos, eol)
                   if (starts_with(line, "event: ")) {
                       current_event = slice(line, 7)
                   } elif (starts_with(line, "data: ")) {
                       current_data = slice(line, 6)
                   }
               }
               pos = eol + 1
           }
       }
       return <- events
   }

Use it with ``request_cb`` to parse a streaming SSE response:

.. code-block:: das

   var chunks : array<string>
   request_cb(req, $(data : string) {
       chunks |> push(clone_string(data))
   }) <| $(var resp) {
       assert(resp.status_code == http_status.OK)
   }
   var full = ""
   for (chunk in chunks) {
       full = "{full}{chunk}"
   }
   var events <- parse_sse_events(full)
   for (evt in events) {
       print("[{evt.event}] {evt.data}\n")
   }

Output::

   [message] first
   [message] second
   [complete] all done

Quick Reference
===============

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - Function
     - Description
   * - ``SSE(uri) <| @(req, resp) { ... }``
     - Server: register SSE endpoint (any HTTP method)
   * - ``set_header(resp, k, v)``
     - Set a response header (e.g., Content-Type)
   * - ``resp |> TEXT_PLAIN(body)``
     - Set body and return 200 OK
   * - ``request_cb(req, on_chunk) <| $(resp) { ... }``
     - Client: streaming request with per-chunk callback
   * - ``$(data : string) { ... }``
     - String chunk callback overload
   * - ``$(data; var size : int) { ... }``
     - Raw bytes chunk callback overload

.. seealso::

   Full source: :download:`tutorials/dasHV/07_sse_and_streaming.das <../../../../tutorials/dasHV/07_sse_and_streaming.das>`

   Previous tutorial: :ref:`tutorial_dasHV_websockets`
