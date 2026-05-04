#pragma once

#include <string>
#include <stdint.h>
#include <stdarg.h>
#include <vector>
#include "squirrel.h"
#include "sqvm.h"
#include "sqstate.h"
#include "squtils.h"
#include "arena.h"

class KeyValueFile;

/*
  Do we need identifiers for errors?
  Unlike warninggs, errors have no id (numeric or symbolic), cannot be turned on and off,
  cannot be of type other than ERROR.
  So instead of reportDiagnostic(), we may just have throwError() function with printf-like args
  and this would break the compilation process immediately.
  And reportDiagnostic() would do just like it does now - only report warnings.
  For convenience, adding GENERAL_COMPILE_ERROR for now - no need to add new identifiers
  to use it in a single place.
  Note how there are not benefits in separate ids for errors over GENERAL_COMPILE_ERROR -
  it already does all what is needed.
*/

#define DIAGNOSTICS \
  DEF_DIAGNOSTIC(GENERAL_COMPILE_ERROR, ERROR, LEX, -1, "", "%s"), \
  DEF_DIAGNOSTIC(LITERAL_OVERFLOW, ERROR, LEX, -1, "", "%s constant overflow"), \
  DEF_DIAGNOSTIC(LITERAL_UNDERFLOW, ERROR, LEX, -1, "", "%s constant underflow"), \
  DEF_DIAGNOSTIC(FP_EXP_EXPECTED, ERROR, LEX, -1, "", "exponent expected"), \
  DEF_DIAGNOSTIC(MALFORMED_NUMBER, ERROR, LEX, -1, "", "malformed number"), \
  DEF_DIAGNOSTIC(HEX_DIGITS_EXPECTED, ERROR, LEX, -1, "", "expected hex digits after '0x'"), \
  DEF_DIAGNOSTIC(HEX_TOO_MANY_DIGITS, ERROR, LEX, -1, "", "too many digits for a hex number"), \
  DEF_DIAGNOSTIC(OCTAL_NOT_SUPPORTED, ERROR, LEX, -1, "", "leading 0 is not allowed, octal numbers are not supported"), \
  DEF_DIAGNOSTIC(TOO_LONG_LITERAL, ERROR, LEX, -1, "", "constant too long"), \
  DEF_DIAGNOSTIC(EMPTY_LITERAL, ERROR, LEX, -1, "", "empty constant"), \
  DEF_DIAGNOSTIC(UNRECOGNISED_ESCAPER, ERROR, LEX, -1, "", "unrecognised escape char"), \
  DEF_DIAGNOSTIC(NEWLINE_IN_CONST, ERROR, LEX, -1, "", "newline in a constant"), \
  DEF_DIAGNOSTIC(UNFINISHED_STRING, ERROR, LEX, -1, "", "unfinished string"), \
  DEF_DIAGNOSTIC(HEX_NUMBERS_EXPECTED, ERROR, LEX, -1, "", "hexadecimal number expected"), \
  DEF_DIAGNOSTIC(UNEXPECTED_CHAR, ERROR, LEX, -1, "", "unexpected character '%s'"), \
  DEF_DIAGNOSTIC(INVALID_TOKEN, ERROR, LEX, -1, "", "invalid token '%s'"), \
  DEF_DIAGNOSTIC(LEX_ERROR_PARSE, ERROR, LEX, -1, "", "error parsing %s"), \
  DEF_DIAGNOSTIC(BRACE_ORDER, ERROR, LEX, -1, "", "in brace order"), \
  DEF_DIAGNOSTIC(NO_PARAMS_IN_STRING_TEMPLATE, ERROR, LEX, -1, "", "no params collected for string interpolation"), \
  DEF_DIAGNOSTIC(COMMENT_IN_STRING_TEMPLATE, ERROR, LEX, -1, "", "comments inside interpolated string are not supported"), \
  DEF_DIAGNOSTIC(EXPECTED_LEX, ERROR, LEX, -1, "", "expected %s"), \
  DEF_DIAGNOSTIC(MACRO_RECURSION, ERROR, LEX, -1, "", "recursion in reader macro"), \
  DEF_DIAGNOSTIC(INVALID_CHAR, ERROR, LEX, -1, "", "Invalid character"), \
  DEF_DIAGNOSTIC(TRAILING_BLOCK_COMMENT, ERROR, LEX, -1, "", "missing \"*/\" in comment"), \
  DEF_DIAGNOSTIC(GLOBAL_CONSTS_ONLY, ERROR, SYNTAX, -1, "", "global can be applied to const and enum only"), \
  DEF_DIAGNOSTIC(END_OF_STMT_EXPECTED, ERROR, SYNTAX, -1, "", "end of statement expected (; or lf)"), \
  DEF_DIAGNOSTIC(EXPECTED_TOKEN, ERROR, SYNTAX, -1, "", "expected '%s'"), \
  DEF_DIAGNOSTIC(INVALID_TYPE_NAME_SUGGESTION, ERROR, SYNTAX, -1, "", "Invalid type name '%s', did you mean '%s'?"), \
  DEF_DIAGNOSTIC(INVALID_TYPE_NAME, ERROR, SYNTAX, -1, "", "Invalid type name '%s'"), \
  DEF_DIAGNOSTIC(UNSUPPORTED_DIRECTIVE, ERROR, SYNTAX, -1, "", "unsupported directive '%s'"), \
  DEF_DIAGNOSTIC(EXPECTED_LINENUM, ERROR, SYNTAX, -1, "", "expected line number after #pos:"), \
  DEF_DIAGNOSTIC(EXPECTED_COLNUM, ERROR, SYNTAX, -1, "", "expected column number after #pos:<line>:"), \
  DEF_DIAGNOSTIC(TOO_BIG_AST, ERROR, SYNTAX, -1, "", "AST too big. Consider simplifying it"), \
  DEF_DIAGNOSTIC(TOO_BIG_STATIC_MEMO, ERROR, SYNTAX, -1, "", "static expression is too big"), \
  DEF_DIAGNOSTIC(MULTIPLE_DOCSTRINGS, ERROR, SYNTAX, -1, "", "multiple docstrings in a single block"), \
  DEF_DIAGNOSTIC(ASSIGN_INSIDE_FORBIDDEN, ERROR, SYNTAX, -1, "", "'=' inside '%s' is forbidden"), \
  DEF_DIAGNOSTIC(BROKEN_SLOT_DECLARATION, ERROR, SYNTAX, -1, "", "cannot break deref/or comma needed after [exp]=exp slot declaration"), \
  DEF_DIAGNOSTIC(ROOT_TABLE_FORBIDDEN, ERROR, SYNTAX, -1, "", "Access to root table is forbidden"), \
  DEF_DIAGNOSTIC(DELETE_OP_FORBIDDEN, ERROR, SYNTAX, -1, "", "Usage of 'delete' operator is forbidden. Use 'o.rawdelete(\"key\")' instead"), \
  DEF_DIAGNOSTIC(COMPILER_INTERNALS_FORBIDDEN, ERROR, SYNTAX, -1, "", "Access to compiler internals is forbidden"), \
  DEF_DIAGNOSTIC(UNINITIALIZED_BINDING, ERROR, SEMA, -1, "", "Binding '%s' must be initialized"), \
  DEF_DIAGNOSTIC(SAME_FOREACH_KV_NAMES, ERROR, SEMA, -1, "", "foreach() key and value names are the same: '%s'"), \
  DEF_DIAGNOSTIC(SCALAR_EXPECTED, ERROR, SYNTAX, -1, "", "scalar expected : %s"), \
  DEF_DIAGNOSTIC(VARARG_WITH_DEFAULT_ARG, ERROR, SYNTAX, -1, "", "function with default parameters cannot have variable number of parameters"), \
  DEF_DIAGNOSTIC(LOOP_CONTROLLER_NOT_IN_LOOP, ERROR, SEMA, -1, "", "'%s' has to be in a loop block"), \
  DEF_DIAGNOSTIC(ASSIGN_TO_EXPR, ERROR, SEMA, -1, "", "can't assign to expression"), \
  DEF_DIAGNOSTIC(BASE_NOT_MODIFIABLE, ERROR, SEMA, -1, "", "'base' cannot be modified"), \
  DEF_DIAGNOSTIC(ASSIGN_TO_BINDING, ERROR, SEMA, -1, "", "can't assign to binding '%s' (probably declaring using 'local' was intended, but 'let' was used)"), \
  DEF_DIAGNOSTIC(LOCAL_SLOT_CREATE, ERROR, SEMA, -1, "", "can't 'create' a local slot"), \
  DEF_DIAGNOSTIC(CANNOT_INC_DEC, ERROR, SEMA, -1, "", "can't '++' or '--' %s"), \
  DEF_DIAGNOSTIC(VARNAME_CONFLICTS, ERROR, SEMA, -1, "", "Variable name %s conflicts with function name"), \
  DEF_DIAGNOSTIC(ONLY_SINGLE_VARIABLE_DECLARATION, ERROR, SEMA, -1, "", "Only single variable declaration is allowed here"), \
  DEF_DIAGNOSTIC(INITIALIZATION_REQUIRED, ERROR, SEMA, -1, "", "Initialization required"), \
  DEF_DIAGNOSTIC(INVALID_ENUM, ERROR, SEMA, -1, "", "invalid enum [no '%s' field in '%s']"), \
  DEF_DIAGNOSTIC(UNKNOWN_SYMBOL, ERROR, SEMA, -1, "", "Unknown variable [%s]"), \
  DEF_DIAGNOSTIC(CANNOT_EVAL_UNARY, ERROR, SEMA, -1, "", "cannot evaluate unary-op"), \
  DEF_DIAGNOSTIC(DUPLICATE_KEY, ERROR, SEMA, -1, "", "duplicate key '%s'"), \
  DEF_DIAGNOSTIC(DUPLICATE_FUNC_ATTR, ERROR, SEMA, -1, "", "duplicate attribute '%s'"), \
  DEF_DIAGNOSTIC(INVALID_SLOT_INIT, ERROR, SEMA, -1, "", "Invalid slot initializer '%s' - no such variable/constant or incorrect expression"), \
  DEF_DIAGNOSTIC(CANNOT_DELETE, ERROR, SEMA, -1, "", "can't delete %s"), \
  DEF_DIAGNOSTIC(CONFLICTS_WITH, ERROR, SEMA, -1, "", "%s name '%s' conflicts with %s"), \
  DEF_DIAGNOSTIC(INC_DEC_NOT_ASSIGNABLE, ERROR, SEMA, -1, "", "argument of inc/dec operation is not assignable"), \
  DEF_DIAGNOSTIC(TYPE_DIFFERS, ERROR, SEMA, -1, "", "%s type differs from the declared type"), \
  DEF_DIAGNOSTIC(INFERRED_TYPE_MISMATCH, ERROR, SEMA, -1, "", "expression of type '%s' cannot be assigned to type '%s'"), \
  DEF_DIAGNOSTIC(SPACE_SEP_FIELD_NAME, ERROR, SEMA, -1, "", "Separate access operator '%s' with its following name is forbidden"), \
  DEF_DIAGNOSTIC(TOO_MANY_SYMBOLS, ERROR, SEMA, -1, "", "internal compiler error: too many %s"), \
  DEF_DIAGNOSTIC(NOT_ALLOWED_IN_CONST, ERROR, SEMA, -1, "", "%s is not allowed in constant"), \
  DEF_DIAGNOSTIC(ID_IS_NOT_CONST, ERROR, SEMA, -1, "", "Identifier '%s' is not a compile-time constant"), \
  DEF_DIAGNOSTIC(CONSTANT_SLOT_NOT_FOUND, ERROR, SEMA, -1, "", "%s slot %s not found in constant object"), \
  DEF_DIAGNOSTIC(CONSTANT_FIELD_NOT_FOUND, ERROR, SEMA, -1, "", "Field '%s' not found in constant object"), \
  DEF_DIAGNOSTIC(PAREN_IS_FUNC_CALL, WARNING, SYNTAX, 190, "paren-is-function-call", "'(' on a new line parsed as function call."), \
  DEF_DIAGNOSTIC(STMT_SAME_LINE, WARNING, SYNTAX, 192, "statement-on-same-line", "Next statement on the same line after '%s' statement."), \
  DEF_DIAGNOSTIC(NULLABLE_OPERANDS, WARNING, SEMA, 200, "potentially-nulled-ops", "%s with potentially nullable expression."), \
  DEF_DIAGNOSTIC(AND_OR_PAREN, WARNING, SEMA, 202, "and-or-paren", "Priority of the '&&' operator is higher than that of the '||' operator. Perhaps parentheses are missing?"), \
  DEF_DIAGNOSTIC(BITWISE_BOOL_PAREN, WARNING, SEMA, 203, "bitwise-bool-paren", "Result of bitwise operation used in boolean expression. Perhaps parentheses are missing?"), \
  DEF_DIAGNOSTIC(BITWISE_OP_TO_BOOL, WARNING, SEMA, 204, "bitwise-apply-to-bool", "The '&' or '|' operator is applied to boolean type. You've probably forgotten to include parentheses or intended to use the '&&' or '||' operator."), \
  DEF_DIAGNOSTIC(UNREACHABLE_CODE, WARNING, SEMA, 205, "unreachable-code", "Unreachable code after '%s'."), \
  DEF_DIAGNOSTIC(ASSIGNED_TWICE, WARNING, SEMA, 206, "assigned-twice", "Variable is assigned twice successively."), \
  DEF_DIAGNOSTIC(NULLABLE_ASSIGNMENT, WARNING, SEMA, 208, "potentially-nulled-assign", "Assignment to potentially nullable expression."), \
  DEF_DIAGNOSTIC(ASG_TO_ITSELF, WARNING, SEMA, 209, "assigned-to-itself", "The variable is assigned to itself."), \
  DEF_DIAGNOSTIC(POTENTIALLY_NULLABLE_INDEX, WARNING, SEMA, 210, "potentially-nulled-index", "Potentially nullable expression used as array index."), \
  DEF_DIAGNOSTIC(DUPLICATE_CASE, WARNING, SEMA, 211, "duplicate-case", "Duplicate case value."), \
  DEF_DIAGNOSTIC(DUPLICATE_IF_EXPR, WARNING, SEMA, 212, "duplicate-if-expression", "Detected pattern 'if (A) {...} else if (A) {...}'. Branch unreachable."), \
  DEF_DIAGNOSTIC(THEN_ELSE_EQUAL, WARNING, SEMA, 213, "then-and-else-equals", "'then' statement is equivalent to 'else' statement."), \
  DEF_DIAGNOSTIC(TERNARY_SAME_VALUES, WARNING, SEMA, 214, "operator-returns-same-val", "Both branches of operator '<> ? <> : <>' are equivalent."), \
  DEF_DIAGNOSTIC(TERNARY_PRIOR, WARNING, SEMA, 215, "ternary-priority", "The '?:' operator has lower priority than the '%s' operator. Perhaps the '?:' operator works in a different way than it was expected."), \
  DEF_DIAGNOSTIC(SAME_OPERANDS, WARNING, SEMA, 216, "same-operands", "Left and right operands of '%s' operator are the same."), \
  DEF_DIAGNOSTIC(UNCOND_TERMINATED_LOOP, WARNING, SEMA, 217, "unconditional-terminated-loop", "Unconditional '%s' inside a loop."), \
  DEF_DIAGNOSTIC(POTENTIALLY_NULLABLE_CONTAINER, WARNING, SEMA, 220, "potentially-nulled-container", "'foreach' on potentially nullable expression."), \
  DEF_DIAGNOSTIC(UNUTILIZED_EXPRESSION, WARNING, SEMA, 221, "result-not-utilized", "Result of operation is not used."), \
  DEF_DIAGNOSTIC(BOOL_AS_INDEX, WARNING, SEMA, 222, "bool-as-index", "Boolean used as array index."), \
  DEF_DIAGNOSTIC(COMPARE_WITH_BOOL, WARNING, SEMA, 223, "compared-with-bool", "Comparison with boolean."), \
  DEF_DIAGNOSTIC(EMPTY_BODY, WARNING, SEMA, 224, "empty-body", "%s has an empty body."), \
  DEF_DIAGNOSTIC(NOT_ALL_PATH_RETURN_VALUE, WARNING, SEMA, 225, "all-paths-return-value", "Not all control paths return a value."), \
  DEF_DIAGNOSTIC(RETURNS_DIFFERENT_TYPES, WARNING, SEMA, 226, "return-different-types", "Function can return different types."), \
  DEF_DIAGNOSTIC(ID_HIDES_ID, WARNING, SEMA, 227, "ident-hides-ident", "%s '%s' hides %s with the same name."), \
  DEF_DIAGNOSTIC(DECLARED_NEVER_USED, WARNING, SEMA, 228, "declared-never-used", "%s '%s' was declared but never used."), \
  DEF_DIAGNOSTIC(COPY_OF_EXPR, WARNING, SEMA, 229, "copy-of-expression", "Duplicate expression found inside the sequence of operations."), \
  DEF_DIAGNOSTIC(IMPORTED_NEVER_USED, WARNING, SEMA, 230, "imported-never-used", "Imported field '%s' was never used."), \
  DEF_DIAGNOSTIC(FORMAT_ARGUMENTS_COUNT, WARNING, SEMA, 231, "format-arguments-count", "Format string: arguments count mismatch."), \
  DEF_DIAGNOSTIC(ALWAYS_T_OR_F, WARNING, SEMA, 232, "always-true-or-false", "Expression is always '%s'."), \
  DEF_DIAGNOSTIC(CONST_IN_BOOL_EXPR, WARNING, SEMA, 233, "const-in-bool-expr", "Constant in a boolean expression."), \
  DEF_DIAGNOSTIC(DIV_BY_ZERO, WARNING, SEMA, 234, "div-by-zero", "Integer division by zero."), \
  DEF_DIAGNOSTIC(ROUND_TO_INT, WARNING, SEMA, 235, "round-to-int", "Result of division will be integer."), \
  DEF_DIAGNOSTIC(SHIFT_PRIORITY, WARNING, SEMA, 236, "shift-priority", "Shift operator has lower priority. Perhaps parentheses are missing?"), \
  DEF_DIAGNOSTIC(NAME_LIKE_SHOULD_RETURN, WARNING, SEMA, 238, "named-like-should-return", "Function name '%s' implies a return value, but its result is never used."), \
  DEF_DIAGNOSTIC(NAME_RET_BOOL, WARNING, SEMA, 239, "named-like-return-bool", "Function name '%s' implies a return boolean type but not all control paths return boolean."), \
  DEF_DIAGNOSTIC(NULL_COALESCING_PRIORITY, WARNING, SEMA, 240, "null-coalescing-priority", "The '??""' operator has a lower priority than the '%s' operator (a??b > c == a??""(b > c)). Perhaps the '??""' operator works in a different way than it was expected."), \
  DEF_DIAGNOSTIC(ALREADY_REQUIRED, WARNING, SEMA, 241, "already-required", "Module '%s' has been required already."), \
  DEF_DIAGNOSTIC(USED_FROM_STATIC, WARNING, SEMA, 244, "used-from-static", "Access 'this.%s' from %s function."), \
  DEF_DIAGNOSTIC(ACCESS_POT_NULLABLE, WARNING, SEMA, 248, "access-potentially-nulled", "'%s' can be null, but is used as a %s without checking."), \
  DEF_DIAGNOSTIC(CMP_WITH_CONTAINER, WARNING, SEMA, 250, "cmp-with-container", "Comparison with a %s."), \
  DEF_DIAGNOSTIC(BOOL_PASSED_TO_STRANGE, WARNING, SEMA, 254, "bool-passed-to-strange", "Boolean passed to '%s' operator."), \
  DEF_DIAGNOSTIC(DUPLICATE_FUNC, WARNING, SEMA, 255, "duplicate-function", "Duplicate function body. Consider functions '%s' and '%s'."), \
  DEF_DIAGNOSTIC(KEY_NAME_MISMATCH, WARNING, SEMA, 256, "key-and-function-name", "Key and function name are not the same ('%s' and '%s')."), \
  DEF_DIAGNOSTIC(DUPLICATE_ASSIGN_EXPR, WARNING, SEMA, 257, "duplicate-assigned-expr", "Duplicate of the assigned expression."), \
  DEF_DIAGNOSTIC(SIMILAR_FUNC, WARNING, SEMA, 258, "similar-function", "Function bodies are very similar. Consider functions '%s' and '%s'."), \
  DEF_DIAGNOSTIC(SIMILAR_ASSIGN_EXPR, WARNING, SEMA, 259, "similar-assigned-expr", "Assigned expression is very similar to one of the previous ones."), \
  DEF_DIAGNOSTIC(NAME_EXPECTS_RETURN, WARNING, SEMA, 260, "named-like-must-return-result", "Function '%s' has name like it should return a value, but not all control paths return a value."), \
  DEF_DIAGNOSTIC(SUSPICIOUS_FMT, WARNING, SYNTAX, 262, "suspicious-formatting", "Suspicious code formatting."), \
  DEF_DIAGNOSTIC(EGYPT_BRACES, WARNING, SYNTAX, 263, "egyptian-braces", "Indentation style: 'egyptian braces' required."), \
  DEF_DIAGNOSTIC(PLUS_STRING, WARNING, SEMA, 264, "plus-string", "Usage of '+' for string concatenation."), \
  DEF_DIAGNOSTIC(FORGOTTEN_DO, WARNING, SEMA, 266, "forgotten-do", "'while' after the statement list (forgot 'do' ?)"), \
  DEF_DIAGNOSTIC(SUSPICIOUS_BRACKET, WARNING, SYNTAX, 267, "suspicious-bracket", "'%s' will be parsed as '%s' (forgot ',' ?)"), \
  DEF_DIAGNOSTIC(MIXED_SEPARATORS, WARNING, SYNTAX, 269, "mixed-separators", "Mixed spaces and commas to separate %s."), \
  DEF_DIAGNOSTIC(EXTEND_TO_APPEND, HINT, SEMA, 270, "extent-to-append", "It is better to use 'append(A, B, ...)' instead of 'extend([A, B, ...])'."), \
  DEF_DIAGNOSTIC(FORGOT_SUBST, WARNING, SEMA, 271, "forgot-subst", "'{}' found inside string (forgot 'subst' or '$' ?)."), \
  DEF_DIAGNOSTIC(NOT_UNARY_OP, WARNING, SYNTAX, 272, "not-unary-op", "This '%s' is not a unary operator. Please use ' ' after it or ',' before it for better understandability."), \
  DEF_DIAGNOSTIC(MISSED_BREAK, WARNING, SEMA, 275, "missed-break", "A 'break' statement is probably missing in a 'switch' statement."), \
  DEF_DIAGNOSTIC(SPACE_AT_EOL, WARNING, LEX, 277, "space-at-eol", "Whitespace at the end of line."), \
  DEF_DIAGNOSTIC(FORBIDDEN_FUNC, WARNING, SEMA, 278, "forbidden-function", "It is forbidden to call '%s' function."), \
  DEF_DIAGNOSTIC(MISMATCH_LOOP_VAR, WARNING, SEMA, 279, "mismatch-loop-variable", "The variable used in for-loop does not match the initialized one."), \
  DEF_DIAGNOSTIC(FORBIDDEN_PARENT_DIR, WARNING, SEMA, 280, "forbidden-parent-dir", "Access to the parent directory is forbidden in this function."), \
  DEF_DIAGNOSTIC(UNWANTED_MODIFICATION, WARNING, SEMA, 281, "unwanted-modification", "Function '%s' modifies the object. You probably didn't want to modify the object here."), \
  DEF_DIAGNOSTIC(USELESS_NULLC, WARNING, SEMA, 283, "useless-null-coalescing", "The expression to the right of the '??""' is null."), \
  DEF_DIAGNOSTIC(CAN_BE_SIMPLIFIED, WARNING, SEMA, 284, "can-be-simplified", "Expression can be simplified."), \
  DEF_DIAGNOSTIC(EXPR_NOT_NULL, WARNING, SEMA, 285, "expr-cannot-be-null", "The expression to the left of the '%s' cannot be null."), \
  DEF_DIAGNOSTIC(DECL_IN_EXPR, WARNING, SEMA, 286, "decl-in-expression", "Declaration used in arith expression as operand."), \
  DEF_DIAGNOSTIC(RANGE_CHECK, WARNING, SEMA, 287, "range-check", "It looks like the range boundaries are not checked correctly. Pay attention to checking with minimum and maximum index."), \
  DEF_DIAGNOSTIC(PARAM_COUNT_MISMATCH, WARNING, SEMA, 288, "param-count", "Function '%s' (%d..%d parameters) is called with the wrong number of arguments (%d)."),\
  DEF_DIAGNOSTIC(PARAM_POSITION_MISMATCH, WARNING, SEMA, 289, "param-pos", "The function parameter '%s' seems to be in the wrong position."), \
  DEF_DIAGNOSTIC(INVALID_UNDERSCORE, WARNING, SEMA, 291, "invalid-underscore", "The name of %s '%s' is invalid. The identifier is marked as unused with a prefix underscore, but it is used."), \
  DEF_DIAGNOSTIC(MODIFIED_CONTAINER, WARNING, SEMA, 292, "modified-container", "The container was modified within the loop."), \
  DEF_DIAGNOSTIC(DUPLICATE_PERSIST_ID, WARNING, SEMA, 293, "duplicate-persist-id", "Duplicate id '%s' passed to 'persist'."), \
  DEF_DIAGNOSTIC(UNDEFINED_GLOBAL, WARNING, SEMA, 295, "undefined-global", "Undefined global identifier '%s'."), \
  DEF_DIAGNOSTIC(CALL_FROM_ROOT, WARNING, SEMA, 297, "call-from-root", "Function '%s' must be called from the root scope."), \
  DEF_DIAGNOSTIC(INTEGER_OVERFLOW, WARNING, SEMA, 304, "integer-overflow", "Integer Overflow."), \
  DEF_DIAGNOSTIC(RELATIVE_CMP_BOOL, WARNING, SEMA, 305, "relative-bool-cmp", "Relative comparison of non-boolean with boolean. It is a potential runtime error."), \
  DEF_DIAGNOSTIC(EQ_PAREN_MISSED, WARNING, SEMA, 306, "eq-paren-miss", "Suspicious expression, probably parens are missing."), \
  DEF_DIAGNOSTIC(GLOBAL_NAME_REDEF, WARNING, SEMA, 307, "global-id-redef", "Redefinition of existing global name '%s'."), \
  DEF_DIAGNOSTIC(BOOL_LAMBDA_REQUIRED, WARNING, SEMA, 308, "bool-lambda-required", "Function '%s' requires lambda which returns boolean."), \
  DEF_DIAGNOSTIC(MISSING_DESTRUCTURED_VALUE, WARNING, SEMA, 309, "missing-destructured-value", "No value for '%s' in destructured declaration."), \
  DEF_DIAGNOSTIC(DESTRUCTURED_TYPE_MISMATCH, WARNING, SEMA, 310, "destructured-type", "Destructured type mismatch, right side is %s."), \
  DEF_DIAGNOSTIC(WRONG_TYPE, WARNING, SEMA, 311, "wrong-type", "Wrong type: expected %s, got %s."), \
  DEF_DIAGNOSTIC(MISSING_FIELD, WARNING, SEMA, 312, "missing-field", "No field '%s' in %s."), \
  DEF_DIAGNOSTIC(MISSING_MODULE, HINT, SEMA, 313, "missing-module", "Module '%s' not found, or it returns null."), \
  DEF_DIAGNOSTIC(SEE_OTHER, HINT, SEMA, 314, "see-other", "You can find %s here."), \
  DEF_DIAGNOSTIC(INVALID_INDENTATION, WARNING, SYNTAX, 315, "invalid-indentation", "Invalid indentation. Pay attention to lines %s and %s."), \
  DEF_DIAGNOSTIC(NOT_A_CONST, WARNING, COMPILETIME, 316, "not-a-const", "Expression in 'static' context must be a constant expression."), \
  DEF_DIAGNOSTIC(STATIC_MEMO_TOO_SIMPLE, WARNING, COMPILETIME, 317, "static-too-simple", "'static' is too simple."), \
  DEF_DIAGNOSTIC(MERGE_EMPTY_TABLE, WARNING, SEMA, 318, "merge-empty-table", "'__merge({})' with an empty table is equivalent to 'clone'. Use 'clone' instead."), \
  DEF_DIAGNOSTIC(EMPTY_ARRAY_RESIZE, WARNING, SEMA, 319, "empty-array-resize", "'[].resize(...)' is slower than 'array(...)'. Use 'array(...)' instead."), \
  DEF_DIAGNOSTIC(CALLBACK_SHOULD_RETURN_VALUE, WARNING, SEMA, 320, "callback-should-return-value", "Callback passed to '%s' must return a value."), \
  DEF_DIAGNOSTIC(PARAM_ASSIGNMENT_IN_LAMBDA, WARNING, SEMA, 321, "param-assign-in-lambda", "Assignment to parameter '%s' in lambda has no effect. Return the expression instead."), \
  DEF_DIAGNOSTIC(CALLBACK_SHOULD_NOT_RETURN, WARNING, SEMA, 322, "callback-should-not-return", "Callback passed to '%s' should not return a value.") \


