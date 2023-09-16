
.. _stdlib_jobque_boost:

==================================
Boost package for jobs and threads
==================================

.. include:: detail/jobque_boost.rst

The JOBQUE boost module implements collection of helper macros and functions to accompany :ref:`JOBQUE <stdlib_jobque>`.

All functions and symbols are in "jobque_boost" module, use require to get access to it. ::

    require daslib/jobque_boost


++++++++++++++++++++
Function annotations
++++++++++++++++++++

.. _handle-jobque_boost-NewJobMacro:

.. das:attribute:: NewJobMacro

this macro handles `new_job` and `new_thread` calls.
the call is replaced with `new_job_invoke` and `new_thread_invoke` accordingly.
a cloning infastructure is generated for the lambda, which is invoked in the new context.

+++++++++++
Invocations
+++++++++++

  *  :ref:`new_job (l:lambda\<\> -const) : void <function-_at_jobque_boost_c__c_new_job__at_>` 
  *  :ref:`new_thread (l:lambda\<\> -const) : void <function-_at_jobque_boost_c__c_new_thread__at_>` 

.. _function-_at_jobque_boost_c__c_new_job__at_:

.. das:function:: new_job(l: lambda<>)

+--------+-------------+
+argument+argument type+
+========+=============+
+l       +lambda<>     +
+--------+-------------+


Create a new job.
    * new context is cloned from the current context.
    * lambda is cloned to the new context.
    * new job is added to the job queue.
    * once new job is invoked, lambda is invoked on the new context on the job thread.

.. _function-_at_jobque_boost_c__c_new_thread__at_:

.. das:function:: new_thread(l: lambda<>)

+--------+-------------+
+argument+argument type+
+========+=============+
+l       +lambda<>     +
+--------+-------------+


Create a new thread
    * new context is cloned from the current context.
    * lambda is cloned to the new context.
    * new thread is created.
    * lambda is invoked on the new context on the new thread.

+++++++++
Iteration
+++++++++

  *  :ref:`for_each (channel:jobque::Channel? const;blk:block\<(res:auto(TT) const#):void\> const) : auto <function-_at_jobque_boost_c__c_for_each_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?_CN_ls_res_gr_0_ls_C_hh_Y_ls_TT_gr_._gr_1_ls_v_gr__builtin_>` 
  *  :ref:`each (channel:jobque::Channel? -const;tinfo:auto(TT) const) : auto <function-_at_jobque_boost_c__c_each_1_ls_H_ls_jobque_c__c_Channel_gr__gr_?_CY_ls_TT_gr_.>` 

.. _function-_at_jobque_boost_c__c_for_each_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?_CN_ls_res_gr_0_ls_C_hh_Y_ls_TT_gr_._gr_1_ls_v_gr__builtin_:

