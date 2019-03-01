#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

int items = 0;

sem_t empty;
sem_t full;
sem_t use;

void* producer (void* v) {
	for (int i=0; i<NUM_ITERATIONS; i++) {
		
		sem_wait(&empty);		
		sem_wait(&use);
		histogram[++items]++;
		sem_post(&use);
		sem_post(&full);
			
	}
	return NULL;
}

void* consumer (void* v) {
	for (int i=0; i<NUM_ITERATIONS; i++) {
		  sem_wait(&full);
		  sem_wait(&use);
		  histogram[--items]++;
		  sem_post(&use);
		  sem_post(&empty);
	}
	return NULL;
}

int main (int argc, char** argv) {
  sem_init(&empty, 0, MAX_ITEMS);
  sem_init(&full, 0, 0);
  sem_init(&use, 0, 1);
  
  pthread_t threads[4];

  pthread_create(&threads[0], NULL, producer, NULL);
  pthread_create(&threads[1], NULL, producer, NULL);
  pthread_create(&threads[2], NULL, consumer, NULL);
  pthread_create(&threads[3], NULL, consumer, NULL);

  for(int i=0;i<4;++i) pthread_join(threads[i], NULL);
  
  printf ("items value histogram:\n");
  int sum=0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }
  assert (sum == sizeof (threads) / sizeof (pthread_t) * NUM_ITERATIONS);
}
