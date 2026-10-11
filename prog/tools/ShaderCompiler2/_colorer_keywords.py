#!/usr/bin/env python3
"""Sync the Colorer .hrc keyword lists with the actual dsc2 implementation.

Sources of truth
----------------
* DSHL keywords  - the `keyword` rule in shlex.dlp.  That rule is what stops a
  word being usable as an identifier, which is exactly the set worth colouring.
  (Do not use the grammar's terminal list: shsyn.whl still declares `fsh_4_1`,
  but the lexer emits no token for it, so it is dead.)
* `@` binding types - the PRESHADER_VARIABLE_TYPE_LIST X-macro in
  variantSemantic.h, which variantSemantic.h:315 turns into the {"@"#type ->
  VariableType} lookup table.

Usage
-----
    python _colorer_keywords.py            rewrite _colorer4 and _colorer5
    python _colorer_keywords.py --check    report drift, exit 1 if any

CATEGORIES below assigns every keyword to a colour group. The grouping follows
the grammar rule each keyword appears in (shsyn.whl); the comment on each group
names those rules. When dsc2 gains or loses a keyword this script fails loudly
until the new word is placed in a group.

When to run
-----------
After any dsc2 change that adds or removes a keyword or an `@` binding type.
Both .hrc files are rewritten together and are meant to stay in lockstep, so do
not hand edit the generated keyword blocks or the `@` rules - the next run would
overwrite the edit. `--check` exits non-zero on drift and is meant for CI.

What this script does NOT manage
--------------------------------
Only the keyword lists and the `@` binding rules are generated. Everything else
in the two .hrc files - schemes, blocks, the hlsl block rules, the "bad shader
stage" error regexps, region declarations - is hand written, and a change there
has to be made in both files by hand. The dialects differ; the <annotation> of
_colorer5/dshl.hrc lists the 4ever/take5 spellings that trip people up.

Verifying a change
------------------
4ever can be rendered without Far, using the console colorizer that ships with
Far 2.0, so grammar edits can be diffed before/after:

    <Far2>\\Plugins\\Colorer\\bin\\colorer.exe -c -h some_file.dshl

take5 has no console tool - check it in Far3.
"""
import os, re, sys, textwrap

# comments in the generated .hrc blocks wrap at this column
COMMENT_WIDTH = 110

HERE = os.path.dirname(os.path.abspath(__file__))
DLP = os.path.join(HERE, 'shlex.dlp')
VSEM = os.path.join(HERE, 'variantSemantic.h')

# --- colour groups -----------------------------------------------------------
# Group -> keywords. The 4ever region names are used as the canonical keys;
# TARGETS maps them to each Colorer dialect's region names.
CATEGORIES = {
    # declarations, shader structure, directives, resource types.
    # rules: shader-decl block-decl supports-stat channel-decl hlsl-compile
    # interval-decl local-decl static-var-mode global-decl static-decl
    # bool-value assume-stat error-stat render-stage-stat shader-directive
    # external-register raytrace-pipeline-stat immediate-const-block
    # static-var-type sampler-decl sh-mat-texture
    'dKeyword': """
        shader block supports channel compile interval local static dynamic
        const global literal always_referenced no_warnings if else hardware
        maybe two_sided real_two_sided assume assume_if_not_assumed error
        render_stage dont_render no_ablend no_dynstcode render_trans
        signed_pack unsigned_pack mul_1k mul_2k mul_4k mul_8k mul_16k mul_32767
        bounding_pack register max_recursion allow_opacity_micromaps material
        immediate_dword_count texture sampler buffer const_buffer tlas
        pixel vertex render_pass multi_render_pass framebuffer_layout subpass pass
    """,
    # value types: channel-type plus the scalar/vector/matrix variable types
    # rules: channel-type local-var-type static-var-type bool-decl
    'dType': """
        float float1 float2 float3 float4 float4x3 float4x4 int int4 bool
        short2 short4 ubyte4 color8 short2n short4n ushort2n ushort4n
        half2 half4 udec3 dec3n
    """,
    # render states, vertex channel usages and material colours
    # rules: state-var zbias-state-stat channel-usage sh-mat-color
    'dState': """
        blend_src blend_dst blend_asrc blend_adst blend_op blend_aop
        blend_factor cull_mode alpha_to_coverage view_instances stencil
        stencil_func stencil_ref stencil_pass stencil_fail stencil_zfail
        color_write z_write z_test z_func z_bias slope_z_bias
        pos norm tc vcol extra diffuse emissive specular ambient
    """,
    # values those states / supports / assume take
    # rules: hardware-support state-plain-value bool-value external-var-name
    'dStateValue': """
        pc dx11 dx12 xbox scarlett vulkan metal metaliOS android ps4 ps5
        fsh_4_0 fsh_5_0 fsh_6_0 fsh_6_2 fsh_6_6 bindless mesh
        none true false NULL
        globtm projtm viewprojtm
        local_view_x local_view_y local_view_z local_view_pos
        world_local_x world_local_y world_local_z world_local_pos
    """,
    # preprocessor-level, handled by shlexterm.cpp / shMacro.cpp
    'dDirective': """
        include include_optional macro endmacro define_macro_if_not_defined
    """,
}

