Heaps
=================================================

Heaps API provides heap object allocation and resource placement in this heap
objects. Resources may be aliased if memory ranges of multiple resources are
overlapping, in this case alive/to-be-used resources must be marked active via
``activate_*`` methods and dead/non-used resources must be marked inactive via
``deactivate_*`` methods.

Use :cpp:class:`ResourceDescription` to query required heap group, size and
alignment with :cpp:func:`get_resource_allocation_properties` Having this data,
allocate fitting heap with :cpp:func:`create_resource_heap` and place resource
there at will with ``place_*_in_resource_heap``.

.. seealso:: For ``ResourceHeapGroup`` description, see :cpp:class:`ResourceHeapGroupProperties`.

Callable D3D:: Methods
----------------------

.. doxygengroup:: HeapD3D
  :project: d3dAPI
  :members:
  :undoc-members:
  :content-only:

.. doxygenfile:: dag_heap.h
  :project: d3dAPI