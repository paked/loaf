#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <us.hpp>

#define REALLOC(Type, ptr, count) ((Type*) realloc(ptr, count * sizeof(Type)))

#include <array.cpp>
#include <lexer.cpp>
#include <bytecode.cpp>

// letter = ('a'...'z' | 'A'...'Z' | '_')
// identifier = letter { letter | number }
//  eg. `exampleThing`, `exampleThing2`, `_thingThing`

array_for(Token);

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
    } else if (t.type == TOKEN_EOF) {
      printf("Got through all tokens\n");

      break;
    } else if (t.type == TOKEN_COMMENT) {
      continue;
    }

    array_Token_add(&tokens, t);

    printf("val: %.*s\n", t.len, t.start);
  }

  printf("have %zu tokens\n", array_count(tokens));

  return test_bytecode();
}

int test_bytecode() {
  Hunk hunk = {0};

  int line = 0;

  int local1 = hunk_addLocal(&hunk, 10);
  int local2 = hunk_addLocal(&hunk, 5);

  hunk_write(&hunk, OP_GET_LOCAL, line);
  hunk_write(&hunk, local1, line++);
  hunk_write(&hunk, OP_GET_LOCAL, line);
  hunk_write(&hunk, local2, line++);

  hunk_write(&hunk, OP_MULTIPLY, line++);

  int constant = hunk_addConstant(&hunk, 25);

  hunk_write(&hunk, OP_CONSTANT, line);
  hunk_write(&hunk, constant, line++);

  hunk_write(&hunk, OP_NEGATE, line++);

  hunk_write(&hunk, OP_ADD, line++);

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
