The APPLY module provides the ``apply`` macro for iterating over struct, tuple,
and variant fields at compile time. Each field is visited with its name and
a reference to its value, enabling generic per-field operations like
serialization, printing, and validation.

All functions and symbols are in "apply" module, use require to get access to it.

.. code-block:: das

    require daslib/apply

Example:

.. code-block:: das

    require daslib/apply

        struct Foo {
            a : int
            b : float
            c : string
        }

        [export]
        def main() {
            var foo = Foo(a = 42, b = 3.14, c = "hello")
            apply(foo) $(name, field) {
                print("{name} = {field}\n")
            }
        }
        // output:
        // a = 42
        // b = 3.14
        // c = hello

.. seealso::

   :ref:`tutorial_apply` — comprehensive tutorial covering structs,
   tuples, variants, ``static_if`` dispatch, mutation, generic
   ``describe``, and the 3-argument annotation form.
