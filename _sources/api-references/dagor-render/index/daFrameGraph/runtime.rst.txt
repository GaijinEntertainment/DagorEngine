Dispatching Work in Nodes at Runtime
=================================================

The general aim of the daFrameGraph API is to do as much boiler-plate stuff for you as possible.
It can automatically manage resources, bind them to shader vars, etc.
However, when porting legacy code to daFrameGraph, it is often necessary to do some of the work yourself by getting direct access to FG-managed textures and buffers.
When working with CPU blobs, getting the data from inside them is often the primary concern.
To facilitate this, it is possible to extract a handle to a resource inside the declaration callback using the :func:`daframegraph::VirtualResourceRequest::handle` method.
This handle should then be captured inside the execution callback and dereferenced to get access to the ManagedResView encapsulating a physical GPU resource and it's D3DRESID (acquired by registering it within the engine resource manager).

.. code-block:: cpp

   auto handle = daframegraph::register_node("node_name", DAFG_PP_NODE_SRC,
     [](daframegraph::Registry registry)
     {
       auto texHndl = registry.read("my_tex")
         .texture()
         .atStage(daframegraph::Stage::PS)
         .useAs(daframegraph::Usage::SHADER_RESOURCE)
         .handle(); // Is only callable after the stage/usage were specified
       return [texHndl]()
         {
           legacy_code(texHndl.view());
         };
     });

Below is the full documentation of the VirtualResourceHandle class.

.. doxygenclass:: dafg::VirtualResourceHandle
  :project: daFrameGraph
  :members:
