
.. _stdlib_jobque:

================
Jobs and threads
================

.. include:: detail/jobque.rst

Apply module implements job que and threading.

All functions and symbols are in "jobque" module, use require to get access to it. ::

    require jobque



++++++++++++++++++
Handled structures
++++++++++++++++++

.. _handle-jobque-JobStatus:

.. das:attribute:: JobStatus

JobStatus property operators are

+-------+----+
+isReady+bool+
+-------+----+
+isValid+bool+
+-------+----+
+size   +int +
+-------+----+


|structure_annotation-jobque-JobStatus|

.. _handle-jobque-Channel:

.. das:attribute:: Channel

Channel property operators are

+-------+----+
+isEmpty+bool+
+-------+----+
+total  +int +
+-------+----+


|structure_annotation-jobque-Channel|

.. _handle-jobque-LockBox:

.. das:attribute:: LockBox

|structure_annotation-jobque-LockBox|

.. _handle-jobque-Atomic32:

.. das:attribute:: Atomic32

|structure_annotation-jobque-Atomic32|

.. _handle-jobque-Atomic64:

.. das:attribute:: Atomic64

|structure_annotation-jobque-Atomic64|

+++++++++++++++++++++++++++
Channel, JobStatus, Lockbox
+++++++++++++++++++++++++++

  *  :ref:`lock_box_create (context:__context const;line:__lineInfo const) : jobque::LockBox? <function-_at_jobque_c__c_lock_box_create_C_c_C_l>` 
  *  :ref:`lock_box_remove (box:jobque::LockBox?& implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_lock_box_remove_&I1_ls_H_ls_jobque_c__c_LockBox_gr__gr__qm__C_c_C_l>` 
  *  :ref:`append (channel:jobque::JobStatus? const implicit;size:int const;context:__context const;line:__lineInfo const) : int <function-_at_jobque_c__c_append_CI1_ls_H_ls_jobque_c__c_JobStatus_gr__gr__qm__Ci_C_c_C_l>` 
  *  :ref:`channel_create (context:__context const;line:__lineInfo const) : jobque::Channel? <function-_at_jobque_c__c_channel_create_C_c_C_l>` 
  *  :ref:`channel_remove (channel:jobque::Channel?& implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_channel_remove_&I1_ls_H_ls_jobque_c__c_Channel_gr__gr__qm__C_c_C_l>` 
  *  :ref:`add_ref (status:jobque::JobStatus? const implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_add_ref_CI1_ls_H_ls_jobque_c__c_JobStatus_gr__gr__qm__C_c_C_l>` 
  *  :ref:`release (status:jobque::JobStatus?& implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_release_&I1_ls_H_ls_jobque_c__c_JobStatus_gr__gr__qm__C_c_C_l>` 
  *  :ref:`join (job:jobque::JobStatus? const implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_join_CI1_ls_H_ls_jobque_c__c_JobStatus_gr__gr__qm__C_c_C_l>` 
  *  :ref:`notify (job:jobque::JobStatus? const implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_notify_CI1_ls_H_ls_jobque_c__c_JobStatus_gr__gr__qm__C_c_C_l>` 
  *  :ref:`notify_and_release (job:jobque::JobStatus?& implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_notify_and_release_&I1_ls_H_ls_jobque_c__c_JobStatus_gr__gr__qm__C_c_C_l>` 
  *  :ref:`job_status_create (context:__context const;line:__lineInfo const) : jobque::JobStatus? <function-_at_jobque_c__c_job_status_create_C_c_C_l>` 
  *  :ref:`job_status_remove (jobStatus:jobque::JobStatus?& implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_job_status_remove_&I1_ls_H_ls_jobque_c__c_JobStatus_gr__gr__qm__C_c_C_l>` 

.. _function-_at_jobque_c__c_lock_box_create_C_c_C_l:

.. das:function:: lock_box_create()

lock_box_create returns  :ref:`jobque::LockBox <handle-jobque-LockBox>` ?

|function-jobque-lock_box_create|

.. _function-_at_jobque_c__c_lock_box_remove_&I1_ls_H_ls_jobque_c__c_LockBox_gr__gr__qm__C_c_C_l:

.. das:function:: lock_box_remove(box: LockBox?& implicit)

