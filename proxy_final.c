#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <math.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#define CHILDBUFSIZE 4096
#define MAXN  128

typedef struct comms {
    char*   buffer;
    int     buffer_capacity;
    int     size;
    int     offset;
    int     dead;
    int     fd[2];
} comm;

int connect(int n, comm* connections, int* child_fd, int first_child_fd);

int main(int argc, char* argv[]){

    if(argc != 3){
      printf("incorrect input, use:  input file children number");
      return -1;
    }
    int n = atoi( argv[2] );
    int child_fd[2] = {};

    if(n < 0){
      printf("children number should be greater than zero");
      return -1;
    }
    int input_fd = open(argv[1], O_RDONLY);

    if(input_fd < 0){
      printf("can't open file!");
      return -1;
    }
    comm* connections = (comm*)calloc(n, sizeof(comm));
    
    switch (connect(n, connections, child_fd, input_fd)) {
      case 1:
        fd_set writeready;				 
        fd_set readready;
        while (1)
        {
          int maxfd = -1;
          FD_ZERO(&writeready);
          FD_ZERO(&readready);

          for(int i = 0; i < n; i++)
          {
            if(connections[i].size == 0 && !connections[i].dead)
            {
              FD_SET(connections[i].fd[0], &readready);

              if(connections[i].fd[0] > maxfd)
                maxfd = connections[i].fd[0];
            }
            if(connections[i].size != 0)
            {
              FD_SET(connections[i].fd[1], &writeready);

              if(connections[i].fd[1] > maxfd)
                maxfd = connections[i].fd[1];
            }
          }
          if(maxfd < 0)
            break;

          if(!select(maxfd+1, &readready, &writeready, NULL, NULL)){
            printf("select error");
	    return -1;
	  }

          for(int i = 0; i < n; i++){
            if(FD_ISSET(connections[i].fd[0], &readready)){
              int read_ret = read(connections[i].fd[0], connections[i].buffer, connections[i].buffer_capacity);

              if(read_ret < 0){
                printf("reading error");
		return -1;
              }
              if(read_ret == 0){
                connections[i].dead = 1;
                connections[i].size = 0;
                close(connections[i].fd[1]);
              }
              else
                connections[i].size = read_ret;
            }
            if(FD_ISSET(connections[i].fd[1], &writeready))
            {
              int write_ret = write(connections[i].fd[1], connections[i].buffer + connections[i].offset, connections[i].size);
              if(write_ret < 0){
                printf("writing error");
                return -1;
              }
              connections[i].size -= write_ret;
              if(connections[i].size == 0)
                connections[i].offset = 0;
              else
                connections[i].offset += write_ret;

            }

          }
        }
        close(input_fd);

        for(int i = 0; i < n; i++)
          free(connections[i].buffer);
        break;

      case 0:
        char* child_buffer = (char*)calloc(CHILDBUFSIZE, sizeof(char));
        int read_ret = 1;

        while(read_ret){
          read_ret = read(child_fd[0], child_buffer, CHILDBUFSIZE);
          if(read_ret < 0){
            printf("reading error");
            return -1;
	  }
          write(child_fd[1], child_buffer, read_ret);
        }
        close(child_fd[0]);
        close(child_fd[1]);
        free(child_buffer);
        break;
    }
    free(connections);
    exit(0);
}

int connect(int n, comm* connections, int* child_fd, int first_child_fd){
    int fd1[2] = {};
    int fd2[2] = {};

    if(pipe(fd1) < 0){
      printf("pipe error");
      return -1;
    }

    switch (fork()){
      case -1:
        printf("fork error");
        return -1;
      case 0:
        close(fd1[0]);
        child_fd[0] = first_child_fd;
        child_fd[1] = fd1[1];
        return 0;
      default:
        close(fd1[1]);
        connections[0].fd[0] = fd1[0];
    }

    for(int i = 0; i < n - 1; i++){
      if(pipe(fd1) < 0){
        printf("pipe error");
        return -1;
    }
      if(pipe(fd2) < 0){
        printf("pipe error");
        return -1;
    }
      switch (fork()){
        case -1:
          printf("fork error");
	  return -1;
        case 0:
          close(fd1[1]);
          close(fd2[0]);
          child_fd[0] = fd1[0];
          child_fd[1] = fd2[1];

        for(int j = 0; j < i; j++){
          close(connections[j].fd[0]);
          close(connections[j].fd[1]);
        }
        return 0;

      default:
        close(fd1[0]);
        close(fd2[1]);

        fcntl(fd1[1], F_SETFL, O_WRONLY | O_NONBLOCK);
        fcntl(fd2[0], F_SETFL, O_RDONLY | O_NONBLOCK);

        connections[i].fd[1]   = fd1[1];
        connections[i+1].fd[0] = fd2[0];
    }
  }
    connections[n - 1].fd[1] = STDOUT_FILENO;

    for(int i = 0; i < n; i++){
        if( ( n - i ) > 4 )
 	  connections[i].buffer_capacity = MAXN;
        else 
          connections[i].buffer_capacity = (int)pow(3, (double)(n - i)) * 1024;
          connections[i].size         = 0;
          connections[i].offset       = 0;
          connections[i].dead         = 0;
          connections[i].buffer          = (char*)calloc(connections[i].buffer_capacity, sizeof(char));
    }

    return 1;
}
