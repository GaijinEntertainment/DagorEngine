This macro reports an error if safe_addr is attempted on the object, which is not local to the scope.
I.e. if the object can `expire` while in scope, with delete, garbage collection, or on the C++ side.
