enum ValueType {
  VALUE_NIL,
  VALUE_NUMBER,
  VALUE_BOOL,
  VALUE_FUNCTION,

  // VALUE_OBJ
};

#define VALUE_IS_NUMBER(v) ((v).type == VALUE_NUMBER)
#define VALUE_IS_BOOL(v) ((v).type == VALUE_BOOL)

struct Value {
  ValueType type;

  union {
    float number;
    bool boolean;

    // Function function;

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

void value_print(Value v) {
  switch (v.type) {
    case VALUE_NUMBER:
      {
        printf("%f", v.as.number);
      } break;
    case VALUE_BOOL:
      {
        printf(v.as.boolean ? "true" : "false");

        break;
      }
    default:
      {
        printf("unknown: %d", v.type);
      }
  };
}

void value_println(Value v) {
  value_print(v);

  printf("\n");
}

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


