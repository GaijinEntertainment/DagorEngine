#removes /*multiline*/ and //simple comments from a single line
def line_remove_comments(starts_commented, line):
    commented = starts_commented
    fixed_line = ""
    if commented:
        c_end = line.find("*/")
        if c_end > -1:
            commented, fixed_line = line_remove_comments(False, line[c_end+2::])
    else:
        c_start = line.find("/*")
        c_simple = line.find("//")
        if c_start>-1 and (c_simple==-1 or c_start<c_simple):
                fixed_line+=line[:c_start]
                commented,line = line_remove_comments(True, line[c_start+2::])
                fixed_line+=line
        elif c_simple>-1:
            comment, line = line_remove_comments(False, line[:c_simple])
            fixed_line+=line+'\n'  # '\n' was removed with comment
        else:
            fixed_line = line
    return commented, fixed_line

#removes /*multiline*/ and //simple comments from set of lines
def text_remove_comments(lines):
    fixed_text = ""
    commented = False
    for line in lines:
        commented, fixed_line = line_remove_comments(commented, line)
        fixed_text += fixed_line
    return fixed_text
