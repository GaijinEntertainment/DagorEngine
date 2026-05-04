.. _utils_daspkg:

.. index::
   single: Utils; daspkg
   single: Utils; Package Manager
   single: Utils; .das_package

===================================
 daspkg --- Package Manager
===================================

daspkg is the daslang package manager.  It installs, updates, builds,
and manages daslang modules from git repositories or a central package
index.

.. contents::
   :local:
   :depth: 2


Quick start
===========

.. code-block:: bash

   # install a package by name (from the index)
   daslang utils/daspkg/main.das -- install daspkg-test-pure

   # install a package by URL
   daslang utils/daspkg/main.das -- install github.com/borisbat/daspkg-test-pure

   # install a specific version
   daslang utils/daspkg/main.das -- install github.com/borisbat/daspkg-test-versions@v1.0

   # install all dependencies listed in .das_package
   daslang utils/daspkg/main.das -- install

   # update all packages
   daslang utils/daspkg/main.das -- update

   # search the package index
   daslang utils/daspkg/main.das -- search math


Why daspkg?
===========

daslang's ``modules/`` directory and ``.das_module`` descriptors
(:ref:`embedding_external_modules`) already provide runtime module
discovery.  daspkg automates the **acquisition** side: cloning from
git, resolving versions, installing transitive dependencies, and
building C++ modules -- so you can go from ``require foo/bar`` to a
working build in one command.

Design principles:

- **Git-based, like Go modules** -- packages are git repositories.
  No centralized registry server to maintain.
- **Executable manifests** -- ``.das_package`` is a daslang script, not
  a static JSON file.  Version resolution, dependency declarations, and
  build steps can contain arbitrary logic.
- **Per-project by default** -- packages install into the project's
  ``modules/`` directory (like ``node_modules``).  Reproducible builds
  by default.  Large modules can be installed **globally** with
  ``--global`` to avoid redundant clones and builds across projects.
- **Two package types** -- pure-daslang (just ``.das`` files, no build)
  and C++ (cmake auto-build from source).


Commands
========

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Command
     - Description
   * - ``install <url|name|path>[@version]``
     - Install a package from git, index, or local path.
   * - ``install`` (no arguments)
     - Install all dependencies from the project's ``.das_package``.
   * - ``remove <name>``
     - Remove an installed package.
   * - ``update [name]``
     - Re-install one or all packages at their pinned version
       (re-clone, rebuild).
   * - ``upgrade [name]``
     - Upgrade one or all packages to the latest version.
   * - ``list``
     - List installed packages.
   * - ``search <query>``
     - Search the package index.
   * - ``build``
     - Build all C/C++ packages (cmake).
   * - ``check``
     - Verify installed packages are present and have ``.das_module``.
   * - ``doctor``
     - Check environment (git, cmake, gh).
   * - ``introduce [url]``
     - Submit a package to the index via PR (requires ``gh`` CLI).
   * - ``withdraw <name>``
     - Remove a package from the index via PR (requires ``gh`` CLI).

All commands that operate on packages (``install``, ``remove``,
``update``, ``upgrade``, ``list``, ``check``, ``build``) accept the
``--global`` flag.

Options:

- ``--root <path>`` -- project root (default: current directory).
- ``--force`` -- force reinstall even if already installed.
- ``--global``, ``-g`` -- operate on global modules in
  ``{das_root}/modules/`` (see :ref:`daspkg_global_modules`).
- ``--color`` / ``--no-color`` -- enable/disable ANSI colored output.
- ``--verbose``, ``-v`` -- print debug details (git commands, resolve
  steps, file operations).
- ``--json`` -- machine-readable JSON output (``search``, ``list``,
  ``check``).


Package sources
===============

Packages can come from three sources:

**Git URL** -- the most common case::

   daspkg install github.com/user/repo
   daspkg install github.com/user/repo@v1.0

**Index name** -- looks up the URL in the curated package index::

   daspkg install das-claude

**Local path** -- copies from a local directory (no git)::

   daspkg install ./path/to/mymodule


The ``.das_package`` manifest
==============================

Every package is described by a ``.das_package`` file -- an executable
daslang script that registers metadata, version resolution,
dependencies, and build info.  This replaces static JSON manifests
with programmable logic.

