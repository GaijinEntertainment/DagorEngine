this macro handles `new_job` and `new_thread` calls.
the call is replaced with `new_job_invoke` and `new_thread_invoke` accordingly.
a cloning infastructure is generated for the lambda, which is invoked in the new context.
