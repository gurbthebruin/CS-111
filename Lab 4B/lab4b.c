// Gurbir Arora
// gurbthebruin@g.ucla.edu
// 105178554

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <mraa.h>
#include <sys/errno.h>
#include <signal.h>
#include <getopt.h>
#include <sys/types.h>
#include "fcntl.h"
#include <poll.h>
#include <sys/time.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>

char scale = 'F';
int period = 1;
FILE *log_fd = 0;
int fd = 0;

void setup(int argc, char* argv[]) {
        struct option long_options[] = {
                {"period", required_argument, 0, 'p'},
                {"scale", required_argument, 0, 's'},
                {"log", required_argument, 0, 'l'},
                {0, 0, 0, 0}
        };

        int run = 1;

        while (run) {
                int opt = getopt_long(argc, argv, "p:s:l:", long_options, NULL);
                if (opt == -1)
                        run = 0;
                if (!run) 
                        break;
                
                switch (opt) {
                        case 'p': 
                                period = atoi(optarg);
                                if (period <= 0) {
                                        fprintf(stderr, "Error: Period out of bounds");
                                        exit(1);
                                }
                                break;
                        
                        case 's':
                                if (optarg[0] == 'F' || optarg[0] == 'f'){
                                        scale = 'F';
                                        break;
                                } else if (optarg[0] == 'C' || optarg[0] == 'c') {
                                        scale = 'C';
                                        break;
                                } else {
					exit(1);
				}
                                break;

                        case 'l':
                                log_fd = fopen(optarg, "w+");
                                if(log_fd == NULL) {
                                        fprintf(stderr, "Error: Failed to log\n");
                                        exit(1);
                                }
                                char *file_fd = optarg;
                                fd = creat(file_fd, 0666);
                                break;
                        default:
                                fprintf(stderr, "Error: Incorrect Usage: Available options: --period -p, --scale -s, and/or --log -l\n");
                                exit(1);
                                break;
                }
        }
}

void handle_button() {
	time_t get_time;
	time(&get_time);
	struct tm *real_time = localtime(&get_time);
	fprintf(stdout, "%.2d:%.2d:%.2d SHUTDOWN\n", real_time->tm_hour, real_time->tm_min, real_time->tm_sec);
	if (log_fd) 
		dprintf(fd, "%.2d:%.2d:%.2d SHUTDOWN\n", real_time->tm_hour, real_time->tm_min, real_time->tm_sec);
	exit(0);
}

int main(int argc, char* argv[]) {
	
	mraa_aio_context sensor;
	mraa_gpio_context button;
	struct pollfd poller[1];
	int counter = 1;
	struct tm *store_time;
	struct timeval clock_qt;
	time_t next_time = 0;
	char storage23[300];
	setup(argc, argv); 
	char *buffer; 
	
	sensor = mraa_aio_init(1);
        button = mraa_gpio_init(60);

	if (sensor == NULL) {
        	fprintf(stderr, "Error: Temperature Sensor Initialization Failed\n");
        	mraa_deinit();
        	exit(1);
    	}

    	if (button == NULL) {
        	fprintf(stderr, "Error: Button Initialization Failed\n");
        	mraa_deinit();
        	exit(1);
    	}

    	mraa_gpio_dir(button, MRAA_GPIO_IN);
	poller[0].fd = STDIN_FILENO; 
    	poller[0].events = POLLIN | POLLHUP | POLLERR; 

    	buffer = (char *)malloc(1024 * sizeof(char));
    	if(buffer == NULL) {
    		fprintf(stderr, "Error: Failed to malloc\n");
    		exit(1);
    	}
	float temp2 = 0.0; 
    	while(1) {
  		gettimeofday(&clock_qt, 0);
		if(mraa_gpio_read(button)) { 
			handle_button();
		}

		if (counter && clock_qt.tv_sec >= next_time) {
			int temp = mraa_aio_read(sensor);
			int B = 4275;
			float R0 = 100000.0;
			float R = 1023.0/((float) temp) - 1.0;
			R = R0*R;
			float celsius = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
			temp2 = celsius; 
			if (scale == 'f' || scale == 'F') 
				temp2 = (celsius * 9)/5 + 32;
			int t = temp2 * 10;
			store_time = localtime(&clock_qt.tv_sec);
			sprintf(storage23, "%02d:%02d:%02d %d.%1d", store_time->tm_hour, store_time->tm_min, store_time->tm_sec, t/10, t%10);
			
			fprintf(stdout, "%s\n", storage23);

			if (log_fd != 0) {
				fprintf(log_fd, "%s\n", storage23);
				fflush(log_fd);
			}
			next_time = clock_qt.tv_sec + period;
		}
    		int polling = poll(poller, 1, 0);
    		if (polling) {
    			fgets(buffer, 1024, stdin);
			buffer[strlen(buffer)-1] = '\0';
			while(*buffer == ' ' || *buffer == '\t') {
				buffer++;
			}
			char *period_begin = strstr(buffer, "PERIOD=");
			char *log_begin = strstr(buffer, "LOG");	
			if(!strcmp(buffer, "SCALE=F")) {
				dprintf(fd, "SCALE=F\n"); 
				scale = 'F'; 
			} else if(!strcmp(buffer, "SCALE=C")) {
				dprintf(fd, "SCALE=C\n");
				scale = 'C'; 
			} else if(!strcmp(buffer, "STOP")) {
				dprintf(fd, "STOP\n");
				counter = 0;
			} else if(!strcmp(buffer, "START")) {
				if(log_fd) { 
					fprintf(log_fd, "START\n");
					fflush(log_fd);
				}
				counter = 1;
			} else if(!strcmp(buffer, "OFF")) {
				if (log_fd) {
					fprintf(log_fd, "OFF\n");
					fflush(log_fd);
					store_time = localtime(&clock_qt.tv_sec);
					sprintf(storage23, "%02d:%02d:%02d SHUTDOWN", store_time->tm_hour, store_time->tm_min, store_time->tm_sec);
					fprintf(stdout, "%s\n", storage23);
				}
				if (log_fd != 0) {
					fprintf(log_fd, "%s\n", storage23);
					fflush(log_fd);
				}
				exit(0); 
			} else if (period_begin == buffer) {
				int period2 = atoi(buffer + 7);
				period = period2; 
 
				if(log_fd) {
					if (!counter) {
						dprintf(fd, "PERIOD=%d\n", period);
					}
				}

				if (log_fd != 0) {
					fprintf(log_fd, "%s\n", buffer);
					fflush(log_fd);
				} 

			} else if (log_begin == buffer) {
				if (log_fd) {
					fprintf(log_fd, "%s\n", buffer);
					fflush(log_fd);
				}
			} 

			}
		}

		mraa_aio_close(sensor);
		mraa_gpio_close(button);

		return 0;
}
