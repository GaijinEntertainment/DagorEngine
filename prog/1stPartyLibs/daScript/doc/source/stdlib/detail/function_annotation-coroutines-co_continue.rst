This macro converts co_continue to yield true.
The idea is that coroutine without specified type is underneath a coroutine which yields bool.
That way co_continue() does not distract from the fact that it is a generator<bool>.
