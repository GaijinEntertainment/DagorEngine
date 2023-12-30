Core Functions
=================================================

The library exposes several global functions for controlling the global
graph state encapsulated inside the library, listed below. The graph
is always singular and global by design. This allows for the most
effective optimization of memory and other resources due to all
information being visible and available to the backend.

However, for separating different logical parts of the global graph,
e.g. for different viewports (editor and game preview) all daBfg entities
(nodes, resources, automatic resolutions, etc) are namespaced. A node
might have the name "/foo/bar/node", but you never actually use this
name to refer to a node, as we don't want to be parsing strings.
Instead, wrapper objects describing a particular namespace are used.

Hence, the code that wants to register various objects inside the graph,
like nodes and resolutions, must use the :cpp:class:`dabfg::NameSpace`
class to do so, although aliases for calling methods of this class on
the global namespace :cpp:func:`dabfg::root` are available.

.. doxygengroup:: DabfgCore
  :project: daBFG
  :members:

.. doxygengroup:: DabfgCoreAliases
  :project: daBFG
  :members:
