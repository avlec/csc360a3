#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items
int items = 0;

pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t data_convar = PTHREAD_COND_INITIALIZER;

void* producer (void* v) {
 	for (int i=0; i<NUM_ITERATIONS; i++) {
		// Acquire lock
		pthread_mutex_lock(&data_mutex);
		
		// While the producer can't access items
		while(items == MAX_ITEMS) {
			pthread_cond_wait(&data_convar, &data_mutex);
			++producer_wait_count;
		}
		// Increase items and update histogram if 
		if(items < MAX_ITEMS) {
			histogram[++items]++;
		} else {
			++producer_wait_count;
		}
		// Release lock
		pthread_cond_broadcast(&data_convar);	
  		pthread_mutex_unlock(&data_mutex);
	}
  	return NULL;
}

void* consumer (void* v) {
  	for (int i=0; i<NUM_ITERATIONS; i++) {
		// Acquire lock
		pthread_mutex_lock(&data_mutex);
		// While the consumer can't access items
		while(items == 0) {
			pthread_cond_wait(&data_convar, &data_mutex);
			++consumer_wait_count;
		}
		// Do the do.
		histogram[--items]++;
		// Release lock
		pthread_cond_broadcast(&data_convar);	
  		pthread_mutex_unlock(&data_mutex);
	}
  	return NULL;
}

int main (int argc, char** argv) {
	pthread_t threads[4];

	pthread_create(&threads[0], NULL, producer, NULL);
	pthread_create(&threads[1], NULL, producer, NULL);
	pthread_create(&threads[2], NULL, consumer, NULL);
	pthread_create(&threads[3], NULL, consumer, NULL);

	for (int i = 0; i < 4; ++i) {
		pthread_join(threads[i], NULL); 
	}

  printf ("producer_wait_count=%d\nconsumer_wait_count=%d\n", producer_wait_count, consumer_wait_count);
  printf ("items value histogram:\n");
  int sum=0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }
  assert (sum == sizeof (threads) / sizeof (pthread_t) * NUM_ITERATIONS);
}
