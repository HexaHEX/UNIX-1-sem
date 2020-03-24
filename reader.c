#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define     BUFFSIZE	2048
const char* ADDRFIFO = "/tmp/addrfifo";
char pidname[12];


int main()
{
	int  n;
	char buf[BUFFSIZE];
	int  pid_int = getpid();

	
	sprintf(pidname, "%d", pid_int);

	if((mkfifo(ADDRFIFO, 0644) < 0) && (errno != EEXIST)){
		printf("mkfifo error");
		exit(1);
	}
	int addr_fd = open(ADDRFIFO, O_WRONLY);
	
	if((addr_fd < 0) && (errno != EEXIST)){
		printf("open fifo error");
		exit(1);
	}
	
	if((mkfifo(pidname, 0644) < 0) && (errno != EEXIST)){
		printf("mkfifo error");
		exit(1);
	}
	int main_fd = open(pidname, O_RDONLY | O_NONBLOCK);
	
	if((main_fd < 0) && (errno != EEXIST)){
		printf("open fifo error");
		exit(1);
	}

	if (write(addr_fd, &pid_int, sizeof(int)) != sizeof(int)){
		printf("write error");
		exit(1);
	}
	close(addr_fd);

	int succesful_rd = 0;

	for(int i = 0; i < 15; i++){
		if((n = read(main_fd, buf, BUFFSIZE)) == 0){
			sleep(1);
		}
		if(n < 0){				
			printf("read fifo error");
			exit(1);
		}
		if(n > 0){
			if (write(STDOUT_FILENO, buf, n) != n){
				printf("write error");
				exit(1);
			}	
			succesful_rd = 1;
			break;
		}
	}

	if(succesful_rd == 0){
		printf("writer timeout!");
		exit(1);
	}

	else	{
		while ((n = read(main_fd, buf, BUFFSIZE)) > 0)
			if (write(STDOUT_FILENO, buf, n) != n){
				printf("write error");
				exit(1);
			}

		if (n < 0){
			printf("read error");
			exit(1);
		}

	}

	remove(pidname);
	exit(0);
}
