#include <stdlib.h> // malloc, realloc
#include <stdio.h> // logf
#include <assert.h> // assert
#include <string.h> // memcmp
#include <stdarg.h>

// TODO(harrison): add some of above dependencies into uslib

#include <us.hpp>

#define REALLOC(Type, ptr, count) ((Type*) realloc(ptr, count * sizeof(Type)))

#include <debug.cpp>

#include <array.cpp>
#include <value.cpp>

#include <table.cpp>

#include <bytecode.cpp>
#include <lexer.cpp>
#include <ast.cpp>
#include <typing.cpp>
#include <parser.cpp>

int main(int argc, char** argv) {
  // TODO(harrison): make sure there is a file here
  FILE* f = fopen(argv[1], "rb");
  if (f == 0) {
    logf("ERROR: can't open file\n");

    return -1;
  }

  fseek(f, 0L, SEEK_END);
  psize fSize = ftell(f);
  rewind(f);

  char* buffer = (char*) malloc(fSize + 1);
  if (buffer == 0) {
    logf("ERROR: not enough memory to read file\n");

    return -1;
  }

  psize bytesRead = fread(buffer, sizeof(char), fSize, f);

  if (bytesRead != fSize) {
    logf("ERROR: could not read file into memory\n");

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
      logf("ERROR lexing code\n");

      break;
    } else if (t.type == TOKEN_COMMENT) {
      continue;
    }

    array_Token_add(&tokens, t);

    logf("val: %.*s (%d)\n", t.len, t.start, t.type);

    if (t.type == TOKEN_EOF) {
      logf("Got through all tokens\n");

      break;
    }
  }

  Hunk hunk = {};
  hunk_init(&hunk);

  Parser parser = {};
  parser_init(&parser, tokens);

  if (!parser_parse(&parser, &hunk)) {
    logf("Couldn't parse program...\n");

    return -1;
  }

  VM vm = {0};

  vm_load(&vm, &hunk);

  ProgramResult res = vm_run(&vm);

  if (res != PROGRAM_RESULT_OK) {
    logf("Program failed executing...\n");
    return 1;
  }

  logf("Program finished executing...\n");

  return 0;
}
