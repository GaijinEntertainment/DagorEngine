
.. _stdlib_uriparser_boost:

================================
Boost package for the URI parser
================================

.. include:: detail/uriparser_boost.rst

The uriparser_boost module implements additional infrastructure for the URI parser.

All functions and symbols are in "uriparser_boost" module, use require to get access to it. ::

    require daslib/uriparser_boost

+++++++++++++++++
Split and compose
+++++++++++++++++

  *  :ref:`uri_split_full_path (uri:uriparser::Uri const implicit) : array\<string\> <function-_at_uriparser_boost_c__c_uri_split_full_path_CIH_ls_uriparser_c__c_Uri_gr_>` 
  *  :ref:`uri_compose_query (query:table\<string;string\> const) : string <function-_at_uriparser_boost_c__c_uri_compose_query_C1_ls_s_gr_2_ls_s_gr_T>` 
  *  :ref:`uri_compose_query_in_order (query:table\<string;string\> const) : string const <function-_at_uriparser_boost_c__c_uri_compose_query_in_order_C1_ls_s_gr_2_ls_s_gr_T>` 
  *  :ref:`uri_compose (scheme:string const;userInfo:string const;hostText:string const;portText:string const;path:string const;query:string const;fragment:string const) : uriparser::Uri <function-_at_uriparser_boost_c__c_uri_compose_Cs_Cs_Cs_Cs_Cs_Cs_Cs>` 

.. _function-_at_uriparser_boost_c__c_uri_split_full_path_CIH_ls_uriparser_c__c_Uri_gr_:

.. das:function:: uri_split_full_path(uri: Uri const implicit)

uri_split_full_path returns array<string>

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+uri     + :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+


Split the full path of a URI into its components.

.. _function-_at_uriparser_boost_c__c_uri_compose_query_C1_ls_s_gr_2_ls_s_gr_T:

.. das:function:: uri_compose_query(query: table<string;string> const)

uri_compose_query returns string

+--------+--------------------------+
+argument+argument type             +
+========+==========================+
+query   +table<string;string> const+
+--------+--------------------------+


Compose a query string from a table of key-value pairs.

.. _function-_at_uriparser_boost_c__c_uri_compose_query_in_order_C1_ls_s_gr_2_ls_s_gr_T:

.. das:function:: uri_compose_query_in_order(query: table<string;string> const)

uri_compose_query_in_order returns string const

+--------+--------------------------+
+argument+argument type             +
+========+==========================+
+query   +table<string;string> const+
+--------+--------------------------+


Compose a query string from a table of key-value pairs, in the sorted order.

.. _function-_at_uriparser_boost_c__c_uri_compose_Cs_Cs_Cs_Cs_Cs_Cs_Cs:

.. das:function:: uri_compose(scheme: string const; userInfo: string const; hostText: string const; portText: string const; path: string const; query: string const; fragment: string const)

uri_compose returns  :ref:`uriparser::Uri <handle-uriparser-Uri>` 

+--------+-------------+
+argument+argument type+
+========+=============+
+scheme  +string const +
+--------+-------------+
+userInfo+string const +
+--------+-------------+
+hostText+string const +
+--------+-------------+
+portText+string const +
+--------+-------------+
+path    +string const +
+--------+-------------+
+query   +string const +
+--------+-------------+
+fragment+string const +
+--------+-------------+


Compose a URI from its components.

+++++++++++++++++++
Component accessors
+++++++++++++++++++

  *  :ref:`scheme (uri:uriparser::Uri const implicit) : string <function-_at_uriparser_boost_c__c_scheme_CIH_ls_uriparser_c__c_Uri_gr_>` 
  *  :ref:`user_info (uri:uriparser::Uri const implicit) : string <function-_at_uriparser_boost_c__c_user_info_CIH_ls_uriparser_c__c_Uri_gr_>` 
  *  :ref:`host (uri:uriparser::Uri const implicit) : string <function-_at_uriparser_boost_c__c_host_CIH_ls_uriparser_c__c_Uri_gr_>` 
  *  :ref:`port (uri:uriparser::Uri const implicit) : string <function-_at_uriparser_boost_c__c_port_CIH_ls_uriparser_c__c_Uri_gr_>` 
  *  :ref:`path (uri:uriparser::Uri const implicit) : string <function-_at_uriparser_boost_c__c_path_CIH_ls_uriparser_c__c_Uri_gr_>` 
  *  :ref:`query (uri:uriparser::Uri const implicit) : string <function-_at_uriparser_boost_c__c_query_CIH_ls_uriparser_c__c_Uri_gr_>` 
  *  :ref:`fragment (uri:uriparser::Uri const implicit) : string <function-_at_uriparser_boost_c__c_fragment_CIH_ls_uriparser_c__c_Uri_gr_>` 

.. _function-_at_uriparser_boost_c__c_scheme_CIH_ls_uriparser_c__c_Uri_gr_:

.. das:function:: scheme(uri: Uri const implicit)

scheme returns string

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+uri     + :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+


Returns the scheme of a URI.

.. _function-_at_uriparser_boost_c__c_user_info_CIH_ls_uriparser_c__c_Uri_gr_:

.. das:function:: user_info(uri: Uri const implicit)

user_info returns string

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+uri     + :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+


Return the user info of a URI.

.. _function-_at_uriparser_boost_c__c_host_CIH_ls_uriparser_c__c_Uri_gr_:

.. das:function:: host(uri: Uri const implicit)

host returns string

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+uri     + :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+


Return the host of a URI.

.. _function-_at_uriparser_boost_c__c_port_CIH_ls_uriparser_c__c_Uri_gr_:

.. das:function:: port(uri: Uri const implicit)

port returns string

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+uri     + :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+


Return the port of a URI.

.. _function-_at_uriparser_boost_c__c_path_CIH_ls_uriparser_c__c_Uri_gr_:

.. das:function:: path(uri: Uri const implicit)

path returns string

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+uri     + :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+


Return the path of a URI.

.. _function-_at_uriparser_boost_c__c_query_CIH_ls_uriparser_c__c_Uri_gr_:

.. das:function:: query(uri: Uri const implicit)

query returns string

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+uri     + :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+


Return the query of a URI.

.. _function-_at_uriparser_boost_c__c_fragment_CIH_ls_uriparser_c__c_Uri_gr_:

.. das:function:: fragment(uri: Uri const implicit)

fragment returns string

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+uri     + :ref:`uriparser::Uri <handle-uriparser-Uri>`  const implicit+
+--------+-------------------------------------------------------------+


Return the fragment of a URI.


