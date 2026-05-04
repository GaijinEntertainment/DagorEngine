.. _utils_daslang_live:

.. index::
   single: Utils; daslang-live
   single: Utils; Live Reload
   single: Utils; Hot Reload

==============================================
 daslang-live --- Live-Reload Application Host
==============================================

``daslang-live`` is a live-reloading application host for daslang.
Edit a ``.das`` file, save it, and the running application picks up
the changes instantly --- preserving windows, GPU state, game entities,
and everything stored in the persistent byte store.  No restart, no
lost state.  Applications range from games to REST APIs to MCP plugins.

.. contents::
   :local:
   :depth: 2


Designing for live reload
=========================

A live-reload host can recompile your script and swap in the new code,
but it cannot know *what matters* to your application.  A GLFW window
handle, a network socket, a game score, an audio buffer --- these are
application-specific.  The host provides the machinery; the application
must cooperate.

Five requirements every live-reloadable application must handle:

1. **Full restart capability.**
   ``init()`` and ``shutdown()`` must be idempotent.  The host may
   cold-start the application at any time --- after a crash, after a
   failed reload, or on first launch.

2. **Compilation failure recovery.**
   A typo should not kill a running application.  The host reverts to
   the old code and pauses, but the application must tolerate being
   frozen mid-frame until the next successful reload.

3. **Runtime exception handling.**
   A crash in ``update()`` should not corrupt persistent state.  The
   host clears the store on exception, so the application must handle
   starting from scratch gracefully.

4. **State persistence.**
   GPU resources (windows, buffers, shaders) and game state (entities,
   scores) live outside the script context.  The application must
   explicitly save and restore what matters across reloads.  The
   ``@live`` macro and ``decs_live`` module automate the common cases,
   but the developer decides what survives and what starts fresh.

5. **Instance management.**
   Only one instance should run.  The host enforces this via a system
   mutex, but the application's external resources (ports, files) must
   also be safe for single-instance operation.

This is by design: explicit control over persistence is safer than
magical state preservation that silently carries stale data.


Quick start
===========

Run the hello example::

   bin/Release/daslang-live.exe examples/daslive/hello/main.das

A GLFW window opens with a colored background.  Edit ``main.das``,
save --- the window stays open and the new code takes effect.

Here is the full ``hello/main.das``:

.. code-block:: das

   options gen2

   require live/glfw_live
   require opengl/opengl_boost
   require live/live_commands
   require live/live_api
   require daslib/json
   require daslib/json_boost
   require live_host

   // --- State ---

   var bg_r = 0.2f
   var bg_g = 0.3f
   var bg_b = 0.5f
   var frame_count : int = 0

   // --- Live commands ---

   [live_command]
   def set_color(input : JsonValue?) : JsonValue? {
       if (input != null && input.value is _object) {
           let tab & = unsafe(input.value as _object)
           var rv = tab?["r"] ?? null
           if (rv != null && rv.value is _number) {
               bg_r = float(rv.value as _number)
           }
           var gv = tab?["g"] ?? null
           if (gv != null && gv.value is _number) {
               bg_g = float(gv.value as _number)
           }
           var bv = tab?["b"] ?? null
           if (bv != null && bv.value is _number) {
               bg_b = float(bv.value as _number)
           }
       }
       return JV("\{\"r\": {bg_r}, \"g\": {bg_g}, \"b\": {bg_b}}")
   }

   // --- Lifecycle ---

   [export]
   def init() {
       live_create_window("Hello daslive", 640, 480)
       print("hello: init (is_reload={is_reload()})\n")
       if (!is_reload()) {
           frame_count = 0
       }
   }

   [export]
   def update() {
       if (!live_begin_frame()) {
           return
       }
       frame_count++
       if (frame_count % 300 == 0) {
           print("hello: frame {frame_count}, dt={get_dt()}, uptime={get_uptime()}\n")
       }
       var w, h : int
       live_get_framebuffer_size(w, h)
       glViewport(0, 0, w, h)
       glClearColor(bg_r, bg_g, bg_b, 1.0)
       glClear(GL_COLOR_BUFFER_BIT)
       live_end_frame()
   }

   [export]
   def shutdown() {
       print("hello: shutdown (frames={frame_count})\n")
       live_destroy_window()
   }

   // Dual-mode: also works with regular daslang.exe
   [export]
   def main() {
       init()
       while (!exit_requested()) {
           update()
       }
       shutdown()
   }

The same script works under both ``daslang-live.exe`` (live reload)
and ``daslang.exe`` (standalone).  Under ``daslang.exe`` the ``main()``
function drives the loop; under ``daslang-live.exe`` the host calls
``init()``, ``update()``, and ``shutdown()`` directly.


Mode detection
==============

The host inspects the script's exported functions to choose a mode:

- **Lifecycle mode** --- the script exports ``init()``.  The host calls
  ``init()``, loops ``update()``, and calls ``shutdown()`` on exit.
  This is the primary live-reload mode.

