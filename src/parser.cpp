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

array_for(ASTNode);

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
    return false;
  }

  if (tok != 0) {
    *tok = *p->head;
  }

  return true;
}

bool parser_expect(Parser *p, TokenType type, Token* tok = 0) {
  if (!parser_allow(p, type, tok)) {
    return false;
  }

  parser_advance(p);

  return true;
}

bool parser_parseComplexExpression(Parser* p, ASTNode* node, TokenType endOn = TOKEN_SEMICOLON);

// int x = 10 + y;
//              ^
// func();
//   ^
// int x = 10 + func(y);
//                ^  ^
bool parser_parseIdentifier(Parser* p, ASTNode* node, Token tIdent) {
  assert(tIdent.type == TOKEN_IDENTIFIER);

  if (parser_expect(p, TOKEN_BRACKET_OPEN)) {
    array(ASTNode) args = array_ASTNode_init();

    if (!parser_allow(p, TOKEN_BRACKET_CLOSE)) {
      while (true) {
        ASTNode n = {};

        if (!parser_parseComplexExpression(p, &n, TOKEN_COMMA)) {
          return false;
        }

        array_ASTNode_add(&args, n);

        if (!parser_expect(p, TOKEN_COMMA)) {
          break;
        }
      }
    }

    if (parser_expect(p, TOKEN_BRACKET_CLOSE)) {
      *node = ast_makeFunctionCall(tIdent, args);

      return true;
    }

    return false;
  }

  *node = ast_makeIdentifier(tIdent);

  return true;
}

bool parser_parseBrackets(Parser* p, ASTNode* node);

// Perform algorithm:
// for: N * N + N * Bx + I
//  - group multiplication and division together until there are no operations of that sort in the top level
//    - becomes: B(N * N) + B(N * Bx) + I
//  - group addition and subtraction together
//    - becomes: B(B(B(N * N) + B(N * Bx)) + I)
// therefore everything has been reduced into a single binary expression
bool parser_parseComplexExpression(Parser* p, ASTNode* node, TokenType endOn) {
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
      ASTNodeType type = AST_NODE_TEST_GREATER;
      if (parser_expect(p, TOKEN_ASSIGNMENT)) {
        type = AST_NODE_TEST_GREATER_EQUAL;
      }

      n = ast_makeOperator(type, t);
    } else if (parser_expect(p, TOKEN_LESSER, &t)) {
      ASTNodeType type = AST_NODE_TEST_LESSER;
      if (parser_expect(p, TOKEN_ASSIGNMENT)) {
        type = AST_NODE_TEST_LESSER_EQUAL;
      }

      n = ast_makeOperator(type, t);
    } else if (parser_expect(p, TOKEN_AND, &t)) {
      n = ast_makeOperator(AST_NODE_TEST_AND, t);
    } else if (parser_expect(p, TOKEN_OR, &t)) {
      n = ast_makeOperator(AST_NODE_TEST_OR, t);
    } else if (parser_expect(p, TOKEN_IDENTIFIER, &t)) {
      if (!parser_parseIdentifier(p, &n, t)) {
        return false;
      }
    } else if (parser_allow(p, TOKEN_BRACKET_OPEN)) {
      if (!parser_parseBrackets(p, &n)) {
        return false;
      }
    } else if (parser_allow(p, endOn) || parser_allow(p, TOKEN_CURLY_OPEN) || parser_allow(p, TOKEN_BRACKET_CLOSE)) {
      break;
    } else {
      logf("token: %.*s. %d\n", t.len, t.start, t.type);

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
    AST_NODE_TEST_EQUAL, AST_NODE_TEST_GREATER, AST_NODE_TEST_LESSER,
    AST_NODE_TEST_GREATER_EQUAL, AST_NODE_TEST_LESSER_EQUAL,

    // Logical
    AST_NODE_TEST_AND, AST_NODE_TEST_OR,
  };

  for (ASTNodeType currentOP : precedenceOrder) {
    psize i = 0;

    while (i < array_count(expression) && array_count(expression) != 1) {
      if (i + 2 >= array_count(expression)) {
        break;
      }

      ASTNode left = LEFT();
      ASTNode op = OP();
      ASTNode right = RIGHT();

      if (op.type != currentOP) {
        array_ASTNode_add(&working, left);
        array_ASTNode_add(&working, op);

        i += 2;

        continue;
      }

      Token t = {};
      t.line = op.line;

      ASTNode compressed = ast_makeOperatorWith(op.type, left, right, t);
      array_ASTNode_add(&working, compressed);

      // Copy nodes after the right node;
      psize j = i + 3;
      while (j < array_count(expression)) {
        ASTNode n = expression[j];
        array_ASTNode_add(&working, n);

        j += 1;
      }

      array_ASTNode_copy(&working, &expression);
      array_ASTNode_zero(&working);
    }

    array_ASTNode_zero(&working);
  }

  assert(array_count(expression) == 1);

  *node = expression[0];

  return true;
}

