#include <webui/httpserver.h>
#include <errno.h>

#include <memory/dag_framemem.h>
#include <generic/dag_tab.h>
#include <util/dag_lookup.h>
#include <stdio.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_console.h>
#include <ioSys/dag_genIo.h>
#include <gui/dag_visConsole.h>
#include <3d/dag_drv3dCmd.h>
#include <image/dag_jpeg.h>
#include <3d/dag_drv3d.h>
#include <ioSys/dag_memIo.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_dataBlock.h>
#include <webui/helpers.h>
#include <debug/dag_debug.h>
#include <debug/dag_logSys.h>
#include <workCycle/dag_delayedAction.h>
#include <math/random/dag_random.h>
#include <perfMon/dag_cpuFreq.h>
#if _TARGET_PC_WIN
#include <windows.h>
#endif
#include <osApiWrappers/dag_stackHlp.h>
#include <stdlib.h>
#include "webui_internal.h"
#include <perfMon/dag_statDrv.h>
#include <debug/dag_debug.h>

using namespace webui;

#if _TARGET_PC || _TARGET_TVOS
#define DEFAULT_TAIL_SIZE (16 << 10)
#define SEND_LOG_TIMEOUT  (10000)

static void show_log(RequestInfo *params)
{
  debug_flush(false);

  char rl[4096];

  file_ptr_t ld = df_open(get_log_filename(), DF_READ);
  if (!ld)
  {
    _snprintf(rl, sizeof(rl), "Error: can't open log '%s'", get_log_filename());
    rl[sizeof(rl) - 1] = 0;
    return html_response(params->conn, rl, HTTP_SERVER_ERROR);
  }

  int file_size = df_length(ld);

  int start_from = -1;
  char **start_from_param = webui::lupval(params->params, "start_from");
  if (start_from_param && *start_from_param[1])
  {
    errno = 0;
    int new_start_from = strtol(start_from_param[1], NULL, 10);
    if (errno == 0)
      start_from = max(0, min(file_size, new_start_from));
  }

  int up_to = -1;
  char **up_to_param = webui::lupval(params->params, "up_to");
  if (up_to_param && *up_to_param[1])
  {
    errno = 0;
    int new_up_to = strtol(up_to_param[1], NULL, 10);
    if (errno == 0)
      up_to = (new_up_to <= 0) ? file_size : min(file_size, new_up_to);
  }

  int tail_size = min(file_size, DEFAULT_TAIL_SIZE);
  bool tail_is_specified = false;
  char **tail = webui::lupval(params->params, "tail");
  if (tail && *tail[1])
  {
    tail_is_specified = true;
    errno = 0;
    int new_tail_size = strtol(tail[1], NULL, 10);
    if (errno == 0)
      tail_size = (new_tail_size <= 0) ? file_size : min(file_size, new_tail_size << 10);
  }

  if (tail_is_specified && (up_to != -1 || start_from != -1))
  {
    df_close(ld);
    return html_response(params->conn, "Error: Cannot specify both 'tail' and 'start_from'/'up_to' params.", HTTP_BAD_REQUEST);
  }

  if (up_to == -1 && start_from == -1)
  {
    start_from = file_size - tail_size;
    up_to = file_size;
  }
  else
  {
    up_to = up_to == -1 ? file_size : up_to;
    start_from = clamp(start_from, 0, up_to);
    up_to = clamp(up_to, start_from, file_size);
  }
  int response_size = up_to - start_from;

  if (response_size <= 0)
  {
    df_close(ld);
    return html_response(params->conn, NULL, HTTP_OK);
  }

  if (df_seek_to(ld, start_from))
  {
    df_close(ld);
    return html_response(params->conn, NULL, HTTP_SERVER_ERROR);
  }
  text_response(params->conn, NULL, response_size);

  int64_t start_time = ref_time_ticks();
  while (response_size > 0)
  {
    int r = df_read(ld, rl, sizeof(rl));
    if (r < 0)
      break;

    tcp_send(params->conn, rl, r);

    if (get_time_usec(start_time) / 1000 >= SEND_LOG_TIMEOUT)
    {
      SNPRINTF(rl, sizeof(rl), "\n\n< aborted due to timeout of %d ms >", SEND_LOG_TIMEOUT);
      tcp_send_str(params->conn, rl);
      break;
    }

    response_size -= r;
  }

  df_close(ld);
}
#endif

