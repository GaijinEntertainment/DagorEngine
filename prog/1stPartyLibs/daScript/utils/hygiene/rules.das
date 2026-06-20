options gen2

module rules shared private

require rule_private_docs public
require rule_collapse_blank_lines public


def public apply_all_rules(src : string) : string {
    return collapse_blank_lines(trim_private_docstrings(src))
}
