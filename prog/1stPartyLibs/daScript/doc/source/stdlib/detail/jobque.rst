.. |function-jobque-append| replace:: Increase entry count to the channel.

.. |function-jobque-with_channel| replace:: Creates `Channel`, makes it available inside the scope of the block.

.. |function-jobque-join| replace:: Wait until channel entry count reaches 0.

.. |function-jobque-notify| replace:: Notify channel that entry is completed (decrease entry count).

.. |function-jobque-with_job_status| replace:: Creates `JobStatus`, makes it available inside the scope of the block.

.. |function-jobque-job_status_create| replace:: Creates job status.

.. |function-jobque-job_status_remove| replace:: Destroys job status.

.. |function-jobque-new_job_invoke| replace:: Creates clone of the current context, moves attached lambda to it.
    Adds a job to a job que, which once invoked will execute the lambda on the context clone.
    `new_job_invoke` is part of the low level (internal) job infrastructure. Recommended approach is to use `jobque_boost::new_job`.

.. |function-jobque-with_job_que| replace:: Makes sure jobque infrastructure is available inside the scope of the block.
    There is cost associated with creating such infrastructure (i.e. creating hardware threads, jobs, etc).
    If jobs are integral part of the application, with_job_que should be high in the call stack.
    If it`s a one-off - it should be encricled accordingly to reduce runtime memory footprint of the application.

.. |function-jobque-get_total_hw_jobs| replace:: Total number of hardware threads supporting job system.

.. |function-jobque-get_total_hw_threads| replace:: Total number of hardware threads available.

.. |function-jobque-new_thread_invoke| replace:: Creates clone of the current context, moves attached lambda to it.
    Creates a thread, invokes the lambda on the new context in that thread.
    `new_thread_invoke` is part of the low level (internal) thread infrastructure. Recommended approach is to use `jobque_boost::new_thread`.

.. |function-jobque-is_job_que_shutting_down| replace:: Returns true if job que infrastructure is shut-down or not initialized.
    This is useful for debug contexts, since it allows to check if job que is still alive.

.. |structure_annotation-jobque-Channel| replace:: Channel provides a way to communicate between multiple contexts, including threads and jobs. Channel has internal entry count.

.. |structure_annotation-jobque-JobStatus| replace:: Job status indicator (ready or not, as well as entry count).

.. |function-jobque-add_ref| replace:: Increase reference count of the job status or channel.

.. |function-jobque-release| replace:: Decrease reference count of the job status or channel. Object is delete when reference count reaches 0.

.. |function-jobque-notify_and_release| replace:: Notify channel or job status that entry is completed (decrease entry count) and decrease reference count of the job status or channel.
    Object is delete when reference count reaches 0.

.. |function-jobque-channel_create| replace:: Creates channel.

.. |function-jobque-channel_remove| replace:: Destroys channel.

.. |structure_annotation-jobque-LockBox| replace:: Lockbox. Similar to channel, only for single object.

.. |function-jobque-lock_box_create| replace:: Creates lockbox.

.. |function-jobque-lock_box_remove| replace:: Destroys lockbox.

.. |function-jobque-with_lock_box| replace:: Creates `LockBox`, makes it available inside the scope of the block.

.. |structure_annotation-jobque-Atomic32| replace:: Atomic 32 bit integer.

.. |function-jobque-atomic32_create| replace:: Creates atomic 32 bit integer.

.. |function-jobque-atomic32_remove| replace:: Destroys atomic 32 bit integer.

.. |function-jobque-with_atomic32| replace:: Creates `Atomic32`, makes it available inside the scope of the block.

.. |function-jobque-set| replace:: Set atomic integer value.

.. |function-jobque-get| replace:: Get atomic integer value.

.. |function-jobque-inc| replace:: Increase atomic integer value and returns result.

.. |function-jobque-dec| replace:: Decrease atomic integer value and returns result.

.. |structure_annotation-jobque-Atomic64| replace:: Atomic 64 bit integer.

.. |function-jobque-atomic64_create| replace:: Creates atomic 64 bit integer.

.. |function-jobque-atomic64_remove| replace:: Destroys atomic 64 bit integer.

.. |function-jobque-with_atomic64| replace:: Creates `Atomic64`, makes it available inside the scope of the block.



