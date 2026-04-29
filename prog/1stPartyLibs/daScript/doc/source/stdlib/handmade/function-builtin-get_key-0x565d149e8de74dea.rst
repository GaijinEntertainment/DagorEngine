Returns the key associated with a value reference obtained during table iteration.
The value must be a reference from a ``values()`` iterator on the same table.
Computes the key via O(1) pointer arithmetic on the parallel key/value arrays.
Throws an error if the value pointer is not inside the table or points to a deleted slot.
