#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define     BUFFSIZE	2048
const char* ADDRFIFO = "/tmp/addrfifo";


int main(int argc, char* argv[])
{
	int  n;
	char buf[BUFFSIZE];
	
	int  pid_int;
	char mainfifo[12];

	if(argc != 2){
		printf("wrong input!");
		exit(1);	
	}
	
	int input_fd = open(argv[1], O_RDONLY);

	if((input_fd < 0) && (errno != EEXIST)){
		printf("wrong file");
		exit(1);	
	}

	if((mkfifo(ADDRFIFO, 0644) < 0) && (errno != EEXIST)){
		printf("makefifo error");
		exit(1);
	}

	int addr_fd = open(ADDRFIFO, O_RDWR);

	if((addr_fd < 0) && (errno != EEXIST)){
		printf("open fifo error");
		exit(1);
	}
	
	n = read(addr_fd, &pid_int, sizeof(int));
	if(n < 0){
		printf("read error");
		exit(1);
	}
	close(addr_fd);

	sprintf(mainfifo, "%d", pid_int);	

	if((mkfifo(mainfifo, 0644) < 0) && (errno != EEXIST))	{
		printf("makefifo error");
		exit(1);		
	}

	int helper = open(mainfifo, O_RDONLY | O_NONBLOCK);
	int main_fd = open(mainfifo, O_WRONLY);

	close(helper);	

	if((main_fd < 0) && (errno != EEXIST)){
		printf("open fifo error");
		exit(1);
	}
	
	n = read(input_fd, buf, BUFFSIZE);
	if (write(main_fd, buf, n) != n){	
		printf("write error");
		exit(1);
	}
	
	
	
	while ((n = read(input_fd, buf, BUFFSIZE)) > 0)
		if (write(main_fd, buf, n) != n){	
			printf("write error");
			exit(1);
		}

	if (n < 0){
		printf("read error");
		exit(1);
	}

	remove(mainfifo);
	exit(0);
}