bool parser_parseBrackets(Parser* p, ASTNode* node) {
  if (parser_expect(p, TOKEN_BRACKET_OPEN)) {
    if (parser_parseComplexExpression(p, node, TOKEN_BRACKET_CLOSE)) {
      if (parser_expect(p, TOKEN_BRACKET_CLOSE)) {
        return true;
      }
    }
  }

  return false;
}

bool parser_parseExpression(Parser* p, ASTNode* node) {
  Token t;

  if (parser_expect(p, TOKEN_TRUE, &t)) {
    *node = ast_makeValue(value_make(true), t);

    return true;
  } else if (parser_expect(p, TOKEN_FALSE, &t)) {
    *node = ast_makeValue(value_make(false), t);

    return true;
  } else if (parser_parseComplexExpression(p, node)) {
    return true;
  } else if (parser_expect(p, TOKEN_IDENTIFIER, &t)) {
    if (parser_parseIdentifier(p, node, t)) {
      return true;
    }
  }

  return false;
}

bool parser_parseBlock(Parser* p, ASTNode* node);

bool parser_parseReturn(Parser* p, ASTNode* node) {
  Token tok;
  if (parser_expect(p, TOKEN_RETURN, &tok)) {
    ASTNode expr = {};

    if (parser_parseExpression(p, &expr)) {
      *node = ast_makeReturn(expr, tok);

      return true;
    }
  }

  return false;
}

bool parser_parseStatement(Parser* p, ASTNode* node) {
  Token tIdent;
  if (parser_expect(p, TOKEN_IDENTIFIER, &tIdent)) {
    ASTNode ident = ast_makeIdentifier(tIdent);

    Token tAss;
    if (parser_expect(p, TOKEN_ASSIGNMENT_DECLARATION, &tAss)) {
      ASTNode val = {};

      if (parser_parseExpression(p, &val)) {
        *node = ast_makeAssignmentDeclaration(ident, val, tAss);

        return true;
      }
    } else if (parser_expect(p, TOKEN_ASSIGNMENT, &tAss)) {
      ASTNode val = {};

      if (parser_parseExpression(p, &val)) {
        *node = ast_makeAssignment(ident, val, tAss);

        return true;
      }
    } else if (parser_parseIdentifier(p, node, tIdent)) {
      return true;
    }
  } else if (parser_expect(p, TOKEN_LOG)) {
    *node = ast_makeLog();

    return true;
  } else if (parser_expect(p, TOKEN_VAR)) {
    if (parser_expect(p, TOKEN_IDENTIFIER, &tIdent)) {
      Token type;

      if (parser_expect(p, TOKEN_IDENTIFIER, &type)) {
        *node = ast_makeDeclaration(tIdent, type);

        return true;
      }
    }
    // TODO(harrison): typecheck!
  } else if (parser_parseReturn(p, node)) {
    return true;
  }

  return false;
}

bool parser_parseIf(Parser* p, ASTNode* node) {
  if (parser_expect(p, TOKEN_IF)) {
      ASTNode condition = {};
      if (parser_parseExpression(p, &condition)) {
        ASTNode block = {};

        if (parser_parseBlock(p, &block)) {
          if (parser_expect(p, TOKEN_ELSE)) {
            ASTNode elseBlock = {};

            if (parser_parseBlock(p, &elseBlock)) {
              *node = ast_makeIf(condition, block, elseBlock);

              return true;
            }
          } else {
            *node = ast_makeIf(condition, block);

            return true;
          }
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
      if (parser_expect(p, TOKEN_BRACKET_OPEN)) {
        array(Parameter) parameters = array_Parameter_init();

        if (!parser_allow(p, TOKEN_BRACKET_CLOSE)) {
          while (true) {
            Parameter param = {};

            if (!parser_expect(p, TOKEN_IDENTIFIER, &param.identifier)) {
              return false;
            }

            if (!parser_expect(p, TOKEN_IDENTIFIER, &param.type)) {
              return false;
            }

            array_Parameter_add(&parameters, param);

            if (!parser_expect(p, TOKEN_COMMA)) {
              break;
            }
          }
        }

        if (parser_expect(p, TOKEN_BRACKET_CLOSE)) {
          Token ret = {};

          // optional return type
          parser_expect(p, TOKEN_IDENTIFIER, &ret);

          ASTNode source;

          if (parser_parseBlock(p, &source)) {
            *node = ast_makeFunctionDeclaration(ident, source, parameters, ret);

            return true;
          }
        }
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
  if (parser_expect(p, TOKEN_CURLY_OPEN)) {
    ASTNode block = {};

    if (parser_parseScope(p, &block)) {
      if (parser_expect(p, TOKEN_CURLY_CLOSE)) {
        *node = block;

        return true;
      }
    }
  }

  return false;
}

bool parser_parse(Parser* p) {
  return parser_parseScope(p, &p->root);
}
