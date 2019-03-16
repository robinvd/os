#include<stdio.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<string.h> 
#include<sys/wait.h> 

// contants
int p_id = 0;
int max_msg = 50;

// set in the main_f
int max_p;
int root_write_fd;

void main_loop(int reader, int writer) {
  int data;

  // loop till EOF is reached
  while (read(reader, &data, sizeof(int)) != 0) {
    data += 1;

    printf("relative pid=%d: %d\n", p_id, data);
    if (data >= max_msg) {
      break;
    }

    write(writer, &data, sizeof(int));
  }

  close(reader);
  close(writer);
  wait(NULL);
}

int make_sub_process() {
  int fd[2];
  if (pipe(fd)==-1)  { 
      fprintf(stderr, "Pipe Failed" ); 
      return 1; 
  }

  pid_t p = fork(); 
  
  if (p < 0)  { 
      fprintf(stderr, "fork Failed" ); 
      return 1; 
  } else if (p > 0)  { 
    // Parent process 
    close(fd[0]);  // Close reading end of first pipe 

    // return writing part
    return fd[1];
  } else { 
    // child process 
    p_id += 1;
    close(fd[1]);  // Close writing end of first pipe 

    int writer;
    if (p_id >= max_p - 1) {
      writer = root_write_fd;
    } else {
      // not the last one thus fork and use that writer
      writer = make_sub_process();
      
      // we arent using the main_write
      // thus close the fd
      close(root_write_fd);
    }

    int reader = fd[0];
    main_loop(reader, writer);

    exit(0);

  }
}

int main() { 
  // setup from PDF
  setbuf(stdout, NULL);
  scanf("%d", &max_p);

  int fd[2];
  int data = 0;

  // make the 'main' pipe
  if (pipe(fd)==-1)  { 
      fprintf(stderr, "Pipe Failed" ); 
      return 1; 
  }
  root_write_fd = fd[1];
  int main_read_fd = fd[0];

  int main_send;
  if (max_p == 1) {
    main_send = fd[1];
  } else {
    main_send = make_sub_process();

    // close after forking in make_sub_process()
    close(fd[1]);
  }


  // write the initial message to the pipe
  //
  // "Each process receives from its left neighbour an integer,
  // prints its relative process-id and the number on the screen"
  //
  // if only "receiving" should be printed,
  // then how does the msg with pid=0 and data=0 ever get printed
  // pid=0 never receives the first msg only sends it.
  printf("relative pid=%d: %d\n", p_id, data);
  write(main_send, &data, sizeof(int));

  // enter the main loop we also respond to messages
  main_loop(main_read_fd, main_send);

  exit(0);
}
