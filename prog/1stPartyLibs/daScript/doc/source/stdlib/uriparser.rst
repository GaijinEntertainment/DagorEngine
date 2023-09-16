
.. _stdlib_uriparser:

===========================================
URI manipulation library based on UriParser
===========================================

.. include:: detail/uriparser.rst

The URIPARSER module exposes uriParser library https://uriparser.github.io to daScript.

All functions and symbols are in "uriparser" module, use require to get access to it. ::

    require uriparser


++++++++++++++++++
Handled structures
++++++++++++++++++

.. _handle-uriparser-UriTextRangeA:

.. das:attribute:: UriTextRangeA

|structure_annotation-uriparser-UriTextRangeA|

.. _handle-uriparser-UriIp4Struct:

.. das:attribute:: UriIp4Struct

UriIp4Struct fields are

+----+--------+
+data+uint8[4]+
+----+--------+


|structure_annotation-uriparser-UriIp4Struct|

.. _handle-uriparser-UriIp6Struct:

.. das:attribute:: UriIp6Struct

UriIp6Struct fields are

+----+---------+
+data+uint8[16]+
+----+---------+


|structure_annotation-uriparser-UriIp6Struct|

.. _handle-uriparser-UriHostDataA:

.. das:attribute:: UriHostDataA

UriHostDataA fields are

+--------+------------------------------------------------------------------+
+ipFuture+ :ref:`uriparser::UriTextRangeA <handle-uriparser-UriTextRangeA>` +
+--------+------------------------------------------------------------------+
+ip4     + :ref:`uriparser::UriIp4Struct <handle-uriparser-UriIp4Struct>` ? +
+--------+------------------------------------------------------------------+
+ip6     + :ref:`uriparser::UriIp6Struct <handle-uriparser-UriIp6Struct>` ? +
+--------+------------------------------------------------------------------+


|structure_annotation-uriparser-UriHostDataA|

.. _handle-uriparser-UriPathSegmentStructA:

.. das:attribute:: UriPathSegmentStructA

UriPathSegmentStructA fields are

+----+-----------------------------------------------------------------------------------+
+next+ :ref:`uriparser::UriPathSegmentStructA <handle-uriparser-UriPathSegmentStructA>` ?+
+----+-----------------------------------------------------------------------------------+
+text+ :ref:`uriparser::UriTextRangeA <handle-uriparser-UriTextRangeA>`                  +
+----+-----------------------------------------------------------------------------------+


|structure_annotation-uriparser-UriPathSegmentStructA|

.. _handle-uriparser-UriUriA:

.. das:attribute:: UriUriA

UriUriA fields are

+------------+-----------------------------------------------------------------------------------+
+hostData    + :ref:`uriparser::UriHostDataA <handle-uriparser-UriHostDataA>`                    +
+------------+-----------------------------------------------------------------------------------+
+absolutePath+int                                                                                +
+------------+-----------------------------------------------------------------------------------+
+hostText    + :ref:`uriparser::UriTextRangeA <handle-uriparser-UriTextRangeA>`                  +
+------------+-----------------------------------------------------------------------------------+
+pathTail    + :ref:`uriparser::UriPathSegmentStructA <handle-uriparser-UriPathSegmentStructA>` ?+
+------------+-----------------------------------------------------------------------------------+
+pathHead    + :ref:`uriparser::UriPathSegmentStructA <handle-uriparser-UriPathSegmentStructA>` ?+
+------------+-----------------------------------------------------------------------------------+
+query       + :ref:`uriparser::UriTextRangeA <handle-uriparser-UriTextRangeA>`                  +
+------------+-----------------------------------------------------------------------------------+
+scheme      + :ref:`uriparser::UriTextRangeA <handle-uriparser-UriTextRangeA>`                  +
+------------+-----------------------------------------------------------------------------------+
+portText    + :ref:`uriparser::UriTextRangeA <handle-uriparser-UriTextRangeA>`                  +
+------------+-----------------------------------------------------------------------------------+
+fragment    + :ref:`uriparser::UriTextRangeA <handle-uriparser-UriTextRangeA>`                  +
+------------+-----------------------------------------------------------------------------------+
+owner       +int                                                                                +
+------------+-----------------------------------------------------------------------------------+
+userInfo    + :ref:`uriparser::UriTextRangeA <handle-uriparser-UriTextRangeA>`                  +
+------------+-----------------------------------------------------------------------------------+


|structure_annotation-uriparser-UriUriA|

.. _handle-uriparser-Uri:

