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
      perror("write error");
      exit(-1);
    }
  }
}

bool is_interactive;
int shell_in;

// launch a program, described by Command c.
// on success, execs the command.
// on error, send the errno to the err pipe (pipeerr).
void launch(Command c, int pgid, int pipeerr, int in, int out, bool foreground) {
  pid_t pid = getpid();
  if (pgid == 0) {
    pgid = pid;
  }
  setpgid (pid, pgid);
  if (foreground) {
    tcsetpgrp(shell_in, pgid);
  }

  // if there is an in/out file then use that
  if (c.in.len > 0) {
    if (in != STDIN_FILENO) {
      close(in);
    }

    char* path = String_to_c(c.in.start[c.in.len-1]);
    in = open(path, O_RDONLY);
    free(path);

    if (in == -1) {
      int res = -1;
      write(pipeerr, &res, sizeof(int));
      exit(-1);
    }
  }
  if (c.out.len > 0) {
    if (out != STDOUT_FILENO) {
      close(out);
    }

    char* path = String_to_c(c.out.start[c.out.len-1]);
    out = open(path, O_WRONLY);
    free(path);
    if (out == -1) {
      int res = -1;
      write(pipeerr, &res, sizeof(int));
      exit(-1);
    }
  }

  if (in != STDIN_FILENO) {
    dup2(in, STDIN_FILENO);
    close(in);
  }
  if (out != STDOUT_FILENO) {
    dup2(out, STDOUT_FILENO);
    close(out);
  }

  char** args = String_to_c_arr(c.args);

  fcntl(pipeerr, F_SETFD, FD_CLOEXEC);
  execvp(args[0], args);

  write(pipeerr, &errno, sizeof(int));

  exit(-1);
}

int run(Line line) {
  pid_t* pid_arr = malloc(sizeof(pid_t) * line.commands.len);
  int last_out;
  if (line.is_fork) {
    last_out = -1;
  } else {
    last_out = STDIN_FILENO;
  }
	int ret_val = 0;

  int err_fd[2];
  if (pipe(err_fd)) {
    perror("pipe err");
    exit(-1);
  }

  for (int i_cmd=0; i_cmd<line.commands.len; i_cmd++) {
    Command command = line.commands.start[i_cmd];

    // input of the next program
    // output in handled by last_out
    int fd[2];
    if (i_cmd == line.commands.len-1) {
      fd[0] = -1;
      fd[1] = STDOUT_FILENO;
    } else if (pipe(fd)) {
      perror("pipe");
      exit(-1);
    }

    // start the subprocess
    // the parent will continue with the loop
    // and spawn the rest of the processes
    pid_arr[i_cmd] = fork();
    if (pid_arr[i_cmd] == -1) {
      perror("fork err");
      exit(-1);
    } else if (pid_arr[i_cmd] == 0) {
      // child
      close(err_fd[0]);

      if (fd[0] != -1) {
        close(fd[0]);
      }
    
      launch(command, pid_arr[0], err_fd[1], last_out, fd[1], false);
    }

    // parent

    if (fd[1] != STDOUT_FILENO) {
      close(fd[1]);
    }
    if (last_out != STDIN_FILENO) {
      close(last_out);
    }
    last_out = fd[0];
  }

  // dont close last_out, as it is stdout
  // close(last_out)

	close(err_fd[1]);

  // check for errors in the command.
  // if an error is found, kill all the childs.
	int msg;
	int b_read = read(err_fd[0], &msg, sizeof(int));
	if (b_read > 0) {
    kill(-pid_arr[0], SIGKILL);
		ret_val = msg;
  }

  if (line.is_fork) {
    // TODO add this to a running list, and dont wait
  }
  // for (int i=0; i<line.commands.len; i++)
    // waitpid(pid_arr[i], NULL, WUNTRACED);
  // }
  while (waitpid(-pid_arr[0], NULL, 0) != -1);

  // cleanup
  Line_drop(line);
  free(pid_arr);

	return ret_val;
}

int run_buildin(String input) {
  if (input.len == strlen("exit") && memcmp(input.start, "exit", input.len) == 0) {
    exit(0);
  }
  
  return false;
}

int main() {
  shell_in = STDIN_FILENO;
  is_interactive = isatty(shell_in);
  setbuf(stdout, NULL);

  while(true) {
    // type_prompt
    if (is_interactive) {
      printf("> ");
    }

    // reading input
    String input = Vecchar_new();
    if (!read_line(stdin, &input)) {
      // read_line returns 0 on EOF
      // so stop the main loop here.
      Vecchar_free(input);
      break;
    }

    if (run_buildin(input)) {
      Vecchar_free(input);
      continue;
    }

    // parsing the input line
    Line line;
    if (!parse(input, &line)) {
      puts("Invalid syntax!");
      Vecchar_free(input);
      continue;
    }

    // execute the command(s)
    int res = run(line);
		if (res == 2) {
			puts("Error: command not found!");
    } else if (res == -1) {
			puts("Error: file not found!");
		} else if (res != 0) {
			printf("failed: %d, %s\n", errno, strerror(errno));
		}

    // cleanup
    // Line_drop(line);
    Vecchar_free(input);
  }

  return 0;
}
