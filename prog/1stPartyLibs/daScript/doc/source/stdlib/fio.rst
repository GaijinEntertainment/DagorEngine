
.. _stdlib_fio:

=========================
File input output library
=========================

.. include:: detail/fio.rst

The FIO module exposes C++ FILE * API, file mapping,  directory and file stat manipulation routines to daScript.

All functions and symbols are in "fio" module, use require to get access to it. ::

    require fio


++++++++++++
Type aliases
++++++++++++

.. _alias-file:

.. das:attribute:: file = fio::FILE const?

|typedef-fio-file|

+++++++++
Constants
+++++++++

.. _global-fio-seek_set:

.. das:attribute:: seek_set = 0

|variable-fio-seek_set|

.. _global-fio-seek_cur:

.. das:attribute:: seek_cur = 1

|variable-fio-seek_cur|

.. _global-fio-seek_end:

.. das:attribute:: seek_end = 2

|variable-fio-seek_end|

.. _global-fio-df_magic:

.. das:attribute:: df_magic = 0x12345678

|variable-fio-df_magic|

.. _struct-fio-df_header:

.. das:attribute:: df_header



df_header fields are

+-----+----+
+magic+uint+
+-----+----+
+size +int +
+-----+----+


|structure-fio-df_header|

++++++++++++++++++
Handled structures
++++++++++++++++++

.. _handle-fio-FStat:

.. das:attribute:: FStat

FStat fields are

+--------+----+
+is_valid+bool+
+--------+----+


FStat property operators are

+------+----------------------------------------------+
+size  +uint64                                        +
+------+----------------------------------------------+
+atime + :ref:`builtin::clock <handle-builtin-clock>` +
+------+----------------------------------------------+
+ctime + :ref:`builtin::clock <handle-builtin-clock>` +
+------+----------------------------------------------+
+mtime + :ref:`builtin::clock <handle-builtin-clock>` +
+------+----------------------------------------------+
+is_reg+bool                                          +
+------+----------------------------------------------+
+is_dir+bool                                          +
+------+----------------------------------------------+


|structure_annotation-fio-FStat|

+++++++++++++
Handled types
+++++++++++++

.. _handle-fio-FILE:

.. das:attribute:: FILE

|any_annotation-fio-FILE|

