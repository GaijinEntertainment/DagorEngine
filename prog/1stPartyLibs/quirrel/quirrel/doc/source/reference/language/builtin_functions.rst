.. _builtin_functions:


==================
Built-in Functions
==================

.. index::
    single: Built-in Functions
    pair: Global Symbols; Built-in Functions


^^^^^^^^^^^^^^
Global Symbols
^^^^^^^^^^^^^^

.. sq:function:: array(size,[fill])

creates and returns array of a specified size. If the optional parameter fill is specified its value will be used to fill the new array's slots. If the fill parameter is omitted, null is used instead.

.. sq:function:: seterrorhandler(func)


sets the runtime error handler

.. sq:function:: callee()


returns the currently running closure

.. sq:function:: setdebughook(hook_func)

sets the debug hook

hook_func should have follow signature
  hook_func(hook_type:integer, source_file:string, line_num:integer, func_name:string)

hook_type can be 'l' - line, 'r' - return, 'c' - call or 'x' for VM shutdown

call of debughook for each line performed only when debuginfo is enabled

.. sq:function:: getroottable()

returns the root table of the VM.

.. sq:function:: getconsttable()

returns the const table of the VM.

.. sq:function:: assert(exp, [message])

throws an exception if exp is null or false. Throws "assertion failed" string by default, or message if specified.
If message argument is function it is evaluated and return value is used as message text. This is to avoid
unnecessary string formatting when it is not needed.

.. sq:function:: print(x)

prints x calling host app printing function set by (:ref:`sq_setprintfunc <sq_setprintfunc>`) call

.. sq:function:: println(x)

prints x adding line feed ('\n') to the resulting string

.. sq:function:: error(x)

prints x calling host app error printing function set by (:ref:`sq_setprintfunc <sq_setprintfunc>`) call

.. sq:function:: errorln(x)

same as error(x) but adds line feed ('\n') to the resulting string

.. sq:function:: compilestring(string,[buffername],[bindings])

compiles a string containing a quirrel script into a function and returns it::

    let compiledscript=compilestring("println(\"ciao\")")
    //run the script
    compiledscript()

or providing compile-time bindings::

    let api = {function foo() {println("foo() called")}}
    let compiledscript=compilestring("foo()", "bindings_test", api)
    compiledscript()

.. sq:function:: collectgarbage()

    Runs the garbage collector and returns the number of reference cycles found (and deleted). This function only works on garbage collector builds.

.. sq:function:: resurrectunreachable()

Runs the garbage collector and returns an array containing all unreachable object found. If no unreachable object is found, null is returned instead. This function is meant to help debugging reference cycles. This function only works on garbage collector builds.

.. sq:function:: type(obj)

return the 'raw' type of an object without invoking the metamethod '_typeof'.

.. sq:function:: getstackinfos(level)

returns the stack informations of a given call stack level. returns a table formatted as follow: ::

    {
        func="DoStuff", //function name

        src="test.nut", //source file

        line=10,        //line number

        locals = {      //a table containing the local variables

            a=10,

            testy="I'm a string"
        }
    }

level = 0 is getstackinfos() itself! level = 1 is the current function, level = 2 is the caller of the current function, and so on. If the stack level doesn't exist the function returns null.

.. sq:function:: newthread(threadfunc)

creates a new cooperative thread object(coroutine) and returns it

.. sq:function:: min(x, y, [z], [w], ...)

returns minimal value of all arguments

.. sq:function:: max(x, y, [z], [w], ...)

returns maximal value of all arguments

.. sq:function:: clamp(x, min_val, max_val)

returns value limited by provided min-max range

creates a new cooperative thread object(coroutine) and returns it

.. sq:function:: freeze(x)

returns immutable reference to given object.
Throws an error if argument is of POD type (to help prevent errors).

.. sq:function:: getobjflags(x)

Given object handle, return its flags that may be:

  * 0 - no special flags
  * SQOBJ_FLAG_IMMUTABLE - bit set if the object handle is immutable

.. sq:function:: getbuildinfo(x)

returns table containing information on VM build parameters.

  * **version** - string values describing the version of VM and compiler.
  * **charsize** - size in bytes of the internal VM representation for characters(1 for ASCII builds 2 for UNICODE builds).
  * **intsize** - size in bytes of the internal VM representation for integers(4 for 32bits builds 8 for 64bits builds).
  * **floatsize** - size in bytes of the internal VM representation for floats(4 for single precision builds 8 for double precision builds).

