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
#include <string.h>
#include <zlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>

struct termios save_term;

int sock;
int pipe4Parent[2];
int pipe4Child[2];
int pid;
int port_fd = 0;
int log_fd = 0;
int compress_fd = 0;
int socket_ret = 0;
z_stream server;
z_stream client;

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
		fprintf(stderr, "Error: Unable to close pipe \n");
		exit(1);
	}

	if(close(pipe4Child[1]) == -1){
                fprintf(stderr, "Error: Unable to close pipe \n");
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
	close(sock);
	close(log_fd);
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

void setup(int argc, char *argv[], int *sol) {
        struct option long_options[] = {
                {"port", required_argument, NULL, 'p'},
                {"log", required_argument, NULL, 'l'},
                {"compress", no_argument, NULL, 'c'},
                {0, 0, 0, 0}
        };	
        log_fd = -1; 
        while ((sol[0] = getopt_long(argc, argv, "p:l:c", long_options, NULL)) != -1) {
                switch (sol[0]) {
                        case 'p':
                                sol[1] = atoi(optarg);
                                sol[2] = 1;
                                break;

                        case 'l':
                                sol[3] = 1;
				log_fd = creat(optarg, 0666);
                                if (log_fd == -1) {
                                        fprintf(stderr, "Error: Unable to log file.\n");
                                }
                                break;

                        case 'c':
                                sol[4] = 1;
                                client.zalloc = Z_NULL;
                                client.zfree = Z_NULL;
                                client.opaque = Z_NULL;
                                server.zalloc = Z_NULL;
                                server.zfree = Z_NULL;
                                server.opaque = Z_NULL;
	
				int error2 = inflateInit(&client);				

                                if (error2 != Z_OK) {
                                        fprintf(stderr, "Error: Unable to compress from client side.\n");
                                        exit(1);
                                }
			
				error2 = deflateInit(&server, Z_DEFAULT_COMPRESSION);
                                if (error2 != Z_OK) {
                                        fprintf(stderr, "Error: Unable to decompress from  client side.\n");
                                        exit(1);
                                }
                                break;
			case '?':
                        default:
                                fprintf(stderr, "Error: Unrecognized Argument: Correct Usage: --port [arg] --log [arg] --compress.\n");
                                exit(1);
                                break;
                }
        }
        if (!sol[2]) {
                fprintf(stderr, "Error: Must specify a port using --port.\n");
                exit(1);
        }

}
/*int compression( int compress, int log, int num, short fd, int determine, char input[]) {
       	
        //int i;
        int sent = strlen("SENT ");
        int bytes = strlen(" bytes: ");
        int received = strlen("RECEIVED ");
        char newline = '\n';
	int buff_size = determine ? 1024 : 256; 
	int writeto = determine ? STDOUT_FILENO : sock; 
	fd = fd - 1;	
	if (compress) {
		char compression_buf[buff_size];
		if (determine) {
			client.avail_in = num;
			client.next_in = (unsigned char *) input;
			client.avail_out = buff_size;
			client.next_out = (unsigned char *) compression_buf;
			do {
				inflate(&client, Z_SYNC_FLUSH);
			} while (client.avail_in > 0);			
		} else {
			server.avail_in = num;
			server.next_in = (unsigned char *) input;
			server.avail_out = buff_size;
			server.next_out = (unsigned char *) compression_buf; 
			do {
				deflate(&server, Z_SYNC_FLUSH);
			} while (server.avail_in > 0);
		}
		write(writeto, compression_buf, buff_size - server.avail_out);
		if( !determine) {
			if (log) {
				char num_bytes[20];
				sprintf(num_bytes, "%d", buff_size - server.avail_out);
				write(log_fd, "SENT ", sent);
				write(log_fd, num_bytes, strlen(num_bytes));
				write(log_fd, " bytes: ", bytes);
				write(log_fd, compression_buf, buff_size - server.avail_out);
				write(log_fd, &newline, 1);
			}
		}
	} else {
		write(writeto, input, num);
		if( !determine) {
			if (log) {
				char num_bytes[20];
				sprintf(num_bytes, "%d", num);
				write(log_fd, "SENT ", sent);
				write(log_fd, num_bytes, strlen(num_bytes));
				write(log_fd, " bytes: ", bytes);
				write(log_fd, input, num);
				write(log_fd, &newline, 1);
			}
			return 1;
		}
	}
	if (log) {
		char num_bytes[20];
		sprintf(num_bytes, "%d", num);
		write(log_fd, "RECEIVED ", received);
		write(log_fd, num_bytes, strlen(num_bytes));
		write(log_fd, " bytes: ", bytes);
		write(log_fd, input, num);
		write(log_fd, &newline, 1);
	}	
	return 1; 
}
*/
void logger(int compress, int sending, char* buff, int num, int buff_size) {
        char num_bytes[10];
        int sent = strlen("SENT ");
        int bytes = strlen(" bytes: ");
        int received = strlen("RECEIVED ");
        char newline = '\n';
        if (sending) {
                if (compress) {
                        sprintf(num_bytes, "%d", buff_size - server.avail_out);
                } else {
                        sprintf(num_bytes, "%d", num);
                }
                write(log_fd, "SENT ", sent);
                write(log_fd, num_bytes, strlen(num_bytes));
                write(log_fd, " bytes: ", bytes);
                if (compress) {
                        write(log_fd, buff, buff_size - server.avail_out);
                } else {
                        write(log_fd, buff, num);
                }
        } else {
                sprintf(num_bytes, "%d", num);
                write(log_fd, "RECEIVED ", received);
                write(log_fd, num_bytes, strlen(num_bytes));
                write(log_fd, " bytes: ", bytes);
                write(log_fd, buff, num);
        }
        write(log_fd, &newline, 1);

}
int compression2(int log, char buf[], int sock, int num, int buffer_size){
	if (buffer_size == 256) {
		char offset[256];
		server.avail_in = num;
		server.next_in = (unsigned char *)buf;
		server.avail_out = 256;
		server.next_out = (unsigned char *)offset;

		do
		{
			deflate(&server, Z_SYNC_FLUSH);
		} while (server.avail_in > 0);

		write(sock, offset, 256 - server.avail_out);

		if (log ){
			logger(1, 1, offset, 0, 256);
		}
	} else {
		char offset[1024];
		client.avail_in = num;
		client.next_in = (unsigned char *)buf;
		client.avail_out = 1024;
		client.next_out = (unsigned char *)offset;

		do
		{
			inflate(&client, Z_SYNC_FLUSH);
		} while (client.avail_in > 0);

		write(STDOUT_FILENO, offset, 1024 - client.avail_out);	
	}
	return 1;  	
}	

int main(int argc, char* argv[]) { 
	int sol[5] = {0, 0, 0, 0, 0}; 
	setup(argc, argv, sol); 		
	char crlf[2] = {'\015', '\012'};
	int port_number = sol[1]; 
		
	int log = sol[3];
	int compress = sol[4];
        struct termios mode;
        
        if (!isatty(STDIN_FILENO)) {
                fprintf(stderr, "Error: No reference to terminal.\n");
                exit(1);
        }
        
        tcgetattr(STDIN_FILENO, &save_term);
        atexit(saved);
        
        tcgetattr(STDIN_FILENO, &mode);
        
        mode.c_iflag = ISTRIP;
        mode.c_oflag = 0;
        mode.c_lflag = 0; 
        mode.c_cc[VMIN] = 1;
        mode.c_cc[VTIME] = 0;
        
        tcsetattr(STDIN_FILENO, TCSANOW, &mode);

        struct sockaddr_in server_address;
        struct hostent* server_found;

        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                fprintf(stderr, "Error: Socket Failed.\n");
                exit(1);
        }

        server_found = gethostbyname("localhost");
	if(server_found == NULL) {
		fprintf(stderr, "Error: Hostname not specified \n");
		tcsetattr(STDIN_FILENO, TCSANOW, &save_term);	
		exit(1);
	}
        memset( (char * ) &server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;

        bcopy( (char *) &server_address.sin_addr.s_addr, (char *) server_found->h_addr, server_found->h_length);

        server_address.sin_port = htons(port_number);

        if (connect(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
                fprintf(stderr, "Error:  Unable to connect from client side.\n");
		tcsetattr(STDIN_FILENO, TCSANOW, &save_term);
                exit(1);
        }


        struct pollfd fd[] = {
                {STDIN_FILENO, POLLIN, 0},
                {sock, POLLIN, 0}
        };

	int i;
	while (1) {
		if (poll(fd, 2, 0) > 0) {
			if (fd[0].revents == POLLIN) {
				char buf[256];
				int num = read(STDIN_FILENO, &buf, 256);
				i = 0;
				while (i < num) {
					switch(buf[i]) {
						case '\015':
						case '\012':
							write(STDOUT_FILENO, crlf, 2);
							break;
						case '\003':
							write(STDOUT_FILENO, "^C\r\n", 4);
							break;
						case '\004':
							write(STDOUT_FILENO, "^D\r\n", 4);
							break;
						default:
							write(STDOUT_FILENO, (buf + i), 1);
							break;
					}
					i++;
				}

				if (compress) {
					int successful = compression2(log, buf, sock, num, 256);
					if (successful == 0) 
						exit(1); 
				} else {
					write(sock, buf, num);
					if (log) {
						logger(compress, 1, buf, num, 256);
					}
				}
			} else if (fd[0].revents == POLLERR) {
				fprintf(stderr, "Error: Polling failed.\n");
				exit(1);
			}

			if (fd[1].revents == POLLIN) {
				char buf[256];
				int num = read(sock, &buf, 256);
				if (num == 0)
					break;
				
				if (compress) {	
				int successful = compression2(log, buf, sock, num, 1024);
                                if (successful == 0)
                                	exit(1);
				}else {
					write(STDOUT_FILENO, buf, num);
				}

				if (log) {
					logger(compress, 0, buf, num, 256);
				}
			} else if (fd[1].revents & POLLERR) {
				exit(0);
			} else if (fd[1].revents & POLLHUP) {
				exit(0);			
			}
		}
	}
        if (compress) {
                inflateEnd(&client);
                deflateEnd(&server);
        }

        exit(0);
}