.. code-block:: das

   options gen2

   require daslib/daspkg

   // Required: package identity
   [export]
   def package() {
       package_name("mymodule")
       package_author("username")
       package_description("What this module does")
       package_source("github.com/user/mymodule")
   }

   // Optional: how to resolve a version request into a git ref
   [export]
   def resolve(sdk_version, version : string) {
       if (version == "" || version == "latest") {
           download_tag("v2.0")
       } elif (version == "1.x") {
           download_tag("v1.5")
       } else {
           download_tag("v{version}")
       }
   }

   // Optional: dependencies on other packages
   [export]
   def dependencies(version : string) {
       require_package("github.com/user/dep-a", ">=1.0")
       require_package("github.com/user/dep-b")
   }

   // Optional: build step (default: no build)
   [export]
   def build() {
       cmake_build()
   }

All functions except ``package()`` are optional.  A repo without
``.das_package`` gets a "dumb clone" -- no version resolution, no
dependency tracking, no build.

Manifest functions
------------------

``package()``
   Required.  Declares package identity with registration functions:

   - ``package_name(name)`` -- unique package name
   - ``package_author(author)`` -- author name
   - ``package_description(desc)`` -- short description
   - ``package_source(url)`` -- canonical source URL

``resolve(sdk_version, version : string)``
   Optional.  Receives the daslang SDK version and the user-requested
   version string, and calls one of:

   .. list-table::
      :header-rows: 1
      :widths: 40 60

      * - Function
        - Effect
      * - ``download_tag("v1.0")``
        - Fetch and checkout git tag ``v1.0``
      * - ``download_branch("main")``
        - Fetch and checkout branch ``main``
      * - ``download_redirect("github.com/org/new-repo", "v2.0")``
        - Re-clone from a different repository

   If no ``resolve()`` function exists, the version string (from
   ``install foo@v1.0``) is used as a git tag directly.  If no
   version is specified, the default branch is used.

``dependencies(version : string)``
   Optional.  Declares dependencies on other packages:

   .. code-block:: das

      require_package("github.com/user/dep-a", ">=1.0")
      require_package("github.com/user/dep-b")

   Version constraints use operators: ``>=``, ``>``, ``<=``, ``<``,
   ``=`` (or bare version for exact match).  Comma-separated AND:
   ``">=1.0,<2.0"``.  Semver parsing strips ``v`` prefix.

``build()``
   Optional.  Declares how to build the package:

   - ``cmake_build()`` -- cmake configure + build with
     ``-DDASLANG_DIR=<sdk_root>``
   - ``custom_build("make all")`` -- run an arbitrary command
   - ``no_build()`` -- explicitly skip build

   If omitted, no build step runs (pure-daslang package).


Install flow
============

When you run ``daspkg install github.com/user/repo@v1.0``:

1. Shallow-clone the package from the default branch into a temp
   directory.
2. If ``.das_package`` exists and has ``resolve()`` -- call it with
   ``sdk_version`` and ``"v1.0"``.  Checkout the resolved tag/branch.
   On redirect, discard the clone and re-clone from the new URL.
3. If resolve returned nothing and a version was requested -- checkout
   the version string as a git tag.
4. Move to ``modules/<name>/``.
5. Record in ``daspkg.lock``.
6. Install transitive dependencies (from ``dependencies()``).
7. Auto-build if ``.das_package`` has ``build()``.


.. _daspkg_global_modules:

Global modules
==============

By default, packages install per-project into ``{root}/modules/``.
Large packages with native builds (e.g. dasImgui) can be installed
**globally** -- once under ``{das_root}/modules/`` -- and shared across
all projects using that daScript SDK.

.. code-block:: bash

   # install globally
   daspkg install --global dasImgui

   # list / update / upgrade / remove / build / check globally
   daspkg list --global
   daspkg update --global dasImgui
   daspkg upgrade --global dasImgui
   daspkg remove --global dasImgui
   daspkg build --global
   daspkg check --global

Install behavior
----------------

- **Global install** (``--global``): clones and builds in
  ``{das_root}/modules/``, records in
  ``{das_root}/modules/.daspkg_global.lock``.
- **Local install auto-uses global**: ``daspkg install foo`` checks the
  global lock file first.  If a compatible global version exists, it
  records a reference (``"global": true`` in the project lock file)
  instead of cloning -- zero network, zero build time.
- **Version mismatch**: if the global version doesn't satisfy the
  requested version, daspkg errors with a suggestion.  Use ``--force``
  to install locally, or ``--global`` to update the global copy.