.. das:attribute:: Uri

Uri fields are

+---+------------------------------------------------------+
+uri+ :ref:`uriparser::UriUriA <handle-uriparser-UriUriA>` +
+---+------------------------------------------------------+


Uri property operators are

+------+----+
+empty +bool+
+------+----+
+size  +int +
+------+----+
+status+int +
+------+----+


|structure_annotation-uriparser-Uri|

+++++++++++++++++++++++++++++++
Initialization and finalization
+++++++++++++++++++++++++++++++

  *  :ref:`Uri () : uriparser::Uri <function-_at_uriparser_c__c_Uri>` 
  *  :ref:`using (arg0:block\<(var arg0:uriparser::Uri# explicit):void\> const implicit) : void <function-_at_uriparser_c__c_using_CI0_ls__hh_XH_ls_uriparser_c__c_Uri_gr__gr_1_ls_v_gr__builtin_>` 
  *  :ref:`Uri (arg0:string const implicit) : uriparser::Uri <function-_at_uriparser_c__c_Uri_CIs>` 
  *  :ref:`using (arg0:string const implicit;arg1:block\<(var arg0:uriparser::Uri# explicit):void\> const implicit) : void <function-_at_uriparser_c__c_using_CIs_CI0_ls__hh_XH_ls_uriparser_c__c_Uri_gr__gr_1_ls_v_gr__builtin_>` 
  *  :ref:`finalize (uri:uriparser::Uri implicit) : void <function-_at_uriparser_c__c_finalize_IH_ls_uriparser_c__c_Uri_gr_>` 
  *  :ref:`clone (dest:uriparser::Uri implicit;src:uriparser::Uri const implicit) : void <function-_at_uriparser_c__c_clone_IH_ls_uriparser_c__c_Uri_gr__CIH_ls_uriparser_c__c_Uri_gr_>` 

.. _function-_at_uriparser_c__c_Uri:

.. das:function:: Uri()

Uri returns  :ref:`uriparser::Uri <handle-uriparser-Uri>` 

|function-uriparser-Uri|

.. _function-_at_uriparser_c__c_using_CI0_ls__hh_XH_ls_uriparser_c__c_Uri_gr__gr_1_ls_v_gr__builtin_:

