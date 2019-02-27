#include "parser.h"
#include "collections.h"
#include <stdio.h>
#include <errno.h>
#include <signal.h>
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

int run(Line line) {
  pid_t* pid_arr = malloc(sizeof(pid_t) * line.commands.len);
  int next_in = STDOUT_FILENO;
	int ret_val = 0;

  int err_fd[2];
  if (pipe(err_fd)) {
    printf("pipe err\n");
    exit(-1);
  }

  for (int i_cmd=line.commands.len-1; i_cmd>=0; i_cmd--) {
    Command command = line.commands.start[i_cmd];

    // output of the next program
    // input in handled by prev_out
    int fd[2];
    if (pipe(fd)) {
      ret_val = errno;
      
      // dont exit, as we want to clean up the processes
      break;
    }
    int output = next_in;
    int input = fd[0];

    // if there is an in ("< file") then use that
    if (command.in.len > 0) {
      char* path = String_to_c(command.in.start[command.in.len-1]);
      input = open(path, O_RDONLY);
      free(path);
    }
    // if there is an out ("> file") then use that
    if (command.out.len > 0) {
      char* path = String_to_c(command.out.start[command.out.len-1]);
      output = open(path, O_WRONLY);
      free(path);
    }
    
    // printf("< %d > %d: ", input, output);
    // println(command.args.start[0]);

    // start the subprocess
    // the parent will continue with the loop
    // and spawn the rest of the processes
    pid_arr[i_cmd] = fork();
    if (pid_arr[i_cmd] == -1) {
      puts("fork err");
    } else if (pid_arr[i_cmd] == 0) {
      // child
      close(err_fd[0]);
      close(fd[1]);
      char** args = String_to_c_arr(command.args);
    
      // set the stdin and stdout of the subprocess
      dup2(input, STDIN_FILENO);
      dup2(output, STDOUT_FILENO);
			fcntl(err_fd[1], F_SETFD, FD_CLOEXEC);
      execvp(args[0], args);

      close(input);
      close(output);
      write(err_fd[1], &errno, sizeof(int));
			close(err_fd[1]);

      exit(-1);
    }

    // parent
    next_in = fd[1];
    close(fd[0]);
    if (input > 3) {
      close(input);
    }
    if (output > 3) {
      close(output);
    }
  }

  // redirect output of the last process to regular stdout
  dup2(STDIN_FILENO, next_in);
  close(next_in);
	close(err_fd[1]);
  // char buffer[] = "out!\n";

  // wait
  // TODO dont wait if is_fork
	char msg;
	int b_read = read(err_fd[0], &msg, sizeof(int));

	if (b_read > 0) {
		for (int i=0; i<line.commands.len; i++) {
			kill(pid_arr[i], SIGKILL);
		}

		ret_val = msg;
  }

  for (int i=0; i<line.commands.len; i++) {
    waitpid(pid_arr[i], NULL, WUNTRACED);
  }

  // cleanup
  Line_drop(line);

	return ret_val;
}

int main() {
  FILE* f = stdin;

  while(true) {
    // type_prompt();
    printf("> ");
    // parsing
    String input = Vecchar_new();
    if (!read_line(f, &input)) {
      // read_line returns 0 on EOF
      // so stop the main loop here.
      break;
    }

    Line line;
    if (!parse(input, &line)) {
      printf("Invalid syntax!");
      continue;
    }

    int res = run(line);
		if (res == 2) {
			puts("Error: command not found!");
		} else if (res != 0) {
			printf("failed: %d, %s\n", errno, strerror(errno));
		}
  }

  return 0;
}
