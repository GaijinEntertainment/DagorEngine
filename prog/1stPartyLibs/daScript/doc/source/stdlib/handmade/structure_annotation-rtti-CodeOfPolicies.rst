Object which holds compilation and simulation settings and restrictions.
Whether ahead-of-time compilation is enabled.
AOT library mode.
Whether standalone context AOT compilation is enabled.
Specifies to AOT if we are compiling a module, or a final program.
Enables AOT of macro code (like 'qmacro_block' etc).
Whether paranoid validation is enabled (extra checks, no optimizations).
Whether cross-platform AOT is enabled (if not, we generate code for the current platform).
File name for AOT output (if not set, we generate a temporary file).
If we are in code completion mode.
Export all functions and global variables.
If not set, we recompile main module each time.
Keep context alive after main function.
Whether to use very safe context (delete of data is delayed, to avoid table[foo]=table[bar] lifetime bugs).
Threshold for reporting candidates for function calls. If less than this number, we always report them.
Maximum number of inference passes.
Stack size.
Whether to intern strings.
Whether to use persistent heap (or linear heap).
Whether multiple contexts are allowed (pinvokes between contexts).
Heap size hint.
String heap size hint.
Whether to use solid context (global variables are cemented at locations, can't be called from other contexts via pinvoke).
Whether macro context uses persistent heap.
Whether macro context does garbage collection.
Maximum size of static variables.
Maximum heap allocated.
Maximum string heap allocated.
Whether to enable RTTI.
Whether to allow unsafe table lookups (via [] operator).
Whether to relax pointer constness rules.
Allows use of version 2 syntax.
Whether to use gen2 make syntax.
Allows relaxing of the assignment rules.
Disables all unsafe operations.
Local references are considered unsafe.
Disallows global variables in this context (except for generated).
Disallows global variables at all in this context.
Disallows global heap in this context.
Only fast AOT, no C++ name generation.
Whether to consider side effects during AOT ordering.
Errors on unused function arguments.
Errors on unused block arguments.
Allows block variable shadowing.
Allows local variable shadowing.
Allows shared lambdas.
Ignore shared modules during compilation.
Default module mode is public.
Disallows use of deprecated features.
Disallows aliasing (if aliasing is allowed, temporary lifetimes are extended).
Enables strict smart pointer checks.
Disallows use of 'init' in structures.
Enables strict unsafe delete checks.
Disallows member functions in structures.
Disallows local class members.
Report invisible functions.
Report private functions.
Enables strict property checks.
Disables all optimizations.
Disables infer-time constant folding.
Fails compilation if AOT is not available.
Fails compilation if AOT export is not available.
Log compile time.
Log total compile time.
Disables fast call optimization.
Reuse stack memory after variables go out of scope.
Force in-scope for POD-like types.
Log in-scope for POD-like types.
Enables debugger support.
Enables debug inference flag.
Enables profiler support.
Enables threadlock context.
JIT enabled - if enabled, JIT will be used to compile code at runtime.
JIT all functions - if enabled, JIT will compile all functions in the module.
JIT debug info - if enabled, JIT will generate debug info for JIT compiled code.
JIT dll mode - if enabled, JIT will generate DLL's into JIT output folder and load them from there.
JIT exe mode - if enabled, JIT will generate standalone executable.
JIT will always emit function prologues, which allows call-stack in debuggers.
JIT output folder (where JIT compiled code will be stored).
JIT optimization level for compiled code (0-3).
JIT size optimization level for compiled code (0-3).
Path to shared library, which is used in JIT.
Path to linker, which is used in JIT.