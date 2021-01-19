// Gurbir Arora
// 105178554
// gurbthebruin@g.ucla.edu

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <termios.h>
#include <sys/wait.h>
#include <getopt.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

struct termios save_term;

int pipe4Parent[2];
int pipe4Child[2];
int pid;


void fork_success() {
	close(pipe4Parent[1]);
	close(STDIN_FILENO);
	if(dup2(pipe4Parent[0], STDIN_FILENO) == -1){ 
		exit(1);
	}
	
	close(pipe4Child[0]);
	close(STDOUT_FILENO);
	if(dup2(pipe4Child[1], STDOUT_FILENO) == -1){
		exit(1);
	}

	close(STDERR_FILENO); 
	if(dup2(pipe4Child[1], STDERR_FILENO) == -1){
		exit(1);
	}

	char file[] = "/bin/bash";
	char *contingent[2] = {file, NULL};

	if (execvp(file, contingent) == -1) {
		fprintf(stderr, "Error: Exec failed.\n");
		exit(1);
	}

}

void fork_parent() {
	if(close(pipe4Parent[0]) == -1){
		fprintf(stderr, "Parent pipe close failure \n");
		exit(1);
	}

	if(close(pipe4Child[1]) == -1){
                fprintf(stderr, "Child pipe close failure \n");
		exit(1);
	}

	struct pollfd fd[] = {
		{STDIN_FILENO, POLLIN, 0},
		{pipe4Child[0], POLLIN, 0}
	};
	
	char crlf[2] = {'\r', '\n'};
	int run = 0;

	while (!run) {
		
		int poll_val = poll(fd, 2, 0);
		if(poll_val == -1) {
			fprintf(stderr, "Error: Poll failed \n");
			exit(1);
		}

		if (fd[0].revents == -1) {
                         fprintf(stderr, "Error: Poll failed \n");
                         exit(1);
                }

		if (fd[1].revents & POLLERR || fd[1].revents & POLLHUP) {
                                run = 1;
                }

		if (poll_val > 0) {
			
			char buff[500];
			int index = 0; 
			int offset = 0;
			char lf;
			int counter = 0;
			int num = (fd[0].revents) ? read(STDIN_FILENO, &buff, 500) : read(pipe4Child[0], &buff, 500);  
			if (fd[0].revents) {
				for (index = 0; index < num; index++) {
				switch (buff[index]) {
					case '\r':
						lf = '\n';
                                                write(STDOUT_FILENO, crlf, 2);
                                                write(pipe4Parent[1], &lf, 1);
						break;
					case '\n':
						lf = '\n';
                                                write(STDOUT_FILENO, crlf, 2);
                                                write(pipe4Parent[1], &lf, 1);
                                                break;
					case '\3':
						kill(pid, SIGINT);
						break;
					case '\4':
						run = 1;
						break;
					default: 
						write(STDOUT_FILENO, (buff + index), 1);
                                                write(pipe4Parent[1], (buff + index), 1);
						break;
					}
				}

			} 

			if (fd[1].revents) {
				for (index = 0; index < num; index++) {
				    switch (buff[index]) {
                                        case '\n':
						write(STDOUT_FILENO, (buff + offset), counter);
                                        	write(STDOUT_FILENO, crlf, 2);
                                        	offset += counter + 1;
                                        	counter = 0;
                                        	continue;
                                        
                                        case '\4':
                                                run = 1;
                                                break;
                            
                                        }
				    counter++;
				}
				write(STDOUT_FILENO, (buff+offset), counter);

			}
		}
	    }

		close(pipe4Child[0]);
		close(pipe4Parent[1]);
		int status;
		
		if (waitpid(pid, &status, 0) < 0) {
			fprintf(stderr, "Error: Waitpid failed \n");
			exit(1);
		}
		if (WIFEXITED(status))
			fprintf(stderr, "Error: Shell now exiting with SIGNAL=%d and STATUS=%d \n", WIFSIGNALED(status), WEXITSTATUS(status));
		exit(0);

}

void saved(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &save_term);
}

void sig_handler() {
	close(pipe4Parent[1]);
	close(pipe4Child[0]);
	kill(pid, SIGINT);
	int status;
        if (waitpid(pid, &status, 0) < 0) {
        	fprintf(stderr, "Error: Waitpid failed \n");
                exit(1);
        }
        if (WIFEXITED(status))
        	fprintf(stderr, "Error: Shell now exiting with SIGNAL=%d and STATUS=%d \n", WIFSIGNALED(status), WEXITSTATUS(status));
	exit(0);
}



int main (int argc, char* argv[]) {
  	
	int shell = 0;
        int run = 1;
        int options = 0;

        struct option long_options[] = {
                {"shell", required_argument, 0, 's'},
                {0, 0, 0, 0}
        };

        while(run){

                options = getopt_long(argc, argv, "s", long_options, 0);

                if(options == -1)
                        run = 0;

                if(!run)
                        break;

                switch (options) {

                        case 's':
                                shell = 1;
                                break;
                        default:
                                fprintf(stderr, "Error: Urecognized argument \n Proper Usage: --shell -s option \n");
                                exit(1);
                                break;
                }
        }
 
	if (shell) {
                if (!isatty(STDIN_FILENO)) {
                        fprintf(stderr, "Error STDIN doesn't make reference to terminal");
                        exit(1);
                }

		int cp = pipe(pipe4Parent);
                int pp = pipe(pipe4Child);

                if (cp != 0 || pp !=0) {
                        fprintf(stderr, "Error: Pipe initialization failed. \n");
                        exit(1);
                }
        	
	
		struct termios restoration;

		tcgetattr(STDIN_FILENO, &save_term);
		atexit(saved);

		tcgetattr(STDIN_FILENO, &restoration);


		restoration.c_iflag = ISTRIP;
		restoration.c_oflag = 0;
		restoration.c_lflag = 0;
                restoration.c_cc[VMIN] = 1;
                restoration.c_cc[VTIME] = 0;


		tcsetattr(STDIN_FILENO, TCSANOW, &restoration);
		signal(SIGPIPE, sig_handler);	

        	pid = fork();

        	if (pid == 0) {
			fork_success();
        	} else if (pid > 0) {
			fork_parent();       
		} else {
		    fprintf(stderr, "Error: Couldn't fork \n");
		    exit(1);
		}
    }
	    
	    	struct termios restoration;

		if (!isatty(STDIN_FILENO)) {
			fprintf(stderr, "Error: No reference to terminal.\n");
			exit(1);
		}

		tcgetattr(STDIN_FILENO, &save_term);
		atexit(saved);

		tcgetattr(STDIN_FILENO, &restoration);
		restoration.c_iflag = ISTRIP;
		restoration.c_oflag = 0;
		restoration.c_lflag = 0;
		restoration.c_cc[VMIN] = 1;
		restoration.c_cc[VTIME] = 0;

	        tcsetattr(STDIN_FILENO, TCSANOW, &restoration);

	    char buff;

	    while (read(STDIN_FILENO, &buff, 25) > 0 && buff != '\4') {

		switch (buff) {
			
			case '\r':
				buff = '\r';
                        	write(STDOUT_FILENO, &buff, 1);
                    		buff = '\n';
                    		write(STDOUT_FILENO, &buff, 1);	
				break;
			case '\n':
                                buff = '\r';
                                write(STDOUT_FILENO, &buff, 1);
                                buff = '\n'; 
                                write(STDOUT_FILENO, &buff, 1); 
                                break;
			default: 
				write(STDOUT_FILENO, &buff, 1);

		}
	    }

	    exit(0);
	}

