.. _theembedding:

*******************************
  Embedding and Integration
*******************************

This section explains how to embed the daslang scripting language into a host
application written in C++ or C (and, by extension, any language that can call
C functions).  It is organized from simple to advanced:

* **Quick Start** — minimal host program, compilation, and evaluation
* **C++ API Reference** — modules, function/type/enum bindings, cast infrastructure
* **C API Reference** — the ``daslang/c_api/c_api.h`` header for C-only hosts and FFI
* **External Modules** — building and distributing modules outside the main
  source tree, ``.das_module`` descriptors, ``find_package(DAS)``
* **Project Files** — ``.das_project`` files for custom module resolution and sandboxing
* **Advanced Topics** — AOT compilation, standalone contexts, class adapters

For step-by-step walk-throughs with complete, compilable source code, see the
:ref:`C++ integration tutorials <tutorials_integration_cpp>` and
:ref:`C integration tutorials <tutorials_integration_c>`.

.. toctree::

   embedding/quickstart.rst
   embedding/cpp_api.rst
   embedding/c_api.rst
   embedding/external_modules.rst
   embedding/project_files.rst
   embedding/advanced.rst

