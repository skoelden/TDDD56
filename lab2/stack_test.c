/*
 * stack_test.c
 *
 *  Created on: 18 Oct 2011
 *  Copyright 2011 Nicolas Melot
 *
 * This file is part of TDDD56.
 *
 *     TDDD56 is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     TDDD56 is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with TDDD56. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stddef.h>
#include <semaphore.h>

#include <unistd.h>

#include "stack.h"
#include "non_blocking.h"

#define test_run(test)                                                  \
    printf("[%s:%s:%i] Running test '%s'... ", __FILE__, __FUNCTION__, __LINE__, #test); \
    test_setup();                                                       \
    if(test())                                                          \
    {                                                                   \
        printf("passed\n");                                             \
    }                                                                   \
    else                                                                \
    {                                                                   \
        printf("failed\n");                                             \
    }                                                                   \
    test_teardown();

typedef int data_t;
#define DATA_SIZE sizeof(data_t)
#define DATA_VALUE 5

stack_t *stack;
data_t data;

#if MEASURE != 0
struct stack_measure_arg
{
    int id;
};
typedef struct stack_measure_arg stack_measure_arg_t;

struct timespec t_start[NB_THREADS], t_stop[NB_THREADS], start, stop;

#if MEASURE == 1
void* stack_measure_pop(void* arg)
{
    stack_measure_arg_t *args = (stack_measure_arg_t*) arg;
    int i;

    int dont_care;
    clock_gettime(CLOCK_MONOTONIC, &t_start[args->id]);
    for (i = 0; i < MAX_PUSH_POP / NB_THREADS; i++)
    {
        stack_pop(stack, &dont_care);
    }
    clock_gettime(CLOCK_MONOTONIC, &t_stop[args->id]);

    return NULL;
}
#elif MEASURE == 2
void* stack_measure_push(void* arg)
{
    stack_measure_arg_t *args = (stack_measure_arg_t*) arg;
    int i;

    clock_gettime(CLOCK_MONOTONIC, &t_start[args->id]);
    for (i = 0; i < MAX_PUSH_POP / NB_THREADS; i++)
    {
        stack_push(stack, args->id);
    }
    clock_gettime(CLOCK_MONOTONIC, &t_stop[args->id]);

    return NULL;
}
#endif
#endif

/* A bunch of optional (but useful if implemented) unit tests for your stack */
void test_init()
{
    // Initialize your test batch
}

void test_setup()
{
    stack = (stack_t*)malloc(sizeof(stack_t));
    stack_init(stack, MAX_PUSH_POP);
}

void test_teardown()
{
    stack_item_t* tmp;
    while(stack->unused != NULL){
        tmp = stack->unused->next;
        free(stack->unused);
        stack->unused = tmp;
    }
    while(stack->head != NULL){
        tmp = stack->head->next;
        free(stack->head);
        stack->head = tmp;
    }

    free(stack);
}

void test_finalize()
{
    // Destroy properly your test batch
}

void* thread_test_push_safe(void* arg)
{
    int i;
    for(i = 0; i < MAX_PUSH_POP/NB_THREADS; i++)
    {
        stack_push(stack, 1);
    }

    return NULL;
}

int test_push_safe()
{
    // Make sure your stack remains in a good state with expected content when
    // several threads push concurrently to it


    pthread_t thread[NB_THREADS];

    int i;
    for(i = 0; i < NB_THREADS; i++)
    {
        pthread_create(&thread[i], NULL, &thread_test_push_safe, NULL);
    }

    for (i = 0; i < NB_THREADS; i++)
    {
        pthread_join(thread[i], NULL);
    }

    // Push the rest
    int retval;
    do
    {
        retval = stack_push(stack, 1);
    }while(retval);


    // check if the stack is in a consistent state
    int stack_ok = 1;
    if(stack->unused != NULL)
    {
        stack_ok = 0;
    }

    int stack_length = 0;
    stack_item_t* tmp = stack->head;
    while(tmp != NULL)
    {
        stack_length++;
        tmp = tmp->next;
    }

    if(stack_length != 2*MAX_PUSH_POP)
    {
        stack_ok = 0;
    }

    // check other properties expected after a push operation
    // (this is to be updated as your stack design progresses)
    //assert(stack->change_this_member == 0);

    // For now, this test always fails
    return stack_ok;
}
void* thread_test_pop_safe(void* arg)
{

    int i;
    int value;
    for(i = 0; i < MAX_PUSH_POP/NB_THREADS; i++)
    {
        stack_pop(stack, &value);
    }

    return NULL;
}

int test_pop_safe()
{
    // Same as the test above for parallel pop operation


    pthread_t thread[NB_THREADS];

    int i;
    for(i = 0; i < NB_THREADS; i++)
    {
        pthread_create(&thread[i], NULL, &thread_test_pop_safe, NULL);
    }

    for (i = 0; i < NB_THREADS; i++)
    {
        pthread_join(thread[i], NULL);
    }

    // Pop the rest
    int value;
    int retval;

    do
    {
        retval = stack_pop(stack, &value);
    }while(retval);


    // check if the stack is in a consistent state
    int stack_ok = 1;
    if(stack->head != NULL)
    {
        stack_ok = 0;
    }

    int unused_length = 0;
    stack_item_t* tmp = stack->unused;
    while(tmp != NULL)
    {
        //printf("%d\n", tmp->value);
        unused_length++;
        tmp = tmp->next;
    }


    if(unused_length != 2*MAX_PUSH_POP)
    {
        stack_ok = 0;
    }

    // check other properties expected after a push operation
    // (this is to be updated as your stack design progresses)
    //assert(stack->change_this_member == 0);

    // For now, this test always fails
    return stack_ok;
}

