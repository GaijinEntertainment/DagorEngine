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

#define DIAGNOSTICS \
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
  DEF_DIAGNOSTIC(CALLBACK_SHOULD_NOT_RETURN, WARNING, SEMA, 322, "callback-should-not-return", "Callback passed to '%s' should not return a value."), \
  DEF_DIAGNOSTIC(DUPLICATE_TERNARY_CONDITION, WARNING, SEMA, 323, "duplicate-ternary-condition", "Nested ternary condition duplicates the outer condition."), \
  DEF_DIAGNOSTIC(FOR_DIRECTION, WARNING, SEMA, 324, "for-direction", "The update expression moves the for-loop variable in the wrong direction."), \
  DEF_DIAGNOSTIC(INVALID_TYPE_STRING, WARNING, SEMA, 325, "invalid-type-string", "Result of type() can never be '%s'%s"), \
  DEF_DIAGNOSTIC(INVALID_TYPEOF_STRING, WARNING, SEMA, 326, "invalid-typeof-string", "Result of typeof can never be '%s'%s"), \
  DEF_DIAGNOSTIC(SUBSUMED_IF_EXPR, WARNING, SEMA, 327, "subsumed-if-expression", "Detected pattern 'if (A) {...} else if (B) {...}' where condition B is already covered by an earlier condition. Branch unreachable."), \
  DEF_DIAGNOSTIC(REDUNDANT_AWAIT, WARNING, SEMA, 328, "redundant-await", "'await' on non-async expression has no effect."), \
  DEF_DIAGNOSTIC(REPEATED_CONDITION, WARNING, SEMA, 329, "repeated-condition", "Condition repeats a condition from an outer 'if'."), \
  DEF_DIAGNOSTIC(ASSIGNED_BACK, WARNING, SEMA, 330, "assigned-back", "Assignment writes back the value that was just copied from this expression."), \
  DEF_DIAGNOSTIC(FUNCTION_RETURNS_SAME_VALUE, WARNING, SEMA, 331, "function-returns-same-value", "Function returns the same value from all value-returning paths."), \
  DEF_DIAGNOSTIC(DUPLICATE_IMPORT, WARNING, SEMA, 332, "duplicate-import", "Imported field '%s' is listed more than once."), \
  DEF_DIAGNOSTIC(WILDCARD_AND_NAMED_IMPORT, WARNING, SEMA, 333, "wildcard-and-named-import", "Module '%s' is imported both with '*' and by explicit names.") \


namespace SQCompilation {

class Node;

struct CompilerError {}; // Exception to throw

enum DiagnosticsId {
#define DEF_DIAGNOSTIC(id, _, __, ___, _____, ______) DI_##id
  DIAGNOSTICS,
#undef DEF_DIAGNOSTIC
  DI_NUM_OF_DIAGNOSTICS
};

enum DiagnosticSeverity {
  DS_HINT,
  DS_WARNING
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
  void reportDiagnostic(enum DiagnosticsId diag, const Node *n, ...);

  void vthrowError(int32_t line, int32_t pos, int32_t width, const char *fmt, va_list args);
  void throwError(int32_t line, int32_t pos, int32_t width, const char *fmt, ...);
  void throwError(const Node *n, const char *fmt, ...);
  void throwError(const char *fmt, ...);

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
  void buildSourceWindow(int32_t line, int32_t pos, int32_t width, std::string &extraInfo);
  // Common emit path shared by warnings/hints (vreportDiagnostic) and errors (vthrowError):
  // builds source window and invokes the diag/error handlers. Caller has already formatted msg.
  void emitDiagnostic(const char *msg, int32_t line, int32_t pos, int32_t width,
                      int32_t intId, const char *textId, SQMessageSeverity severity);

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
