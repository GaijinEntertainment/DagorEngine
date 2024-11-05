Heaps
=================================================

Heaps API provides heap object allocation and resource placement in this heap objects.
Resources may be aliased if memory ranges of multiple resources are overlapping,
in this case alive/to-be-used resources must be marked active via :cpp:func:`activate_*` methods
and dead/non-used resources must be marked inactive via :cpp:func:`deactivate_*` methods

Use :cpp:class:`ResourceDescription` to query required heap group, size and aligment with :cpp:func:`get_resource_allocation_properties`
Having this data, allocate fitting heap with :cpp:func:`create_resource_heap` and place resource there at will
with :cpp:func:`place_*_in_resource_heap`

For :cpp:class:`ResourceHeapGroup` description see :cpp:class:`ResourceHeapGroupProperties`

Callable d3d:: methods
=================================================

.. doxygengroup:: HeapD3D
  :project: d3dAPI
  :members:
  :undoc-members:
  :content-only:

.. autodoxygenfile:: dag_heap.h
  :project: d3dAPI