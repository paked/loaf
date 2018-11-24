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

  ASTNode root;
};

void parser_init(Parser* p, array(Token) tokens) {
  p->tokens = tokens;

  p->head = p->tokens;
}

void parser_advance(Parser* p) {
  // TODO(harrison): skip comment tokens
  p->head += 1;
}

bool parser_allow(Parser* p, TokenType type, Token* tok = 0) {
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

bool parser_expect(Parser *p, TokenType type, Token* tok = 0) {
  if (!parser_allow(p, type, tok)) {
    return false;
  }

  parser_advance(p);

  return true;
}

// int x = 10 + y;
//              ^
// func();
//   ^
// int x = 10 + func(y);
//                ^  ^
bool parser_parseIdentifier(Parser* p, ASTNode* node, Token tIdent) {
  assert(tIdent.type == TOKEN_IDENTIFIER);

  if (parser_expect(p, TOKEN_BRACKET_OPEN)) {
    if (parser_expect(p, TOKEN_BRACKET_CLOSE)) {
      *node = ast_makeFunctionCall(tIdent);

      return true;
    }

    return false;
  }

  /*
  if (parser_expect(TOKEN_BRACKET_OPEN)) {
    *node = ast_makeFunctionCall(tIdent, parameters);
    return true;
  }*/

  *node = ast_makeIdentifier(tIdent);

  return true;
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
bool parser_parseNumericalExpression(Parser* p, ASTNode* node, TokenType endOn = TOKEN_SEMICOLON) {
  array(ASTNode) expression = array_ASTNode_init();

  while (true) {
    ASTNode n = {};
    Token t = {};

    if (parser_expect(p, TOKEN_NUMBER, &t)) {
      n = ast_makeNumber(us_parseInt(t.start, t.len), t);
    } else if (parser_expect(p, TOKEN_ADD, &t)) {
      n = ast_makeOperator(AST_NODE_ADD, t);
    } else if (parser_expect(p, TOKEN_SUBTRACT, &t)) {
      n = ast_makeOperator(AST_NODE_SUBTRACT, t);
    } else if (parser_expect(p, TOKEN_MULTIPLY, &t)) {
      n = ast_makeOperator(AST_NODE_MULTIPLY, t);
    } else if (parser_expect(p, TOKEN_DIVIDE, &t)) {
      n = ast_makeOperator(AST_NODE_DIVIDE, t);
    } else if (parser_expect(p, TOKEN_EQUALS, &t)) {
      n = ast_makeOperator(AST_NODE_TEST_EQUAL, t);
    } else if (parser_expect(p, TOKEN_GREATER, &t)) {
      n = ast_makeOperator(AST_NODE_TEST_GREATER, t);
    } else if (parser_expect(p, TOKEN_LESSER, &t)) {
      n = ast_makeOperator(AST_NODE_TEST_LESSER, t);
    } else if (parser_expect(p, TOKEN_IDENTIFIER, &t)) {
      if (!parser_parseIdentifier(p, &n, t)) {
        return false;
      }
    } else if (parser_allow(p, TOKEN_BRACKET_OPEN)) {
      if (!parser_parseBrackets(p, &n)) {
        return false;
      }
    } else if (parser_allow(p, endOn) || parser_allow(p, TOKEN_CURLY_OPEN)) {
      break;
    } else {
      printf("token: %.*s. %d\n", t.len, t.start, t.type);

      assert(!"Unknown token type");
    }

    array_ASTNode_add(&expression, n);
  }

#define LEFT() (expression[i])
#define OP() (expression[i + 1])
#define RIGHT() (expression[i + 2])

  array(ASTNode) working = array_ASTNode_init();

  ASTNodeType precedenceOrder[] = {
    // Numeric
    AST_NODE_MULTIPLY, AST_NODE_DIVIDE, AST_NODE_ADD, AST_NODE_SUBTRACT,

    // Comparative
    AST_NODE_TEST_EQUAL, AST_NODE_TEST_GREATER, AST_NODE_TEST_LESSER
  };

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

        continue;
      }

      if (!ast_nodeHasValue(right)) {
        assert(!"Right node doesn't have value");

        return false;
      }

      Token t = {};
      t.line = op.line;

      printf("We have a valid binary operator\n");
      ASTNode compressed = ast_makeOperatorWith(op.type, left, right, t);
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

bool parser_parseBooleanExpression(Parser* p, ASTNode* node) {
  return false;
}

bool parser_parseBrackets(Parser* p, ASTNode* node) {
  if (parser_expect(p, TOKEN_BRACKET_OPEN)) {
    if (parser_parseNumericalExpression(p, node, TOKEN_BRACKET_CLOSE)) {
      if (parser_expect(p, TOKEN_BRACKET_CLOSE)) {
        return true;
      }
    }
  }

  return false;
}

bool parser_parseExpression(Parser* p, ASTNode* node) {
  printf("parsing expression\n");

  Token t;

  if (parser_expect(p, TOKEN_TRUE, &t)) {
    *node = ast_makeValue(value_make(true), t);

    return true;
  } else if (parser_expect(p, TOKEN_FALSE, &t)) {
    *node = ast_makeValue(value_make(false), t);

    return true;
  } else if (parser_parseNumericalExpression(p, node)) {
    return true;
  } else if (parser_expect(p, TOKEN_IDENTIFIER, &t)) {
    if (parser_parseIdentifier(p, node, t)) {
      return true;
    }
  }


  return false;
}

bool parser_parseBlock(Parser* p, ASTNode* node);

bool parser_parseStatement(Parser* p, ASTNode* node) {
  printf("beginning parse statemetn\n");

  Token tIdent;
  if (parser_expect(p, TOKEN_IDENTIFIER, &tIdent)) {
    ASTNode ident = ast_makeIdentifier(tIdent);

    Token tAss;
    if (parser_expect(p, TOKEN_ASSIGNMENT_DECLARATION, &tAss)) {
      ASTNode val = {};

      printf("getting expression\n");
      if (parser_parseExpression(p, &val)) {
        printf("doing the thing: %d\n", val.type);
        printf("parsed assdecl\n");

        *node = ast_makeAssignmentDeclaration(ident, val, tAss);
        return true;
      }
    } else if (parser_expect(p, TOKEN_ASSIGNMENT, &tAss)) {
      ASTNode val = {};
      printf("getting expression\n");

      if (parser_parseExpression(p, &val)) {
        printf("doing the thing: %d\n", val.type);
        printf("parsed ass\n");

        *node = ast_makeAssignment(ident, val, tAss);
        return true;
      }
    } else if (parser_parseIdentifier(p, node, tIdent)) {
      return true;
    }

    Token t = *p->head;

    printf("%.*s %d\n", t.len, t.start, t.type);
  } else if (parser_expect(p, TOKEN_LOG)) {
    printf("HELL FUCKING LO????\n");
    *node = ast_makeLog();
    return true;
  }

  printf("failed parse statemetn\n");
  return false;
}

bool parser_parseIf(Parser* p, ASTNode* node) {
  if (parser_expect(p, TOKEN_IF)) {
      ASTNode condition = {};
      if (parser_parseExpression(p, &condition)) {
        ASTNode block = {};

        if (parser_parseBlock(p, &block)) {
          *node = ast_makeIf(condition, block);

          return true;
        }
      }
  }

  return false;
}

bool parser_parseBlock(Parser* p, ASTNode* node);

bool parser_parseFunctionDeclaration(Parser* p, ASTNode* node) {
  if (parser_expect(p, TOKEN_FUNC)) {
    Token ident;

    if (parser_expect(p, TOKEN_IDENTIFIER, &ident)) {
      ASTNode source;

      if (parser_parseBlock(p, &source)) {
        *node = ast_makeFunctionDeclaration(ident, source);

        return true;
      }
    }
  }

  return false;
}

bool parser_parseScope(Parser* p, ASTNode* node) {
  ASTNode block = ast_makeRoot();

  while (p->head->type != TOKEN_EOF && p->head->type != TOKEN_CURLY_CLOSE) {
    ASTNode next = {};

    if (parser_parseIf(p, &next)) {
      // do nothing
    } else if (parser_parseFunctionDeclaration(p, &next)) {
      // do nothing
    } else if (parser_parseBlock(p, &next)) {
      // do nothing
    } else if (parser_parseStatement(p, &next)) {
      // TODO(harrison): make this next check part of the parseStatement function
      if (!parser_expect(p, TOKEN_SEMICOLON)) {
        assert(!"Syntax error: Expected semicolon but did not find one\n");

        return false;
      }
    } else {
      assert(!"Don't know how to parse this");
    }

    ast_root_add(&block, next);
  }

  *node = block;

  return true;
}

bool parser_parseBlock(Parser* p, ASTNode* node) {
  printf("parsing block\n");
  if (parser_expect(p, TOKEN_CURLY_OPEN)) {
    printf("found curly open\n");

    ASTNode block = {};

    if (parser_parseScope(p, &block)) {
      if (parser_expect(p, TOKEN_CURLY_CLOSE)) {
        *node = block;

        return true;
      }
    }
  }

  printf("failed parsing block %d\n", p->head->type);

  return false;
}

typedef bool (*ParseFunction)(Parser* p, ASTNode* node);

ParseFunction parseFunctions[] = {parser_parseStatement};
psize parseFunctionsLen = sizeof(parseFunctions)/sizeof(ParseFunction);

bool parser_parse(Parser* p, Hunk* hunk) {
  parser_parseScope(p, &p->root);

  printf("finished building AST\n");

  Scope scope = {};
  scope_init(&scope);

  if (!ast_writeBytecode(&p->root, hunk, &scope)) {
    printf("Could not generate bytecode\n");

    return false;
  }

  // Need to show some output
  hunk_write(hunk, OP_RETURN, 0);

  return true;
}
