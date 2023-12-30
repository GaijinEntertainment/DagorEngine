#ifndef  _SQCOMPILATIONCONTEXT_H_
#define _SQCOMPILATIONCONTEXT_H_ 1

#include <string>
#include <stdint.h>
#include <stdarg.h>
#include <setjmp.h>
#include <vector>
#include "squirrel.h"
#include "sqvm.h"
#include "sqstate.h"
#include "squtils.h"
#include "arena.h"

class KeyValueFile;

#define DIAGNOSTICS \
  DEF_DIAGNOSTIC(LITERAL_OVERFLOW, ERROR, LEX, -1, "", "%s constant overflow"), \
  DEF_DIAGNOSTIC(LITERAL_UNDERFLOW, ERROR, LEX, -1, "", "%s constant underflow"), \
  DEF_DIAGNOSTIC(FP_EXP_EXPECTED, ERROR, LEX, -1, "", "exponent expected"), \
  DEF_DIAGNOSTIC(MALFORMED_NUMBER, ERROR, LEX, -1, "", "malformed number"), \
  DEF_DIAGNOSTIC(HEX_DIGITS_EXPECTED, ERROR, LEX, -1, "", "expected hex digits after '0x'"), \
  DEF_DIAGNOSTIC(HEX_TOO_MANY_DIGITS, ERROR, LEX, -1, "", "too many digits for an Hex number"), \
  DEF_DIAGNOSTIC(OCTAL_NOT_SUPPORTED, ERROR, LEX, -1, "", "leading 0 is not allowed, octal numbers are not supported"), \
  DEF_DIAGNOSTIC(TOO_LONG_LITERAL, ERROR, LEX, -1, "", "constant too long"), \
  DEF_DIAGNOSTIC(EMPTY_LITERAL, ERROR, LEX, -1, "", "empty constant"), \
  DEF_DIAGNOSTIC(UNRECOGNISED_ESCAPER, ERROR, LEX, -1, "", "unrecognised escaper char"), \
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
  DEF_DIAGNOSTIC(UNSUPPORTED_DIRECTIVE, ERROR, SYNTAX, -1, "", "unsupported directive '%s'"), \
  DEF_DIAGNOSTIC(EXPECTED_LINENUM, ERROR, SYNTAX, -1, "", "expected line number after #pos:"), \
  DEF_DIAGNOSTIC(EXPECTED_COLNUM, ERROR, SYNTAX, -1, "", "expected column number after #pos:<line>:"), \
  DEF_DIAGNOSTIC(TOO_BIG_AST, ERROR, SYNTAX, -1, "", "AST too big. Consider simplifying it"), \
  DEF_DIAGNOSTIC(INCORRECT_INTRA_ASSIGN, ERROR, SYNTAX, -1, "", ": intra-expression assignment can be used only in 'if', 'for', 'while' or 'switch'"), \
  DEF_DIAGNOSTIC(ASSIGN_INSIDE_FORBIDDEN, ERROR, SYNTAX, -1, "", "'=' inside '%s' is forbidden"), \
  DEF_DIAGNOSTIC(BROKEN_SLOT_DECLARATION, ERROR, SYNTAX, -1, "", "cannot break deref/or comma needed after [exp]=exp slot declaration"), \
  DEF_DIAGNOSTIC(ROOT_TABLE_FORBIDDEN, ERROR, SYNTAX, -1, "", "Access to root table is forbidden"), \
  DEF_DIAGNOSTIC(DELETE_OP_FORBIDDEN, ERROR, SYNTAX, -1, "", "Usage of 'delete' operator if forbidden. Use 'o.rawdelete(\"key\")' instead"), \
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
  DEF_DIAGNOSTIC(INVALID_ENUM, ERROR, SEMA, -1, "", "invalid enum [no '%s' field in '%s']"), \
  DEF_DIAGNOSTIC(UNKNOWN_SYMBOL, ERROR, SEMA, -1, "", "Unknown variable [%s]"), \
  DEF_DIAGNOSTIC(CANNOT_EVAL_UNARY, ERROR, SEMA, -1, "", "cannot evaluate unary-op"), \
  DEF_DIAGNOSTIC(DUPLICATE_KEY, ERROR, SEMA, -1, "", "duplicate key '%s'"), \
  DEF_DIAGNOSTIC(INVALID_SLOT_INIT, ERROR, SEMA, -1, "", "Invalid slot initializer '%s' - no such variable/constant or incorrect expression"), \
  DEF_DIAGNOSTIC(CANNOT_DELETE, ERROR, SEMA, -1, "", "can't delete %s"), \
  DEF_DIAGNOSTIC(CONFLICTS_WITH, ERROR, SEMA, -1, "", "%s name '%s' conflicts with %s"), \
  DEF_DIAGNOSTIC(LOCAL_CLASS_SYNTAX, ERROR, SEMA, -1, "", "cannot create a local class with the syntax (class <local>)"), \
  DEF_DIAGNOSTIC(INVALID_CLASS_NAME, ERROR, SEMA, -1, "", "invalid class name or context"), \
  DEF_DIAGNOSTIC(INC_DEC_NOT_ASSIGNABLE, ERROR, SEMA, -1, "", "argument of inc/dec operation is not assignable"), \
  DEF_DIAGNOSTIC(SUSPICIOUS_SYNTAX_RANGE_LOOP, ERROR, SEMA, -1, "", "Very suspicious range-loop syntax construction"), \
  DEF_DIAGNOSTIC(ID_RANGE_LOOP, ERROR, SEMA, -1, "", "iterator name (identifier) is expected in 'for ([variable:] [from,] to[, step])', got 'expression'"), \
  DEF_DIAGNOSTIC(COMMA_RANGE_LOOP, ERROR, SEMA, -1, "", "too many expressions separated by comma in '%s' - loop"), \
  DEF_DIAGNOSTIC(SPACE_SEP_FIELD_NAME, ERROR, SEMA, -1, "", "Separate access operator '%s' with it's following name is forbidden"), \
  DEF_DIAGNOSTIC(TOO_MANY_SYMBOLS, ERROR, SEMA, -1, "", "internal compiler error: too many %s"), \
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
  DEF_DIAGNOSTIC(THEN_ELSE_EQUAL, WARNING, SEMA, 213, "then-and-else-equals", "then' statement is equivalent to 'else' statement."), \
  DEF_DIAGNOSTIC(TERNARY_SAME_VALUES, WARNING, SEMA, 214, "operator-returns-same-val", "Both branches of operator '<> ? <> : <>' are equivalent."), \
  DEF_DIAGNOSTIC(TERNARY_PRIOR, WARNING, SEMA, 215, "ternary-priority", "The '?:' operator has lower priority than the '%s' operator. Perhaps the '?:' operator works in a different way than it was expected."), \
  DEF_DIAGNOSTIC(SAME_OPERANDS, WARNING, SEMA, 216, "same-operands", "Left and right operands of '%s' operator are the same."), \
  DEF_DIAGNOSTIC(UNCOND_TERMINATED_LOOP, WARNING, SEMA, 217, "unconditional-terminated-loop", "Unconditional '%s' inside a loop."), \
  DEF_DIAGNOSTIC(POTENTIALLY_NULLABLE_CONTAINER, WARNING, SEMA, 220, "potentially-nulled-container", "'foreach' on potentially nullable expression."), \
  DEF_DIAGNOSTIC(UNUTILIZED_EXPRESSION, WARNING, SEMA, 221, "result-not-utilized", "Result of operation is not used."), \
  DEF_DIAGNOSTIC(BOOL_AS_INDEX, WARNING, SEMA, 222, "bool-as-index", "Boolean used as array index."), \
  DEF_DIAGNOSTIC(COMPARE_WITH_BOOL, WARNING, SEMA, 223, "compared-with-bool", "Comparison with boolean."), \
  DEF_DIAGNOSTIC(EMPTY_BODY, WARNING, SEMA, 224, "empty-body", "'%s' operator has an empty body."), \
  DEF_DIAGNOSTIC(NOT_ALL_PATH_RETURN_VALUE, WARNING, SEMA, 225, "all-paths-return-value", "Not all control paths return a value."), \
  DEF_DIAGNOSTIC(RETURNS_DIFFERENT_TYPES, WARNING, SEMA, 226, "return-different-types", "Function can return different types."), \
  DEF_DIAGNOSTIC(ID_HIDES_ID, WARNING, SEMA, 227, "ident-hides-ident", "%s '%s' hides %s with the same name."), \
  DEF_DIAGNOSTIC(DECLARED_NEVER_USED, WARNING, SEMA, 228, "declared-never-used", "%s '%s' was declared but never used."), \
  DEF_DIAGNOSTIC(COPY_OF_EXPR, WARNING, SEMA, 229, "copy-of-expression", "Duplicate expression found inside the sequence of operations."), \
  DEF_DIAGNOSTIC(FORMAT_ARGUMENTS_COUNT, WARNING, SEMA, 231, "format-arguments-count", "Format string: arguments count mismatch."), \
  DEF_DIAGNOSTIC(ALWAYS_T_OR_F, WARNING, SEMA, 232, "always-true-or-false", "Expression is always '%s'."), \
  DEF_DIAGNOSTIC(CONST_IN_BOOL_EXPR, WARNING, SEMA, 233, "const-in-bool-expr", "Constant in a boolean expression."), \
  DEF_DIAGNOSTIC(DIV_BY_ZERO, WARNING, SEMA, 234, "div-by-zero", "Integer division by zero."), \
  DEF_DIAGNOSTIC(ROUND_TO_INT, WARNING, SEMA, 235, "round-to-int", "Result of division will be integer."), \
  DEF_DIAGNOSTIC(SHIFT_PRIORITY, WARNING, SEMA, 236, "shift-priority", "Shift operator has lower priority. Perhaps parentheses are missing?"), \
  DEF_DIAGNOSTIC(NAME_LIKE_SHOULD_RETURN, WARNING, SEMA, 238, "named-like-should-return", "Function name '%s' implies a return value, but its result is never used."), \
  DEF_DIAGNOSTIC(NAME_RET_BOOL, WARNING, SEMA, 239, "named-like-return-bool", "Function name '%s' implies a return boolean type but not all control paths returns boolean."), \
  DEF_DIAGNOSTIC(NULL_COALESCING_PRIORITY, WARNING, SEMA, 240, "null-coalescing-priority", "The '??""' operator has a lower priority than the '%s' operator (a??b > c == a??""(b > c)). Perhaps the '??""' operator works in a different way than it was expected."), \
  DEF_DIAGNOSTIC(ALREADY_REQUIRED, WARNING, SEMA, 241, "already-required", "Module '%s' has been required already."), \
  DEF_DIAGNOSTIC(USED_FROM_STATIC, WARNING, SEMA, 244, "used-from-static", "Access 'this.%s' from %s function."), \
  DEF_DIAGNOSTIC(FUNC_CAN_RET_NULL, WARNING, SEMA, 247, "func-can-return-null", "Function '%s' can return null, but its result is used here."), \
  DEF_DIAGNOSTIC(ACCESS_POT_NULLABLE, WARNING, SEMA, 248, "access-potentially-nulled", "'%s' can be null, but is used as a %s without checking."), \
  DEF_DIAGNOSTIC(CMP_WITH_CONTAINER, WARNING, SEMA, 250, "cmp-with-container", "Comparison with a %s."), \
  DEF_DIAGNOSTIC(BOOL_PASSED_TO_STRANGE, WARNING, SEMA, 254, "bool-passed-to-strange", "Boolean passed to '%s' operator."), \
  DEF_DIAGNOSTIC(DUPLICATE_FUNC, WARNING, SEMA, 255, "duplicate-function", "Duplicate function body. Consider functions '%s' and '%s'."), \
  DEF_DIAGNOSTIC(KEY_NAME_MISMATCH, WARNING, SEMA, 256, "key-and-function-name", "Key and function name are not the same ('%s' and '%s')."), \
  DEF_DIAGNOSTIC(DUPLICATE_ASSIGN_EXPR, WARNING, SEMA, 257, "duplicate-assigned-expr", "Duplicate of the assigned expression."), \
  DEF_DIAGNOSTIC(SIMILAR_FUNC, WARNING, SEMA, 258, "similar-function", "Function bodies are very similar. Consider functions '%s' and '%s'."), \
  DEF_DIAGNOSTIC(SIMILAR_ASSIGN_EXPR, WARNING, SEMA, 259, "similar-assigned-expr", "Assigned expression is very similar to one of the previous ones."), \
  DEF_DIAGNOSTIC(NAME_EXPECTS_RETURN, WARNING, SEMA, 260, "named-like-must-return-result", "Function '%s' has name like it should return a value, but not all control paths returns a value."), \
  DEF_DIAGNOSTIC(SUSPICIOUS_FMT, WARNING, SYNTAX, 262, "suspicious-formatting", "Suspicious code formatting."), \
  DEF_DIAGNOSTIC(EGYPT_BRACES, WARNING, SYNTAX, 263, "egyptian-braces", "Indentation style: 'egyptian braces' required."), \
  DEF_DIAGNOSTIC(PLUS_STRING, WARNING, SEMA, 264, "plus-string", "Usage of '+' for string concatenation."), \
  DEF_DIAGNOSTIC(FORGOTTEN_DO, WARNING, SEMA, 266, "forgotten-do", "'while' after the statement list (forgot 'do' ?)"), \
  DEF_DIAGNOSTIC(SUSPICIOUS_BRACKET, WARNING, SYNTAX, 267, "suspicious-bracket", "'%s' will be parsed as '%s' (forgot ',' ?)"), \
  DEF_DIAGNOSTIC(MIXED_SEPARATORS, WARNING, SYNTAX, 269, "mixed-separators", "Mixed spaces and commas to separate %s."), \
  DEF_DIAGNOSTIC(EXTEND_TO_APPEND, HINT, SEMA, 270, "extent-to-append", "It is better to use 'append(A, B, ...)' instead of 'extend([A, B, ...])'."), \
  DEF_DIAGNOSTIC(FORGOT_SUBST, WARNING, SEMA, 271, "forgot-subst", "'{}' found inside string (forgot 'subst' or '$' ?)."), \
  DEF_DIAGNOSTIC(NOT_UNARY_OP, WARNING, SYNTAX, 272, "not-unary-op", "This '%s' is not unary operator. Please use ' ' after it or ',' before it for better understandability."), \
  DEF_DIAGNOSTIC(ITER_IN_CLOSURE, WARNING, SEMA, 274, "iterator-in-lambda", "Iterator '%s' is trying to be captured in closure."), \
  DEF_DIAGNOSTIC(MISSED_BREAK, WARNING, SEMA, 275, "missed-break", "A 'break' statement is probably missing in a 'switch' statement."), \
  DEF_DIAGNOSTIC(SPACE_AT_EOL, WARNING, LEX, 277, "space-at-eol", "Whitespace at the end of line."), \
  DEF_DIAGNOSTIC(FORBIDDEN_FUNC, WARNING, SEMA, 278, "forbidden-function", "It is forbidden to call '%s' function."), \
  DEF_DIAGNOSTIC(MISMATCH_LOOP_VAR, WARNING, SEMA, 279, "mismatch-loop-variable", "The variable used in for-loop does not match the initialized one."), \
  DEF_DIAGNOSTIC(FORBIDDEN_PARENT_DIR, WARNING, SEMA, 280, "forbidden-parent-dir", "Access to the parent directory is forbidden in this function."), \
  DEF_DIAGNOSTIC(UNWANTED_MODIFICATION, WARNING, SEMA, 281, "unwanted-modification", "Function '%s' modifies object. You probably didn't want to modify the object here."), \
  DEF_DIAGNOSTIC(INEXPR_PRIORITY, WARNING, SEMA, 282, "inexpr-assign-priority", "Operator ':=' has lower priority. Perhaps parentheses are missing?."), \
  DEF_DIAGNOSTIC(USELESS_NULLC, WARNING, SEMA, 283, "useless-null-coalescing", "The expression to the right of the '??""' is null."), \
  DEF_DIAGNOSTIC(CAN_BE_SIMPLIFIED, WARNING, SEMA, 284, "can-be-simplified", "Expression can be simplified."), \
  DEF_DIAGNOSTIC(EXPR_NOT_NULL, WARNING, SEMA, 285, "expr-cannot-be-null", "The expression to the left of the '%s' cannot be null."), \
  DEF_DIAGNOSTIC(DECL_IN_EXPR, WARNING, SEMA, 286, "decl-in-expression", "Declaration used in arith expression as operand."), \
  DEF_DIAGNOSTIC(RANGE_CHECK, WARNING, SEMA, 287, "range-check", "It looks like the range boundaries are not checked correctly. Pay attention to checking with minimum and maximum index."), \
  DEF_DIAGNOSTIC(PARAM_COUNT_MISMATCH, WARNING, SEMA, 288, "param-count", "Function '%s' is called with the wrong number of parameters."),\
  DEF_DIAGNOSTIC(PARAM_POSITION_MISMATCH, WARNING, SEMA, 289, "param-pos", "The function parameter '%s' seems to be in the wrong position."), \
  DEF_DIAGNOSTIC(INVALID_UNDERSCORE, WARNING, SEMA, 291, "invalid-underscore", "The name of %s '%s' is invalid. The identifier is marked as an unused with a prefix underscore, but it is used."), \
  DEF_DIAGNOSTIC(MODIFIED_CONTAINER, WARNING, SEMA, 292, "modified-container", "The container was modified within the loop."), \
  DEF_DIAGNOSTIC(DUPLICATE_PERSIST_ID, WARNING, SEMA, 293, "duplicate-persist-id", "Duplicate id '%s' passed to 'persist'."), \
  DEF_DIAGNOSTIC(UNDEFINED_GLOBAL, WARNING, SEMA, 295, "undefined-global", "Undefined global identifier '%s'."), \
  DEF_DIAGNOSTIC(CALL_FROM_ROOT, WARNING, SEMA, 297, "call-from-root", "Function '%s' must be called from the root scope."), \
  DEF_DIAGNOSTIC(REASSIGN_WITH_NO_USAGE, WARNING, SEMA, 301, "re-assign-no-use", "Re-assign variable without usage of previous assign."), \
  DEF_DIAGNOSTIC(POSSIBLE_GARBAGE, WARNING, SEMA, 302, "possible-garbage", "Not all paths initialize variable %s, it could potentially contain garbage."), \
  DEF_DIAGNOSTIC(UNINITIALIZED_VAR, WARNING, SEMA, 303, "uninitialized-variable", "Usage of uninitialized variable."), \
  DEF_DIAGNOSTIC(INTEGER_OVERFLOW, WARNING, SEMA, 304, "integer-overflow", "Integer Overflow."), \
  DEF_DIAGNOSTIC(RELATIVE_CMP_BOOL, WARNING, SEMA, 305, "relative-bool-cmp", "Relative comparison non-boolean with boolean. It is potential runtime error"), \
  DEF_DIAGNOSTIC(EQ_PAREN_MISSED, WARNING, SEMA, 306, "eq-paren-miss", "Suspicious expression, probably parens are missed."), \
  DEF_DIAGNOSTIC(GLOBAL_NAME_REDEF, WARNING, SEMA, 307, "global-id-redef", "Redefinition of existed global name '%s'."), \
  DEF_DIAGNOSTIC(BOOL_LAMBDA_REQUIRED, WARNING, SEMA, 308, "bool-lambda-required", "Function '%s' reuires lambda which returns boolean.")