class PrintfDriver : public console::IVisualConsoleDriver
{
public:
  char *buf;
  size_t bufSize;
  size_t offs;

  PrintfDriver(char *b, size_t bs) : buf(b), bufSize(bs), offs(0) {}

  void puts(const char *str, console::LineType /*type*/) override
  {
    eastl::string replaced = str;
    auto replace = [&](const eastl::string &s, const eastl::string &to) {
      size_t n = 0;
      while ((n = replaced.find(s, n)) != eastl::string::npos)
      {
        replaced.replace(n, s.size(), to);
        n += to.size();
      }
    };
    replace("&", "&amp;");
    replace("\"", "&quot;");
    replace("'", "&#039;");
    replace("<", "&lt;");
    replace(">", "&gt;");
    int l = (int)replaced.length();
    if ((offs + l) < bufSize - 1)
      offs += _snprintf(buf + offs, bufSize - offs - 1, "%s", replaced.c_str());
  }

  void init(const char *) {}
  void shutdown() {}
  void render() {}
  void update() {}
  bool processKey(int, int, bool) { return false; }
  void reset_important_lines() {}
  void show() {}
  void hide() {}
  bool is_visible() { return true; }
  real get_cur_height() { return 1.f; }
  void set_cur_height(real) {}
  void save_text(const char *) {}
  virtual void setFontId(int) {}
  virtual const char *getEditTextBeforeModify() const override { return nullptr; }
  virtual void set_progress_indicator(const char *, const char *) override{};
};

