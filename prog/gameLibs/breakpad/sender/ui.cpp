#include "ui.h"
#include "lang.h"

#include <assert.h>
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace breakpad
{
namespace ui
{

class BaseWindow
{
public:
  static void on_btn_close(Fl_Widget *, void *udata) { ((BaseWindow *)udata)->quit(Response::Close); }
  static void on_btn_restart(Fl_Widget *, void *udata) { ((BaseWindow *)udata)->quit(Response::Restart); }

public:
  BaseWindow(breakpad::Configuration &c, const std::string &t, const std::string &m) : cfg(c), title(t), text(m) {}

  virtual ~BaseWindow()
  {
    for (auto &b : this->rBtns)
      delete b.widget;
    for (auto &b : this->lBtns)
      delete b.widget;
    for (auto &o : this->options)
      delete o.widget;
    delete this->msg;
    delete this->form;
  }

  Response show()
  {
    if (!this->form)
      this->create();

    if (this->form)
    {
      this->form->show();
      while (!this->shouldQuit && Fl::wait())
        ;
    }
    return this->response;
  }

protected:
  void quit(Response r = Response::Close)
  {
    this->response = r;
    this->shouldQuit = true;
  }

  virtual void addOption(const char *txt, Fl_Callback *cb, bool p) { this->options.emplace_back(txt, cb, p); }
  virtual void addBtnLeft(const char *txt, Fl_Callback *cb) { this->lBtns.emplace_back(txt, cb); }
  virtual void addBtnRight(const char *txt, Fl_Callback *cb) { this->rBtns.emplace_back(txt, cb); }

private:
  void create()
  {
    auto const alignment = FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_WRAP;
    Fl::scheme("gtk+");
    this->form = new Fl_Window(320, 160, this->title.c_str());
    this->form->begin();
    this->msg = new Fl_Box(0, 0, 320, 160, this->text.c_str());
    for (int i = 0; i < this->rBtns.size(); ++i)
    {
      Button &b = this->rBtns[i];
      if (i)
        b.widget = new Fl_Button(300, 140, 20, 20, b.text.c_str());
      else
        b.widget = new Fl_Return_Button(300, 140, 20, 20, b.text.c_str());
    }
    for (auto &b : this->lBtns)
      b.widget = new Fl_Button(0, 140, 20, 20, b.text.c_str());
    for (auto &o : this->options)
      o.widget = new Fl_Check_Button(0, 0, 320, 160, o.text.c_str());
    this->form->end();

    this->msg->align(alignment);
    for (auto &o : this->options)
    {
      o.widget->align(alignment);
      o.widget->callback(o.cb, this);
      o.widget->value(o.pressed ? 1 : 0);
    }
    for (auto &b : this->lBtns)
    {
      b.widget->align(alignment);
      b.widget->callback(b.cb, this);
    }
    for (auto &b : this->rBtns)
    {
      b.widget->align(alignment);
      b.widget->callback(b.cb, this);
    }

    this->resize();
    this->form->set_modal();
  }

  static const int pad = 5;
  static const int extra = 5 * pad;
  struct Size
  {
    int x = 0, y = 0;
  };
  Size measure(Fl_Widget *w, int lim_x = 0)
  {
    fl_font(w->labelfont(), w->labelsize());
    Size sz;
    sz.x = lim_x;
    fl_measure(w->label(), sz.x, sz.y);
    sz.x += 2 * pad;
    sz.y += 2 * pad;
    return sz;
  }


  void resize()
  {
#define _SUM_BTNS(s)        \
  assert(allBtns.y == s.y); \
  allBtns.x += s.x + pad;   \
  allBtns.y = s.y;
    Size allBtns;
    allBtns.x = extra;
    for (auto &b : this->lBtns)
    {
      b.size = measure(b.widget);
      _SUM_BTNS(b.size);
    }
    for (auto &b : this->rBtns)
    {
      b.size = measure(b.widget);
      if (b == this->rBtns[0])
        b.size.x += b.size.y; // For return icon
      _SUM_BTNS(b.size);
    }
#undef _SUM_BTNS

    Size msgSize = measure(this->msg);
    if (msgSize.x < allBtns.x)
      msgSize.x = allBtns.x;

    Size wndSize = msgSize;

    for (auto &o : this->options)
    {
      o.size = measure(o.widget, wndSize.x);
      wndSize.y += o.size.y + pad;
    }

    wndSize.y += pad + allBtns.y;
    wndSize.x += 2 * pad;
    wndSize.y += 2 * pad;

    Size scrnPos, scrnSize;
    Fl::screen_xywh(scrnPos.x, scrnPos.y, scrnSize.x, scrnSize.y);
    this->form->resize(scrnPos.x + (scrnSize.x - wndSize.x) / 2, scrnPos.y + (scrnSize.y - wndSize.y) / 2, wndSize.x, wndSize.y);
    this->form->size_range(wndSize.x, wndSize.y, wndSize.x, wndSize.y);

    this->msg->resize(pad, pad, msgSize.x, msgSize.y);
    int top = pad + msgSize.y;
    for (auto &o : this->options)
    {
      top += pad;
      o.widget->resize(pad, top, msgSize.x, o.size.y);
      top += o.size.y;
    }

    int left = 0, bot = wndSize.y - pad - allBtns.y;
    for (auto &b : this->lBtns)
    {
      left += pad;
      b.widget->resize(left, bot, b.size.x, allBtns.y);
      left += b.size.x;
    }

    int right = wndSize.x;
    for (auto &b : this->rBtns)
    {
      right -= pad + b.size.x;
      b.widget->resize(right, bot, b.size.x, allBtns.y);
    }
  }

protected:
  breakpad::Configuration &cfg;

private:
  std::string title, text;
  Response response = Response::Close;
  bool shouldQuit = false;
  Fl_Window *form = nullptr;
  Fl_Box *msg = nullptr;

  struct Button
  {
    Button(const char *t, Fl_Callback *c, bool p = false) : text(t), cb(c), pressed(p) {} // VOID
    const std::string text;
    Fl_Button *widget = nullptr;
    Fl_Callback *cb = nullptr;
    Size size;
    bool pressed = false;
    bool operator==(const Button &rhs) const { return this->widget == rhs.widget; }
  };

  std::vector<Button> options;
  std::vector<Button> lBtns;
  std::vector<Button> rBtns;
}; // class BaseWindow


class QueryWindow : public BaseWindow
{
public:
  static void on_btn_send(Fl_Widget *, void *udata) { ((QueryWindow *)udata)->quit(Response::Send); }
  static void on_opt_logs(Fl_Widget *w, void *udata)
  {
    QueryWindow *self = (QueryWindow *)udata;
    Fl_Check_Button *btn = static_cast<Fl_Check_Button *>(w);
    self->cfg.sendLogs = btn->value();
  }
  static void on_opt_email(Fl_Widget *w, void *udata)
  {
    QueryWindow *self = (QueryWindow *)udata;
    Fl_Check_Button *btn = static_cast<Fl_Check_Button *>(w);
    self->cfg.allowEmail = btn->value();
  }

public:
  QueryWindow(breakpad::Configuration &c) :
    BaseWindow(c, c.productTitle + lang(UI_TXT_SUBMISSION_TITLE), lang(UI_TXT_SUBMISSION_QUERY))
  {
    this->addOption(lang(UI_TXT_SUBMISSION_ALLOW_LOGS), on_opt_logs, c.sendLogs);
    this->addOption(lang(UI_TXT_SUBMISSION_ALLOW_EMAIL), on_opt_email, c.allowEmail);

    if (c.parent.empty())
      this->addBtnRight(lang(UI_TXT_BTN_SEND_AND_CLOSE), on_btn_send);
    else
    {
      this->addBtnRight(lang(UI_TXT_BTN_SEND_AND_RESTART), on_btn_send);
      this->addBtnRight(lang(UI_TXT_BTN_RESTART), BaseWindow::on_btn_restart);
    }

    this->addBtnLeft(lang(UI_TXT_BTN_CLOSE), BaseWindow::on_btn_close);
  }
}; // struct QueryWindow


class NotifyWindow : public BaseWindow
{
public:
  static void on_btn_copy(Fl_Widget *, void *udata)
  {
    const char *uuid = ((NotifyWindow *)udata)->getUuid();
    Fl::copy(uuid, strlen(uuid), 0);
    Fl::copy(uuid, strlen(uuid), 1);
  }

public:
  NotifyWindow(breakpad::Configuration &c, const std::string &m, bool s) :
    BaseWindow(c, c.productTitle + lang(UI_TXT_DELIVERY_TITLE), (s ? lang(UI_TXT_DELIVERY_OK) : lang(UI_TXT_DELIVERY_FAILED)) + m),
    uuid(s ? m : "")
  {
    this->addBtnRight(lang(UI_TXT_BTN_CLOSE), BaseWindow::on_btn_close);
    if (!c.parent.empty())
      this->addBtnRight(lang(UI_TXT_BTN_RESTART), BaseWindow::on_btn_restart);

    if (s)
      this->addBtnLeft(lang(UI_TXT_BTN_COPY_UUID), on_btn_copy);
  }

  const char *getUuid() { return this->uuid.c_str(); }

private:
  std::string uuid;
}; // class NotifyWindow


Response notify(breakpad::Configuration &cfg, const std::string &msg, bool success)
{
  NotifyWindow wnd(cfg, msg, success);
  return wnd.show();
}


Response query(breakpad::Configuration &cfg)
{
  QueryWindow wnd(cfg);
  return wnd.show();
}

} // namespace ui
} // namespace breakpad
