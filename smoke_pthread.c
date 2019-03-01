#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_ITERATIONS 50

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

struct Agent {
  pthread_mutex_t mutex;
  pthread_cond_t  match;
  pthread_cond_t  paper;
  pthread_cond_t  tobacco;
  pthread_cond_t  smoke;
};

struct Agent* createAgent() {
  struct Agent* agent = malloc (sizeof (struct Agent));
  pthread_mutex_init(&agent->mutex, NULL);
  pthread_cond_init(&agent->paper, NULL);
  pthread_cond_init(&agent->match, NULL);
  pthread_cond_init(&agent->tobacco, NULL);
  pthread_cond_init(&agent->smoke, NULL); 
  return agent;
}

/**
 * You might find these declarations helpful.
 *   Note that Resource enum had values 1, 2 and 4 so you can combine resources;
 *   e.g., having a MATCH and PAPER is the value MATCH | PAPER == 1 | 2 == 3
 */
enum Resource            {    MATCH = 1, PAPER = 2,   TOBACCO = 4};
char* resource_name [] = {"", "match",   "paper", "", "tobacco"};

int signal_count [5];  // # of times resource signalled
int smoke_count  [5];  // # of times smoker with resource smoked

int available_ingredients = 0;

/**
 * This is the agent procedure.  It is complete and you shouldn't change it in
 * any material way.  You can re-write it if you like, but be sure that all it does
 * is choose 2 random reasources, signal their condition variables, and then wait
 * wait for a smoker to smoke.
 */
void* agent (void* av) {
  struct Agent* a = av;
  static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};
  static const int matching_smoker[] = {TOBACCO,     PAPER,         MATCH};
  
  pthread_mutex_lock(&a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      int r = random() % 3;
      signal_count [matching_smoker [r]] ++;
      int c = choices [r];
      if (c & MATCH) {
        VERBOSE_PRINT ("match available\n");
        pthread_cond_signal(&a->match);
      }
      if (c & PAPER) {
        VERBOSE_PRINT ("paper available\n");
        pthread_cond_signal(&a->paper);
      }
      if (c & TOBACCO) {
        VERBOSE_PRINT ("tobacco available\n");
        pthread_cond_signal(&a->tobacco);
      }
      VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
      pthread_cond_wait(&a->smoke, &a->mutex);
    }
  pthread_mutex_unlock(&a->mutex);
  return NULL;
}

struct Informant {
	pthread_mutex_t * agent;
	pthread_cond_t * ingredient;
	int ingredient_type;
	pthread_cond_t * checkingredients;
};

struct Informant* createInformant(pthread_mutex_t* agent, pthread_cond_t* ingredient, int ingredient_type, pthread_cond_t* checkingredients) {
	struct Informant* i = (struct Informant*) malloc(sizeof(struct Informant));
	i->agent = agent;
	i->ingredient = ingredient;
	i->ingredient_type = ingredient_type;
	i->checkingredients = checkingredients;
	return i;
}

struct Informant** createInformants(struct Agent* a, pthread_cond_t* checkingredients) {
	struct Informant** is = (struct Informant**) malloc(sizeof(struct Informant*)*3);
	is[0] = createInformant(&a->mutex, &a->match, MATCH, checkingredients);
	is[1] = createInformant(&a->mutex, &a->paper, PAPER, checkingredients);
	is[2] = createInformant(&a->mutex, &a->tobacco, TOBACCO, checkingredients);
	return is;
}

void* informant(void *iv) {
	struct Informant* i = (struct Informant*)iv;
	pthread_mutex_lock(i->agent);
	for(;;) {
		pthread_cond_wait(i->ingredient, i->agent);
		available_ingredients |= i->ingredient_type;
		pthread_cond_broadcast(i->checkingredients);
	}
	pthread_mutex_unlock(i->agent);
}

struct Smoker {
	pthread_mutex_t* agent;
	pthread_cond_t* checkingredients;
	int required_ingredients;
	pthread_cond_t* smoke;
};

struct Smoker* createSmoker(pthread_mutex_t* agent, pthread_cond_t* checkingredients, int required_ingredients, pthread_cond_t* smoke) {
	struct Smoker* s = (struct Smoker*) malloc(sizeof(struct Smoker));
	s->agent = agent;
	s->checkingredients = checkingredients;
	s->required_ingredients = required_ingredients;
	s->smoke = smoke;
	return s;
}

struct Smoker** createSmokers(struct Agent* a, pthread_cond_t* checkingredients) {
	struct Smoker** ss = (struct Smoker**) malloc(sizeof(struct Smoker*)*3);
	ss[0] = createSmoker(&a->mutex, checkingredients, MATCH | PAPER,   &a->smoke);
	ss[1] = createSmoker(&a->mutex, checkingredients, MATCH | TOBACCO, &a->smoke);
	ss[2] = createSmoker(&a->mutex, checkingredients, PAPER | TOBACCO, &a->smoke);
	return ss;
}

void* smoker(void *sv) {
	struct Smoker* s = (struct Smoker*)sv;
	pthread_mutex_lock(s->agent);
	for(;;) {
		while(available_ingredients != s->required_ingredients)
			pthread_cond_wait(s->checkingredients, s->agent);
		available_ingredients = 0;
		if(~(s->required_ingredients) & MATCH)
			smoke_count[MATCH]++;
		if(~(s->required_ingredients) & PAPER)
			smoke_count[PAPER]++;
		if(~(s->required_ingredients) & TOBACCO)
			smoke_count[TOBACCO]++;
		pthread_cond_signal(s->smoke);
	}
	pthread_mutex_unlock(s->agent);
}

int main (int argc, char** argv) {
  struct Agent*  a = createAgent();
 
  pthread_cond_t checkingredients; 
  pthread_cond_init(&checkingredients, NULL);
  struct Smoker **    ss = createSmokers(a, &checkingredients);
  struct Informant ** is = createInformants(a, &checkingredients);

  pthread_t threads[7]; 

  pthread_create(&threads[0], NULL, smoker, ss[0]);
  pthread_create(&threads[1], NULL, smoker, ss[1]);
  pthread_create(&threads[2], NULL, smoker, ss[2]);
  pthread_create(&threads[3], NULL, informant, is[0]);
  pthread_create(&threads[4], NULL, informant, is[1]);
  pthread_create(&threads[5], NULL, informant, is[2]);

  pthread_create(&threads[6], NULL, agent, a);
  pthread_join(threads[6], NULL);
  
  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
}
