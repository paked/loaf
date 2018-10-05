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

// Perform algorithm:
// for: N * N + N * Bx + I
//  - group multiplication and division together until there are no operations of that sort in the top level
//    - becomes: B(N * N) + B(N * Bx) + I
//  - group addition and subtraction together
//    - becomes: B(B(B(N * N) + B(N * Bx)) + I)
// therefore everything has been reduced into a single binary expression
bool parser_parseExpression(Parser* p, ASTNode* node, TokenType endOn = TOKEN_SEMICOLON) {
  array(ASTNode) expression = array_ASTNode_init();

  while (true) {
    ASTNode n = {};
    Token t = {};

    if (parser_expect(p, TOKEN_NUMBER, &t)) {
      parser_advance(p);

      n = ast_makeNumber(us_parseInt(t.start, t.len));
    } else if (parser_expect(p, TOKEN_ADD, &t)) {
      parser_advance(p);

      n = ast_makeOperator(AST_NODE_ADD);
    } else if (parser_expect(p, TOKEN_SUBTRACT, &t)) {
      parser_advance(p);

      n = ast_makeOperator(AST_NODE_SUBTRACT);
    } else if (parser_expect(p, TOKEN_MULTIPLY, &t)) {
      parser_advance(p);

      n = ast_makeOperator(AST_NODE_MULTIPLY);
    } else if (parser_expect(p, TOKEN_DIVIDE, &t)) {
      parser_advance(p);

      n = ast_makeOperator(AST_NODE_DIVIDE);
    } else if (parser_expect(p, TOKEN_BRACKET_OPEN)) {
      if (!parser_parseBrackets(p, &n)) {
        return false;
      }
    } else if (parser_expect(p, endOn)) {
      break;
    } else {
      assert(!"Unknown token type");
    }

    array_ASTNode_add(&expression, n);
  }

#define LEFT() (expression[i])
#define OP() (expression[i + 1])
#define RIGHT() (expression[i + 2])

  array(ASTNode) working = array_ASTNode_init();

  ASTNodeType precedenceOrder[] = {AST_NODE_MULTIPLY, AST_NODE_DIVIDE, AST_NODE_ADD, AST_NODE_SUBTRACT };

  for (ASTNodeType currentOP : precedenceOrder) {
    psize i = 0;

    while (i < array_count(expression) && array_count(expression) != 1) {
      if (i + 2 >= array_count(expression)) {
        printf("can't find anymore of current operation type, searching next\n");
        break;
      }

      ASTNode left = LEFT();
      ASTNode op = OP();
      ASTNode right = RIGHT();

      if (!ast_nodeHasValue(left)) {
        assert(!"Left node doesn't have value");

        return false;
      }

      if (op.type != currentOP) {
        array_ASTNode_add(&working, left);
        array_ASTNode_add(&working, op);

        i += 2;

        printf("found wrong operation type, adding to working and moving on\n");

        // TODO(harrison): skip other types instead of failing

        continue;
      }

      if (!ast_nodeHasValue(right)) {
        assert(!"Right node doesn't have value");

        return false;
      }

      printf("We have a valid binary operator\n");
      ASTNode compressed = ast_makeOperatorWith(op.type, left, right);
      array_ASTNode_add(&working, compressed);

      // Copy nodes after the right node;
      psize j = i + 3;
      while (j < array_count(expression)) {
        ASTNode n = expression[j];
        array_ASTNode_add(&working, n);

        j += 1;

        printf("copying %zu\n", array_count(expression) - 3);
      }

      array_ASTNode_copy(&working, &expression);
      array_ASTNode_zero(&working);

      printf("done\n");
    }

    array_ASTNode_zero(&working);
  }

  assert(array_count(expression) == 1);

  *node = expression[0];

  return true;
}

bool parser_parseBrackets(Parser* p, ASTNode* node) {
  if (parser_expect(p, TOKEN_BRACKET_OPEN)) {
    parser_advance(p);
    if (parser_parseExpression(p, node, TOKEN_BRACKET_CLOSE)) {
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

      ASTNode val = {};

      printf("getting expression\n");
      if (parser_parseExpression(p, &val)) {
        printf("doing the thing: %d\n", val.type);
        printf("parsed assdecl\n");

        *node = ast_makeAssignmentDeclaration(ident, val);
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
        printf("Parsed a statement!\n");

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