.. _default_delegates:

-----------------
Default delegates
-----------------

Except null and userdata every quirrel object has a default delegate containing a set of functions to manipulate and retrieve information from the object itself.

^^^^^^^^
Integer
^^^^^^^^

.. sq:function:: integer.tofloat()

convert the number to float and returns it


.. sq:function:: integer.tostring()

converts the number to string and returns it


.. sq:function:: integer.tointeger()

dummy function; returns the value of the integer.


.. sq:function:: integer.tochar()

returns a string containing a single character represented by the integer.


.. sq:function:: integer.weakref()

dummy function; returns the integer itself.

^^^^^
Float
^^^^^

.. sq:function:: float.tofloat()

returns the value of the float(dummy function)


.. sq:function:: float.tointeger()

converts the number to integer and returns it


.. sq:function:: float.tostring()

converts the number to string and returns it


.. sq:function:: float.tochar()

returns a string containing a single character represented by the integer part of the float.


.. sq:function:: float.weakref()

dummy function; returns the float itself.

^^^^
Bool
^^^^

.. sq:function:: bool.tofloat()

returns 1.0 for true 0.0 for false


.. sq:function:: bool.tointeger()

returns 1 for true 0 for false


.. sq:function:: bool.tostring()

returns "true" for true and "false" for false


.. sq:function:: bool.weakref()

dummy function; returns the bool itself.

^^^^^^
String
^^^^^^

.. sq:function:: string.len()

returns the string length


.. sq:function:: string.tointeger([base])

Converts the string to integer and returns it. An optional parameter base can be specified--if a base is not specified, it defaults to base 10.


.. sq:function:: string.tofloat()

converts the string to float and returns it


.. sq:function:: string.tostring()

returns the string (really, a dummy function)


.. sq:function:: string.slice(start,[end])

returns a section of the string as new string. Copies from start to the end (not included). If start is negative the index is calculated as length + start, if end is negative the index is calculated as length + end. If end is omitted end is equal to the string length.


.. sq:function:: string.indexof(substr,[startidx])

Searches for a sub string (substr) starting from the index startidx and returns the position of its first occurrence. If startidx is omitted the search operation starts from the beginning of the string. The function returns null if substr is not found.

.. sq:function:: string.contains(substr,[startidx])

Checks if the string contains a sub string (substr) anywhere starting from the index startidx. Returns boolean value.


.. sq:function:: string.tolower()

returns a lowercase copy of the string.


.. sq:function:: string.toupper()

returns a uppercase copy of the string.


.. sq:function:: string.weakref()

returns a weak reference to the object.

.. sq:function:: string.subst(...)

This delegate is used to format strings. A format string can contain variable positional arguments and table keys.
As parameters, you can pass an arbitrary number of tables and arbitrary number of positional arguments. If the key is found in several tables,
then the most value from the leftmost table will be used.

Example: ::

"Score: {0}".subst(4200) => "Score: 4200"
"x={0} y={1} z={2}".subst(42, 45.53, -10.8) => "x=42 y=45.53 z=-10.8"
"Score: {score}".subst({score=4200}) => "Score: 4200"
"x={x} y={y} z={z}".subst({y=45.53, x=42, z=-10.8}) => "x=42 y=45.53 z=-10.8"
"Type: {type}, Health: {hp}".subst({hp=100, damage=5}, {isAir=true, type="helicopter"}) => "Type: helicopter, Health: 100"
"Type: {type}, Pos: x={0} y={1} z={2}".subst({isAir=true, type="helicopter"}, 42, 45.53, -10.8) => "Type: helicopter, Pos: x=42 y=45.53 z=-10.8"
"Score: {0}".subst() => "Score: {0}"
"Score: {score}".subst({}) => "Score: {score}"

.. sq:function:: string.replace(from, to)

Replaces all occurrences of 'from' substring to 'to'

.. sq:function:: string.join(arr, [filter])

Concatenate all items in provided array using string itself as separator.
Example: ::
", ".join(["a", "b", "c"]) // => "a, b, c"

