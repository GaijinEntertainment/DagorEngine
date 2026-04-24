.. _tutorial_dasHV_http_requests_advanced:

===========================================
HV-02 — Advanced HTTP Requests
===========================================

.. index::
    single: Tutorial; HTTP
    single: Tutorial; HttpRequest
    single: Tutorial; dasHV
    single: Tutorial; Authentication
    single: Tutorial; Timeout

This tutorial covers the ``with_http_request`` RAII pattern — a safe way
to build fully-controlled HTTP requests with method, headers, body, timeouts,
authentication, query parameters, redirects, and content type.

Prerequisites: :ref:`tutorial_dasHV_http_requests` (the fire-and-forget API).

The with_http_request Pattern
=============================

``with_http_request`` allocates an ``HttpRequest``, passes it to a block
as a **temporary pointer** (``?#``), and deletes it when the block returns.
The temporary pointer prevents the caller from deleting or storing the
request outside the block — safe RAII lifetime:

.. code-block:: das

   with_http_request() <| $(var req) {
       req.method = http_method.GET
       req.url := "http://example.com/api"
       request(req) <| $(resp) {
           print("{resp.status_code}: {resp.body}\n")
       }
   }

.. important::

   ``HttpRequest`` is a native C++ object with virtual methods and
   ``std::string`` members. **Never** create one as ``var req : HttpRequest``
   — that zero-initializes a C++ object, causing undefined behavior.
   Use ``with_http_request()`` or ``new HttpRequest`` + ``unsafe { delete }``.

String fields (``url``, ``body``) are backed by ``std::string`` and must
be assigned with ``:=`` (clone), not ``=`` (copy).

Body and Headers
================

Set the body field and add custom headers with ``set_header()`` and
``set_content_type()``:

.. code-block:: das

   with_http_request() <| $(var req) {
       req.method = http_method.POST
       req.url := "{base_url}/echo"
       req.body := "Hello from request builder!"
       set_header(req, "X-Custom", "tutorial-02")
       set_content_type(req, "text/plain")
       request(req) <| $(resp) {
           print("{resp.status_code}: {resp.body}\n")
       }
   }

Timeouts
========

Two timeout helpers control the total request time and just the
connection phase:

.. code-block:: das

   set_timeout(req, 30)          // total request timeout (seconds)
   set_connect_timeout(req, 5)   // connection phase only

Both take ``int`` — no ``uint16`` cast needed.

Authentication
==============

Two convenience helpers set the ``Authorization`` header:

.. code-block:: das

   // Basic Auth — "Authorization: Basic <base64(user:pass)>"
   set_basic_auth(req, "username", "password")

   // Bearer Token — "Authorization: Bearer <token>"
   set_bearer_token_auth(req, "eyJhbGciOiJIUzI1NiJ9.example")

These are equivalent to calling ``set_header(req, "Authorization", ...)``
manually.

Query Parameters
================

``set_param`` appends ``?key=value`` to the URL; multiple parameters are
joined with ``&``. ``get_param`` reads a parameter back:

.. code-block:: das

   set_param(req, "page", "1")
   set_param(req, "limit", "10")

   let page = get_param(req, "page")   // "1"

Redirect Control
================

By default the HTTP client follows 3xx redirects automatically. You can
disable this:

.. code-block:: das

   allow_redirect(req, false)   // do NOT follow redirects

Content-Type
============

A shorthand for ``set_header(req, "Content-Type", ...)``:

.. code-block:: das

   set_content_type(req, "application/json")

Inspecting Request Fields
=========================

After building a request, you can read its fields before sending:

.. code-block:: das

   print("method:  {req.method}\n")
   print("url:     {req.url}\n")
   print("body:    {req.body}\n")
   print("timeout: {int(req.timeout)}\n")

Note: ``timeout`` is ``uint16`` — cast to ``int`` for decimal printing.

Quick Reference
===============

==========================================  ==============================================
Function                                    Description
==========================================  ==============================================
``with_http_request() <| $(var req) {}``    RAII request — auto new + delete
``request(req) <| $(resp) { ... }``         Send the request
``set_header(req, key, value)``             Add a custom header
``set_content_type(req, ct)``               Set Content-Type header
``set_basic_auth(req, user, pass)``         Basic authentication
``set_bearer_token_auth(req, token)``       Bearer token authentication
``set_timeout(req, seconds)``               Total request timeout
``set_connect_timeout(req, seconds)``       Connection phase timeout
``set_param(req, key, value)``              Add a query parameter
``get_param(req, key)``                     Read a query parameter
``allow_redirect(req, on)``                 Enable/disable auto-redirect
==========================================  ==============================================

.. seealso::

   Full source: :download:`tutorials/dasHV/02_http_requests_advanced.das <../../../../tutorials/dasHV/02_http_requests_advanced.das>`

   Previous tutorial: :ref:`tutorial_dasHV_http_requests`
