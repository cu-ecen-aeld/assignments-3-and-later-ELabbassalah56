#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

// Optional: use these functions to add debug or error prints to your application
// #define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading DEBUG: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)
#define ret return
#define N ((void *) 0)


void* threadfunc(void* thread_param)
{

    // wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    thread_data_t * thread_func_args = (thread_data_t *)thread_param;
    int returnCode;
    usleep(thread_func_args->wait_to_obtain_ms * 1000);
    returnCode = pthread_mutex_lock(thread_func_args->mtx);
    if(returnCode != 0)
    {
        ERROR_LOG("Failed to lock the mutex of thread ID %lu , returnCode : %d (%s)",pthread_self(),returnCode,strerror(returnCode));
        thread_func_args->thread_complete_success = false;
        ret thread_func_args;
    }

    usleep(thread_func_args->wait_to_release_ms * 1000);

    returnCode = pthread_mutex_unlock(thread_func_args->mtx);
    if(returnCode != 0)
    {
        ERROR_LOG("Failed to unlock the mutex of thread ID %lu , returnCode : %d (%s)",pthread_self(),returnCode,strerror(returnCode));
        thread_func_args->thread_complete_success = false;
        ret thread_func_args;
    }

    thread_func_args->thread_complete_success = true;
    ret thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    thread_data_t * ptrThreadData = malloc(sizeof(thread_data_t));
    if(!ptrThreadData)
    {
        ERROR_LOG("No suffecient memory avaliable to allocate the thread data is NULL");
        ret false; 
    }
    
    ptrThreadData->wait_to_obtain_ms = wait_to_obtain_ms;
    ptrThreadData->mtx = mutex;
    ptrThreadData->wait_to_release_ms = wait_to_release_ms;
    ptrThreadData->thread_complete_success = false;
    int returnCode = pthread_create(thread,N,threadfunc,ptrThreadData);
    if(returnCode != 0)
    {
        ERROR_LOG("Failed to create thread with return code: %d  (%s)",returnCode,strerror(returnCode));
        free(ptrThreadData);
        ret false;
    }
    DEBUG_LOG("Successful creating thread in Process ID: %d and thread ID: %lu",getpid(), (unsigned long)*thread);
    ret true;
}

