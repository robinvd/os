#include "parser.h"
#include "collections.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef enum {
  INPUT,
  OUTPUT,
  BG,
  PIPE,
} Op;

int parse(String input, Line* output) {
  State state = State_new(input);

  int res = parseLine(&state, output);
  if (res < 1) {
    return false;

  }

  return true;
}

void transfer(int src, int dest) {
  char buffer[4096];
  while (true) {
    int bytes_read = read(src, buffer, sizeof(buffer));
    if (bytes_read == 0) {
      return;
    }
    int bytes_written = write(dest, buffer, bytes_read);
    if (bytes_written < 0) {
      printf("write error\n");
      exit(-1);
    }
  }
}

void run(Line line) {
  int next_in = STDOUT_FILENO;

  for (int i_cmd=line.commands.len-1; i_cmd>=0; i_cmd--) {
    Command command = line.commands.start[i_cmd];

    // output of the next program
    // input in handled by prev_out
    int fd[2];
    if (pipe(fd)) {
      printf("pipe err\n");
      exit(-1);
    }
    int output = next_in;
    int input = fd[0];

    // if there is an in ("< file") then use that
    // if (command.in.len > 0) {
      // char* path = String_to_c(command.in.start[command.in.len-1]);
      // input = open(path, O_RDONLY);
      // free(path);
    // }

    // start the subprocess
    // the parent will continue with the loop
    // and spawn the rest of the processes
    if (fork() != 0) {
      // parent
      next_in = fd[1];
      close(fd[0]);

      if (input != STDIN_FILENO) {
        close(input);
      }
    } else {
      // child
      char** args = String_to_c_arr(command.args);
    
      // set the stdin and stdout of the subprocess
      dup2(input, STDIN_FILENO);
      dup2(output, STDOUT_FILENO);
      close(fd[0]);
      close(fd[1]);
      execvp(args[0], args);
    }
  }

  // redirect output of the last process to regular stdout
  dup2(STDIN_FILENO, next_in);
  close(next_in);
  // char buffer[] = "out!\n";

  // wait
  // TODO dont wait if is_fork
  wait(NULL);

  // cleanup
  Line_drop(line);
}

int main() {
  // setbuf(stdout, NULL);
  FILE* f = stdin;

  while(true) {
    // type_prompt();
    String input = Vecchar_new();
    if (!read_line(f, &input)) {
      puts("end of file");
      break;
    }

    Line line;
    if (!parse(input, &line)) {
      printf("parse err\n");
      continue;
    }

    run(line);
    // if (fork() != 0) {
    //   int status;
    //   Vecchar_drop(input);
    //   Line_drop(line);
    //   waitpid(-1, &status, 0);
    // } else {
    //   printf("child\n");
    //   run(line);

    //   printf("error in child, control returned\n");
    //   exit(-1);
    // }
  }


  // return 0;
  // printf("success\n");
  // if (line.is_fork) {
  //   printf("forked\n");
  // }

  // // for(VecIterLine i = LineVec_iter(line); VecIterLine_next(i);) {
  // for (int i_cmd=0; i_cmd<line.commands.len; i_cmd++) {
  //   Command c = line.commands.start[i_cmd];
  //   for (int i=0; i<c.in.len; i++) {
  //     printf("in: ");
  //     println(c.in.start[i]);
  //   }
  //   for (int i=0; i<c.out.len; i++) {
  //     printf("out: ");
  //     println(c.out.start[i]);
  //   }
  //   for (int i=0; i<c.args.len; i++) {
  //     printf("parsed: ");
  //     println(c.args.start[i]);
  //   }
  // }

  return 0;
}
