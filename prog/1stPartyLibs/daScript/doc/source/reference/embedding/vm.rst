.. _embedding_vm:

========================
daScript Virtual Machine
========================

-----------------------
Tree-based Interpretion
-----------------------

The Virtual Machine of daScript consists of a small execution context, which manages stack and heap allocation.
The compiled program itself is a Tree of "Nodes" (called SimNode) which can evaluate virtual functions. These Nodes don't check argument types, assuming that all such checks were done by compiler.
Why tree-based? Tree-based interpreters are slow!

Yes, Tree-based interpreters are infamous for their low performance compared to byte-code interpreters.

However, this opinion is typically based on a comparison between AST (abstract syntax tree) interpreters of dynamically typed languages with optimized register- or stack-based optimized interpreters. 
Due to their simplicity of implementation, AST-based interpreters are also seen in a lot of "home-brewed" naive interpreters, giving tree-based interpreters additional bad fame. 
The AST usually contains a lot of data that is useless or unnecessary for interpretation, and big tree depth and complexity. 

It is also hard to even make an AST interpreter of a statically typed language which will somehow benefit from statically typed data - basically each tree node visitor will still return both the value and type information in generic form.

In comparison, a good byte-code VM interpreter of a typical dynamically typed language will feature a tight core loop of all or the most frequent instructions (probably with computed goto) and additionally can statically (or during execution) infer types and optimize code for it.

**Register**- and **stack-based**- VMs each have their own trade-offs, notably with generally fewer generated instructions/fused instructions, fewer memory moves/indirect memory access for register-based VMs, and smaller instruction sets and easier implementation for stack-based VMs.

The more "core types" the VM has, the more instructions will probably be needed in the instruction set and/or the instruction cost increases.
Although dynamically typed languages usually don't have many core types, and some can embed all their main type's values and type information in just 64bits (using NAN-tagging, for example), that still usually leaves one of these core types (table/class/object) to be implemented with associative containers lookups (unordered_map/hashmap).
That is not optimal for cache locality/performance, and also makes interop with host (C++) types slow and inefficient. 

Interop with host/C++ functions because of that is usually slow, as it requires complex and slow arguments/return value type conversion, and/or associative table(s) lookup.

So, typically, host functions calls are very "heavy", and programmers usually can't optimize scripts by extracting just some of functionality into C++ function - they have to re-write big chunks/loops.

Increasing the amount of core internal types can help (for example, making "float3", a typical type in game development, one of the "core" types), but this makes instruction set bigger/slower, increases the complexity of type conversion, and usually introduces mandatory indirection (due to limited bitsize of value type), which is also not good for cache locality. 

However, **daScript** *does not* interpret the AST, nor is it a dynamically typed language. 

Instead, for run-time program execution it emits a different tree (Simulation Tree), which doesn't require type information to be provided for arguments/return types, since it is statically typed, and all the types are known.

For the daScript ABI, 128bit words are used, which are natural to most of modern hardware. 

The chosen representation helps branch prediction, increases cache locality, and provides a mix of stack and register based code - each 'Node' utilizes native machine registers. 

It is also important to note that the amount of "types" and "instructions" doesn't matter much - what matters is the amount of different instructions used in a particular program/function.

Type conversion between the VM and C++ ABIs is straightforward (and for most of types is as simple as a move instruction), so it is very fast and cache-friendly. 

It also makes it possible for the programmer to optimize particular functionality (in interpretation) by extracting it to a C++/host function - basically "fusing" instructions into one. 

Adding new user-types is rather simple and not painful performance- or engineering-wise. 

"Value" types have to fit into 128bits and have to be relocatable and zero-initialized (i.e. should be trivially destructible, and allow memcpy and memsetting with zeroes); all other types are "RefTypes" or "Boxed Types", which means they can be operated on in the script only as references/pointers.

There are no limits on the amount of user types, neither is there a performance impact caused by using such types (besides the obvious cost of indirection for Boxed/Ref Types).

Using generalized nodes additionally allows for a seamless mix of interpretation and Ahead of Time compiled code in the run-time - i.e. if some of the functions in the script were changed, the unchanged portion would still be running the optimized AoT code.

These are the main reasons why tree-based interpretation (not to be confused with AST-based) was chosen for the daScript interpreter, and why its interpreter is faster than most, if not all, byte code based script interpreters.



-----------------
Execution Context
-----------------

The daScript Execution Context is light-weight. It basically consists of stack allocation and two heap allocators (for strings and everything else).
One Context can be used to execute different programs; however, if the program has any global state in the heap, all calls to the program have to be done within the same Context.

It is possible to call stop-the-world garbage collection on a Context (this call is better to be done outside the program execution; it's unsafe otherwise).

However, the cost of resetting context (i.e. deallocate all memory) is extremely low, and (depending on memory usage) can be as low as several instructions,
which allows the simplest and fastest form of memory management for all of the stateless scripts - just reset the context each frame or each call.
This basically turns Context heap management into form of 'bump/stack allocator', significantly simplifying and optimizing memory management.

There are certain ways (including code of policies) to ensure that none of the scripts are using global variables at all, or at least global variables which require heap memory.

For example, one can split all contexts into several cateories: one context for all stateless script programs, and one context for each GC'ed (or, additionally, ``unsafe``) script.
The stateless context is then reset as often as needed (for example, each 'top' call from C++ or each frame/tick), and on GC-ed contexts one can call garbage collection as soon as it is needed (using some heurestics of memory usage/performance).

Each context can be used only in one thread simultaneously, i.e. for multi-threading you will need one Context for each simultaneously running thread.

To exchange data/communicate between different contexts, use 'channels' or some other custom-brewed C++ hosted code of that kind.
