Controlling the Graph State
=================================================

When a node is created using :cpp:func:`dabfg::register_node`, a handle
object is returned that must be used to control the lifetime of this
node. As soon as the handle is destroyed, the node is removed from the
graph.

The recommended approach for managing handles is to use feature specific
daECS entities. The idea is to encapsulate all of the "settings" required
for creating a node for a feature as well as a handle for that node
inside a single entity and automatically re-create the node when the
settings change using ECS tracking capabilities. When the feature is
disabled, the entity is destroyed and the node is removed from the graph
automagically.

Note also that the handles are generational, so it is safe to simply
assign a new node handle to a component without explicitly resetting the
old node handle.

.. doxygenclass:: dabfg::NodeHandle
  :project: daBFG
  :members:
