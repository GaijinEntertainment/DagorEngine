daBFG API
=================================================

daBFG is a library for constructing and executing a *frame graph*, a computational graph for specifying a rendering pipeline of an real time application. It supports many fun features, such as:
 * Homogenous management of many types of resources: transient and temporal, CPU data, GPU textures and buffers, external resources like system backbuffers
 * Memory aliasing for all internally managed resources (even temporal ones!)
 * Automatic barriers
 * Global state management: (fake) render passes, bindings, VRS and more!
 * Multiplexing support: want to run your graph multiple times for VR? No problem! It just works (as long as you annotate node multiplexing modes correctly and don't do any global state crimes)
 * More to come soon!

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   daBFG/core
   daBFG/declaringNodes
   daBFG/graphState
   daBFG/runtime
   daBFG/das
   daBFG/bfg_ecs
   daBFG/daBfgCore