# Built-in sources usable as an @-binding value inside an hlsl{} block.
HLSL_BUILTINS = """
    immediate_dword_count
    globtm projtm viewprojtm
    local_view_x local_view_y local_view_z local_view_pos
    world_local_x world_local_y world_local_z world_local_pos
"""

TARGETS = {
    '4ever': {
        'path': os.path.join(HERE, '_colorer4', 'dshl.hrc'),
        'encoding': 'cp1251',
        'regions': {k: k for k in CATEGORIES},
        'hlsl_region': 'dHlslPreProc2',
        'at_regions': ('dKeyword', 'dString'),
        'word_end': r'\B',          # 4ever: \B == end of word
    },
    'take5': {
        'path': os.path.join(HERE, '_colorer5', 'dshl.hrc'),
        'encoding': 'utf-8',
        'regions': {'dKeyword': 'Keyword', 'dType': 'Type', 'dState': 'State',
                    'dStateValue': 'StateValue', 'dDirective': 'Directive'},
        'hlsl_region': 'HlslPreProc2',
        'at_regions': ('Keyword', 'String'),
        'word_end': r'\b',          # take5: Perl word boundary
    },
}


def dshl_keywords():
    s = re.sub(r'//[^\n]*', '', open(DLP, encoding='utf-8', errors='replace').read())
    m = re.search(r'^keyword\s*->(.*?);\s*$', s, re.S | re.M)
    if not m:
        sys.exit('could not find the `keyword` rule in shlex.dlp')
    return set(re.findall(r"'([^']+)'", m.group(0)))


def at_types():
    """-> [(group, [type, ...]), ...] in the order variantSemantic.h declares them."""
    s = open(VSEM, encoding='utf-8', errors='replace').read()
    block = re.search(r'#define PRESHADER_VARIABLE_TYPE_LIST_FLOAT.*?'
                      r'#define PRESHADER_VARIABLE_TYPE_LIST\b', s, re.S)
    if not block:
        sys.exit('could not find PRESHADER_VARIABLE_TYPE_LIST in variantSemantic.h')
    groups = []
    for m in re.finditer(r'#define PRESHADER_VARIABLE_TYPE_LIST_(\w+)\(\)((?:[^\n\\]|\\\n)*)',
                         block.group(0)):
        names = re.findall(r'TYPE\((\w+)\)', m.group(2))
        if names:
            groups.append((m.group(1), names))
    if not groups:
        sys.exit('could not parse the PRESHADER_VARIABLE_TYPE_LIST_* groups')
    return groups


def build_keyword_block(words, region, indent, ignorecase=False):
    ic = ' ignorecase="yes"' if ignorecase else ''
    lines = ['%s<keywords region="%s"%s>' % (indent, region, ic)]
    lines += ['%s   <word name="%s"/>' % (indent, w) for w in words]
    lines += ['%s</keywords>' % indent]
    return '\n'.join(lines)


def replace_keywords(text, region, words, ignorecase=False):
    pat = re.compile(r'([ \t]*)<keywords region="%s"([^>]*)>.*?</keywords>'
                     % re.escape(region), re.S)
    m = pat.search(text)
    if not m:
        sys.exit('no <keywords region="%s"> block found' % region)
    indent = m.group(1)
    ic = ignorecase or 'ignorecase' in m.group(2)
    return pat.sub(lambda _: build_keyword_block(words, region, indent, ic), text, count=1)


