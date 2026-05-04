.. _tutorial_dasHV_http_requests:

===========================================
HV-01 — Simple HTTP Requests
===========================================

.. index::
    single: Tutorial; HTTP
    single: Tutorial; HTTP Requests
    single: Tutorial; dasHV
    single: Tutorial; GET
    single: Tutorial; POST

This tutorial covers daslang's HTTP client bindings in the ``dashv`` module —
making GET, POST, PUT, PATCH, DELETE, and HEAD requests, passing custom
headers, and reading responses.

Setup
=====

Import the ``dashv`` module (via its boost helper)::

   require dashv/dashv_boost

All request functions create a fresh connection per call — ideal for scripts,
tools, and one-off requests. For persistent connections, custom timeouts, and
authentication, see :ref:`tutorial_dasHV_http_requests_advanced`.

GET Requests
============

The simplest call retrieves a resource with no body:

.. code-block:: das

   GET("http://example.com/api") <| $(resp) {
       print("status: {resp.status_code}, body: {resp.body}\n")
   }

Pass custom headers with a table literal:

.. code-block:: das

   GET(url, {"Accept" => "text/plain", "X-Request-Id" => "42"}) <| $(resp) {
       print("{resp.status_code}: {resp.body}\n")
   }

POST Requests
=============

POST sends a body string. An optional headers table is accepted as
the third argument:

.. code-block:: das

   POST(url, "hello") <| $(resp) {
       print("{resp.status_code}: {resp.body}\n")
   }

   // with headers
   POST(url, "\{\"msg\":\"hi\"\}", {"Content-Type" => "application/json"}) <| $(resp) {
       print("{resp.status_code}: {resp.body}\n")
   }

PUT and PATCH
=============

PUT and PATCH follow the same signature as POST — a URL, a body,
and optional headers and form-data tables:

.. code-block:: das

   PUT(url, "payload") <| $(resp) { ... }
   PUT(url, "payload", headers) <| $(resp) { ... }
   PUT(url, "", headers, form_data) <| $(resp) { ... }

   PATCH(url, "payload") <| $(resp) { ... }
   PATCH(url, "payload", headers) <| $(resp) { ... }
   PATCH(url, "", headers, form_data) <| $(resp) { ... }

DELETE and HEAD
===============

DELETE takes a URL and optional headers. HEAD is identical in
signature to GET but returns only the response headers (no body):

.. code-block:: das

   DELETE(url) <| $(resp) { ... }
   HEAD(url) <| $(resp) { ... }

Response Headers
================

Read individual headers with ``get_header``, or iterate all with
``each_header``.  Both accept an ``HttpResponse?`` pointer.
``each_header`` includes ``Set-Cookie`` entries (on responses) and
``Cookie`` entries (on requests) — see Tutorial 05 for the parsed
``each_cookie`` alternative.

.. code-block:: das

   GET(url) <| $(resp) {
       let ct = get_header(resp, "Content-Type")
       print("Content-Type: {ct}\n")

       each_header(resp) <| $(key, value) {
           print("  {key}: {value}\n")
       }
   }

Status Message
==============

``status_message`` returns the human-readable reason phrase (e.g.
``"OK"``, ``"Not Found"``):

.. code-block:: das

   GET(url) <| $(resp) {
       let msg = status_message(resp)
       print("Status: {resp.status_code} {msg}\n")
   }

Binary Data
===========

``resp.body`` is a string, but ``resp.content_length`` tells you the
exact byte count for binary payloads:

.. code-block:: das

   GET(url) <| $(resp) {
       print("Received {resp.content_length} bytes\n")
   }

Quick Reference
===============

====================================  ==============================================
Function                              Description
====================================  ==============================================
``GET(url) <| $(resp) { ... }``       GET request
``GET(url, headers) <| ...``          GET with custom headers
``POST(url, body) <| ...``            POST request
``POST(url, body, headers) <| ...``   POST with custom headers
``PUT(url, body) <| ...``             PUT request
``PUT(url, body, headers) <| ...``    PUT with custom headers
``PATCH(url, body) <| ...``           PATCH request
``PATCH(url, body, headers) <| ...``  PATCH with custom headers
``DELETE(url) <| ...``                DELETE request
``DELETE(url, headers) <| ...``       DELETE with custom headers
``HEAD(url) <| ...``                  HEAD request
``HEAD(url, headers) <| ...``         HEAD with custom headers
``get_header(resp, key)``             Read a single response header
``each_header(resp) <| $(k, v) {}``   Iterate all headers (including Set-Cookie)
``status_message(resp)``              Human-readable status phrase
====================================  ==============================================

.. seealso::

   Full source: :download:`tutorials/dasHV/01_http_requests.das <../../../../tutorials/dasHV/01_http_requests.das>`

   Next tutorial: :ref:`tutorial_dasHV_http_requests_advanced`