.. warning:: 
  This is unsafe operation.

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+box     + :ref:`jobque::LockBox <handle-jobque-LockBox>` ?& implicit+
+--------+-----------------------------------------------------------+


|function-jobque-lock_box_remove|

.. _function-_at_jobque_c__c_append_CI1_ls_H_ls_jobque_c__c_JobStatus_gr__gr__qm__Ci_C_c_C_l:

.. das:function:: append(channel: JobStatus? const implicit; size: int const)

append returns int

+--------+--------------------------------------------------------------------+
+argument+argument type                                                       +
+========+====================================================================+
+channel + :ref:`jobque::JobStatus <handle-jobque-JobStatus>` ? const implicit+
+--------+--------------------------------------------------------------------+
+size    +int const                                                           +
+--------+--------------------------------------------------------------------+


|function-jobque-append|

.. _function-_at_jobque_c__c_channel_create_C_c_C_l:

.. das:function:: channel_create()

channel_create returns  :ref:`jobque::Channel <handle-jobque-Channel>` ?

.. warning:: 
  This is unsafe operation.

|function-jobque-channel_create|

.. _function-_at_jobque_c__c_channel_remove_&I1_ls_H_ls_jobque_c__c_Channel_gr__gr__qm__C_c_C_l:

.. das:function:: channel_remove(channel: Channel?& implicit)

.. warning:: 
  This is unsafe operation.

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+channel + :ref:`jobque::Channel <handle-jobque-Channel>` ?& implicit+
+--------+-----------------------------------------------------------+


|function-jobque-channel_remove|

.. _function-_at_jobque_c__c_add_ref_CI1_ls_H_ls_jobque_c__c_JobStatus_gr__gr__qm__C_c_C_l:

.. das:function:: add_ref(status: JobStatus? const implicit)

+--------+--------------------------------------------------------------------+
+argument+argument type                                                       +
+========+====================================================================+
+status  + :ref:`jobque::JobStatus <handle-jobque-JobStatus>` ? const implicit+
+--------+--------------------------------------------------------------------+


|function-jobque-add_ref|

.. _function-_at_jobque_c__c_release_&I1_ls_H_ls_jobque_c__c_JobStatus_gr__gr__qm__C_c_C_l:

.. das:function:: release(status: JobStatus?& implicit)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+status  + :ref:`jobque::JobStatus <handle-jobque-JobStatus>` ?& implicit+
+--------+---------------------------------------------------------------+


|function-jobque-release|

.. _function-_at_jobque_c__c_join_CI1_ls_H_ls_jobque_c__c_JobStatus_gr__gr__qm__C_c_C_l:

.. das:function:: join(job: JobStatus? const implicit)

+--------+--------------------------------------------------------------------+
+argument+argument type                                                       +
+========+====================================================================+
+job     + :ref:`jobque::JobStatus <handle-jobque-JobStatus>` ? const implicit+
+--------+--------------------------------------------------------------------+


|function-jobque-join|

.. _function-_at_jobque_c__c_notify_CI1_ls_H_ls_jobque_c__c_JobStatus_gr__gr__qm__C_c_C_l:

.. das:function:: notify(job: JobStatus? const implicit)

+--------+--------------------------------------------------------------------+
+argument+argument type                                                       +
+========+====================================================================+
+job     + :ref:`jobque::JobStatus <handle-jobque-JobStatus>` ? const implicit+
+--------+--------------------------------------------------------------------+


|function-jobque-notify|

.. _function-_at_jobque_c__c_notify_and_release_&I1_ls_H_ls_jobque_c__c_JobStatus_gr__gr__qm__C_c_C_l:

.. das:function:: notify_and_release(job: JobStatus?& implicit)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+job     + :ref:`jobque::JobStatus <handle-jobque-JobStatus>` ?& implicit+
+--------+---------------------------------------------------------------+


|function-jobque-notify_and_release|

.. _function-_at_jobque_c__c_job_status_create_C_c_C_l:

.. das:function:: job_status_create()

job_status_create returns  :ref:`jobque::JobStatus <handle-jobque-JobStatus>` ?

|function-jobque-job_status_create|

.. _function-_at_jobque_c__c_job_status_remove_&I1_ls_H_ls_jobque_c__c_JobStatus_gr__gr__qm__C_c_C_l:

.. das:function:: job_status_remove(jobStatus: JobStatus?& implicit)

