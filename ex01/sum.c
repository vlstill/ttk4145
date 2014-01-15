// gcc 4.7.2 +
// gcc -std=gnu99 -Wall -g -o helloworld_c helloworld_c.c -lpthread

#include <pthread.h>
#include <stdio.h>
#include <stdint.h>

int i = 0;

// Note the return type: void*
void* adder(void* _step){
    int step = (int)((intptr_t)(_step));
    for(int x = 0; x < 1000000; x++){
        i += step;
    }
    return NULL;
}

int main(){
    pthread_t adder_thr;
    pthread_t subst_thr;
    pthread_create(&adder_thr, NULL, adder, (void*)(1) );
    pthread_create(&subst_thr, NULL, adder, (void*)(-1) );
    pthread_join(adder_thr, NULL);
    pthread_join(subst_thr, NULL);
    printf("%i\n", i);
    return 0;
}
