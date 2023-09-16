import bpy

def get_text_clear(text):
    if bpy.data.texts.get(text) is None:
        bpy.data.texts.new(name=text)
    text=bpy.data.texts[text]
    text.clear()
    return text

def get_text(text):
    if bpy.data.texts.get(text) is None:
        bpy.data.texts.new(name=text)
    text=bpy.data.texts[text]
    return text