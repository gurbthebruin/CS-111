// Gurbir Arora 
// gurbthebruin@g.ucla.edu
// 105178554

#include <stdio.h>
#include "SortedList.h"
#include <string.h>
#include <stdlib.h>
#include <sched.h>

int opt_yield = 0;

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
	if (!list)
		return;
	if (!element)
		return; 
	SortedList_t *temp = list->next; 

	while (temp != list && strcmp(element->key, temp->key) > 0) 
		temp = temp->next;
		
	if (opt_yield && INSERT_YIELD) 
		sched_yield();

	element->prev = temp->prev;
	element->next = temp;
	temp->prev->next = element;
	temp->prev = element;
	
}

int SortedList_delete(SortedListElement_t *element) {
	if (element == NULL) 
        	return 1;

	if (element->next->prev != element)
		return 1;
	if (element->prev->next != element)
		return 1; 

    	if (opt_yield & DELETE_YIELD)
        	sched_yield();

	element->prev->next = element->next;
	element->next->prev = element->prev; 

    	return 0;

}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
    	if (list == NULL) {
        	return NULL;
    	}

    	SortedListElement_t* temp = list->next;
	if (opt_yield & LOOKUP_YIELD) {
		sched_yield();
	}

    	while (temp != list) {
    		if (strcmp(key, temp->key) == 0) {
            		return temp;
        	} else if (strcmp(key, temp->key) < 0)
			break;
	temp = temp->next;
    	}
    	return NULL;
}

int SortedList_length(SortedList_t *list) {
	if (list == NULL) 
        	return -1;
    	
    	int index = 0;
    	SortedListElement_t* temp = list->next;

    	while (temp != NULL) {
        	if (opt_yield & LOOKUP_YIELD) 
          		sched_yield();
        	temp = temp->next;
        	index++;
    	}
    	return index;
}
