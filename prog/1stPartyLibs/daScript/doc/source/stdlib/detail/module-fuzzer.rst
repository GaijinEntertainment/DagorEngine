The FUZZER module implements facilities for the fuzz testing.

The idea behind the fuzz testing is to feed random data to the testing function and see if it crashes.
`panic` is considered a valid behavior, and in fact ignored.
Fuzz tests work really well in combination with the sanitizers (asan, ubsan, etc).

All functions and symbols are in "fuzzer" module, use require to get access to it. ::

    require daslib/fuzzer