Optional filter parameter can be specified.
When it is set to true (boolean), default filter is used which keeps items which are non-null and not "" (empty string).
When filter is a function, it is called for every item and must return true for elements that should be included in resulting string.
Example: ::
", ".join(["a", null, "b", "", "", "c"], true) // => "a, b, c"
", ".join(["a", null, "b", "", "", "c"], @(v) v!=null)) // => "a, b, , , c"

.. sq:function:: string.concat(...)

Concatenate all arguments using string itself as separator.
Example: ::
", ".concat("a", "b", "c") // => "a, b, c"

.. sq:function:: string.split([sep])

Return a list of the words in the string, using sep as the delimiter string.
If sep is given, consecutive delimiters are not grouped together and are deemed to delimit empty strings
(for example, '1,,2'.split(',') returns ['1', '', '2']).
The sep argument may consist of multiple characters (for example, '1<>2<>3'.split('<>') returns ['1', '2', '3']).
Splitting an empty string with a specified separator returns [''].

If sep is not specified or is None, a different splitting algorithm is applied:
runs of consecutive whitespace are regarded as a single separator, and the result will contain no empty strings
at the start or end if the string has leading or trailing whitespace.
Consequently, splitting an empty string or a string consisting of just whitespace without providing a separator returns [].

.. sq:function:: string.split_by_chars(separators [, skipempty])

    returns an array of strings split at each point where a separator character occurs in `str`.
    The separator is not returned as part of any array element.
    The parameter `separators` is a string that specifies the characters as to be used for the splitting.
    The parameter `skipempty` is a boolean (default false). If `skipempty` is true, empty strings are not added to array.

    ::

        eg.
        let a = "1.2-3;;4/5".split_by_chars(".-/;")
        // the result will be  [1,2,3,,4,5]
        or
        let b = "1.2-3;;4/5".split_by_chars(,".-/;",true)
        // the result will be  [1,2,3,4,5]

.. sq:function:: string.hash()

