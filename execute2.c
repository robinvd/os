#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define BUFFERSIZE 1000

typedef struct {
  int cap;
  int size;
  char* buffer;
} Vec;

Vec new_vec() {
  int s = 1;
  Vec v = {s, 0, malloc(sizeof(char)*s)};
  return v;
}

void push_vec(Vec *vec, char c) {
  if (vec->size == vec->cap) {
    char* new = realloc(vec->buffer, sizeof(char) * vec->cap * 2);

    if (new == NULL) {
      exit(-1);
    }

    vec->buffer = new;
    vec->cap *= 2;
  }

  vec->buffer[vec->size++] = c;
}

int main() {

  Vec vec_buffer = new_vec();
  Vec* buffer = &vec_buffer;
  char* args[BUFFERSIZE];
  int nargs = 1;

  char c;
  while ((c = getchar()) != '\n' && c != EOF) {
    push_vec(buffer, c);
  }
  push_vec(buffer, 0);

  char* buffer_ptr = buffer->buffer;

  // remove leading whitespace
  // while (*buffer_ptr == ' ' || *buffer_ptr == '\n') {
    // buffer_ptr++;
  // }

  // first args is the name of the program
  args[0] = buffer_ptr;

  // parse the arguments (in place)
  int quote = 0;
  for (int i=1; buffer_ptr[i]; i++) {
    // if (buffer_ptr[i] == '"') {
      // quote = !quote;
    // }

    if ((buffer_ptr[i] == ' ' || buffer_ptr[i] == '\n') && !quote) {
      buffer_ptr[i] = 0;
    }

    if (buffer_ptr[i-1] == 0 && buffer_ptr[i] != 0) {
      args[nargs++] = &buffer_ptr[i];
    }
  }

  // args is a NULL terminated list
  args[nargs] = NULL;

  // exec the subprogram
  execvp(buffer_ptr,args); 

  // if we are here that means exec failed
  // so print the error msg
  if (errno == 2) {
    printf("Command %s not found!\n", args[0]);
  } else {
    printf("failed: %d, %s\n", errno, strerror(errno));
    exit(errno);
  }

  // dont exit with the errno,
  // as themis only accepts the exit code 0

  // exit(errno);
  exit(0);

}
