// gcc 4.7.2 +
// gcc -std=gnu99 -Wall -g -o helloworld_c helloworld_c.c -lpthread

#include <pthread.h>
#include <stdio.h>
#include <stdint.h>

int i = 0;

typedef struct Args {
    int step;
    pthread_mutex_t *mutex;
} Args;

// Note the return type: void*
void* adder(void* _args) {
    Args *args = (Args*)(_args);
    const int step = args->step;

    for (int x = 0; x < 1000000; x++) {
        pthread_mutex_lock( args->mutex );
        i += step;
        pthread_mutex_unlock( args->mutex );
    }
    return NULL;
}

int main() {
    pthread_t adder_thr;
    pthread_t subst_thr;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    Args add_args = { 1, &mutex };
    Args sub_args = { -1, &mutex };

    pthread_create(&adder_thr, NULL, adder, &add_args );
    pthread_create(&subst_thr, NULL, adder, &sub_args );

    pthread_join(adder_thr, NULL);
    pthread_join(subst_thr, NULL);
    printf("%i\n", i);
    return 0;
}
