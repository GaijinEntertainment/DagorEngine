This macro implements embedding of the REGEX object into the AST::
  var op_regex <- %regex~operator[^a-zA-Z_]%%
Regex is compiled at the time of parsing, and the resulting object is embedded into the AST.