.. das:function:: using(arg0: block<(var arg0:uriparser::Uri# explicit):void> const implicit)

+--------+----------------------------------------------------------------------------+
+argument+argument type                                                               +
+========+============================================================================+
+arg0    +block<( :ref:`uriparser::Uri <handle-uriparser-Uri>` #):void> const implicit+
+--------+----------------------------------------------------------------------------+


|function-uriparser-using|

.. _function-_at_uriparser_c__c_Uri_CIs:

.. das:function:: Uri(arg0: string const implicit)

Uri returns  :ref:`uriparser::Uri <handle-uriparser-Uri>` 

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+arg0    +string const implicit+
+--------+---------------------+


|function-uriparser-Uri|

.. _function-_at_uriparser_c__c_using_CIs_CI0_ls__hh_XH_ls_uriparser_c__c_Uri_gr__gr_1_ls_v_gr__builtin_:

.. das:function:: using(arg0: string const implicit; arg1: block<(var arg0:uriparser::Uri# explicit):void> const implicit)

+--------+----------------------------------------------------------------------------+
+argument+argument type                                                               +
+========+============================================================================+
+arg0    +string const implicit                                                       +
+--------+----------------------------------------------------------------------------+
+arg1    +block<( :ref:`uriparser::Uri <handle-uriparser-Uri>` #):void> const implicit+
+--------+----------------------------------------------------------------------------+


|function-uriparser-using|

.. _function-_at_uriparser_c__c_finalize_IH_ls_uriparser_c__c_Uri_gr_:

.. das:function:: finalize(uri: Uri implicit)

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+uri     + :ref:`uriparser::Uri <handle-uriparser-Uri>`  implicit+
+--------+-------------------------------------------------------+


|function-uriparser-finalize|

.. _function-_at_uriparser_c__c_clone_IH_ls_uriparser_c__c_Uri_gr__CIH_ls_uriparser_c__c_Uri_gr_:

.. das:function:: clone(dest: Uri implicit; src: Uri const implicit)

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+dest    + :ref:`uriparser::Uri <handle-uriparser-Uri>`  implicit      +
+--------+-------------------------------------------------------------+
+src     + :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+


|function-uriparser-clone|

+++++++++++++++++++
Escape and unescape
+++++++++++++++++++

  *  :ref:`escape_uri (uriStr:string const implicit;spaceToPlus:bool const;normalizeBreaks:bool const;context:__context const) : string <function-_at_uriparser_c__c_escape_uri_CIs_Cb_Cb_C_c>` 
  *  :ref:`unescape_uri (uriStr:string const implicit;context:__context const) : string <function-_at_uriparser_c__c_unescape_uri_CIs_C_c>` 

.. _function-_at_uriparser_c__c_escape_uri_CIs_Cb_Cb_C_c:

.. das:function:: escape_uri(uriStr: string const implicit; spaceToPlus: bool const; normalizeBreaks: bool const)

escape_uri returns string

+---------------+---------------------+
+argument       +argument type        +
+===============+=====================+
+uriStr         +string const implicit+
+---------------+---------------------+
+spaceToPlus    +bool const           +
+---------------+---------------------+
+normalizeBreaks+bool const           +
+---------------+---------------------+


|function-uriparser-escape_uri|

.. _function-_at_uriparser_c__c_unescape_uri_CIs_C_c:

.. das:function:: unescape_uri(uriStr: string const implicit)

unescape_uri returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+uriStr  +string const implicit+
+--------+---------------------+


|function-uriparser-unescape_uri|

+++++++++++++++++
Uri manipulations
+++++++++++++++++

  *  :ref:`strip_uri (uri:uriparser::Uri const implicit;query:bool const;fragment:bool const) : uriparser::Uri <function-_at_uriparser_c__c_strip_uri_CIH_ls_uriparser_c__c_Uri_gr__Cb_Cb>` 
  *  :ref:`add_base_uri (base:uriparser::Uri const implicit;relative:uriparser::Uri const implicit) : uriparser::Uri <function-_at_uriparser_c__c_add_base_uri_CIH_ls_uriparser_c__c_Uri_gr__CIH_ls_uriparser_c__c_Uri_gr_>` 
  *  :ref:`remove_base_uri (base:uriparser::Uri const implicit;relative:uriparser::Uri const implicit) : uriparser::Uri <function-_at_uriparser_c__c_remove_base_uri_CIH_ls_uriparser_c__c_Uri_gr__CIH_ls_uriparser_c__c_Uri_gr_>` 
  *  :ref:`normalize (uri:uriparser::Uri implicit) : bool <function-_at_uriparser_c__c_normalize_IH_ls_uriparser_c__c_Uri_gr_>` 
  *  :ref:`string (uri:uriparser::Uri const implicit;context:__context const) : string <function-_at_uriparser_c__c_string_CIH_ls_uriparser_c__c_Uri_gr__C_c>` 
  *  :ref:`string (range:uriparser::UriTextRangeA const implicit;context:__context const) : string <function-_at_uriparser_c__c_string_CIH_ls_uriparser_c__c_UriTextRangeA_gr__C_c>` 
  *  :ref:`uri_for_each_query_kv (uri:uriparser::Uri const implicit;block:block\<(var arg0:string#;var arg1:string#):void\> const implicit;context:__context const;lineinfo:__lineInfo const) : void <function-_at_uriparser_c__c_uri_for_each_query_kv_CIH_ls_uriparser_c__c_Uri_gr__CI0_ls__hh_s;_hh_s_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`normalize_uri (uriStr:string const implicit;context:__context const) : string <function-_at_uriparser_c__c_normalize_uri_CIs_C_c>` 

.. _function-_at_uriparser_c__c_strip_uri_CIH_ls_uriparser_c__c_Uri_gr__Cb_Cb:

.. das:function:: strip_uri(uri: Uri const implicit; query: bool const; fragment: bool const)

strip_uri returns  :ref:`uriparser::Uri <handle-uriparser-Uri>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+uri     + :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+
+query   +bool const                                                   +
+--------+-------------------------------------------------------------+
+fragment+bool const                                                   +
+--------+-------------------------------------------------------------+


|function-uriparser-strip_uri|

.. _function-_at_uriparser_c__c_add_base_uri_CIH_ls_uriparser_c__c_Uri_gr__CIH_ls_uriparser_c__c_Uri_gr_:

.. das:function:: add_base_uri(base: Uri const implicit; relative: Uri const implicit)

add_base_uri returns  :ref:`uriparser::Uri <handle-uriparser-Uri>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+base    + :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+
+relative+ :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+


|function-uriparser-add_base_uri|

.. _function-_at_uriparser_c__c_remove_base_uri_CIH_ls_uriparser_c__c_Uri_gr__CIH_ls_uriparser_c__c_Uri_gr_:

.. das:function:: remove_base_uri(base: Uri const implicit; relative: Uri const implicit)

remove_base_uri returns  :ref:`uriparser::Uri <handle-uriparser-Uri>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+base    + :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+
+relative+ :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+


|function-uriparser-remove_base_uri|

.. _function-_at_uriparser_c__c_normalize_IH_ls_uriparser_c__c_Uri_gr_:

.. das:function:: normalize(uri: Uri implicit)

normalize returns bool

+--------+-------------------------------------------------------+
+argument+argument type                                          +
+========+=======================================================+
+uri     + :ref:`uriparser::Uri <handle-uriparser-Uri>`  implicit+
+--------+-------------------------------------------------------+


|function-uriparser-normalize|

.. _function-_at_uriparser_c__c_string_CIH_ls_uriparser_c__c_Uri_gr__C_c:

.. das:function:: string(uri: Uri const implicit)

string returns string

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+uri     + :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+


|function-uriparser-string|

.. _function-_at_uriparser_c__c_string_CIH_ls_uriparser_c__c_UriTextRangeA_gr__C_c:

.. das:function:: string(range: UriTextRangeA const implicit)

string returns string

+--------+---------------------------------------------------------------------------------+
+argument+argument type                                                                    +
+========+=================================================================================+
+range   + :ref:`uriparser::UriTextRangeA <handle-uriparser-UriTextRangeA>`  const implicit+
+--------+---------------------------------------------------------------------------------+


|function-uriparser-string|

.. _function-_at_uriparser_c__c_uri_for_each_query_kv_CIH_ls_uriparser_c__c_Uri_gr__CI0_ls__hh_s;_hh_s_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: uri_for_each_query_kv(uri: Uri const implicit; block: block<(var arg0:string#;var arg1:string#):void> const implicit)

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+uri     + :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+
+block   +block<(string#;string#):void> const implicit                 +
+--------+-------------------------------------------------------------+


|function-uriparser-uri_for_each_query_kv|

.. _function-_at_uriparser_c__c_normalize_uri_CIs_C_c:

.. das:function:: normalize_uri(uriStr: string const implicit)

normalize_uri returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+uriStr  +string const implicit+
+--------+---------------------+


|function-uriparser-normalize_uri|

+++++++++++++++++++++
File name conversions
+++++++++++++++++++++

  *  :ref:`to_unix_file_name (uri:uriparser::Uri const implicit;context:__context const) : string <function-_at_uriparser_c__c_to_unix_file_name_CIH_ls_uriparser_c__c_Uri_gr__C_c>` 
  *  :ref:`to_windows_file_name (uri:uriparser::Uri const implicit;context:__context const) : string <function-_at_uriparser_c__c_to_windows_file_name_CIH_ls_uriparser_c__c_Uri_gr__C_c>` 
  *  :ref:`to_file_name (uri:uriparser::Uri const implicit;context:__context const) : string <function-_at_uriparser_c__c_to_file_name_CIH_ls_uriparser_c__c_Uri_gr__C_c>` 
  *  :ref:`uri_from_file_name (filename:string const implicit) : uriparser::Uri <function-_at_uriparser_c__c_uri_from_file_name_CIs>` 
  *  :ref:`uri_from_windows_file_name (filename:string const implicit) : uriparser::Uri <function-_at_uriparser_c__c_uri_from_windows_file_name_CIs>` 
  *  :ref:`uri_from_unix_file_name (filename:string const implicit) : uriparser::Uri <function-_at_uriparser_c__c_uri_from_unix_file_name_CIs>` 
  *  :ref:`uri_to_unix_file_name (uriStr:string const implicit;context:__context const) : string <function-_at_uriparser_c__c_uri_to_unix_file_name_CIs_C_c>` 
  *  :ref:`uri_to_windows_file_name (uriStr:string const implicit;context:__context const) : string <function-_at_uriparser_c__c_uri_to_windows_file_name_CIs_C_c>` 
  *  :ref:`unix_file_name_to_uri (uriStr:string const implicit;context:__context const) : string <function-_at_uriparser_c__c_unix_file_name_to_uri_CIs_C_c>` 
  *  :ref:`windows_file_name_to_uri (uriStr:string const implicit;context:__context const) : string <function-_at_uriparser_c__c_windows_file_name_to_uri_CIs_C_c>` 
  *  :ref:`uri_to_file_name (uriStr:string const implicit;context:__context const) : string <function-_at_uriparser_c__c_uri_to_file_name_CIs_C_c>` 
  *  :ref:`file_name_to_uri (uriStr:string const implicit;context:__context const) : string <function-_at_uriparser_c__c_file_name_to_uri_CIs_C_c>` 

.. _function-_at_uriparser_c__c_to_unix_file_name_CIH_ls_uriparser_c__c_Uri_gr__C_c:

.. das:function:: to_unix_file_name(uri: Uri const implicit)

to_unix_file_name returns string

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+uri     + :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+


|function-uriparser-to_unix_file_name|

.. _function-_at_uriparser_c__c_to_windows_file_name_CIH_ls_uriparser_c__c_Uri_gr__C_c:

.. das:function:: to_windows_file_name(uri: Uri const implicit)

to_windows_file_name returns string

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+uri     + :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+


|function-uriparser-to_windows_file_name|

.. _function-_at_uriparser_c__c_to_file_name_CIH_ls_uriparser_c__c_Uri_gr__C_c:

.. das:function:: to_file_name(uri: Uri const implicit)

to_file_name returns string

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+uri     + :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+


|function-uriparser-to_file_name|

.. _function-_at_uriparser_c__c_uri_from_file_name_CIs:

.. das:function:: uri_from_file_name(filename: string const implicit)

uri_from_file_name returns  :ref:`uriparser::Uri <handle-uriparser-Uri>` 

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+filename+string const implicit+
+--------+---------------------+


|function-uriparser-uri_from_file_name|

.. _function-_at_uriparser_c__c_uri_from_windows_file_name_CIs:

.. das:function:: uri_from_windows_file_name(filename: string const implicit)

uri_from_windows_file_name returns  :ref:`uriparser::Uri <handle-uriparser-Uri>` 

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+filename+string const implicit+
+--------+---------------------+


|function-uriparser-uri_from_windows_file_name|

.. _function-_at_uriparser_c__c_uri_from_unix_file_name_CIs:

.. das:function:: uri_from_unix_file_name(filename: string const implicit)

uri_from_unix_file_name returns  :ref:`uriparser::Uri <handle-uriparser-Uri>` 

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+filename+string const implicit+
+--------+---------------------+


|function-uriparser-uri_from_unix_file_name|

.. _function-_at_uriparser_c__c_uri_to_unix_file_name_CIs_C_c:

.. das:function:: uri_to_unix_file_name(uriStr: string const implicit)

uri_to_unix_file_name returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+uriStr  +string const implicit+
+--------+---------------------+


|function-uriparser-uri_to_unix_file_name|

.. _function-_at_uriparser_c__c_uri_to_windows_file_name_CIs_C_c:

.. das:function:: uri_to_windows_file_name(uriStr: string const implicit)

uri_to_windows_file_name returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+uriStr  +string const implicit+
+--------+---------------------+


|function-uriparser-uri_to_windows_file_name|

.. _function-_at_uriparser_c__c_unix_file_name_to_uri_CIs_C_c:

.. das:function:: unix_file_name_to_uri(uriStr: string const implicit)

unix_file_name_to_uri returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+uriStr  +string const implicit+
+--------+---------------------+


|function-uriparser-unix_file_name_to_uri|

.. _function-_at_uriparser_c__c_windows_file_name_to_uri_CIs_C_c:

.. das:function:: windows_file_name_to_uri(uriStr: string const implicit)

windows_file_name_to_uri returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+uriStr  +string const implicit+
+--------+---------------------+


|function-uriparser-windows_file_name_to_uri|

.. _function-_at_uriparser_c__c_uri_to_file_name_CIs_C_c:

.. das:function:: uri_to_file_name(uriStr: string const implicit)

uri_to_file_name returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+uriStr  +string const implicit+
+--------+---------------------+


|function-uriparser-uri_to_file_name|

.. _function-_at_uriparser_c__c_file_name_to_uri_CIs_C_c:

.. das:function:: file_name_to_uri(uriStr: string const implicit)

file_name_to_uri returns string

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+uriStr  +string const implicit+
+--------+---------------------+


|function-uriparser-file_name_to_uri|

++++
GUID
++++

  *  :ref:`make_new_guid (context:__context const;at:__lineInfo const) : string <function-_at_uriparser_c__c_make_new_guid_C_c_C_l>` 

.. _function-_at_uriparser_c__c_make_new_guid_C_c_C_l:

.. das:function:: make_new_guid()

make_new_guid returns string

|function-uriparser-make_new_guid|