Returns integer hash value of a string. It is always non-negative (so it doesn't always match Quirrel string internal hash value).

.. sq:function:: string.lstrip()

    Strips white-space-only characters that might appear at the beginning of the given string
    and returns the new stripped string.

.. sq:function:: string.rstrip()

    Strips white-space-only characters that might appear at the end of the given string
    and returns the new stripped string.

.. sq:function:: string.strip()

    Strips white-space-only characters that might appear at the beginning or end of the given string and returns the new stripped string.

.. sq:function:: string.startswith(cmp)

    returns `true` if the beginning of the string `str` matches the string `cmp`; otherwise returns `false`

^^^^^
Table
^^^^^

.. sq:function:: table.len()

returns the number of slots contained in a table


.. sq:function:: table.rawget(key)

tries to get a value from the slot 'key' without employing delegation


.. sq:function:: table.rawset(key,val)

Sets the slot 'key' with the value 'val' without employing delegation. If the slot does not exists, it will be created. Returns table itself.


.. sq:function:: table.rawdelete(key)

Deletes the slot key without employing delegation and returns its value. If the slot does not exists, returns null.


.. sq:function:: table.rawin(key)

returns true if the slot 'key' exists. the function has the same effect as the operator 'in' but does not employ delegation.


.. sq:function:: table.weakref()

returns a weak reference to the object.


.. sq:function:: table.tostring()

Tries to invoke the _tostring metamethod. If that fails, it returns "(table : pointer)".


.. sq:function:: table.clear()

removes all the slots from the table. Returns table itself.

.. sq:function:: table.filter(func(val, [key], [table_ref]))

Creates a new table with all values that pass the test implemented by the provided function. In detail, it creates a new table, invokes the specified function for each key-value pair in the original table; if the function returns 'true', then the value is added to the newly created table at the same key.

.. sq:function:: table.keys()

returns an array containing all the keys of the table slots.

.. sq:function:: table.values()

returns an array containing all the values of the table slots.

.. sq:function:: table.topairs()

returns an array containing arrays of pairs [key, value]. Useful when you need to sort data from table.

.. sq:function:: table.map(func(slot_value, [slot_key], [table_ref]))

Creates a new table of the same size. For each element in the original table invokes the function 'func' and assigns the return value of the function to the corresponding slot of the newly created table.
Provided func can accept up to 3 arguments: slot value (required), slot key in table (optional), reference to table itself (optional).
If callback func throws null, the element is skipped and not added to destination table.

.. sq:function:: table.each(func(slot_value, [slot_key], [table_ref]))

Iterates a table and calls provided function for each element.

.. sq:function:: table.findindex(func(slot_value, [slot_key], [table_ref]))

Performs a linear search calling provided function for each value in the table.
Returns the index of the value if it was found (callback returned true (non-false) value) or null otherwise.

.. sq:function:: table.findvalue(func(slot_value, [slot_key], [table_ref]), [def=null])

Performs a linear search calling provided function for each value in the table.
Returns matched value (for which callback returned non-false value) or default value otherwise (null if not provided).

.. sq:function:: table.reduce(func(accumulator, slot_value, [slot_key], [table_ref]), [initializer])

Reduces a table to a single value (similar to array.reduce()).
For each table slot invokes the function 'func' passing the initial value
(or value from the previous callback call) and the value of the current element.
Callback function can also take optional parameters: key in table for current value and reference to table itself.
Iteration order is not determined.

.. sq:function:: table.__merge(table_1, [table_2], [table_3], ...)

This delegate is used to create new table from old and given.
Arguments to merge fields from can be tables, classes and instances.

.. sq:function:: table.getfuncinfos()

If table has a delegate with _call() metamethod, get info about it (see function.getfuncinfos() for details).


Example: ::

    let foo = {fizz=1}
    let bar = foo.__merge({buzz=2})
    => foo == {fizz=1}; bar={fizz=1, buzz=2}


.. sq:function:: table.__update(table_1, [table_2], [table_3], ...)

This delegate is used to update new table with values from given ones.
In other words it mutates table with data from provided tables.

Example: ::

    let foo = {fizz=1}
    let bar = foo.__update({buzz=2})
    => foo == {fizz=1, bazz=2}; bar={fizz=1, buzz=2}


^^^^^^
Array
^^^^^^

.. sq:function:: array.len()

returns the length of the array


.. sq:function:: array.append(val, [val_2], [val_3], ...)

sequentially appends the values of arguments 'val' to the end of the array. Returns array itself.


.. sq:function:: array.extend(array_1, [array_2], [array_3], ...)

Extends the array by appending all the items in all the arrays passed as arguments. Returns target array itself.


.. sq:function:: array.pop()

removes a value from the back of the array and returns it.


.. sq:function:: array.top()

returns the value of the array with the higher index


.. sq:function:: array.insert(idx,val)

inserts the value 'val' at the position 'idx' in the array. Returns array itself.


.. sq:function:: array.remove(idx)

removes the value at the position 'idx' in the array and returns its value.


.. sq:function:: array.resize(size,[fill])

Resizes the array. If the optional parameter 'fill' is specified, its value will be used to fill the new array's slots when the size specified is bigger than the previous size. If the fill parameter is omitted, null is used instead. Returns array itself.


.. sq:function:: array.sort([compare_func])

Sorts the array in-place. A custom compare function can be optionally passed. The function prototype as to be the following.::

    function custom_compare(a,b)
    {
        if(a>b) return 1
        else if(a<b) return -1
        return 0;
    }

a more compact version of a custom compare can be written using a lambda expression and the operator <=> ::

    arr.sort(@(a,b) a <=> b);

Returns array itself.

.. sq:function:: array.reverse()

reverse the elements of the array in place. Returns array itself.


.. sq:function:: array.slice(start,[end])

Returns a section of the array as new array. Copies from start to the end (not included). If start is negative the index is calculated as length + start, if end is negative the index is calculated as length + end. If end is omitted end is equal to the array length.


.. sq:function:: array.weakref()

returns a weak reference to the object.


.. sq:function:: array.tostring()

returns the string "(array : pointer)".


.. sq:function:: array.totable()

Creates a table from arrays containing arrays of pairs [key,value]. Reverse of table.topairs().


.. sq:function:: array.clear()

removes all the items from the array


.. sq:function:: array.map(func(item_value, [item_index], [array_ref]))

Creates a new array of the same size. For each element in the original array invokes the function 'func' and assigns the return value of the function to the corresponding element of the newly created array.
Provided func can accept up to 3 arguments: array item value (required), array item index (optional), reference to array itself (optional).
If callback func throws null, the element is skipped and not added to destination array.


.. sq:function:: array.apply(func([item_value, [item_index], [array_ref]))

for each element in the array invokes the function 'func' and replace the original value of the element with the return value of the function.

.. sq:function:: array.each(func(item_value, [item_index], [array_ref]))

Iterates an array and calls provided function for each element.

.. sq:function:: array.reduce(func(prevval,curval,[index],[array_ref]), [initializer])

Reduces an array to a single value. For each element in the array invokes the function 'func' passing
the initial value (or value from the previous callback call) and the value of the current element.
Callback can optionally accept index of current value and reference to array itself.
The return value of the function is then used as 'prevval' for the next element.
If the optional initializer is present, it is placed before the items of the array in the calculation,
and serves as a default when the sequence is empty.
If initializer is not given then for sequence contains only one item, reduce() returns the first item,
and for empty sequence returns null.

Given an sequence with 2 or more elements (including initializer) calls the function with the first two elements as the parameters,
gets that result, then calls the function with that result and the third element, gets that result,
calls the function with that result and the fourth parameter and so on until all element have been processed.
Finally, returns the return value of the last invocation of func.


.. sq:function:: array.filter(func(val, [index], [array_ref]))

Creates a new array with all elements that pass the test implemented by the provided function. In detail, it creates a new array, for each element in the original array invokes the specified function passing the index of the element and it's value; if the function returns 'true', then the value of the corresponding element is added on the newly created array.

.. sq:function:: array.indexof(value)

Performs a linear search for the value in the array. Returns the index of the value if it was found null otherwise.

.. sq:function:: array.contains(value)

Performs a linear search for the value in the array. Returns true if it was found and false otherwise.

.. sq:function:: array.findindex(func(item_value, [item_index], [array_ref]))

Performs a linear search calling provided function for each value in the array.
Returns the index of the value if it was found (callback returned true (non-false) value) or null otherwise.

.. sq:function:: array.findvalue(func(item_value, [item_index], [array_ref]), [def=null])

Performs a linear search calling provided function for each value in the array.
Returns matched value (for which callback returned non-false value) or default value otherwise (null if not provided).

.. sq:function:: array.replace(source_arr)

Copies content of source array into given array by replacing its contents. Returns target array itself.

^^^^^^^^
Function
^^^^^^^^

.. sq:function:: function.call(_this,args...)

calls the function with the specified environment object('this') and parameters


.. sq:function:: function.pcall(_this,args...)

calls the function with the specified environment object('this') and parameters, this function will not invoke the error callback in case of failure(pcall stays for 'protected call')


.. sq:function:: function.acall(array_args)

calls the function with the specified environment object('this') and parameters. The function accepts an array containing the parameters that will be passed to the called function.Where array_args has to contain the required 'this' object at the [0] position.


.. sq:function:: function.pacall(array_args)

calls the function with the specified environment object('this') and parameters. The function accepts an array containing the parameters that will be passed to the called function.Where array_args has to contain the required 'this' object at the [0] position. This function will not invoke the error callback in case of failure(pacall stays for 'protected array call')


.. sq:function:: function.weakref()

returns a weak reference to the object.


.. sq:function:: function.tostring()

returns the string "(closure : pointer)".


.. sq:function:: function.bindenv(env)

clones the function(aka closure) and bind the environment object to it(table,class or instance). the this parameter of the newly create function will always be set to env. Note that the created function holds a weak reference to its environment object so cannot be used to control its lifetime.


.. sq:function:: function.getfuncinfos()

returns a table containing informations about the function, like parameters, name and source name; ::

    //the data is returned as a table is in form
    //pure quirrel function
    {
      native = false
      name = "zefuncname"
      src = "/somthing/something.nut"
      parameters = ["a","b","c"]
      defparams = [1,"def"]
      varargs = 2
      freevars = 0
    }
    //native C function
    {
      native = true
      name = "zefuncname"
      paramscheck = 2
      typecheck = [83886082,83886384] //this is the typemask (see C defines OT_INTEGER,OT_FLOAT etc...)
      freevars = 2
    }

.. sq:function:: function.getfreevar(idx)

returns a table containing information about given free variable ::
  { name="foo", value=5 }


^^^^^
Class
^^^^^

.. sq:function:: class.instance()

returns a new instance of the class. this function does not invoke the instance constructor. The constructor must be explicitly called (eg. class_inst.constructor(class_inst) ).


.. sq:function:: class.rawin(key)

returns true if the slot 'key' exists. the function has the same effect as the operator 'in' but does not employ delegation.


.. sq:function:: class.weakref()

returns a weak reference to the object.


.. sq:function:: class.tostring()

returns the string "(class : pointer)".


.. sq:function:: class.rawget(key)

tries to get a value from the slot 'key' without employing delegation


.. sq:function:: class.rawset(key,val)

sets the slot 'key' with the value 'val' without employing delegation. If the slot does not exists, it will be created.


.. sq:function:: class.newmember(key,val,[bstatic])

sets/adds the slot 'key' with the value 'val'. If bstatic is true the slot will be added as static. If the slot does not exists , it will be created.


.. sq:function:: class.getfuncinfos()

If class has _call() metamethod, get info about it (see function.getfuncinfos() for details).

.. sq:function:: class.getmetamethod(name)

Returns metamethod closure (e.g. Foo.getmetamethod("_add")) or null if method is not implemented in class.

.. sq:function:: class.__merge(table_or_class_1, [table_or_class_2], [table_or_class_3], ...)

This delegate is used to create new class from old and given.
Arguments to merge fields from can be tables, classes and instances.

.. sq:function:: class.__update(table_1, [table_2], [table_3], ...)

This delegate is used to update new table with values from given ones.
In other words it mutates table with data from provided tables.

^^^^^^^^^^^^^^
Class Instance
^^^^^^^^^^^^^^

.. sq:function:: instance.getclass()

returns the class that created the instance.


.. sq:function:: instance.rawin(key)

    :param key: ze key

returns true if the slot 'key' exists. the function has the same effect as the operator 'in' but does not employ delegation.


.. sq:function:: instance.weakref()

returns a weak reference to the object.


.. sq:function:: instance.tostring()

tries to invoke the _tostring metamethod, if failed. returns "(instance : pointer)".


.. sq:function:: instance.rawget(key)

tries to get a value from the slot 'key' without employing delegation


.. sq:function:: instance.rawset(key,val)

sets the slot 'key' with the value 'val' without employing delegation. If the slot does not exists, it will be created.

.. sq:function:: instance.getfuncinfos()

If instance has _call() metamethod, get info about it (see function.getfuncinfos() for details).

.. sq:function:: instance.getmetamethod(name)

Returns metamethod closure (e.g. foo.getmetamethod("_add")) or null if method is not implemented in class.


^^^^^^^^^^^^^^
Generator
^^^^^^^^^^^^^^


.. sq:function:: generator.getstatus()

returns the status of the generator as string : "running", "dead" or "suspended".


.. sq:function:: generator.weakref()

returns a weak reference to the object.


.. sq:function:: generator.tostring()

returns the string "(generator : pointer)".

^^^^^^^^^^^^^^
Thread
^^^^^^^^^^^^^^

.. sq:function:: thread.call(...)

starts the thread with the specified parameters


.. sq:function:: thread.wakeup([wakeupval])

wakes up a suspended thread, accepts a optional parameter that will be used as return value for the function that suspended the thread(usually suspend())


.. sq:function:: thread.wakeupthrow(objtothrow,[propagateerror = true])

wakes up a suspended thread, throwing an exception in the awaken thread, throwing the object 'objtothrow'.


.. sq:function:: thread.getstatus()

returns the status of the thread ("idle","running","suspended")


.. sq:function:: thread.weakref()

returns a weak reference to the object.


.. sq:function:: thread.tostring()

returns the string "(thread : pointer)".


.. sq:function:: thread.getstackinfos(stacklevel)

returns the stack frame informations at the given stack level (0 is the current function 1 is the caller and so on).

^^^^^^^^^^^^^^
Weak Reference
^^^^^^^^^^^^^^

.. sq:function:: weakreference.ref()

returns the object that the weak reference is pointing at; null if the object that was point at was destroyed.


.. sq:function:: weakreference.weakref()

returns a weak reference to the object.


.. sq:function:: weakreference.tostring()

returns the string "(weakref : pointer)".

^^^^^^^^^^^^^^
Userdata
^^^^^^^^^^^^^^

.. sq:function:: userdata.getfuncinfos()

If userdata has _call() metamethod in delegate, get info about it (see function.getfuncinfos() for details).