void console_gen(RequestInfo *params)
{
  const char *form = "<script type='text/javascript'>\n"
                     "function conrequest(str,q)\n"
                     "{\n"
                     "  var x = new XMLHttpRequest();\n"
                     "  x.onreadystatechange=function()\n"
                     "  {\n"
                     "    if (x.readyState==4 && x.status==200)\n"
                     "      document.getElementById('txtHint').innerHTML=x.responseText;\n"
                     "  }\n"
                     "  x.open('GET','console?'+q+'='+encodeURIComponent(str),true);\n"
                     "  x.send();\n"
                     "}\n"
                     "function run() { conrequest(document.getElementById('concmd').value, 'concmd'); }\n"
                     "function on_key_up(event)\n"
                     "{\n"
                     "  var q = 'q';\n"
                     "  var keyNum = 0;\n"
                     "  if (window.event) keyNum = event.keyCode;\n"
                     "  else if (event.which) keyNum = event.which;\n"
                     "  if (keyNum == 13) q = 'concmd';\n"
                     "  conrequest(document.getElementById('concmd').value,q);\n"
                     "}\n"
                     "function hint(h) { document.getElementById('concmd').value = h; }\n"
                     "window.onload=function() { conrequest('','q'); }\n"
                     "</script>\n"
                     "Console Command:\n"
                     "<input type='text' id='concmd' size='128' onkeyup='on_key_up(event)'/>\n"
                     "<input type='button' value='Run' onclick='run()' />\n"
                     "<p id='txtHint'></p>\n";
  if (!params->params)
    html_response(params->conn, form);
  else
  {
    YAMemSave save(512 << 10);
    save.printf("<html><body>");
    if (!strcmp(*params->params, "concmd"))
    {
      size_t oldOffs = save.offset;
      save.printf("<pre>");
      const int footerLen = 100;
      PrintfDriver drv(save.mem + save.offset, save.bufSize - save.offset - footerLen);

      console::IVisualConsoleDriver *old_drv = console::get_visual_driver();
      console::set_visual_driver_raw(&drv);

      const char *str = *(params->params + 1);
      console::command(str);
#if _TARGET_PC
      console::add_history_command(str);
#endif

      console::set_visual_driver_raw(old_drv);

      if (*(save.mem + save.offset)) // was something written?
      {
        save.offset = strlen(save.mem);
        save.printf("</pre>");
      }
      else // nope
      {
        save.offset = oldOffs;
        save.printf("<br/>console command '%s' executed", *(params->params + 1));
      }
    }
    else if (!strcmp(*params->params, "q"))
    {
      const char *te = *(params->params + 1);

      static constexpr int MAX_STRS = 30;

      // gather matched console commands
      console::CommandList list(framemem_ptr());
      list.reserve(256);
      console::get_command_list(list); // this kinda slow
      Tab<console::CommandStruct *> mlist(framemem_ptr());
      mlist.reserve(MAX_STRS);
      if (te && *te)
        for (int i = 0; i < list.size() && mlist.size() < MAX_STRS; ++i)
          if (strstr(list[i].command.str(), te) == list[i].command.str())
            mlist.push_back(&list[i]);

      // gather matched history strings
      Tab<const char *> hist_strs(framemem_ptr());
      const char *hs = "";
      if (te && *te)
        do
        {
          hs = console::get_prev_command();
          if (strstr(hs, te) == hs)
            hist_strs.push_back(hs);
        } while (*hs);

      Tab<const char *> history(framemem_ptr());
      history.reserve(MAX_STRS);
      hs = "";
      do
      {
        hs = console::get_prev_command();
        if (*hs && history.size() < MAX_STRS)
          history.push_back(hs);
      } while (*hs);

      char cmdText[384];
      if (hist_strs.size() || history.size() || (mlist.size() > 1 || (mlist.size() == 1 && mlist[0]->command != te)))
      {
        save.printf("\n<table>\n");
        int cnt = max(max(mlist.size(), hist_strs.size()), history.size());
        for (int i = 0; i < cnt; ++i)
        {
          save.printf("  <tr align='center'>");

          // available console commands
          if (i < mlist.size())
          {
            console::CommandStruct *cmd = mlist[i];
            SNPRINTF(cmdText, sizeof(cmdText), "%s  ", cmd->command.str());
            for (int k = 1; k < cmd->minArgs; k++)
              strncat(cmdText, "x ", min(2, int(sizeof(cmdText) - strlen(cmdText) - 1)));
            if (cmd->minArgs != cmd->maxArgs)
            {
              strncat(cmdText, "[", min(1, int(sizeof(cmdText) - strlen(cmdText) - 1)));
              for (int k = cmd->minArgs; k < cmd->maxArgs; k++)
                if (k != cmd->maxArgs - 1)
                  strncat(cmdText, "x ", min(2, int(sizeof(cmdText) - strlen(cmdText) - 1)));
                else
                  strncat(cmdText, "x", min(1, int(sizeof(cmdText) - strlen(cmdText) - 1)));
              strncat(cmdText, "]", min(1, int(sizeof(cmdText) - strlen(cmdText) - 1)));
            }
            save.printf("<td><a href=\"javascript:hint('%s')\">%s</a></td>", cmd->command.str(), cmdText);
          }
          else
            save.printf("<td>&nbsp;</td>");

          // matched history
          if (i < hist_strs.size())
            save.printf("<td><a href=\"javascript:conrequest('%s','concmd')\">%s</a></td>", hist_strs[i], hist_strs[i]);
          else
            save.printf("<td>&nbsp;</td>");

          // all history
          if (i < history.size())
            save.printf("<td><a href=\"javascript:conrequest('%s','concmd')\">%s</a></td>", history[i], history[i]);
          else
            save.printf("<td>&nbsp;</td>");

          save.printf("</tr>\n");
        }
        save.printf("</table>\n");
      }
    }
    else
      return html_response(params->conn, NULL, HTTP_NOT_FOUND);
    save.printf("</body></html>");
    html_response_raw(params->conn, save.mem);
  }
}


struct VromFile
{
  const char *name;
  const PatchableTab<const char> *data;
};

static const char *is_text_file(const char *fname)
{
  static const char *text_exts[] = {".blk", ".nut", ".csv", ".css", ".txt"};
  const char *t = NULL;
  for (int i = 0; i < countof(text_exts) && !t; ++i)
    t = strstr(fname, text_exts[i]);
  return t;
}

