.. _stdlib_stdiolib:

========================
The Input/Output library
========================

the i/o library implements basic input/output routines.

--------------
Quirrel API
--------------

++++++++++++++
Global Symbols
++++++++++++++


.. sq:data:: stderr

    File object bound on the os *standard error* stream

.. sq:data:: stdin

    File object bound on the os *standard input* stream

.. sq:data:: stdout

    File object bound on the os *standard output* stream


++++++++++++++
The file class
++++++++++++++

    The file object implements a stream on a operating system file.

.. sq:class:: file(path, patten)

    It's constructor imitates the behaviour of the C runtime function fopen for eg. ::

        local myfile = file("test.xxx","wb+");

    creates a file with read/write access in the current directory.

.. sq:function:: file.close()

    closes the file.

.. sq:function:: file.eos()

    returns a non null value if the read/write pointer is at the end of the stream.

.. sq:function:: file.flush()

    flushes the stream.return a value != null if succeeded, otherwise returns null

.. sq:function:: file.len()

    returns the length of the stream

.. sq:function:: file.readblob(size)

    :param int size: number of bytes to read

    read n bytes from the stream and returns them as blob

.. sq:function:: file.readn(type)

    :param int type: type of the number to read

    reads a number from the stream according to the type parameter.

    `type` can have the following values:

+--------------+--------------------------------------------------------------------------------+----------------------+
| parameter    | return description                                                             |  return type         |
+==============+================================================================================+======================+
| 'l'          | processor dependent, 32bits on 32bits processors, 64bits on 64bits processors  |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'i'          | 32bits number                                                                  |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 's'          | 16bits signed integer                                                          |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'w'          | 16bits unsigned integer                                                        |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'c'          | 8bits signed integer                                                           |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'b'          | 8bits unsigned integer                                                         |  integer             |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'f'          | 32bits float                                                                   |  float               |
+--------------+--------------------------------------------------------------------------------+----------------------+
| 'd'          | 64bits float                                                                   |  float               |
+--------------+--------------------------------------------------------------------------------+----------------------+

.. sq:function:: file.resize(size)

    :param int size: the new size of the blob in bytes

    resizes the blob to the specified `size`

.. sq:function:: file.seek(offset [,origin])

    :param int offset: indicates the number of bytes from `origin`.
    :param int origin: origin of the seek

                        +--------------+-------------------------------------------+
                        |  'b'         |  beginning of the stream                  |
                        +--------------+-------------------------------------------+
                        |  'c'         |  current location                         |
                        +--------------+-------------------------------------------+
                        |  'e'         |  end of the stream                        |
                        +--------------+-------------------------------------------+

    Moves the read/write pointer to a specified location.

.. note:: If origin is omitted the parameter is defaulted as 'b'(beginning of the stream).

.. sq:function:: file.tell()

    returns the read/write pointer absolute position

.. sq:function:: file.writeblob(src)

    :param blob src: the source blob containing the data to be written

    writes a blob in the stream

.. sq:function:: file.writen(n, type)

    :param number n: the value to be written
    :param int type: type of the number to write

    writes a number in the stream formatted according to the `type` pamraeter

    `type` can have the following values:

+--------------+--------------------------------------------------------------------------------+
| parameter    | return description                                                             |
+==============+================================================================================+
| 'i'          | 32bits number                                                                  |
+--------------+--------------------------------------------------------------------------------+
| 's'          | 16bits signed integer                                                          |
+--------------+--------------------------------------------------------------------------------+
| 'w'          | 16bits unsigned integer                                                        |
+--------------+--------------------------------------------------------------------------------+
| 'c'          | 8bits signed integer                                                           |
+--------------+--------------------------------------------------------------------------------+
| 'b'          | 8bits unsigned integer                                                         |
+--------------+--------------------------------------------------------------------------------+
| 'f'          | 32bits float                                                                   |
+--------------+--------------------------------------------------------------------------------+
| 'd'          | 64bits float                                                                   |
+--------------+--------------------------------------------------------------------------------+


--------------
C API
--------------

.. _sqstd_register_iolib:

.. c:function:: SQRESULT sqstd_register_iolib(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT
    :remarks: The function aspects a table on top of the stack where to register the global library functions.

    initialize and register the io library in the given VM.

++++++++++++++
File Object
++++++++++++++

.. c:function:: SQRESULT sqstd_createfile(HSQUIRRELVM v, SQFILE file, SQBool owns)

    :param HSQUIRRELVM v: the target VM
    :param SQFILE file: the stream that will be rapresented by the file object
    :param SQBool owns: if different true the stream will be automatically closed when the newly create file object is destroyed.
    :returns: an SQRESULT

    creates a file object bound to the SQFILE passed as parameter
    and pushes it in the stack

.. c:function:: SQRESULT sqstd_getfile(HSQUIRRELVM v, SQInteger idx, SQFILE* file)

    :param HSQUIRRELVM v: the target VM
    :param SQInteger idx: and index in the stack
    :param SQFILE* file: A pointer to a SQFILE handle that will store the result
    :returns: an SQRESULT

    retrieve the pointer of a stream handle from an arbitrary
    position in the stack.

