#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"
#include "spinlock.h"

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items
int items = 0;

uthread_mutex_t data_mutex;
uthread_cond_t  data_convar;

void* producer (void* v) {
 	for (int i=0; i<NUM_ITERATIONS; i++) {
		// Acquire lock
		uthread_mutex_lock(data_mutex);
		// While the producer can't access items
		while(items == MAX_ITEMS) {
			uthread_cond_wait(data_convar);
			++producer_wait_count;
		}
		// Increase items and update histogram if 
		histogram[++items]++;
		// Release lock
		uthread_cond_broadcast(data_convar);	
  		uthread_mutex_unlock(data_mutex);
	}
  	return NULL;
}

void* consumer (void* v) {
  	for (int i=0; i<NUM_ITERATIONS; i++) {
		// Acquire lock
		uthread_mutex_lock(data_mutex);
		// While the consumer can't access items
		while(items == 0) {
			uthread_cond_wait(data_convar);
			++consumer_wait_count;
		}
		// Do the do.
		histogram[--items]++;
		// Release lock
		uthread_cond_broadcast(data_convar);	
  		uthread_mutex_unlock(data_mutex);
	}
  	return NULL;
}

int main (int argc, char** argv) {
	uthread_init(4);

	data_mutex = uthread_mutex_create(&data_mutex);
	data_convar = uthread_cond_create(data_mutex);	

	uthread_t threads[4];

	threads[0] = uthread_create(producer, NULL);
	threads[1] = uthread_create(producer, NULL);
	threads[2] = uthread_create(consumer, NULL);
	threads[3] = uthread_create(consumer, NULL);

	for (int i = 0; i < 4; ++i) {
		uthread_join(threads[i], NULL); 
	}

	printf ("producer_wait_count=%d\nconsumer_wait_count=%d\n", producer_wait_count, consumer_wait_count);
  	printf ("items value histogram:\n");
  	int sum=0;
  	for (int i = 0; i <= MAX_ITEMS; i++) {
    	printf ("  items=%d, %d times\n", i, histogram [i]);
    	sum += histogram [i];
  	}
  	assert (sum == sizeof (threads) / sizeof (uthread_t) * NUM_ITERATIONS);
}
