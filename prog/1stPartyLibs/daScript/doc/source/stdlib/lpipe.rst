
.. _stdlib_lpipe:

===========
lpipe macro
===========

.. include:: detail/lpipe.rst

The lpipe module implements lpipe pattern, which allows piping blocks and expressions onto the previous line.

All functions and symbols are in "lpipe" module, use require to get access to it. ::

    require daslib/lpipe


+++++++++++
Call macros
+++++++++++

.. _call-macro-lpipe-lpipe:

.. das:attribute:: lpipe

This macro will implement the lpipe function. It allows piping blocks the previous line call. For example::

    def take2(a,b:block)
        invoke(a)
        invoke(b)
    ...
    take2 <|
        print("block1\n")
    lpipe <|    // this block will pipe into take2
        print("block2\n")