.. warning:: 
  This is unsafe operation.

+---------+---------------------------------------------------------------+
+argument +argument type                                                  +
+=========+===============================================================+
+jobStatus+ :ref:`jobque::JobStatus <handle-jobque-JobStatus>` ?& implicit+
+---------+---------------------------------------------------------------+


|function-jobque-job_status_remove|

+++++++
Queries
+++++++

  *  :ref:`get_total_hw_jobs (context:__context const;line:__lineInfo const) : int <function-_at_jobque_c__c_get_total_hw_jobs_C_c_C_l>` 
  *  :ref:`get_total_hw_threads () : int <function-_at_jobque_c__c_get_total_hw_threads>` 
  *  :ref:`is_job_que_shutting_down () : bool <function-_at_jobque_c__c_is_job_que_shutting_down>` 

.. _function-_at_jobque_c__c_get_total_hw_jobs_C_c_C_l:

.. das:function:: get_total_hw_jobs()

get_total_hw_jobs returns int

|function-jobque-get_total_hw_jobs|

.. _function-_at_jobque_c__c_get_total_hw_threads:

.. das:function:: get_total_hw_threads()

get_total_hw_threads returns int

|function-jobque-get_total_hw_threads|

.. _function-_at_jobque_c__c_is_job_que_shutting_down:

.. das:function:: is_job_que_shutting_down()

is_job_que_shutting_down returns bool

|function-jobque-is_job_que_shutting_down|

++++++++++++++++++++
Internal invocations
++++++++++++++++++++

  *  :ref:`new_job_invoke (lambda:lambda\<\> const;function:function\<\> const;lambdaSize:int const;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_new_job_invoke_C_at__C_at__at__Ci_C_c_C_l>` 
  *  :ref:`new_thread_invoke (lambda:lambda\<\> const;function:function\<\> const;lambdaSize:int const;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_new_thread_invoke_C_at__C_at__at__Ci_C_c_C_l>` 
  *  :ref:`new_debugger_thread (block:block\<\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_new_debugger_thread_CI_builtin__C_c_C_l>` 

.. _function-_at_jobque_c__c_new_job_invoke_C_at__C_at__at__Ci_C_c_C_l:

.. das:function:: new_job_invoke(lambda: lambda<> const; function: function<> const; lambdaSize: int const)

+----------+----------------+
+argument  +argument type   +
+==========+================+
+lambda    +lambda<> const  +
+----------+----------------+
+function  +function<> const+
+----------+----------------+
+lambdaSize+int const       +
+----------+----------------+


|function-jobque-new_job_invoke|

.. _function-_at_jobque_c__c_new_thread_invoke_C_at__C_at__at__Ci_C_c_C_l:

.. das:function:: new_thread_invoke(lambda: lambda<> const; function: function<> const; lambdaSize: int const)

+----------+----------------+
+argument  +argument type   +
+==========+================+
+lambda    +lambda<> const  +
+----------+----------------+
+function  +function<> const+
+----------+----------------+
+lambdaSize+int const       +
+----------+----------------+


|function-jobque-new_thread_invoke|

.. _function-_at_jobque_c__c_new_debugger_thread_CI_builtin__C_c_C_l:

.. das:function:: new_debugger_thread(block: block<> const implicit)

+--------+----------------------+
+argument+argument type         +
+========+======================+
+block   +block<> const implicit+
+--------+----------------------+


|function-jobque-new_debugger_thread|

