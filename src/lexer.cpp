enum TokenType : uint32 {
  TOKEN_ILLEGAL,
  TOKEN_EOF,

  TOKEN_COMMENT,

  TOKEN_IDENTIFIER,

  TOKEN_NUMBER,

  TOKEN_ASSIGNMENT_DECLARATION,

  TOKEN_ADD,
  TOKEN_SUBTRACT,
  TOKEN_MULTIPLY,
  TOKEN_DIVIDE,

  TOKEN_BRACKET_OPEN,
  TOKEN_BRACKET_CLOSE,
};

struct Token {
  TokenType type;

  char* start;
  int len;
  int line;
};

struct Scanner {
  char* source;
  char* head;

  int line;
};

void scanner_load(Scanner* scn, char* buf) {
  scn->source = buf;
  scn->head = buf;
}

void scanner_skipWhitespace(Scanner* scn) {
  while (*scn->head != '\0') {
    char c = *scn->head;

    if (us_isNewline(c)) {
      scn->head += 1;
      
      scn->line += 1;
      
      continue;
    }

    if (us_isSpace(c)) {
      scn->head += 1;

      continue;
    }

    return;
  }
}

char scanner_peek(Scanner* scn, int amount = 1) {
  return *(scn->head + amount);
}

// NOTE(harrison): see EBNF definition of letter above
bool scanner_isLetter(Scanner* scn) {
  return us_isLetter(*scn->head) || *scn->head == '_';
}

Token scanner_readNumber(Scanner* scn) {
  Token t = {};
  t.type = TOKEN_NUMBER;
  t.line = scn->line;
  t.start = scn->head;

  while (*scn->head != '\0') {
    if (!us_isDigit(*scn->head)) {
      break;
    }

    t.len += 1;
    scn->head += 1;
  }

  return t;
}

Token scanner_readIdentifier(Scanner* scn) {
  Token t = {};
  t.type = TOKEN_IDENTIFIER;
  t.line = scn->line;
  t.start = scn->head;

  while (*scn->head != '\0') {
    if (!(scanner_isLetter(scn) || us_isDigit(*scn->head))) {
      break;
    }

    if (t.len == 0 && us_isDigit(*scn->head)) {
        break;
    }

    t.len += 1;
    scn->head += 1;
  }

  return t;
}

Token scanner_readComment(Scanner* scn) {
  Token t = {};
  t.type = TOKEN_COMMENT;
  t.line = scn->line;
  t.start = scn->head;

  scn->head += 2;
  t.len += 2;

  int depth = 1;

  while (*scn->head != '\0') {
    if (*scn->head == '/' && scanner_peek(scn) == '*') {
      depth += 1;
    } else if (*scn->head == '/' && scanner_peek(scn, -1) == '*') {
      depth -= 1;
    }

    t.len += 1;
    scn->head += 1;

    if (depth <= 0) {
      break;
    }
  }

  return t;
}

Token scanner_getToken(Scanner* scn) {
  scanner_skipWhitespace(scn);

  // Start with an illegal token
  Token t = {};
  t.line = scn->line;

  if (*scn->head == '\0') {
    t.type = TOKEN_EOF;
  } else if (us_isDigit(*scn->head)) {
    t = scanner_readNumber(scn);
/*  
    } else if (scanner_isKeyword(scn)) {
    t = scanner_readKeyword();
*/
  } else if (scanner_isLetter(scn)) {
    t = scanner_readIdentifier(scn);
  } else {
    switch (*scn->head) {
      case ':':
        {
          if (scanner_peek(scn) == '=') {
            t.type = TOKEN_ASSIGNMENT_DECLARATION;
            t.start = scn->head;
            t.len = 2;

            scn->head += t.len;
          }
        } break;
      case '+':
        {
          t.type = TOKEN_ADD;
          t.start = scn->head;
          t.len = 1;

          scn->head += t.len;
        } break;
      case '(':
        {
          t.type = TOKEN_BRACKET_OPEN;
          t.start = scn->head;
          t.len = 1;

          scn->head += t.len;
        } break;
      case ')':
        {
          t.type = TOKEN_BRACKET_CLOSE;
          t.start = scn->head;
          t.len = 1;

          scn->head += t.len;
        } break;
      case '/':
        {
          if (scanner_peek(scn) == '*') {
            t = scanner_readComment(scn);
          }
        } break;
      default:
        {
          printf("Something is broken!\n");
        } break;
    }

  }

  return t;
}
