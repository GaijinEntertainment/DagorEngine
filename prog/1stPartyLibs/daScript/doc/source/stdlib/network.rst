
.. _stdlib_network:

======================
Network socket library
======================

.. include:: detail/network.rst

The NETWORK module implements basic TCP socket listening server (currently only one connection).
It would eventually be expanded to support client as well.

It its present form its used in Daslang Visual Studio Code plugin and upcoming debug server.

All functions and symbols are in "network" module, use require to get access to it. ::

    require network


++++++++++++++++++
Handled structures
++++++++++++++++++

.. _handle-network-NetworkServer:

.. das:attribute:: NetworkServer

|structure_annotation-network-NetworkServer|

+++++++
Classes
+++++++

.. _struct-network-Server:

.. das:attribute:: Server

|class-network-Server|

it defines as follows

  | _server : smart_ptr< :ref:`network::NetworkServer <handle-network-NetworkServer>` >

.. das:function:: Server.make_server_adapter(self: Server)

|method-network-Server.make_server_adapter|

.. das:function:: Server.init(self: Server; port: int const)

init returns bool

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`network::Server <struct-network-Server>` +
+--------+------------------------------------------------+
+port    +int const                                       +
+--------+------------------------------------------------+


|method-network-Server.init|

.. das:function:: Server.restore(self: Server; shared_orphan: smart_ptr<network::NetworkServer>&)

+-------------+--------------------------------------------------------------------------+
+argument     +argument type                                                             +
+=============+==========================================================================+
+self         + :ref:`network::Server <struct-network-Server>`                           +
+-------------+--------------------------------------------------------------------------+
+shared_orphan+smart_ptr< :ref:`network::NetworkServer <handle-network-NetworkServer>` >&+
+-------------+--------------------------------------------------------------------------+


|method-network-Server.restore|

.. das:function:: Server.save(self: Server; shared_orphan: smart_ptr<network::NetworkServer>&)

+-------------+--------------------------------------------------------------------------+
+argument     +argument type                                                             +
+=============+==========================================================================+
+self         + :ref:`network::Server <struct-network-Server>`                           +
+-------------+--------------------------------------------------------------------------+
+shared_orphan+smart_ptr< :ref:`network::NetworkServer <handle-network-NetworkServer>` >&+
+-------------+--------------------------------------------------------------------------+


|method-network-Server.save|

.. das:function:: Server.has_session(self: Server)

has_session returns bool

|method-network-Server.has_session|

.. das:function:: Server.is_open(self: Server)

is_open returns bool

|method-network-Server.is_open|

.. das:function:: Server.is_connected(self: Server)

is_connected returns bool

|method-network-Server.is_connected|

.. das:function:: Server.tick(self: Server)

|method-network-Server.tick|

.. das:function:: Server.send(self: Server; data: uint8? const; size: int const)

send returns bool

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`network::Server <struct-network-Server>` +
+--------+------------------------------------------------+
+data    +uint8? const                                    +
+--------+------------------------------------------------+
+size    +int const                                       +
+--------+------------------------------------------------+


|method-network-Server.send|

.. das:function:: Server.onConnect(self: Server)

|method-network-Server.onConnect|

.. das:function:: Server.onDisconnect(self: Server)

|method-network-Server.onDisconnect|

.. das:function:: Server.onData(self: Server; buf: uint8? const; size: int const)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`network::Server <struct-network-Server>` +
+--------+------------------------------------------------+
+buf     +uint8? const                                    +
+--------+------------------------------------------------+
+size    +int const                                       +
+--------+------------------------------------------------+


|method-network-Server.onData|

.. das:function:: Server.onError(self: Server; msg: string const; code: int const)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`network::Server <struct-network-Server>` +
+--------+------------------------------------------------+
+msg     +string const                                    +
+--------+------------------------------------------------+
+code    +int const                                       +
+--------+------------------------------------------------+


|method-network-Server.onError|

.. das:function:: Server.onLog(self: Server; msg: string const)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`network::Server <struct-network-Server>` +
+--------+------------------------------------------------+
+msg     +string const                                    +
+--------+------------------------------------------------+


|method-network-Server.onLog|

++++++++++++++++++++++++++
Low lever NetworkServer IO
++++++++++++++++++++++++++

  *  :ref:`make_server (class:void? const implicit;info:rtti::StructInfo const? const implicit;context:__context const) : bool <function-_at_network_c__c_make_server_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c>`
  *  :ref:`server_init (server:smart_ptr\<network::NetworkServer\> const implicit;port:int const;context:__context const;at:__lineInfo const) : bool <function-_at_network_c__c_server_init_CI1_ls_H_ls_network_c__c_NetworkServer_gr__gr_?M_Ci_C_c_C_l>`
  *  :ref:`server_is_open (server:smart_ptr\<network::NetworkServer\> const implicit;context:__context const;at:__lineInfo const) : bool <function-_at_network_c__c_server_is_open_CI1_ls_H_ls_network_c__c_NetworkServer_gr__gr_?M_C_c_C_l>`
  *  :ref:`server_is_connected (server:smart_ptr\<network::NetworkServer\> const implicit;context:__context const;at:__lineInfo const) : bool <function-_at_network_c__c_server_is_connected_CI1_ls_H_ls_network_c__c_NetworkServer_gr__gr_?M_C_c_C_l>`
  *  :ref:`server_tick (server:smart_ptr\<network::NetworkServer\> const implicit;context:__context const;at:__lineInfo const) : void <function-_at_network_c__c_server_tick_CI1_ls_H_ls_network_c__c_NetworkServer_gr__gr_?M_C_c_C_l>`
  *  :ref:`server_send (server:smart_ptr\<network::NetworkServer\> const implicit;data:uint8? const implicit;size:int const;context:__context const;at:__lineInfo const) : bool <function-_at_network_c__c_server_send_CI1_ls_H_ls_network_c__c_NetworkServer_gr__gr_?M_CI1_ls_u8_gr_?_Ci_C_c_C_l>`
  *  :ref:`server_restore (server:smart_ptr\<network::NetworkServer\> const implicit;class:void? const implicit;info:rtti::StructInfo const? const implicit;context:__context const;at:__lineInfo const) : void <function-_at_network_c__c_server_restore_CI1_ls_H_ls_network_c__c_NetworkServer_gr__gr_?M_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c_C_l>`

