Declaring Node
=================================================

The basic syntax for creating a new node is as follows:

.. code-block:: cpp

   auto handle = daframegraph::register_node("node_name", DAFG_PP_NODE_SRC,
     [](daframegraph::Registry registry)
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

.. _dafg-bindless-shader-var:

.. rubric:: Bindless bindings

Besides ``bindToShaderVar()``, a resource request can be bound *bindlessly*:
the resource is registered in a bindless heap and its slot index is written into a
named ``int`` shader variable (``-1`` when the resource is missing or the driver has
no bindless support).

.. code-block:: cpp

   registry.read("your_tex").texture()
     .atStage(dafg::Stage::PS_OR_CS)
     .bindlessShaderVar("your_bindless_index");

   registry.create("your_sampler")
     .blob<d3d::SamplerHandle>(d3d::request_sampler(d3d::SamplerInfo{}))
     .bindlessShaderVar("your_sampler_index");

The shader consumes the index through the matching ``int`` variable, either via the
``@bindless*`` DSHL sugar or a manual ``BINDLESS_*_ARRAY`` declaration. See
:ref:`explicit-bindless-resources` in the DSHL reference. The full request API is
documented in :cpp:class:`dafg::VirtualResourceRequest` below.

.. doxygenclass:: dafg::Registry
  :project: daFrameGraph
  :members:

.. doxygenclass:: dafg::NameSpaceRequest
  :project: daFrameGraph
  :members:

.. doxygenclass:: dafg::AutoResolutionRequest
  :project: daFrameGraph
  :members:

.. doxygennamespace:: dafg::multiplexing
  :project: daFrameGraph
  :members:

.. doxygenclass:: dafg::StateRequest
  :project: daFrameGraph
  :members:

.. doxygenfile:: resourceCreation.h
  :project: daFrameGraph

.. doxygenclass:: dafg::VirtualResourceCreationSemiRequest
  :project: daFrameGraph
  :members:

.. doxygenclass:: dafg::VirtualResourceSemiRequest
  :project: daFrameGraph
  :members:

.. doxygenclass:: dafg::VirtualResourceRequest
  :project: daFrameGraph
  :members:

.. doxygenclass:: dafg::VirtualPassRequest
  :project: daFrameGraph
  :members:
