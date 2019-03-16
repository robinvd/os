#ifndef COLLECTIONS
#define COLLECTIONS

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAKELISTH(itemT) \
typedef struct ListInner ## itemT { \
  itemT item; \
  struct ListInner ## itemT* next; \
} ListInner ## itemT; \
void List ## itemT ## _push_back(List ## itemT* list, itemT item); \
List ## itemT Vec ## itemT ## _new(); \

#define MAKELIST(itemT) \
typedef struct List ## itemT {\
  ListInner ## itemT* start; \
  ListInner ## itemT* end; \
} List ## itemT; \
void List ## itemT ## _push_back(List ## itemT* list, itemT item) { \
  ListInner ## itemT* new = malloc(sizeof(ListInner ## itemT)); \
  if (list->end == NULL) { \
    list->start = new; \
    list->end = new; \
  } else { \
    list->end->next = new; \
    list->end = list->end->next; \
  } \
  list->end->item = item; \
} \
List ## itemT Vec ## itemT ## _new() { \
  return (List ## itemT){NULL, NULL}; \
}

#define MAKEVECH(itemT) \
typedef struct Vec ## itemT {\
  itemT* start; \
  int len; \
  int cap; \
} Vec ## itemT; \
void Vec ## itemT ## _push_back(Vec ## itemT* vec, itemT item); \
Vec ## itemT Vec ## itemT ## _new(); \
Vec ## itemT Vec ## itemT ## _take(Vec ## itemT vec, size_t n); \
Vec ## itemT Vec ## itemT ## _drop(Vec ## itemT vec, size_t n); \
void Vec ## itemT ## _free(Vec ## itemT vec); \
void Vec ## itemT ## _clear(Vec ## itemT* vec); \
int Vec ## itemT ## _eq(Vec ## itemT a, Vec ## itemT b); \

#define MAKEVEC(itemT) \
void Vec ## itemT ## _push_back(Vec ## itemT* vec, itemT item) { \
  if (vec->len == vec->cap) { \
    vec->cap *= 2; \
    vec->start = (itemT*)realloc(vec->start, sizeof(itemT)*vec->cap); \
    if (vec->start == NULL) { \
      printf("err allocating new vec space\n"); \
      exit(-1); \
    } \
  } \
  vec->start[vec->len] = item; \
  vec->len += 1; \
} \
Vec ## itemT Vec ## itemT ## _new() { \
  return (Vec ## itemT){(itemT*)malloc(32*sizeof(itemT)), 0, 32}; \
} \
void Vec ## itemT ## _free(Vec ## itemT vec) { \
  if (vec.cap > 0) { \
    free(vec.start); \
  } \
} \
void Vec ## itemT ## _clear(Vec ## itemT* vec) { \
  vec->start = 0; \
} \
Vec ## itemT Vec ## itemT ## _take(Vec ## itemT vec, size_t n) { \
  return (Vec ## itemT) {vec.start, n, 0}; \
} \
Vec ## itemT Vec ## itemT ## _drop(Vec ## itemT vec, size_t n) { \
  return (Vec ## itemT) {vec.start + n, vec.len-n, 0}; \
} \
Vec ## itemT Vec ## itemT ## _clone(Vec ## itemT vec) { \
  Vec ## itemT res = {malloc(sizeof(itemT) * vec.len), vec.len, vec.len}; \
  memcpy(res.start, vec.start, sizeof(itemT) * vec.len); \
  return res; \
} \
int Vec ## itemT ## _eq(Vec ## itemT a, Vec ## itemT b) { \
  if (a.len == b.len && memcmp(a.start, b.start, a.len) == 0) { \
    return true; \
  } else { \
    return false; \
  } \
} \
// VecIter ## itemT Vec ## itemT ## _iter() { \
//
// }

// typedef struct {
  // char* start;
  // int len;
// } String;
// MAKEVECH(String)

MAKEVECH(char)

typedef Vecchar String;
MAKEVECH(String)

String str(char* input);
int read_line(FILE* f, String* output);
void println(String str);
char** String_to_c_arr(VecString vec);
char* String_to_c(String vec);

#endif
