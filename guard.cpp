#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "string.h"

// yes, we can guard on strtok too because the guard prevent the calling of the strtok until the first thread done

pthread_mutex_t locker1 = PTHREAD_MUTEX_INITIALIZER;

class Guard {

    pthread_mutex_t *lock;

public:
    Guard(pthread_mutex_t* l) {
        lock = l;
        pthread_mutex_lock(this->lock);
    }
    ~Guard() { pthread_mutex_unlock(this->lock); }
};

int* num1;
void *funcc( void *ptr ){
    Guard g(&locker1);
    char* th=(char*)ptr;

    for (size_t i = 0; i < 10; i++)
    {
        int* j = (int*) malloc(sizeof(int));
        num1=j;
        printf("%s : %p \n",th, num1);
    }
    puts("");
    return nullptr;
}
int test_guard()
{
    pthread_t th1, th2;
    char th1Name[128];
    char th2Name[128];
    strcpy(th1Name,"th1");
    strcpy(th2Name,"th2");

    pthread_create( &th1, NULL, funcc, th1Name);
    pthread_create( &th2, NULL, funcc, th2Name);

    pthread_join( th1, NULL);
    pthread_join( th2, NULL);

    return 1;
}