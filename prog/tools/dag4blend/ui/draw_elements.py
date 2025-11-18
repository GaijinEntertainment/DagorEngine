#This file contains reusable blocks from drawing custom UI


# draws custom header, controlled by bool value or element of bool array
# control value is used for non-inicialized properties.
# control.name always exists; control[name] is None until user sets some value via owner.name
def draw_custom_header(layout, label, control_owner, control_name, index = None, icon = 'BLANK1', is_api_defined = True, label_offset = 0):
    header = layout.row(align = True)
    if is_api_defined:
        value = getattr(control_owner, control_name)
    else:
        value = control_owner[control_name]
    if index is None:
        header.prop(control_owner, control_name, text = "", emboss = False, expand = True,
                    icon = 'DOWNARROW_HLT' if value else 'RIGHTARROW_THIN')
        for i in range(label_offset):  # used to align text, when extra buttons added on the right
            header.prop(control_owner, control_name, text = "",emboss = False, expand = True, icon = 'BLANK1')
        header.prop(control_owner, control_name, text = label, emboss = False, expand = True)
        header.prop(control_owner, control_name, text = "",    emboss = False, expand = True, icon = icon)
    else:
        value = value[index]
        header.prop(control_owner, control_name, text = "", emboss = False, expand = True, index = index,
                    icon = 'DOWNARROW_HLT' if value else 'RIGHTARROW_THIN')
        for i in range(label_offset):  # used to align text, when extra buttons added on the right
            header.prop(control_owner, control_name, text = "",index = index,emboss = False, expand = True, icon = 'BLANK1')
        header.prop(control_owner, control_name, text = label, index = index, emboss = False, expand = True)
        header.prop(control_owner, control_name, text = "",    index = index, emboss = False, expand = True, icon = icon)
    return header

def draw_custom_toggle(layout, property_owner, property_name, is_api_defined = True):
    row = layout.row()
    if is_api_defined:
        value = getattr(property_owner, property_name)
        row.prop(property_owner,property_name, toggle = True,
                    icon = 'CHECKBOX_HLT' if value else 'CHECKBOX_DEHLT')
    else:
        value = property_owner[property_name]
        row.prop(property_owner, f'["{property_name}"]', toggle = True,
                    icon = 'CHECKBOX_HLT' if value else 'CHECKBOX_DEHLT')
    return