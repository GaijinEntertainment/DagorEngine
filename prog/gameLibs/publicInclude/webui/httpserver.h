//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

namespace webui
{
// ENetSocket
#if _TARGET_PC_WIN | _TARGET_XBOX
typedef uintptr_t SocketType; // aka SOCKET or UINT_PTR
#else
typedef int SocketType;
#endif

enum HttpCode
{
  HTTP_OK = 200,
  HTTP_BAD_REQUEST = 400,
  HTTP_NOT_FOUND = 404,
  HTTP_SERVER_ERROR = 500,
  HTTP_NOT_IMPLEMENTED = 501,
};

struct RequestInfo
{
  SocketType conn; // open socket for write to
  uint32_t from_host;
  uint16_t from_port;

  struct HttpPlugin *plugin; // what module (plugin) is responsible for handle this request

  char *buf;     // raw buffer of request (all GET requests expected to fit there), for POST content use read_POST_content()
  int readedLen; // how many bytes already readed from buf (typically strlen of buf)

  char *request_method; // GET or POST
  char *uri;            // requested url
  char *http_version;   // typically 1.0 or 1.1
  char **params;        // parsed (decoded) params in pairs of strings (format [key, value, key, value, null]), could be null as well

  // POST params
  int content_len;
  char *content;

  // http headers
  int num_headers;
  struct HttpHeader
  {
    char *name;
    char *value;
  } http_headers[64];

  const char *get_header(const char *name, const char *def = NULL) const; // get by key
  void *read_POST_content() const;                                        // alloc buf and read from socket in it
};

struct HttpPlugin
{
  const char *name;        // should not contain spaces, length is not bounded
  const char *description; // used for index generation only, if NULL - not shown in index

  // is this plugin needed to be added on index generation?
  bool (*show_precondition)();
  // called when user clicked on link for this plugin (should be valid)
  void (*generate_response)(RequestInfo *params);
};

static constexpr int MAX_PLUGINS_LISTS = 8;
extern HttpPlugin *plugin_lists[MAX_PLUGINS_LISTS];

struct Config
{
  const char *bindAddr; // specify NULL for any host
  uint16_t bindPort;    // specify 0 for default (23456)
};

void startup(Config *cfg = NULL);
void shutdown();
void update();
void update_sync();
uint16_t get_binded_port();

// post html response (<html> & <body> tags appended automatically, Content-type set as text/html)
void html_response(SocketType conn, const char *body /* could be null */, HttpCode code = HTTP_OK);
// same as prev but without any tags
void html_response_raw(SocketType conn, const char *body, int body_len = -1, HttpCode code = HTTP_OK);
void html_response_raw(SocketType conn, const char *type, const char *body, int body_len = -1, HttpCode code = HTTP_OK);
// send text response, text could be null (but in that case len should be correct), or len could be -1 (then text should be correct)
void text_response(SocketType conn, const char *text, int len = -1);
// send json response (application/json)
void json_response(SocketType conn, const char *text, int len = -1);
// send any binary data/file, optional fname for correct file save
void binary_response(SocketType conn, const void *data, int len, const char *fname = NULL);
// send any file
void html_response_file(SocketType conn, const char *filename);


bool tcp_send(SocketType conn, const void *data, int data_len);
bool tcp_send_str(SocketType conn, const char *str, int str_len = -1);

// look up value by key, return NULL if not found
char **lupval(char **key_values, const char *key, char **start_from = NULL);

}; // namespace webui
