typedef int (*PrintPtr) (char const *str, ...);

int logf(const char* fmt, ...) {
  va_list args;

  va_start(args, fmt);
  int n = vfprintf(stderr, fmt, args);

  va_end(args);
  
  return n;
}
