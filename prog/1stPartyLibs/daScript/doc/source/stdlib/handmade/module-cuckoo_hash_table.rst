The CUCKOO_HASH_TABLE module implements a cuckoo hash table data structure.
Cuckoo hashing provides worst-case O(1) lookup time by using multiple hash
functions and displacing existing entries on collision.

All functions and symbols are in "cuckoo_hash_table" module, use require to get access to it.

.. code-block:: das

    require daslib/cuckoo_hash_table
