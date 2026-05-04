The DASHV module provides HTTP and WebSocket networking built on top of the
`libhv <https://github.com/ithewei/libhv>`_ library. It exposes WebSocket
client/server types, HTTP request/response handling, route registration,
cookie and form-data helpers, and an HTTP client for making outbound requests.

Use ``dashv_boost`` for the high-level daScript wrappers (``HvWebServer``,
``HvWebSocketClient``). The low-level C++ bindings live in this module.

All functions and symbols are in "dashv" module, use require to get access to it.

.. code-block:: das

    require dashv

See also:

  * :ref:`dashv_boost <stdlib_dashv_boost>` — high-level wrapper classes
  * `Tutorial HV-01 — HTTP requests <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasHV/01_http_requests.das>`_
  * `Tutorial HV-02 — Advanced HTTP requests <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasHV/02_http_requests_advanced.das>`_
  * `Tutorial HV-03 — HTTP server <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasHV/03_http_server.das>`_
  * `Tutorial HV-04 — Advanced HTTP server <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasHV/04_http_server_advanced.das>`_
  * `Tutorial HV-05 — Cookies and forms <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasHV/05_cookies_and_forms.das>`_
  * `Tutorial HV-06 — WebSockets <https://github.com/GaijinEntertainment/daScript/blob/master/tutorials/dasHV/06_websockets.das>`_
