.. _stdlib_stddatetimelib:

====================
The Datetime library
====================

The datetime library date and time manipulation facilities

--------------
Squirrel API
--------------

++++++++++++++
Global Symbols
++++++++++++++

.. sq:function:: clock()

    returns a float representing the number of seconds elapsed since the start of the process

.. sq:function:: date([time [, format]])

    returns a table containing a date/time split into the slots:

+-------------+----------------------------------------+
| sec         | Seconds after minute (0 - 59).         |
+-------------+----------------------------------------+
| min         | Minutes after hour (0 - 59).           |
+-------------+----------------------------------------+
| hour        | Hours since midnight (0 - 23).         |
+-------------+----------------------------------------+
| day         | Day of month (1 - 31).                 |
+-------------+----------------------------------------+
| month       | Month (0 - 11; January = 0).           |
+-------------+----------------------------------------+
| year        | Year (current year).                   |
+-------------+----------------------------------------+
| wday        | Day of week (0 - 6; Sunday = 0).       |
+-------------+----------------------------------------+
| yday        | Day of year (0 - 365; January 1 = 0).  |
+-------------+----------------------------------------+

if `time` is omitted the current time is used.

if `format` can be 'l' local time or 'u' UTC time, if omitted is defaulted as 'l'(local time).

.. sq:function:: time()

    returns the number of seconds elapsed since midnight 00:00:00, January 1, 1970.

    the result of this function can be formatted through the function `date()`

--------------
C API
--------------

.. _sqstd_register_datetimelib:

.. c:function:: SQRESULT sqstd_register_datetimelib(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT
    :remarks: The function expects a table on top of the stack where to register the global library functions.

    initialize and register the datetime library in the given VM.
