// Gurbir Arora 
// gurbthebruin@g.ucla.edu
// 105178554

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sched.h>
#include <errno.h>
#include <pthread.h>
#include <getopt.h>

long long counter;
long threads;
long iterations;
int opt_yield;
char sync2 = 0;
int lock = 0;
pthread_mutex_t mutex; 

void add(long long *pointer, long long value) {
	long long sum = *pointer + value;
        if (opt_yield)
                sched_yield();
	*pointer = sum;
}

void printer(long revs, long speed, long avg_time) {
	switch (opt_yield) {
		case 1: 
			switch (sync2) {	
				case 'm':
					fprintf(stdout, "add-yield-m,%ld,%ld,%ld,%ld,%ld,%lld\n", threads, iterations, revs, speed, avg_time, counter);
					break;
				case 's':
					fprintf(stdout, "add-yield-s,%ld,%ld,%ld,%ld,%ld,%lld\n", threads, iterations, revs, speed, avg_time, counter);
					break;
				case 'c': 
					fprintf(stdout, "add-yield-c,%ld,%ld,%ld,%ld,%ld,%lld\n", threads, iterations, revs, speed, avg_time, counter);
					break;
				case 0: 
					fprintf(stdout, "add-yield-none,%ld,%ld,%ld,%ld,%ld,%lld\n", threads, iterations, revs, speed, avg_time, counter);							    break;
			}
		break;			
		case 0: 
			switch (sync2) {
                                case 'm':
                                        fprintf(stdout, "add-m,%ld,%ld,%ld,%ld,%ld,%lld\n", threads, iterations, revs, speed, avg_time, counter);
					break;
                                case 's':
                                        fprintf(stdout, "add-s,%ld,%ld,%ld,%ld,%ld,%lld\n", threads, iterations, revs, speed, avg_time, counter);
					break;
                                case 'c':
                                        fprintf(stdout, "add-c,%ld,%ld,%ld,%ld,%ld,%lld\n", threads, iterations, revs, speed, avg_time, counter);
					break;
                                case 0:
				 	fprintf(stdout, "add-none,%ld,%ld,%ld,%ld,%ld,%lld\n", threads, iterations, revs, speed, avg_time, counter);
					break;
                        }; 
		break;
	}
}	
void* thread_add(void* arg) {
	long i = 0;	
	for (i = 0; i < iterations; i++) { 
		long long old = 0;
		switch (sync2) {
			case 'm':
				pthread_mutex_lock(&mutex);
				add(&counter, 1);	
				pthread_mutex_unlock(&mutex);
				
				pthread_mutex_lock(&mutex);
				add(&counter, -1);
				pthread_mutex_unlock(&mutex);
				break;
			case 's':
				while (__sync_lock_test_and_set(&lock, 1));
				add(&counter, 1);
				 __sync_lock_release(&lock);
				
				while (__sync_lock_test_and_set(&lock, 1));
				add(&counter, -1);	
				 __sync_lock_release(&lock);
				break;
			case 'c':
				do {
					old = counter;
					if (opt_yield) {
						sched_yield();
					}
				} while (__sync_val_compare_and_swap(&counter, old, old+1) != old);
				do {
					old = counter;
					if (opt_yield) {
						sched_yield();
					}
				} while (__sync_val_compare_and_swap(&counter, old, old-1) != old);
				break;
			default:
				add(&counter, 1);
				add(&counter, -1);
				break;
 		};
	}
	return arg;
}

int main(int argc, char* argv[]) {

	threads = 1;
	iterations = 1;
	opt_yield = 0;
	sync2 = 0;
	counter = 0;
	struct option long_options[] = {
		{"threads", required_argument, 0, 't'},
		{"iterations", required_argument, 0, 'i'},
		{"yield", no_argument, 0, 'y'},
		{"sync", required_argument, 0, 's'},
		{0, 0, 0, 0}
	};

	int opt;
	while ((opt = getopt_long(argc, argv, "", long_options, NULL)) != -1) {
		switch (opt) {
			case 't':
				threads = atoi(optarg);
				break;
			case 'i':
				iterations = atoi(optarg);
				break;
			case 'y':
				opt_yield = 1;
				break;
			case 's':
				sync2 = optarg[0];
				break;
			default:
				fprintf(stderr, "Error: Incorrect Arguments Specified: Correct Usage: --thread -t, --iterations -i, --yield -y, --sync -s \n");
				exit(1);
		}
	}

	struct timespec initial, closer;
	clock_gettime(CLOCK_MONOTONIC, &initial);

	pthread_t *thread_tags = malloc(threads * sizeof(pthread_t));
	if (!thread_tags) {
		fprintf(stderr, "Error: Malloc Failed\n");
		exit(1);
	}

	if (sync2 == 'm') {
		if (pthread_mutex_init(&mutex, NULL) != 0){
			fprintf(stderr, "Error: Mutex Failed\n");
			exit(1);
		}	
	}

	int i;
	for (i = 0; i < threads; i++) {
		if (pthread_create(&thread_tags[i], NULL, &thread_add, NULL) != 0) {
			fprintf(stderr, "Could not create threads\n");
			exit(1);
		}
	}

	for (i = 0; i < threads; i++) {
		pthread_join(thread_tags[i], NULL);
	}
	clock_gettime(CLOCK_MONOTONIC, &closer);

	long revs = threads * iterations * 2;
	long speed = 1000000000L * (closer.tv_sec - initial.tv_sec) + closer.tv_nsec - initial.tv_nsec;
	long avg_time = speed / revs;

	printer(revs, speed, avg_time); 
	if (sync2 == 'm')
	    pthread_mutex_destroy(&mutex);

	free(thread_tags);
	exit(0);
}