def replace_at_regexps(text, groups, cfg):
    """Regenerate the @-type rules: one per PRESHADER_VARIABLE_TYPE_LIST_* group,
    in the order variantSemantic.h declares them, so a rule here lines up with a
    group there and diffs stay readable."""
    kw, st = cfg['at_regions']
    # drop the header this function wrote last time, so re-running is a no-op
    text = re.sub(r'[ \t]*<!-- @-binding types,.*?-->\n', '', text, flags=re.S)
    pat = re.compile(r'([ \t]*)<regexp match=\'/\\b\[a-zA-Z_\]\\w\*\\s\*\(@\).*?\n', re.S)
    hits = pat.findall(text)
    if not hits:
        sys.exit('no @-binding regexps found')
    indent = hits[0]

    header = ('@-binding types, one rule per PRESHADER_VARIABLE_TYPE_LIST_* group in '
              'variantSemantic.h plus the state-block binding types. Generated by '
              '_colorer_keywords.py. Longest alternative '
              'first (texCubeArray before texCube before tex), and every branch bracketed: '
              '4ever matches nothing at all for a bare a|b|c in a match regexp.')
    lines = textwrap.wrap(header, COMMENT_WIDTH - len(indent) - 5,
                          initial_indent=indent + '<!-- ',
                          subsequent_indent=indent + '     ')
    lines[-1] += ' -->'
    for name, types in groups:
        # longest first so e.g. texCubeArray wins over texCube and tex
        alt = '|'.join('(%s)' % t for t in sorted(types, key=lambda t: (-len(t), t)))
        lines.append("%s<regexp match='/\\b[a-zA-Z_]\\w*\\s*(@)(%s)%s/' "
                     "region1=\"%s\" region2=\"%s\"/> <!-- %s -->"
                     % (indent, alt, cfg['word_end'], kw, st, name))
    rule = '\n'.join(lines) + '\n'

    text = pat.sub('\x00', text, count=len(hits))
    first = text.index('\x00')
    return text[:first] + rule + text[first:].replace('\x00', '')


def apply(cfg, keywords, types, check):
    text = original = open(cfg['path'], encoding=cfg['encoding']).read()
    for group, region in cfg['regions'].items():
        text = replace_keywords(text, region, sorted(CATEGORIES[group].split()))
    text = replace_keywords(text, cfg['hlsl_region'], sorted(HLSL_BUILTINS.split()))
    text = replace_at_regexps(text, types, cfg)
    name = os.path.basename(os.path.dirname(cfg['path']))
    if text == original:
        print('  %-10s up to date' % name)
        return False
    if check:
        print('  %-10s OUT OF DATE' % name)
        return True
    open(cfg['path'], 'w', encoding=cfg['encoding'], newline='\n').write(text)
    print('  %-10s updated' % name)
    return True


def main():
    check = '--check' in sys.argv
    keywords = dshl_keywords()
    types = at_types()
    types.append(('STATE_BLOCK', ['ps_output']))

    categorised = set()
    for g, words in CATEGORIES.items():
        for w in words.split():
            if w in categorised:
                sys.exit('keyword %r listed in two groups' % w)
            categorised.add(w)

    # every @-binding source usable inside an hlsl{} block is itself a DSHL
    # keyword, so a name dropped from dsc2 must not linger here either
    stale_builtins = sorted(set(HLSL_BUILTINS.split()) - keywords)
    if stale_builtins:
        sys.exit('HLSL_BUILTINS names dsc2 no longer knows: %s'
                 % ' '.join(stale_builtins))

    missing = sorted(keywords - categorised)
    extra = sorted(categorised - keywords)
    if missing or extra:
        if missing:
            print('dsc2 has keywords this script does not colour:', ' '.join(missing))
        if extra:
            print('this script colours words dsc2 no longer knows:', ' '.join(extra))
        sys.exit('CATEGORIES is out of sync with shlex.dlp - fix it and re-run')

    n_types = sum(len(v) for _, v in types)
    print('%d DSHL keywords, %d @-binding types in %d groups'
          % (len(keywords), n_types, len(types)))
    drift = [apply(cfg, keywords, types, check) for cfg in TARGETS.values()]
    if check and any(drift):
        sys.exit(1)


if __name__ == '__main__':
    main()