.. _function-_at_network_c__c_make_server_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c:

.. das:function:: make_server(class: void? const implicit; info: rtti::StructInfo const? const implicit)

make_server returns bool

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+class   +void? const implicit                                                    +
+--------+------------------------------------------------------------------------+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit+
+--------+------------------------------------------------------------------------+


|function-network-make_server|

.. _function-_at_network_c__c_server_init_CI1_ls_H_ls_network_c__c_NetworkServer_gr__gr_?M_Ci_C_c_C_l:

.. das:function:: server_init(server: smart_ptr<network::NetworkServer> const implicit; port: int const)

server_init returns bool

+--------+----------------------------------------------------------------------------------------+
+argument+argument type                                                                           +
+========+========================================================================================+
+server  +smart_ptr< :ref:`network::NetworkServer <handle-network-NetworkServer>` > const implicit+
+--------+----------------------------------------------------------------------------------------+
+port    +int const                                                                               +
+--------+----------------------------------------------------------------------------------------+


|function-network-server_init|

.. _function-_at_network_c__c_server_is_open_CI1_ls_H_ls_network_c__c_NetworkServer_gr__gr_?M_C_c_C_l:

.. das:function:: server_is_open(server: smart_ptr<network::NetworkServer> const implicit)

server_is_open returns bool

+--------+----------------------------------------------------------------------------------------+
+argument+argument type                                                                           +
+========+========================================================================================+
+server  +smart_ptr< :ref:`network::NetworkServer <handle-network-NetworkServer>` > const implicit+
+--------+----------------------------------------------------------------------------------------+


|function-network-server_is_open|

.. _function-_at_network_c__c_server_is_connected_CI1_ls_H_ls_network_c__c_NetworkServer_gr__gr_?M_C_c_C_l:

.. das:function:: server_is_connected(server: smart_ptr<network::NetworkServer> const implicit)

server_is_connected returns bool

+--------+----------------------------------------------------------------------------------------+
+argument+argument type                                                                           +
+========+========================================================================================+
+server  +smart_ptr< :ref:`network::NetworkServer <handle-network-NetworkServer>` > const implicit+
+--------+----------------------------------------------------------------------------------------+


|function-network-server_is_connected|

.. _function-_at_network_c__c_server_tick_CI1_ls_H_ls_network_c__c_NetworkServer_gr__gr_?M_C_c_C_l:

.. das:function:: server_tick(server: smart_ptr<network::NetworkServer> const implicit)

+--------+----------------------------------------------------------------------------------------+
+argument+argument type                                                                           +
+========+========================================================================================+
+server  +smart_ptr< :ref:`network::NetworkServer <handle-network-NetworkServer>` > const implicit+
+--------+----------------------------------------------------------------------------------------+


|function-network-server_tick|

.. _function-_at_network_c__c_server_send_CI1_ls_H_ls_network_c__c_NetworkServer_gr__gr_?M_CI1_ls_u8_gr_?_Ci_C_c_C_l:

.. das:function:: server_send(server: smart_ptr<network::NetworkServer> const implicit; data: uint8? const implicit; size: int const)

server_send returns bool

+--------+----------------------------------------------------------------------------------------+
+argument+argument type                                                                           +
+========+========================================================================================+
+server  +smart_ptr< :ref:`network::NetworkServer <handle-network-NetworkServer>` > const implicit+
+--------+----------------------------------------------------------------------------------------+
+data    +uint8? const implicit                                                                   +
+--------+----------------------------------------------------------------------------------------+
+size    +int const                                                                               +
+--------+----------------------------------------------------------------------------------------+


|function-network-server_send|

.. _function-_at_network_c__c_server_restore_CI1_ls_H_ls_network_c__c_NetworkServer_gr__gr_?M_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c_C_l:

.. das:function:: server_restore(server: smart_ptr<network::NetworkServer> const implicit; class: void? const implicit; info: rtti::StructInfo const? const implicit)

+--------+----------------------------------------------------------------------------------------+
+argument+argument type                                                                           +
+========+========================================================================================+
+server  +smart_ptr< :ref:`network::NetworkServer <handle-network-NetworkServer>` > const implicit+
+--------+----------------------------------------------------------------------------------------+
+class   +void? const implicit                                                                    +
+--------+----------------------------------------------------------------------------------------+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit                +
+--------+----------------------------------------------------------------------------------------+


|function-network-server_restore|


