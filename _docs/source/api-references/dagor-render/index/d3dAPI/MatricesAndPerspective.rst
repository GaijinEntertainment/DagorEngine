D3D API for work with matrices and perspective object
=================================================

The file contains depricated methods that shouldn't be a part of d3d API.
We a re going to remove getters/setters because they are used for global state access.
Other matric calculation methods should be moved out of d3d namespace.

.. autodoxygenfile:: dag_matricesAndPerspective.h
  :project: d3dAPI
