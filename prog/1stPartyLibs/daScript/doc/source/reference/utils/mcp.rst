.. _utils_mcp:

.. index::
   single: Utils; MCP Server
   single: Utils; AI Tool Integration
   single: Utils; Model Context Protocol

===========================================
 MCP Server --- AI Tool Integration
===========================================

The daslang MCP server exposes 28 compiler-backed tools to AI coding
assistants via the `Model Context Protocol <https://modelcontextprotocol.io/>`_.
It provides compilation diagnostics, type inspection, go-to-definition,
find-references, AST dump, AOT generation, expression evaluation,
parse-aware grep, and more.

Uses stdio transport -- no extra build dependencies.

.. contents::
   :local:
   :depth: 2


Quick start
===========

Test the server manually::

   # Windows
   bin/Release/daslang.exe utils/mcp/main.das

   # Linux
   ./bin/daslang utils/mcp/main.das

Configure in ``.mcp.json`` (project root):

.. code-block:: json

   {
     "mcpServers": {
       "daslang": {
         "command": "bin/Release/daslang.exe",
         "args": ["utils/mcp/main.das"]
       }
     }
   }

Or add via CLI::

   claude mcp add daslang -- bin/Release/daslang.exe utils/mcp/main.das

Claude Code starts and stops the server automatically with each
session.


Tools
=====

The server exposes 28 tools.  Each tool is invoked via MCP's
``tools/call`` method.

Compilation and diagnostics
---------------------------

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Tool
     - Description
   * - ``compile_check``
     - Compile a ``.das`` file and return errors/warnings plus a
       categorized function listing on success.  Optional ``json``
       for structured output.
   * - ``list_functions``
     - Compile a ``.das`` file and list all user functions, class
       methods, and generic instances (after macro expansion).
   * - ``list_types``
     - Compile a ``.das`` file and list all structs, classes (with
       fields), enums (with values), and type aliases.
   * - ``list_requires``
     - Compile a ``.das`` file and list all ``require`` dependencies
       (direct and transitive), with source file paths.  Optional
       ``json`` for structured output.
   * - ``list_modules``
     - List all available daslang modules (builtin C++ modules and
       daslib).  Optional ``json`` for structured output.
   * - ``list_module_api``
     - List all functions, types, enums, and globals exported by a
       builtin or daslib module.  Optional ``compact`` mode for large
       modules, ``filter`` for substring matching, ``section`` to
       limit output (e.g. ``functions``, ``structs``).

Code navigation
---------------

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Tool
     - Description
   * - ``goto_definition``
     - Given a cursor position (file, line, column), resolve the
       definition of the symbol under the cursor.  Returns location,
       kind, and source snippet.
   * - ``type_of``
     - Given a cursor position, return the resolved type of the
       expression under the cursor (innermost to outermost).
   * - ``find_references``
     - Find all references to the symbol under the cursor.  Scope:
       ``file`` (default) or ``all`` (all loaded modules).
   * - ``find_symbol``
     - Cross-module symbol search (functions, generics, structs,
       handled types, enums, globals, typedefs, fields).
       Case-insensitive substring by default; ``=query`` for exact
       match.

Cursor-based tools (``goto_definition``, ``type_of``,
``find_references``) support a ``no_opt`` parameter that disables
compiler optimizations to preserve the full AST -- useful when globals,
enum values, or bitfield constants get constant-folded away.

Program introspection
---------------------

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Tool
     - Description
   * - ``ast_dump``
     - Dump AST of an expression or compiled function.
       ``mode=ast`` returns S-expression, ``mode=source`` returns
       post-macro daslang code.  Optional ``lineinfo``.
   * - ``program_log``
     - Produce full post-compilation program text (like
       ``options log``).  Shows all types, globals, and functions after
       macro expansion.  Optional ``function`` filter.
   * - ``describe_type``
     - Describe a type's fields, methods, values, and base type.
       Supports structs, classes, handled types, enums, bitfields,
       variants, tuples, typedefs.

Execution
---------

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Tool
     - Description
   * - ``run_script``
     - Run a ``.das`` file or inline code snippet and return
       stdout/stderr.
   * - ``run_test``
     - Run dastest on a ``.das`` test file and return pass/fail
       results.  Optional ``json`` for structured output.
   * - ``eval_expression``
     - Evaluate a daslang expression and return its printed result.
       Supports module imports via ``require`` parameter.

