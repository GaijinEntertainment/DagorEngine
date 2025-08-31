.. _arrays:


=====
Array
=====

.. index::
    single: Arrays

An array is a sequence of values indexed by an integer number from 0 to the size of the
array minus 1. An array's elements can be obtained by their index::

  var a = fixed_array<int>(1, 2, 3, 4) // fixed size of array is 4, and content is [1, 2, 3, 4]
  assert(a[0] == 1)

  var b: array<int>
  push(b,1)
  assert(b[0] == 1)

Alternative syntax is::

  var a = fixed_array(1,2,3,4)

There are static arrays (of fixed size, allocated on the stack), and dynamic arrays (size is dynamic, allocated on the heap)::

  var a = fixed_array(1, 2, 3, 4) // fixed size of array is 4, and content is [1, 2, 3, 4]
  var b: array<string>            // empty dynamic array
  push(b, "some")                 // now it is 1 element of "some"
  b |> push("some")               // same as above line, but using pipe operator

Dynamic sub-arrays can be created out of any array type via range indexing::

  var a  = fixed_array(1,2,3,4)
  let b <- a[1..3]               //  b is [2,3]

In reality `a[b]`, where b is a range, is equivalent to `subarray(a, b)`.

Resizing, insertion, and deletion of dynamic arrays and array elements is done through a set of
standard functions (see :ref:`built-in functions <stdlib__builtin>`).

The relevant builtin functions are: ``push``, ``push_clone``, ``emplace``, ``reserve``, ``resize``, ``erase``, ``length``, ``clear``, ``empty`` and ``capacity``.

Arrays (as well as tables, structures, and handled types) are passed to functions by reference only.

Arrays cannot be copied; only cloned or moved. ::

  def clone_array(var a, b: array<string>)
    a := b      // a is not a deep copy of b
    clone(a, b) // same as above

  def move_array(var a, b: array<string>)
    a <- b  // a is no points to same data as b, and b is empty.

Arrays can be constructed inline::

	let arr = fixed_array(1.,2.,3.,4.5)

This expands to::

	let arr : float[4] = fixed_array<float>(1.,2.,3.,4.5)

Dynamic arrays can also be constructed inline::

	let arr <- ["one"; "two"; "three"]

This is syntactic equivalent to::

	let arr : array<string> <- array<string>("one","two","three")

Alternative syntax is::

  let arr <- array(1., 2., 3., 4.5)
  let arr <- array<float>(1., 2., 3., 4.5)

Arrays of structures can be constructed inline::

  struct Foo {
    x : int = 1
    y : int = 2
  }

  var a <- array struct<Foo>((x=11,y=22),(x=33),(y=44))    // dynamic array of Foo with 'construct' syntax (creates 3 different copies of Foo)

Arrays of variants can be constructed inline::

  variant Bar {
    i : int
    f : float
    s : string
  }

  var a <- array variant<Bar>(Bar(i=1), Bar(f=2.), Bar(s="3"))    // dynamic array of Bar (creates 3 different copies of Bar)

Arrays of tuples can be constructed inline::

  var a <- array tuple<i:int,s:string>((i=1,s="one"),(i=2,s="two"),(i=3,s="three"))    // dynamic array of tuples (creates 3 different copies of tuple)

When array elements can't be copied, use ``push_clone`` to insert a clone of a value, or ``emplace`` to move it in.

``resize`` can potentially create new array elements. Those elements are initialized with 0.

``reserve`` is there for performance reasons. Generally, array capacity doubles, if exceeded.
``reserve`` allows you to specify the exact known capacity and significantly reduce the overhead of multiple ``push`` operations.

``empty`` tells you if the dynamic array is empty.

``clear`` clears the array, but does not free the memory. It is equivalent to ``resize(0)``.

``length`` returns the number of elements in the array.

``capacity`` returns the number of elements that can be stored in the array without reallocating.

It's possible to iterate over an array via a regular ``for`` loop. ::

  for ( x in [1,2,3,4] ) {
    print("x = {x}\n")
  }

Additionally, a collection of unsafe iterators is provided::

  def each ( a : auto(TT)[] ) : iterator<TT&>
  def each ( a : array<auto(TT)> ) : iterator<TT&>

The reason both are unsafe operations is that they do not capture the array.

Search functions are available for both static and dynamic arrays::

  def find_index ( arr : array<auto(TT)> implicit; key : TT )
  def find_index ( arr : auto(TT)[] implicit; key : TT )
  def find_index_if ( arr : array<auto(TT)> implicit; blk : block<(key:TT):bool> )
  def find_index_if ( arr : auto(TT)[] implicit; blk : block<(key:TT):bool> )


