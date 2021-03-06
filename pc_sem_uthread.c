#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "uthread.h"
#include "uthread_sem.h"

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

int items = 0;

uthread_sem_t empty;
uthread_sem_t full;
uthread_sem_t use;

void* producer (void* v) {
	for (int i=0; i<NUM_ITERATIONS; i++) {
		
		uthread_sem_wait(empty);		

		uthread_sem_wait(use);
		histogram[++items]++;
		uthread_sem_signal(use);

		uthread_sem_signal(full);
			
	}
	return NULL;
}

void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {

		  uthread_sem_wait(full);

		  uthread_sem_wait(use);
		  histogram[--items]++;
		  uthread_sem_signal(use);

		  uthread_sem_signal(empty);

  }
  return NULL;
}

int main (int argc, char** argv) {
  uthread_init (4);

  empty = uthread_sem_create(MAX_ITEMS);
  full = uthread_sem_create(0);
  use = uthread_sem_create(1);

  uthread_t threads[4];

  threads[0] = uthread_create(producer, NULL);
  threads[1] = uthread_create(producer, NULL);
  threads[2] = uthread_create(consumer, NULL);
  threads[3] = uthread_create(consumer, NULL);

  for(int i=0;i<4;++i) uthread_join(threads[i], NULL);
  
  printf ("items value histogram:\n");
  int sum=0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }
  assert (sum == sizeof (threads) / sizeof (uthread_t) * NUM_ITERATIONS);
}