- **Dependencies**: global packages' dependencies also install globally.
  Built-in SDK modules already in ``{das_root}/modules/`` are skipped
  automatically.

Coexistence (local + global)
----------------------------

A package can exist both locally and globally.  The C++ runtime
(``require_dynamic_modules``) handles this via **shadow detection**:

- If the same module directory exists in both ``{das_root}/modules/``
  and ``{project_root}/modules/``, the **local version wins**.
- A warning is printed:
  ``Warning: local 'dasImgui' shadows global -- using local``
- This is safe -- removing the local copy seamlessly falls back to the
  global one.

Remove behavior
---------------

- ``daspkg remove --global foo`` -- deletes ``{das_root}/modules/foo/``
  and removes from global lock file.
- ``daspkg remove foo`` (where project entry has ``"global": true``) --
  only removes the project lock file reference; the global directory is
  **not** deleted.

CMake integration
-----------------

Global packages that use ``cmake_build()`` or ``custom_build()`` get a
``.daspkg_standalone`` marker file.  The main daScript ``CMakeLists.txt``
skips directories with this marker during auto-discovery, preventing
build errors from standalone CMakeLists.txt files that expect
``DASLANG_DIR`` to be set explicitly.


Project layout
==============

::

   my_project/
     main.das
     .das_package                   # project manifest (lists dependencies)
     daspkg.lock                    # installed packages, versions, sources
     modules/
       <package_name>/
         .das_module                # provided by package author
         .das_package               # package manifest
         _build/                    # cmake build directory (if C++ package)
         *.shared_module            # built C++ output (if applicable)
         *.das                      # source files
       .daspkg_cache/               # index cache (gitignored)
       .daspkg_tmp/                 # temp dir during install (gitignored)

   {das_root}/                       # daScript SDK root
     modules/
       .daspkg_global.lock           # global lock file (gitignored)
       <global_package>/
         .daspkg_standalone          # marker: built by daspkg, skip in CMake
         ...                         # same structure as local packages


Lock file
=========

``daspkg.lock`` tracks installed packages:

.. code-block:: json

   {
       "sdk_version": "",
       "packages": [
           {
               "name": "mymodule",
               "source": "github.com/user/mymodule",
               "version": "1.0",
               "tag": "v1.0",
               "branch": "",
               "root": true,
               "local": false,
               "global": false
           }
       ]
   }

- **root** -- ``true`` if the user explicitly installed this package;
  ``false`` if it was pulled in as a transitive dependency.
- **version** -- what the user requested.
- **tag/branch** -- what ``.das_package`` resolved to (the actual git ref).
- **local** -- ``true`` for packages installed from a local path.
- **global** -- ``true`` if the package is resolved from a global
  install in ``{das_root}/modules/`` (no local copy).

The global lock file (``{das_root}/modules/.daspkg_global.lock``) uses
the same format to track globally installed packages.


Package index
=============

The `daspkg-index <https://github.com/borisbat/daspkg-index>`_
repository hosts a curated ``packages.json`` file.  Each entry
contains the package name, URL, description, author, license, tags,
and modules.

Searching::

   daspkg search math

Installing by name::

   daspkg install das-claude

Publishing a package to the index::

   daspkg introduce github.com/user/mymodule

This clones the index repo, adds your package to ``packages.json``,
and creates a PR on GitHub (requires ``gh`` CLI).

Removing a package from the index::

   daspkg withdraw mymodule


Use-case examples
=================

Pure daslang package
--------------------

A module that is just ``.das`` files -- the most common case.

Package structure::

   mymodule/
     .das_package
     .das_module
     daslib/
       mymodule.das

``.das_package``:

.. code-block:: das

   options gen2
   require daslib/daspkg

   [export]
   def package() {
       package_name("mymodule")
       package_description("My pure daslang module")
   }


C++ package with cmake build
-----------------------------

A module that wraps native code.  daspkg auto-builds it after install.

.. code-block:: das

   options gen2
   require daslib/daspkg

   [export]
   def package() {
       package_name("fastmath")
       package_description("Fast math via native C code")
   }

   [export]
   def build() {
       cmake_build()
   }

What happens on install:

1. Clone repo
2. ``cmake -B _build -S . -DDASLANG_DIR=<das_root>``
3. ``cmake --build _build --config Release``
4. The built ``.shared_module`` is available in ``modules/fastmath/_build/``


