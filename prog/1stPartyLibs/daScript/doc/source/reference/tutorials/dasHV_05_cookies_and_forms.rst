.. _tutorial_dasHV_cookies_and_forms:

=======================================
HV-05 — Cookies and Form / File Upload
=======================================

.. index::
    single: Tutorial; Cookies
    single: Tutorial; Form Upload
    single: Tutorial; File Upload
    single: Tutorial; URL-encoded Form
    single: Tutorial; dasHV

This tutorial covers cookie management and form/file upload — both on
the client side (building requests) and on the server side (reading
submitted data).

Prerequisites: :ref:`tutorial_dasHV_http_server` and
:ref:`tutorial_dasHV_http_server_advanced`.

Cookies
=======

Setting Cookies on a Response
-----------------------------

Inside a route handler, ``add_cookie(resp, name, value)`` appends a
``Set-Cookie`` header.  An extended overload accepts domain, path,
max-age, secure, and httponly flags:

.. code-block:: das

   GET("/set-cookies") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
       // Simple cookie
       add_cookie(resp, "session", "abc123")
       // Extended: domain, path, max_age, secure, httponly
       add_cookie(resp, "prefs", "dark-mode", "", "/", 3600, false, true)
       return resp |> TEXT_PLAIN("cookies set")
   }

Reading Cookies from a Request
------------------------------

On the server side, ``get_cookie`` reads a named cookie from the
request pointer:

.. code-block:: das

   GET("/read-cookies") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
       let session = get_cookie(req, "session")
       let prefs   = get_cookie(req, "prefs")
       // session and prefs are strings; empty if not found
       ...
   }

Iterating All Cookies
---------------------

``each_cookie`` iterates every cookie on a message, returning parsed
name/value pairs.  ``each_header`` also yields cookies, but as
serialized ``Set-Cookie`` (response) or ``Cookie`` (request) values.
Use ``each_cookie`` when you need individual fields; use
``each_header`` for a complete view of all HTTP headers.

.. code-block:: das

   each_cookie(req) <| $(name, value) {
       print("{name} = {value}\n")
   }

Sending Cookies from the Client
--------------------------------

When building a request with ``with_http_request``, ``add_cookie``
adds cookies that will be sent with the request.  Inside the
``with_http_request`` block, ``req`` is already a pointer, so no
``addr()`` is needed:

.. code-block:: das

   with_http_request() <| $(var req) {
       req.method = http_method.GET
       req.url := "http://localhost:8080/read-cookies"
       add_cookie(req, "session", "abc123")
       add_cookie(req, "prefs", "dark-mode")
       request(req) <| $(resp) {
           print("{resp.body}\n")
       }
   }

Multipart Form Data
====================

Client Side — Building Form Requests
--------------------------------------

Use ``set_form_data`` for text fields and ``set_form_file`` for file
attachments.  These set ``Content-Type: multipart/form-data``
automatically:

.. code-block:: das

   with_http_request() <| $(var req) {
       req.method = http_method.POST
       req.url := "http://localhost:8080/upload"
       set_form_data(req, "title", "My Document")
       set_form_data(req, "description", "A test upload")
       set_form_file(req, "attachment", "/path/to/file.txt")
       request(req) <| $(resp) {
           print("{resp.body}\n")
       }
   }

Server Side — Reading Form Fields
-----------------------------------

``get_form_data`` reads a single text field.  ``each_form_field``
iterates all fields (text and file):

.. code-block:: das

   POST("/upload") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
       let title = get_form_data(req, "title")
       each_form_field(req) <| $(name, content, filename) {
           if (!empty(filename)) {
               print("file: {filename}\n")
           } else {
               print("{name}: {content}\n")
           }
       }
       return resp |> TEXT_PLAIN("ok")
   }

Server Side — Saving Uploaded Files
-------------------------------------

``save_form_file(req, field_name, directory)`` writes the uploaded
file to disk.  If the path is a directory, the original filename is
preserved:

.. code-block:: das

   let status = save_form_file(req, "attachment", upload_dir)
   // status == 200 on success, 400 on bad request, 500 on I/O error

URL-Encoded Form Data
======================

For simple ``application/x-www-form-urlencoded`` submissions:

.. code-block:: das

   // Client side
   with_http_request() <| $(var req) {
       req.method = http_method.POST
       req.url := "http://localhost:8080/login"
       set_url_encoded(req, "username", "admin")
       set_url_encoded(req, "password", "secret123")
       request(req) <| $(resp) { ... }
   }

   // Server side
   POST("/login") <| @(var req : HttpRequest?; var resp : HttpResponse?) : http_status {
       let username = get_url_encoded(req, "username")
       let password = get_url_encoded(req, "password")
       ...
   }

Quick Reference
===============

.. list-table::
   :header-rows: 1
   :widths: 45 55

   * - Function
     - Description
   * - ``add_cookie(resp, name, value)``
     - Set simple cookie on response
   * - ``add_cookie(resp, name, value, domain, path, max_age, secure, httponly)``
     - Set cookie with extended attributes
   * - ``add_cookie(req, name, value)``
     - Add cookie to outgoing request
   * - ``get_cookie(req_ptr, name)``
     - Get cookie value (empty string if not found)
   * - ``each_cookie(msg_ptr) <| $(name, value) { }``
     - Iterate all cookies
   * - ``set_form_data(req, name, value)``
     - Set multipart form field (client)
   * - ``set_form_file(req, name, filepath)``
     - Set file upload field (client)
   * - ``get_form_data(req_ptr, name)``
     - Get form field value (server)
   * - ``save_form_file(req_ptr, name, dir)``
     - Save uploaded file to disk (server)
   * - ``each_form_field(req_ptr) <| $(name, content, filename) { }``
     - Iterate all form fields (server)
   * - ``set_url_encoded(req, key, value)``
     - Set URL-encoded form field (client)
   * - ``get_url_encoded(req_ptr, key)``
     - Get URL-encoded form field (server)

Source: ``tutorials/dasHV/05_cookies_and_forms.das``

.. seealso::

   Full source: :download:`tutorials/dasHV/05_cookies_and_forms.das <../../../../tutorials/dasHV/05_cookies_and_forms.das>`

   Next tutorial: :ref:`tutorial_dasHV_websockets`
