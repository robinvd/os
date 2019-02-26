#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAKELIST(itemT) \
typedef struct ListInner ## itemT { \
  itemT item; \
  struct ListInner ## itemT* next; \
} ListInner ## itemT; \
\
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

#define MAKEVEC(itemT) \
typedef struct Vec ## itemT {\
  itemT* start; \
  int len; \
  int cap; \
} Vec ## itemT; \
void Vec ## itemT ## _push_back(Vec ## itemT* vec, itemT item) { \
  if (vec->len == vec->cap) { \
    vec->cap *= 2; \
    vec->start = realloc(vec->start, sizeof(itemT)*vec->cap); \
    if (vec->start == NULL) { \
      printf("err allocating new vec space\n"); \
      exit(-1); \
    } \
  } \
  vec->start[vec->len] = item; \
  vec->len += 1; \
} \
Vec ## itemT Vec ## itemT ## _new() { \
  return (Vec ## itemT){malloc(32*sizeof(itemT)), 0, 32}; \
} \
// VecIter ## itemT Vec ## itemT ## _iter() { \
//
// }


typedef enum {
  INPUT,
  OUTPUT,
  BG,
  PIPE,
} Op;

typedef struct {
  char* start;
  int len;
} String;
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

// the state struct for the parser
// parsers should take this as input
// and 
// return 0 for failed, no input taken
// return -1 for failed, input taken
// return 1 for success
typedef struct {
  String total_input;
  String input;
} State;

State state_new(String input) {
  return (State){input, input};
}

int advance(State* state, int n) {
  if (n > state->input.len) {
    return false;
  }

  state->input.start += n;
  state->input.len -= n;

  return true;
}

int take(State* state, String* output, int n) {
  char* ptr = state->input.start;

  int res = advance(state, n);
  if (!res) {
    return res;
  }

  output->start = ptr;
  output->len = n;
  
  return true;
}

int takeWhile(State* state_start, String* output, int (*f)(char)) {
  State state_local = *state_start;
  State* state = &state_local;

  String output_local = {state->input.start, 0};

  while(f(state->input.start[0]) && state->input.len > 0) {
    output_local.len += 1;
    advance(state, 1);
  }

  if (output_local.len == 0) {
    return false;
  }

  *state_start = *state;
  *output = output_local;
  return true;
}

int isWhitespace(char x) {
  return x == ' ' || x == '\n' || x == '\t';
}

void parseWhitespace(State* state) {
  String buffer;
  takeWhile(state, &buffer, isWhitespace);
}

int parseString(State* state, String* output, String input) {
  if (input.len > state->input.len) {
    return false;
  }

  int res = memcmp(state->input.start, input.start, sizeof(char)*input.len);

  if (res != 0) {
    return false;
  }
  
  *output = state->input;
  output->len = input.len;

  state->input.start += input.len;
  state->input.len -= input.len;

  parseWhitespace(state);
  return true;
} 

int parseEOF(State* state) {
  if (state->input.len == 0) {
    return true;
  } else {
    return false;
  }
}

int isAlpha(char x) {
  return (x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z');
}

int isAlphaNum(char x) {
  return isAlpha(x) || (x >= '0' && x <= '9');
}

int parseIdent(State* state, String* output) {
  String buffer;
  int res = takeWhile(state, &buffer, isAlpha);

  if (res < 1) {
    return res;
  }

  parseWhitespace(state);
  *output = buffer;
  return true;

}

int anyof(char x, String str) {
  for (int i=0; i<str.len; i++) {
    if (str.start[i] == x) {
      return true;
    }
  }

  return false;
}

int isPathChar(char x) {
  return !anyof(x, str("& \t\n|<>"));
}

int parsePath(State* state, String* output) {
  String buffer;
  int res = takeWhile(state, &buffer, isPathChar);
  if (res < 1) {
    return res;
  }

  parseWhitespace(state);
  *output = buffer;
  return true;
}

typedef struct Command {
  VecString args;
  VecString in;
  VecString out;
} Command;
MAKEVEC(Command)

typedef struct Line {
  VecCommand commands;
  int is_fork;
} Line;

int parseLine(State* state, Line* result) {
  int res = 0;
  String resBuffer;
  Command curr = {VecString_new(), VecString_new(), VecString_new()};
  Line line = {VecCommand_new(), false};

  // in pseudo code:
  // sepby("|", many1(
  //   oneof(
  //     ("<", path),
  //     (">", path),
  //     (arg),
  //   )
  // ))
  while (1) {
    res = parseString(state, &resBuffer, str("<"));
    if (res > 0) {
      String path;
      res = parsePath(state, &path);

      if (res < 1) {
        return -1;
      }

      VecString_push_back(&curr.in, path);
      continue;
    }

    res = parseString(state, &resBuffer, str(">"));
    if (res > 0) {
      String path;
      res = parsePath(state, &path);

      if (res < 1) {
        return -1;
      }

      VecString_push_back(&curr.out, path);
      continue;
    }

    String command;
    res = parseIdent(state, &command);
    if (res < 0) {
      return -1;
    }
    if (res > 0) {
      VecString_push_back(&curr.args, command);
      continue;
    }

    res = parseString(state, &resBuffer, str("|"));
    if (res < 0) {
      return -1;
    }

    if (res >= 0) {
      VecCommand_push_back(&line.commands, curr);
      curr = (Command){VecString_new(), VecString_new(), VecString_new()};
    }
    if (res == 0) {
      // stop if there is no new operator
      break;
    }
  }

  // parseFork;
  res = parseString(state, &resBuffer, str("&"));
  if (res) {
    line.is_fork = true;
  }

  if (parseEOF(state) < 1) {
    return -1;
  }

  // todo remove
  *result = line;
  return true;
}

Line parse(String input) {
  State state = state_new(input);

  Line result;
  int res = parseLine(&state, &result);
  if (res < 1) {
    printf("err: (%d)\n", res);
    printf("remaining: (%d)'%.*s'\n", state.input.len, state.input.len, state.input.start);
  }

  return result;

}

int main() {
  setbuf(stdout, NULL);

  Line line = parse(str("< in <intwo > out echo test one | echo test two > out2 &"));
  printf("success\n");

  if (line.is_fork) {
    printf("forked\n");
  }

  // for(VecIterLine i = LineVec_iter(line); VecIterLine_next(i);) {
  for (int i_cmd=0; i_cmd<line.commands.len; i_cmd++) {
    Command c = line.commands.start[i_cmd];
    for (int i=0; i<c.in.len; i++) {
      printf("in: ");
      println(c.in.start[i]);
    }
    for (int i=0; i<c.out.len; i++) {
      printf("out: ");
      println(c.out.start[i]);
    }
    for (int i=0; i<c.args.len; i++) {
      printf("parsed: ");
      println(c.args.start[i]);
    }
  }

  return 0;
}
