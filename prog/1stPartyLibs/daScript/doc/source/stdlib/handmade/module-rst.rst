The RST module implements the documentation generation pipeline for daslang.
It uses RTTI to introspect modules, types, and functions, then produces
reStructuredText output suitable for Sphinx documentation builds.

All functions and symbols are in "rst" module, use require to get access to it.

.. code-block:: das

    require daslib/rst
