#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define     BUFFSIZE	1024
const char* ADDRFIFO = "/tmp/addrfifo";
char pidname[12];

void err_sys(const char* error){
	perror(error);
	exit(1);
}

int main(){
	int  n;
	char buf[BUFFSIZE];
	int  pid_int = getpid();

	
	sprintf(pidname, "%d", pid_int);

	if((mkfifo(ADDRFIFO, 0644) < 0) && (errno != EEXIST))
		err_sys("MKFIFO ERROR");

	int addr_fd = open(ADDRFIFO, O_WRONLY);
	
	if((addr_fd < 0) && (errno != EEXIST))
		err_sys("OPEN FIFO ERROR");
	
	if((mkfifo(pidname, 0644) < 0) && (errno != EEXIST))
		err_sys("MKFIFO ERROR");
	
	int main_fd = open(pidname, O_RDONLY | O_NONBLOCK);
	
	if((main_fd < 0) && (errno != EEXIST)){

		err_sys("OPEN FIFO ERROR");
	}

	if (write(addr_fd, &pid_int, sizeof(int)) != sizeof(int)){
			err_sys("WRITE ERROR");
	}
	close(addr_fd);
	int ec = 0;
	int succesful_rd = 0;


	while (1){
		if((n = read(main_fd, buf, BUFFSIZE)) == 0){
			//printf("!!!!!!");
			sleep(2);
		}
		if(n < 0 && errno != EAGAIN){				
			err_sys("FIFO READ ERROR, continue");
			break;
		} else {
			ec++;
		}
		if( ec > 15){
			//printf("EAGAIN ERROR\n");
			break;
		}
		if(n > 0){
			if (write(STDOUT_FILENO, buf, n) != n){

				err_sys("WRITE ERROR");
			}	
			while ((n = read(main_fd, buf, BUFFSIZE)) > 0)
			if (write(STDOUT_FILENO, buf, n) != n){

				err_sys("WRITE ERROR");
			}
			if (n == 0){
				
				break;
			}

		}
	}

	//remove(pidname);
	exit(0);
}
