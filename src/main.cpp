#include <stdlib.h> // malloc, realloc
#include <stdio.h> // printf
#include <assert.h> // assert
#include <string.h> // memcmp

// TODO(harrison): rewrite some functions

#include <us.hpp>

#define REALLOC(Type, ptr, count) ((Type*) realloc(ptr, count * sizeof(Type)))

#include <array.cpp>
#include <lexer.cpp>
#include <bytecode.cpp>

// letter = ('a'...'z' | 'A'...'Z' | '_')
// number = {'0'...'9'}
// identifier = letter { letter | number }
//
// operator = "+"
//
// term = identifier | number | "(" numerical_expression ")"
// expression = term [ operator term ]
//
// function_call = identifier "(" expression ")"
//
// assignment = identifier ":=" expression
// statement = assignment | function_call

//  eg. `exampleThing`, `exampleThing2`, `_thingThing`

struct Variable {
  // Info for name string
  char* start;
  int len;

  int index;
};

array_for(Variable);

// TODO(harrison): remove some of the duplicate from scope_{exists,get,set} functions
struct Scope {
  array(Variable) variables;
};

void scope_init(Scope* s) {
  s->variables = array_Variable_init();
}

bool scope_exists(Scope* s, char* name, int len) {
  for (psize i = 0; i < array_count(s->variables); i++) {
    Variable v = s->variables[i];

    if (v.len != len) {
      continue;
    }

    if (memcmp(name, v.start, len) == 0) {
      return true;
    }
  }

  return false;
}

bool scope_get(Scope* s, char* name, int len, Variable* var) {
  for (psize i = 0; i < array_count(s->variables); i++) {
    Variable v = s->variables[i];

    if (v.len != len) {
      continue;
    }

    if (memcmp(name, v.start, len) == 0) {
      *var = v;

      return true;
    }
  }

  return false;
}

void scope_set(Scope* s, Variable var) {
  // If the variable exists, update it
  for (psize i = 0; i < array_count(s->variables); i++) {
    Variable v = s->variables[i];

    if (v.len != var.len) {
      continue;
    }

    if (memcmp(var.start, v.start, var.len) == 0) {
      s->variables[i] = var;

      return;
    }
  }

  // If it doesn't, add a new one
  array_Variable_add(&s->variables, var);
}


enum ASTNodeType : uint32 {
  AST_NODE_INVALID,
  AST_NODE_ROOT,

  AST_NODE_ASSIGNMENT,
  AST_NODE_ASSIGNMENT_DECLARATION,

  AST_NODE_ADD,
  AST_NODE_SUBTRACT,
  AST_NODE_MULTIPLY,
  AST_NODE_DIVIDE,

  AST_NODE_IDENTIFIER,
  AST_NODE_NUMBER
};

struct ASTNode;

array_for_name(ASTNode*, ASTNodep);
struct ASTNode_Root {
  array(ASTNode*) children;
};

struct ASTNode_Binary {
  ASTNode *left; // Must be an identifier
  ASTNode *right; // Must be an expression (which returns a number)
};

struct ASTNode_Identifier {
  Token token;
};

struct ASTNode_Number {
  int number;
};

struct ASTNode {
  ASTNodeType type;

  union {
    ASTNode_Root root;

    ASTNode_Binary assignmentDeclaration;
    ASTNode_Binary assignment;

    ASTNode_Binary add;
    ASTNode_Binary subtract;
    ASTNode_Binary multiply;
    ASTNode_Binary divide;

    ASTNode_Identifier identifier;
    ASTNode_Number number;
  };
};

ASTNode ast_makeRoot() {
  ASTNode node = {};
  node.type = AST_NODE_ROOT;

  node.root.children = array_ASTNodep_init();

  return node;
}

ASTNode ast_makeIdentifier(Token t) {
  ASTNode node = {};
  node.type = AST_NODE_IDENTIFIER;
  node.identifier.token = t;

  return node;
};

void ast_root_add(ASTNode* parent, ASTNode child) {
  assert(parent->type == AST_NODE_ROOT);

  ASTNode* c = (ASTNode*) malloc(sizeof(child));
  *c = child;

  array_ASTNodep_add(&parent->root.children, c);
}

ASTNode ast_makeNumber(int n) {
  ASTNode node = {};
  node.type = AST_NODE_NUMBER;
  node.number.number = n;

  return node;
}

ASTNode ast_makeAssignmentDeclaration(ASTNode left, ASTNode right) {
  ASTNode node = {};
  node.type = AST_NODE_ASSIGNMENT_DECLARATION;

  // TODO(harrison): free!
  ASTNode* l = (ASTNode*) malloc(sizeof(left));
  ASTNode* r = (ASTNode*) malloc(sizeof(right));

  *l = left;
  *r = right;

  node.assignmentDeclaration.left = l;
  node.assignmentDeclaration.right = r;

  return node;
};