Package with dependencies
-------------------------

.. code-block:: das

   options gen2
   require daslib/daspkg

   [export]
   def package() {
       package_name("game-utils")
       package_description("Game utility collection")
   }

   [export]
   def dependencies(version : string) {
       require_package("github.com/user/math-lib")
       require_package("github.com/user/ecs-helpers", ">=2.0")
   }


SDK-aware version resolution
-----------------------------

A package that ships different versions for different SDK releases:

.. code-block:: das

   options gen2
   require daslib/daspkg

   [export]
   def resolve(sdk_version, version : string) {
       if (version == "" || version == "latest") {
           download_tag("v2.0")
       } elif (version == "1.x") {
           download_tag("v1.5")
       } else {
           download_tag("v{version}")
       }
   }


Repository redirect
-------------------

A package that has moved to a new repository:

.. code-block:: das

   options gen2
   require daslib/daspkg

   [export]
   def resolve(sdk_version, version : string) {
       download_redirect("github.com/neworg/new-repo", "v3.0")
   }

daspkg clones the old repo, sees the redirect, discards the clone,
and re-clones from the new URL.


Branch tracking
---------------

Track a branch instead of tagged releases:

.. code-block:: das

   options gen2
   require daslib/daspkg

   [export]
   def resolve(sdk_version, version : string) {
       download_branch("main")
   }


Project with dependencies
-------------------------

A project can list its own dependencies in a root ``.das_package``:

.. code-block:: das

   options gen2
   require daslib/daspkg

   [export]
   def package() {
       package_name("my-game")
       package_description("My game project")
   }

   [export]
   def dependencies(version : string) {
       require_package("github.com/user/ecs-lib")
       require_package("github.com/user/render-lib")
   }

.. code-block:: bash

   cd my-game/
   daspkg install        # installs all dependencies
   daspkg update         # updates all
   daspkg check          # verifies everything is present


Version model
=============

Three version axes:

- **Package version** -- semver of the package itself.
- **daslang version** -- which SDK release the package is compatible
  with.  The ``resolve()`` function receives ``sdk_version`` and can
  return different tags for different SDK versions.
- **Dependencies** -- other packages with their own version
  constraints.

C++ packages are sensitive to the exact daslang version (ABI
compatibility).  Pure-daslang packages are more tolerant (only
language-level compatibility).

Diamond dependencies
--------------------

Only one version of a package can be installed -- daslang has a single
``require`` namespace.

**Current rule:** first installed wins.  If ``C@1.0`` is already
installed and package B wants ``C>=2.0``, daspkg skips it.  You can
force-reinstall a specific version with ``--force``.


Requirements
============

- **git** -- required for all remote operations.
- **cmake** -- required for building C/C++ packages.
- **gh** (GitHub CLI) -- optional, only needed for
  ``introduce``/``withdraw``.

Run ``daspkg doctor`` to check your environment.


Architecture
============

daspkg is implemented entirely in daslang:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - File
     - Description
   * - ``utils/daspkg/main.das``
     - CLI entry point -- parses args, dispatches to commands.
   * - ``utils/daspkg/commands.das``
     - Command implementations: install, remove, update, build, check,
       doctor.
   * - ``utils/daspkg/index.das``
     - Package index: fetch, search, introduce, withdraw.
   * - ``utils/daspkg/lockfile.das``
     - ``daspkg.lock`` read/write.
   * - ``utils/daspkg/package_runner.das``
     - In-process ``.das_package`` compiler -- compiles, simulates,
       and extracts metadata via ``invoke_in_context``.
   * - ``utils/daspkg/utils.das``
     - Shared utilities.
   * - ``daslib/daspkg.das``
     - API module that ``.das_package`` scripts ``require``.

The **package runner** compiles ``.das_package`` scripts in-process
using ``compile_file`` + ``simulate`` + ``invoke_in_context``.  It
calls the exported functions (``package``, ``resolve``,
``dependencies``, ``build``) and reads accumulated state from
``daslib/daspkg`` module globals via ``get_context_global_variable``.


.. seealso::

   :ref:`embedding_external_modules` -- ``.das_module`` descriptors and module resolution

   `daspkg-index <https://github.com/borisbat/daspkg-index>`_ -- the curated package index

   ``examples/daspkg/`` -- example projects using daspkg
