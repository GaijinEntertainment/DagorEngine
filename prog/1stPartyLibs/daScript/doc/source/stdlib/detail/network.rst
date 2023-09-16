.. |class-network-Server| replace:: Single socket listener combined with single socket connection.

.. |method-network-Server.init| replace:: Initializes server with specific port

.. |method-network-Server.restore| replace:: Restore server state from after the context switch.

.. |method-network-Server.save| replace:: Saves server to orphaned state to support context switching and live reloading. The idea is that server is saved to the orphaned state, which is not part of the context state.

.. |method-network-Server.has_session| replace:: Returns true if network session already exists. This is used to determine if the server should be initialized or not.

.. |method-network-Server.is_open| replace:: Returns true if server is listening to the port.

.. |method-network-Server.is_connected| replace:: Returns true if server is connected to the client.

.. |method-network-Server.tick| replace:: This needs to be called periodically to support the server communication and connections.

.. |method-network-Server.send| replace:: Send data.

.. |method-network-Server.onConnect| replace:: This callback is called when server accepts the connection.

.. |method-network-Server.onDisconnect| replace:: This callback is called when server or client drops the connection.

.. |method-network-Server.onData| replace:: This callback is called when data is received from the client.

.. |method-network-Server.onError| replace:: This callback is called on any error.

.. |method-network-Server.onLog| replace:: This is how server logs are printed.

.. |function-network-make_server| replace:: Creates new instance of the server.

.. |function-network-server_init| replace:: Initializes server with given port.

.. |function-network-server_is_connected| replace:: Returns true if server is connected to the client.

.. |function-network-server_is_open| replace:: Returns true if server is listening to the port.

.. |function-network-server_restore| replace:: Restores server from orphaned state.

.. |function-network-server_send| replace:: Sends data from server to the client.

.. |function-network-server_tick| replace:: This needs to be called periodically for the server to work.

.. |structure_annotation-network-NetworkServer| replace:: Base impliemntation of the server.

.. |method-network-Server.make_server_adapter| replace:: Creates new instance of the server adapter. Adapter is responsible for communicating with the Server class.