ASTNode ast_makeAssignment(ASTNode left, ASTNode right) {
  ASTNode node = {};
  node.type = AST_NODE_ASSIGNMENT;

  // TODO(harrison): free!
  ASTNode* l = (ASTNode*) malloc(sizeof(left));
  ASTNode* r = (ASTNode*) malloc(sizeof(right));

  *l = left;
  *r = right;

  node.assignment.left = l;
  node.assignment.right = r;

  return node;
}

// TODO(harrison): properly propogate errors
bool ast_writeBytecode(ASTNode* node, Hunk* hunk, Scope* scope) {
  switch (node->type) {
    case AST_NODE_INVALID:
      {
        printf("reached an invalid source path\n");
        return false;
      } break;
    case AST_NODE_ROOT:
      {
        for (psize i = 0; i < array_count(node->root.children); i++) {
          ASTNode* child = node->root.children[i];

          if (!ast_writeBytecode(child, hunk, scope)) {
            return false;
          }
        }
      } break;
    case AST_NODE_ASSIGNMENT:
      {
        ASTNode* left = node->assignmentDeclaration.left;
        assert(left->type == AST_NODE_IDENTIFIER);

        ASTNode* right = node->assignmentDeclaration.right;
        assert(right->type == AST_NODE_NUMBER);

        // write instructions to push right side onto stack
        if (!ast_writeBytecode(right, hunk, scope)) {
          return false;
        }

        if (!scope_exists(scope, left->identifier.token.start, left->identifier.token.len)) {
          printf("ERROR: variable doesn't exist!\n");

          return false;
        }

        Variable var = {};
        var.start = left->identifier.token.start;
        var.len = left->identifier.token.len;

        var.index = hunk_addLocal(hunk, 0);

        scope_set(scope, var);

        hunk_write(hunk, OP_SET_LOCAL, 0);
        hunk_write(hunk, var.index, 0);

        hunk_write(hunk, OP_GET_LOCAL, 0);
        hunk_write(hunk, var.index, 0);     
      } break;
    case AST_NODE_ASSIGNMENT_DECLARATION:
      {
        ASTNode* left = node->assignmentDeclaration.left;
        assert(left->type == AST_NODE_IDENTIFIER);

        ASTNode* right = node->assignmentDeclaration.right;
        assert(right->type == AST_NODE_NUMBER);

        // write instructions to push right side onto stack
        if (!ast_writeBytecode(right, hunk, scope)) {
          return false;
        }

        if (scope_exists(scope, left->identifier.token.start, left->identifier.token.len)) {
          printf("ERROR: variable already exists!\n");

          return false;
        }

        Variable var = {};
        var.start = left->identifier.token.start;
        var.len = left->identifier.token.len;

        var.index = hunk_addLocal(hunk, 0);

        scope_set(scope, var);

        hunk_write(hunk, OP_SET_LOCAL, 0);
        hunk_write(hunk, var.index, 0);

        hunk_write(hunk, OP_GET_LOCAL, 0);
        hunk_write(hunk, var.index, 0);
      } break;
    case AST_NODE_NUMBER:
      {
        int constant = hunk_addConstant(hunk, node->number.number);
        hunk_write(hunk, OP_CONSTANT, 0);
        hunk_write(hunk, constant, 0);     
      } break;
    default:
      {
        printf("ERROR: Don't know how to get bytecode from node type %d\n", node->type);

        return false;
      }
  }

  return true;
}

/*
       ROOT
        |
    ASSIGNMENT
    |        |
   left     right
    |        |
identifier constant
    |        |
   "x"       10

assignment->right->push (instructions to push right side onto stack)
assignment->left->retrieve (retrieve variable type (local vs global) and relevant get/set op codes, get variable identifier from opcodes and write those instructions to set a variable to the top value of the stack)
*/

array_for(Token);

struct Parser {
  array(Token) tokens;
  Token* head;

  Hunk hunk;

  ASTNode root;
};

void parser_init(Parser* p, array(Token) tokens) {
  p->tokens = tokens;

  p->head = p->tokens;
  p->hunk = {};
}

bool parser_expect(Parser* p, TokenType type, Token* tok = 0) {
  // TODO(harrison): skip comment tokens

  if (p->head->type != type) {
    printf("did not match %.*s (head:%d vs want:%d)\n", p->head->len, p->head->start, p->head->type, type);

    return false;
  }

  if (tok != 0) {
    *tok = *p->head;
  }

  printf("matched %.*s %d\n", p->head->len, p->head->start, p->head->type);

  return true;
}

void parser_advance(Parser* p) {
  p->head += 1;
}


bool parser_parseBrackets(Parser* p, ASTNode* node);

