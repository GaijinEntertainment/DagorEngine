#pragma once

#include <daRg/dag_behavior.h>
#include <sqrat.h>


namespace textlayout
{
class FormattedText;
}


namespace darg
{


class BhvTextArea : public darg::Behavior
{
public:
  BhvTextArea();

  virtual void onAttach(Element *elem);
  virtual void onDetach(Element *elem, DetachMode);
  virtual void onDelete(Element *elem);
  virtual void onElemSetup(Element *, SetupMode);

  static void recalc_content(const Element *elem, int axis, const Point2 &elem_size, Point2 &out_size);
  static int get_preformatted_flags(const Element *elem, Sqrat::Object &preformatted);
  static void update_formatted_text(textlayout::FormattedText *fmt_text, Element *elem);

private:
  static void deleteData(Element *elem);
};


extern BhvTextArea bhv_text_area;


} // namespace darg
