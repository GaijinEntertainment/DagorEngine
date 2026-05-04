The DASHV_BOOST module provides high-level daScript wrapper classes for the
low-level ``dashv`` C++ bindings.

``HvWebServer`` wraps the WebSocket/HTTP server with convenient route
registration (``GET``, ``POST``, ``PUT``, ``DELETE``, ``PATCH``, ``HEAD``,
``ANY``), static file serving, and WebSocket event callbacks.

``HvWebSocketClient`` wraps the WebSocket client with ``onOpen``,
``onClose``, and ``onMessage`` callbacks.

All functions and symbols are in "dashv_boost" module, use require to get access to it.

.. code-block:: das

    require dashv/dashv_boost

See also:

  * :ref:`dashv <stdlib_dashv>` — low-level C++ bindings
  * `Tutorial HV-01 — HTTP requests <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasHV/01_http_requests.das>`_
  * `Tutorial HV-02 — Advanced HTTP requests <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasHV/02_http_requests_advanced.das>`_
  * `Tutorial HV-03 — HTTP server <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasHV/03_http_server.das>`_
  * `Tutorial HV-04 — Advanced HTTP server <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasHV/04_http_server_advanced.das>`_
  * `Tutorial HV-05 — Cookies and forms <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasHV/05_cookies_and_forms.das>`_
  * `Tutorial HV-06 — WebSockets <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasHV/06_websockets.das>`_
