.. _arrays:


=====
Array
=====

.. index::
    single: Arrays

An array is a sequence of values indexed by an integer number from 0 to the size of the
array minus 1. An array's elements can be obtained by their index.
::

  var a = [[int[4] 1; 2; 3; 4]] // fixed size of array is 4, and content is [1, 2, 3, 4]
  assert(a[0] == 1)

  var b: array<int>
  push(b,1)
  assert(b[0] == 1)

There are static arrays (of fixed size, allocated on the stack), and dynamic arrays (size is dynamic, allocated on the heap)::

  var a = [[int[4] 1; 2; 3; 4]] // fixed size of array is 4, and content is [1, 2, 3, 4]
  var b: array<string>          // empty dynamic array
  push(b, "some")               // now it is 1 element of "some"
  b |> push("some")             // same as above line, but using pipe operator

Dynamic sub-arrays can be created out of any array type via range indexing::

  var a  = [[int[4] 1; 2; 3; 4]]
  let b <- a[1..3]               //  b is [{int 2;3}]

In reality `a[b]`, where b is a range, is equivalent to `subarray(a, b)`.

Resizing, insertion, and deletion of dynamic arrays and array elements is done through a set of
standard functions (see :ref:`built-in functions <stdlib__builtin>`).

The relevant builtin functions are: ``push``, ``push_clone``, ``emplace``, ``reserve``, ``resize``, ``erase``, ``length``, ``clear``, and ``capacity``.

Arrays (as well as tables, structures, and handled types) are passed to functions by reference only.

Arrays cannot be copied; only cloned or moved. ::

  def clone_array(var a, b: array<string>)
    a := b      // a is not a deep copy of b
    clone(a, b) // same as above

  def move_array(var a, b: array<string>)
    a <- b  // a is no points to same data as b, and b is empty.

Arrays can be constructed inline::

	let arr = [[auto 1.; 2.; 3.; 4.5]]

This expands to::

	let arr : float[4] = [[float[4] 1.; 2.; 3.; 4.5]]

Dynamic arrays can also be constructed inline::

	let arr <- [{auto "one"; "two"; "three"}]

This is syntactic equivalent to::

	let arr : array<string> <- to_array_move([[string[3] "one"; "two"; "three"]])

If only one element is specified, local data construction is of that element::

	let i1 = [[int 1]]	// same is the line bellow
	let i2 = 1

To create an array of an unspecified type, use [] syntax::

	let ai1 <- [[int[] 1]]	// actually [[int[1] 1]]
	let ai2 <- [[auto[] 1]	// same as above

When array elements can't be copied, use ``push_clone`` to insert a clone of a value, or ``emplace`` to move it in.

``resize`` can potentially create new array elements. Those elements are initialized with 0.

``reserve`` is there for performance reasons. Generally, array capacity doubles, if exceeded.
``reserve`` allows you to specify the exact known capacity and significantly reduce the overhead of multiple ``push`` operations.

It's possible to iterate over an array via a regular ``for`` loop::

	for x in [[int[] 1;2;3;4]]
		print("x = {x}\n")

Additionally, a collection of unsafe iterators is provided::

  def each ( a : auto(TT)[] ) : iterator<TT&>
  def each ( a : array<auto(TT)> ) : iterator<TT&>

The reason both are unsafe operations is that they do not capture the array.

Search functions are available for both static and dynamic arrays::

  def find_index ( arr : array<auto(TT)> implicit; key : TT )
  def find_index ( arr : auto(TT)[] implicit; key : TT )
  def find_index_if ( arr : array<auto(TT)> implicit; blk : block<(key:TT):bool> )
  def find_index_if ( arr : auto(TT)[] implicit; blk : block<(key:TT):bool> )


