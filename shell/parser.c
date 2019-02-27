#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "collections.h"
#include "parser.h"

MAKEVEC(Command)

State State_new(String input) {
  return (State){input, input};
}

void State_clear(State* state) {
  Vecchar_clear(&state->input);
}

void State_free(State* state) {
  Vecchar_free(state->input);
}

void Command_free(Command c) {
  VecString_free(c.args);
  VecString_free(c.in);

rst

  VecString_free(c.out);
}



void Line_drop(Line l) {
  for (int i=0; i<l.commands.len; i++) {
    Command_free(l.commands.start[i]);
  }
  VecCommand_free(l.commands);
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

  String output_local = {state->input.start, 0, 0};

  while(state->input.len > 0 && f(state->input.start[0])) {
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

int anyof(char x, String str) {
  for (int i=0; i<str.len; i++) {
    if (str.start[i] == x) {
      return true;
    }
  }

  return false;
}

int isAlpha(char x) {
  return (x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z');
}

int isAlphaNum(char x) {
  return isAlpha(x) || (x >= '0' && x <= '9');
}

int isArg(char x) {
  return isAlphaNum(x) || anyof(x, str("-/_"));
}

int parseIdent(State* state, String* output) {
  String buffer;
  int res = takeWhile(state, &buffer, isArg);

  if (res < 1) {
    return res;
  }

  parseWhitespace(state);
  *output = buffer;
  return true;

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
    // res = parseIdent(state, &command);
    res = parsePath(state, &command);
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

