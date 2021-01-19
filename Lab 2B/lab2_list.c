// Gurbir Arora
// gurbthebruin@g.ucla.edu
// 105178554

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include "SortedList.h"
#include <limits.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

unsigned long iterations;
SortedList_t* headNode;
unsigned long lists;
char sync1;
SortedListElement_t* elements;
unsigned long threads;
int* lock;
pthread_mutex_t* mutex;

void m_handler(unsigned long lists, int opt) {
	unsigned long i = 0;
	if (!opt) {
            i = 0; 
	    mutex = malloc(sizeof(pthread_mutex_t) * lists);
            if (mutex == NULL) {
                  fprintf(stderr, "Error: Malloc failed\n");
                  exit(1);
            }
	    
            while (i < lists) {
                  if (pthread_mutex_init((mutex + i), NULL) != 0) {
                    fprintf(stderr, "Error: Mutex initialization failed\n");
                    exit(1);
                  }
		  i++; 
            }
	} else if (opt) {
  	    i = 0; 
	    while(i < lists){
                  pthread_mutex_destroy(mutex + i);
		  i++;
            }
            free(mutex);
	} 	
}

void s_handler(unsigned long lists, int opt) {
	if (!opt) {
            unsigned long i = 0; 
	    lock = malloc(sizeof(int) * lists);
            if (lock == NULL) {
                  fprintf(stderr, "Error: Malloc failed\n");
                  exit(1);
            }

            for (i = 0; i < lists; i++) {
                  lock[i] = 0;
            }
	} 
}


void printer(int opt_yield, char sync1) {
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

	if (!sync1)
		fprintf(stdout, "-none");
	else if (sync1 == 's')
		fprintf(stdout, "-s");
	else if (sync1 == 'm')
		fprintf(stdout, "-m");
}

void seg_fault_fc() {
        fprintf(stderr, "Error: Segmentation fault :( ");
        exit(2);
}

void* thread_needle(void *stuff) {
    long timer1 = 0;
    struct timespec initializer, closer;
    SortedListElement_t* array = stuff;
    unsigned long i = 0;
    while (i < iterations) {
	const char* map = (array+i) -> key; 
     int randomizer = 31;

     while (*map) {
            randomizer = 28 * randomizer ^ *map;
            map++;
      }

        unsigned int hash = randomizer % lists;
	switch (sync1) {
		case 'm':
			clock_gettime(CLOCK_MONOTONIC, &initializer);
			pthread_mutex_lock(mutex + hash);
			clock_gettime(CLOCK_MONOTONIC, &closer);
 		       	timer1 += 1000000000L * (closer.tv_sec - initializer.tv_sec) + closer.tv_nsec - initializer.tv_nsec;
        		SortedList_insert(headNode + hash, (SortedListElement_t *) (array+i));
			pthread_mutex_unlock(mutex + hash);
			break; 
		case 's':
			clock_gettime(CLOCK_MONOTONIC, &initializer);
			while (__sync_lock_test_and_set(lock + hash, 1)); 
			clock_gettime(CLOCK_MONOTONIC, &closer);
       			timer1 += 1000000000L * (closer.tv_sec - initializer.tv_sec) + closer.tv_nsec - initializer.tv_nsec;
        		SortedList_insert(headNode + hash, (SortedListElement_t *) (array+i));
			__sync_lock_release(lock + hash);	
			break; 
		default:  
			clock_gettime(CLOCK_MONOTONIC, &initializer);
			clock_gettime(CLOCK_MONOTONIC, &closer);
        		timer1 += 1000000000L * (closer.tv_sec - initializer.tv_sec) + closer.tv_nsec - initializer.tv_nsec;
        		SortedList_insert(headNode + hash, (SortedListElement_t *) (array+i));
			break; 
		}
		i++; 
    }

        unsigned long length2 = 0;
	i = 0;
        while (i < lists) {
	     switch (sync1) {
		case 'm': 
			clock_gettime(CLOCK_MONOTONIC, &initializer);
     		        pthread_mutex_lock(mutex + i);
                	clock_gettime(CLOCK_MONOTONIC, &closer);
                	timer1 += 1000000000L * (closer.tv_sec - initializer.tv_sec) + closer.tv_nsec - initializer.tv_nsec;
			length2 += SortedList_length(headNode + i);
			pthread_mutex_unlock(mutex + i);
			break;
		case 's': 
			while (__sync_lock_test_and_set(lock + i, 1));
			length2 += SortedList_length(headNode + i);
			__sync_lock_release(lock + i);
			break;
		default: 
			length2 += SortedList_length(headNode + i);
		}
	   i++;  
      }  
    if (length2 < iterations) {
        fprintf(stderr, "Error: failed to create list.\n");
        exit(2);
    }

    char *curr_key = malloc(sizeof(char)*256);

    SortedListElement_t *ptr;
    i = 0;

    while (i < iterations) {
	const char* map = (array+i) -> key;
     	int randomizer = 31;

     	while (*map) {
            randomizer = 28 * randomizer ^ *map;
            map++;
      	}

        unsigned int hash = randomizer % lists;
	int n = SortedList_delete(ptr);
	switch (sync1) {
		case 'm':
        		strcpy(curr_key, (array+i)->key);
			clock_gettime(CLOCK_MONOTONIC, &initializer);
			pthread_mutex_lock(mutex + hash);
			clock_gettime(CLOCK_MONOTONIC, &closer);
			timer1 += 1000000000L * (closer.tv_sec - initializer.tv_sec) + closer.tv_nsec - initializer.tv_nsec;
			ptr = SortedList_lookup(headNode + hash, curr_key);
			if (ptr == NULL) {
                    	    fprintf(stderr, "Error: Failed to search from list\n");
                	    exit(2);
		        }
        		n = SortedList_delete(ptr);
            		if (n != 0) {
                    	    fprintf(stderr, "Error: Failed to delete from list\n");
                	    exit(2);
            		}
			pthread_mutex_unlock(mutex + hash);					
			break;
		case 's':
                        strcpy(curr_key, (array+i)->key);
                        clock_gettime(CLOCK_MONOTONIC, &initializer);			
			while (__sync_lock_test_and_set(lock + hash, 1));
                        clock_gettime(CLOCK_MONOTONIC, &closer);
                        timer1 += 1000000000L * (closer.tv_sec - initializer.tv_sec) + closer.tv_nsec - initializer.tv_nsec;
                        ptr = SortedList_lookup(headNode + hash, curr_key);
                        if (ptr == NULL) {
                            fprintf(stderr, "Error: Failed to search from list\n");
                            exit(2);
                        }
                        n = SortedList_delete(ptr);
                        if (n != 0) {
                            fprintf(stderr, "Error: Failed to delete from list\n");
                            exit(2);
                        }
			 __sync_lock_release(lock + hash);
			break;
		default: 
                        strcpy(curr_key, (array+i)->key);
                        clock_gettime(CLOCK_MONOTONIC, &initializer);
			clock_gettime(CLOCK_MONOTONIC, &closer);
                        timer1 += 1000000000L * (closer.tv_sec - initializer.tv_sec) + closer.tv_nsec - initializer.tv_nsec;
                        ptr = SortedList_lookup(headNode + hash, curr_key);
                        if (ptr == NULL) {
                            fprintf(stderr, "Error: Failed to search from list\n");
                            exit(2);
                        }
                        n = SortedList_delete(ptr);
                        if (n != 0) {
                            fprintf(stderr, "Error: Failed to search from list\n");
                            exit(2);
                        }
			break;	
	}
	i++; 

    }

        return (void *) timer1;
}