- **Simple mode** --- the script only exports ``main()``.  The host
  calls ``main()`` directly, behaving identically to ``daslang.exe``.


Lifecycle
=========

**Normal startup:**
``init()`` → ``update()`` loop → ``shutdown()`` on exit.

**Reload cycle** (triggered by file change or ``POST /reload``):

1. ``[before_reload]`` functions are called (save state).
2. ``shutdown()`` is called.
3. The host recompiles the script.
4. A new context is created.
5. ``[after_reload]`` functions are called (restore state).
6. ``init()`` is called in the new context.

**Failed reload:**
The host reverts to the old context, pauses execution, and stores the
compilation error.  Retrieve it via ``GET /error`` or
``get_last_error()``.  The next successful reload unpauses
automatically.

**Runtime exception:**
The host pauses, clears the persistent store (potentially corrupted),
and sets the error.


Core API
========

``require live_host``

Lifecycle and timing
--------------------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Function
     - Description
   * - ``is_live_mode() : bool``
     - True when running under ``daslang-live.exe``.
   * - ``is_reload() : bool``
     - True during the first frame after a reload.
   * - ``request_exit()``
     - Signal the host to exit after the current frame.
   * - ``exit_requested() : bool``
     - True after ``request_exit()`` or window close.
   * - ``request_reload(full : bool)``
     - Request a reload.  ``full=true`` also clears ``@live`` vars.
   * - ``get_dt() : float``
     - Frame delta time in seconds.
   * - ``get_uptime() : float``
     - Time since process start.  Does **not** reset on reload.
   * - ``get_fps() : float``
     - Current frames per second.
   * - ``is_paused() : bool``
     - True when execution is paused.
   * - ``set_paused(v : bool)``
     - Pause or unpause execution.

Persistent store
----------------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Function
     - Description
   * - ``live_store_bytes(key, data)``
     - Store a byte array under a string key.  Survives reloads.
   * - ``live_load_bytes(key, data) : bool``
     - Load a byte array.  Returns ``false`` if the key is not found.
   * - ``get_last_error() : string``
     - Last compilation error (empty string if none).


Reload annotations
==================

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Annotation
     - Description
   * - ``[before_reload]``
     - Called before ``shutdown()`` during a reload.  Save state here.
   * - ``[after_reload]``
     - Called after recompile, before ``init()``.  Restore state here.
   * - ``[before_update]``
     - Called every frame before ``update()``.  Used internally by
       ``live_api``.

The host discovers annotated functions by name prefix
(``__before_reload_*``, ``__after_reload_*``, ``__before_update_*``).
Multiple functions with the same annotation are all called.


State persistence
=================

``@live`` variables (recommended)
---------------------------------

``require live/live_vars``

Tag globals with ``@live`` and the macro auto-generates
``[before_reload]``/``[after_reload]`` handlers that serialize them
via ``Archive``.  No manual save/restore needed:

.. code-block:: das

   options gen2

   require live/live_vars
   require live_host

   var @live counter = 0
   var @live name = "world"
   var @live values : array<float>

   [export]
   def init() {
       if (!is_reload()) {
           counter = 0
           name = "world"
           values |> clear()
           values |> push(1.0)
           values |> push(2.0)
           values |> push(3.0)
           print("init: first run\n")
       } else {
           print("init: reload - counter={counter}, name={name}\n")
       }
   }

A hash of each variable's init expression is stored alongside the
data.  If you change the default value in code, old stored data is
discarded and the new default takes effect --- safe format migration.

Works with POD types, strings, enums, arrays, tables, and structs
with serializable fields.

Full reload (``request_reload(true)`` or ``POST /reload/full``) clears
all ``@live`` entries.

Manual serialization
--------------------

For complex cases (handles, non-serializable types), use
``[before_reload]``/``[after_reload]`` with ``live_store_bytes``/
``live_load_bytes`` and the ``Archive`` module.  See
``examples/daslive/reload_test/`` for the full pattern.


Helper modules
==============

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Module
     - Description
   * - ``live/glfw_live``
     - GLFW window that persists across reloads.
   * - ``live/opengl_live``
     - OpenGL screenshot command.
   * - ``live/decs_live``
     - Auto-serialization of DECS entities across reloads.
   * - ``live/live_commands``
     - ``[live_command]`` annotation for REST-callable functions.
   * - ``live/live_vars``
     - ``@live`` variable macro (auto-persistence).
   * - ``live/live_watch``
     - File watcher (auto-reload on save).
   * - ``live/live_watch_boost``
     - File watcher with diagnostic commands (recommended over
       ``live_watch``).
   * - ``live/live_api``
     - REST API server on port 9090.
   * - ``live/audio_live``
     - Audio state persistence across reloads.

``live/glfw_live``
------------------

The GLFW window handle is stored in the persistent byte store and
survives reloads.  Key functions:

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Function
     - Description
   * - ``live_create_window(title, w, h)``
     - Create or retrieve the persistent GLFW window.
   * - ``live_destroy_window()``
     - Destroy the window (skipped during reload).
   * - ``live_begin_frame() : bool``
     - Poll events, return ``false`` if window should close.
   * - ``live_end_frame()``
     - Swap buffers.
   * - ``live_get_framebuffer_size(w, h)``
     - Query framebuffer dimensions.

