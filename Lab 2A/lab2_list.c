// Gurbir Arora 
// gurbthebruin@g.ucla.edu
// 105178554


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include "SortedList.h"

SortedList_t headNode;
SortedListElement_t* elements;

pthread_mutex_t mutex;

void printer(int opt_yield, char sync) {
 	fprintf(stdout, "list");
        if (opt_yield == 0)
                fprintf(stdout, "-none");
        else if (opt_yield == 1)
                fprintf(stdout, "-i");
        else if (opt_yield == 2)
                fprintf(stdout, "-d");
        else if (opt_yield == 3)
                fprintf(stdout, "-id");
        else if (opt_yield == 4)
                fprintf(stdout, "-l");
        else if (opt_yield == 5)
                fprintf(stdout, "-il");
        else if (opt_yield == 6)
                fprintf(stdout, "-dl");
        else if (opt_yield == 7)
                fprintf(stdout, "-idl");

        if (!sync)
                fprintf(stdout, "-none");
        else if (sync == 's')
                fprintf(stdout, "-s");
        else if (sync == 'm')
                fprintf(stdout, "-m");
}

void seg_fault_fc() {
	fprintf(stderr, "Error: Segmentation fault :( ");
	exit(2);
}

struct args {
	long iterations;
	char sync;
};

void m_sync_handler(long iterations, SortedListElement_t* sorted_list_arr) {
	pthread_mutex_lock(&mutex);
        long i;
        for (i = 0; i < iterations; i++) {
                SortedList_insert(&headNode, (SortedListElement_t *) (sorted_list_arr+i));
        }
        long list_length = SortedList_length(&headNode);

        if (list_length < iterations) {
                fprintf(stderr, "Error: list not large enough\n");
                exit(2);
        }

        char *curr_key = malloc(sizeof(char)*256);

        SortedListElement_t *ptr;

        for (i = 0; i < iterations; i++) {
                strcpy(curr_key, (sorted_list_arr+i)->key);
                ptr = SortedList_lookup(&headNode, curr_key);
                if (ptr == NULL) {
                        fprintf(stderr, "Error: Lookup Failed\n");
                        exit(2);
                }

                int n = SortedList_delete(ptr);
                if (n != 0) {
                        fprintf(stderr, "Error: Delete Failed\n");
                        exit(2);
                }
        }
        pthread_mutex_unlock(&mutex);	
}

void s_sync_handler(char sync, long iterations, SortedListElement_t* sorted_list_arr, int lock) {
	if (sync == 's')
		while (__sync_lock_test_and_set(&lock, 1)); 
        long i;
        for (i = 0; i < iterations; i++) {
                SortedList_insert(&headNode, (SortedListElement_t *) (sorted_list_arr+i));
        }
        long list_length = SortedList_length(&headNode);

        if (list_length < iterations) {
                fprintf(stderr, "Error: list not large enough\n");
                exit(2);
        }

        char *curr_key = malloc(sizeof(char)*256);

        SortedListElement_t *ptr;

        for (i = 0; i < iterations; i++) {
                strcpy(curr_key, (sorted_list_arr+i)->key);
                ptr = SortedList_lookup(&headNode, curr_key);
                if (ptr == NULL) {
                        fprintf(stderr, "Error: Lookup Failed\n");
                        exit(2);
                }

                int n = SortedList_delete(ptr);
                if (n != 0) {
                        fprintf(stderr, "Error: Lookup Failed\n");
                        exit(2);
                }
        }
	if (sync == 's')
  	      __sync_lock_release(&lock);
}

void* thread_needle(void *filler) {
	int lock = 0; 
	SortedListElement_t* sorted_list_arr = filler;

	long iterations = ((struct args*)filler)->iterations;
	char sync  = ((struct args*)filler)->sync;    	
	if (sync == 'm') {
 		m_sync_handler(iterations, sorted_list_arr);       	
    	} else if (sync == 's') {
        	s_sync_handler(sync, iterations, sorted_list_arr, lock);
    	}
 
   	return NULL;
}

