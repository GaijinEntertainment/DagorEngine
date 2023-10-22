Declaring a Node
=================================================

The basic syntax for creating a new node is as follows:

.. code-block:: cpp

   auto handle = dabfg::register_node("node_name", DABFG_PP_NODE_SRC,
     [](dabfg::Registry registry)
     {
       // use registry's methods to declare what your node does
       return []()
         {
           // dispatch GPU commands
         };
     });

The outer lambda is referred to as the *declaration callback* and is executed rarely, in most cases even once.
The declaration callback returns another lambda, referred to as the *execution callback*, which can be executed once or several times per frame depending on the current multiplexing settings.
See :doc:`runtime` for further information on runtime behavior.
Below is a comprehensive documentation of :cpp:class:`dabfg::Registry` and the related request builder objects.
These must be used inside the declaration callback for specifying the following data:

 * What resources your node is going to operate on,
 * What it is going to do with them,
 * What bindings it requires for executing it's work (shader resources, render targets, etc)
 * What global state the node requires (e.g. shader blocks)
 * Additional metadata (multiplexing mode, priority, explicit dependencies, etc)

.. doxygenclass:: dabfg::Registry
  :project: daBFG
  :members:

.. doxygenclass:: dabfg::NameSpaceRequest
  :project: daBFG
  :members:

.. doxygenclass:: dabfg::AutoResolutionRequest
  :project: daBFG
  :members:

.. doxygennamespace:: dabfg::multiplexing
  :project: daBFG
  :members:

.. doxygenstruct:: dabfg::VrsRequirements
  :project: daBFG
  :members:

.. doxygenclass:: dabfg::StateRequest
  :project: daBFG
  :members:

.. doxygenfile:: resourceCreation.h
  :project: daBFG

.. doxygenclass:: dabfg::VirtualResourceCreationSemiRequest
  :project: daBFG
  :members:

.. doxygenclass:: dabfg::VirtualResourceSemiRequest
  :project: daBFG
  :members:

.. doxygenclass:: dabfg::VirtualResourceRequest
  :project: daBFG
  :members:

.. doxygenclass:: dabfg::VirtualPassRequest
  :project: daBFG
  :members:
