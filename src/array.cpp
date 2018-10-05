// A basic dynamic array implementation. Probably should go into uslib.
//
// Usage:
//
// array_for(int);
//
// int main(void) {
//    array(int) numbers = array_int_init();
//
//    array_int_add(&numbers, 10);
//    array_int_add(&numbers, 20);
//    array_int_add(&numbers, -30);
//
//    return 0;
// }

#define ARRAY_CAPACITY_GROW(c) ( ((c) < 8) ? 8 : (c)*2 )

struct ArrayHeader {
  psize count;
  psize capacity;
};

#define array(Type) Type*

#define array_header(Array) ((ArrayHeader*) Array - 1)
#define array_count(Array) (array_header(Array)->count)

// The following macro generates two functions: one to initialise an array, one
// to add elements to previously initialised array.
//
// I can't think of a way to un-macro it right now, so it's going to stay as is.
#define array_for_name(Type, name) \
Type* array_ ## name ## _init() { \
  int initialCapacity = ARRAY_CAPACITY_GROW(0); \
\
  ArrayHeader* header = (ArrayHeader*) realloc(0, sizeof(ArrayHeader) + sizeof(Type) * initialCapacity); \
  header->count = 0; \
  header->capacity = initialCapacity; \
\
  return (Type*) (header + 1); \
} \
\
\
void array_## name ## _add (Type** array, Type value) { \
  ArrayHeader* header = array_header(*array); \
\
  if (header->capacity < header->count + 1) { \
    header->capacity = ARRAY_CAPACITY_GROW(header->capacity); \
    header = (ArrayHeader*) realloc(header, sizeof(*header) + (header->capacity * sizeof(Type))); \
\
    *array = (Type*) (header + 1); \
  } \
\
  (*array)[header->count] = value; \
  header->count += 1; \
} \
\
void array_ ## name ## _zero(Type** array) { \
  ArrayHeader* header = array_header(*array); \
  header->count = 0; \
\
  memset(*array, 0, header->capacity * sizeof(Type)); \
}\
\
void array_## name ##_copy(Type** from, Type** to) { \
  array_ ## name ## _zero(to); \
\
  for (psize i = 0; i < array_count(*from); i++) { \
    array_ ## name ## _add(to, (*from)[i]); \
  } \
}

#define array_for(Type) array_for_name(Type, Type)