namespace SQCompilation {

struct CompilerError {}; // Exception to throw

enum DiagnosticsId {
#define DEF_DIAGNOSTIC(id, _, __, ___, _____, ______) DI_##id
  DIAGNOSTICS,
#undef DEF_DIAGNOSTIC
  DI_NUM_OF_DIAGNOSTICS
};

enum DiagnosticSeverity {
  DS_HINT,
  DS_WARNING,
  DS_ERROR
};

enum CommentKind {
  CK_NONE,
  CK_LINE,    // | // foo bar
  CK_BLOCK,   // | /* foo bar */
  CK_ML_BLOCK // | /* foo \n bar */
};

struct CommentData {

  CommentData() : kind(CK_NONE), size(0), text(nullptr), commentLine(-1), begin(-1), end(-1) {}
  CommentData(enum CommentKind k, size_t s, const char *t, SQInteger n, SQInteger b, SQInteger e) : kind(k), size(s), text(t), commentLine(n), begin(b), end(e) {}

  const enum CommentKind kind;
  const size_t size;
  const char *text;

  const SQInteger commentLine; // stand for line number in multiline block comment, or 0 otherwise
  const SQInteger begin, end;

  bool isLineComment() const { return kind == CK_LINE; }
  bool isBlockComment() const { return kind == CK_BLOCK; }
  bool isMultiLineComment() const { return kind == CK_ML_BLOCK; }
};

class Comments {
public:
  using LineCommentsList = sqvector<CommentData>;