void setup(int argc, char* argv[]) {
    struct option long_options[] = {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"yield", required_argument, 0, 'y'},
        {"sync", required_argument, 0, 's'},
        {"lists", required_argument, 0, 'l'},
        {0, 0, 0, 0}
    };
    int opt; 
        while ((opt = getopt_long(argc, argv, "t:i:y:s:l:", long_options, NULL)) != -1) {
                switch (opt) {
                        case 't':
                                threads = atoi(optarg);
                                break;
                        case 'i':
                                iterations = atoi(optarg);
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
                sync1 = optarg[0];
                break;
            case 'l':
                lists = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Error: Incorrect Arguments Specified: Correct Usage: --thread -t, --iterations -i, --yield -y, --sync -s, --lists -l \n");
                exit(1);
        }
    }

}

int main(int argc, char* argv[]) {

    signal(SIGSEGV, seg_fault_fc);
    threads = 1;
    iterations = 1;
    opt_yield = 0;
    sync1 = 0;
    lists = 1;

    setup(argc, argv);
    unsigned long i; 
    headNode = malloc(sizeof(SortedList_t) * lists);
    i = 0; 
    while (i < lists) {
        headNode[i].next = NULL;
        headNode[i].key = NULL;
        headNode[i].prev = NULL;
	i++;
    }

    unsigned long numCounter = threads * iterations;
    elements = malloc(sizeof(SortedListElement_t) * numCounter);

    if (!elements) {
        fprintf(stderr, "Error: Failed to malloc\n");
        exit(1);
    }

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

    if (sync1 == 'm') {
	m_handler(lists, 0);        
    } else if (sync1 == 's') {
        s_handler(lists, 0);   
    }

        pthread_t *threading = malloc(sizeof(pthread_t) * threads);
        if ((sync1 == 'm' && pthread_mutex_init(mutex, NULL) != 0) || !threading) {
                fprintf(stderr, "Error: Failed to create mutex\n");
                exit(1);
        }      
        struct timespec initializer, closer;
      clock_gettime(CLOCK_MONOTONIC, &initializer);
      for (i = 0; i < threads; i++) {
          if (pthread_create(&threading[i], NULL, &thread_needle, (void *) (elements + iterations * i))) {
              fprintf(stderr, "Error: Incorrect List Length\n");
	      exit(1);
          }
      }

      long counter4 = 0;
      void** delay = (void *) malloc(sizeof(void**));

      for (i = 0; i < threads; i++){
          pthread_join(threading[i], delay);
          counter4 += (long) *delay;
      }

      clock_gettime(CLOCK_MONOTONIC, &closer);

      long length2 = 0;
      for (i = 0; i < lists; i++) {
          length2 += SortedList_length(headNode + i);
      }

      if (length2) {
          fprintf(stderr, "Error: list length\n");
          exit(2);
      }
    printer(opt_yield, sync1);      
    long ops = threads * iterations * 3;
    long run_timer1 = 1000000000L * (closer.tv_sec - initializer.tv_sec) + closer.tv_nsec - initializer.tv_nsec;
    long timer23 = run_timer1 / ops;
    long list_count = lists;
    long wait_for_lock = counter4/ops;

    fprintf(stdout, ",%ld,%ld,%ld,%ld,%ld,%ld,%ld\n", threads, iterations, list_count, ops, run_timer1, timer23, wait_for_lock);

    free(elements);
    free(threading);

    if (sync1 == 'm') {
         m_handler(lists, 1);
     } else if (sync1 == 's') {
         free (lock);
     }

     free(delay);
    exit(0);
}

