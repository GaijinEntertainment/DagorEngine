.. _very_safe_context:


==================
Very safe context
==================

.. index::
    single: Very Safe Context

Daslang prioritizes runtime performance and development speed over absolute safety.
Outside of ``unsafe`` blocks the language enforces safety checks that prevent common programming errors,
but certain fundamentally unsafe patterns cannot be eliminated without sacrificing performance.

``options very_safe_context`` mitigates the most dangerous of these patterns.

---------------------------------
The problem: aliased references
---------------------------------

Some expressions produce multiple references into the same container, where an earlier reference can be
invalidated by a later operation.
Consider the following example:

.. code-block:: das

    options gen2
    options very_safe_context

    struct Foo {
        a : int
    }

    def move_data(var a, b : Foo) {
        b <- a
    }

    def good_index(var a : array<Foo>; index : int) : int {
        if (index >= length(a)) {
            resize(a, index + 1)
        }
        return index
    }

    [export]
    def main() {
        var data : array<Foo>
        data[good_index(data, 5)].a = 42
        move_data(data[good_index(data, 5)], data[good_index(data, 100)])
        print("data = {data}\n")
    }

Both ``data[5]`` and ``data[100]`` must share the same lifetime, but ``5`` is evaluated before the resize
and ``100`` after it. No order of operations can make this code correct with unboxed containers and
pass-by-reference semantics. Equivalent C++ code exhibits the same behavior.

The issue is even more apparent with tables:

.. code-block:: das

    tab[key1] <- tab[key2]  // may rehash the table, invalidating the key1 reference

---------------------------------
What ``very_safe_context`` does
---------------------------------

When ``options very_safe_context`` is enabled, array and table memory is not freed on resize.
Instead, the old buffer is left for the garbage collector to reclaim later.
This prevents most crashes and often produces correct results because existing references
continue to point at valid — though possibly stale — copies of the data.

Completely eliminating this class of bugs would require either boxing all containers or introducing
a borrow checker, neither of which is practical for the language's performance goals.

Some of these issues can also be caught by linting or optional runtime checks.

---------------------------------
Iteration and container moves
---------------------------------

A related hazard occurs when a container is moved while it is being iterated:

.. code-block:: das

    var a <- [1, 2, 3, 4]
    var b : array<int>
    for (i, x in count(), a) {
        if (i == 0) {
            b <- a
        }
        x++
    }

This example throws a runtime error, but only at the exit of the loop.
Detecting it earlier would require expensive per-iteration or per-move checks.
In the meantime, the loop body operates on a shadow copy of the data.
