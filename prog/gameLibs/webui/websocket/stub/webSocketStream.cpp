#include <webui/websocket/webSocketStream.h>

namespace websocket
{
bool start(int) { return false; }
void update() {}
void stop() {}
int get_port() { return -1; }

void send_log(const char *) {}
void send_message(const DataBlock &) {}
void send_buffer(const char *, int) {}
void send_binary(dag::ConstSpan<uint8_t>) {}
void send_binary(const uint8_t *, int) {}
void send_binary(BsonStream &) {}
void add_message_listener(MessageListener *) {}
void remove_message_listener(MessageListener *) {}

void MessageListener::addCommand(const char *, const MessageListener::Method &) {}
bool MessageListener::processCommand(const char *, const DataBlock &) { return false; }
} // namespace websocket