struct TableEntry {
  String key;
  Value val;

  int64 hash;
};

bool tableEntry_sameKey(TableEntry left, TableEntry right) {
  if (left.key.len != right.key.len || left.hash != right.hash) {
    return false;
  }

  return strncmp(left.key.str, right.key.str, left.key.len) == 0;
}

#define TABLE_MAX_LOAD (0.7)
#define TABLE_CAPACITY_GROW(c) ( ((c) < 1) ? 8 : (c)*2 )
struct Table {
  TableEntry* entries;

  int count;
  int capacity;
};

void table_init(Table* t) {
  t->entries = 0;
  t->count = 0;
  t->capacity = 0;
}

// From: http://www.cse.yorku.ca/~oz/hash.html
uint64 table_hash(String str) {
  uint64 hash = 5381;
  char c;

  for (int i = 0; i < str.len - 1; i++) {
    c = str.str[i];
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }

  return hash;
}

bool table_full(Table* t) {
  return t->capacity * TABLE_MAX_LOAD < t->count + 1;
}

// NOTE(harrison): INTERNAL USE ONLY.
bool table_addEntry(Table* t, TableEntry entry) {
  if (t->capacity <= 0) {
    return false;
  }

  int index = entry.hash % t->capacity;

  for (int offset = 0; offset < t->capacity; offset += 1) {
    int i = (index + offset) % t->capacity;

    if (t->entries[i].hash == 0) {
      t->entries[i] = entry;
      t->count += 1;

      return true;
    } else if (tableEntry_sameKey(entry, t->entries[i])) {
      t->entries[i] = entry;

      return true;
    }
  }

  return false;
}

void table_resize(Table* t) {
  int cap = t->capacity;
  TableEntry* entries = t->entries;

  t->capacity = TABLE_CAPACITY_GROW(t->capacity);
  t->entries = (TableEntry*) malloc(sizeof(TableEntry) * t->capacity);

  memset(t->entries, 0, sizeof(TableEntry)*t->capacity);

  for (int i = 0; i < cap; i += 1) {
    TableEntry e = entries[i];

    if (e.hash == 0) {
      continue;
    }

    assert(table_addEntry(t, e));
  }

  free(entries);
}

void table_set(Table* t, String str, Value val) {
  if (table_full(t)) {
    table_resize(t);
  }

  TableEntry e = {};
  e.key = str;
  e.val = val;
  e.hash = table_hash(str);

  assert(table_addEntry(t, e));
}

bool table_get(Table* t, String key, Value* v) {
  if (t->capacity <= 0) {
    return false;
  }

  // TODO(harrison): Is this too slow? Benchmark
  int64 index = key.hash % t->capacity;

  for (int offset = 0; offset < t->capacity; offset += 1) {
    int64 i = (index + offset) % t->capacity;
    TableEntry entry = t->entries[i];

    if (entry.hash == 0) {
      printf("hello\n");
      continue;
    }

    if (key.len == entry.key.len && key.hash == entry.hash) {
      if (strncmp(key.str, entry.key.str, key.len) == 0) {
        if (v != 0) {
          *v = t->entries[i].val;
        }

        return true;
      }
    }

    printf("fuck\n");
  }

  return false;
}
