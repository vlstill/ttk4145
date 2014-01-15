// gcc 4.7.2 +
// gcc -std=gnu99 -Wall -g -o helloworld_c helloworld_c.c -lpthread

#include <pthread.h>
#include <stdio.h>
#include <stdint.h>

typedef struct params {
    int step;
    pthread_mutex_t* lock;
} params;

int i = 0;

// Note the return type: void*
void* adder(void* arg) {
    params* local = (params*) arg;
    for(int x = 0; x < 1000000; x++){
        pthread_mutex_lock(local->lock);
        i += local->step;
	pthread_mutex_unlock(local->lock);
    }
    return NULL;
}

int main() {
    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);

    pthread_t adder_thr;
    pthread_t subst_thr;
    params params1={1,&lock};
    params params2={-1,&lock};
 
    pthread_create(&adder_thr, NULL, adder, (void*) &params1 );
    pthread_create(&subst_thr, NULL, adder, (void*) &params2 );
    pthread_join(adder_thr, NULL);
    pthread_join(subst_thr, NULL);
    printf("%i\n", i);

    pthread_mutex_destroy(&lock);

    return 0;
}