.. das:function:: for_each(channel: jobque::Channel? const; blk: block<(res:auto(TT) const#):void> const)

for_each returns auto

.. warning:: 
  This function is deprecated.

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+channel + :ref:`jobque::Channel <handle-jobque-Channel>` ? const+
+--------+-------------------------------------------------------+
+blk     +block<(res:auto(TT) const#):void> const                +
+--------+-------------------------------------------------------+


reads input from the channel (in order it was pushed) and invokes the block on each input.
stops once channel is depleted (internal entry counter is 0)
this can happen on multiple threads or jobs at the same time.

.. _function-_at_jobque_boost_c__c_each_1_ls_H_ls_jobque_c__c_Channel_gr__gr_?_CY_ls_TT_gr_.:

.. das:function:: each(channel: jobque::Channel?; tinfo: auto(TT) const)

each returns auto

.. warning:: 
  This function is deprecated.

+--------+-------------------------------------------------+
+argument+argument type                                    +
+========+=================================================+
+channel + :ref:`jobque::Channel <handle-jobque-Channel>` ?+
+--------+-------------------------------------------------+
+tinfo   +auto(TT) const                                   +
+--------+-------------------------------------------------+


this iterator is used to iterate over the channel in order it was pushed.
iterator stops once channel is depleted (internal entry counter is 0)
iteration can happen on multiple threads or jobs at the same time.

++++++++++++
Passing data
++++++++++++

  *  :ref:`push_clone (channel:jobque::Channel? const;data:auto(TT) const) : auto <function-_at_jobque_boost_c__c_push_clone_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?_CY_ls_TT_gr_.>` 
  *  :ref:`push (channel:jobque::Channel? const;data:auto? const) : auto <function-_at_jobque_boost_c__c_push_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?_C1_ls_._gr_?>` 

.. _function-_at_jobque_boost_c__c_push_clone_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?_CY_ls_TT_gr_.:

.. das:function:: push_clone(channel: jobque::Channel? const; data: auto(TT) const)

push_clone returns auto

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+channel + :ref:`jobque::Channel <handle-jobque-Channel>` ? const+
+--------+-------------------------------------------------------+
+data    +auto(TT) const                                         +
+--------+-------------------------------------------------------+


clones data and pushed value to the channel (at the end)

.. _function-_at_jobque_boost_c__c_push_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?_C1_ls_._gr_?:

.. das:function:: push(channel: jobque::Channel? const; data: auto? const)

push returns auto

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+channel + :ref:`jobque::Channel <handle-jobque-Channel>` ? const+
+--------+-------------------------------------------------------+
+data    +auto? const                                            +
+--------+-------------------------------------------------------+


pushes value to the channel (at the end)

++++++++++++++++++++++++
Internal capture details
++++++++++++++++++++++++

  *  :ref:`capture_jobque_channel (ch:jobque::Channel? const) : jobque::Channel? <function-_at_jobque_boost_c__c_capture_jobque_channel_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?>` 
  *  :ref:`capture_jobque_job_status (js:jobque::JobStatus? const) : jobque::JobStatus? <function-_at_jobque_boost_c__c_capture_jobque_job_status_C1_ls_H_ls_jobque_c__c_JobStatus_gr__gr_?>` 
  *  :ref:`release_capture_jobque_channel (ch:jobque::Channel? const) : void <function-_at_jobque_boost_c__c_release_capture_jobque_channel_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?>` 
  *  :ref:`release_capture_jobque_job_status (js:jobque::JobStatus? const) : void <function-_at_jobque_boost_c__c_release_capture_jobque_job_status_C1_ls_H_ls_jobque_c__c_JobStatus_gr__gr_?>` 

.. _function-_at_jobque_boost_c__c_capture_jobque_channel_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?:

.. das:function:: capture_jobque_channel(ch: jobque::Channel? const)

capture_jobque_channel returns  :ref:`jobque::Channel <handle-jobque-Channel>` ?

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+ch      + :ref:`jobque::Channel <handle-jobque-Channel>` ? const+
+--------+-------------------------------------------------------+


this function is used to capture a channel that is used by the jobque.

.. _function-_at_jobque_boost_c__c_capture_jobque_job_status_C1_ls_H_ls_jobque_c__c_JobStatus_gr__gr_?:

.. das:function:: capture_jobque_job_status(js: jobque::JobStatus? const)

capture_jobque_job_status returns  :ref:`jobque::JobStatus <handle-jobque-JobStatus>` ?

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+js      + :ref:`jobque::JobStatus <handle-jobque-JobStatus>` ? const+
+--------+-----------------------------------------------------------+


this function is used to capture a job status that is used by the jobque.

.. _function-_at_jobque_boost_c__c_release_capture_jobque_channel_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?:

.. das:function:: release_capture_jobque_channel(ch: jobque::Channel? const)

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+ch      + :ref:`jobque::Channel <handle-jobque-Channel>` ? const+
+--------+-------------------------------------------------------+


this function is used to release a channel that is used by the jobque.

.. _function-_at_jobque_boost_c__c_release_capture_jobque_job_status_C1_ls_H_ls_jobque_c__c_JobStatus_gr__gr_?:

.. das:function:: release_capture_jobque_job_status(js: jobque::JobStatus? const)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+js      + :ref:`jobque::JobStatus <handle-jobque-JobStatus>` ? const+
+--------+-----------------------------------------------------------+


this function is used to release a job status that is used by the jobque.

+++++++++++++
Uncategorized
+++++++++++++

.. _function-_at_jobque_boost_c__c_capture_jobque_lock_box_C1_ls_H_ls_jobque_c__c_LockBox_gr__gr_?:

.. das:function:: capture_jobque_lock_box(js: jobque::LockBox? const)

capture_jobque_lock_box returns  :ref:`jobque::LockBox <handle-jobque-LockBox>` ?

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+js      + :ref:`jobque::LockBox <handle-jobque-LockBox>` ? const+
+--------+-------------------------------------------------------+


this function is used to capture a lock box that is used by the jobque.

.. _function-_at_jobque_boost_c__c_release_capture_jobque_lock_box_C1_ls_H_ls_jobque_c__c_LockBox_gr__gr_?:

.. das:function:: release_capture_jobque_lock_box(js: jobque::LockBox? const)

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+js      + :ref:`jobque::LockBox <handle-jobque-LockBox>` ? const+
+--------+-------------------------------------------------------+


this function is used to release a lock box that is used by the jobque.

.. _function-_at_jobque_boost_c__c_gather_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?_CN_ls_arg_gr_0_ls_C_hh_Y_ls_TT_gr_._gr_1_ls_v_gr__builtin_:

.. das:function:: gather(ch: jobque::Channel? const; blk: block<(arg:auto(TT) const#):void> const)

gather returns auto

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+ch      + :ref:`jobque::Channel <handle-jobque-Channel>` ? const+
+--------+-------------------------------------------------------+
+blk     +block<(arg:auto(TT) const#):void> const                +
+--------+-------------------------------------------------------+


reads input from the channel (in order it was pushed) and invokes the block on each input.
afterwards input is consumed

.. _function-_at_jobque_boost_c__c_gather_ex_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?_CN_ls_arg;info;ctx_gr_0_ls_C_hh_Y_ls_TT_gr_.;C1_ls_CH_ls_rtti_c__c_TypeInfo_gr__gr_?;H_ls_rtti_c__c_Context_gr__gr_1_ls_v_gr__builtin_:

.. das:function:: gather_ex(ch: jobque::Channel? const; blk: block<(arg:auto(TT) const#;info:rtti::TypeInfo const? const;var ctx:rtti::Context -const):void> const)

gather_ex returns auto

+--------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                                                                            +
+========+=========================================================================================================================================================+
+ch      + :ref:`jobque::Channel <handle-jobque-Channel>` ? const                                                                                                  +
+--------+---------------------------------------------------------------------------------------------------------------------------------------------------------+
+blk     +block<(arg:auto(TT) const#;info: :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>`  const? const;ctx: :ref:`rtti::Context <handle-rtti-Context>` ):void> const+
+--------+---------------------------------------------------------------------------------------------------------------------------------------------------------+


reads input from the channel (in order it was pushed) and invokes the block on each input.
afterwards input is consumed

.. _function-_at_jobque_boost_c__c_gather_and_forward_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?_CN_ls_arg_gr_0_ls_C_hh_Y_ls_TT_gr_._gr_1_ls_v_gr__builtin_:

.. das:function:: gather_and_forward(ch: jobque::Channel? const; toCh: jobque::Channel? const; blk: block<(arg:auto(TT) const#):void> const)

gather_and_forward returns auto

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+ch      + :ref:`jobque::Channel <handle-jobque-Channel>` ? const+
+--------+-------------------------------------------------------+
+toCh    + :ref:`jobque::Channel <handle-jobque-Channel>` ? const+
+--------+-------------------------------------------------------+
+blk     +block<(arg:auto(TT) const#):void> const                +
+--------+-------------------------------------------------------+


reads input from the channel (in order it was pushed) and invokes the block on each input.
afterwards input is consumed

.. _function-_at_jobque_boost_c__c_peek_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?_CN_ls_arg_gr_0_ls_C_hh_Y_ls_TT_gr_._gr_1_ls_v_gr__builtin_:

.. das:function:: peek(ch: jobque::Channel? const; blk: block<(arg:auto(TT) const#):void> const)

peek returns auto

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+ch      + :ref:`jobque::Channel <handle-jobque-Channel>` ? const+
+--------+-------------------------------------------------------+
+blk     +block<(arg:auto(TT) const#):void> const                +
+--------+-------------------------------------------------------+


reads input from the channel (in order it was pushed) and invokes the block on each input.
afterwards input is not consumed

.. _function-_at_jobque_boost_c__c_for_each_clone_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?_CN_ls_res_gr_0_ls_C_hh_Y_ls_TT_gr_._gr_1_ls_v_gr__builtin_:

.. das:function:: for_each_clone(channel: jobque::Channel? const; blk: block<(res:auto(TT) const#):void> const)

for_each_clone returns auto

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+channel + :ref:`jobque::Channel <handle-jobque-Channel>` ? const+
+--------+-------------------------------------------------------+
+blk     +block<(res:auto(TT) const#):void> const                +
+--------+-------------------------------------------------------+


reads input from the channel (in order it was pushed) and invokes the block on each input.
stops once channel is depleted (internal entry counter is 0)
this can happen on multiple threads or jobs at the same time.

.. _function-_at_jobque_boost_c__c_pop_one_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?_CN_ls_res_gr_0_ls_C_hh_Y_ls_TT_gr_._gr_1_ls_v_gr__builtin_:

.. das:function:: pop_one(channel: jobque::Channel? const; blk: block<(res:auto(TT) const#):void> const)

pop_one returns auto

.. warning:: 
  This function is deprecated.

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+channel + :ref:`jobque::Channel <handle-jobque-Channel>` ? const+
+--------+-------------------------------------------------------+
+blk     +block<(res:auto(TT) const#):void> const                +
+--------+-------------------------------------------------------+


reads one command from channel

.. _function-_at_jobque_boost_c__c_pop_and_clone_one_C1_ls_H_ls_jobque_c__c_Channel_gr__gr_?_CN_ls_res_gr_0_ls_C_hh_Y_ls_TT_gr_._gr_1_ls_v_gr__builtin_:

.. das:function:: pop_and_clone_one(channel: jobque::Channel? const; blk: block<(res:auto(TT) const#):void> const)

pop_and_clone_one returns auto

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+channel + :ref:`jobque::Channel <handle-jobque-Channel>` ? const+
+--------+-------------------------------------------------------+
+blk     +block<(res:auto(TT) const#):void> const                +
+--------+-------------------------------------------------------+


reads one command from channel

.. _function-_at_jobque_boost_c__c_set_C1_ls_H_ls_jobque_c__c_LockBox_gr__gr_?_CY_ls_TT_gr_.:

.. das:function:: set(box: jobque::LockBox? const; data: auto(TT) const)

set returns auto

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+box     + :ref:`jobque::LockBox <handle-jobque-LockBox>` ? const+
+--------+-------------------------------------------------------+
+data    +auto(TT) const                                         +
+--------+-------------------------------------------------------+


sets value to the lock box

.. _function-_at_jobque_boost_c__c_set_C1_ls_H_ls_jobque_c__c_LockBox_gr__gr_?_C1_ls_._gr_?:

.. das:function:: set(box: jobque::LockBox? const; data: auto? const)

set returns auto

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+box     + :ref:`jobque::LockBox <handle-jobque-LockBox>` ? const+
+--------+-------------------------------------------------------+
+data    +auto? const                                            +
+--------+-------------------------------------------------------+


sets value to the lock box

.. _function-_at_jobque_boost_c__c_get_C1_ls_H_ls_jobque_c__c_LockBox_gr__gr_?_CN_ls_res_gr_0_ls_C_hh_Y_ls_TT_gr_._gr_1_ls_v_gr__builtin_:

.. das:function:: get(box: jobque::LockBox? const; blk: block<(res:auto(TT) const#):void> const)

get returns auto

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+box     + :ref:`jobque::LockBox <handle-jobque-LockBox>` ? const+
+--------+-------------------------------------------------------+
+blk     +block<(res:auto(TT) const#):void> const                +
+--------+-------------------------------------------------------+


reads value from the lock box and invokes the block on it

.. _function-_at_jobque_boost_c__c_update_C1_ls_H_ls_jobque_c__c_LockBox_gr__gr_?_CN_ls_res_gr_0_ls__hh_Y_ls_TT_gr_._gr_1_ls_v_gr__builtin_:

.. das:function:: update(box: jobque::LockBox? const; blk: block<(var res:auto(TT)# -const):void> const)

update returns auto

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+box     + :ref:`jobque::LockBox <handle-jobque-LockBox>` ? const+
+--------+-------------------------------------------------------+
+blk     +block<(res:auto(TT)#):void> const                      +
+--------+-------------------------------------------------------+


update value in the lock box and invokes the block on it

.. _function-_at_jobque_boost_c__c_clear_C1_ls_H_ls_jobque_c__c_LockBox_gr__gr_?_CY_ls_TT_gr_.:

.. das:function:: clear(box: jobque::LockBox? const; type_: auto(TT) const)

clear returns auto

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+box     + :ref:`jobque::LockBox <handle-jobque-LockBox>` ? const+
+--------+-------------------------------------------------------+
+type_   +auto(TT) const                                         +
+--------+-------------------------------------------------------+


clear value from the lock box

.. _function-_at_jobque_boost_c__c_each_clone_1_ls_H_ls_jobque_c__c_Channel_gr__gr_?_CY_ls_TT_gr_.:

.. das:function:: each_clone(channel: jobque::Channel?; tinfo: auto(TT) const)

each_clone returns auto

+--------+-------------------------------------------------+
+argument+argument type                                    +
+========+=================================================+
+channel + :ref:`jobque::Channel <handle-jobque-Channel>` ?+
+--------+-------------------------------------------------+
+tinfo   +auto(TT) const                                   +
+--------+-------------------------------------------------+


this iterator is used to iterate over the channel in order it was pushed.
iterator stops once channel is depleted (internal entry counter is 0)
iteration can happen on multiple threads or jobs at the same time.


