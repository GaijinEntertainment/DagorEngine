D3D API for Working with Matrices and Perspective Object
========================================================

The file contains deprecated methods that should not be part of the D3D API. We
are going to remove the getters and setters, as they are used for accessing
global state. Other matrix calculation methods should be moved out of the
``d3d`` namespace.

.. doxygenfile:: dag_matricesAndPerspective.h
  :project: d3dAPI
