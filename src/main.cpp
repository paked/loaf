#include <stdlib.h>
#include <stdio.h>

#include <us.hpp>

#define REALLOC(Type, ptr, count) ((Type*) realloc(ptr, count * sizeof(Type)))

#include <bytecode.cpp>

enum TokenType : uint32 {
  TOKEN_ILLEGAL,
  TOKEN_EOF,

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

char scanner_peek(Scanner* scn) {
  return *(scn->head + 1);
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
          }
        } break;
      case '+':
        {
          t.type = TOKEN_ADD;
          t.start = scn->head;
          t.len = 1;
        } break;
      case '(':
        {
          t.type = TOKEN_BRACKET_OPEN;
          t.start = scn->head;
          t.len = 1;
        } break;
      case ')':
        {
          t.type = TOKEN_BRACKET_CLOSE;
          t.start = scn->head;
          t.len = 1;
        } break;
      default:
        {
          printf("Something is broken!\n");
        } break;
    }

    scn->head += t.len;
  }

  return t;
}

// letter = ('a'...'z' | 'A'...'Z' | '_')
// identifier = letter { letter | number }
//  eg. `exampleThing`, `exampleThing2`, `_thingThing`

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

  Token t;
  while (true) {
    t = scanner_getToken(&scanner);

    if (t.type == TOKEN_ILLEGAL) {
      printf("ERROR lexing code\n");

      break;
    } else if (t.type == TOKEN_EOF) {
      printf("Got through all tokens\n");

      break;
    }

    printf("val: %.*s\n", t.len, t.start);
  }

  return test_bytecode();
}

int test_bytecode() {
  Hunk hunk = {0};

  int line = 0;

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
  }

  VM vm = {0};

  vm_load(&vm, &hunk);

  ProgramResult res = vm_run(&vm);

  if (res != PROGRAM_RESULT_OK) {
    printf("Program failed executing...");
    return 1;
  }

  printf("Program finished executing...\n");

  return 0;
}