+++++++++++++++++
File manipulation
+++++++++++++++++

  *  :ref:`remove (name:string const implicit) : bool <function-_at_fio_c__c_remove_CIs>` 
  *  :ref:`fopen (name:string const implicit;mode:string const implicit) : fio::FILE const? const <function-_at_fio_c__c_fopen_CIs_CIs>` 
  *  :ref:`fclose (file:fio::FILE const? const implicit;context:__context const;line:__lineInfo const) : void <function-_at_fio_c__c_fclose_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_C_c_C_l>` 
  *  :ref:`fflush (file:fio::FILE const? const implicit;context:__context const;line:__lineInfo const) : void <function-_at_fio_c__c_fflush_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_C_c_C_l>` 
  *  :ref:`fprint (file:fio::FILE const? const implicit;text:string const implicit;context:__context const;line:__lineInfo const) : void <function-_at_fio_c__c_fprint_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CIs_C_c_C_l>` 
  *  :ref:`fread (file:fio::FILE const? const implicit;context:__context const;line:__lineInfo const) : string <function-_at_fio_c__c_fread_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_C_c_C_l>` 
  *  :ref:`fmap (file:fio::FILE const? const implicit;block:block\<(var arg0:array\<uint8\>#):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_fio_c__c_fmap_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CI0_ls__hh_1_ls_u8_gr_A_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`fgets (file:fio::FILE const? const implicit;context:__context const;line:__lineInfo const) : string <function-_at_fio_c__c_fgets_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_C_c_C_l>` 
  *  :ref:`fwrite (file:fio::FILE const? const implicit;text:string const implicit;context:__context const;line:__lineInfo const) : void <function-_at_fio_c__c_fwrite_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CIs_C_c_C_l>` 
  *  :ref:`feof (file:fio::FILE const? const implicit) : bool <function-_at_fio_c__c_feof_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?>` 
  *  :ref:`fseek (file:fio::FILE const? const implicit;offset:int64 const;mode:int const;context:__context const;line:__lineInfo const) : int64 <function-_at_fio_c__c_fseek_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_Ci64_Ci_C_c_C_l>` 
  *  :ref:`ftell (file:fio::FILE const? const implicit;context:__context const;line:__lineInfo const) : int64 <function-_at_fio_c__c_ftell_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_C_c_C_l>` 
  *  :ref:`fstat (file:fio::FILE const? const implicit;stat:fio::FStat implicit;context:__context const;line:__lineInfo const) : bool <function-_at_fio_c__c_fstat_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_IH_ls_fio_c__c_FStat_gr__C_c_C_l>` 
  *  :ref:`stat (file:string const implicit;stat:fio::FStat implicit) : bool <function-_at_fio_c__c_stat_CIs_IH_ls_fio_c__c_FStat_gr_>` 
  *  :ref:`fstdin () : fio::FILE const? const <function-_at_fio_c__c_fstdin>` 
  *  :ref:`fstdout () : fio::FILE const? const <function-_at_fio_c__c_fstdout>` 
  *  :ref:`fstderr () : fio::FILE const? const <function-_at_fio_c__c_fstderr>` 
  *  :ref:`getchar () : int <function-_at_fio_c__c_getchar>` 
  *  :ref:`fload (file:fio::FILE const? const;size:int const;blk:block\<(data:array\<uint8\> const):void\> const) : void <function-_at_fio_c__c_fload_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_Ci_CN_ls_data_gr_0_ls_C1_ls_u8_gr_A_gr_1_ls_v_gr__builtin_>` 
  *  :ref:`fopen (name:string const;mode:string const;blk:block\<(f:fio::FILE const? const):void\> const) : auto <function-_at_fio_c__c_fopen_Cs_Cs_CN_ls_f_gr_0_ls_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_gr_1_ls_v_gr__builtin_>` 
  *  :ref:`stat (path:string const) : fio::FStat <function-_at_fio_c__c_stat_Cs>` 
  *  :ref:`fstat (f:fio::FILE const? const) : fio::FStat <function-_at_fio_c__c_fstat_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?>` 
  *  :ref:`fread (f:fio::FILE const? const;blk:block\<(data:string const#):auto\> const) : auto <function-_at_fio_c__c_fread_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CN_ls_data_gr_0_ls_C_hh_s_gr_1_ls_._gr__builtin_>` 
  *  :ref:`fload (f:fio::FILE const? const;buf:auto(BufType) const -const) : auto <function-_at_fio_c__c_fload_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CY_ls_BufType_gr_.>` 
  *  :ref:`fsave (f:fio::FILE const? const;buf:auto(BufType) const) : auto <function-_at_fio_c__c_fsave_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CY_ls_BufType_gr_.>` 
  *  :ref:`fread (f:fio::FILE const? const;buf:auto(BufType) const implicit) : auto <function-_at_fio_c__c_fread_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CIY_ls_BufType_gr_.>` 
  *  :ref:`fread (f:fio::FILE const? const;buf:array\<auto(BufType)\> const implicit) : auto <function-_at_fio_c__c_fread_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CI1_ls_Y_ls_BufType_gr_._gr_A>` 
  *  :ref:`fwrite (f:fio::FILE const? const;buf:auto(BufType) const implicit) : auto <function-_at_fio_c__c_fwrite_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CIY_ls_BufType_gr_.>` 
  *  :ref:`fwrite (f:fio::FILE const? const;buf:array\<auto(BufType)\> const implicit) : auto <function-_at_fio_c__c_fwrite_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CI1_ls_Y_ls_BufType_gr_._gr_A>` 

.. _function-_at_fio_c__c_remove_CIs:

.. das:function:: remove(name: string const implicit)

remove returns bool

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+name    +string const implicit+
+--------+---------------------+


|function-fio-remove|

.. _function-_at_fio_c__c_fopen_CIs_CIs:

.. das:function:: fopen(name: string const implicit; mode: string const implicit)

fopen returns  :ref:`fio::FILE <handle-fio-FILE>`  const? const

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+name    +string const implicit+
+--------+---------------------+
+mode    +string const implicit+
+--------+---------------------+


|function-fio-fopen|

.. _function-_at_fio_c__c_fclose_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_C_c_C_l:

.. das:function:: fclose(file: fio::FILE const? const implicit)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+file    + :ref:`fio::FILE <handle-fio-FILE>`  const? const implicit+
+--------+----------------------------------------------------------+


|function-fio-fclose|

.. _function-_at_fio_c__c_fflush_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_C_c_C_l:

.. das:function:: fflush(file: fio::FILE const? const implicit)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+file    + :ref:`fio::FILE <handle-fio-FILE>`  const? const implicit+
+--------+----------------------------------------------------------+


|function-fio-fflush|

.. _function-_at_fio_c__c_fprint_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CIs_C_c_C_l:

.. das:function:: fprint(file: fio::FILE const? const implicit; text: string const implicit)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+file    + :ref:`fio::FILE <handle-fio-FILE>`  const? const implicit+
+--------+----------------------------------------------------------+
+text    +string const implicit                                     +
+--------+----------------------------------------------------------+


|function-fio-fprint|

.. _function-_at_fio_c__c_fread_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_C_c_C_l:

.. das:function:: fread(file: fio::FILE const? const implicit)

fread returns string

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+file    + :ref:`fio::FILE <handle-fio-FILE>`  const? const implicit+
+--------+----------------------------------------------------------+


|function-fio-fread|

.. _function-_at_fio_c__c_fmap_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CI0_ls__hh_1_ls_u8_gr_A_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: fmap(file: fio::FILE const? const implicit; block: block<(var arg0:array<uint8>#):void> const implicit)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+file    + :ref:`fio::FILE <handle-fio-FILE>`  const? const implicit+
+--------+----------------------------------------------------------+
+block   +block<(array<uint8>#):void> const implicit                +
+--------+----------------------------------------------------------+


|function-fio-fmap|

.. _function-_at_fio_c__c_fgets_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_C_c_C_l:

.. das:function:: fgets(file: fio::FILE const? const implicit)

fgets returns string

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+file    + :ref:`fio::FILE <handle-fio-FILE>`  const? const implicit+
+--------+----------------------------------------------------------+


|function-fio-fgets|

.. _function-_at_fio_c__c_fwrite_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CIs_C_c_C_l:

.. das:function:: fwrite(file: fio::FILE const? const implicit; text: string const implicit)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+file    + :ref:`fio::FILE <handle-fio-FILE>`  const? const implicit+
+--------+----------------------------------------------------------+
+text    +string const implicit                                     +
+--------+----------------------------------------------------------+


|function-fio-fwrite|

.. _function-_at_fio_c__c_feof_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?:

.. das:function:: feof(file: fio::FILE const? const implicit)

feof returns bool

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+file    + :ref:`fio::FILE <handle-fio-FILE>`  const? const implicit+
+--------+----------------------------------------------------------+


|function-fio-feof|

.. _function-_at_fio_c__c_fseek_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_Ci64_Ci_C_c_C_l:

.. das:function:: fseek(file: fio::FILE const? const implicit; offset: int64 const; mode: int const)

fseek returns int64

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+file    + :ref:`fio::FILE <handle-fio-FILE>`  const? const implicit+
+--------+----------------------------------------------------------+
+offset  +int64 const                                               +
+--------+----------------------------------------------------------+
+mode    +int const                                                 +
+--------+----------------------------------------------------------+


|function-fio-fseek|

.. _function-_at_fio_c__c_ftell_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_C_c_C_l:

.. das:function:: ftell(file: fio::FILE const? const implicit)

ftell returns int64

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+file    + :ref:`fio::FILE <handle-fio-FILE>`  const? const implicit+
+--------+----------------------------------------------------------+


|function-fio-ftell|

.. _function-_at_fio_c__c_fstat_CI1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_IH_ls_fio_c__c_FStat_gr__C_c_C_l:

.. das:function:: fstat(file: fio::FILE const? const implicit; stat: FStat implicit)

fstat returns bool

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+file    + :ref:`fio::FILE <handle-fio-FILE>`  const? const implicit+
+--------+----------------------------------------------------------+
+stat    + :ref:`fio::FStat <handle-fio-FStat>`  implicit           +
+--------+----------------------------------------------------------+


|function-fio-fstat|

.. _function-_at_fio_c__c_stat_CIs_IH_ls_fio_c__c_FStat_gr_:

.. das:function:: stat(file: string const implicit; stat: FStat implicit)

stat returns bool

+--------+-----------------------------------------------+
+argument+argument type                                  +
+========+===============================================+
+file    +string const implicit                          +
+--------+-----------------------------------------------+
+stat    + :ref:`fio::FStat <handle-fio-FStat>`  implicit+
+--------+-----------------------------------------------+


|function-fio-stat|

.. _function-_at_fio_c__c_fstdin:

.. das:function:: fstdin()

fstdin returns  :ref:`fio::FILE <handle-fio-FILE>`  const? const

|function-fio-fstdin|

.. _function-_at_fio_c__c_fstdout:

.. das:function:: fstdout()

fstdout returns  :ref:`fio::FILE <handle-fio-FILE>`  const? const

|function-fio-fstdout|

.. _function-_at_fio_c__c_fstderr:

.. das:function:: fstderr()

fstderr returns  :ref:`fio::FILE <handle-fio-FILE>`  const? const

|function-fio-fstderr|

.. _function-_at_fio_c__c_getchar:

.. das:function:: getchar()

getchar returns int

|function-fio-getchar|

.. _function-_at_fio_c__c_fload_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_Ci_CN_ls_data_gr_0_ls_C1_ls_u8_gr_A_gr_1_ls_v_gr__builtin_:

.. das:function:: fload(file: file; size: int const; blk: block<(data:array<uint8> const):void> const)

+--------+-------------------------------------------+
+argument+argument type                              +
+========+===========================================+
+file    + :ref:`file <alias-file>`                  +
+--------+-------------------------------------------+
+size    +int const                                  +
+--------+-------------------------------------------+
+blk     +block<(data:array<uint8> const):void> const+
+--------+-------------------------------------------+


|function-fio-fload|

.. _function-_at_fio_c__c_fopen_Cs_Cs_CN_ls_f_gr_0_ls_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_gr_1_ls_v_gr__builtin_:

.. das:function:: fopen(name: string const; mode: string const; blk: block<(f:fio::FILE const? const):void> const)

fopen returns auto

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+name    +string const                                    +
+--------+------------------------------------------------+
+mode    +string const                                    +
+--------+------------------------------------------------+
+blk     +block<(f: :ref:`file <alias-file>` ):void> const+
+--------+------------------------------------------------+


|function-fio-fopen|

.. _function-_at_fio_c__c_stat_Cs:

.. das:function:: stat(path: string const)

stat returns  :ref:`fio::FStat <handle-fio-FStat>` 

+--------+-------------+
+argument+argument type+
+========+=============+
+path    +string const +
+--------+-------------+


|function-fio-stat|

.. _function-_at_fio_c__c_fstat_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?:

.. das:function:: fstat(f: file)

fstat returns  :ref:`fio::FStat <handle-fio-FStat>` 

+--------+--------------------------+
+argument+argument type             +
+========+==========================+
+f       + :ref:`file <alias-file>` +
+--------+--------------------------+


|function-fio-fstat|

.. _function-_at_fio_c__c_fread_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CN_ls_data_gr_0_ls_C_hh_s_gr_1_ls_._gr__builtin_:

.. das:function:: fread(f: file; blk: block<(data:string const#):auto> const)

fread returns auto

+--------+--------------------------------------+
+argument+argument type                         +
+========+======================================+
+f       + :ref:`file <alias-file>`             +
+--------+--------------------------------------+
+blk     +block<(data:string const#):auto> const+
+--------+--------------------------------------+


|function-fio-fread|

.. _function-_at_fio_c__c_fload_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CY_ls_BufType_gr_.:

.. das:function:: fload(f: file; buf: auto(BufType) const)

fload returns auto

+--------+--------------------------+
+argument+argument type             +
+========+==========================+
+f       + :ref:`file <alias-file>` +
+--------+--------------------------+
+buf     +auto(BufType) const       +
+--------+--------------------------+


|function-fio-fload|

.. _function-_at_fio_c__c_fsave_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CY_ls_BufType_gr_.:

.. das:function:: fsave(f: file; buf: auto(BufType) const)

fsave returns auto

+--------+--------------------------+
+argument+argument type             +
+========+==========================+
+f       + :ref:`file <alias-file>` +
+--------+--------------------------+
+buf     +auto(BufType) const       +
+--------+--------------------------+


|function-fio-fsave|

.. _function-_at_fio_c__c_fread_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CIY_ls_BufType_gr_.:

.. das:function:: fread(f: file; buf: auto(BufType) const implicit)

fread returns auto

+--------+----------------------------+
+argument+argument type               +
+========+============================+
+f       + :ref:`file <alias-file>`   +
+--------+----------------------------+
+buf     +auto(BufType) const implicit+
+--------+----------------------------+


|function-fio-fread|

.. _function-_at_fio_c__c_fread_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CI1_ls_Y_ls_BufType_gr_._gr_A:

.. das:function:: fread(f: file; buf: array<auto(BufType)> const implicit)

fread returns auto

+--------+-----------------------------------+
+argument+argument type                      +
+========+===================================+
+f       + :ref:`file <alias-file>`          +
+--------+-----------------------------------+
+buf     +array<auto(BufType)> const implicit+
+--------+-----------------------------------+


|function-fio-fread|

.. _function-_at_fio_c__c_fwrite_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CIY_ls_BufType_gr_.:

.. das:function:: fwrite(f: file; buf: auto(BufType) const implicit)

fwrite returns auto

+--------+----------------------------+
+argument+argument type               +
+========+============================+
+f       + :ref:`file <alias-file>`   +
+--------+----------------------------+
+buf     +auto(BufType) const implicit+
+--------+----------------------------+


|function-fio-fwrite|

.. _function-_at_fio_c__c_fwrite_CY_ls_file_gr_1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_CI1_ls_Y_ls_BufType_gr_._gr_A:

.. das:function:: fwrite(f: file; buf: array<auto(BufType)> const implicit)

fwrite returns auto

+--------+-----------------------------------+
+argument+argument type                      +
+========+===================================+
+f       + :ref:`file <alias-file>`          +
+--------+-----------------------------------+
+buf     +array<auto(BufType)> const implicit+
+--------+-----------------------------------+


|function-fio-fwrite|

+++++++++++++++++
Path manipulation
+++++++++++++++++

  *  :ref:`dir_name (name:string const implicit;context:__context const;line:__lineInfo const) : string <function-_at_fio_c__c_dir_name_CIs_C_c_C_l>` 
  *  :ref:`base_name (name:string const implicit;context:__context const;line:__lineInfo const) : string <function-_at_fio_c__c_base_name_CIs_C_c_C_l>` 
  *  :ref:`get_full_file_name (path:string const implicit;context:__context const;at:__lineInfo const) : string <function-_at_fio_c__c_get_full_file_name_CIs_C_c_C_l>` 

.. _function-_at_fio_c__c_dir_name_CIs_C_c_C_l:

.. das:function:: dir_name(name: string const implicit)

dir_name returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+name    +string const implicit+
+--------+---------------------+


|function-fio-dir_name|

.. _function-_at_fio_c__c_base_name_CIs_C_c_C_l:

.. das:function:: base_name(name: string const implicit)

base_name returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+name    +string const implicit+
+--------+---------------------+


|function-fio-base_name|

.. _function-_at_fio_c__c_get_full_file_name_CIs_C_c_C_l:

.. das:function:: get_full_file_name(path: string const implicit)

get_full_file_name returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+path    +string const implicit+
+--------+---------------------+


|function-fio-get_full_file_name|

++++++++++++++++++++++
Directory manipulation
++++++++++++++++++++++

  *  :ref:`mkdir (path:string const implicit) : bool <function-_at_fio_c__c_mkdir_CIs>` 
  *  :ref:`dir (path:string const;blk:block\<(filename:string const):void\> const) : auto <function-_at_fio_c__c_dir_Cs_CN_ls_filename_gr_0_ls_Cs_gr_1_ls_v_gr__builtin_>` 

.. _function-_at_fio_c__c_mkdir_CIs:

.. das:function:: mkdir(path: string const implicit)

mkdir returns bool

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+path    +string const implicit+
+--------+---------------------+


|function-fio-mkdir|

.. _function-_at_fio_c__c_dir_Cs_CN_ls_filename_gr_0_ls_Cs_gr_1_ls_v_gr__builtin_:

.. das:function:: dir(path: string const; blk: block<(filename:string const):void> const)

dir returns auto

+--------+-----------------------------------------+
+argument+argument type                            +
+========+=========================================+
+path    +string const                             +
+--------+-----------------------------------------+
+blk     +block<(filename:string const):void> const+
+--------+-----------------------------------------+


|function-fio-dir|

++++++++++++++++++++
OS specific routines
++++++++++++++++++++

  *  :ref:`sleep (msec:uint const) : void <function-_at_fio_c__c_sleep_Cu>` 
  *  :ref:`exit (exitCode:int const) : void <function-_at_fio_c__c_exit_Ci>` 
  *  :ref:`popen (command:string const implicit;scope:block\<(arg0:fio::FILE const? const):void\> const implicit;context:__context const;at:__lineInfo const) : int <function-_at_fio_c__c_popen_CIs_CI0_ls_C1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`popen_binary (command:string const implicit;scope:block\<(arg0:fio::FILE const? const):void\> const implicit;context:__context const;at:__lineInfo const) : int <function-_at_fio_c__c_popen_binary_CIs_CI0_ls_C1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`get_env_variable (var:string const implicit;context:__context const) : string <function-_at_fio_c__c_get_env_variable_CIs_C_c>` 
  *  :ref:`sanitize_command_line (var:string const implicit;context:__context const;at:__lineInfo const) : string <function-_at_fio_c__c_sanitize_command_line_CIs_C_c_C_l>` 

.. _function-_at_fio_c__c_sleep_Cu:

.. das:function:: sleep(msec: uint const)

+--------+-------------+
+argument+argument type+
+========+=============+
+msec    +uint const   +
+--------+-------------+


|function-fio-sleep|

.. _function-_at_fio_c__c_exit_Ci:

.. das:function:: exit(exitCode: int const)

.. warning:: 
  This is unsafe operation.

+--------+-------------+
+argument+argument type+
+========+=============+
+exitCode+int const    +
+--------+-------------+


|function-fio-exit|

.. _function-_at_fio_c__c_popen_CIs_CI0_ls_C1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: popen(command: string const implicit; scope: block<(arg0:fio::FILE const? const):void> const implicit)

popen returns int

.. warning:: 
  This is unsafe operation.

+--------+------------------------------------------------------------------------------+
+argument+argument type                                                                 +
+========+==============================================================================+
+command +string const implicit                                                         +
+--------+------------------------------------------------------------------------------+
+scope   +block<( :ref:`fio::FILE <handle-fio-FILE>`  const? const):void> const implicit+
+--------+------------------------------------------------------------------------------+


|function-fio-popen|

.. _function-_at_fio_c__c_popen_binary_CIs_CI0_ls_C1_ls_CH_ls_fio_c__c_FILE_gr__gr_?_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: popen_binary(command: string const implicit; scope: block<(arg0:fio::FILE const? const):void> const implicit)

popen_binary returns int

.. warning:: 
  This is unsafe operation.

+--------+------------------------------------------------------------------------------+
+argument+argument type                                                                 +
+========+==============================================================================+
+command +string const implicit                                                         +
+--------+------------------------------------------------------------------------------+
+scope   +block<( :ref:`fio::FILE <handle-fio-FILE>`  const? const):void> const implicit+
+--------+------------------------------------------------------------------------------+


|function-fio-popen_binary|

.. _function-_at_fio_c__c_get_env_variable_CIs_C_c:

.. das:function:: get_env_variable(var: string const implicit)

get_env_variable returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+var     +string const implicit+
+--------+---------------------+


|function-fio-get_env_variable|

.. _function-_at_fio_c__c_sanitize_command_line_CIs_C_c_C_l:

.. das:function:: sanitize_command_line(var: string const implicit)

sanitize_command_line returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+var     +string const implicit+
+--------+---------------------+


|function-fio-sanitize_command_line|


