#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define DBG_MSG "Hello World : I am multithread! I am multithread!"
#define NUM_PTR 20 

char *thread_ptr[NUM_PTR];
char *main_ptr[NUM_PTR];

void *thread_func(void *arg)
{
    for (int i = 0; i < NUM_PTR; ++i) {
        for (int j = 0; j < 1; ++j) {
            thread_ptr[i] = (char*)arg + i + j;
            if (thread_ptr[i] == NULL) {
                printf("Thread: pointer is NULL\n");
            } else {
                printf("Thread: %p %s\n", thread_ptr[i], thread_ptr[i]);
            }
        }
    }
    return NULL;
}

int main()
{
    char *msg = (char *)malloc(sizeof(char)*(strlen(DBG_MSG) + 1));
    strncpy(msg, DBG_MSG, (strlen(DBG_MSG)+1));

    pthread_t thread_id;
    if(pthread_create(&thread_id, NULL, thread_func, msg)) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    for (int i = 0; i < NUM_PTR; ++i) {
        main_ptr[i] = msg + i;
        if (main_ptr[i] == NULL) {
            printf("Main: pointer is NULL\n"); 
        } else {
             printf("Main: %s\n", main_ptr[i]);
        }
    }
    
    free(msg);
    printf("Main: memory freed\n");
    if(pthread_join(thread_id, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return 2;
    }

    printf("Main: Checking values for each ptr\n");
    for (int i = 0; i < NUM_PTR; ++i) {
        printf("Thread :%p Main :%p\n", thread_ptr[i], main_ptr[i]);
    }
    return 0;
}