  Comments(SQAllocContext ctx) : _commentsList(ctx), emptyComments(ctx) {}
  ~Comments();

  const LineCommentsList &lineComment(int line) const;

  sqvector<LineCommentsList> &commentsList() { return _commentsList; }
  const sqvector<LineCommentsList> &commentsList() const { return _commentsList; }

  void pushNewLine() { _commentsList.push_back(LineCommentsList(_commentsList._alloc_ctx)); }

private:
  sqvector<LineCommentsList> _commentsList;
  LineCommentsList emptyComments;
};

class SQCompilationContext
{
public:
  SQCompilationContext(SQVM *vm, Arena *a, const char *sn, const char *code, size_t csize, const Comments *comments, bool raiseError);
  ~SQCompilationContext();

  const char *sourceName() const { return _sourceName; }

  SQAllocContext allocContext() const { return _ss(_vm)->_alloc_ctx; }
  SQVM *getVm() const { return _vm; }

  static void vrenderDiagnosticHeader(enum DiagnosticsId diag, std::string &msg, va_list args);
  static void renderDiagnosticHeader(enum DiagnosticsId diag, std::string *msg, ...);
  void vreportDiagnostic(enum DiagnosticsId diag, int32_t line, int32_t pos, int32_t width, va_list args);
  void reportDiagnostic(enum DiagnosticsId diag, int32_t line, int32_t pos, int32_t width, ...);
  bool isDisabled(enum DiagnosticsId id, int line, int pos);
  bool isRequireDisabled(int line, int col);

  static void printAllWarnings(FILE *ostream);
  static bool enableWarning(const char *diagName, bool state);
  static bool enableWarning(int32_t id, bool state);
  static void switchSyntaxWarningsState(bool state);

  Arena *arena() const { return _arena; }

private:


  void buildLineMap();

  const char *findLine(int lineNo);
  bool isBlankLine(const char *l);

  const char *_sourceName;

  SQVM *_vm;

  const char *_code;
  size_t _codeSize;

  Arena *_arena;

  bool _raiseError;

  sqvector<int> _linemap;
  const Comments *_comments;
};

} // namespace SQCompilation