static void vromfs_enumerator(RequestInfo *params)
{
  const char *hdrs[] = {"#", "name", "size", "edit"};
  int total_mem = 0;

  Tab<VromFile> all_vrom_files(framemem_ptr());
  iterate_vroms([&](VirtualRomFsData *v, size_t) {
    all_vrom_files.reserve(all_vrom_files.size() + v->files.map.size());
    for (int j = 0; j < v->files.map.size(); ++j)
    {
      VromFile &f = all_vrom_files.push_back();
      f.name = v->files.map[j];
      f.data = &v->data[j];
    }
    return true; // continue
  });

  YAMemSave buf(128 << 10);

  const char *submit_js = "\n<script type=\"text/javascript\">\n"
                          "function on_submit(id)\n"
                          "{\n"
                          "  var x = new XMLHttpRequest();\n"
                          "  x.open('POST', '%s?submit='+id, false);\n"
                          "  x.send(document.getElementById('texa').value)\n"
                          "}\n"
                          "</script>\n";
  buf.printf(submit_js, params->plugin->name);

  if (params->params)
  {
    const char *cmds[] = {"show", "edit", "submit"};
    int cmdi = lup(*params->params, cmds, countof(cmds));
    switch (cmdi)
    {
      case 0: // show
      {
        int idx = strtol(*(params->params + 1), NULL, 0);
        if ((uint32_t)idx < all_vrom_files.size())
        {
          VromFile &vf = all_vrom_files[idx];
          buf.offset = 0;
          if (strstr(vf.name, ".blk"))
          {
            InPlaceMemLoadCB load_cb(vf.data->data(), vf.data->size());
            DataBlock b(framemem_ptr());
            b.loadFromStream(load_cb);
            b.saveToTextStream(buf);
            b.reset();
            text_response(params->conn, buf.mem, (int)buf.offset);
          }
          else
          {
            if (is_text_file(vf.name))
              text_response(params->conn, vf.data->data(), vf.data->size());
            else
              binary_response(params->conn, (void *)vf.data->data(), vf.data->size(), dd_get_fname(vf.name));
          }
          return;
        }
      }
      break;

      case 1: // edit
      {
        int idx = strtol(*(params->params + 1), NULL, 0);
        if ((uint32_t)idx < all_vrom_files.size())
        {
          VromFile &vf = all_vrom_files[idx];
          if (is_text_file(vf.name))
          {
            buf.printf("<a href=\"javascript:on_submit(%d)\">submit</a><br/><br/>\n\n", idx, vf.name);
            buf.printf("<textarea id=\"texa\" rows=\"80\" cols=\"100\">\n", idx);
            if (strstr(vf.name, ".blk"))
            {
              InPlaceMemLoadCB load_cb(vf.data->data(), vf.data->size() * 2);
              DataBlock b(framemem_ptr());
              b.loadFromStream(load_cb);
              b.saveToTextStream(buf);
            }
            else
              buf.printf("%.*s", vf.data->size(), vf.data->data());
            buf.printf("</textarea>\n");
            html_response(params->conn, buf.mem);
            return;
          }
        }
      }
      break;

      case 2: // submit
      {
        int idx = strtol(*(params->params + 1), NULL, 0);
        void *post = params->read_POST_content();
        if ((uint32_t)idx >= all_vrom_files.size() || !post)
          html_response_raw(params->conn, NULL, -1, HTTP_BAD_REQUEST);
        else
        {
          VromFile &vf = all_vrom_files[idx];
          ((PatchableTab<const char> *)vf.data)->init(post, params->content_len); // FIXME : mem leak!
          debug("patch '%s' to %d bytes", vf.name, params->content_len);
          html_response_raw(params->conn, "");
        }
        return;
      }
      break;
    }
  }

  buf.printf("\n<table border>\n");
  print_table_header(buf, hdrs, countof(hdrs));
  for (int i = 0; i < all_vrom_files.size(); ++i)
  {
    VromFile &vf = all_vrom_files[i];

    buf.printf("  <tr align=\"center\">");
    TCOL("%d", i);
    TCOL("<a href=\"%s?show=%d\">%s</a>", params->plugin->name, i, vf.name);
    TCOL("%d bytes (%dK)", vf.data->size(), vf.data->size() >> 10);
    if (is_text_file(vf.name))
      TCOL("<a href=\"%s?edit=%d\">edit</a>", params->plugin->name, i);
    else
      TCOL("<td>&nbsp;</td>");
    buf.printf("</tr>\n");
    total_mem += vf.data->size();
  }
  buf.printf("\n</table>\n");

  buf.printf("</br><h3>Total %d K</h3>", total_mem >> 10);

  html_response(params->conn, buf.mem);
}