namespace SQCompilation {

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
  CommentData(enum CommentKind k, size_t s, const SQChar *t, SQInteger n, SQInteger b, SQInteger e) : kind(k), size(s), text(t), commentLine(n), begin(b), end(e) {}

  const enum CommentKind kind;
  const size_t size;
  const SQChar *text;

  const SQInteger commentLine; // stand for line number in multiline block comment, or 0 otherwise
  const SQInteger begin, end;

  bool isLineComment() const { return kind == CK_LINE; }
  bool isBlockComment() const { return kind == CK_BLOCK; }
  bool isMultiLineComment() const { return kind == CK_ML_BLOCK; }
};

class Comments {
public:
  using LineCommentsList = sqvector<CommentData>;

  Comments(SQAllocContext ctx) : _commentsList(ctx) {}
  ~Comments();

  const LineCommentsList &lineComment(int line) const;

  sqvector<LineCommentsList> &commentsList() { return _commentsList; }
  const sqvector<LineCommentsList> &commentsList() const { return _commentsList; }

  void pushNewLine() { _commentsList.push_back(LineCommentsList(_commentsList._alloc_ctx)); }

private:
  sqvector<LineCommentsList> _commentsList;
};

class SQCompilationContext
{
public:
  SQCompilationContext(SQVM *vm, Arena *a, const SQChar *sn, const char *code, size_t csize, const Comments *comments, bool raiseError);
  ~SQCompilationContext();

