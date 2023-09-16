#include <util/dag_convar.h>
#include <util/dag_console.h>
#include <imgui/imgui.h>

static ConVarBase *convars_tail = NULL;

ConVarBase::ConVarBase(const char *name_, const char *help_tip, int flags_) : name(name_), helpTip(help_tip), flags(flags_)
{
  G_ASSERT(name); // can't be NULL

  if (!isFlagSet(CVF_UNREGISTERED))
  {
    // Note on multithreading.
    // This is intentionally designed to be signleathreaded as 99.9% of (predicted) use cases of convars are
    // global/static vars which are obviously single threaded (also to avoid locking overhead).
    // If you really need to create/destroy convars asynchronously - it's you job as a programmer to make sure
    // that all construction/destruction is done in linear fashion.
    nextVar = convars_tail;
    convars_tail = this;
  }
}

ConVarBase::~ConVarBase()
{
  if (!isFlagSet(CVF_UNREGISTERED))
  {
    for (ConVarBase **cv = &convars_tail; *cv; cv = &(*cv)->nextVar)
      if (*cv == this)
      {
        *cv = nextVar;
        return;
      }
    G_ASSERT(0); // var not found
  }
}

template <>
void ConVarT<int, true>::describeValue(char *buf, size_t buf_size) const
{
  snprintf(buf, buf_size, "%s = %d [%d..%d]", getName(), get(), getMin(), getMax());
}
template <>
void ConVarT<int, false>::describeValue(char *buf, size_t buf_size) const
{
  snprintf(buf, buf_size, "%s = %d", getName(), get());
}

template <>
void ConVarT<float, true>::describeValue(char *buf, size_t buf_size) const
{
  snprintf(buf, buf_size, "%s = %g [%g..%g]", getName(), get(), getMin(), getMax());
}
template <>
void ConVarT<float, false>::describeValue(char *buf, size_t buf_size) const
{
  snprintf(buf, buf_size, "%s = %g", getName(), get());
}

template <>
void ConVarT<bool, false>::describeValue(char *buf, size_t buf_size) const
{
  snprintf(buf, buf_size, "%s = %s", getName(), get() ? "true" : "false");
}

template <>
void ConVarT<int, true>::parseString(const char *buf)
{
  if (buf)
    set(console::ICommandProcessor::to_int(buf));
  else
  {
    int nextI = get() + 1;
    set(nextI > getMax() ? getMin() : nextI); // inc with wrap
  }
}

template <>
void ConVarT<int, false>::parseString(const char *buf)
{
  if (buf)
    set(console::ICommandProcessor::to_int(buf));
}

template <>
void ConVarT<float, true>::parseString(const char *buf)
{
  if (buf)
    set(console::ICommandProcessor::to_real(buf));
}

template <>
void ConVarT<float, false>::parseString(const char *buf)
{
  if (buf)
    set(console::ICommandProcessor::to_real(buf));
}

template <>
void ConVarT<bool, false>::parseString(const char *buf)
{
  if (buf)
    set(console::ICommandProcessor::to_bool(buf));
  else
    set(!get()); // toggle
}

template <>
bool ConVarT<int, true>::imguiWidget(const char *label_override)
{
  const char *label = label_override != nullptr ? label_override : getName();
  return ImGui::SliderInt(label, &value, getMin(), getMax());
}
template <>
bool ConVarT<int, false>::imguiWidget(const char *label_override)
{
  const char *label = label_override != nullptr ? label_override : getName();
  return ImGui::InputInt(label, &value);
}

template <>
bool ConVarT<float, true>::imguiWidget(const char *label_override)
{
  const char *label = label_override != nullptr ? label_override : getName();
  return ImGui::SliderFloat(label, &value, getMin(), getMax());
}
template <>
bool ConVarT<float, false>::imguiWidget(const char *label_override)
{
  const char *label = label_override != nullptr ? label_override : getName();
  return ImGui::DragFloat(label, &value);
}

template <>
bool ConVarT<bool, false>::imguiWidget(const char *label_override)
{
  const char *label = label_override != nullptr ? label_override : getName();
  return ImGui::Checkbox(label, &value);
}

template class ConVarT<int, false>;
template class ConVarT<int, true>;
template class ConVarT<float, false>;
template class ConVarT<float, true>;
template class ConVarT<bool, false>;

ConVarIterator::ConVarIterator() : next(convars_tail) {}
ConVarBase *ConVarIterator::nextVar()
{
  ConVarBase *ret = next;
  next = next ? next->getNextVar() : next;
  return ret;
}
