#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

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

enum ASTNodeType : uint32 {
  AST_NODE_ROOT,
  AST_NODE_ASSIGNMENT_DECLARATION,

  AST_NODE_IDENTIFIER,
  AST_NODE_NUMBER
};

struct ASTNode;

array_for_name(ASTNode*, ASTNodep);
struct ASTNode_Root {
  array(ASTNode*) children;
};

struct ASTNode_AssignmentDeclaration {
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
    ASTNode_AssignmentDeclaration assignmentDeclaration;

    ASTNode_Identifier identifier;
    ASTNode_Number number;
  };
};

array_for(Token);

struct Parser {
  array(Token) tokens;
  Token* head;

  Hunk hunk;

  ASTNode root;
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

bool parser_parseStatement(Parser* p, ASTNode* parent) {
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

        ASTNode assignmentDeclaration = ast_makeAssignmentDeclaration(ident, val);

        ast_root_add(parent, assignmentDeclaration);

        printf("parsed assdecl\n");
        return true;
      }
    } else if (parser_expect(p, TOKEN_BRACKET_OPEN)) {
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
    }

    Token t = *p->head;

    printf("%.*s %d\n", t.len, t.start, t.type);
  }

  printf("failed parse statemetn\n");
  return false;
}

void parser_parse(Parser* p) {
  p->root = ast_makeRoot();

  while (p->head->type != TOKEN_EOF) {
    printf("iterating on %.*s %d\n", p->head->len, p->head->start, p->head->type);
    if (parser_expect(p, TOKEN_IDENTIFIER)) {
      if (!parser_parseStatement(p, &p->root)) {
        printf("unknown statement\n");
        break;
      }

      printf("parsed statement correctly\n");

      continue;
    } else {
      printf("ERROR: could not parse tokens\n");

      break;
    }

    p->head += 1;
  }
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

  parser_parse(&parser);

  return test_bytecode();
}

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
