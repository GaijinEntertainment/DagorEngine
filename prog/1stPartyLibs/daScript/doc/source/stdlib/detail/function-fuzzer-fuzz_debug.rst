run block however many times
do not ignore panic, so that we can see where the runtime fails
this is here so that `fuzz` can be easily replaced with `fuzz_debug` for the purpose of debugging
