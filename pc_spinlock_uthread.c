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
int histogram [MAX_ITEMS+1] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // histogram [i] == # of times list stored i items
int items = 0;
spinlock_t data_lock;

void* producer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
		// Acquire lock
		spinlock_lock(&data_lock);
		
		// Increase items and update histogram if 
		if(items < MAX_ITEMS) {
			histogram[++items]++;
		} else {
			++producer_wait_count;
			--i;
		}

		// Release lock
		spinlock_unlock(&data_lock);
  }
  return NULL;
}

void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
		// Acquire lock
		spinlock_lock(&data_lock);
		
		
		// Increase items and update histogram if 
		if(items > 0) {
			histogram[--items]++;
		} else {
			++consumer_wait_count;
			--i;
		}
		
		// Release lock
		spinlock_unlock(&data_lock);
	}
  return NULL;
}

int main (int argc, char** argv) {

	spinlock_create(&data_lock);
			
	uthread_init(4);

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