array_for(ASTNode);

bool parser_parseExpression(Parser* p, ASTNode* node) {
  int i = 0;
  array(ASTNode) nodes = array_ASTNode_init();

  while (true) {
    ASTNode* n = &nodes[i];
    Token t;

    if (parser_expect(p, TOKEN_NUMBER, &t)) {
      parser_advance(p);

      // add numbers
    } else if (parser_expect(p, TOKEN_IDENTIFIER, &t)) {
      parser_advance(p);

      // add identifier
    } else if (parser_expect(p, TOKEN_ADD, &t)) {
      parser_advance(p);
    } else if (parser_expect(p, TOKEN_SUBTRACT, &t)) {
      parser_advance(p);
    } else if (parser_expect(p, TOKEN_MULTIPLY, &t)) {
      parser_advance(p);
    } else if (parser_expect(p, TOKEN_DIVIDE, &t)) {
      parser_advance(p);
    } else if (parser_expect(p, TOKEN_BRACKET_OPEN)) {
      parser_parseBrackets(p, n);
    } else {
      printf("Failed to parse expression: unknown token\n");

      return false;
    }

    i += 1;
  }

  // Perform algorithm:
  // for: N * N + N * Bx + I
  //  - group multiplication and division together until there are no operations of that sort in the top level
  //    - becomes: B(N * N) + B(N * Bx) + I
  //  - group addition and subtraction together
  //    - becomes: B(B(B(N * N) + B(N * Bx)) + I)
  // therefore everything has been reduced into a single binary expression

  return true;
}

bool parser_parseBrackets(Parser* p, ASTNode* node) {
  if (parser_expect(p, TOKEN_BRACKET_OPEN)) {
    parser_advance(p);
    if (parser_parseExpression(p, node)) {
      if (parser_expect(p, TOKEN_BRACKET_CLOSE)) {
        parser_advance(p);

        return true;
      }
    }
  }

  return false;
}

bool parser_parseStatement(Parser* p, ASTNode* node) {
  printf("beginning parse statemetn\n");

  Token tIdent;
  if (parser_expect(p, TOKEN_IDENTIFIER, &tIdent)) {
    parser_advance(p);

    ASTNode ident = ast_makeIdentifier(tIdent);

    if (parser_expect(p, TOKEN_ASSIGNMENT_DECLARATION)) {
      parser_advance(p);

      // TODO(harrison): parse expression
      Token tVal;
      if (parser_expect(p, TOKEN_NUMBER, &tVal)) {
        parser_advance(p);

        ASTNode val = ast_makeNumber(us_parseInt(tVal.start, tVal.len));

        *node = ast_makeAssignmentDeclaration(ident, val);

        printf("parsed assdecl\n");
        return true;
      }
    } else if (parser_expect(p, TOKEN_ASSIGNMENT)) {
      parser_advance(p);

      Token tVal;
      if (parser_expect(p, TOKEN_NUMBER, &tVal)) {
        parser_advance(p);

        ASTNode val = ast_makeNumber(us_parseInt(tVal.start, tVal.len));

        *node = ast_makeAssignment(ident, val);
      }

      printf("parsed ass\n");

      return true;
    }
    /* else if (parser_expect(p, TOKEN_BRACKET_OPEN)) {
      parser_advance(p);

      // TODO(harrison): parse expression
      Token tIdentVal;

      if (parser_expect(p, TOKEN_IDENTIFIER, &tIdentVal)) {
        parser_advance(p);
        if (parser_expect(p, TOKEN_BRACKET_CLOSE)) {
          parser_advance(p);

          printf("parsed function call\n");
          // TODO(harrison): add function call node to tree

          return true;
        }
      }
    }*/

    Token t = *p->head;

    printf("%.*s %d\n", t.len, t.start, t.type);
  }

  printf("failed parse statemetn\n");
  return false;
}

typedef bool (*ParseFunction)(Parser* p, ASTNode* node);

ParseFunction parseFunctions[] = {parser_parseStatement};
psize parseFunctionsLen = sizeof(parseFunctions)/sizeof(ParseFunction);

bool parser_parse(Parser* p) {
  p->root = ast_makeRoot();

  while (p->head->type != TOKEN_EOF) {
    printf("iterating on %.*s %d\n", p->head->len, p->head->start, p->head->type);

    ASTNode node = {};

    for (psize i = 0; i < parseFunctionsLen; i++) {
      if (parseFunctions[i](p, &node)) {
        printf("Parsed a statement\n!");

        break;
      }
    }

    if (!parser_expect(p, TOKEN_SEMICOLON)) {
      printf("Syntax error: Expected semicolon but did not find one\n");

      return false;
    }

    parser_advance(p);

    ast_root_add(&p->root, node);
  }

  printf("finished building AST\n");

  Scope scope = {};
  scope_init(&scope);

  if (!ast_writeBytecode(&p->root, &p->hunk, &scope)) {
    printf("Could not generate bytecode\n");

    return false;
  }

  // Need to show some output
  hunk_write(&p->hunk, OP_RETURN, 0);

  return true;
}

