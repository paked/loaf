#include <stdlib.h> // malloc, realloc
#include <stdio.h> // printf
#include <assert.h> // assert
#include <string.h> // memcmp

// TODO(harrison): add some of above dependencies into uslib

#include <us.hpp>

#define REALLOC(Type, ptr, count) ((Type*) realloc(ptr, count * sizeof(Type)))

#include <array.cpp>

#include <bytecode.cpp>
#include <lexer.cpp>
#include <ast.cpp>
#include <parser.cpp>

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