int main(int argc, char* argv[]) {

	signal(SIGSEGV, seg_fault_fc);
	unsigned long numCounter = 0;
	unsigned long threads = 1;
	struct args *pass = (struct args *)malloc(sizeof(struct args));
	pass->iterations = 1;
	opt_yield = 0;
	pass->sync = 0;

	struct option options[] = {
		{"threads", required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield", required_argument, NULL, 'y'},
		{"sync", required_argument, NULL, 's'},
		{0, 0, 0, 0}
	};

	int opt;
	while ((opt = getopt_long(argc, argv, "", options, NULL)) != -1) {
		switch (opt) {
			case 't':
				threads = atoi(optarg);
				break;
			case 'i':
				pass->iterations = atoi(optarg);
				break;
			case 'y':
				;
				int index;
				index = 0; 
				int length; 
				length = strlen(optarg); 
				while (index < length) {
					switch (optarg[index]) {
						case 'i':
							opt_yield |= INSERT_YIELD; 	
							break; 
						case 'd':
							opt_yield |= DELETE_YIELD; 
							break; 
						case 'l':
							opt_yield |= LOOKUP_YIELD; 
							break; 
						default: 
							fprintf(stderr, "Error: Yield Unspecified");
							break; 
					};
					index++; 
				}
				break;
			case 's':
				pass->sync = optarg[0];
				break;
			default:
				fprintf(stderr, "Error: Incorrect Arguments Specified: Correct Usage: --thread -t, --iterations -i, --yield -y, --sync -s \n");
				exit(1);
		}
	}

	numCounter = threads * pass->iterations;
	
	elements = malloc(sizeof(SortedListElement_t) * numCounter);
        if (!elements) {
		fprintf(stderr, "Error: Failed to malloc\n");
		exit(1);
	}
	unsigned long i = 0;
	for (i = 0; i < numCounter; i++) {
		char *map = malloc(2 * sizeof(char));
		if (!map) {
			fprintf(stderr, "Error: Failed to malloc\n");
			exit(1); 
		}	
		map[0] = rand() % 25 + 'A';
		map[1] = '\0';
		elements[i].key = map;
	}

	pthread_t *thread_ids = malloc(sizeof(pthread_t) * threads);

	if ((pass->sync == 'm' && pthread_mutex_init(&mutex, NULL) != 0) || !thread_ids) {
      		fprintf(stderr, "Error: Failed to create mutex\n");
      		exit(1);
  	}
  	struct timespec initial, closing;
  	clock_gettime(CLOCK_MONOTONIC, &initial);

  	for (i = 0; i < threads; i++) {
  		if (pthread_create(&thread_ids[i], NULL, &thread_needle, (void *) (elements + pass->iterations * i)) != 0) {
  			fprintf(stderr, "Error: Failed to create thread\n");
  			exit(1);
  		}
  	}

  	for (i = 0; i < threads; i++) {
  		pthread_join(thread_ids[i], NULL);
  	}

  	clock_gettime(CLOCK_MONOTONIC, &closing);

  	if (SortedList_length(&headNode) != 0) {
  		fprintf(stderr, "Error: Incorrect List Length\n");
  		exit(2);
  	}
	
	printer(opt_yield, pass->sync);	  
	long operations = threads * pass->iterations * 3;
	long run_time = 1000000000L * (closing.tv_sec - initial.tv_sec) + closing.tv_nsec - initial.tv_nsec;
	long time_per_op = run_time / operations;
	long num_lists = 1;
	fprintf(stdout, ",%ld,%ld,%ld,%ld,%ld,%ld\n", threads, pass->iterations, num_lists, operations, run_time, time_per_op);

	free(elements);
	free(thread_ids);

	if (pass->sync == 'm') {
		pthread_mutex_destroy(&mutex);
	}

	exit(0);
}