#if _TARGET_PC_WIN && !_TARGET_64BIT && DAGOR_DBGLEVEL > 0

#define BYTES_PER_PIXEL (32)
#define MAX_PIC_X       1280
struct DebugChunk
{
  int generation;
  int dataSize;
  void *stack[18]; // hardcode
  IMemAlloc *allocator;
  DebugChunk *prev, *next;
  unsigned frontGuard[1];
};

static void memory_map(RequestInfo *params) // Note: not thread safe
{
  int bytes_per_pixel = BYTES_PER_PIXEL;
  int skipBytes = INT_MAX;
  char **pars = params->params;
  while (pars && *pars)
  {
    const char *cmds[] = {"rc", "bpp", "skip"};
    int cmdi = lup(*pars, cmds, countof(cmds));
    errno = 0;
    switch (cmdi)
    {
      case 0: // rc
      {
        DebugChunk *dc_ptr = (DebugChunk *)strtoul(*(pars + 1), NULL, 0); // Not 64-bit ready
        if (errno != 0 || !dc_ptr)
          return html_response(params->conn, NULL, HTTP_BAD_REQUEST);
#if _TARGET_PC_WIN
        if (::IsBadReadPtr(dc_ptr, sizeof(DebugChunk)))
          return html_response(params->conn, NULL, HTTP_BAD_REQUEST);
#endif
        if (dc_ptr->generation != 0x100)
          return html_response(params->conn, NULL, HTTP_BAD_REQUEST);

        YAMemSave buf;
        buf.printf("%d bytes (%d K)\n\n", dc_ptr->dataSize, dc_ptr->dataSize >> 10);
        buf.printf("%s", stackhlp_get_call_stack_str(dc_ptr->stack, countof(dc_ptr->stack)).str());
        return text_response(params->conn, buf.mem, (int)buf.offset);
      }
      break;

      case 1: // bpp
      {
        int bpp = strtol(*(pars + 1), NULL, 0);
        if (errno != 0 || bpp < 4 || (bpp & (bpp - 1)) != 0)
          return html_response(params->conn, NULL, HTTP_BAD_REQUEST);
        bytes_per_pixel = bpp;
      }
      break;

      case 2: // skip
      {
        skipBytes = strtol(*(pars + 1), NULL, 0);
        if (errno != 0)
          return html_response(params->conn, NULL, HTTP_BAD_REQUEST);
      }
      break;
    };
    pars += 2; // next pair
  }

  YAMemSave buf(16 << 20);

  DebugChunk *dc = ((DebugChunk *)buf.mem) - 1;
  if (dc->generation != 0x100)
    return html_response(params->conn, NULL, HTTP_SERVER_ERROR);

  DebugChunk *top = dc;
  do
  {
#if _TARGET_PC_WIN
    if (::IsBadReadPtr(top, sizeof(DebugChunk)))
      return html_response(params->conn, NULL, HTTP_SERVER_ERROR);
#endif
    if (!top->next)
      break;
    top = top->next;
  } while (1);

  buf.printf("<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n");
  const char *ajax_op = "<script type=\"text/javascript\"><![CDATA[\n"
                        "function rc(dc_ptr)\n"
                        "{\n"
                        "  var x = new XMLHttpRequest();\n"
                        "  x.onreadystatechange=function()\n"
                        "  {\n"
                        "    if (x.readyState==4 && x.status==200)\n"
                        "      alert(x.responseText)\n"
                        "  }\n"
                        "  x.open('GET','memory_map?rc='+dc_ptr, true);\n"
                        "  x.send();\n"
                        "}\n"
                        "]]></script>\n";
  buf.printf("%s", ajax_op);

  int curx = 0, cury = 0;
  int maxx = MAX_PIC_X;
  for (DebugChunk *c = top; c; c = c->prev)
  {
    if (((void *)(c + 1)) == (void *)buf.mem)
      continue;
    int l = c->dataSize; // dlmalloc_get_usable_size()?
    if (l < bytes_per_pixel || l > skipBytes)
      continue; // too small or too big
    int lpix = (l + bytes_per_pixel - 1) / bytes_per_pixel;
    int rand_seed = 0;
    for (int j = 0; j < 8; ++j)
      rand_seed += (int)c->stack[j];
    int cr = _rnd(rand_seed) & 255;
    int cg = _rnd(rand_seed) & 255;
    int cb = _rnd(rand_seed) & 255;
    size_t saved_offset = buf.offset;
    int rx = curx, ry = cury;
    while (lpix)
    {
      int new_curx = min(curx + lpix, maxx);
      buf.printf("<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" style=\"stroke:rgb(%d,%d,%d)\" "
                 "onclick=\"rc(0x%p)\" />\n",
        curx, cury, new_curx, cury, cr, cg, cb, c);
      lpix -= min(lpix, maxx - curx);
      if (new_curx == maxx)
      {
        cury += 1;
        curx = 0;
      }
      else
        curx = new_curx;
    }
    // need generate polygon?
    if (cury > ry)
    {
      buf.offset = saved_offset; // override lines with poly
      buf.printf("<polygon points=\"");
      // top poly half
      if (rx != 0)
      {
        buf.printf("%d,%d ", 0, ry + 1);
        buf.printf("%d,%d ", rx, ry + 1);
        buf.printf("%d,%d ", rx, ry);
        buf.printf("%d,%d ", maxx, ry);
      }
      else
      {
        buf.printf("%d,%d ", 0, ry);
        buf.printf("%d,%d ", maxx, ry);
      }
      // bottom poly half
      if (curx != 0)
      {
        buf.printf("%d,%d ", maxx, cury - 1);
        buf.printf("%d,%d ", curx, cury - 1);
        buf.printf("%d,%d ", curx, cury);
        buf.printf("%d,%d ", 0, cury);
      }
      else
      {
        buf.printf("%d,%d ", maxx, cury - 1);
        buf.printf("%d,%d ", 0, cury - 1);
      }
      buf.mem[--buf.offset] = 0; // remove extra space
      buf.printf("\" style=\"fill:rgb(%d,%d,%d)\" onclick=\"rc(0x%p)\" />\n", cr, cg, cb, c);
    }
  }
  buf.printf("</svg>\n<body>\n<html>");

  html_response_raw(params->conn, buf.mem);
}
#endif // _TARGET_PC && DAGOR_DBGLEVEL > 0

struct QuitAction : public DelayedAction
{
  void performAction()
  {
    delete this;
    quit_game(0);
  }
};

static void on_quit(RequestInfo *params)
{
  add_delayed_action(new QuitAction());
  return html_response_raw(params->conn, "<html><body><h1>Good Bye</h1></body></html>");
}

#define DEF_TO_STR(x)     DEF_TO_STR_IMP(x)
#define DEF_TO_STR_IMP(x) #x

#undef DEF_TO_STR_IMP
#undef DEF_TO_STR

webui::HttpPlugin webui::dagor_http_plugins[] = {{"quit", "quit from game", NULL, on_quit},
#if _TARGET_PC || _TARGET_TVOS
  {"log", "show debug log (currently pc only)", NULL, show_log},
#endif
  {"console", "execute commands", NULL, console_gen},
  {"vromfs", "show content of virtual file systems (vromfs)", NULL, vromfs_enumerator},
#if _TARGET_PC_WIN && !_TARGET_64BIT && DAGOR_DBGLEVEL > 0
  {"memory_map", "visual memory allocations map (pc only, experimental)", NULL, memory_map},
#endif
  {NULL}};
