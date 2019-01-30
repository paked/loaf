enum TokenType : uint32 {
  TOKEN_ILLEGAL,
  TOKEN_EOF,

  TOKEN_SEMICOLON,
  TOKEN_COMMA,

  TOKEN_COMMENT,

  TOKEN_IDENTIFIER,

  TOKEN_NUMBER,

  TOKEN_TRUE,
  TOKEN_FALSE,

  TOKEN_IF,
  TOKEN_FUNC,
  TOKEN_RETURN,
  TOKEN_VAR,

  TOKEN_ASSIGNMENT,
  TOKEN_ASSIGNMENT_DECLARATION,

  TOKEN_ADD,
  TOKEN_SUBTRACT,
  TOKEN_MULTIPLY,
  TOKEN_DIVIDE,

  TOKEN_EQUALS,
  TOKEN_GREATER,
  TOKEN_LESSER,

  TOKEN_AND,
  TOKEN_OR,

  TOKEN_BRACKET_OPEN,  // (
  TOKEN_BRACKET_CLOSE, // (

  TOKEN_CURLY_OPEN,   // {
  TOKEN_CURLY_CLOSE,  // }

  TOKEN_LOG
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

  bool reachedNewline;
  Token lastToken;
};

void scanner_load(Scanner* scn, char* buf) {
  scn->source = buf;
  scn->head = buf;
  scn->line = 1;
}

void scanner_skipWhitespace(Scanner* scn) {
  while (*scn->head != '\0') {
    char c = *scn->head;

    if (us_isNewline(c)) {
      scn->reachedNewline = true;
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
      t.type = TOKEN_ILLEGAL;
      break;
    }

    t.len += 1;
    scn->head += 1;
  }

  // true
  // false
  // if
  // func
  // var
  // return
  if (t.len == 4) {
    if (strncmp(t.start, "true", 4) == 0) {
      t.type = TOKEN_TRUE;
    } else if (strncmp(t.start, "func", 4) == 0) {
      t.type = TOKEN_FUNC;
    }
  } else if (t.len == 5) {
    if (strncmp(t.start, "false", 5) == 0) {
      t.type = TOKEN_FALSE;
    }
  } else if (t.len == 3) {
    if (strncmp(t.start, "log", 3) == 0) {
      t.type = TOKEN_LOG;
    } else if (strncmp(t.start, "var", 3) == 0) {
      t.type = TOKEN_VAR;
    }
  } else if (t.len == 2) {
    if (strncmp(t.start, "if", 2) == 0) {
      t.type = TOKEN_IF;
    }
  } else if (t.len == 6) {
    if (strncmp(t.start, "return", 6) == 0) {
      t.type = TOKEN_RETURN;
    }
  }

  return t;
}

Token scanner_readMultilineComment(Scanner* scn) {
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

    if (scanner_peek(scn) == '\n') {
      scn->line += 1;
    }

    t.len += 1;
    scn->head += 1;

    if (depth <= 0) {
      break;
    }
  }

  return t;
}

Token scanner_readSinglelineComment(Scanner* scn) {
  Token t = {};
  t.type = TOKEN_COMMENT;
  t.line = scn->line;
  t.start = scn->head;

  scn->head += 2;
  t.len += 2;

  while (*scn->head != '\0') {
    char c = *scn->head;

    if (us_isNewline(c)) {
      break;
    }

    t.len += 1;
    scn->head += 1;
  }

  return t;
}

Token scanner_getToken(Scanner* scn) {
  scanner_skipWhitespace(scn);

  if (scn->reachedNewline) {
    Token breakToken = {};
    breakToken.type = TOKEN_SEMICOLON;
    breakToken.start = scn->head;
    breakToken.len = 0;
    breakToken.line = scn->line;

    switch (scn->lastToken.type) {
      case TOKEN_NUMBER:
      case TOKEN_TRUE:
      case TOKEN_FALSE:
      case TOKEN_IDENTIFIER:
      case TOKEN_BRACKET_CLOSE:
      case TOKEN_LOG:
        {
          scn->lastToken = breakToken;

          return breakToken;
        } break;
      default:
        {
          // whatever
        } break;
    }

    scn->reachedNewline = false;
  }

  // Start with an illegal token
  Token t = {};
  t.line = scn->line;

  if (*scn->head == '\0') {
    t.type = TOKEN_EOF;
  } else if (us_isDigit(*scn->head)) {
    t = scanner_readNumber(scn);
  } else if (scanner_isLetter(scn)) {
    // NOTE(harrison): this will also map keywords
    t = scanner_readIdentifier(scn);
  } else {
#define SIMPLE_TOKEN(Type) \
    do { \
      t.type = Type; \
      t.start = scn->head; \
      t.len = 1; \
\
      scn->head += t.len; \
    } while (false)
#define SIMPLE_CASE(Char, Type) \
    case Char: \
      { \
        SIMPLE_TOKEN(Type); \
      } break;
    switch (*scn->head) {
      case '=':
        {
          if (scanner_peek(scn) == '=') {
            t.type = TOKEN_EQUALS;
            t.start = scn->head;
            t.len = 2;

            scn->head += t.len;
          } else {
            SIMPLE_TOKEN(TOKEN_ASSIGNMENT);
          }
        } break;
      case ':':
        {
          if (scanner_peek(scn) == '=') {
            t.type = TOKEN_ASSIGNMENT_DECLARATION;
            t.start = scn->head;
            t.len = 2;

            scn->head += t.len;
          }
        } break;
      case '/':
        {
          if (scanner_peek(scn) == '*') {
            t = scanner_readMultilineComment(scn);
          } else if (scanner_peek(scn) == '/') {
            t = scanner_readSinglelineComment(scn);
          } else {
            SIMPLE_TOKEN(TOKEN_DIVIDE);
          }
        } break;
      case '&':
        {
          if (scanner_peek(scn) == '&') {
            t.type = TOKEN_AND;
            t.start = scn->head;
            t.len = 2;

            scn->head += t.len;
          }
        } break;
      case '|':
        {
          if (scanner_peek(scn) == '|') {
            t.type = TOKEN_OR;
            t.start = scn->head;
            t.len = 2;

            scn->head += t.len;
          }
        } break;
      SIMPLE_CASE(',', TOKEN_COMMA);
      SIMPLE_CASE('<', TOKEN_LESSER);
      SIMPLE_CASE('>', TOKEN_GREATER);
      SIMPLE_CASE(';', TOKEN_SEMICOLON);
      SIMPLE_CASE('+', TOKEN_ADD);
      SIMPLE_CASE('-', TOKEN_SUBTRACT);
      SIMPLE_CASE('*', TOKEN_MULTIPLY);
      SIMPLE_CASE('(', TOKEN_BRACKET_OPEN);
      SIMPLE_CASE(')', TOKEN_BRACKET_CLOSE);
      SIMPLE_CASE('{', TOKEN_CURLY_OPEN);
      SIMPLE_CASE('}', TOKEN_CURLY_CLOSE);
      default:
        {
          assert(!"Something is broken!\n");
        } break;
    }
#undef SIMPLE_TOKEN
#undef SIMPLE_CASE
  }

  if (t.type != TOKEN_COMMENT) {
    scn->lastToken = t;
  }

  return t;
}
