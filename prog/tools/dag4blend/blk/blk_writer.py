from .datablock import datablock


def ensure_quotation_marks(str):
    if not str.startswith('"'):
        str = '"' + str
    if not str.endswith('"'):
        str += '"'
    return str

def write_datablock(datablock, text, tabs = 0):
    if datablock.parent is not None:
        text.write("  " * tabs)
        text.write(f"{datablock.type}")
        text.write("{\n")
        tabs += 1
    for include in datablock.includes:
        text.write("  "*tabs)
        text.write(f'include "{include}"\n')
    parameters = datablock.parameters
    for param in datablock.parameters:
        text.write("  " * tabs)
        text.write(f"{param}")
        if datablock.parameters[param].get("type"):
            text.write(f":{parameters[param]['type']}")
        if parameters[param]['type'] in ['t', 'text']:
            value = ensure_quotation_marks(parameters[param]['value'])
        else:
            value = parameters[param]['value']
        text.write(f"={value}\n")
    for child in datablock.datablocks:
        write_datablock(child, text, tabs)
    if datablock.parent is not None:
        tabs -= 1
        text.write("  " * tabs + "}\n")
    return
