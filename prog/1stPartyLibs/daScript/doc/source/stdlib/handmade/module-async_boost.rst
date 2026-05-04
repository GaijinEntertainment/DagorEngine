The ASYNC_BOOST module implements an async/await pattern for daslang using
generator-based cooperative multitasking. It provides the ``[async]`` function
annotation, ``await`` for waiting on results, and ``await_next_frame`` for
suspending until the next step. Under the hood every ``[async]`` function is
transformed into a state-machine generator — no threads, channels, or job
queues are involved.

All functions and symbols are in "async_boost" module, use require to get access to it.

.. code-block:: das

    require daslib/async_boost

.. seealso::

   :ref:`tutorial_async` — Tutorial 49: Async / Await.

   :ref:`stdlib_coroutines` — coroutines module (underlying generator framework).
