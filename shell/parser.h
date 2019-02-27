#ifndef PARSER
#define PARSER

#include "collections.h"

typedef struct Command {
  VecString args;
  VecString in;
  VecString out;
} Command;
MAKEVECH(Command)

void Command_drop(Command c);

typedef struct Line {
  VecCommand commands;
  int is_fork;
} Line;

void Line_drop(Line l);

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

State State_new(String input);
void State_clear(State* state);
void State_drop(State* state);

int parseLine(State* state, Line* result);

#endif
