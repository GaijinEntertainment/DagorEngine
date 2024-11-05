// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <webui/httpserver.h>
#include <osApiWrappers/dag_sockets.h>
#include <osApiWrappers/dag_atomic.h>
#include <util/dag_globDef.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_threads.h>
#include <memory/dag_mem.h>
#include <EASTL/unique_ptr.h>
#include <osApiWrappers/dag_critSec.h>
#include <util/dag_string.h>
#include <memory/dag_framemem.h>
#include <ctype.h>
#include <math/dag_mathBase.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_localConv.h>
#include <stdlib.h>
#include <limits.h>
#include "webui_internal.h"

#if _TARGET_PC_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#include "windows.h"
#elif _TARGET_PC_LINUX
#include <unistd.h>
#include <limits.h>
#endif


#if _TARGET_PC_WIN | _TARGET_XBOX | _TARGET_C3
// nothing
#else // posix os
#include <sys/socket.h>
#endif


#define debug(...) logmessage(_MAKE4C('HTTP'), __VA_ARGS__)

#define RECEIVE_POST_DATA_TIMEOUT_MSEC (1000)

#if _TARGET_XBOX
#define HTTP_PORT 4600 // the only allowed on xbox
#else
#define HTTP_PORT 23456
#endif
#define BIND_PORT_ATTEMPTS 128

extern "C" const char *dagor_exe_build_date;
extern "C" const char *dagor_exe_build_time;

