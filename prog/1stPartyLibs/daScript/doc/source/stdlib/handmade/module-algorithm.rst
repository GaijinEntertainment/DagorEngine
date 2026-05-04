The ALGORITHM module provides array and collection manipulation algorithms including
sorting, searching, set operations, element removal, and more.

Key features:

* **Search**: `lower_bound`, `upper_bound`, `binary_search`, `equal_range`
* **Array manipulation**: `unique`, `sort_unique`, `reverse`, `combine`, `fill`, `rotate`, `erase_all`, `min_element`, `max_element`, `is_sorted`, `topological_sort`
* **Set operations** (on tables): `intersection`, `union`, `difference`, `symmetric_difference`, `identical`, `is_subset`

Most functions also support fixed-size arrays via `[expect_any_array]` overloads.

See :ref:`tutorial_algorithm` for a hands-on tutorial.

All functions and symbols are in "algorithm" module, use require to get access to it.

.. code-block:: das

    require daslib/algorithm

Example:

.. code-block:: das

    require daslib/algorithm

        [export]
        def main() {
            var arr <- [3, 1, 4, 1, 5, 9, 2, 6, 5]
            sort_unique(arr)
            print("sort_unique: {arr}\n")
            print("has 4: {binary_search(arr, 4)}\n")
            print("has 7: {binary_search(arr, 7)}\n")
            print("lower_bound(4): {lower_bound(arr, 4)}\n")
            print("upper_bound(4): {upper_bound(arr, 4)}\n")
            let er = equal_range(arr, 5)
            print("equal_range(5): {er}\n")
            print("min index: {min_element(arr)}\n")
            print("is_sorted: {is_sorted(arr)}\n")
        }