Code generation and transformation
-----------------------------------

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Tool
     - Description
   * - ``format_file``
     - Format a ``.das`` file using the built-in formatter.
   * - ``convert_to_gen2``
     - Convert a ``.das`` file from gen1 syntax to gen2 using
       ``das-fmt``.  Optional ``inplace`` flag.
   * - ``aot``
     - Generate AOT (ahead-of-time) C++ code for a ``.das`` file or a
       single function.  Overloaded names return a disambiguation list.

Parse-aware search (tree-sitter)
---------------------------------

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Tool
     - Description
   * - ``grep_usage``
     - Parse-aware symbol search across ``.das`` files using ast-grep +
       tree-sitter.  Finds identifier occurrences excluding comments
       and strings.  Conditional on ``sg`` CLI.
   * - ``outline``
     - List all declarations in a file or set of files using
       tree-sitter.  Works on broken/incomplete code -- no compilation
       needed.  Conditional on ``sg`` CLI.

Live-reload control
-------------------

These tools interact with a running :ref:`daslang-live <utils_daslang_live>`
instance via its REST API.  All accept an optional ``port`` parameter
(default 9090).

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Tool
     - Description
   * - ``live_launch``
     - Launch ``daslang-live.exe`` on a script if not already running.
       Sets working directory to the script's folder.
   * - ``live_status``
     - Query the running instance for fps, uptime, paused state, and
       error status.
   * - ``live_error``
     - Retrieve the last compilation error (plain text).
   * - ``live_reload``
     - Trigger an incremental or full reload.
   * - ``live_pause``
     - Pause or unpause execution.
   * - ``live_command``
     - Dispatch a ``[live_command]`` by name with JSON arguments.
   * - ``live_shutdown``
     - Gracefully shut down the running instance.


ast-grep / tree-sitter setup
=============================

The ``grep_usage`` and ``outline`` tools use
`ast-grep <https://ast-grep.github.io/>`_ (``sg`` CLI) with a custom
tree-sitter grammar for daslang.  The ``sgconfig.yml`` config file is
platform-specific (shared library extension differs), so it is
gitignored.

Copy the appropriate template to ``sgconfig.yml`` in the project root:

.. code-block:: bash

   # Windows
   cp sgconfig.yml.windows sgconfig.yml

   # Linux
   cp sgconfig.yml.linux sgconfig.yml

   # macOS
   cp sgconfig.yml.osx sgconfig.yml


Architecture
============

- Each tool invocation runs in a **separate thread** with its own
  context/heap -- when the thread ends, its memory is freed without GC.
- Protocol logic lives in ``protocol.das``, the entry point is
  ``main.das``.
- Heap is collected after each request when over threshold (1 MB).
- Tool handlers are modular: each tool lives in ``tools/*.das``,
  shared utilities in ``tools/common.das``.


Protocol
========

The server implements `MCP <https://modelcontextprotocol.io/>`_ via
JSON-RPC 2.0 over stdio:

- Reads newline-delimited JSON (NDJSON) from stdin.
- Writes JSON-RPC responses to stdout (one line per message).
- Handles: ``initialize``, ``tools/list``, ``tools/call``, ``ping``.
- Logs to stderr and to ``utils/mcp/mcp_server.log``.
- File paths passed to tools are resolved relative to the server's
  working directory.


Configuring Claude Code permissions
=====================================

Optionally, allow all MCP tools without prompting by adding to
``.claude/settings.json``:

.. code-block:: json

   {
     "permissions": {
       "allow": [
         "mcp__daslang__compile_check",
         "mcp__daslang__list_functions",
         "mcp__daslang__list_types",
         "mcp__daslang__run_test",
         "mcp__daslang__format_file",
         "mcp__daslang__run_script",
         "mcp__daslang__ast_dump",
         "mcp__daslang__list_modules",
         "mcp__daslang__find_symbol",
         "mcp__daslang__list_requires",
         "mcp__daslang__list_module_api",
         "mcp__daslang__convert_to_gen2",
         "mcp__daslang__goto_definition",
         "mcp__daslang__type_of",
         "mcp__daslang__find_references",
         "mcp__daslang__program_log",
         "mcp__daslang__eval_expression",
         "mcp__daslang__describe_type",
         "mcp__daslang__grep_usage",
         "mcp__daslang__outline",
         "mcp__daslang__aot"
       ]
     }
   }


.. seealso::

   ``utils/mcp/README.md`` -- setup and configuration details

   `Model Context Protocol specification <https://modelcontextprotocol.io/>`_

   :ref:`utils_daslang_live` -- the live-reload application host controlled by ``live_*`` tools