++++++++++++
Construction
++++++++++++

  *  :ref:`with_lock_box (block:block\<(var arg0:jobque::LockBox?):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_with_lock_box_CI0_ls_1_ls_H_ls_jobque_c__c_LockBox_gr__gr__qm__gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`with_channel (block:block\<(var arg0:jobque::Channel?):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_with_channel_CI0_ls_1_ls_H_ls_jobque_c__c_Channel_gr__gr__qm__gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`with_channel (count:int const;block:block\<(var arg0:jobque::Channel?):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_with_channel_Ci_CI0_ls_1_ls_H_ls_jobque_c__c_Channel_gr__gr__qm__gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`with_job_status (total:int const;block:block\<(var arg0:jobque::JobStatus?):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_with_job_status_Ci_CI0_ls_1_ls_H_ls_jobque_c__c_JobStatus_gr__gr__qm__gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`with_job_que (block:block\<void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_with_job_que_CI1_ls_v_gr__builtin__C_c_C_l>` 

.. _function-_at_jobque_c__c_with_lock_box_CI0_ls_1_ls_H_ls_jobque_c__c_LockBox_gr__gr__qm__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: with_lock_box(block: block<(var arg0:LockBox?):void> const implicit)

+--------+------------------------------------------------------------------------------+
+argument+argument type                                                                 +
+========+==============================================================================+
+block   +block<( :ref:`jobque::LockBox <handle-jobque-LockBox>` ?):void> const implicit+
+--------+------------------------------------------------------------------------------+


|function-jobque-with_lock_box|

.. _function-_at_jobque_c__c_with_channel_CI0_ls_1_ls_H_ls_jobque_c__c_Channel_gr__gr__qm__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: with_channel(block: block<(var arg0:Channel?):void> const implicit)

+--------+------------------------------------------------------------------------------+
+argument+argument type                                                                 +
+========+==============================================================================+
+block   +block<( :ref:`jobque::Channel <handle-jobque-Channel>` ?):void> const implicit+
+--------+------------------------------------------------------------------------------+


|function-jobque-with_channel|

.. _function-_at_jobque_c__c_with_channel_Ci_CI0_ls_1_ls_H_ls_jobque_c__c_Channel_gr__gr__qm__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: with_channel(count: int const; block: block<(var arg0:Channel?):void> const implicit)

+--------+------------------------------------------------------------------------------+
+argument+argument type                                                                 +
+========+==============================================================================+
+count   +int const                                                                     +
+--------+------------------------------------------------------------------------------+
+block   +block<( :ref:`jobque::Channel <handle-jobque-Channel>` ?):void> const implicit+
+--------+------------------------------------------------------------------------------+


|function-jobque-with_channel|

.. _function-_at_jobque_c__c_with_job_status_Ci_CI0_ls_1_ls_H_ls_jobque_c__c_JobStatus_gr__gr__qm__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: with_job_status(total: int const; block: block<(var arg0:JobStatus?):void> const implicit)

+--------+----------------------------------------------------------------------------------+
+argument+argument type                                                                     +
+========+==================================================================================+
+total   +int const                                                                         +
+--------+----------------------------------------------------------------------------------+
+block   +block<( :ref:`jobque::JobStatus <handle-jobque-JobStatus>` ?):void> const implicit+
+--------+----------------------------------------------------------------------------------+


|function-jobque-with_job_status|

.. _function-_at_jobque_c__c_with_job_que_CI1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: with_job_que(block: block<void> const implicit)

+--------+----------------------+
+argument+argument type         +
+========+======================+
+block   +block<> const implicit+
+--------+----------------------+


|function-jobque-with_job_que|

++++++
Atomic
++++++

  *  :ref:`atomic32_create (context:__context const;line:__lineInfo const) : jobque::Atomic32? <function-_at_jobque_c__c_atomic32_create_C_c_C_l>` 
  *  :ref:`atomic32_remove (atomic:jobque::Atomic32?& implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_atomic32_remove_&I1_ls_H_ls_jobque_c__c_Atomic32_gr__gr__qm__C_c_C_l>` 
  *  :ref:`with_atomic32 (block:block\<(var arg0:jobque::Atomic32?):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_with_atomic32_CI0_ls_1_ls_H_ls_jobque_c__c_Atomic32_gr__gr__qm__gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`set (atomic:jobque::Atomic32? const implicit;value:int const;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_set_CI1_ls_H_ls_jobque_c__c_Atomic32_gr__gr__qm__Ci_C_c_C_l>` 
  *  :ref:`get (atomic:jobque::Atomic32? const implicit;context:__context const;line:__lineInfo const) : int <function-_at_jobque_c__c_get_CI1_ls_H_ls_jobque_c__c_Atomic32_gr__gr__qm__C_c_C_l>` 
  *  :ref:`inc (atomic:jobque::Atomic32? const implicit;context:__context const;line:__lineInfo const) : int <function-_at_jobque_c__c_inc_CI1_ls_H_ls_jobque_c__c_Atomic32_gr__gr__qm__C_c_C_l>` 
  *  :ref:`dec (atomic:jobque::Atomic32? const implicit;context:__context const;line:__lineInfo const) : int <function-_at_jobque_c__c_dec_CI1_ls_H_ls_jobque_c__c_Atomic32_gr__gr__qm__C_c_C_l>` 
  *  :ref:`atomic64_create (context:__context const;line:__lineInfo const) : jobque::Atomic64? <function-_at_jobque_c__c_atomic64_create_C_c_C_l>` 
  *  :ref:`atomic64_remove (atomic:jobque::Atomic64?& implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_atomic64_remove_&I1_ls_H_ls_jobque_c__c_Atomic64_gr__gr__qm__C_c_C_l>` 
  *  :ref:`with_atomic64 (block:block\<(var arg0:jobque::Atomic64?):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_with_atomic64_CI0_ls_1_ls_H_ls_jobque_c__c_Atomic64_gr__gr__qm__gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`set (atomic:jobque::Atomic64? const implicit;value:int64 const;context:__context const;line:__lineInfo const) : void <function-_at_jobque_c__c_set_CI1_ls_H_ls_jobque_c__c_Atomic64_gr__gr__qm__Ci64_C_c_C_l>` 
  *  :ref:`get (atomic:jobque::Atomic64? const implicit;context:__context const;line:__lineInfo const) : int64 <function-_at_jobque_c__c_get_CI1_ls_H_ls_jobque_c__c_Atomic64_gr__gr__qm__C_c_C_l>` 
  *  :ref:`inc (atomic:jobque::Atomic64? const implicit;context:__context const;line:__lineInfo const) : int64 <function-_at_jobque_c__c_inc_CI1_ls_H_ls_jobque_c__c_Atomic64_gr__gr__qm__C_c_C_l>` 
  *  :ref:`dec (atomic:jobque::Atomic64? const implicit;context:__context const;line:__lineInfo const) : int64 <function-_at_jobque_c__c_dec_CI1_ls_H_ls_jobque_c__c_Atomic64_gr__gr__qm__C_c_C_l>` 

.. _function-_at_jobque_c__c_atomic32_create_C_c_C_l:

.. das:function:: atomic32_create()

atomic32_create returns  :ref:`jobque::Atomic32 <handle-jobque-Atomic32>` ?

|function-jobque-atomic32_create|

.. _function-_at_jobque_c__c_atomic32_remove_&I1_ls_H_ls_jobque_c__c_Atomic32_gr__gr__qm__C_c_C_l:

.. das:function:: atomic32_remove(atomic: Atomic32?& implicit)

.. warning:: 
  This is unsafe operation.

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+atomic  + :ref:`jobque::Atomic32 <handle-jobque-Atomic32>` ?& implicit+
+--------+-------------------------------------------------------------+


|function-jobque-atomic32_remove|

.. _function-_at_jobque_c__c_with_atomic32_CI0_ls_1_ls_H_ls_jobque_c__c_Atomic32_gr__gr__qm__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: with_atomic32(block: block<(var arg0:Atomic32?):void> const implicit)

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+block   +block<( :ref:`jobque::Atomic32 <handle-jobque-Atomic32>` ?):void> const implicit+
+--------+--------------------------------------------------------------------------------+


|function-jobque-with_atomic32|

.. _function-_at_jobque_c__c_set_CI1_ls_H_ls_jobque_c__c_Atomic32_gr__gr__qm__Ci_C_c_C_l:

.. das:function:: set(atomic: Atomic32? const implicit; value: int const)

+--------+------------------------------------------------------------------+
+argument+argument type                                                     +
+========+==================================================================+
+atomic  + :ref:`jobque::Atomic32 <handle-jobque-Atomic32>` ? const implicit+
+--------+------------------------------------------------------------------+
+value   +int const                                                         +
+--------+------------------------------------------------------------------+


|function-jobque-set|

.. _function-_at_jobque_c__c_get_CI1_ls_H_ls_jobque_c__c_Atomic32_gr__gr__qm__C_c_C_l:

.. das:function:: get(atomic: Atomic32? const implicit)

get returns int

+--------+------------------------------------------------------------------+
+argument+argument type                                                     +
+========+==================================================================+
+atomic  + :ref:`jobque::Atomic32 <handle-jobque-Atomic32>` ? const implicit+
+--------+------------------------------------------------------------------+


|function-jobque-get|

.. _function-_at_jobque_c__c_inc_CI1_ls_H_ls_jobque_c__c_Atomic32_gr__gr__qm__C_c_C_l:

.. das:function:: inc(atomic: Atomic32? const implicit)

inc returns int

+--------+------------------------------------------------------------------+
+argument+argument type                                                     +
+========+==================================================================+
+atomic  + :ref:`jobque::Atomic32 <handle-jobque-Atomic32>` ? const implicit+
+--------+------------------------------------------------------------------+


|function-jobque-inc|

.. _function-_at_jobque_c__c_dec_CI1_ls_H_ls_jobque_c__c_Atomic32_gr__gr__qm__C_c_C_l:

.. das:function:: dec(atomic: Atomic32? const implicit)

dec returns int

+--------+------------------------------------------------------------------+
+argument+argument type                                                     +
+========+==================================================================+
+atomic  + :ref:`jobque::Atomic32 <handle-jobque-Atomic32>` ? const implicit+
+--------+------------------------------------------------------------------+


|function-jobque-dec|

.. _function-_at_jobque_c__c_atomic64_create_C_c_C_l:

.. das:function:: atomic64_create()

atomic64_create returns  :ref:`jobque::Atomic64 <handle-jobque-Atomic64>` ?

|function-jobque-atomic64_create|

.. _function-_at_jobque_c__c_atomic64_remove_&I1_ls_H_ls_jobque_c__c_Atomic64_gr__gr__qm__C_c_C_l:

.. das:function:: atomic64_remove(atomic: Atomic64?& implicit)

.. warning:: 
  This is unsafe operation.

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+atomic  + :ref:`jobque::Atomic64 <handle-jobque-Atomic64>` ?& implicit+
+--------+-------------------------------------------------------------+


|function-jobque-atomic64_remove|

.. _function-_at_jobque_c__c_with_atomic64_CI0_ls_1_ls_H_ls_jobque_c__c_Atomic64_gr__gr__qm__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: with_atomic64(block: block<(var arg0:Atomic64?):void> const implicit)

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+block   +block<( :ref:`jobque::Atomic64 <handle-jobque-Atomic64>` ?):void> const implicit+
+--------+--------------------------------------------------------------------------------+


|function-jobque-with_atomic64|

.. _function-_at_jobque_c__c_set_CI1_ls_H_ls_jobque_c__c_Atomic64_gr__gr__qm__Ci64_C_c_C_l:

.. das:function:: set(atomic: Atomic64? const implicit; value: int64 const)

+--------+------------------------------------------------------------------+
+argument+argument type                                                     +
+========+==================================================================+
+atomic  + :ref:`jobque::Atomic64 <handle-jobque-Atomic64>` ? const implicit+
+--------+------------------------------------------------------------------+
+value   +int64 const                                                       +
+--------+------------------------------------------------------------------+


|function-jobque-set|

.. _function-_at_jobque_c__c_get_CI1_ls_H_ls_jobque_c__c_Atomic64_gr__gr__qm__C_c_C_l:

.. das:function:: get(atomic: Atomic64? const implicit)

get returns int64

+--------+------------------------------------------------------------------+
+argument+argument type                                                     +
+========+==================================================================+
+atomic  + :ref:`jobque::Atomic64 <handle-jobque-Atomic64>` ? const implicit+
+--------+------------------------------------------------------------------+


|function-jobque-get|

.. _function-_at_jobque_c__c_inc_CI1_ls_H_ls_jobque_c__c_Atomic64_gr__gr__qm__C_c_C_l:

.. das:function:: inc(atomic: Atomic64? const implicit)

inc returns int64

+--------+------------------------------------------------------------------+
+argument+argument type                                                     +
+========+==================================================================+
+atomic  + :ref:`jobque::Atomic64 <handle-jobque-Atomic64>` ? const implicit+
+--------+------------------------------------------------------------------+


|function-jobque-inc|

.. _function-_at_jobque_c__c_dec_CI1_ls_H_ls_jobque_c__c_Atomic64_gr__gr__qm__C_c_C_l:

.. das:function:: dec(atomic: Atomic64? const implicit)

dec returns int64

+--------+------------------------------------------------------------------+
+argument+argument type                                                     +
+========+==================================================================+
+atomic  + :ref:`jobque::Atomic64 <handle-jobque-Atomic64>` ? const implicit+
+--------+------------------------------------------------------------------+


|function-jobque-dec|


