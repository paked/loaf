enum ValueType {
  VALUE_NIL,

  VALUE_NUMBER,
  VALUE_BOOL,
  VALUE_STRING,

  VALUE_FUNCTION,

  // VALUE_OBJ
};

#define VALUE_IS_NUMBER(v) ((v).type == VALUE_NUMBER)
#define VALUE_IS_BOOL(v) ((v).type == VALUE_BOOL)

struct Hunk;

struct Function {
  Hunk* hunk;
};

struct String {
  char* str;
  int len;

  int64 hash;
};

uint64 table_hash(String str);

void string_make(String* s, char* start, int len) {
  s->len = len + 1;
  // TODO(harrison): free
  s->str = (char*) malloc(s->len * sizeof(char));

  strncpy(s->str, start, len);
  s->str[len] = '\0';

  s->hash = table_hash(*s);
}

struct Value {
  ValueType type;

  union {
    float number; // VALUE_NUMBER
    bool boolean; // VALUE_BOOL
    Function function; // VALUE_FUNCTION
    String string; // VALUE_STRING

    // Object* object;
  } as;
};

Value value_make(ValueType type) {
  Value v = {};
  v.type = type;

  switch (v.type) {
    case VALUE_NUMBER:
      { v.as.number = 0; } break;
    case VALUE_BOOL:
      { v.as.boolean = false; } break;
    default:
      {
        assert(!"Unknown value type");
      }
  }

  return v;
}

Value value_make(float t) {
  Value v = {};
  v.type = VALUE_NUMBER;

  v.as.number = t;

  return v;
}

Value value_make(bool t) {
  Value v = {};
  v.type = VALUE_BOOL;

  v.as.boolean = t;

  return v;
}

Value value_make(Hunk* hunk) {
  Function f = {};
  f.hunk = hunk;

  Value v = {};

  v.type = VALUE_FUNCTION;
  v.as.function = f;

  return v;
}

Value value_make(char* start, int len) {
  String s = {};
  string_make(&s, start, len);

  Value v = {};
  v.type = VALUE_STRING;
  v.as.string = s;

  return v;
}

void value_printTo(PrintPtr f, Value v) {
  switch (v.type) {
    case VALUE_NUMBER:
      {
        f("%f", v.as.number);
      } break;
    case VALUE_BOOL:
      {
        f(v.as.boolean ? "true" : "false");
      } break;
    case VALUE_STRING:
      {
        f("'%s'", v.as.string.str);
      } break;
    case VALUE_FUNCTION:
      {
        f("[function @ %p]", v.as.function.hunk);
      } break;
    default:
      {
        f("unknown: %d", v.type);
      }
  };
}

void value_printlnTo(PrintPtr f, Value v) {
  value_printTo(f, v);

  f("\n");
}

#define value_print(v) value_printTo(printf, (v))
#define value_println(v) value_printlnTo(printf, (v))

#define value_log(v) value_printTo(logf, (v))
#define value_logln(v) value_printlnTo(logf, (v))

bool value_equals(Value left, Value right) {
  if (left.type != right.type) {
    // TODO(harrison): this should not be possible. We need a type system.

    assert(!"Can't compare variables of different types");
  }

  switch (left.type) {
    case VALUE_BOOL:
      {
        return left.as.boolean == right.as.boolean;
      } break;
    case VALUE_NUMBER:
      {
        return us_equals(left.as.number, right.as.number);
      } break;
    default:
      {
        assert(!"Unknown type");
      }
  }

  return false;
}