``live/decs_live``
------------------

Require this module and all DECS entities auto-persist across reloads.
Guard ``decs::restart()`` with ``is_reload()`` to avoid wiping
restored entities:

.. code-block:: das

   if (!is_reload()) {
       decs::restart()  // only on first run, not after reload
   }

``live/live_commands``
----------------------

Functions annotated ``[live_command]`` are callable via
``POST /command``.  Signature:
``def cmd_name(input : JsonValue?) : JsonValue?``.
Convention: prefix with ``cmd_``.  The ``set_color`` command in the
hello example above demonstrates the pattern.


REST API
========

``require live/live_api`` starts an HTTP server on port 9090.
Configure the port with ``live_api_set_port()`` before ``init()``.

Endpoints
---------

.. list-table::
   :header-rows: 1
   :widths: 10 20 70

   * - Method
     - Path
     - Description
   * - GET
     - ``/status``
     - JSON: ``fps``, ``uptime``, ``paused``, ``dt``, ``has_error``.
   * - GET
     - ``/error``
     - Plain text: last compilation error.
   * - POST
     - ``/reload``
     - Incremental reload.
   * - POST
     - ``/reload/full``
     - Full recompile (clears ``@live`` vars).
   * - POST
     - ``/pause``
     - Pause execution.  Returns 503 if compile error active.
   * - POST
     - ``/unpause``
     - Resume execution.
   * - POST
     - ``/shutdown``
     - Graceful shutdown.
   * - POST
     - ``/command``
     - Dispatch a ``[live_command]`` via JSON body:
       ``{"name":"cmd_name","args":{...}}``.
   * - ANY
     - ``*``
     - JSON help with all endpoints and curl examples.

curl examples
-------------

Check status::

   curl http://localhost:9090/status

Trigger a reload::

   curl -X POST http://localhost:9090/reload

Call a live command::

   curl -X POST http://localhost:9090/command \
     -d '{"name":"set_color","args":{"r":1.0,"g":0.0,"b":0.0}}'

When using the daslang MCP server, prefer ``live_*`` MCP tools over
curl (see :ref:`utils_mcp`).


CLI reference
=============

::

   daslang-live.exe [options] script.das [-- script-args...]

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Flag
     - Description
   * - ``-project <file>``
     - Project file (``.das_project``).
   * - ``-dasroot <path>``
     - Override ``DAS_ROOT``.
   * - ``-cwd``
     - Change to the script's directory before loading.
   * - ``--``
     - Separator: everything after is passed to the script.


Examples
========

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Path
     - Description
   * - ``examples/daslive/hello/``
     - Minimal GLFW window with background color tuning.
   * - ``examples/daslive/triangle/``
     - DECS + OpenGL shaders, rotating triangle.
   * - ``examples/daslive/arcanoid/``
     - Full breakout game: DECS, audio, particles, 30+ live commands.
   * - ``examples/daslive/sequence/``
     - Card board game: multi-module, bot AI, tournament runner.
   * - ``examples/daslive/tank_game/``
     - 3D tank combat with dynamic lighting.
   * - ``examples/daslive/live_vars_demo/``
     - ``@live`` variable persistence demo.
   * - ``examples/daslive/reload_test/``
     - Manual state persistence with ``[before_reload]``/``[after_reload]``.
   * - ``examples/daslive/test_commands/``
     - ``[live_command]`` registration and dispatch.
   * - ``examples/daslive/test_api/``
     - JSON-based live commands via REST.
   * - ``examples/daslive/test_api_http/``
     - Full HTTP REST API integration test.
   * - ``examples/daslive/test_decs_reload/``
     - DECS entity persistence verification.
   * - ``examples/daslive/test_watch/``
     - File watcher integration test.


Tips and gotchas
================

- ``.das_module`` changes require restarting ``daslang-live.exe``
  (module paths are registered at startup only).
- ``get_uptime()`` does **not** reset on reload --- use
  ``is_reload()`` to detect reloads.
- Debug agents persist across reloads; their code is **not** updated
  on reload (restart required to pick up agent code changes).
- ``[live_command]`` functions cannot be defined in the same module
  that registers them --- use a separate module.
- Failed reload pauses the host --- check ``GET /error`` or
  ``get_last_error()``.
- A single-instance lock prevents running two ``daslang-live.exe``
  processes on the same script.
- Guard ``decs::restart()`` with ``if (!is_reload())`` to avoid
  wiping restored entities.


.. seealso::

   ``examples/daslive/`` -- live-reload example applications

   `Running It Live <https://borisbat.github.io/dascf-blog/2026/03/20/running-it-live/>`_ -- blog post on live-coding philosophy

   :ref:`utils_mcp` -- MCP server with live-reload control tools (``live_*`` tools)
