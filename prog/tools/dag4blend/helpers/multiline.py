import textwrap
def _label_multiline(context, text, parent):
    chars = int(context.region.width / 7)
    wrapper = textwrap.TextWrapper(width=chars)
    text_lines = wrapper.wrap(text=text)
    for text_line in text_lines:
        parent.label(text=text_line)