  jmp_buf &errorJump() { return _errorjmp; }
  const SQChar *sourceName() const { return _sourceName; }

  SQAllocContext allocContext() const { return _ss(_vm)->_alloc_ctx; }

  static void vrenderDiagnosticHeader(enum DiagnosticsId diag, std::string &msg, va_list args);
  static void renderDiagnosticHeader(enum DiagnosticsId diag, std::string *msg, ...);
  void vreportDiagnostic(enum DiagnosticsId diag, int32_t line, int32_t pos, int32_t width, va_list args);
  void reportDiagnostic(enum DiagnosticsId diag, int32_t line, int32_t pos, int32_t width, ...);
  bool isDisabled(enum DiagnosticsId id, int line, int pos);

  static void printAllWarnings(FILE *ostream);
  static void flipWarningsState();
  static bool enableWarning(const char *diagName, bool state);
  static bool enableWarning(int32_t id, bool state);
  static void switchSyntaxWarningsState(bool state);

  Arena *arena() const { return _arena; }

  static void resetConfig();
  static bool loadConfigFile(const KeyValueFile &configBlk);
  static bool loadConfigFile(const char *configFile);

  static std::vector<std::string> function_can_return_string;
  static std::vector<std::string> function_should_return_bool_prefix;
  static std::vector<std::string> function_should_return_something_prefix;
  static std::vector<std::string> function_result_must_be_utilized;
  static std::vector<std::string> function_can_return_null;
  static std::vector<std::string> function_calls_lambda_inplace;
  static std::vector<std::string> function_takes_boolean_lambda;
  static std::vector<std::string> format_function_name;
  static std::vector<std::string> function_forbidden;
  static std::vector<std::string> function_forbidden_parent_dir;
  static std::vector<std::string> function_modifies_object;
  static std::vector<std::string> function_must_be_called_from_root;

  static std::vector<std::string> std_identifier;
  static std::vector<std::string> std_function;

private:


  void buildLineMap();

  const char *findLine(int lineNo);

  const SQChar *_sourceName;

  SQVM *_vm;

  const SQChar *_code;
  size_t _codeSize;

  jmp_buf _errorjmp;
  Arena *_arena;

  bool _raiseError;

  sqvector<const char *> _linemap;
  const Comments *_comments;
};

} // namespace SQCompilation

#endif // ! _SQCOMPILATIONCONTEXT_H_