int test_bytecode();

int main(int argc, char** argv) {
  FILE* f = fopen("example.ls", "rb");
  if (f == 0) {
    printf("ERROR: can't open file\n");

    return -1;
  }

  fseek(f, 0L, SEEK_END);
  psize fSize = ftell(f);
  rewind(f);

  char* buffer = (char*) malloc(fSize + 1);
  if (buffer == 0) {
    printf("ERROR: not enough memory to read file\n");

    return -1;
  }

  psize bytesRead = fread(buffer, sizeof(char), fSize, f);

  if (bytesRead != fSize) {
    printf("ERROR: could not read file into memory\n");

    return -1;
  }

  buffer[bytesRead] = '\0';

  fclose(f);

  Scanner scanner = {0};

  scanner_load(&scanner, buffer);

  array(Token) tokens = array_Token_init();

  Token t;
  while (true) {
    t = scanner_getToken(&scanner);

    if (t.type == TOKEN_ILLEGAL) {
      printf("ERROR lexing code\n");

      break;
    } else if (t.type == TOKEN_COMMENT) {
      continue;
    }

    array_Token_add(&tokens, t);

    printf("val: %.*s\n", t.len, t.start);

    if (t.type == TOKEN_EOF) {
      printf("Got through all tokens\n");

      break;
    }
  }

  Parser parser = {};
  parser_init(&parser, tokens);

  if (!parser_parse(&parser)) {
    return -1;
  }

  VM vm = {0};

  vm_load(&vm, &parser.hunk);

  ProgramResult res = vm_run(&vm);

  if (res != PROGRAM_RESULT_OK) {
    printf("Program failed executing...\n");
    return 1;
  }

  printf("Program finished executing...\n");

  return 0;
}

#if 0
int test_bytecode() {
  Hunk hunk = {0};

  int line = 0;

  int local1 = hunk_addLocal(&hunk, 10);
  int local2 = hunk_addLocal(&hunk, 5);

  int constant = hunk_addConstant(&hunk, 25);
  hunk_write(&hunk, OP_CONSTANT, line);
  hunk_write(&hunk, constant, line++);

  hunk_write(&hunk, OP_SET_LOCAL, line);
  hunk_write(&hunk, local1, line++);

  constant = hunk_addConstant(&hunk, 10);
  hunk_write(&hunk, OP_CONSTANT, line);
  hunk_write(&hunk, constant, line++);

  hunk_write(&hunk, OP_SET_LOCAL, line);
  hunk_write(&hunk, local2, line++);

  hunk_write(&hunk, OP_GET_LOCAL, line);
  hunk_write(&hunk, local1, line++);

  hunk_write(&hunk, OP_GET_LOCAL, line);
  hunk_write(&hunk, local2, line++);

  hunk_write(&hunk, OP_MULTIPLY, line++);

  hunk_write(&hunk, OP_RETURN, line++);

  /*
  int constant = hunk_addConstant(&hunk, 22);
  if (constant == -1) {
    printf("ERROR: could not add constant\n");

    return -1;
  }

  if (!hunk_write(&hunk, OP_CONSTANT, line++)) {
    printf("ERROR: could not write chunk data\n");
    return -1;
  }

  if (!hunk_write(&hunk, constant, line)) {
    printf("ERROR: could not write chunk data\n");

    return -1;
  }

  if (!hunk_write(&hunk, OP_NEGATE, line++)) {
    printf("ERROR: could not write chunk data\n");
    return -1;
  }

  constant = hunk_addConstant(&hunk, 3);
  if (constant == -1) {
    printf("ERROR: could not add constant\n");

    return -1;
  }

  if (!hunk_write(&hunk, OP_CONSTANT, line++)) {
    printf("ERROR: could not write chunk data\n");
    return -1;
  }

  if (!hunk_write(&hunk, constant, line)) {
    printf("ERROR: could not write chunk data\n");

    return -1;
  }

  if (!hunk_write(&hunk, OP_MULTIPLY, line++)) {
    printf("ERROR: could not write chunk data\n");
    return -1;
  }

  if (!hunk_write(&hunk, OP_RETURN, line++)) {
    printf("ERROR: could not write chunk data\n");
    return -1;
  }*/

  VM vm = {0};

  vm_load(&vm, &hunk);

  ProgramResult res = vm_run(&vm);

  if (res != PROGRAM_RESULT_OK) {
    printf("Program failed executing...\n");
    return 1;
  }

  printf("Program finished executing...\n");

  return 0;
}
#endif
