.. _embedding_build_configuration:

========================
Build Configuration
========================

.. _squirrel_64bits:

--------------------------------
Quirrel on 64-bit architectures
--------------------------------

.. index::
    single: Quirrel on 64-bit architectures
    single: 64 bits

Quirrel can be compiled on 64-bit architectures by defining '_SQ64' in the C++
preprocessor. This flag should be defined in any project that includes 'squirrel.h'.

.. _userdata_alignment:

------------------
Userdata Alignment
------------------

.. index:: single: Userdata Alignment

Both class instances and userdatas can have a buffer associated to them.
Quirrel specifies the alignment(in bytes) through the preprocessor defining 'SQ_ALIGNMENT'.
By default SQ_ALIGNMENT is defined as 4 for 32-bit builds and 8 for 64-bit builds and builds that use 64-bit floats.
It is possible to override the value of SQ_ALIGNMENT respecting the following rules.
SQ_ALIGNMENT shall be less than or equal to SQ_MALLOC alignments, and it shall be power of 2.

.. note:: This only applies for userdata allocated by the VM, specified via sq_setclassudsize() or belonging to a userdata object.
        userpointers specified by the user are not affected by alignment rules.

.. _standalone_vm:
