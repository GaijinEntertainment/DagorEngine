Core Functions
=================================================

The library exposes several global functions for controlling the global
graph state encapsulated inside the library, listed below. The graph
is always singular and global by design. This allows for the most
effective optimization of memory and other resources due to all
information being visible and available to the backend.

.. doxygenfile:: bfg.h
  :project: daBFG

.. doxygenfile:: externalState.h
  :project: daBFG
