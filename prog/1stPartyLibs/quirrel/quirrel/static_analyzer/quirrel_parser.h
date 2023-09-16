#pragma once

#include "quirrel_lexer.h"
#include <new>

#define ALLOW_FUNCTION_PARAMS_WITHOUT_COMMA 1
#define ALLOW_ASSIGNMENT_IN_LAMBDA 1

extern FILE * out_stream;


#define NODE_TYPES \
  NODE_TYPE(PNT_UNKNOWN, "unknown-op") \
  NODE_TYPE(PNT_NULL, "null") \
  NODE_TYPE(PNT_INTEGER, "int-number") \
  NODE_TYPE(PNT_BOOL, "boolean") \
  NODE_TYPE(PNT_FLOAT, "float-number") \
  NODE_TYPE(PNT_STRING, "string-literal") \
  NODE_TYPE(PNT_IDENTIFIER, "identifier") \
  NODE_TYPE(PNT_VAR_PARAMS, "var-params") \
  NODE_TYPE(PNT_KEY_VALUE, "key-value") \
  NODE_TYPE(PNT_MAKE_KEY, "make-key") \
  NODE_TYPE(PNT_LIST_OF_KEYS_ARRAY, "list-of-keys-array") \
  NODE_TYPE(PNT_LIST_OF_KEYS_TABLE, "list-of-keys-table") \
  NODE_TYPE(PNT_READER_MACRO, "reader-macro") \
  NODE_TYPE(PNT_UNARY_PRE_OP, "unary-pre-op") \
  NODE_TYPE(PNT_UNARY_POST_OP, "unary-post-op") \
  NODE_TYPE(PNT_BINARY_OP, "binary-op") \
  NODE_TYPE(PNT_TERNARY_OP, "ternary-op") \
  NODE_TYPE(PNT_EXPRESSION_PAREN, "expression-paren") \
  NODE_TYPE(PNT_ROOT, "root") \
  NODE_TYPE(PNT_THIS, "this") \
  NODE_TYPE(PNT_BASE, "base") \
  NODE_TYPE(PNT_ACCESS_MEMBER, "access-member") \
  NODE_TYPE(PNT_ACCESS_MEMBER_IF_NOT_NULL, "access-member-if-not-null") \
  NODE_TYPE(PNT_FUNCTION_CALL, "function-call") \
  NODE_TYPE(PNT_FUNCTION_CALL_IF_NOT_NULL, "function-call-if-not-null") \
  NODE_TYPE(PNT_ARRAY_CREATION, "array-creation") \
  NODE_TYPE(PNT_TABLE_CREATION, "table-creation") \
  NODE_TYPE(PNT_CONST_DECLARATION, "const-declaration") \
  NODE_TYPE(PNT_GLOBAL_CONST_DECLARATION, "global-const-declaration") \
  NODE_TYPE(PNT_LOCAL_VAR_DECLARATION, "local-var-declaration") \
  NODE_TYPE(PNT_VAR_DECLARATOR, "var-declarator") \
  NODE_TYPE(PNT_INEXPR_VAR_DECLARATOR, "inexpr-var-declarator") \
  NODE_TYPE(PNT_EMPTY_STATEMENT, "empty-statement") \
  NODE_TYPE(PNT_STATEMENT_LIST, "statement-list") \
  NODE_TYPE(PNT_IF_ELSE, "if-else") \
  NODE_TYPE(PNT_WHILE_LOOP, "while-loop") \
  NODE_TYPE(PNT_DO_WHILE_LOOP, "do-while-loop") \
  NODE_TYPE(PNT_FOR_LOOP, "for-loop") \
  NODE_TYPE(PNT_FOR_EACH_LOOP, "for-each-loop") \
  NODE_TYPE(PNT_LOCAL_FUNCTION, "local-function") \
  NODE_TYPE(PNT_FUNCTION, "function") \
  NODE_TYPE(PNT_LAMBDA, "lambda") \
  NODE_TYPE(PNT_FUNCTION_PARAMETERS_LIST, "function-parameters-list") \
  NODE_TYPE(PNT_FUNCTION_PARAMETER, "function-parameter") \
  NODE_TYPE(PNT_RETURN, "return") \
  NODE_TYPE(PNT_YIELD, "yield") \
  NODE_TYPE(PNT_BREAK, "break") \
  NODE_TYPE(PNT_CONTINUE, "continue") \
  NODE_TYPE(PNT_THROW, "throw") \
  NODE_TYPE(PNT_TRY_CATCH, "try-catch") \
  NODE_TYPE(PNT_SWITCH_STATEMENT, "switch-statement") \
  NODE_TYPE(PNT_SWITCH_CASE, "switch-case") \
  NODE_TYPE(PNT_CLASS, "class") \
  NODE_TYPE(PNT_LOCAL_CLASS, "local-class") \
  NODE_TYPE(PNT_ATTRIBUTES_LIST, "attributes-list") \
  NODE_TYPE(PNT_ATTRIBUTE, "attribute") \
  NODE_TYPE(PNT_CLASS_METHOD, "class-method") \
  NODE_TYPE(PNT_CLASS_CONSTRUCTOR, "class-constructor") \
  NODE_TYPE(PNT_ACCESS_CONSTRUCTOR, "access-constructor") \
  NODE_TYPE(PNT_CLASS_MEMBER, "class-member") \
  NODE_TYPE(PNT_STATIC_CLASS_MEMBER, "static-class-member") \
  NODE_TYPE(PNT_ENUM, "enum") \
  NODE_TYPE(PNT_GLOBAL_ENUM, "global-enum") \
  NODE_TYPE(PNT_ENUM_MEMBER, "enum-member") \
  NODE_TYPE(PNT_IMPORT_VAR_DECLARATION, "import-var-declaration") \


