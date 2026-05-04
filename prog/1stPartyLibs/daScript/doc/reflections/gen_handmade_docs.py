#!/usr/bin/env python3
"""
Generate proper documentation for daslang handmade RST stubs.

Reads stubs from handmade/ and detail/ (//! comment output),
and writes improved documentation.

Usage:
    python gen_handmade_docs.py [--dry-run] [--filter PATTERN]
"""

import os
import re
import sys
import glob
import argparse

DAS_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
HANDMADE = os.path.join(DAS_ROOT, "doc", "source", "stdlib", "handmade")
DETAIL = os.path.join(DAS_ROOT, "doc", "source", "stdlib", "detail")

STUB_MARKER = "// stub"

# ---- Utility helpers --------------------------------------------------------

def read_file(path):
    """Read file content, return None if missing."""
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            return f.read()
    except FileNotFoundError:
        return None

def write_file(path, content):
    """Write content to file."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)

def parse_stub(content):
    """Parse a stub file into (is_stub, declaration_line, rest).
    Returns (True, decl, rest) if starts with // stub, else (False, '', content).
    """
    if content is None:
        return (True, "", "")
    lines = content.replace("\r\n", "\n").split("\n")
    # Remove trailing empty lines
    while lines and lines[-1].strip() == "":
        lines.pop()
    if lines and lines[0].strip() == STUB_MARKER:
        decl = lines[1] if len(lines) > 1 else ""
        rest = "\n".join(lines[2:]) if len(lines) > 2 else ""
        return (True, decl, rest)
    return (False, "", "\n".join(lines))

def parse_filename(fname):
    """Parse handmade filename into (category, module, name).
    e.g. 'function-builtin-push-0xABCD.rst' -> ('function', 'builtin', 'push-0xABCD')
    e.g. 'module-builtin.rst' -> ('module', 'builtin', '')
    e.g. 'structure_annotation-ast-ExprBlock.rst' -> ('structure_annotation', 'ast', 'ExprBlock')
    """
    base = fname.replace(".rst", "")

    # Categories with underscores must be matched first
    for prefix in ["structure_annotation", "function_annotation", "structure_macro",
                    "typeinfo_macro", "reader_macro", "variant_macro", "call_macro",
                    "any_annotation"]:
        if base.startswith(prefix + "-"):
            rest = base[len(prefix) + 1:]
            parts = rest.split("-", 1)
            module = parts[0]
            name = parts[1] if len(parts) > 1 else ""
            return (prefix, module, name)

    # Simple categories
    parts = base.split("-", 2)
    category = parts[0]
    module = parts[1] if len(parts) > 1 else ""
    name = parts[2] if len(parts) > 2 else ""
    return (category, module, name)

def get_old_content(fname):
    """Legacy stub — handmade_old/ has been removed.  Always returns None."""
    return None

def get_detail_content(category, module, name):
    """Get content from detail/ directory (from //! comments).
    Detail files use the same naming convention.
    """
    if not name:
        fname = f"{category}-{module}.rst" if module else f"{category}.rst"
    else:
        fname = f"{category}-{module}-{name}.rst"
    path = os.path.join(DETAIL, fname)
    content = read_file(path)
    if content is None:
        return None
    # Detail files are RST substitution definitions for stubs
    if "to be documented" in content:
        return None
    return content.strip()

def get_detail_function_content(module, name):
    """Search detail/ for function files matching module and name prefix.
    Function files have hashes, so we need glob matching.
    """
    # name includes the hash, so try exact match first
    fname = f"function-{module}-{name}.rst"
    path = os.path.join(DETAIL, fname)
    content = read_file(path)
    if content and "to be documented" not in content:
        return content.strip()
    return None

def clean_old_text(text):
    """Clean up old documentation text - fix wording, remove leading spaces."""
    if not text:
        return text
    # Remove leading space that some old files have
    text = text.strip()
    # Remove redundant "// stub" lines that crept in
    if text.startswith(STUB_MARKER):
        return None
    return text


# ---- Category-specific documentation generators ----------------------------

# Module descriptions from //! comments in daslib/*.das files
MODULE_DESCRIPTIONS = {}

def load_module_descriptions():
    """Load //! module descriptions from daslib/*.das files."""
    daslib = os.path.join(DAS_ROOT, "daslib")
    for f in sorted(glob.glob(os.path.join(daslib, "*.das"))):
        modname = os.path.basename(f).replace(".das", "")
        lines = []
        with open(f, "r", encoding="utf-8", errors="replace") as fh:
            in_module_comment = False
            for line in fh:
                line = line.rstrip()
                if line.startswith("//!"):
                    in_module_comment = True
                    text = line[3:].lstrip() if len(line) > 3 else ""
                    lines.append(text)
                elif in_module_comment:
                    break  # End of first //! block
                elif line.startswith("module ") or line.startswith("options ") or line.startswith("require ") or line == "":
                    continue  # Skip module/options/require/blank before //!
                else:
                    break  # Non-comment, non-header line
        if lines:
            MODULE_DESCRIPTIONS[modname] = " ".join(l for l in lines if l).strip()


def generate_module_doc(module, old_content, stub_decl):
    """Generate module description documentation."""
    # Use old content if good, otherwise use //! description
    desc = None
    if old_content:
        desc = clean_old_text(old_content)

    if not desc or desc.startswith(STUB_MARKER) or desc == f"Module {module}" or desc == f"Module $":
        # Try //! module description
        modname = module if module != "builtin" and module != "$" else None
        if modname and modname in MODULE_DESCRIPTIONS:
            desc_text = MODULE_DESCRIPTIONS[modname]
            mod_upper = modname.upper()
            desc = f"The {mod_upper} module {desc_text[0].lower()}{desc_text[1:]}" if desc_text else None
            if desc:
                desc += f"\n\nAll functions and symbols are in \"{modname}\" module, use require to get access to it. ::\n\n    require daslib/{modname}"

    if not desc:
        # Fallback: generate from module name
        display = module if module != "$" else "builtin"
        desc = f"Module {display}"

    return desc


def generate_enum_doc(module, name, old_content, stub_decl, detail_content):
    """Generate enumeration documentation with correct line count.
    Must produce exactly 1 header line + N value lines.
    """
    if old_content:
        cleaned = clean_old_text(old_content)
        if cleaned:
            return cleaned

    if detail_content:
        return detail_content

    # Generate placeholder: we need to know the field count
    # The stub only has "enum EnumName", field count will be validated by das2rst
    # We'll generate a generic description
    return f"Enumeration {name}."


def generate_structure_doc(module, name, old_content, stub_decl, detail_content):
    """Generate structure documentation with correct line count."""
    if old_content:
        cleaned = clean_old_text(old_content)
        if cleaned:
            return cleaned

    if detail_content:
        return detail_content

    return f"Structure {name}."


def generate_class_doc(module, name, old_content, stub_decl, detail_content):
    """Generate class documentation."""
    if old_content:
        cleaned = clean_old_text(old_content)
        if cleaned:
            return cleaned

    if detail_content:
        return detail_content

    return f"Class {name}."


def generate_function_doc(module, name, old_content, stub_decl, detail_content):
    """Generate function documentation."""
    if old_content:
        cleaned = clean_old_text(old_content)
        if cleaned:
            return cleaned

    if detail_content:
        return detail_content

    # Generate from declaration
    if stub_decl:
        return generate_func_desc_from_decl(name, stub_decl)

    return f"Function {name}."


def generate_func_desc_from_decl(name, decl):
    """Generate a function description from its declaration stub line."""
    # Parse "def funcname (args...) : rettype"
    # Extract the base function name (strip hash suffix)
    func_name = name.split("-0x")[0] if "-0x" in name else name
    # Remove dots and backticks from property names
    func_name = func_name.replace(".`", ".").replace("`", "")

    return f"{func_name}."


def generate_typedef_doc(module, name, old_content, stub_decl, detail_content):
    """Generate typedef documentation."""
    if old_content:
        cleaned = clean_old_text(old_content)
        if cleaned:
            return cleaned

    if detail_content:
        return detail_content

    # Parse stub_decl: "typedef Name = type aka Name"
    if stub_decl and "=" in stub_decl:
        parts = stub_decl.split("=", 1)
        type_desc = parts[1].strip()
        # Remove " aka Name" suffix
        if " aka " in type_desc:
            type_desc = type_desc.split(" aka ")[0].strip()
        return f"Type alias for ``{type_desc}``."

    return f"Typedef {name}."


def generate_annotation_doc(category, module, name, old_content, stub_decl, detail_content):
    """Generate annotation/macro documentation."""
    if old_content:
        cleaned = clean_old_text(old_content)
        if cleaned:
            return cleaned

    if detail_content:
        return detail_content

    # Generate category-appropriate description
    cat_labels = {
        "function_annotation": f"Function annotation ``[{name}]``.",
        "structure_annotation": f"Annotation {name}.",
        "structure_macro": f"Structure annotation ``[{name}]``.",
        "annotation": f"Annotation ``{name}``.",
        "reader_macro": f"Reader macro ``%{name}``.",
        "variant_macro": f"Variant macro ``{name}``.",
        "typeinfo_macro": f"Typeinfo macro ``{name}``.",
        "call_macro": f"Call macro ``{name}``.",
    }
    return cat_labels.get(category, f"{category} {name}.")


def generate_variable_doc(module, name, old_content, stub_decl, detail_content):
    """Generate variable/constant documentation."""
    if old_content:
        cleaned = clean_old_text(old_content)
        if cleaned:
            return cleaned

    if detail_content:
        return detail_content

    return f"Constant ``{name}``."


# ---- Main processing --------------------------------------------------------

def process_file(fname, dry_run=False):
    """Process a single handmade stub file."""
    filepath = os.path.join(HANDMADE, fname)
    content = read_file(filepath)
    if content is None:
        return "SKIP", "file not found"

    is_stub, stub_decl, stub_rest = parse_stub(content)
    if not is_stub:
        return "KEEP", "already documented"

    category, module, name = parse_filename(fname)

    # Get reference material
    old_content = get_old_content(fname)

    # Get detail content (from //! comments)
    detail_content = None
    if category == "function":
        detail_content = get_detail_function_content(module, name)
    elif name:
        detail_content = get_detail_content(category, module, name)

    # Generate new documentation
    new_content = None

    if category == "module":
        new_content = generate_module_doc(module, old_content, stub_decl)
    elif category == "enumeration":
        new_content = generate_enum_doc(module, name, old_content, stub_decl, detail_content)
    elif category == "structure":
        new_content = generate_structure_doc(module, name, old_content, stub_decl, detail_content)
    elif category == "class":
        new_content = generate_class_doc(module, name, old_content, stub_decl, detail_content)
    elif category == "function":
        new_content = generate_function_doc(module, name, old_content, stub_decl, detail_content)
    elif category == "typedef":
        new_content = generate_typedef_doc(module, name, old_content, stub_decl, detail_content)
    elif category == "Variable":
        new_content = generate_variable_doc(module, name, old_content, stub_decl, detail_content)
    elif category in ("function_annotation", "structure_annotation", "structure_macro",
                       "annotation", "reader_macro", "variant_macro", "typeinfo_macro",
                       "call_macro", "any_annotation"):
        new_content = generate_annotation_doc(category, module, name, old_content, stub_decl, detail_content)
    else:
        return "SKIP", f"unknown category: {category}"

    if new_content is None:
        return "SKIP", "no content generated"

    # Write the new file
    if not dry_run:
        write_file(filepath, new_content + "\n")

    source = "old" if old_content else ("detail" if detail_content else "generated")
    return "WRITE", f"source={source}"


def main():
    parser = argparse.ArgumentParser(description="Generate handmade RST documentation")
    parser.add_argument("--dry-run", action="store_true", help="Don't write files")
    parser.add_argument("--filter", type=str, default="", help="Filter by filename pattern")
    parser.add_argument("--stats", action="store_true", help="Print statistics only")
    args = parser.parse_args()

    load_module_descriptions()
    print(f"Loaded {len(MODULE_DESCRIPTIONS)} module descriptions from //! comments")

    files = sorted(os.listdir(HANDMADE))
    files = [f for f in files if f.endswith(".rst")]

    if args.filter:
        files = [f for f in files if args.filter in f]

    stats = {"WRITE": 0, "KEEP": 0, "SKIP": 0}
    sources = {"old": 0, "detail": 0, "generated": 0}

    for fname in files:
        action, msg = process_file(fname, dry_run=args.dry_run)
        stats[action] = stats.get(action, 0) + 1

        if action == "WRITE" and "source=" in msg:
            src = msg.split("source=")[1]
            sources[src] = sources.get(src, 0) + 1

        if not args.stats and action != "KEEP":
            print(f"  {action}: {fname} ({msg})")

    print(f"\n=== Summary ===")
    print(f"  Total files: {len(files)}")
    print(f"  Written:     {stats.get('WRITE', 0)}")
    print(f"  Kept:        {stats.get('KEEP', 0)}")
    print(f"  Skipped:     {stats.get('SKIP', 0)}")
    print(f"\n  Sources: old={sources.get('old', 0)}, detail={sources.get('detail', 0)}, generated={sources.get('generated', 0)}")


if __name__ == "__main__":
    main()
