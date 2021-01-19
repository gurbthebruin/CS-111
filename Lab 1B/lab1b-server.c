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
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <zlib.h>

struct termios save_term;
int pipe4Parent[2];
int pipe4Child[2];
int pid;
int sock_backup; 
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
		fprintf(stderr, "Error: Piping Failed \n");
		exit(1);
	}

	if(close(pipe4Child[1]) == -1){
                fprintf(stderr, "Error: Piping Failed \n");
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
			int counter = 0;
			int num = (fd[0].revents) ? read(STDIN_FILENO, &buff, 500) : read(pipe4Child[0], &buff, 500);  
			if (fd[0].revents) {
				for (index = 0; index < num; index++) {
					char lf = '0';
					switch (buff[index]) {
						case '\r':
							lf = '\n';
							write(pipe4Parent[1], &lf, 1);
							break;
						case '\n':
							lf = '\n';
							write(pipe4Parent[1], &lf, 1);
							break;
						case '\3':
							kill(pid, SIGINT);
							break;
						case '\4':
							run = 1;
							break;
						default:
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
void setup(int argc, char *argv[], int *sol) {

        struct option options[] = {
                {"shell", required_argument, NULL, 's'},
                {"port", required_argument, NULL, 'p'},
                {"compress", no_argument, NULL, 'c'},
                {0, 0, 0, 0}
        };

        while ( (sol[0] = getopt_long(argc, argv, "", options, NULL)) != -1) {
                switch (sol[0]) {
                        case 's':
                                sol[1] = 1;
                                break;
                        case 'p':
                                sol[2] = atoi(optarg);
                                sol[3] = 1;
                                break;

                        case 'c':
                                sol[4] = 1;

                                client.zalloc = Z_NULL;
                                client.zfree = Z_NULL;
                                client.opaque = Z_NULL;
                                server.zalloc = Z_NULL;
                                server.zfree = Z_NULL;
                                server.opaque = Z_NULL;

                                int error2 = inflateInit(&server);

                                if (error2 != Z_OK) {
                                        fprintf(stderr, "Error: Unable to compress from server side.\n");
                                        exit(1);
                                }

                                error2 = deflateInit(&client, Z_DEFAULT_COMPRESSION);
                                if (error2 != Z_OK) {
                                        fprintf(stderr, "Error: Unable to decompress from server side.\n");
                                        exit(1);
                                }
				break;

                        default:
				fprintf(stderr, "Error: Unrecognized Argument: Correct Usage: --shell [arg] --port [arg] --compress.\n");
                                exit(1);
                                break;
                }
        }	

}

void compression_handler(int compress, int count, char *buff, int j) {
	char crlf[2] = {'\015', '\012'};
	if (compress) {
		char compress_buff[256];
		client.avail_in = count;
		client.next_in = (unsigned char *) (buff + j);
		client.avail_out = 256;
		client.next_out = (unsigned char *) compress_buff;

		do {
			deflate(&client, Z_SYNC_FLUSH);
		} while (client.avail_in > 0); 
		write(sock_backup, compress_buff, 256 - client.avail_out);
		char compress_buff2[256];
		client.avail_in = 2;
		client.next_in = (unsigned char *) (crlf);
		client.avail_out = 256;
		client.next_out = (unsigned char *) compress_buff2;

		do {
			deflate(&client, Z_SYNC_FLUSH);
		} while (client.avail_in > 0);

		write(sock_backup, compress_buff2, 256 - client.avail_out);

	} else {
		write(sock_backup, (buff + j), count);
		write(sock_backup, crlf, 2);
	}

}

int main(int argc, char* argv[]) {

	int sol[5] = {0, 0, 0, 0, 0};
	setup(argc, argv, sol);
        int port_number = sol[2];
        int compress = sol[4];
        int port = sol[3];
        int shell = sol[1];
	int sock;  
        if (shell)
                shell = 1;
	if (!port) {
		fprintf(stderr, "Error: Must specify a port using --port.\n");
		exit(1);
	}  
        unsigned int c_size;
        struct sockaddr_in s_address, c_address;
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                fprintf(stderr, "Error: Socket failed on server side\n");
                exit(1);
        }

        bzero( (char *) &s_address, sizeof(s_address));
        s_address.sin_family = AF_INET;
        s_address.sin_addr.s_addr = INADDR_ANY;	
        if (!port_number) {
		fprintf(stderr, "Error: Port Number DNE\n");
                exit(1);
	}
	s_address.sin_port = htons(port_number);
        int binder = bind(sock, (struct sockaddr *) &s_address, sizeof(s_address)); 
	if (binder < 0) {
                fprintf(stderr, "Error: Bind failed\n");
                exit(1);
        }
        listen(sock, 6);
        c_size = sizeof(c_address);
        sock_backup = accept(sock, (struct sockaddr *) &c_address, &c_size);
	if (sock_backup < 0) {
                fprintf(stderr, "Error: Receiving Failed\n");
                exit(1);
        }
        if (pipe(pipe4Child) != 0) {
                fprintf(stderr, "Error: Piping failed\n");
                exit(1);
        }
        if (pipe(pipe4Parent) != 0) {
                fprintf(stderr, "Error: Piping failed\n");
                exit(1);
        }

        signal(SIGPIPE, sig_handler);

        pid = fork();

	if (pid == -1) {
                fprintf(stderr, "Error: Fork failed\n");
                exit(1);
        } else if (pid == 0) {
		fork_success();
        } else if (pid > 0) {

                close(pipe4Parent[0]);
                close(pipe4Child[1]);

                struct pollfd fd[] = {
                        {sock_backup, POLLIN, 0},
                        {pipe4Child[0], POLLIN, 0}
                };

                int run = 0;
                int i;

                int status;
                while (!run) {

                        if (waitpid (pid, &status, WNOHANG) != 0) {
                                close(sock);
                                close(sock_backup);
                                close(pipe4Child[0]);
                                close(pipe4Parent[1]);
                                fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WIFSIGNALED(status), WEXITSTATUS(status));
                                exit(0);
                        }

                        if (poll(fd, 2, 0) > 0) {
                                if (fd[0].revents == POLLIN) {
                                        char buff[256];
                                        int num = read(sock_backup, &buff, 256);

                                        if (compress) {
                                                char compress_buff[1024];
                                                server.avail_in = num;
                                                server.next_in = (unsigned char *) buff;
                                                server.avail_out = 1024;
                                                server.next_out = (unsigned char *) compress_buff;

                                                do {
                                                        inflate(&server, Z_SYNC_FLUSH);
                                                } while (server.avail_in > 0);

                                                for (i = 0; (unsigned int) i < 1024 - server.avail_out; i++) {
							char lf = '0';
							switch (compress_buff[i]) {
								case '\r':
									lf = '\n';
									write(pipe4Parent[1], &lf, 1);
									break;
								case '\n':
									lf = '\n';
									write(pipe4Parent[1], &lf, 1);
									break;
								case '\3':
									kill(pid, SIGINT);
									break;
								case '\4':
									run = 1;
									break;
								default:
									write(pipe4Parent[1], (compress_buff + i), 1);
									break;
								}
                                                }

                                        } else {	
                                                for (i = 0; i < num; i++) {
							char lf = '0';
							switch (buff[i]) {
                                                                case '\r':
                                                                        lf = '\n';
                                                                        write(pipe4Parent[1], &lf, 1);
                                                                        break;
                                                                case '\n':
                                                                        lf = '\n';
                                                                        write(pipe4Parent[1], &lf, 1);
                                                                        break;
                                                                case '\3':
                                                                        kill(pid, SIGINT);
                                                                        break;
                                                                case '\4':
                                                                        run = 1;
                                                                        break;
                                                                default:
                                                                        write(pipe4Parent[1], (buff + i), 1);
                                                                        break;
                                                                }	
                                                }
                                        }

                                } else if (fd[0].revents == POLLERR) {
                                        fprintf(stderr, "Error: Polling failed.\n");
                                        exit(1);
                                }

                                if (fd[1].revents == POLLIN) {
                                        char buff[256];
                                        int num = read(pipe4Child[0], &buff, 256);

                                        int count = 0;
                                        int j;
                                        for (i = 0, j = 0; i < num; i++) {
                                              	if (buff[i] == '\004') {
                                                        run = 1;

                                                } else if (buff[i] == '\012') {
							compression_handler(compress, count, buff, j); 
                                                        j += count + 1;
                                                        count = 0;
                                                        continue;
                                                }
                                                count++;
                                        }

                                        write(sock_backup, (buff+j), count);

                                } else if (fd[1].revents & POLLERR) {
                                        run = 1;
                                } else if (fd[1].revents & POLLHUP)
					run = 1;
                        }
                }

                close(sock);
                close(sock_backup);
                close(pipe4Child[0]);
                close(pipe4Parent[1]);
                int status2;
        	waitpid(pid, &status2, 0);
        	fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WIFSIGNALED(status2), WEXITSTATUS(status2));

        } 

        if (compress) {
                inflateEnd(&server);
                deflateEnd(&client);
        }
        exit(0);
}
                                        
