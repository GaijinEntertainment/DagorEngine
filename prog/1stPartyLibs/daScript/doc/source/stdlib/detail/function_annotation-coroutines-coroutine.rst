This macro converts coroutine function into generator, adds return false.
Daslang impelmentation of coroutine is generator based. Function is converted into a state machine,
which can be resumed and suspended. The function is converted into a generator.
Generator yields bool if its a void coroutine, and yields the return type otherwise.
If return type is specified coroutine can serve as an advanced form of a generator.