namespace webui
{

static void helloworld(RequestInfo *params) { html_response(params->conn, "Hello World!"); }
HttpPlugin dummy_list[] = {{"hello", "hello world module", NULL, helloworld}, {NULL}};

HttpPlugin *plugin_lists[MAX_PLUGINS_LISTS] = {dummy_list, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

// Skip the characters until one of the delimiters characters found.
// 0-terminate resulting word. Skip the rest of the delimiters if any.
// Advance pointer to buffer to the next word. Return found 0-terminated word.
static char *skip(char **buf, const char *delimiters)
{
  char *p, *begin_word, *end_word, *end_delimiters;

  begin_word = *buf;
  end_word = begin_word + strcspn(begin_word, delimiters);
  end_delimiters = end_word + strspn(end_word, delimiters);

  for (p = end_word; p < end_delimiters; p++)
    *p = '\0';

  *buf = end_delimiters;

  return begin_word;
}

/** Convert a two-char hex string into the char it represents. **/
static char x2c(char *what)
{
  char digit;
  digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A') + 10 : (what[0] - '0'));
  digit *= 16;
  digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A') + 10 : (what[1] - '0'));
  return digit;
}

/** Reduce any %xx escape sequences to the characters they represent. **/
static void unescape_url(char *url)
{
  int i, j;
  for (i = 0, j = 0; url[j]; ++i, ++j)
  {
    if ((url[i] = url[j]) == '%')
    {
      url[i] = x2c(&url[j + 1]);
      j += 2;
    }
  }
  url[i] = '\0';
}

/** Read the CGI input and place all name/val pairs into list.        **/
/** Returns list containing name1, value1, name2, value2, ... , NULL  **/
static char **getcgivars(char *cgiinput)
{
  if (!cgiinput || !*cgiinput)
    return NULL;

  int i;
  char **cgivars;
  char **pairlist;
  int paircount;
  char *nvpair;
  char *eqpos;

  /** Change all plusses back to spaces. **/
  for (i = 0; cgiinput[i]; i++)
    if (cgiinput[i] == '+')
      cgiinput[i] = ' ';

  /** First, split on "&" and ";" to extract the name-value pairs into **/
  /**   pairlist.                                                      **/
  pairlist = (char **)memalloc(256 * sizeof(char **), tmpmem);
  paircount = 0;
  nvpair = strtok(cgiinput, "&;");
  while (nvpair)
  {
    pairlist[paircount++] = str_dup(nvpair, tmpmem);
    if (!(paircount % 256))
      pairlist = (char **)memrealloc_default(pairlist, (paircount + 256) * sizeof(char **));
    nvpair = strtok(NULL, "&;");
  }
  pairlist[paircount] = 0; /* terminate the list with NULL */

  /** Then, from the list of pairs, extract the names and values. **/
  cgivars = (char **)memalloc((paircount * 2 + 1) * sizeof(char **), strmem);
  for (i = 0; i < paircount; i++)
  {
    eqpos = strchr(pairlist[i], '=');
    if (eqpos)
    {
      *eqpos = '\0';
      unescape_url(cgivars[i * 2 + 1] = str_dup(eqpos + 1, strmem));
    }
    else
      unescape_url(cgivars[i * 2 + 1] = str_dup("", strmem));
    unescape_url(cgivars[i * 2] = str_dup(pairlist[i], strmem));
  }
  cgivars[paircount * 2] = 0; /* terminate the list with NULL */

  /** Free anything that needs to be freed. **/
  for (i = 0; pairlist[i]; i++)
    memfree(pairlist[i], tmpmem);
  memfree(pairlist, tmpmem);

  /** Return the list of name-value strings. **/
  return cgivars;
}

const char *RequestInfo::get_header(const char *name, const char *def) const
{
  for (int i = 0; i < num_headers; ++i)
    if (!dd_stricmp(http_headers[i].name, name))
      return http_headers[i].value;
  return def;
}

void *RequestInfo::read_POST_content() const
{
  if (!content_len || strcmp(request_method, "POST") != 0 || !content)
    return NULL;
  int headers_size = content - buf;
  int already_readed = readedLen - headers_size;
  if (already_readed < 0)
    return NULL;
  void *ret = tmpmem->tryAlloc(content_len + 1);
  if (!ret)
    return NULL;

  int totalWaitMsec = 0;

  // copy already readed from socket part
  memcpy(ret, content, already_readed);
  // read from socket rest
  if (already_readed < content_len)
  {
    char *ptr = (char *)ret + already_readed;
    int left = content_len - already_readed;
    char *end = ptr + left;
    int readed;

    int waitNextRecvMsec = 0;

    do
    {
      readed = os_socket_recvfrom(conn, ptr, left);
      if (readed < 0)
      {
        totalWaitMsec++;
        waitNextRecvMsec++;
        sleep_msec(1);

        if (waitNextRecvMsec > RECEIVE_POST_DATA_TIMEOUT_MSEC)
        {
          debug("%s receive from socket failed", __FUNCTION__);
          tmpmem->free(ret);
          return NULL;
        }
      }
      else if (readed > 0)
      {
        waitNextRecvMsec = 0;
        ptr += readed;
        left += readed;
      }
    } while (readed && ptr < end);
    if (ptr != end)
    {
      tmpmem->free(ret);
      return NULL;
    }
  }

  (void)totalWaitMsec;
  if (totalWaitMsec > 1)
    debug("read_POST_content() waited data for time = %d ms", totalWaitMsec);

  ((char *)ret)[content_len] = 0;

  return ret;
}

class HttpServer : public DaThread
{
public:
  WinCritSec updateCritSection;
  WinCritSec reqCrit;
  Tab<RequestInfo> requests;
  os_socket_t listenSocket;
  uint16_t listenPort;

  HttpServer(Config *cfg) :
    DaThread("HttpServer", 128 << 10, 0, WORKER_THREADS_AFFINITY_MASK),
    listenSocket(OS_SOCKET_INVALID),
    listenPort(0),
    requests(midmem)
  {
    stripStackInMinidump();
    if (os_sockets_init() < 0)
    {
      debug("sockets init failed");
      return;
    }

    listenSocket = os_socket_create(OSAF_IPV4, OST_TCP);
    if (listenSocket == OS_SOCKET_INVALID)
    {
      debug("socket create failed");
      os_sockets_shutdown();
      return;
    }

    int port = cfg && cfg->bindPort ? cfg->bindPort : HTTP_PORT;
    const char *bindAddr = cfg && cfg->bindAddr && *cfg->bindAddr ? cfg->bindAddr : "0.0.0.0";
    os_socket_addr adr;
    os_socket_addr_from_string(OSAF_IPV4, bindAddr, &adr, sizeof(adr));
    bool port_binded = false;
    for (int i = 0; i < BIND_PORT_ATTEMPTS; ++i)
    {
      os_socket_addr_set_port(&adr, sizeof(adr), port);
      if (os_socket_bind(listenSocket, &adr, sizeof(adr)) < 0)
      {
        char errorBuf[512];
        int err = os_socket_last_error();
        debug("bind on port %d failed with error %d ('%s')", port, err, os_socket_error_str(err, errorBuf, sizeof(errorBuf)));
        port++;
      }
      else
      {
        port_binded = true;
        break;
      }
    }

    if (!port_binded)
    {
      debug("%s can't bind port, give up after %d tries", __FUNCTION__, BIND_PORT_ATTEMPTS);
      goto ctor_fail;
    }

    if (os_socket_listen(listenSocket, 16) < 0)
    {
      debug("listen failed");
      goto ctor_fail;
    }

    listenPort = port;

    debug("%s started on %s:%d", __FUNCTION__, bindAddr, port);
    debug_flush(false);

    return;

  ctor_fail:

    os_socket_close(listenSocket);
    listenSocket = OS_SOCKET_INVALID;
    os_sockets_shutdown();
  }

  ~HttpServer()
  {
    if (listenSocket == OS_SOCKET_INVALID)
      return;

    interlocked_release_store(terminating, 1);
    // wake up thread (plaform specific way)
#if _TARGET_PC_WIN | _TARGET_XBOX | _TARGET_C3
    os_socket_close(listenSocket); // MS doesn't support waking thread via shutdown() as by docs it should be
#else
    ::shutdown(listenSocket, SHUT_RDWR);
#endif
    // ensure that thread is finished
    if (!updateCritSection.timedLock(100)) // wait at most 100msec, to avoid long shutdown. better mem leak than shutdown.
      logwarn("http game server couldn't wait for lock, mem leak possible");
    else
      updateCritSection.unlock();
    this->terminate(true, 1000);
#if !(_TARGET_PC_WIN | _TARGET_XBOX)
    os_socket_close(listenSocket);
#endif
    listenSocket = OS_SOCKET_INVALID;

    for (int i = 0; i < requests.size(); ++i)
    {
      char **pars = requests[i].params;
      if (pars)
      {
        for (char **p = pars; *p; ++p)
          memfree(*p, strmem);
        memfree(pars, strmem);
      }
      os_socket_close(requests[i].conn);
      memfree(requests[i].buf, tmpmem);
    }

    os_sockets_shutdown();
  }

  bool isValid() const { return listenSocket != OS_SOCKET_INVALID; }

  static void generateIndex(SocketType conn, char *req, int)
  {
    String exeName = get_name_of_executable();
    Tab<char> buf(framemem_ptr());
    buf.resize(32 << 10);
    char *end = buf.data() + (buf.capacity() - 1);
    char *p = buf.data();
    p += _snprintf(buf.data(), end - p,
      "<head>\n"
      "  <style>\n"
      "    body { font-family: Verdana, sans-serif; }\n"
      "  </style>\n"
      "</head>\n"
      "<html>\n<head><title>%s - Web UI</title></head>\n<body>\n"
      "<h1>%s - %s</h1>\n<table>\n",
      exeName.c_str(), exeName.c_str(), get_platform_string_id());

    char *was_p = p;

    for (int i = 0; i < countof(plugin_lists); ++i)
    {
      HttpPlugin *pl = plugin_lists[i];
      if (!pl)
        continue;
      for (; pl->name && p < end; ++pl)
      {
        if (!pl->description) // hided
          continue;

        if (!pl->show_precondition || pl->show_precondition())
          p += _snprintf(p, end - p, "  <tr><td><a href=\"%s\">%s - %s</a></td></tr>\n", pl->name, pl->name, pl->description);
        else
          p += _snprintf(p, end - p,
            "  <tr><td><font color=grey>%s - %s (non active due to precondition)"
            "</font></td></tr>\n",
            pl->name, pl->description);
      }
    }

    if (p != was_p)
      p += _snprintf(p, end - p, "</table>\n");
    else
      p += _snprintf(p, end - p, "  <tr><td>No active http modules</td></tr>\n</table>\n");

    p += _snprintf(p, end - p, "<p>Build : %s %s, %s<br/>\n", dagor_exe_build_date, dagor_exe_build_time, get_platform_string_id());
    p += _snprintf(p, end - p, "Time Since Start : %d sec<br/>\n", get_time_msec() / 1000);
    p += _snprintf(p, end - p, "Process Id : %d</p>\n", get_process_uid());

    const char *footer = "</body>\n</html>";
    p = (char *)min((uintptr_t)p, (uintptr_t)(end - strlen(footer)));
    p += _snprintf(p, end - p, "%s", footer);
    *p = 0;

    html_response_raw(conn, buf.data());
    os_socket_close(conn);
    memfree(req, tmpmem);
  }

  static void parse_http_headers(char **p, RequestInfo *info)
  {
    info->num_headers = 0;
    for (int i = 0; i < countof(info->http_headers); ++i)
    {
      info->http_headers[i].name = skip(p, ": ");
      info->http_headers[i].value = skip(p, "\r\n");

      if (!info->http_headers[i].name[0])
        break;

      ++info->num_headers;

      // detect empty line (after all headers)
      char *bp = *p - 1;
      for (; *bp == 0; --bp)
        ;
      if (*p - bp >= 4)
        break;
    }
  }

#define REQUEST_FAIL(code)                       \
  {                                              \
    html_response((SocketType)conn, NULL, code); \
    return false;                                \
  }

  bool handleRequest(char *req, int reqSize, SocketType conn, os_socket_addr *from)
  {
    RequestInfo ri;
    memset(&ri, 0, sizeof(ri));

    uint32_t host = 0;
    os_socket_addr_get(from, sizeof(*from), &host);
    ri.from_host = os_ntohl(host);
    ri.from_port = os_socket_addr_get_port(from, sizeof(*from));

    char *p = req;
    for (; *p && isspace((unsigned char)*p); ++p) // skip initial spaces
      ;

    ri.request_method = skip(&p, " ");
    if (strcmp(ri.request_method, "GET") != 0 && strcmp(ri.request_method, "POST") != 0)
      REQUEST_FAIL(HTTP_NOT_IMPLEMENTED);

    ri.uri = skip(&p, " ");
    if (*ri.uri != '/') // uri should starts from '/' (not HTTP 1.1 compatible!)
      REQUEST_FAIL(HTTP_BAD_REQUEST);
    ++ri.uri; // skip '/'

    ri.http_version = skip(&p, "\r\n");
    if (strncmp(ri.http_version, "HTTP/", 5) != 0)
      REQUEST_FAIL(HTTP_BAD_REQUEST);
    ri.http_version += 5; // skip HTTP/

    parse_http_headers(&p, &ri);

    char *params_pos = strchr(ri.uri, '?');
    if (params_pos)
      *params_pos = 0;

    if (*ri.uri == '\0') // special case for empty string - index
    {
      if (params_pos)
        *params_pos = '?'; // return back
      generateIndex(conn, req, reqSize);
      return true;
    }

    for (int i = 0; i < countof(plugin_lists); ++i)
    {
      HttpPlugin *pl = plugin_lists[i];
      if (!pl)
        continue;
      for (; pl->name; ++pl)
      {
        if (strcmp(pl->name, ri.uri) != 0)
          continue;

        if (params_pos)
          *params_pos = '?'; // return back

        if (!pl->generate_response) // no handler?
          REQUEST_FAIL(HTTP_NOT_IMPLEMENTED);

        ri.buf = req;
        ri.readedLen = reqSize;

        if (strcmp(ri.request_method, "POST") == 0)
        {
          ri.content = p;
          const char *content_length = ri.get_header("Content-Length");
          if (content_length)
          {
            ri.content_len = strtol(content_length, NULL, 0);
            if (ri.content_len == INT_MAX || ri.content_len == INT_MIN) // bad length
              REQUEST_FAIL(HTTP_BAD_REQUEST);
            if (ri.content_len)
              ri.content = p;
          }
          const char *content_type = ri.get_header("Content-Type", "");
          if (!strcmp(content_type, "application/x-www-form-urlencoded") && ri.content_len)
            ri.params = getcgivars(ri.content);
        }

        if (!ri.params && params_pos)
          ri.params = getcgivars(params_pos + 1);
        ri.plugin = pl;
        ri.conn = conn;

        reqCrit.lock();
        memcpy(&requests.push_back(), &ri, sizeof(ri));
        reqCrit.unlock();

        return true;
      }
    }

    REQUEST_FAIL(HTTP_NOT_FOUND);
  }

#undef REQUEST_FAIL

  void update()
  {
    os_socket_addr from;
    int fromLen = sizeof(from);
    os_socket_t conn = os_socket_accept(listenSocket, &from, &fromLen);
    if (isThreadTerminating())
      return;
    else if (conn == OS_SOCKET_INVALID)
    {
      sleep_msec(1000); // Recovery timeout to not overload the system with requests.
      return;
    }

    constexpr int sz = 64 << 10; // 64 kb
    char stackTmp[sz];
    int readed = os_socket_recvfrom(conn, stackTmp, sz - 1);
    if (readed < 0)
    {
      debug("socket receive failed");
      return;
    }
    if (readed < 16)
    {
      os_socket_close(conn);
      return;
    }
    if (isThreadTerminating())
      return;
    // we can terminate update unil now.
    updateCritSection.lock(); // ensure we dont leak mem on exit
    char *tmp_buf = (char *)memalloc(sz, tmpmem);
    memcpy(tmp_buf, stackTmp, readed);
    tmp_buf[readed] = 0;

    if (!handleRequest(tmp_buf, readed, conn, &from))
    {
      os_socket_close(conn);
      memfree(tmp_buf, tmpmem);
    }
    updateCritSection.unlock();
  }

  void execute()
  {
    while (!isThreadTerminating())
      update();
  }

  void processRequests()
  {
    reqCrit.lock();
    while (!requests.empty())
    {
      RequestInfo r = *requests.data();
      erase_items(requests, 0, 1);
      reqCrit.unlock();

      r.plugin->generate_response(&r);
      char **pars = r.params;
      if (pars)
      {
        for (char **p = pars; *p; ++p)
          memfree(*p, strmem);
        memfree(pars, strmem);
      }
      os_socket_close(r.conn);
      memfree(r.buf, tmpmem);

      reqCrit.lock();
    }
    reqCrit.unlock();
  }
};

static eastl::unique_ptr<HttpServer> http_server;

void startup(Config *cfg)
{
  if (http_server) // already started
    return;

  HttpPlugin *pl = NULL;
  for (int i = 0; i < countof(plugin_lists) && !pl; ++i)
    pl = plugin_lists[i];
  G_ASSERTF(pl, "%s should be at least valid plugin list", __FUNCTION__);

  auto s = eastl::make_unique<HttpServer>(cfg);
  if (s->isValid())
  {
    s->start();
    http_server = eastl::move(s);
  }
}

void shutdown() { http_server.reset(); }

void update()
{
  if (http_server)
    http_server->processRequests();
}

void update_sync()
{
  if (http_server)
  {
    interlocked_release_store(http_server->terminating, 1);
    http_server->update();
    http_server->processRequests();
  }
}

uint16_t get_binded_port() { return http_server ? http_server->listenPort : 0; }

bool tcp_send(SocketType conn, const void *data, int data_len)
{
  if (data_len <= 0)
    return false;
  char *ptr = (char *)data;
  int left = data_len;
  for (; left;) // could be freeze?
  {
    int sent = os_socket_send(conn, ptr, left);
    if (sent < 0)
    {
      debug("%s socket sent for %u bytes failed", __FUNCTION__, left);
      return false;
    }
    ptr += sent;
    left -= sent;
  }
  return true;
}

bool tcp_send_str(SocketType conn, const char *str, int str_len)
{
  return str && tcp_send(conn, (void *)str, str_len < 0 ? (int)strlen(str) : str_len);
}

// Socket SEND
#define SSEND(conn, str) tcp_send_str(conn, str)

static const char *get_http_status(HttpCode &code)
{
  switch (code)
  {
    case HTTP_OK: return "OK";
    case HTTP_BAD_REQUEST: return "Bad Request";
    case HTTP_NOT_FOUND: return "Not Found";
    case HTTP_NOT_IMPLEMENTED: return "Not Implemented";
    case HTTP_SERVER_ERROR:
    default: code = HTTP_SERVER_ERROR; return "Server Error";
  };
}

void html_response(SocketType conn, const char *body, HttpCode code)
{
  char header[512];

  const char *status = get_http_status(code);
  if (!body)
    body = status;

  const char *htmlHeader = (code == HTTP_OK) ? "<html><body><p><a href=\"/\">Back To Main</a></p>" : "<html><body>";
  const char *htmlFooter = "</body></html>";

  _snprintf(header, sizeof(header),
    "HTTP/1.1 %d %s\r\n"
    "Connection: close\r\n"
    "Cache-Control: no-cache, no-store, must-revalidate\r\n"
    "Pragma: no-cache\r\n"
    "Expires: 0\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Content-Type: text/html; charset=utf-8\r\n"
    "Content-Length: %d\r\n"
    "\r\n"
    "%s",
    code, status, int(strlen(body) + strlen(htmlHeader) + strlen(htmlFooter)), htmlHeader);
  header[sizeof(header) - 1] = 0;

  SSEND(conn, header);
  SSEND(conn, body);
  SSEND(conn, htmlFooter);
}

void html_response_raw(SocketType conn, const char *body, int body_len, HttpCode code)
{
  html_response_raw(conn, "text/html", body, body_len, code);
}

void html_response_raw(SocketType conn, const char *type, const char *body, int body_len, HttpCode code)
{
  char header[512];

  const char *status = get_http_status(code);
  if (!body)
    body = status;

  if (body_len < 0)
    body_len = (int)strlen(body);

  _snprintf(header, sizeof(header),
    "HTTP/1.1 %d %s\r\n"
    "Connection: close\r\n"
    "Cache-Control: no-cache, no-store, must-revalidate\r\n"
    "Pragma: no-cache\r\n"
    "Expires: 0\r\n"
    "Content-Type: %s; charset=utf-8\r\n"
    "Content-Length: %d\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "\r\n",
    code, status, type, body_len);
  header[sizeof(header) - 1] = 0;

  SSEND(conn, header);
  tcp_send_str(conn, body, body_len);
}

void text_response(SocketType conn, const char *text, int len)
{
  G_ASSERT(text || len > 0);

  if (len < 0)
    len = (int)strlen(text);

  char header[256];

  _snprintf(header, sizeof(header),
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\n"
    "Cache-Control: no-cache, no-store, must-revalidate\r\n"
    "Pragma: no-cache\r\n"
    "Expires: 0\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: %d\r\n"
    "\r\n",
    len);
  header[sizeof(header) - 1] = 0;

  SSEND(conn, header);
  if (text)
    tcp_send_str(conn, text, len);
}

void json_response(SocketType conn, const char *text, int len)
{
  G_ASSERT(text);
  if (len < 0)
    len = (int)strlen(text);

  char header[256];
  _snprintf(header, sizeof(header),
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\n"
    "Cache-Control: no-cache, no-store, must-revalidate\r\n"
    "Pragma: no-cache\r\n"
    "Expires: 0\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %d\r\n"
    "\r\n",
    len);
  header[sizeof(header) - 1] = 0;

  SSEND(conn, header);
  tcp_send_str(conn, text, len);
}

void binary_response(SocketType conn, const void *data, int len, const char *fname)
{
  char header[256];
  _snprintf(header, sizeof(header),
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\n"
    "Cache-Control: no-cache, no-store, must-revalidate\r\n"
    "Pragma: no-cache\r\n"
    "Expires: 0\r\n"
    "Content-Type: application/octet-stream\r\n"
    "%s%s%s"
    "Content-Length: %d\r\n"
    "Content-Transfer-Encoding: binary\r\n"
    "\r\n",
    fname ? "Content-Disposition: attachment; filename=" : "", fname ? fname : "", fname ? "\r\n" : "", len);
  header[sizeof(header) - 1] = 0;

  SSEND(conn, header);
  tcp_send(conn, data, len);
}

void html_response_file(SocketType conn, const char *filename)
{
  file_ptr_t f = df_open(filename, DF_READ | DF_IGNORE_MISSING);
  if (!f)
  {
    G_ASSERTF(0, "html_response_file: cannot open file '%s'", filename);
    return;
  }

  int len = df_length(f);
  Tab<char> data;
  if (len > 0)
    data.resize(len + 1);
  bool ok = (len > 0) ? (df_read(f, data.data(), len) == len) : (len == 0);
  df_close(f);

  if (ok)
  {
    data[len] = 0;
    html_response_raw(conn, data.data());
  }
  else
    G_ASSERTF(0, "html_response_file: error reading file '%s'", filename);
}


char **lupval(char **key_values, const char *key, char **start_from)
{
  if (!start_from)
    start_from = key_values;
  for (char **p = start_from; p && *p; p += 2)
    if (!strcmp(key, *p))
      return p;
  return NULL;
}

}; // namespace webui

String get_name_of_executable()
{
#if _TARGET_PC_WIN
  char name[MAX_PATH];
  GetModuleFileNameA(NULL, name, sizeof(name));
  const char *slash = max(strrchr(name, '\\'), strrchr(name, '/'));
  return String(slash ? slash + 1 : name);
#elif _TARGET_PC_LINUX
  char name[PATH_MAX];
  int len = readlink("/proc/self/exe", name, sizeof(name) - 1);
  if (len != -1)
  {
    name[len] = 0;
    const char *slash = strrchr(name, '/');
    return String(slash ? slash + 1 : name);
  }
  return String();
#else
  return String();
#endif
}