// 3 Threads should be enough to raise and detect the ABA problem
#define ABA_NB_THREADS 3

void* thread_test_aba_1(void* arg)
{
    int value;
    stack_pop_force_aba_1(stack, &value);
    return NULL;
}

void* thread_test_aba_2(void* arg)
{
    sem_wait(&sem_aba_1);
    int value;
    stack_pop_force_aba_2(stack, &value);
    stack_push(stack, 1);
    sem_post(&sem_aba_4);
    return NULL;
}

void* thread_test_aba_3(void* arg)
{
    sem_wait(&sem_aba_2);
    int value;
    stack_pop(stack, &value);
    sem_post(&sem_aba_3);
    return NULL;
}

int test_aba()
{
#if NON_BLOCKING == 1 || NON_BLOCKING == 2


    sem_init(&sem_aba_1, 0, 0);
    sem_init(&sem_aba_2, 0, 0);
    sem_init(&sem_aba_3, 0, 0);
    sem_init(&sem_aba_4, 0, 0);
    int success, aba_detected = 0;

    pthread_t thread1;
    pthread_t thread2;
    pthread_t thread3;

    pthread_create(&thread1, NULL, &thread_test_aba_1, NULL);
    pthread_create(&thread2, NULL, &thread_test_aba_2, NULL);
    pthread_create(&thread3, NULL, &thread_test_aba_3, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);

    //printf("\n");
    //printf("Three first elements of stack: %d, %d, %d\n", stack->head,
    //       stack->head->next, stack->head->next->next);
    //printf("Three first elements of unused: %d, %d, %d\n", stack->unused, stack->unused->next,
    //    stack->unused->next->next);
    if(stack->head == stack->unused->next)
    {
        aba_detected = 1;
    }

    success = aba_detected;
    return success;
#else
    // No ABA is possible with lock-based synchronization. Let the test succeed only
    return 1;
#endif
}

// We test here the CAS function
struct thread_test_cas_args
{
    int id;
    size_t* counter;
    pthread_mutex_t *lock;
};
typedef struct thread_test_cas_args thread_test_cas_args_t;

void* thread_test_cas(void* arg)
{
#if NON_BLOCKING != 0
    thread_test_cas_args_t *args = (thread_test_cas_args_t*) arg;
    int i;
    size_t old, local;

    for (i = 0; i < MAX_PUSH_POP; i++)
    {
        do {
            old = *args->counter;
            local = old + 1;
#if NON_BLOCKING == 1
        } while (cas(args->counter, old, local) != old);
#elif NON_BLOCKING == 2
    } while (software_cas(args->counter, old, local, args->lock) != old);
#endif
}
#endif

return NULL;
}

// Make sure Compare-and-swap works as expected
int test_cas()
{
#if NON_BLOCKING == 1 || NON_BLOCKING == 2
    pthread_attr_t attr;
    pthread_t thread[NB_THREADS];
    thread_test_cas_args_t args[NB_THREADS];
    pthread_mutexattr_t mutex_attr;
    pthread_mutex_t lock;

    size_t counter;

    int i, success;

    counter = 0;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutex_init(&lock, &mutex_attr);

    for (i = 0; i < NB_THREADS; i++)
    {
        args[i].id = i;
        args[i].counter = &counter;
        args[i].lock = &lock;
        pthread_create(&thread[i], &attr, &thread_test_cas, (void*) &args[i]);
    }

    for (i = 0; i < NB_THREADS; i++)
    {
        pthread_join(thread[i], NULL);
    }

    success = counter == (size_t)(NB_THREADS * MAX_PUSH_POP);

    if (!success)
    {
        printf("Got %ti, expected %i. ", counter, NB_THREADS * MAX_PUSH_POP);
    }

    assert(success);
    return success;
#else
    return 1;
#endif
}

int main(int argc, char **argv)
{
    setbuf(stdout, NULL);
// MEASURE == 0 -> run unit tests
#if MEASURE == 0
    test_init();

    test_run(test_cas);

    test_run(test_push_safe);
    test_run(test_pop_safe);
    test_run(test_aba);

    test_finalize();
#else
    int i;
    pthread_t thread[NB_THREADS];
    pthread_attr_t attr;
    stack_measure_arg_t arg[NB_THREADS];

    test_setup();
    pthread_attr_init(&attr);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < NB_THREADS; i++)
    {
        arg[i].id = i;
#if MEASURE == 1
        pthread_create(&thread[i], &attr, stack_measure_pop, (void*)&arg[i]);
#else
        pthread_create(&thread[i], &attr, stack_measure_push, (void*)&arg[i]);
#endif
    }

    for (i = 0; i < NB_THREADS; i++)
    {
        pthread_join(thread[i], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC, &stop);

    test_teardown();

    // Print out results
    for (i = 0; i < NB_THREADS; i++)
    {
        printf("%i %i %li %i %li %i %li %i %li\n", i, (int) start.tv_sec,
               start.tv_nsec, (int) stop.tv_sec, stop.tv_nsec,
               (int) t_start[i].tv_sec, t_start[i].tv_nsec, (int) t_stop[i].tv_sec,
               t_stop[i].tv_nsec);
    }
#endif

    return 0;
}
