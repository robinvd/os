#include "collections.h"

MAKEVEC(char)
MAKEVEC(String)

String str(char* input) {
  String out;
  out.start = input;
  out.len = strlen(input);

  return out;
}

void println(String str) {
  printf("%.*s\n", str.len, str.start);
}

int read_line(FILE* f, String* output) {
  char c;
  int start_len = output->len;
  while ((c = fgetc(f)) != EOF && c != '\n') {
    Vecchar_push_back(output, c);
  }

  if (c == EOF && output->len == start_len) {
    return false;
  } else {
    return true;
  }
}

char** String_to_c_arr(VecString vec) {
  char** result = malloc(sizeof(char*) * (vec.len + 1));

  for (int i=0; i<vec.len; i++) {
    String str = vec.start[i];
    result[i] = malloc(sizeof(char) * (str.len + 1));
    memcpy(result[i], str.start, str.len);
    result[i][str.len] = '\0';
  }

  result[vec.len] = NULL;

  return result;
}

char* String_to_c(String str) {
  char* result = malloc(sizeof(char) * str.len + 1);
  memcpy(result, str.start, str.len);
  result[str.len] = '\0';

  return result;
}