#define NODE_TYPE(x, y) x,
enum NodeType
{
  NODE_TYPES
  PNT_COUNT
};
#undef NODE_TYPE

extern const char * node_type_names[];
extern Token emptyToken;

struct Node
{
  Token & tok;
  NodeType nodeType;
  std::vector<Node *> children;

  Node(Token & token) : tok(token), nodeType(PNT_UNKNOWN) {}
  Node() : tok(emptyToken), nodeType(PNT_UNKNOWN) {}

  void print(int indent = 0)
  {
    char buf[64];
    if (tok.type == TK_INTEGER)
      snprintf(buf, 64, "%llu", (long long unsigned int)tok.u.i);
    if (tok.type == TK_FLOAT)
      snprintf(buf, 64, "%g", tok.u.d);

    fprintf(out_stream, "%s%s '%s'\n", std::string(indent, ' ').c_str(),
      node_type_names[nodeType],
      tok.type == TK_INTEGER || tok.type == TK_FLOAT ? buf : tok.u.s);

    for (size_t i = 0; i < children.size(); i++)
      if (children[i])
        children[i]->print(indent + 2);
      else
        fprintf(out_stream, "%s*empty*\n", std::string(indent + 2, ' ').c_str());
  }
};


struct NodeList
{
  std::vector< std::vector<Node> > nodes;
  int pos;

  NodeList()
  {
    pos = 0;
    std::vector<Node> v;
    v.resize(16);
    nodes.emplace_back(v);
  }

  Node * newNode(Token & token)
  {
    Node * res = &(nodes.back()[pos]);
    pos++;
    if (pos >= nodes.back().size())
    {
      int newSize = int(nodes.back().size() * 2);
      if (newSize > 256)
        newSize = 256;

      std::vector<Node> v;
      v.resize(newSize);
      nodes.emplace_back(v);
      pos = 0;
    }

    res = new (res) Node(token);
    return res;
  }
};


Node * sq3_parse(Lexer & lex);


